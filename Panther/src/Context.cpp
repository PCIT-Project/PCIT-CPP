////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/Context.h"

#include <filesystem>
namespace fs = std::filesystem;

#include "./tokens/Tokenizer.h"
#include "./AST/Parser.h"
#include "./symbol_proc/SymbolProcBuilder.h"
#include "./sema/SemanticAnalyzer.h"
#include "../src/sema_to_pir/SemaToPIR.h"

#include <PIR.h>


namespace pcit::panther{


	//////////////////////////////////////////////////////////////////////
	// path helpers

	EVO_NODISCARD static auto path_exitsts(const fs::path& path) -> bool {
		auto ec = std::error_code();
		return std::filesystem::exists(path, ec) && (ec.value() == 0);
	}

	EVO_NODISCARD static auto path_is_pthr_file(const fs::path& path) -> bool {
		return path_exitsts(path) && path.extension() == ".pthr";
	}

	EVO_NODISCARD static auto normalize_path(const fs::path& path, const fs::path& base_path) -> fs::path {
		return (base_path / path).lexically_normal();
	}


	//////////////////////////////////////////////////////////////////////
	// misc


	Context::~Context(){
		if(this->constexpr_jit_engine.isInitialized()){
			this->constexpr_jit_engine.deinit();
		}
	}



	auto Context::optimalNumThreads() -> unsigned {
		return unsigned(core::ThreadPool<Task>::optimalNumThreads());
	}




	//////////////////////////////////////////////////////////////////////
	// build targets

	// TODO: force shutdown when hit fail condition
	auto Context::tokenize() -> evo::Result<> {
		const auto worker = [&](Task& task) -> evo::Result<> {
			this->tokenize_impl(std::move(task.as<FileToLoad>().path), task.as<FileToLoad>().compilation_config_id);
			return evo::Result<>();
		};

		if(this->_config.isMultiThreaded()){
			auto local_work_manager = core::ThreadPool<Task>();

			auto tasks = evo::SmallVector<Task>();
			for(FileToLoad& file_to_load : this->files_to_load){
				tasks.emplace_back(std::move(file_to_load));
			}

			local_work_manager.startup(this->_config.numThreads);
			local_work_manager.work(std::move(tasks), worker);
			local_work_manager.waitUntilDoneWorking();
			local_work_manager.shutdown();

		}else{
			auto local_work_manager = core::SingleThreadedWorkQueue<Task>(worker);

			for(FileToLoad& file_to_load : this->files_to_load){
				local_work_manager.emplaceTask(std::move(file_to_load));
			}

			this->files_to_load.clear();

			local_work_manager.run();
		}

		return evo::Result<>::fromBool(this->num_errors == 0);
	}



	auto Context::parse() -> evo::Result<> {
		const auto worker = [&](Task& task) -> evo::Result<> {
			this->parse_impl(std::move(task.as<FileToLoad>().path), task.as<FileToLoad>().compilation_config_id);
			return evo::Result<>();
		};

		if(this->_config.isMultiThreaded()){
			auto local_work_manager = core::ThreadPool<Task>();

			auto tasks = evo::SmallVector<Task>();
			for(FileToLoad& file_to_load : this->files_to_load){
				tasks.emplace_back(std::move(file_to_load));
			}

			local_work_manager.startup(this->_config.numThreads);
			local_work_manager.work(std::move(tasks), worker);
			local_work_manager.waitUntilDoneWorking();
			local_work_manager.shutdown();

		}else{
			auto local_work_manager = core::SingleThreadedWorkQueue<Task>(worker);

			for(FileToLoad& file_to_load : this->files_to_load){
				local_work_manager.emplaceTask(std::move(file_to_load));
			}

			this->files_to_load.clear();

			local_work_manager.run();
		}

		return evo::Result<>::fromBool(this->num_errors == 0);
	}


	auto Context::buildSymbolProcs() -> evo::Result<> {
		const auto worker = [&](Task& task) -> evo::Result<> {
			this->build_symbol_procs_impl(
				std::move(task.as<FileToLoad>().path), task.as<FileToLoad>().compilation_config_id
			);
			return evo::Result<>();
		};

		if(this->_config.isMultiThreaded()){
			auto local_work_manager = core::ThreadPool<Task>();

			auto tasks = evo::SmallVector<Task>();
			for(FileToLoad& file_to_load : this->files_to_load){
				tasks.emplace_back(std::move(file_to_load));
			}

			local_work_manager.startup(this->_config.numThreads);
			local_work_manager.work(std::move(tasks), worker);
			local_work_manager.waitUntilDoneWorking();
			local_work_manager.shutdown();

		}else{
			auto local_work_manager = core::SingleThreadedWorkQueue<Task>(worker);

			for(FileToLoad& file_to_load : this->files_to_load){
				local_work_manager.emplaceTask(std::move(file_to_load));
			}

			this->files_to_load.clear();

			local_work_manager.run();
		}

		return evo::Result<>::fromBool(this->num_errors == 0);
	}


	static constexpr auto round_up_to_nearest_multiple_of_64(size_t num) -> size_t {
		return (num + (64 - 1)) & ~(64 - 1);
	}

	auto Context::analyzeSemantics() -> evo::Result<> {
		if(this->buildSymbolProcs().isError()){ return evo::resultError; }

		if(this->type_manager.primitivesInitialized() == false){
			this->type_manager.initPrimitives();
		}

		if(this->constexpr_jit_engine.isInitialized() == false){
			const evo::Expected<void, evo::SmallVector<std::string>> jit_init_result = 
				this->constexpr_jit_engine.init(pir::JITEngine::InitConfig{
					.allowDefaultSymbolLinking = false,
				});

			if(jit_init_result.has_value() == false){
				auto infos = evo::SmallVector<Diagnostic::Info>();
				for(const std::string& error : jit_init_result.error()){
					infos.emplace_back(std::format("Message from LLVM: \"{}\"", error));
				}

				this->emitError(
					Diagnostic::Code::MISC_LLVM_ERROR,
					Diagnostic::Location::NONE,
					"Error trying to initalize PIR JITEngine",
					std::move(infos)
				);
				return evo::resultError;
			}

			const evo::Expected<void, evo::SmallVector<std::string>> register_result = 
			this->constexpr_jit_engine.registerFuncs({
				pir::JITEngine::FuncRegisterInfo(
					"PTHR.JIT.return_generic_int",
					[](core::GenericValue* return_value, uint64_t* data, uint64_t bitwidth) -> void {
						*return_value = core::GenericValue(core::GenericInt(
							unsigned(bitwidth),
							evo::ArrayProxy<uint64_t>(data, round_up_to_nearest_multiple_of_64(bitwidth) / 64)
						));
					}
				),
				pir::JITEngine::FuncRegisterInfo(
					"PTHR.JIT.return_generic_bool",
					[](core::GenericValue* return_value, bool value) -> void {
						*return_value = core::GenericValue(value);
					}
				),
				pir::JITEngine::FuncRegisterInfo(
					"PTHR.JIT.return_generic_f16",
					[](core::GenericValue* return_value, uint16_t* value) -> void {
						*return_value = core::GenericValue(
							core::GenericFloat::createF16(core::GenericInt::create<uint16_t>(*value))
						);
					}
				),
				pir::JITEngine::FuncRegisterInfo(
					"PTHR.JIT.return_generic_bf16",
					[](core::GenericValue* return_value, uint16_t* value) -> void {
						*return_value = core::GenericValue(
							core::GenericFloat::createBF16(core::GenericInt::create<uint16_t>(*value))
						);
					}
				),
				pir::JITEngine::FuncRegisterInfo(
					"PTHR.JIT.return_generic_f32",
					[](core::GenericValue* return_value, float32_t value) -> void {
						*return_value = core::GenericValue(core::GenericFloat::createF32(value));
					}
				),
				pir::JITEngine::FuncRegisterInfo(
					"PTHR.JIT.return_generic_f64",
					[](core::GenericValue* return_value, float64_t value) -> void {
						*return_value = core::GenericValue(core::GenericFloat::createF64(value));
					}
				),
				pir::JITEngine::FuncRegisterInfo(
					"PTHR.JIT.return_generic_f80",
					[](core::GenericValue* return_value, uint64_t* value) -> void {
						*return_value = core::GenericValue(
							core::GenericFloat::createF80(core::GenericInt(80, evo::ArrayProxy<uint64_t>(value, 2)))
						);
					}
				),
				pir::JITEngine::FuncRegisterInfo(
					"PTHR.JIT.return_generic_f128",
					[](core::GenericValue* return_value, uint64_t* value) -> void {
						*return_value = core::GenericValue(
							core::GenericFloat::createF128(core::GenericInt(128, evo::ArrayProxy<uint64_t>(value, 2)))
						);
					}
				),
				pir::JITEngine::FuncRegisterInfo(
					"PTHR.JIT.return_generic_char",
					[](core::GenericValue* return_value, char value) -> void {
						*return_value = core::GenericValue(value);
					}
				),
			});


			if(register_result.has_value() == false){
				auto infos = evo::SmallVector<Diagnostic::Info>();
				for(const std::string& error : register_result.error()){
					infos.emplace_back(std::format("Message from LLVM: \"{}\"", error));
				}

				this->emitFatal(
					Diagnostic::Code::MISC_LLVM_ERROR,
					Diagnostic::Location::NONE,
					Diagnostic::createFatalMessage("Error trying to register functions to PIR JITEngine"),
					std::move(infos)
				);
				return evo::resultError;
			}


			this->constexpr_sema_to_pir_data.createNeededJITInterfaceFuncDecls(this->constexpr_pir_module);
		}

		for(const Source::ID& source_id : this->source_manager){
			Source& source = this->source_manager[source_id];

			source.sema_scope_id = this->sema_buffer.scope_manager.createScope();
			this->sema_buffer.scope_manager.getScope(*source.sema_scope_id)
				.pushLevel(this->sema_buffer.scope_manager.createLevel());

			source.is_ready_for_sema = true;
		}

		const auto worker = [&](Task& task_variant) -> evo::Result<> {
			task_variant.visit([&](auto& task) -> void {
				using TaskType = std::decay_t<decltype(task)>;

				if constexpr(std::is_same<TaskType, FileToLoad>()){
					evo::debugFatalBreak("Should never hit this task");

				}else if constexpr(std::is_same<TaskType, SymbolProc::ID>()){
					analyze_semantics(*this, task);

				}else{
					static_assert(false, "Unsupported task type");
				}
			});

			return evo::Result<>::fromBool(!this->hasHitFailCondition());
		};


		auto setup_tasks = [this](auto& work_manager_inst) -> void {
			for(uint32_t i = 0; const SymbolProc& symbol_proc : this->symbol_proc_manager.iterSymbolProcs()){
				EVO_DEFER([&](){ i += 1; });

				if(symbol_proc.isReadyToBeAddedToWorkQueue()){
					work_manager_inst.addTask(SymbolProc::ID(i));
				}
			}
		};


		if(this->_config.isMultiThreaded()){
			auto& work_manager_inst = this->work_manager.emplace<core::ThreadQueue<Task>>(worker);

			setup_tasks(work_manager_inst);

			work_manager_inst.startup(this->_config.numThreads);
			work_manager_inst.waitUntilDoneWorking();

			#if defined(PCIT_CONFIG_DEBUG)
				for(size_t i = 0; i < 100; i+=1){
					std::this_thread::yield();
					evo::debugAssert(work_manager_inst.isWorking() == false, "Thought was done working, was not...");
				}
			#endif

			work_manager_inst.shutdown();

		}else{
			auto& work_manager_inst = this->work_manager.emplace<core::SingleThreadedWorkQueue<Task>>(worker);

			setup_tasks(work_manager_inst);

			work_manager_inst.run();
		}


		if(this->symbol_proc_manager.notAllProcsDone() && this->num_errors == 0){
			auto infos = evo::SmallVector<Diagnostic::Info>();

			if(this->_config.isMultiThreaded()){
				infos.emplace_back("This may be caused by the multi-threading during semantic analysis. "
							"Until a fix is made, try the single-threaded mode as it should be more stable.");
			}else{
				infos.emplace_back("This may have been caused by an uncaught circular dependency");
			}

			this->emitFatal(
				Diagnostic::Code::MISC_STALL_DETECTED,
				Diagnostic::Location::NONE,
				std::format(
					"Stall detected while compiling ({}/{} symbols were not completed)",
					this->symbol_proc_manager.numProcsNotDone(),
					this->symbol_proc_manager.numProcs()
				),
				std::move(infos)
			);

			#if defined(PCIT_CONFIG_DEBUG)
				evo::log::debug("Collecting data to look at in the debugger (`symbol_proc_list`)...");
				auto symbol_proc_list = std::vector<const SymbolProc*>();
				for(const SymbolProc& symbol_proc : this->symbol_proc_manager.iterSymbolProcs()){
					symbol_proc_list.emplace_back(&symbol_proc);
				}
				evo::breakpoint();
			#endif

			return evo::resultError;
		}

		
		return evo::Result<>::fromBool(this->num_errors == 0);
	}




	auto Context::lowerToAndPrintPIR(core::Printer& printer) -> void {
		auto module = pir::Module(evo::copy(this->_config.title), this->_config.platform);

		auto sema_to_pir_data = SemaToPIR::Data(SemaToPIR::Data::Config{
			.useReadableNames   = true,
			.checkedMath        = true,
			.isJIT              = false,
			.addSourceLocations = true,
		});

		auto sema_to_pir = SemaToPIR(*this, module, sema_to_pir_data);
		sema_to_pir.lower();

		pcit::pir::printModule(module, printer);
	}





	//////////////////////////////////////////////////////////////////////
	// adding sources

	auto Context::addSourceFile(const fs::path& path, Source::CompilationConfig::ID compilation_config_id)
	-> AddSourceResult {
		evo::debugAssert(this->mayAddSourceFile(), "Cannot add any source files");
		if(path_exitsts(path) == false){ return AddSourceResult::DOESNT_EXIST; }

		const Source::CompilationConfig& compilation_config =
			this->source_manager.getSourceCompilationConfig(compilation_config_id);
		this->files_to_load.emplace_back(
			normalize_path(path, compilation_config.basePath), compilation_config_id
		);

		return AddSourceResult::SUCCESS;
	}

	auto Context::addSourceDirectory(
		const fs::path& directory, Source::CompilationConfig::ID compilation_config_id
	) -> AddSourceResult {
		evo::debugAssert(this->mayAddSourceFile(), "Cannot add any source files");
		if(path_exitsts(directory) == false){ return AddSourceResult::DOESNT_EXIST; }
		if(std::filesystem::is_directory(directory) == false){ return AddSourceResult::NOT_DIRECTORY; }

		const Source::CompilationConfig& compilation_config =
			this->source_manager.getSourceCompilationConfig(compilation_config_id);

		for(const fs::path& file_path : std::filesystem::directory_iterator(directory)){
			if(path_is_pthr_file(file_path)){
				this->files_to_load.emplace_back(
					normalize_path(file_path, compilation_config.basePath), compilation_config_id
				);
			}
		}

		return AddSourceResult::SUCCESS;
	}

	auto Context::addSourceDirectoryRecursive(
		const fs::path& directory, Source::CompilationConfig::ID compilation_config_id
	) -> AddSourceResult {
		evo::debugAssert(this->mayAddSourceFile(), "Cannot add any source files");
		if(path_exitsts(directory) == false){ return AddSourceResult::DOESNT_EXIST; }
		if(std::filesystem::is_directory(directory) == false){ return AddSourceResult::NOT_DIRECTORY; }

		const Source::CompilationConfig& compilation_config =
			this->source_manager.getSourceCompilationConfig(compilation_config_id);

		for(const fs::path& file_path : std::filesystem::recursive_directory_iterator(directory)){
			if(path_is_pthr_file(file_path)){
				this->files_to_load.emplace_back(
					normalize_path(file_path, compilation_config.basePath), compilation_config_id
				);
			}
		}

		return AddSourceResult::SUCCESS;
	}

	auto Context::addStdLib(const fs::path& directory) -> AddSourceResult {
		evo::debugAssert(this->mayAddSourceFile(), "Cannot add any source files");
		evo::debugAssert(directory.is_absolute(), "std lib directory must be absolute");
		evo::debugAssert(this->added_std_lib == false, "already added std lib");

		if(path_exitsts(directory) == false){ return AddSourceResult::DOESNT_EXIST; }
		if(std::filesystem::is_directory(directory) == false){ return AddSourceResult::NOT_DIRECTORY; }

		const Source::CompilationConfig::ID compilation_config_id = 
			this->source_manager.emplace_source_compilation_config(directory);
		const Source::CompilationConfig& compilation_config =
			this->source_manager.getSourceCompilationConfig(compilation_config_id);

		for(const fs::path& file_path : std::filesystem::recursive_directory_iterator(directory)){
			if(path_is_pthr_file(file_path)){
				const fs::path normalized_path = normalize_path(file_path, compilation_config.basePath);
				if(file_path.stem() == "std"){  this->source_manager.add_special_name_path("std", normalized_path);  }
				if(file_path.stem() == "math"){ this->source_manager.add_special_name_path("math", normalized_path); }

				this->files_to_load.emplace_back(normalized_path, compilation_config_id);
			}
		}

		this->added_std_lib = true;

		return AddSourceResult::SUCCESS;
	}



	//////////////////////////////////////////////////////////////////////
	// task 

	EVO_NODISCARD auto Context::load_source(
		fs::path&& path, Source::CompilationConfig::ID compilation_config_id
	) -> evo::Result<Source::ID> {
		if(std::filesystem::exists(path) == false){
			this->emitError(
				Diagnostic::Code::MISC_FILE_DOES_NOT_EXIST,
				Diagnostic::Location::NONE,
				std::format("File \"{}\" does not exist", path.string())
			);
			return evo::resultError;
		}

		evo::Result<std::string> file_data = evo::fs::readFile(path.string());
		if(file_data.isError()){
			this->emitError(
				Diagnostic::Code::MISC_LOAD_FILE_FAILED,
				Diagnostic::Location::NONE,
				std::format("Failed to load file: \"{}\"", path.string())	
			);
			return evo::resultError;
		}

		return this->source_manager.create_source(std::move(path), std::move(file_data.value()), compilation_config_id);
	}


	auto Context::tokenize_impl(fs::path&& path, Source::CompilationConfig::ID compilation_config_id)
	-> void {
		const evo::Result<Source::ID> new_source = this->load_source(std::move(path), compilation_config_id);
		if(new_source.isError()){ return; }

		this->trace("Loaded file: \"{}\"", this->source_manager[new_source.value()].getPath().string());

		if(panther::tokenize(*this, new_source.value()).isError()){ return; }
		this->trace("Tokenized file: \"{}\"", this->source_manager[new_source.value()].getPath().string());
	}


	auto Context::parse_impl(fs::path&& path, Source::CompilationConfig::ID compilation_config_id)
	-> void {
		const evo::Result<Source::ID> new_source = this->load_source(std::move(path), compilation_config_id);
		if(new_source.isError()){ return; }

		this->trace("Loaded file: \"{}\"", this->source_manager[new_source.value()].getPath().string());

		if(panther::tokenize(*this, new_source.value()).isError()){ return; }
		this->trace("Tokenized file: \"{}\"", this->source_manager[new_source.value()].getPath().string());

		if(panther::parse(*this, new_source.value()).isError()){ return; }
		this->trace("Parsed file: \"{}\"", this->source_manager[new_source.value()].getPath().string());
	}

	auto Context::build_symbol_procs_impl(
		fs::path&& path, Source::CompilationConfig::ID compilation_config_id
	) -> evo::Result<Source::ID> {
		const evo::Result<Source::ID> new_source = this->load_source(std::move(path), compilation_config_id);
		if(new_source.isError()){ return evo::resultError; }

		this->trace("Loaded file: \"{}\"", this->source_manager[new_source.value()].getPath().string());

		if(panther::tokenize(*this, new_source.value()).isError()){ return evo::resultError; }
		this->trace("Tokenized file: \"{}\"", this->source_manager[new_source.value()].getPath().string());

		if(panther::parse(*this, new_source.value()).isError()){ return evo::resultError; }
		this->trace("Parsed file: \"{}\"", this->source_manager[new_source.value()].getPath().string());

		if(panther::build_symbol_procs(*this, new_source.value()).isError()){ return evo::resultError; }
		this->trace(
			"Built Symbol Processes of file: \"{}\"", this->source_manager[new_source.value()].getPath().string()
		);

		return new_source.value();
	}



	//////////////////////////////////////////////////////////////////////
	// misc


	auto Context::lookupSourceID(std::string_view lookup_path, const Source& calling_source)
	-> evo::Expected<Source::ID, LookupSourceIDError> {
		if(lookup_path.empty()){
			return evo::Unexpected(LookupSourceIDError::EMPTY_PATH);
		}

		const std::optional<Source::ID> special_name_lookup = 
			this->source_manager.lookupSpecialNameSourceID(lookup_path);

		if(special_name_lookup.has_value()){ return special_name_lookup.value(); }


		// generate path
		fs::path file_path = [&]() -> fs::path {
			fs::path relative_dir = calling_source.getPath();
			relative_dir.remove_filename();

			if(lookup_path.starts_with("./")){
				return relative_dir / fs::path(lookup_path.substr(2));

			}else if(lookup_path.starts_with(".\\")){
				return relative_dir / fs::path(lookup_path.substr(3));

			// }else if(lookup_path.starts_with("/") || lookup_path.starts_with("\\")){
			// 	return relative_dir / fs::path(lookup_path.substr(1));

			}else if(lookup_path.starts_with("../") || lookup_path.starts_with("..\\")){
				return relative_dir / fs::path(lookup_path);

			}else{
				const Source::CompilationConfig& compilation_config = this->source_manager.getSourceCompilationConfig(
					calling_source.getCompilationConfigID()
				);
				return compilation_config.basePath / fs::path(lookup_path);
			}
		}().lexically_normal();


		if(calling_source.getPath() == file_path){
			return evo::Unexpected(LookupSourceIDError::SAME_AS_CALLER);
		}

		std::optional<Source::ID> lookup_source_id = this->source_manager.lookupSourceID(file_path.string());
		if(lookup_source_id.has_value()){ return lookup_source_id.value(); }

		const bool current_dynamic_file_load_contains =  [&](){
			const auto lock = std::scoped_lock(this->current_dynamic_file_load_lock);
			return this->current_dynamic_file_load.contains(file_path);
		}();
			
		if(current_dynamic_file_load_contains){
			// TODO: better waiting
			while(lookup_source_id.has_value() == false){
				std::this_thread::yield();
				std::this_thread::yield();
				std::this_thread::yield();
				lookup_source_id = this->source_manager.lookupSourceID(file_path.string());
			}

			const Source& lookup_source = this->source_manager[lookup_source_id.value()];

			while(lookup_source.is_ready_for_sema == false){
				std::this_thread::yield();
				std::this_thread::yield();
				std::this_thread::yield();
			}

			return lookup_source_id.value();
		}

		this->current_dynamic_file_load.emplace(file_path);

		if(evo::fs::exists(file_path.string())){
			if(this->_config.mode == Config::Mode::COMPILE){
				return evo::Unexpected(LookupSourceIDError::NOT_ONE_OF_SOURCES);
			}

			const evo::Result<Source::ID> dep_analysis_res = this->build_symbol_procs_impl(
				std::move(file_path), calling_source.getCompilationConfigID()
			);

			{
				const auto lock = std::scoped_lock(this->current_dynamic_file_load_lock);
				this->current_dynamic_file_load.erase(file_path);
			}

			if(dep_analysis_res.isSuccess()){
				Source& source = this->source_manager[dep_analysis_res.value()];
				source.sema_scope_id = this->sema_buffer.scope_manager.createScope();
				this->sema_buffer.scope_manager.getScope(*source.sema_scope_id)
					.pushLevel(this->sema_buffer.scope_manager.createLevel());

				this->work_manager.visit([&](auto& work_manager_inst) -> void {
					if constexpr(std::is_same<std::decay_t<decltype(work_manager_inst)>, std::monostate>()){
						evo::debugFatalBreak("Should never be importing module if no work manager is running");

					}else{
						for(const auto& global_symbol_proc : source.global_symbol_procs){
							const SymbolProc::ID symbol_proc_id = global_symbol_proc.second;
							const SymbolProc& symbol_proc = this->symbol_proc_manager.getSymbolProc(symbol_proc_id);
							if(symbol_proc.isWaiting() == false){
								work_manager_inst.addTask(symbol_proc_id);
							}
						}
					}
				});

				source.is_ready_for_sema = true;

				return dep_analysis_res.value();

			}else{
				return evo::Unexpected(LookupSourceIDError::FAILED_DURING_ANALYSIS_OF_NEWLY_LOADED);
			}
		}

		return evo::Unexpected(LookupSourceIDError::DOESNT_EXIST);
	}




	auto Context::emit_diagnostic_impl(const Diagnostic& diagnostic) -> void {
		const auto lock_guard = std::lock_guard(this->diagnostic_callback_mutex);

		this->_diagnostic_callback(*this, diagnostic);
	}

	
}
