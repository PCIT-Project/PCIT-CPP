//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "../include/Context.h"

#include "./Tokenizer.h"
#include "./Parser.h"
#include "./SemanticAnalyzer.h"
#include "./ASGToLLVMIR.h"


#include <llvm_interface.h>


namespace pcit::panther{


	Context::Context(DiagnosticCallback diagnostic_callback, const Config& _config) : 
		callback(diagnostic_callback),
		config(_config),
		type_manager(core::Platform::Windows, core::Architecture::x86)
	{
		evo::debugAssert(this->config.maxNumErrors > 0, "Max num errors cannot be 0");
	}


	Context::~Context() {
		if(this->isMultiThreaded() && this->threadsRunning()){
			this->shutdownThreads();
		}
	}
	

	auto Context::optimalNumThreads() -> evo::uint {
		return std::thread::hardware_concurrency() - 1;
	}



	auto Context::threadsRunning() const -> bool {
		evo::debugAssert(this->isMultiThreaded(), "Context is not set to be multi-threaded");

		if(this->workers.empty()){ return false; }

		return this->shutting_down_threads.test() == false;
	}


	auto Context::startupThreads() -> void {
		evo::debugAssert(this->isMultiThreaded(), "Context is not set to be multi-threaded");
		evo::debugAssert(this->threadsRunning() == false, "Threads already running");

		this->workers.reserve(this->config.numThreads);

		static constexpr auto worker_controller_impl = [](std::stop_token stop_token, Worker& worker) -> void {
			while(stop_token.stop_requested() == false){
				worker.get_task();
			}

			worker.done();
		};
			

		for(size_t i = 0; i < this->config.numThreads; i+=1){
			Worker& new_worker = this->workers.emplace_back(this);

			auto worker_controller = [&new_worker](std::stop_token stop_token) -> void {
				worker_controller_impl(stop_token, new_worker);
			};

			new_worker.getThread() = std::jthread(worker_controller);
			this->num_threads_running += 1;
			new_worker.getThread().detach();
		}

		this->emitDebug("pcit::panther::Context started up threads");
	}

	auto Context::shutdownThreads() -> void {
		evo::debugAssert(this->isMultiThreaded(), "Context is not set to be multi-threaded");
		evo::debugAssert(this->workers.empty() == false, "Threads are not running");

		const bool already_shutting_down = this->shutting_down_threads.test_and_set();
		if(already_shutting_down){ return; }
		EVO_DEFER([&]() -> void { this->shutting_down_threads.clear(); } );

		if(this->workers.empty()){ return; }

		for(Worker& worker : workers){
			worker.getThread().request_stop();
		}

		while(this->num_threads_running != 0){}

		this->workers.clear();

		this->task_group_running = false;

		this->emitDebug("pcit::panther::Context shutdown threads");
	}


	auto Context::waitForAllTasks() -> void {
		while(this->multiple_task_stages_left){
			std::this_thread::yield();	
		}

		if(this->threadsRunning()){
			this->wait_for_all_current_tasks();
		}
	}




	auto Context::loadFiles(evo::ArrayProxy<fs::path> file_paths) -> void {
		evo::debugAssert(
			this->isSingleThreaded() || this->threadsRunning(),
			"Context is set to be multi-threaded, but threads are not running"
		);

		evo::debugAssert(this->task_group_running == false, "Task group already running");


		this->task_group_running = true;

		for(const fs::path& file_path : file_paths){
			this->tasks.emplace(std::make_unique<Task>(LoadFileTask(file_path)));
		}

		if(this->isSingleThreaded()){
			this->consume_tasks_single_threaded();
		}
	}


	auto Context::tokenizeLoadedFiles() -> void {
		evo::debugAssert(this->task_group_running == false, "Task group already running");

		{
			const auto lock_guard = std::lock_guard(this->src_manager_mutex);
			
			this->task_group_running = true;
			for(const Source::ID::Iterator source_id_iter : this->src_manager){
				this->tasks.emplace(std::make_unique<Task>(TokenizeFileTask(*source_id_iter)));
			}
		}

		if(this->isSingleThreaded()){
			this->consume_tasks_single_threaded();
		}
	}


	auto Context::parseLoadedFiles() -> void {
		evo::debugAssert(this->task_group_running == false, "Task group already running");

		{
			const auto lock_guard = std::lock_guard(this->src_manager_mutex);
			
			this->task_group_running = true;
			for(const Source::ID::Iterator source_id_iter : this->src_manager){
				this->tasks.emplace(std::make_unique<Task>(ParseFileTask(*source_id_iter)));
			}
		}

		if(this->isSingleThreaded()){
			this->consume_tasks_single_threaded();
		}
	}


	auto Context::semanticAnalysisLoadedFiles() -> void {
		evo::debugAssert(this->task_group_running == false, "Task group already running");

		this->multiple_task_stages_left = true;

		if(this->type_manager.primitivesInitialized() == false){
			this->type_manager.initPrimitives();
			this->init_intrinsics();
		}

		if(isIntrinsicLookupTableSetup() == false){
			setupIntrinsicLookupTable();
		}


		{ // global declarations
			const auto lock_guard = std::lock_guard(this->src_manager_mutex);

			this->task_group_running = true;
			for(const Source::ID::Iterator source_id_iter : this->src_manager){
				this->tasks.emplace(std::make_unique<Task>(SemaGlobalDeclsTask(*source_id_iter)));
			}

			if(this->isSingleThreaded()){
				this->consume_tasks_single_threaded();
			}else{
				this->wait_for_all_current_tasks();
			}
		}
		

		const std::string error_msg = this->comptime_executor.init();
		EVO_DEFER([&](){ this->comptime_executor.deinit(); });

		if(error_msg.empty() == false){
			this->emitFatal(
				Diagnostic::Code::LLLVMDataLayoutError,
				std::nullopt,
				Diagnostic::createFatalMessage(error_msg)
			);
			return;
		}


		{ // global stmts comptime
			const auto lock_guard = std::lock_guard(this->src_manager_mutex);

			this->task_group_running = true;
			for(const Source::ID::Iterator source_id_iter : this->src_manager){
				this->tasks.emplace(std::make_unique<Task>(SemaGlobalStmtsComptimeTask(*source_id_iter)));
			}
			
			if(this->isSingleThreaded()){
				this->consume_tasks_single_threaded();
			}else{
				this->wait_for_all_current_tasks();
			}
		}

		if(this->errored()){ return; }

		{ // global stmts runtime
			const auto lock_guard = std::lock_guard(this->src_manager_mutex);

			this->task_group_running = true;
			for(const Source::ID::Iterator source_id_iter : this->src_manager){
				this->tasks.emplace(std::make_unique<Task>(SemaGlobalStmtsRuntimeTask(*source_id_iter)));
			}

			this->multiple_task_stages_left = false;

			if(this->isSingleThreaded()){
				this->consume_tasks_single_threaded();
			}
		}
	}




	auto Context::lower_to_llvmir(
		llvmint::LLVMContext& llvm_context,
		llvmint::Module& module,
		bool add_runtime,
		bool use_readable_registers
	)-> bool {
		auto asg_to_llvmir = ASGToLLVMIR(*this, llvm_context, module, ASGToLLVMIR::Config(use_readable_registers));
		asg_to_llvmir.lower();

		if(add_runtime){
			if(this->entry.has_value() == false){
				this->emitError(
					Diagnostic::Code::MiscNoEntrySet,
					std::nullopt,
					"No entry function was declared"
				);
				return false;
			}

			asg_to_llvmir.addRuntime();
		}

		return true;
	}


	auto Context::printLLVMIR(bool add_runtime) -> evo::Result<std::string> {
		auto llvm_context = llvmint::LLVMContext();
		llvm_context.init();

		const evo::Result<std::string> printed_llvm_ir = [&](){
			auto module = llvmint::Module();
			module.init("testing", llvm_context);
			EVO_DEFER([&](){ module.deinit(); });

			const std::string target_triple = module.getDefaultTargetTriple();

			const std::string data_layout_error = module.setDataLayout(
				target_triple,
				llvmint::Module::Relocation::Default,
				llvmint::Module::CodeSize::Default,
				llvmint::Module::OptLevel::None,
				false
			);

			if(!data_layout_error.empty()){
				this->emitFatal(
					Diagnostic::Code::LLLVMDataLayoutError,
					std::nullopt,
					Diagnostic::createFatalMessage(
						std::format("Failed to set data layout with message: {}", data_layout_error)
					)
				);
				return evo::Result<std::string>(evo::resultError);
			}

			module.setTargetTriple(target_triple);


			if(this->lower_to_llvmir(llvm_context, module, add_runtime, true) == false){
				return evo::Result<std::string>(evo::resultError);
			}

			return evo::Result<std::string>(module.print());
		}();

		llvm_context.deinit();

		return printed_llvm_ir;
	}



	auto Context::run() -> evo::Result<uint8_t> {
		auto llvm_context = llvmint::LLVMContext();
		llvm_context.init();

		const evo::Result<uint8_t> result = [&](){
			auto module = llvmint::Module();
			module.init("testing", llvm_context);
			EVO_DEFER([&](){ module.deinit(); });

			const std::string target_triple = module.getDefaultTargetTriple();

			const std::string data_layout_error = module.setDataLayout(
				target_triple,
				llvmint::Module::Relocation::Default,
				llvmint::Module::CodeSize::Default,
				llvmint::Module::OptLevel::None,
				true
			);

			if(!data_layout_error.empty()){
				this->emitFatal(
					Diagnostic::Code::LLLVMDataLayoutError,
					std::nullopt,
					Diagnostic::createFatalMessage(
						std::format("Failed to set data layout with message: {}", data_layout_error)
					)
				);
				return evo::Result<uint8_t>(evo::resultError);
			}

			module.setTargetTriple(target_triple);


			if(this->lower_to_llvmir(llvm_context, module, true, false) == false){
				return evo::Result<uint8_t>(evo::resultError);
			}

			return evo::Result<uint8_t>(module.run<uint8_t>("main"));
		}();

		llvm_context.deinit();

		return result;
	}


	auto Context::lookupSourceID(const std::string_view path) -> evo::Result<Source::ID> {
		for(const Source::ID source_id : this->src_manager){
			const Source& source = this->src_manager[source_id];

			if(source.getLocationAsString() == path){
				return source_id;
			}
		}

		return evo::resultError;
	}



	auto Context::lookupRelativeSourceID(const fs::path& file_location, const std::string_view src_path)
    -> evo::Expected<Source::ID, LookupSourceIDError> {

		// check is valid path
		if(src_path.empty()){ return evo::Unexpected(LookupSourceIDError::EmptyPath); }

		fs::path relative_dir = file_location;
		relative_dir.remove_filename();

		// generate path
		const fs::path lookup_path = [&]() -> fs::path {
			if(src_path.starts_with("./")){
				return relative_dir / fs::path(src_path.substr(2));

			}else if(src_path.starts_with(".\\")){
				return relative_dir / fs::path(src_path.substr(3));

			// }else if(src_path.starts_with("/") || src_path.starts_with("\\")){
			// 	return relative_dir / fs::path(src_path.substr(1));

			}else if(src_path.starts_with("../") || src_path.starts_with("..\\")){
				return relative_dir / fs::path(src_path);

			}else{
				return this->config.basePath / fs::path(src_path);
			}
		}().lexically_normal();

		if(file_location == lookup_path){
			return evo::Unexpected(LookupSourceIDError::SameAsCaller);
		}

		// look for path
		for(const Source::ID source_id : this->src_manager){
			const Source& source = this->src_manager[source_id];

			if(source.getLocationAsString() == lookup_path){
				return source_id;
			}
		}


		if(evo::fs::exists(lookup_path.string())){
			return evo::Unexpected(LookupSourceIDError::NotOneOfSources);			
		}

		return evo::Unexpected(LookupSourceIDError::DoesntExist);
	}


	auto Context::setEntry(const ASG::Func::LinkID& entry_id) -> bool {
		const auto lock = std::unique_lock(this->entry_mutex);

		if(this->entry.has_value()){ return false; }
		this->entry = entry_id;

		this->emitTrace("Set entry function (source id: {})", entry_id.sourceID().get());

		return true;
	}

	auto Context::getEntry() const -> std::optional<ASG::Func::LinkID> {
		const auto lock = std::shared_lock(this->entry_mutex);

		return this->entry;
	}



	auto Context::getIntrinsic(Intrinsic::Kind kind) const -> const Intrinsic& {
		evo::debugAssert(kind != Intrinsic::Kind::_max_, "Intrinsic::Kind::_max_ is not a valid intrinsic to lookup");

		return this->intrinsics[size_t(kind)];
	}

	auto Context::getTemplatedIntrinsic(TemplatedIntrinsic::Kind kind) const -> const TemplatedIntrinsic& {
		evo::debugAssert(
			kind != TemplatedIntrinsic::Kind::_max_,
			"TemplatedIntrinsic::Kind::_max_ is not a valid templated intrinsic to lookup"
		);

		return this->templated_intrinsics[size_t(kind)];
	}



	auto Context::wait_for_all_current_tasks() -> void {
		evo::debugAssert(this->isMultiThreaded(), "Context is not set to be multi-threaded");
		evo::debugAssert(this->threadsRunning(), "Threads are not running");
		evo::debugAssert(
			this->hasHitFailCondition() == false, "Context hit a fail condition, threads should be shutdown instead"
		);

		if(this->shutting_down_threads.test()){ return; }

		while(this->tasks.empty() == false){
			std::this_thread::yield();
		}

		while(true){
			bool all_done = true;

			for(const Worker& worker : this->workers){
				if(worker.isWorking()){
					all_done = false;
					break;
				}
			}

			if(all_done){
				break;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(32));
		}

		this->task_group_running = false;
	}



	auto Context::emit_diagnostic_impl(const Diagnostic& diagnostic) -> void {
		const auto lock_guard = std::lock_guard(this->callback_mutex);

		this->callback(*this, diagnostic);
	}


	auto Context::notify_task_errored() -> void {
		if(this->num_errors >= this->config.maxNumErrors){
			this->hit_fail_condition = true;
		}

		if(this->hasHitFailCondition() && this->isMultiThreaded()){
			// in a separate thread to prevent process from dead-locking when waiting for threads
			//  	(the worker thread that calls this function is never able to call its `done` function)
			std::thread([&]() -> void {
				this->shutdownThreads();
			}).detach();
		}
	}


	auto Context::consume_tasks_single_threaded() -> void {
		evo::debugAssert(this->isSingleThreaded(), "Context is not set to be single threaded");

		auto worker = Worker(this);

		while(this->tasks.empty() == false && this->hasHitFailCondition() == false){
			worker.get_task_single_threaded();
		}

		this->task_group_running = false;
	}



	auto Context::init_intrinsics() -> void {
		auto create_intrinsic = [&](
			evo::SmallVector<BaseType::Function::Param>&& params,
			evo::SmallVector<BaseType::Function::ReturnParam>&& return_params
		) -> Intrinsic {
			return Intrinsic(
				this->type_manager.getOrCreateFunction(
					BaseType::Function(std::move(params), std::move(return_params), false)
				)
			);
		};



		///////////////////////////////////
		// non-templated

		this->intrinsics[size_t(Intrinsic::Kind::Breakpoint)] = create_intrinsic(
			evo::SmallVector<BaseType::Function::Param>(),
			evo::SmallVector<BaseType::Function::ReturnParam>{
				BaseType::Function::ReturnParam(std::nullopt, TypeInfo::VoidableID::Void())
			}
		);

		this->intrinsics[size_t(Intrinsic::Kind::_printHelloWorld)] = create_intrinsic(
			evo::SmallVector<BaseType::Function::Param>(),
			evo::SmallVector<BaseType::Function::ReturnParam>{
				BaseType::Function::ReturnParam(std::nullopt, TypeInfo::VoidableID::Void())
			}
		);


		///////////////////////////////////
		// templated

		this->templated_intrinsics[size_t(TemplatedIntrinsic::Kind::IsSameType)] = TemplatedIntrinsic(
			evo::SmallVector<std::optional<TypeInfo::ID>>{std::nullopt, std::nullopt},
			evo::SmallVector<TemplatedIntrinsic::Param>(),
			evo::SmallVector<TemplatedIntrinsic::ReturnParam>{TypeManager::getTypeBool()}
		);

		this->templated_intrinsics[size_t(TemplatedIntrinsic::Kind::IsTriviallyCopyable)] = TemplatedIntrinsic(
			evo::SmallVector<std::optional<TypeInfo::ID>>{std::nullopt},
			evo::SmallVector<TemplatedIntrinsic::Param>(),
			evo::SmallVector<TemplatedIntrinsic::ReturnParam>{TypeManager::getTypeBool()}
		);

		this->templated_intrinsics[size_t(TemplatedIntrinsic::Kind::IsTriviallyDestructable)] = TemplatedIntrinsic(
			evo::SmallVector<std::optional<TypeInfo::ID>>{std::nullopt},
			evo::SmallVector<TemplatedIntrinsic::Param>(),
			evo::SmallVector<TemplatedIntrinsic::ReturnParam>{TypeManager::getTypeBool()}
		);

		this->templated_intrinsics[size_t(TemplatedIntrinsic::Kind::IsPrimitive)] = TemplatedIntrinsic(
			evo::SmallVector<std::optional<TypeInfo::ID>>{std::nullopt},
			evo::SmallVector<TemplatedIntrinsic::Param>(),
			evo::SmallVector<TemplatedIntrinsic::ReturnParam>{TypeManager::getTypeBool()}
		);

		this->templated_intrinsics[size_t(TemplatedIntrinsic::Kind::IsIntegral)] = TemplatedIntrinsic(
			evo::SmallVector<std::optional<TypeInfo::ID>>{std::nullopt},
			evo::SmallVector<TemplatedIntrinsic::Param>(),
			evo::SmallVector<TemplatedIntrinsic::ReturnParam>{TypeManager::getTypeBool()}
		);

		this->templated_intrinsics[size_t(TemplatedIntrinsic::Kind::IsFloatingPoint)] = TemplatedIntrinsic(
			evo::SmallVector<std::optional<TypeInfo::ID>>{std::nullopt},
			evo::SmallVector<TemplatedIntrinsic::Param>(),
			evo::SmallVector<TemplatedIntrinsic::ReturnParam>{TypeManager::getTypeBool()}
		);



		this->templated_intrinsics[size_t(TemplatedIntrinsic::Kind::SizeOf)] = TemplatedIntrinsic(
			evo::SmallVector<std::optional<TypeInfo::ID>>{std::nullopt},
			evo::SmallVector<TemplatedIntrinsic::Param>(),
			evo::SmallVector<TemplatedIntrinsic::ReturnParam>{TypeManager::getTypeUSize()}
		);

		this->templated_intrinsics[size_t(TemplatedIntrinsic::Kind::GetTypeID)] = TemplatedIntrinsic(
			evo::SmallVector<std::optional<TypeInfo::ID>>{std::nullopt},
			evo::SmallVector<TemplatedIntrinsic::Param>(),
			evo::SmallVector<TemplatedIntrinsic::ReturnParam>{TypeManager::getTypeTypeID()}
		);

		this->templated_intrinsics[size_t(TemplatedIntrinsic::Kind::BitCast)] = TemplatedIntrinsic(
			evo::SmallVector<std::optional<TypeInfo::ID>>{std::nullopt, std::nullopt},
			evo::SmallVector<TemplatedIntrinsic::Param>{
				TemplatedIntrinsic::Param(strings::StringCode::Value, AST::FuncDecl::Param::Kind::Read, uint32_t(0)),
			},
			evo::SmallVector<TemplatedIntrinsic::ReturnParam>{uint32_t(1)}
		);
	}



	//////////////////////////////////////////////////////////////////////
	// Worker


	auto Context::Worker::done() -> void {
		this->is_working = false;
		this->context->num_threads_running -= 1;
	}


	auto Context::Worker::get_task() -> void {
		evo::debugAssert(this->context->isMultiThreaded(), "Context is not set to be multi-threaded");

		auto task = std::unique_ptr<Task>();

		{
			const auto lock_guard = std::lock_guard(this->context->tasks_mutex);

			if(this->context->tasks.empty() == false){
				task.reset(this->context->tasks.front().release());
				this->context->tasks.pop();
			}else{
				this->context->task_group_running = false;
			}
		}

		if(task == nullptr){
			this->is_working = false;
			std::this_thread::sleep_for(std::chrono::milliseconds(32));

		}else{
			this->is_working = true;
			this->run_task(*task);
		}
	}


	auto Context::Worker::get_task_single_threaded() -> void {
		evo::debugAssert(this->context->isSingleThreaded(), "Context is not set to be single-threaded");

		this->is_working = true;

		if(this->context->tasks.empty() == false){
			auto task = std::unique_ptr<Task>(this->context->tasks.front().release());
			this->context->tasks.pop();
			this->run_task(*task);
		}

		this->is_working = false;
	}


	auto Context::Worker::run_task(const Task& task) -> void {
		task.visit([&](auto& value) -> void {
			using ValueT = std::decay_t<decltype(value)>;

			if constexpr(std::is_same_v<ValueT, LoadFileTask>){             this->run_load_file(value);         }
			else if constexpr(std::is_same_v<ValueT, TokenizeFileTask>){    this->run_tokenize_file(value);     }
			else if constexpr(std::is_same_v<ValueT, ParseFileTask>){       this->run_parse_file(value);        }
			else if constexpr(std::is_same_v<ValueT, SemaGlobalDeclsTask>){ this->run_sema_global_decls(value); }
			else if constexpr(std::is_same_v<ValueT, SemaGlobalStmtsComptimeTask>){
				this->run_sema_global_comptime_stmts(value);
			}else if constexpr(std::is_same_v<ValueT, SemaGlobalStmtsRuntimeTask>){
				this->run_sema_global_runtime_stmts(value);
			}
		});

	}



	auto Context::Worker::run_load_file(const LoadFileTask& task) -> void {
		if(evo::fs::exists(task.path.string()) == false){
			this->context->num_errors += 1;
			this->context->emit_diagnostic_internal(
				Diagnostic::Level::Error, Diagnostic::Code::MiscFileDoesNotExist, std::nullopt,
				std::format("File \"{}\" does not exist", task.path.string())
			);
			return;
		}

		auto file = evo::fs::File();

		const bool open_res = file.open(task.path.string(), evo::fs::FileMode::Read);
		if(open_res == false){
			file.close();
			this->context->num_errors += 1;
			this->context->emit_diagnostic_internal(
				Diagnostic::Level::Error, Diagnostic::Code::MiscLoadFileFailed, std::nullopt,
				std::format("Failed to load file: \"{}\"", task.path.string())
			);
			return;
		}

		const evo::Result<std::string> data_res = file.read();
		file.close();
		if(data_res.isError()){
			this->context->num_errors += 1;
			this->context->emit_diagnostic_internal(
				Diagnostic::Level::Error, Diagnostic::Code::MiscLoadFileFailed, std::nullopt,
				std::format("Failed to load file: \"{}\"", task.path.string())
			);
			return;
		}

		this->context->emitTrace("Loaded file: \"{}\"", task.path.string());

		const auto lock_guard = std::lock_guard(this->context->src_manager_mutex);
		this->context->getSourceManager().addSource(std::move(task.path), std::move(data_res.value()));
	}



	auto Context::Worker::run_tokenize_file(const TokenizeFileTask& task) -> void {
		auto tokenizer = Tokenizer(*this->context, task.source_id);

		const bool tokenize_result = tokenizer.tokenize();
		if(tokenize_result == false){ return; }

		const SourceManager& source_manager = this->context->getSourceManager();
		const Source& source = source_manager.getSource(task.source_id);
		this->context->emitTrace("Tokenized file: \"{}\"", source.getLocationAsString());
	}



	auto Context::Worker::run_parse_file(const ParseFileTask& task) -> void {
		auto parser = Parser(*this->context, task.source_id);

		const bool parse_result = parser.parse();
		if(parse_result == false){ return; }

		const SourceManager& source_manager = this->context->getSourceManager();
		const Source& source = source_manager.getSource(task.source_id);
		this->context->emitTrace("Parsed file: \"{}\"", source.getLocationAsString());
	}


	auto Context::Worker::run_sema_global_decls(const SemaGlobalDeclsTask& task) -> void {
		SourceManager& source_manager = this->context->getSourceManager();
		Source& source = source_manager.getSource(task.source_id);
		source.global_scope_level = this->context->getScopeManager().createLevel();

		auto semantic_analyzer = SemanticAnalyzer(*this->context, task.source_id);

		if(semantic_analyzer.analyze_global_declarations() == false){ return; }

		this->context->emitTrace("Sema Global Decls: \"{}\"", source.getLocationAsString());
	}


	auto Context::Worker::run_sema_global_comptime_stmts(const SemaGlobalStmtsComptimeTask& task) -> void {
		auto semantic_analyzer = SemanticAnalyzer(*this->context, task.source_id);

		if(semantic_analyzer.analyze_global_comptime_stmts() == false){ return; }

		const SourceManager& source_manager = this->context->getSourceManager();
		const Source& source = source_manager.getSource(task.source_id);
		this->context->emitTrace("Sema Global Stmts Comptime: \"{}\"", source.getLocationAsString());
	}


	auto Context::Worker::run_sema_global_runtime_stmts(const SemaGlobalStmtsRuntimeTask& task) -> void {
		auto semantic_analyzer = SemanticAnalyzer(*this->context, task.source_id);

		if(semantic_analyzer.analyze_global_runtime_stmts() == false){ return; }

		const SourceManager& source_manager = this->context->getSourceManager();
		const Source& source = source_manager.getSource(task.source_id);
		this->context->emitTrace("Sema Global Stmts Runtime: \"{}\"", source.getLocationAsString());
	}



}