//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "../include/Context.h"

#include "./Tokenizer.h"
#include "./Parser.h"
#include "./sema/SemanticAnalyzer.h"

namespace pcit::panther{


	Context::Context(DiagnosticCallback diagnostic_callback, const Config& _config) 
		: callback(diagnostic_callback), config(_config) {
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

		this->wait_for_all_current_tasks();
	}




	auto Context::loadFiles(evo::ArrayProxy<fs::path> file_paths) -> void {
		evo::debugAssert(
			this->isSingleThreaded() || this->threadsRunning(),
			"Context is set to be multi-threaded, but threads are not running"
		);

		evo::debugAssert(this->task_group_running == false, "Task group already running");


		this->task_group_running = true;

		// TODO: maybe check if any files had been loaded yet
		this->getSourceManager().reserveSources(file_paths.size());

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
			for(Source* source : this->src_manager.sources){
				this->tasks.emplace(std::make_unique<Task>(TokenizeFileTask(source->getID())));
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
			for(Source* source : this->src_manager.sources){
				this->tasks.emplace(std::make_unique<Task>(ParseFileTask(source->getID())));
			}
		}

		if(this->isSingleThreaded()){
			this->consume_tasks_single_threaded();
		}
	}


	auto Context::semanticAnalysisLoadedFiles() -> void {
		evo::debugAssert(this->task_group_running == false, "Task group already running");

		this->multiple_task_stages_left = true;

		if(this->type_manager.builtinsInitialized() == false){
			this->type_manager.initBuiltins();
		}

		{
			const auto lock_guard = std::lock_guard(this->src_manager_mutex);

			this->task_group_running = true;
			for(Source* source : this->src_manager.sources){
				this->tasks.emplace(std::make_unique<Task>(SemaGlobalDeclsTask(source->getID())));
			}
		}

		if(this->isSingleThreaded()){
			this->consume_tasks_single_threaded();
		}else{
			this->wait_for_all_current_tasks();
		}

		// TODO: the rest of semantic analysis

		this->multiple_task_stages_left = false;
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
		source.global_scope_level = this->context->getScopeManager().createScopeLevel();

		auto semantic_analyzer = sema::SemanticAnalyzer(*this->context, task.source_id);

		if(semantic_analyzer.analyze_global_declarations() == false){ return; }

		this->context->emitTrace("Sema Global Decls: \"{}\"", source.getLocationAsString());
	}



}