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

#include <clang_interface.h>
#include <llvm_interface.h>



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

	auto Context::NumThreads::optimalMulti() -> NumThreads {
		return NumThreads(uint32_t(core::ThreadPool<Task>::optimalNumThreads()));
	}


	Context::~Context(){
		if(this->constexpr_jit_engine.isInitialized()){
			this->constexpr_jit_engine.deinit();
		}
	}


	//////////////////////////////////////////////////////////////////////
	// build targets


	auto Context::tokenize() -> evo::Result<> {
		this->started_any_target = true;

		const auto worker = [&](Task& task) -> evo::Result<> {
			this->tokenize_impl(std::move(task.as<FileToLoad>().path), task.as<FileToLoad>().package_id);
			return evo::Result<>();
		};

		if(this->_config.numThreads.isMulti()){
			auto local_work_manager = core::ThreadPool<Task>();

			auto tasks = evo::SmallVector<Task>();
			for(FileToLoad& file_to_load : this->files_to_load){
				tasks.emplace_back(std::move(file_to_load));
			}

			local_work_manager.startup(this->_config.numThreads.getNum());
			local_work_manager.work(std::move(tasks), worker);
			local_work_manager.waitUntilDoneWorking();
			local_work_manager.shutdown();

		}else{
			auto local_work_manager = core::SingleThreadedWorkQueue<Task>(worker);

			for(FileToLoad& file_to_load : this->files_to_load){
				local_work_manager.addTask(std::move(file_to_load));
			}

			this->files_to_load.clear();

			local_work_manager.run();
		}

		return evo::Result<>::fromBool(this->num_errors == 0);
	}



	auto Context::parse() -> evo::Result<> {
		this->started_any_target = true;

		const auto worker = [&](Task& task) -> evo::Result<> {
			this->parse_impl(std::move(task.as<FileToLoad>().path), task.as<FileToLoad>().package_id);
			return evo::Result<>();
		};

		if(this->_config.numThreads.isMulti()){
			auto local_work_manager = core::ThreadPool<Task>();

			auto tasks = evo::SmallVector<Task>();
			for(FileToLoad& file_to_load : this->files_to_load){
				tasks.emplace_back(std::move(file_to_load));
			}

			local_work_manager.startup(this->_config.numThreads.getNum());
			local_work_manager.work(std::move(tasks), worker);
			local_work_manager.waitUntilDoneWorking();
			local_work_manager.shutdown();

		}else{
			auto local_work_manager = core::SingleThreadedWorkQueue<Task>(worker);

			for(FileToLoad& file_to_load : this->files_to_load){
				local_work_manager.addTask(std::move(file_to_load));
			}

			this->files_to_load.clear();

			local_work_manager.run();
		}

		return evo::Result<>::fromBool(this->num_errors == 0);
	}


	auto Context::buildSymbolProcs() -> evo::Result<> {
		this->started_any_target = true;

		const auto worker = [&](Task& task) -> evo::Result<> {
			this->build_symbol_procs_impl(
				std::move(task.as<FileToLoad>().path), task.as<FileToLoad>().package_id
			);
			return evo::Result<>();
		};

		if(this->_config.numThreads.isMulti()){
			auto local_work_manager = core::ThreadPool<Task>();

			auto tasks = evo::SmallVector<Task>();
			for(FileToLoad& file_to_load : this->files_to_load){
				tasks.emplace_back(std::move(file_to_load));
			}

			local_work_manager.startup(this->_config.numThreads.getNum());
			local_work_manager.work(std::move(tasks), worker);
			local_work_manager.waitUntilDoneWorking();
			local_work_manager.shutdown();

		}else{
			auto local_work_manager = core::SingleThreadedWorkQueue<Task>(worker);

			for(FileToLoad& file_to_load : this->files_to_load){
				local_work_manager.addTask(std::move(file_to_load));
			}

			this->files_to_load.clear();

			local_work_manager.run();
		}

		return evo::Result<>::fromBool(this->num_errors == 0);
	}


	EVO_NODISCARD static auto clang_type_to_panther_type(
		clangint::Type clang_type,
		TypeManager& type_manager,
		const std::unordered_map<std::string, BaseType::ID>& type_map
	) -> std::optional<TypeInfo::VoidableID> {

		auto base_type_id = std::optional<BaseType::ID>();

		if(clang_type.baseType.is<clangint::BaseType::Primitive>()){
			switch(clang_type.baseType.as<clangint::BaseType::Primitive>()){
				break; case clangint::BaseType::Primitive::VOID: return TypeInfo::VoidableID::Void();

				break; case clangint::BaseType::Primitive::ISIZE: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_ISIZE);

				break; case clangint::BaseType::Primitive::I8: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_I_N, 8);

				break; case clangint::BaseType::Primitive::I16: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_I_N, 16);

				break; case clangint::BaseType::Primitive::I32: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_I_N, 32);

				break; case clangint::BaseType::Primitive::I64: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_I_N, 64);

				break; case clangint::BaseType::Primitive::I128: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_I_N, 128);

				break; case clangint::BaseType::Primitive::USIZE: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_USIZE);

				break; case clangint::BaseType::Primitive::UI8: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_UI_N, 8);

				break; case clangint::BaseType::Primitive::UI16: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_UI_N, 16);

				break; case clangint::BaseType::Primitive::UI32: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_UI_N, 32);

				break; case clangint::BaseType::Primitive::UI64: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_UI_N, 64);

				break; case clangint::BaseType::Primitive::UI128: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_UI_N, 128);

				break; case clangint::BaseType::Primitive::F16: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_F16);

				break; case clangint::BaseType::Primitive::BF16: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_BF16);

				break; case clangint::BaseType::Primitive::F32: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_F32);

				break; case clangint::BaseType::Primitive::F64: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_F64);

				break; case clangint::BaseType::Primitive::F80: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_F80);

				break; case clangint::BaseType::Primitive::F128: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_F128);

				break; case clangint::BaseType::Primitive::BYTE: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_BYTE);

				break; case clangint::BaseType::Primitive::BOOL: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_BOOL);

				break; case clangint::BaseType::Primitive::CHAR: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_CHAR);

				break; case clangint::BaseType::Primitive::RAWPTR: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_RAWPTR);

				break; case clangint::BaseType::Primitive::C_SHORT: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_C_WCHAR);

				break; case clangint::BaseType::Primitive::C_WCHAR: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_C_SHORT);

				break; case clangint::BaseType::Primitive::C_USHORT: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_C_USHORT);

				break; case clangint::BaseType::Primitive::C_INT: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_C_INT);

				break; case clangint::BaseType::Primitive::C_UINT: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_C_UINT);

				break; case clangint::BaseType::Primitive::C_LONG: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_C_LONG);

				break; case clangint::BaseType::Primitive::C_ULONG: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_C_ULONG);

				break; case clangint::BaseType::Primitive::C_LONG_LONG: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_C_LONG_LONG);

				break; case clangint::BaseType::Primitive::C_ULONG_LONG: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_C_ULONG_LONG);

				break; case clangint::BaseType::Primitive::C_LONG_DOUBLE: 
					base_type_id = type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_C_LONG_DOUBLE);

				break; case clangint::BaseType::Primitive::UNKNOWN: return std::nullopt;
			}

		}else if(clang_type.baseType.is<clangint::BaseType::NamedDecl>()){
			const std::string& decl_name = clang_type.baseType.as<clangint::BaseType::NamedDecl>().name;

			const auto find = type_map.find(decl_name);
			if(find == type_map.end()){ return std::nullopt; }

			base_type_id = find->second;

		}else{
			evo::debugAssert(clang_type.baseType.is<clangint::BaseType::Function>(), "Unsupported clangint::BaseType");

			const clangint::BaseType::Function& func_type = clang_type.baseType.as<clangint::BaseType::Function>();

			auto params = evo::SmallVector<BaseType::Function::Param>();
			auto return_types = evo::SmallVector<TypeInfo::VoidableID>();
			auto error_types = evo::SmallVector<TypeInfo::VoidableID>();

			params.reserve(func_type.getParamTypes().size());
			for(const clangint::Type& param_type : func_type.getParamTypes()){
				const std::optional<TypeInfo::VoidableID> panther_type = 
					clang_type_to_panther_type(param_type, type_manager, type_map);
				if(panther_type.has_value() == false){ return std::nullopt; }

				// TODO(FUTURE): why when including windows.h is this needed
				if(panther_type->isVoid()){ return std::nullopt; }

				params.emplace_back(panther_type->asTypeID(), BaseType::Function::Param::Kind::C, true);
			}

			const std::optional<TypeInfo::VoidableID> return_panther_type = 
				clang_type_to_panther_type(func_type.getReturnType(), type_manager, type_map);
			if(return_panther_type.has_value() == false){ return std::nullopt; }

			return_types.emplace_back(*return_panther_type);

			base_type_id = type_manager.getOrCreateFunction(
				BaseType::Function(
					std::move(params), std::move(return_types), std::move(error_types), true, false, false
				)
			);
		}


		auto qualifiers = evo::SmallVector<TypeInfo::Qualifier>();

		for(const clangint::Type::Qualifier& qualifier : clang_type.qualifiers){
			switch(qualifier){
				break; case clangint::Type::Qualifier::POINTER:
					qualifiers.emplace_back(true, !clang_type.isConst, false, true);

				break; case clangint::Type::Qualifier::CONST_POINTER:
					qualifiers.emplace_back(true, false, false, true);

				break; case clangint::Type::Qualifier::L_VALUE_REFERENCE:
					qualifiers.emplace_back(true, !clang_type.isConst, false, false);

				break; case clangint::Type::Qualifier::R_VALUE_REFERENCE:
					qualifiers.emplace_back(true, !clang_type.isConst, false, false);
			}
		}


		return TypeInfo::VoidableID(type_manager.getOrCreateTypeInfo(TypeInfo(*base_type_id, std::move(qualifiers))));
	}




	auto Context::analyzeSemantics() -> evo::Result<> {
		this->started_any_target = true;

		if(this->buildSymbolProcs().isError()){ return evo::resultError; }

		if(this->type_manager.primitivesInitialized() == false){
			this->type_manager.initPrimitives();
		}

		this->init_intrinsic_infos();

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

		}

			
		if(this->compileOtherLangHeaders().isError()){ return evo::resultError; }


		for(const Source::ID& source_id : this->source_manager.getSourceIDRange()){
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

				}else if constexpr(std::is_same<TaskType, CHeaderToLoad>()){
					evo::debugFatalBreak("Should never hit this task");

				}else if constexpr(std::is_same<TaskType, CPPHeaderToLoad>()){
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
					if(symbol_proc.isPriority()){
						work_manager_inst.addPriorityTask(SymbolProc::ID(i));
					}else{
						work_manager_inst.addTask(SymbolProc::ID(i));
					}

					this->symbol_proc_manager.getSymbolProc(SymbolProc::ID(i)).setStatusInQueue();
				}
			}
		};


		if(this->_config.numThreads.isMulti()){
			auto& work_manager_inst = this->work_manager.emplace<core::ThreadQueue<Task>>(worker);

			setup_tasks(work_manager_inst);

			work_manager_inst.startup(this->_config.numThreads.getNum());
			work_manager_inst.waitUntilDoneWorking();

			#if defined(PCIT_CONFIG_DEBUG)
				for(size_t i = 0; i < 100; i+=1){
					std::this_thread::yield();
					if(this->symbol_proc_manager.allProcsDone()){ break; }
					if(this->num_errors > 0 || this->encountered_fatal){ break; }

					if(work_manager_inst.isWorking()){
						evo::log::fatal("Thought was done working, was not...");
						evo::log::debug("Collecting data to look at in the debugger (`symbol_proc_list`)...");
						auto symbol_proc_list = std::vector<const SymbolProc*>();
						for(const SymbolProc& symbol_proc : this->symbol_proc_manager.iterSymbolProcs()){
							symbol_proc_list.emplace_back(&symbol_proc);
						}

						// Prevent escape from breakpoint
						while(true){
							evo::breakpoint(); // not temporary debugging
						}
					}
				}
			#endif

			work_manager_inst.shutdown();

		}else{
			auto& work_manager_inst = this->work_manager.emplace<core::SingleThreadedWorkQueue<Task>>(worker);

			setup_tasks(work_manager_inst);

			work_manager_inst.run();
		}




		if(this->symbol_proc_manager.notAllProcsDone() && this->num_errors == 0){
			if(this->symbol_proc_manager.numBuiltinSymbolsWaitedOn() != 0){
				std::string message = [&]() -> std::string {
					if(this->symbol_proc_manager.numBuiltinSymbolsWaitedOn() == 1){
						return std::format(
							"Missing definition of {} builtin symbols that is required by target source code",
							this->symbol_proc_manager.numBuiltinSymbolsWaitedOn()
						);

					}else{
						return "Missing definition of builtin symbol that is required by target source code";
					}
				}();

				auto infos = evo::SmallVector<Diagnostic::Info>();

				infos.emplace_back(
					"Either include the Panther standard library or define the following builtin symbols:"
				);

				for(
					size_t i = 0;
					const SymbolProcManager::BuiltinSymbolInfo& builtin_symbol_info 
					: this->symbol_proc_manager.builtin_symbols
				){
					if(builtin_symbol_info.waited_on_by.empty()){ continue; }

					switch(SymbolProc::BuiltinSymbolKind(i)){
						break; case SymbolProc::BuiltinSymbolKind::ARRAY_ITERABLE:
							infos.emplace_back("\t> array.IIterable");
						break; case SymbolProc::BuiltinSymbolKind::ARRAY_ITERABLE_RT:
							infos.emplace_back("\t> array.IIterableRT");
						break; case SymbolProc::BuiltinSymbolKind::ARRAY_REF_ITERABLE_REF:
							infos.emplace_back("\t> arrayRef.IIterableRef");
						break; case SymbolProc::BuiltinSymbolKind::ARRAY_REF_ITERABLE_REF_RT:
							infos.emplace_back("\t> arrayRef.IIterableRefRT");
						break; case SymbolProc::BuiltinSymbolKind::ARRAY_MUT_REF_ITERABLE_MUT_REF:
							infos.emplace_back("\t> arrayMutRef.IIterableMutRef");
						break; case SymbolProc::BuiltinSymbolKind::ARRAY_MUT_REF_ITERABLE_MUT_REF_RT:
							infos.emplace_back("\t> arrayMutRef.IIterableMutRefRT");
					}
				}
					
				this->emitError(
					Diagnostic::Code::MISC_BUILTIN_NOT_DEFINED,
					Diagnostic::Location::NONE,
					std::move(message),
					std::move(infos)
				);

			}else{
				auto infos = evo::SmallVector<Diagnostic::Info>();

				infos.emplace_back(
					std::format(
						"{}/{} symbols were completed ({} not completed, {} suspended)",
						this->symbol_proc_manager.numProcs() - this->symbol_proc_manager.numProcsNotDone(),
						this->symbol_proc_manager.numProcs(),
						this->symbol_proc_manager.numProcsNotDone(),
						this->symbol_proc_manager.numProcsSuspended()
					)
				);

				if(this->_config.numThreads.isMulti()){
					infos.emplace_back("This may be caused by the multi-threading during semantic analysis. "
								"Until a fix is made, try the single-threaded mode as it should be more stable.");
				}else{
					infos.emplace_back("This may have been caused by an uncaught circular dependency");
				}

				this->emitFatal(
					Diagnostic::Code::MISC_STALL_DETECTED,
					Diagnostic::Location::NONE,
					"Stall detected while compiling",
					std::move(infos)
				);

				#if defined(PCIT_CONFIG_DEBUG)
					evo::log::debug("Collecting data to look at in the debugger (`symbol_proc_list`)...");
					auto symbol_proc_list = std::vector<const SymbolProc*>();

					for(const SymbolProc& symbol_proc : this->symbol_proc_manager.iterSymbolProcs()){
						symbol_proc_list.emplace_back(&symbol_proc);
					}

					evo::log::debug(
						"For pretty print version of info in debugger, `context.symbol_proc_manager.debug_dump(true)`"
					);

					// Prevent escape from breakpoint
					while(true){
						evo::breakpoint(); // not temporary debugging
					}
				#endif
			}

			return evo::resultError;
		}

		
		return evo::Result<>::fromBool(this->num_errors == 0);
	}



	auto Context::compileOtherLangHeaders() -> evo::Result<> {
		const auto worker = [&](Task& task_variant) -> evo::Result<> {
			task_variant.visit([&](auto& task) -> void {
				using TaskType = std::decay_t<decltype(task)>;

				if constexpr(std::is_same<TaskType, FileToLoad>()){
					evo::debugFatalBreak("Should never hit this task");

				}else if constexpr(std::is_same<TaskType, CHeaderToLoad>()){
					this->analyze_clang_header_impl(std::move(task.path), task.add_includes_to_pub_api, false);

				}else if constexpr(std::is_same<TaskType, CPPHeaderToLoad>()){
					this->analyze_clang_header_impl(std::move(task.path), task.add_includes_to_pub_api, true);

				}else if constexpr(std::is_same<TaskType, SymbolProc::ID>()){
					evo::debugFatalBreak("Should never hit this task");

				}else{
					static_assert(false, "Unsupported task type");
				}
			});

			return evo::Result<>::fromBool(!this->hasHitFailCondition());
		};

		if(this->_config.numThreads.isMulti()){
			auto local_work_manager = core::ThreadPool<Task>();

			auto tasks = evo::SmallVector<Task>();
			for(CHeaderToLoad& c_header_to_load : this->c_headers_to_load){
				tasks.emplace_back(std::move(c_header_to_load));
			}
			for(CPPHeaderToLoad& cpp_header_to_load : this->cpp_headers_to_load){
				tasks.emplace_back(std::move(cpp_header_to_load));
			}

			local_work_manager.startup(this->_config.numThreads.getNum());
			local_work_manager.work(std::move(tasks), worker);
			local_work_manager.waitUntilDoneWorking();
			local_work_manager.shutdown();

		}else{
			auto local_work_manager = core::SingleThreadedWorkQueue<Task>(worker);

			for(CHeaderToLoad& c_header_to_load : this->c_headers_to_load){
				local_work_manager.addTask(std::move(c_header_to_load));
			}
			for(CPPHeaderToLoad& cpp_header_to_load : this->cpp_headers_to_load){
				local_work_manager.addTask(std::move(cpp_header_to_load));
			}

			this->files_to_load.clear();

			local_work_manager.run();
		}

		this->c_headers_to_load.clear();
		this->cpp_headers_to_load.clear();

		return evo::Result<>::fromBool(this->num_errors == 0);
	}





	auto Context::lowerToPIR(EntryKind entry_kind, pir::Module& module) -> evo::Result<> {
		auto sema_to_pir_data = SemaToPIR::Data(SemaToPIR::Data::Config{
			.useReadableNames     = true,
			.checkedMath          = true,
			.isJIT                = false,
			.addSourceLocations   = true,
			.useDebugUnreachables = false,
		});

		if(this->_config.mode == Config::Mode::BUILD_SYSTEM){
			sema_to_pir_data.createJITBuildFuncDecls(module);
		}


		auto sema_to_pir = SemaToPIR(*this, module, sema_to_pir_data);
		sema_to_pir.lower();

		switch(entry_kind){
			case EntryKind::NONE: {
				// do nothing
			} break;

			case EntryKind::CONSOLE_EXECUTABLE: {
				if(this->entry.has_value() == false){
					this->emitError(
						Diagnostic::Code::MISC_NO_ENTRY,
						Diagnostic::Location::NONE,
						"No function with the [#entry] attribute found"
					);
					return evo::resultError;
				}

				sema_to_pir.createConsoleExecutableEntry(*this->entry);
			} break;

			case EntryKind::WINDOWED_EXECUTABLE: {
				if(this->entry.has_value() == false){
					this->emitError(
						Diagnostic::Code::MISC_NO_ENTRY,
						Diagnostic::Location::NONE,
						"No function with the [#entry] attribute found"
					);
					return evo::resultError;
				}

				sema_to_pir.createWindowedExecutableEntry(*this->entry);
			} break;
		}

		return evo::Result<>();
	}




	EVO_NODISCARD static auto analyze_and_print_clang_diagnostics(
		const pcit::clangint::DiagnosticList& diagnostic_list,
		Context& context,
		SourceManager& source_manager,
		bool is_cpp
	) -> evo::Result<> {
		bool errored = false;

		for(size_t i = 0; i < diagnostic_list.diagnostics.size(); i+=1){
			const auto get_location = [&]() -> Diagnostic::Location {
				if(diagnostic_list.diagnostics[i].location.has_value()){
					return Diagnostic::Location(
						ClangSource::Location(
							source_manager.getOrCreateClangSourceID(
								evo::copy(diagnostic_list.diagnostics[i].location->filePath), is_cpp, true
							).id,
							diagnostic_list.diagnostics[i].location->line,
							diagnostic_list.diagnostics[i].location->collumn	
						)	
					);
				}else{
					return Diagnostic::Location::NONE;
				}
			};

			const Diagnostic::Location location = get_location();


			using ClangDiagnosticLevel = clangint::DiagnosticList::Diagnostic::Level;

			switch(diagnostic_list.diagnostics[i].level){
				case ClangDiagnosticLevel::FATAL: case ClangDiagnosticLevel::ERROR: {
					std::string message = "Clang: " + diagnostic_list.diagnostics[i].message;

					auto infos = evo::SmallVector<Diagnostic::Info>();
					while(
						i + 1 < diagnostic_list.diagnostics.size()
						&& diagnostic_list.diagnostics[i + 1].level == ClangDiagnosticLevel::NOTE
					){
						i += 1;
						infos.emplace_back("Note: " + diagnostic_list.diagnostics[i].message, get_location());
					}

					context.emitError(Diagnostic::Code::CLANG, location, std::move(message), std::move(infos));
					errored = true;
				} break;

				case ClangDiagnosticLevel::WARNING: {
					std::string message = "Clang: " + diagnostic_list.diagnostics[i].message;

					auto infos = evo::SmallVector<Diagnostic::Info>();
					while(
						i + 1 < diagnostic_list.diagnostics.size()
						&& diagnostic_list.diagnostics[i + 1].level == ClangDiagnosticLevel::NOTE
					){
						i += 1;
						infos.emplace_back("Note: " + diagnostic_list.diagnostics[i].message, get_location());
					}

					context.emitWarning(Diagnostic::Code::CLANG, location, std::move(message), std::move(infos));
				} break;

				case ClangDiagnosticLevel::REMARK: {
					// TODO(FUTURE): 
					evo::unimplemented("ClangDiagnosticLevel::REMARK");
				} break;

				case ClangDiagnosticLevel::NOTE: {
					evo::debugFatalBreak("Should have been consumed by a FATAL, ERROR, or WARNING");
				} break;

				case ClangDiagnosticLevel::IGNORED: {
					// TODO(FUTURE): 
					evo::unimplemented("ClangDiagnosticLevel::IGNORED");
				} break;
			}
		}

		return evo::Result<>::fromBool(!errored);
	}


	static auto get_clang_module(
		const ClangSource& clang_source,
		llvmint::LLVMContext& llvm_context,
		clangint::DiagnosticList& diagnostic_list,
		core::Target target,
		Context& context,
		SourceManager& source_manager
	) -> evo::Result<llvm::Module*> {
		const auto opts = [&]() -> evo::Variant<clangint::COpts, clangint::CPPOpts> {
			if(clang_source.isCPP()){
				return clangint::CPPOpts();
			}else{
				return clangint::COpts();
			}
		}();

		if(clang_source.isHeader()){
			auto header_interface_str = std::format("#include \"{}\"\n", clang_source.getPath().string());

			if(clang_source.isCPP()){
				for(const std::string& func_name : clang_source.getInlinedFuncNames()){
					header_interface_str +=
						std::format("auto __PTHR_inline_saver_{} = {};\n", func_name, func_name);
				}

			}else{
				for(const std::string& func_name : clang_source.getInlinedFuncNames()){
					header_interface_str +=
						std::format("__auto_type __PTHR_inline_saver_{} = {};\n", func_name, func_name);
				}
			}


			evo::Result<llvm::Module*> clang_module = clangint::getSourceLLVM(
				"PTHR_CLANG_INLINE_INTERFACE",
				header_interface_str,
				opts,
				target,
				llvm_context.native(),
				diagnostic_list
			);

			if(clang_module.isError()){
				std::ignore = analyze_and_print_clang_diagnostics(
					diagnostic_list, context, source_manager, clang_source.isCPP()
				);
				return evo::resultError;
			}

			if(analyze_and_print_clang_diagnostics(
				diagnostic_list, context, source_manager, clang_source.isCPP()
			).isError()){
				return evo::resultError;
			}

			diagnostic_list.diagnostics.clear();
			return clang_module.value();

		}else{
			evo::unimplemented("Clang Source file");
		}
	}


	auto Context::runEntry(bool allow_default_symbol_linking) -> evo::Result<uint8_t> {
		if(this->entry.has_value() == false){
			this->emitError(
				Diagnostic::Code::MISC_NO_ENTRY,
				Diagnostic::Location::NONE,
				"No function with the [#entry] attribute found"
			);
			return evo::resultError;
		}

		auto module = pir::Module(evo::copy(this->_config.title), core::Target::getCurrent());

		auto sema_to_pir_data = SemaToPIR::Data(SemaToPIR::Data::Config{
			#if defined(PCIT_CONFIG_DEBUG)
				.useReadableNames = true,
			#else
				.useReadableNames = false,
			#endif
			.checkedMath          = true,
			.isJIT                = true,
			.addSourceLocations   = true,
			.useDebugUnreachables = true,
		});
		if(this->_config.mode == Config::Mode::BUILD_SYSTEM){
			sema_to_pir_data.createJITBuildFuncDecls(module);
		}

		auto sema_to_pir = SemaToPIR(*this, module, sema_to_pir_data);
		sema_to_pir.lower();
		const pir::Function::ID pir_entry = sema_to_pir.createJITEntry(*this->entry);

		auto jit_engine = pir::JITEngine();
		jit_engine.init(pir::JITEngine::InitConfig{
			.allowDefaultSymbolLinking = allow_default_symbol_linking,
		});
		EVO_DEFER([&](){ jit_engine.deinit(); });

		if(this->_config.mode == Config::Mode::BUILD_SYSTEM){
			if(this->register_build_system_jit_funcs(jit_engine).isError()){
				return evo::resultError;
			}
		}


		{
			const evo::Expected<void, evo::SmallVector<std::string>> add_module_result = jit_engine.addModule(module);
			if(add_module_result.has_value() == false){
				this->jit_engine_result_emit_diagnositc(add_module_result.error());
				return evo::resultError;
			}
		}


		///////////////////////////////////
		// clang 


		auto clang_modules = evo::SmallVector<std::pair<llvmint::LLVMContext, llvm::Module*>>();
		clang_modules.reserve(source_manager.getClangSourceIDRange().size());

		auto diagnostic_list = clangint::DiagnosticList();

		for(ClangSource::ID clang_source_id : source_manager.getClangSourceIDRange()){
			const ClangSource& clang_source = source_manager[clang_source_id];

			auto llvm_context = llvmint::LLVMContext();
			llvm_context.init();


			const evo::Result<llvm::Module*> clang_module = get_clang_module(
				clang_source, llvm_context, diagnostic_list, this->_config.target, *this, this->source_manager
			);

			if(clang_module.isError()){ return evo::resultError; }

			clang_modules.emplace_back(std::move(llvm_context), clang_module.value());
		}


		for(auto& pair : clang_modules){
			const evo::Expected<void, evo::SmallVector<std::string>> add_module_result =
				jit_engine.addModule(pair.first.steal(), pair.second);

			if(add_module_result.has_value() == false){
				this->jit_engine_result_emit_diagnositc(add_module_result.error());
				return evo::resultError;
			}
		}



		///////////////////////////////////
		// run

		return jit_engine.getFuncPtr<uint8_t(*)(void)>("PTHR.entry")();
	}






	static auto get_clang_modules(
		llvmint::LLVMContext& llvm_context,
		SourceManager& source_manager,
		core::Target target,
		Context& context
	) -> evo::Result<evo::SmallVector<llvm::Module*>> {
		auto clang_modules = evo::SmallVector<llvm::Module*>();
		clang_modules.reserve(source_manager.getClangSourceIDRange().size());

		auto diagnostic_list = clangint::DiagnosticList();

		for(ClangSource::ID clang_source_id : source_manager.getClangSourceIDRange()){
			const ClangSource& clang_source = source_manager[clang_source_id];

			const evo::Result<llvm::Module*> clang_module = get_clang_module(
				clang_source, llvm_context, diagnostic_list, target, context, source_manager
			);

			if(clang_module.isError()){ return evo::resultError; }

			clang_modules.emplace_back(clang_module.value());
		}

		return clang_modules;
	}


	auto Context::lowerToLLVMIR(pir::Module& module) -> evo::Result<std::string> {
		auto llvm_context = llvmint::LLVMContext();
		llvm_context.init();
		EVO_DEFER([&](){ llvm_context.deinit(); });

		evo::Result<evo::SmallVector<llvm::Module*>> clang_modules = 
			get_clang_modules(llvm_context, this->source_manager, this->_config.target, *this);

		if(clang_modules.isError()){ return evo::resultError; }

		return pir::lowerToLLVMIR(
			module,
			pir::OptMode::O0,
			llvm_context.native(),
			std::move(clang_modules.value())
		);
	}

	auto Context::lowerToAssembly(pir::Module& module) -> evo::Result<std::string> {
		auto llvm_context = llvmint::LLVMContext();
		llvm_context.init();
		EVO_DEFER([&](){ llvm_context.deinit(); });

		evo::Result<evo::SmallVector<llvm::Module*>> clang_modules = 
			get_clang_modules(llvm_context, this->source_manager, this->_config.target, *this);

		if(clang_modules.isError()){ return evo::resultError; }

		return pir::lowerToAssembly(
			module,
			pir::OptMode::O0,
			llvm_context.native(),
			std::move(clang_modules.value())
		);
	}

	auto Context::lowerToObject(pir::Module& module) -> evo::Result<std::vector<evo::byte>> {
		auto llvm_context = llvmint::LLVMContext();
		llvm_context.init();
		EVO_DEFER([&](){ llvm_context.deinit(); });

		evo::Result<evo::SmallVector<llvm::Module*>> clang_modules = 
			get_clang_modules(llvm_context, this->source_manager, this->_config.target, *this);

		if(clang_modules.isError()){ return evo::resultError; }

		return pir::lowerToObject(
			module,
			pir::OptMode::O0,
			llvm_context.native(),
			std::move(clang_modules.value())
		);
	}





	//////////////////////////////////////////////////////////////////////
	// adding sources

	auto Context::addSourceFile(const fs::path& path, Source::Package::ID package_id) -> AddSourceResult {
		evo::debugAssert(this->mayAddSourceFile(), "Cannot add any source files");
		// if(path_exitsts(path) == false){ return AddSourceResult::DOESNT_EXIST; }

		const Source::Package& package = this->source_manager.getPackage(package_id);
		this->files_to_load.emplace_back(normalize_path(path, package.basePath), package_id);

		return AddSourceResult::SUCCESS;
	}

	auto Context::addSourceDirectory(const fs::path& directory, Source::Package::ID package_id) -> AddSourceResult {
		evo::debugAssert(this->mayAddSourceFile(), "Cannot add any source files");
		if(path_exitsts(directory) == false){ return AddSourceResult::DOESNT_EXIST; }
		if(std::filesystem::is_directory(directory) == false){ return AddSourceResult::NOT_DIRECTORY; }

		const Source::Package& package = this->source_manager.getPackage(package_id);

		const fs::path target_directory = normalize_path(directory, package.basePath);
		for(const fs::path& file_path : std::filesystem::directory_iterator(target_directory)){
			if(path_is_pthr_file(file_path)){
				this->files_to_load.emplace_back(normalize_path(file_path, package.basePath), package_id);
			}
		}

		return AddSourceResult::SUCCESS;
	}

	auto Context::addSourceDirectoryRecursive(const fs::path& directory, Source::Package::ID package_id)
	-> AddSourceResult {
		evo::debugAssert(this->mayAddSourceFile(), "Cannot add any source files");
		if(path_exitsts(directory) == false){ return AddSourceResult::DOESNT_EXIST; }
		if(std::filesystem::is_directory(directory) == false){ return AddSourceResult::NOT_DIRECTORY; }

		const Source::Package& package = this->source_manager.getPackage(package_id);

		const fs::path target_directory = normalize_path(directory, package.basePath);
		for(const fs::path& file_path : std::filesystem::recursive_directory_iterator(target_directory)){
			if(path_is_pthr_file(file_path)){
				this->files_to_load.emplace_back(normalize_path(file_path, package.basePath), package_id);
			}
		}

		return AddSourceResult::SUCCESS;
	}

	auto Context::addStdLib(Source::Package::ID package_id) -> void {
		evo::debugAssert(this->mayAddSourceFile(), "Cannot add any source files");
		evo::debugAssert(this->added_std_lib == false, "already added std lib");

		const Source::Package& package = this->source_manager.getPackage(package_id);

		const fs::path normalized_path = package.basePath.lexically_normal();

		for(const fs::path& file_path : std::filesystem::recursive_directory_iterator(normalized_path)){
			if(path_is_pthr_file(file_path)){
				if(file_path.stem() == "std"){ this->source_manager.add_special_name_path("std", file_path); }
			}
		}

		this->added_std_lib = true;
	}



	auto Context::addCHeaderFile(const fs::path& path, bool add_includes_to_pub_api) -> AddSourceResult {
		evo::debugAssert(this->mayAddSourceFile(), "Cannot add any source files");
		if(path_exitsts(path) == false){ return AddSourceResult::DOESNT_EXIST; }

		this->c_headers_to_load.emplace_back(
			normalize_path(path, std::filesystem::current_path()), add_includes_to_pub_api
		);
		return AddSourceResult::SUCCESS;
	}

	auto Context::addCPPHeaderFile(const fs::path& path, bool add_includes_to_pub_api) -> AddSourceResult {
		evo::debugAssert(this->mayAddSourceFile(), "Cannot add any source files");
		if(path_exitsts(path) == false){ return AddSourceResult::DOESNT_EXIST; }

		this->cpp_headers_to_load.emplace_back(
			normalize_path(path, std::filesystem::current_path()), add_includes_to_pub_api
		);
		return AddSourceResult::SUCCESS;
	}



	//////////////////////////////////////////////////////////////////////
	// task 

	EVO_NODISCARD auto Context::load_source(
		fs::path&& path, Source::Package::ID package_id
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

		return this->source_manager.create_source(std::move(path), std::move(file_data.value()), package_id);
	}


	auto Context::tokenize_impl(fs::path&& path, Source::Package::ID package_id)
	-> void {
		const evo::Result<Source::ID> new_source = this->load_source(std::move(path), package_id);
		if(new_source.isError()){ return; }

		if(panther::tokenize(*this, new_source.value()).isError()){ return; }
	}


	auto Context::parse_impl(fs::path&& path, Source::Package::ID package_id)
	-> void {
		const evo::Result<Source::ID> new_source = this->load_source(std::move(path), package_id);
		if(new_source.isError()){ return; }

		if(panther::tokenize(*this, new_source.value()).isError()){ return; }
		if(panther::parse(*this, new_source.value()).isError()){ return; }
	}

	auto Context::build_symbol_procs_impl(
		fs::path&& path, Source::Package::ID package_id
	) -> evo::Result<Source::ID> {
		const evo::Result<Source::ID> new_source = this->load_source(std::move(path), package_id);
		if(new_source.isError()){ return evo::resultError; }

		if(panther::tokenize(*this, new_source.value()).isError()){ return evo::resultError; }
		if(panther::parse(*this, new_source.value()).isError()){ return evo::resultError; }
		if(panther::build_symbol_procs(*this, new_source.value()).isError()){ return evo::resultError; }

		return new_source.value();
	}



	auto Context::analyze_clang_header_impl(std::filesystem::path&& path, bool add_includes_to_pub_api, bool is_cpp)
	-> void {
		const std::string filepath_str = path.string();

		const evo::Result<std::string> file = evo::fs::readFile(filepath_str);
		if(file.isError()){
			this->emitError(
				Diagnostic::Code::MISC_LOAD_FILE_FAILED,
				Diagnostic::Location::NONE,
				std::format("Failed to load file: \"{}\"", filepath_str)
			);
			return;
		}


		ClangSource::ID created_clang_source_id = this->source_manager.create_clang_source(
			std::filesystem::path(filepath_str), evo::copy(file.value()), is_cpp, true
		);


		auto diagnostic_list = pcit::clangint::DiagnosticList();

		const auto opts = [&]() -> evo::Variant<clangint::COpts, clangint::CPPOpts> {
			if(is_cpp){
				return clangint::CPPOpts();
			}else{
				return clangint::COpts();
			}
		}();

		auto clang_api = pcit::clangint::API();
		const evo::Result<> get_clang_header_api_res = pcit::clangint::getHeaderAPI(
			filepath_str, std::string_view(file.value()), opts, this->getConfig().target, diagnostic_list, clang_api
		);


		const bool errored =
			analyze_and_print_clang_diagnostics(diagnostic_list, *this, this->source_manager, is_cpp).isError();

		if(get_clang_header_api_res.isError() || errored){ return; }


		ClangSource& created_clang_source = this->source_manager[created_clang_source_id];


		auto type_map = std::unordered_map<std::string, BaseType::ID>();

		for(size_t i = 0; const clangint::API::Decl& decl : clang_api.getDecls()){
			EVO_DEFER([&](){ i += 1; });

			decl.visit([&](const auto& decl_ptr) -> void {
				using DeclPtr = std::decay_t<decltype(decl_ptr)>;
				
				const ClangSource::ID source_clang_source_id =
					this->source_manager.getOrCreateClangSourceID(evo::copy(decl_ptr->declFilePath), is_cpp, true).id;

				ClangSource& source_clang_source = this->source_manager[source_clang_source_id];

				if constexpr(std::is_same<DeclPtr, clangint::API::Alias*>()){
					const clangint::API::Alias& alias_decl = *decl_ptr;

					const std::optional<TypeInfo::VoidableID> panther_type = 
						clang_type_to_panther_type(alias_decl.type, this->type_manager, type_map);

					if(panther_type.has_value() == false){ return; }

					if(panther_type->isVoid()){ return; }


					const ClangSource::Symbol created_symbol = source_clang_source.getOrCreateSourceSymbol(
						alias_decl.name,
						[&]() -> ClangSource::Symbol {
							return this->type_manager.createAlias(
								BaseType::Alias(
									source_clang_source_id,
									source_clang_source.createDeclInfo(
										alias_decl.name, alias_decl.declLine, alias_decl.declCollumn
									),
									std::nullopt,
									panther_type->asTypeID(),
									false
								)
							);
						}
					);

					type_map.emplace(alias_decl.name, created_symbol.as<BaseType::ID>());

					if(add_includes_to_pub_api || source_clang_source_id == created_clang_source_id){
						created_clang_source.addImportedSymbol(alias_decl.name, created_symbol, source_clang_source_id);
					}

				}else if constexpr(std::is_same<DeclPtr, clangint::API::Struct*>()){
					const clangint::API::Struct& struct_decl = *decl_ptr;

					auto member_vars = evo::SmallVector<BaseType::Struct::MemberVar>();
					member_vars.reserve(struct_decl.members.size());
					for(const clangint::API::Struct::Member& member_var : struct_decl.members){
						const std::optional<TypeInfo::VoidableID> panther_type = 
							clang_type_to_panther_type(member_var.type, this->type_manager, type_map);

						if(panther_type.has_value() == false){ return; }

						member_vars.emplace_back(
							AST::VarDef::Kind::VAR,
							source_clang_source.createDeclInfo(
								member_var.name, member_var.declLine, member_var.declCollumn
							),
							panther_type->asTypeID(),
							std::nullopt
						);
					}

					auto member_vars_abi = evo::SmallVector<BaseType::Struct::MemberVar*>();
					member_vars_abi.reserve(member_vars.size());
					for(BaseType::Struct::MemberVar& value : member_vars){
						member_vars_abi.emplace_back(&value);
					}

					const ClangSource::Symbol created_symbol = source_clang_source.getOrCreateSourceSymbol(
						struct_decl.name,
						[&]() -> ClangSource::Symbol {
							const BaseType::ID created_struct_id = this->type_manager.createStruct(
								BaseType::Struct(
									source_clang_source_id,
									source_clang_source.createDeclInfo(
										struct_decl.name, struct_decl.declLine, struct_decl.declCollumn
									),
									std::nullopt,
									std::nullopt,
									std::numeric_limits<uint32_t>::max(),
									std::move(member_vars),
									std::move(member_vars_abi),
									nullptr,
									nullptr,
									false,
									true,
									false,
									true
								)
							);

							return ClangSource::Symbol(std::in_place_type<BaseType::ID>, created_struct_id);
						}
					);

					type_map.emplace(struct_decl.name, created_symbol.as<BaseType::ID>());

					if(add_includes_to_pub_api || source_clang_source_id == created_clang_source_id){
						created_clang_source.addImportedSymbol(
							struct_decl.name, created_symbol, source_clang_source_id
						);
					}

				}else if constexpr(std::is_same<DeclPtr, clangint::API::Union*>()){
					const clangint::API::Union& union_decl = *decl_ptr;

					auto fields = evo::SmallVector<BaseType::Union::Field>();
					fields.reserve(union_decl.fields.size());
					for(const clangint::API::Union::Field& field : union_decl.fields){
						const std::optional<TypeInfo::VoidableID> panther_type = 
							clang_type_to_panther_type(field.type, this->type_manager, type_map);
						if(panther_type.has_value() == false){ return; }

						fields.emplace_back(
							source_clang_source.createDeclInfo(field.name, field.declLine, field.declCollumn),
							panther_type->asTypeID()
						);
					}

					const ClangSource::Symbol created_symbol = source_clang_source.getOrCreateSourceSymbol(
						union_decl.name,
						[&]() -> ClangSource::Symbol {
							return this->type_manager.createUnion(
								BaseType::Union(
									source_clang_source_id,
									source_clang_source.createDeclInfo(
										union_decl.name, union_decl.declLine, union_decl.declCollumn
									),
									std::nullopt,
									std::move(fields),
									nullptr,
									nullptr,
									false,
									true
								)
							);
						}
					);

					type_map.emplace(union_decl.name, created_symbol.as<BaseType::ID>());

					if(add_includes_to_pub_api || source_clang_source_id == created_clang_source_id){
						created_clang_source.addImportedSymbol(
							union_decl.name, created_symbol, source_clang_source_id
						);
					}

				}else if constexpr(std::is_same<DeclPtr, clangint::API::Function*>()){
					const clangint::API::Function& function_decl = *decl_ptr;

					const clangint::Type func_type =
						clangint::Type(function_decl.type, evo::SmallVector<clangint::Type::Qualifier>(), true);

					const std::optional<TypeInfo::VoidableID> panther_func_type = 
						clang_type_to_panther_type(func_type, this->type_manager, type_map);
					if(panther_func_type.has_value() == false){ return; }


					const ClangSource::Symbol created_symbol = source_clang_source.getOrCreateSourceSymbol(
						function_decl.name,
						[&]() -> ClangSource::Symbol {
							auto params = evo::SmallVector<sema::Func::Param>();
							params.reserve(function_decl.params.size());
							for(const clangint::API::Function::Param& param : function_decl.params){
								params.emplace_back(
									source_clang_source.createDeclInfo(param.name, param.declLine, param.declCollumn),
									std::nullopt
								);
							}

							return this->sema_buffer.createFunc(
								source_clang_source_id,
								source_clang_source.createDeclInfo(
									function_decl.name, function_decl.declLine, function_decl.declCollumn
								),
								function_decl.mangled_name,
								std::nullopt,
								this->type_manager.getTypeInfo(panther_func_type->asTypeID()).baseTypeID().funcID(),
								std::move(params),
								evo::SmallVector<Token::ID>(),
								evo::SmallVector<Token::ID>(),
								std::nullopt,
								uint32_t(function_decl.params.size()),
								false,
								false,
								false,
								true,
								false
							);
						}
					);

					if(add_includes_to_pub_api || source_clang_source_id == created_clang_source_id){
						created_clang_source.addImportedSymbol(
							function_decl.name, created_symbol, source_clang_source_id
						);

						if(function_decl.isInlined){
							created_clang_source.addInlinedFuncName(function_decl.name);
						}
					}


				}else if constexpr(std::is_same<DeclPtr, clangint::API::GlobalVar*>()){
					const clangint::API::GlobalVar& global_var_decl = *decl_ptr;

					const std::optional<TypeInfo::VoidableID> panther_var_type = 
						clang_type_to_panther_type(global_var_decl.type, this->type_manager, type_map);
					if(panther_var_type.has_value() == false){ return; }

					if(panther_var_type->isVoid()){ return; }


					const ClangSource::Symbol created_symbol = source_clang_source.getOrCreateSourceSymbol(
						global_var_decl.name,
						[&]() -> ClangSource::Symbol {
							return this->sema_buffer.createGlobalVar(
								global_var_decl.isConst ? AST::VarDef::Kind::CONST : AST::VarDef::Kind::VAR,
								source_clang_source_id,
								source_clang_source.createDeclInfo(
									global_var_decl.name, global_var_decl.declLine, global_var_decl.declCollumn
								),
								global_var_decl.mangled_name,
								std::nullopt,
								std::optional<sema::Expr>(),
								panther_var_type->asTypeID(),
								false,
								false,
								std::nullopt
							);
						}
					);

					if(add_includes_to_pub_api || source_clang_source_id == created_clang_source_id){
						created_clang_source.addImportedSymbol(
							global_var_decl.name, created_symbol, source_clang_source_id
						);
					}

				}else{
					static_assert(false, "Unsupported decl type");
				}
			});
		}


		for(const clangint::API::Macro& macro : clang_api.getMacros()){
			const auto decl_info_id = [&]() -> std::optional<ClangSource::DeclInfoID> {
				if(macro.declFilePath.empty()){
					return std::nullopt;
				}else{
					return created_clang_source.createDeclInfo(macro.name, macro.declLine, macro.declCollumn);
				}
			}();

			created_clang_source.addDefine(macro.name, decl_info_id);

			switch(macro.value.kind()){
				case clangint::MacroExpr::Kind::NONE: {
					// do nothing, have no value or is not supported
				} break;

				case clangint::MacroExpr::Kind::BOOL: {
					created_clang_source.addImportedSymbol(
						macro.name,
						this->sema_buffer.createGlobalVar(
							AST::VarDef::Kind::DEF,
							created_clang_source_id,
							created_clang_source.createDeclInfo(
								macro.name, macro.declLine, macro.declCollumn
							),
							macro.name,
							std::nullopt,
							std::optional<sema::Expr>(sema::Expr(
								this->sema_buffer.createBoolValue(clang_api.macroExprBuffer.getBool(macro.value))
							)),
							TypeManager::getTypeBool(),
							false,
							false,
							std::nullopt
						),
						created_clang_source_id
					);
				} break;

				case clangint::MacroExpr::Kind::INTEGER: {
					const clangint::MacroExpr::Integer& int_value = clang_api.macroExprBuffer.getInteger(macro.value);

					const std::optional<TypeInfo::ID> int_type = [&]() -> std::optional<TypeInfo::ID> {
						if(int_value.type.has_value()){
							return clang_type_to_panther_type(*int_value.type, this->type_manager, type_map)
								.value().asTypeID();
						}else{
							return std::nullopt;
						}
					}();

					const sema::Expr expr_value = [&](){
						if(int_type.has_value()){
							return sema::Expr(
								this->sema_buffer.createIntValue(
									core::GenericInt::create<uint64_t>(int_value.value),
									this->type_manager.getTypeInfo(*int_type).baseTypeID()
								)
							);
						}else{
							return sema::Expr(
								this->sema_buffer.createIntValue(
									core::GenericInt::create<uint64_t>(int_value.value), std::nullopt
								)
							);
						}
					}();

					created_clang_source.addImportedSymbol(
						macro.name,
						this->sema_buffer.createGlobalVar(
							AST::VarDef::Kind::DEF,
							created_clang_source_id,
							created_clang_source.createDeclInfo(
								macro.name, macro.declLine, macro.declCollumn
							),
							macro.name,
							std::nullopt,
							std::optional<sema::Expr>(expr_value),
							int_type,
							false,
							false,
							std::nullopt
						),
						created_clang_source_id
					);
				} break;

				case clangint::MacroExpr::Kind::FLOAT: {
					const clangint::MacroExpr::Float& float_value = clang_api.macroExprBuffer.getFloat(macro.value);

					const std::optional<TypeInfo::ID> float_type = [&]() -> std::optional<TypeInfo::ID> {
						if(float_value.type.has_value()){
							return clang_type_to_panther_type(*float_value.type, this->type_manager, type_map)
								.value().asTypeID();
						}else{
							return std::nullopt;
						}
					}();

					const sema::Expr expr_value = [&](){
						if(float_type.has_value()){
							return sema::Expr(
								this->sema_buffer.createFloatValue(
									core::GenericFloat::createF64(float_value.value),
									this->type_manager.getTypeInfo(*float_type).baseTypeID()
								)
							);
						}else{
							return sema::Expr(
								this->sema_buffer.createFloatValue(
									core::GenericFloat::createF64(float_value.value), std::nullopt
								)
							);
						}
					}();

					created_clang_source.addImportedSymbol(
						macro.name,
						this->sema_buffer.createGlobalVar(
							AST::VarDef::Kind::DEF,
							created_clang_source_id,
							created_clang_source.createDeclInfo(
								macro.name, macro.declLine, macro.declCollumn
							),
							macro.name,
							std::nullopt,
							std::optional<sema::Expr>(expr_value),
							float_type,
							false,
							false,
							std::nullopt
						),
						created_clang_source_id
					);
				} break;

				case clangint::MacroExpr::Kind::IDENT: {
					const std::string& ident_value = clang_api.macroExprBuffer.getIdent(macro.value);

					const std::optional<ClangSource::SymbolInfo> imported_symbol =
						created_clang_source.getImportedSymbol(ident_value);

					if(imported_symbol.has_value()){
						created_clang_source.addImportedSymbol(
							macro.name, imported_symbol.value().symbol, created_clang_source_id
						);
					}
				} break;
			}
		}


		created_clang_source.setSymbolImportComplete();
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
				const Source::Package& package = this->source_manager.getPackage(calling_source.getPackageID());
				return package.basePath / fs::path(lookup_path);
			}
		}().lexically_normal();


		if(calling_source.getPath() == file_path){
			return evo::Unexpected(LookupSourceIDError::SAME_AS_CALLER);
		}

		std::optional<Source::ID> lookup_source_id = this->source_manager.lookupSourceID(file_path.string());
		if(lookup_source_id.has_value()){ return lookup_source_id.value(); }

		const bool current_dynamic_file_load_contains = [&](){
			const auto lock = std::scoped_lock(this->current_dynamic_file_load_lock);
			return this->current_dynamic_file_load.contains(file_path);
		}();
			
		if(current_dynamic_file_load_contains){
			// TODO(PERF): better waiting
			while(lookup_source_id.has_value() == false){
				std::this_thread::yield();
				lookup_source_id = this->source_manager.lookupSourceID(file_path.string());
			}

			const Source& lookup_source = this->source_manager[lookup_source_id.value()];

			while(lookup_source.is_ready_for_sema == false){
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
				std::move(file_path), calling_source.getPackageID()
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
							SymbolProc& symbol_proc = this->symbol_proc_manager.getSymbolProc(symbol_proc_id);
							if(symbol_proc.isWaiting() == false){
								symbol_proc.setStatusInQueue();
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



	auto Context::lookupClangSourceID(std::string_view lookup_path, const Source& calling_source, bool is_cpp)
	-> evo::Expected<ClangSource::ID, LookupSourceIDError> {
		if(lookup_path.empty()){
			return evo::Unexpected(LookupSourceIDError::EMPTY_PATH);
		}


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
				const Source::Package& package = this->source_manager.getPackage(calling_source.getPackageID());
				return package.basePath / fs::path(lookup_path);
			}
		}().lexically_normal();


		if(calling_source.getPath() == file_path){
			return evo::Unexpected(LookupSourceIDError::SAME_AS_CALLER);
		}

		std::optional<ClangSource::ID> lookup_source_id = this->source_manager.lookupClangSourceID(file_path.string());
		if(lookup_source_id.has_value()){
			if(is_cpp != this->source_manager[*lookup_source_id].isCPP()){
				return evo::Unexpected(LookupSourceIDError::WRONG_LANGUAGE);
			}
			return lookup_source_id.value();
		}

		const bool current_dynamic_file_load_contains = [&](){
			const auto lock = std::scoped_lock(this->current_dynamic_file_load_lock);
			return this->current_dynamic_file_load.contains(file_path);
		}();
			
		if(current_dynamic_file_load_contains){
			// TODO(PERF): better waiting
			while(lookup_source_id.has_value() == false){
				std::this_thread::yield();
				lookup_source_id = this->source_manager.lookupClangSourceID(file_path.string());
			}

			const ClangSource& lookup_source = this->source_manager[lookup_source_id.value()];

			while(lookup_source.isSymbolImportComplete() == false){
				std::this_thread::yield();
			}

			return lookup_source_id.value();
		}

		this->current_dynamic_file_load.emplace(file_path);

		if(evo::fs::exists(file_path.string())){
			if(this->_config.mode == Config::Mode::COMPILE){
				return evo::Unexpected(LookupSourceIDError::NOT_ONE_OF_SOURCES);
			}

			this->analyze_clang_header_impl(std::move(file_path), true, is_cpp);
		}

		return evo::Unexpected(LookupSourceIDError::DOESNT_EXIST);
	}




	auto Context::emit_diagnostic_impl(const Diagnostic& diagnostic) -> void {
		const auto lock_guard = std::lock_guard(this->diagnostic_callback_mutex);

		this->_diagnostic_callback(*this, diagnostic);
	}



	auto Context::jit_engine_result_emit_diagnositc(const evo::SmallVector<std::string>& messages) -> void {
		auto infos = evo::SmallVector<Diagnostic::Info>();
		for(const std::string& error : messages){
			infos.emplace_back(std::format("Message from LLVM: \"{}\"", error));
		}

		this->emitFatal(
			Diagnostic::Code::MISC_LLVM_ERROR,
			Diagnostic::Location::NONE,
			Diagnostic::createFatalMessage("Error trying to register functions to PIR JITEngine"),
			std::move(infos)
		);
	}



	auto Context::register_build_system_jit_funcs(pir::JITEngine& jit_engine) -> evo::Result<> {
		struct StringView{
			const char* data;
			size_t size;

			operator std::string_view(){ return std::string_view(this->data, this->size); }
			operator std::string(){ return std::string(this->data, this->size); }
		};

		using PackageID = uint32_t;


		const evo::Expected<void, evo::SmallVector<std::string>> register_result = 
			jit_engine.registerFuncs({
				pir::JITEngine::FuncRegisterInfo(
					"PTHR.BUILD.buildSetNumThreads",
					[](Context* context, uint32_t num_threads){
						context->build_system_config.numThreads = NumThreads(num_threads);
					}
				),
				pir::JITEngine::FuncRegisterInfo(
					"PTHR.BUILD.buildSetOutput",
					[](Context* context, uint32_t output){
						context->build_system_config.output = BuildSystemConfig::Output(output);
					}
				),
				pir::JITEngine::FuncRegisterInfo(
					"PTHR.BUILD.buildSetStdLibPackage",
					[](Context* context, PackageID package_id){
						context->build_system_config.stdLibPackageID = Source::Package::ID(package_id);
					}
				),
				pir::JITEngine::FuncRegisterInfo(
					"PTHR.BUILD.buildCreatePackage",
					[](Context* context, StringView* path, StringView* name, Source::Package::Warns* warnings)
					-> PackageID {
						const PackageID package_id = PackageID(context->build_system_config.packages.size());

						context->build_system_config.packages.emplace_back(
							normalize_path(
								fs::path(static_cast<std::string_view>(*path)),
								context->getConfig().workingDirectory
							),
							static_cast<std::string>(*name),
							*warnings
						);

						return package_id;
					}
				),
				pir::JITEngine::FuncRegisterInfo(
					"PTHR.BUILD.buildAddSourceFile",
					[](Context* context, StringView* file_path, PackageID package_id) -> void {
						context->build_system_config.sourceFiles.emplace_back(
							std::string(file_path->data, file_path->size), Source::Package::ID(package_id)
						);
					}
				),
				pir::JITEngine::FuncRegisterInfo(
					"PTHR.BUILD.buildAddSourceDirectory",
					[](Context* context, StringView* file_path, PackageID package_id, bool is_recursive) -> void {
						context->build_system_config.sourceDirectories.emplace_back(
							static_cast<std::string>(*file_path),
							Source::Package::ID(package_id),
							is_recursive
						);
					}
				),
				pir::JITEngine::FuncRegisterInfo(
					"PTHR.BUILD.buildAddCHeaderFile",
					[](Context* context, StringView* file_path, bool add_includes_to_pub_api) -> void {
						context->build_system_config.cLangFiles.emplace_back(
							static_cast<std::string>(*file_path), add_includes_to_pub_api, false, true
						);
					}
				),
				pir::JITEngine::FuncRegisterInfo(
					"PTHR.BUILD.buildAddCPPHeaderFile",
					[](Context* context, StringView* file_path, bool add_includes_to_pub_api) -> void {
						context->build_system_config.cLangFiles.emplace_back(
							static_cast<std::string>(*file_path), add_includes_to_pub_api, true, true
						);
					}
				),
			});

		if(register_result.has_value() == false){
			this->jit_engine_result_emit_diagnositc(register_result.error());
			return evo::resultError;
		}

		return evo::Result<>();
	}



	auto Context::init_builtin_modules() -> void {
		auto sema_to_pir = SemaToPIR(*this, this->constexpr_pir_module, this->constexpr_sema_to_pir_data);

		///////////////////////////////////
		// helper types

		const TypeInfo::ID anonymous_type_deducer_type_id = this->type_manager.getOrCreateTypeInfo(
			TypeInfo(
				this->type_manager.createTypeDeducer(
					BaseType::TypeDeducer(std::nullopt, std::nullopt)
				)
			)
		);


		///////////////////////////////////
		// builtin modules

		BuiltinModule& pthr_module = this->source_manager[BuiltinModule::ID::PTHR];
		const BuiltinModule::StringID pthr_module_this_string = pthr_module.createString("this");

		BuiltinModule& build_module = this->source_manager[BuiltinModule::ID::BUILD];


		//////////////////
		// PackageWarningSettings

		{
			auto package_warning_settings_members = evo::SmallVector<BaseType::Struct::MemberVar>{
				BaseType::Struct::MemberVar(
					AST::VarDef::Kind::VAR,
					build_module.createString("methodCallOnNonMethod"),
					TypeManager::getTypeBool(),
					BaseType::Struct::MemberVar::DefaultValue(sema::Expr(this->sema_buffer.createBoolValue(true)), true)
				),
				BaseType::Struct::MemberVar(
					AST::VarDef::Kind::VAR,
					build_module.createString("deleteMovedFromExpr"),
					TypeManager::getTypeBool(),
					BaseType::Struct::MemberVar::DefaultValue(sema::Expr(this->sema_buffer.createBoolValue(true)), true)
				),
				BaseType::Struct::MemberVar(
					AST::VarDef::Kind::VAR,
					build_module.createString("deleteTriviallyDeletableType"),
					TypeManager::getTypeBool(),
					BaseType::Struct::MemberVar::DefaultValue(sema::Expr(this->sema_buffer.createBoolValue(true)), true)
				),
				BaseType::Struct::MemberVar(
					AST::VarDef::Kind::VAR,
					build_module.createString("constexprIfCond"),
					TypeManager::getTypeBool(),
					BaseType::Struct::MemberVar::DefaultValue(sema::Expr(this->sema_buffer.createBoolValue(true)), true)
				),
				BaseType::Struct::MemberVar(
					AST::VarDef::Kind::VAR,
					build_module.createString("alreadyUnsafe"),
					TypeManager::getTypeBool(),
					BaseType::Struct::MemberVar::DefaultValue(sema::Expr(this->sema_buffer.createBoolValue(true)), true)
				),
				BaseType::Struct::MemberVar(
					AST::VarDef::Kind::VAR,
					build_module.createString("experimentalF80"),
					TypeManager::getTypeBool(),
					BaseType::Struct::MemberVar::DefaultValue(sema::Expr(this->sema_buffer.createBoolValue(true)), true)
				),
			};


			auto package_warning_settings_member_vars_abi = evo::SmallVector<BaseType::Struct::MemberVar*>();
			package_warning_settings_member_vars_abi.reserve(package_warning_settings_members.size());
			for(BaseType::Struct::MemberVar& member : package_warning_settings_members){
				package_warning_settings_member_vars_abi.emplace_back(&member);
			}

			const BaseType::ID package_warning_settings_type = this->type_manager.createStruct(
				BaseType::Struct(
					BuiltinModule::ID::BUILD,
					build_module.createString("PackageWarningSettings"),
					std::nullopt,
					std::nullopt,
					std::numeric_limits<uint32_t>::max(),
					std::move(package_warning_settings_members),
					std::move(package_warning_settings_member_vars_abi),
					nullptr,
					nullptr,
					false,
					true,
					false,
					false
				)
			);

			sema_to_pir.lowerStruct(package_warning_settings_type.structID());

			build_module.createSymbol("PackageWarningSettings", package_warning_settings_type);
		}


		//////////////////
		// PackageID

		{
			const BaseType::ID package_id = this->type_manager.createAlias(
				BaseType::Alias(
					BuiltinModule::ID::BUILD,
					pthr_module.createString("PackageID"),
					std::nullopt,
					TypeManager::getTypeUI32(),
					false
				)
			);

			build_module.createSymbol("PackageID", package_id);
		}



		//////////////////
		// IIterator

		const BaseType::ID iterator_id = this->type_manager.createInterface(
			BaseType::Interface(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("IIterator"),
				std::nullopt,
				std::nullopt,
				false,
				false
			)
		);

		pthr_module.createSymbol("IIterator", iterator_id);

		{
			const TypeInfo::ID iterator_type_id = this->type_manager.getOrCreateTypeInfo(TypeInfo(iterator_id));

			BaseType::Interface& iterator_type = this->type_manager.getInterface(iterator_id.interfaceID());


			// func next = (this mut) -> Void;
			const BaseType::ID next_type_id = this->type_manager.getOrCreateFunction(
				BaseType::Function(
					evo::SmallVector<BaseType::Function::Param>{
						BaseType::Function::Param(iterator_type_id, BaseType::Function::Param::Kind::MUT, false)
					},
					evo::SmallVector<TypeInfo::VoidableID>{TypeInfo::VoidableID::Void()},
					evo::SmallVector<TypeInfo::VoidableID>(),
					false,
					false,
					false
				)
			);

			const sema::Func::ID next_func_id = this->sema_buffer.createFunc(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("next"),
				std::string(),
				std::nullopt,
				next_type_id.funcID(),
				evo::SmallVector<sema::Func::Param>{sema::Func::Param(pthr_module_this_string, std::nullopt)},
				evo::SmallVector<Token::ID>(),
				evo::SmallVector<Token::ID>(),
				std::nullopt,
				1,
				false,
				false,
				false,
				false,
				false
			);

			this->sema_buffer.funcs[next_func_id].status = sema::Func::Status::INTERFACE_METHOD_NO_DEFAULT;

			iterator_type.methods.emplace_back(next_func_id);


			// func get = (this) -> $$*;
			const TypeInfo::ID get_return_type = this->type_manager.getOrCreateTypeInfo(
				TypeInfo(
					this->type_manager.createTypeDeducer(BaseType::TypeDeducer(std::nullopt, std::nullopt)),
					evo::SmallVector<TypeInfo::Qualifier>{TypeInfo::Qualifier::createPtr()}
				)
			);

			const BaseType::ID get_type_id = this->type_manager.getOrCreateFunction(
				BaseType::Function(
					evo::SmallVector<BaseType::Function::Param>{
						BaseType::Function::Param(iterator_type_id, BaseType::Function::Param::Kind::READ, false)
					},
					evo::SmallVector<TypeInfo::VoidableID>{get_return_type},
					evo::SmallVector<TypeInfo::VoidableID>(),
					false,
					false,
					false
				)
			);

			const sema::Func::ID get_func_id = this->sema_buffer.createFunc(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("get"),
				std::string(),
				std::nullopt,
				get_type_id.funcID(),
				evo::SmallVector<sema::Func::Param>{sema::Func::Param(pthr_module_this_string, std::nullopt)},
				evo::SmallVector<Token::ID>(),
				evo::SmallVector<Token::ID>(),
				std::nullopt,
				1,
				false,
				false,
				false,
				false,
				false
			);

			this->sema_buffer.funcs[get_func_id].status = sema::Func::Status::INTERFACE_METHOD_NO_DEFAULT;

			iterator_type.methods.emplace_back(get_func_id);



			// func atEnd = (this) -> Bool;
			const BaseType::ID at_end_type_id = this->type_manager.getOrCreateFunction(
				BaseType::Function(
					evo::SmallVector<BaseType::Function::Param>{
						BaseType::Function::Param(iterator_type_id, BaseType::Function::Param::Kind::READ, false)
					},
					evo::SmallVector<TypeInfo::VoidableID>{TypeManager::getTypeBool()},
					evo::SmallVector<TypeInfo::VoidableID>(),
					false,
					false,
					false
				)
			);

			const sema::Func::ID at_end_func_id = this->sema_buffer.createFunc(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("atEnd"),
				std::string(),
				std::nullopt,
				at_end_type_id.funcID(),
				evo::SmallVector<sema::Func::Param>{sema::Func::Param(pthr_module_this_string, std::nullopt)},
				evo::SmallVector<Token::ID>(),
				evo::SmallVector<Token::ID>(),
				std::nullopt,
				1,
				false,
				false,
				false,
				false,
				false
			);

			this->sema_buffer.funcs[at_end_func_id].status = sema::Func::Status::INTERFACE_METHOD_NO_DEFAULT;

			iterator_type.methods.emplace_back(at_end_func_id);
		}



		//////////////////
		// IMutIterator

		const BaseType::ID mut_iterator_id = this->type_manager.createInterface(
			BaseType::Interface(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("IMutIterator"),
				std::nullopt,
				std::nullopt,
				false,
				false
			)
		);

		pthr_module.createSymbol("IMutIterator", mut_iterator_id);

		{
			const TypeInfo::ID mut_iterator_type_id = this->type_manager.getOrCreateTypeInfo(TypeInfo(mut_iterator_id));

			BaseType::Interface& mut_iterator_type = this->type_manager.getInterface(mut_iterator_id.interfaceID());


			// func next = (this mut) -> Void;
			const BaseType::ID next_type_id = this->type_manager.getOrCreateFunction(
				BaseType::Function(
					evo::SmallVector<BaseType::Function::Param>{
						BaseType::Function::Param(mut_iterator_type_id, BaseType::Function::Param::Kind::MUT, false)
					},
					evo::SmallVector<TypeInfo::VoidableID>{TypeInfo::VoidableID::Void()},
					evo::SmallVector<TypeInfo::VoidableID>(),
					false,
					false,
					false
				)
			);

			const sema::Func::ID next_func_id = this->sema_buffer.createFunc(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("next"),
				std::string(),
				std::nullopt,
				next_type_id.funcID(),
				evo::SmallVector<sema::Func::Param>{sema::Func::Param(pthr_module_this_string, std::nullopt)},
				evo::SmallVector<Token::ID>(),
				evo::SmallVector<Token::ID>(),
				std::nullopt,
				1,
				false,
				false,
				false,
				false,
				false
			);

			this->sema_buffer.funcs[next_func_id].status = sema::Func::Status::INTERFACE_METHOD_NO_DEFAULT;

			mut_iterator_type.methods.emplace_back(next_func_id);


			// func get = (this) -> $$*mut;
			const TypeInfo::ID get_return_type = this->type_manager.getOrCreateTypeInfo(
				TypeInfo(
					this->type_manager.createTypeDeducer(BaseType::TypeDeducer(std::nullopt, std::nullopt)),
					evo::SmallVector<TypeInfo::Qualifier>{TypeInfo::Qualifier::createMutPtr()}
				)
			);

			const BaseType::ID get_type_id = this->type_manager.getOrCreateFunction(
				BaseType::Function(
					evo::SmallVector<BaseType::Function::Param>{
						BaseType::Function::Param(mut_iterator_type_id, BaseType::Function::Param::Kind::READ, false)
					},
					evo::SmallVector<TypeInfo::VoidableID>{get_return_type},
					evo::SmallVector<TypeInfo::VoidableID>(),
					false,
					false,
					false
				)
			);

			const sema::Func::ID get_func_id = this->sema_buffer.createFunc(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("get"),
				std::string(),
				std::nullopt,
				get_type_id.funcID(),
				evo::SmallVector<sema::Func::Param>{sema::Func::Param(pthr_module_this_string, std::nullopt)},
				evo::SmallVector<Token::ID>(),
				evo::SmallVector<Token::ID>(),
				std::nullopt,
				1,
				false,
				false,
				false,
				false,
				false
			);

			this->sema_buffer.funcs[get_func_id].status = sema::Func::Status::INTERFACE_METHOD_NO_DEFAULT;

			mut_iterator_type.methods.emplace_back(get_func_id);



			// func atEnd = (this) -> Bool;
			const BaseType::ID at_end_type_id = this->type_manager.getOrCreateFunction(
				BaseType::Function(
					evo::SmallVector<BaseType::Function::Param>{
						BaseType::Function::Param(mut_iterator_type_id, BaseType::Function::Param::Kind::READ, false)
					},
					evo::SmallVector<TypeInfo::VoidableID>{TypeManager::getTypeBool()},
					evo::SmallVector<TypeInfo::VoidableID>(),
					false,
					false,
					false
				)
			);

			const sema::Func::ID at_end_func_id = this->sema_buffer.createFunc(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("atEnd"),
				std::string(),
				std::nullopt,
				at_end_type_id.funcID(),
				evo::SmallVector<sema::Func::Param>{sema::Func::Param(pthr_module_this_string, std::nullopt)},
				evo::SmallVector<Token::ID>(),
				evo::SmallVector<Token::ID>(),
				std::nullopt,
				1,
				false,
				false,
				false,
				false,
				false
			);

			this->sema_buffer.funcs[at_end_func_id].status = sema::Func::Status::INTERFACE_METHOD_NO_DEFAULT;

			mut_iterator_type.methods.emplace_back(at_end_func_id);
		}


		//////////////////
		// IIterable

		const BaseType::ID iterable_id = this->type_manager.createInterface(
			BaseType::Interface(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("IIterable"),
				std::nullopt,
				std::nullopt,
				false,
				false
			)
		);

		pthr_module.createSymbol("IIterable", iterable_id);

		{
			const TypeInfo::ID iterable_type_id = this->type_manager.getOrCreateTypeInfo(TypeInfo(iterable_id));

			BaseType::Interface& iterable_type = this->type_manager.getInterface(iterable_id.interfaceID());


			// func createIterator = (this) -> impl($$:@pthr.IIterator);
			const BaseType::ID create_iterator_type_id = this->type_manager.getOrCreateFunction(
				BaseType::Function(
					evo::SmallVector<BaseType::Function::Param>{
						BaseType::Function::Param(iterable_type_id, BaseType::Function::Param::Kind::READ, false)
					},
					evo::SmallVector<TypeInfo::VoidableID>{
						this->type_manager.getOrCreateTypeInfo(
							TypeInfo(
								this->type_manager.getOrCreateInterfaceMap(
									BaseType::InterfaceMap(
										anonymous_type_deducer_type_id, iterator_id.interfaceID()
									)
								)
							)
						)
					},
					evo::SmallVector<TypeInfo::VoidableID>(),
					false,
					false,
					false
				)
			);

			const sema::Func::ID create_iterator_func_id = this->sema_buffer.createFunc(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("createIterator"),
				std::string(),
				std::nullopt,
				create_iterator_type_id.funcID(),
				evo::SmallVector<sema::Func::Param>{sema::Func::Param(pthr_module_this_string, std::nullopt)},
				evo::SmallVector<Token::ID>(),
				evo::SmallVector<Token::ID>(),
				std::nullopt,
				1,
				false,
				false,
				false,
				false,
				false
			);

			this->sema_buffer.funcs[create_iterator_func_id].status = sema::Func::Status::INTERFACE_METHOD_NO_DEFAULT;

			iterable_type.methods.emplace_back(create_iterator_func_id);



			// func createIterator = (this mut) -> impl($$:@pthr.IMutIterator);
			const BaseType::ID create_mut_iterator_type_id = this->type_manager.getOrCreateFunction(
				BaseType::Function(
					evo::SmallVector<BaseType::Function::Param>{
						BaseType::Function::Param(iterable_type_id, BaseType::Function::Param::Kind::MUT, false)
					},
					evo::SmallVector<TypeInfo::VoidableID>{
						this->type_manager.getOrCreateTypeInfo(
							TypeInfo(
								this->type_manager.getOrCreateInterfaceMap(
									BaseType::InterfaceMap(
										anonymous_type_deducer_type_id, mut_iterator_id.interfaceID()
									)
								)
							)
						)
					},
					evo::SmallVector<TypeInfo::VoidableID>(),
					false,
					false,
					false
				)
			);

			const sema::Func::ID create_mut_iterator_func_id = this->sema_buffer.createFunc(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("createIterator"),
				std::string(),
				std::nullopt,
				create_mut_iterator_type_id.funcID(),
				evo::SmallVector<sema::Func::Param>{sema::Func::Param(pthr_module_this_string, std::nullopt)},
				evo::SmallVector<Token::ID>(),
				evo::SmallVector<Token::ID>(),
				std::nullopt,
				1,
				false,
				false,
				false,
				false,
				false
			);

			this->sema_buffer.funcs[create_mut_iterator_func_id].status =
				sema::Func::Status::INTERFACE_METHOD_NO_DEFAULT;

			iterable_type.methods.emplace_back(create_mut_iterator_func_id);
		}



		//////////////////
		// IIterableRef

		const BaseType::ID iterable_ref_id = this->type_manager.createInterface(
			BaseType::Interface(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("IIterableRef"),
				std::nullopt,
				std::nullopt,
				false,
				false
			)
		);

		pthr_module.createSymbol("IIterableRef", iterable_ref_id);

		{
			const TypeInfo::ID iterable_ref_type_id = this->type_manager.getOrCreateTypeInfo(TypeInfo(iterable_ref_id));

			BaseType::Interface& iterable_ref_type = this->type_manager.getInterface(iterable_ref_id.interfaceID());


			// func createIterator = (this) -> impl($$:@pthr.IIterator);
			const BaseType::ID create_iterator_type_id = this->type_manager.getOrCreateFunction(
				BaseType::Function(
					evo::SmallVector<BaseType::Function::Param>{
						BaseType::Function::Param(iterable_ref_type_id, BaseType::Function::Param::Kind::READ, false)
					},
					evo::SmallVector<TypeInfo::VoidableID>{
						this->type_manager.getOrCreateTypeInfo(
							TypeInfo(
								this->type_manager.getOrCreateInterfaceMap(
									BaseType::InterfaceMap(
										anonymous_type_deducer_type_id, iterator_id.interfaceID()
									)
								)
							)
						)
					},
					evo::SmallVector<TypeInfo::VoidableID>(),
					false,
					false,
					false
				)
			);

			const sema::Func::ID create_iterator_func_id = this->sema_buffer.createFunc(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("createIterator"),
				std::string(),
				std::nullopt,
				create_iterator_type_id.funcID(),
				evo::SmallVector<sema::Func::Param>{sema::Func::Param(pthr_module_this_string, std::nullopt)},
				evo::SmallVector<Token::ID>(),
				evo::SmallVector<Token::ID>(),
				std::nullopt,
				1,
				false,
				false,
				false,
				false,
				false
			);

			this->sema_buffer.funcs[create_iterator_func_id].status = sema::Func::Status::INTERFACE_METHOD_NO_DEFAULT;

			iterable_ref_type.methods.emplace_back(create_iterator_func_id);
		}



		//////////////////
		// IIterableMutRef

		const BaseType::ID iterable_mut_ref_id = this->type_manager.createInterface(
			BaseType::Interface(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("IIterableMutRef"),
				std::nullopt,
				std::nullopt,
				false,
				false
			)
		);

		pthr_module.createSymbol("IIterableMutRef", iterable_mut_ref_id);

		{
			const TypeInfo::ID iterable_mut_ref_type_id =
				this->type_manager.getOrCreateTypeInfo(TypeInfo(iterable_mut_ref_id));

			BaseType::Interface& iterable_mut_ref_type =
				this->type_manager.getInterface(iterable_mut_ref_id.interfaceID());


			// func createIterator = (this) -> impl($$:@pthr.IMutIterator);
			const BaseType::ID create_iterator_type_id = this->type_manager.getOrCreateFunction(
				BaseType::Function(
					evo::SmallVector<BaseType::Function::Param>{
						BaseType::Function::Param(
							iterable_mut_ref_type_id, BaseType::Function::Param::Kind::READ, false
						)
					},
					evo::SmallVector<TypeInfo::VoidableID>{
						this->type_manager.getOrCreateTypeInfo(
							TypeInfo(
								this->type_manager.getOrCreateInterfaceMap(
									BaseType::InterfaceMap(
										anonymous_type_deducer_type_id, mut_iterator_id.interfaceID()
									)
								)
							)
						)
					},
					evo::SmallVector<TypeInfo::VoidableID>(),
					false,
					false,
					false
				)
			);

			const sema::Func::ID create_iterator_func_id = this->sema_buffer.createFunc(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("createIterator"),
				std::string(),
				std::nullopt,
				create_iterator_type_id.funcID(),
				evo::SmallVector<sema::Func::Param>{sema::Func::Param(pthr_module_this_string, std::nullopt)},
				evo::SmallVector<Token::ID>(),
				evo::SmallVector<Token::ID>(),
				std::nullopt,
				1,
				false,
				false,
				false,
				false,
				false
			);

			this->sema_buffer.funcs[create_iterator_func_id].status = sema::Func::Status::INTERFACE_METHOD_NO_DEFAULT;

			iterable_mut_ref_type.methods.emplace_back(create_iterator_func_id);
		}


		//////////////////
		// IIteratorRT

		const BaseType::ID iterator_rt_id = this->type_manager.createInterface(
			BaseType::Interface(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("IIteratorRT"),
				std::nullopt,
				std::nullopt,
				false,
				false
			)
		);

		pthr_module.createSymbol("IIteratorRT", iterator_rt_id);

		{
			const TypeInfo::ID iterator_type_id = this->type_manager.getOrCreateTypeInfo(TypeInfo(iterator_rt_id));

			BaseType::Interface& iterator_type = this->type_manager.getInterface(iterator_rt_id.interfaceID());


			// func next = (this mut) #rt -> Void;
			const BaseType::ID next_type_id = this->type_manager.getOrCreateFunction(
				BaseType::Function(
					evo::SmallVector<BaseType::Function::Param>{
						BaseType::Function::Param(iterator_type_id, BaseType::Function::Param::Kind::MUT, false)
					},
					evo::SmallVector<TypeInfo::VoidableID>{TypeInfo::VoidableID::Void()},
					evo::SmallVector<TypeInfo::VoidableID>(),
					false,
					false,
					false
				)
			);

			const sema::Func::ID next_func_id = this->sema_buffer.createFunc(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("next"),
				std::string(),
				std::nullopt,
				next_type_id.funcID(),
				evo::SmallVector<sema::Func::Param>{sema::Func::Param(pthr_module_this_string, std::nullopt)},
				evo::SmallVector<Token::ID>(),
				evo::SmallVector<Token::ID>(),
				std::nullopt,
				1,
				false,
				false,
				true,
				false,
				false
			);

			this->sema_buffer.funcs[next_func_id].status = sema::Func::Status::INTERFACE_METHOD_NO_DEFAULT;

			iterator_type.methods.emplace_back(next_func_id);


			// func get = (this) #rt -> $$*;
			const TypeInfo::ID get_return_type = this->type_manager.getOrCreateTypeInfo(
				TypeInfo(
					this->type_manager.createTypeDeducer(BaseType::TypeDeducer(std::nullopt, std::nullopt)),
					evo::SmallVector<TypeInfo::Qualifier>{TypeInfo::Qualifier::createPtr()}
				)
			);

			const BaseType::ID get_type_id = this->type_manager.getOrCreateFunction(
				BaseType::Function(
					evo::SmallVector<BaseType::Function::Param>{
						BaseType::Function::Param(iterator_type_id, BaseType::Function::Param::Kind::READ, false)
					},
					evo::SmallVector<TypeInfo::VoidableID>{get_return_type},
					evo::SmallVector<TypeInfo::VoidableID>(),
					false,
					false,
					false
				)
			);

			const sema::Func::ID get_func_id = this->sema_buffer.createFunc(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("get"),
				std::string(),
				std::nullopt,
				get_type_id.funcID(),
				evo::SmallVector<sema::Func::Param>{sema::Func::Param(pthr_module_this_string, std::nullopt)},
				evo::SmallVector<Token::ID>(),
				evo::SmallVector<Token::ID>(),
				std::nullopt,
				1,
				false,
				false,
				true,
				false,
				false
			);

			this->sema_buffer.funcs[get_func_id].status = sema::Func::Status::INTERFACE_METHOD_NO_DEFAULT;

			iterator_type.methods.emplace_back(get_func_id);



			// func atEnd = (this) #rt -> Bool;
			const BaseType::ID at_end_type_id = this->type_manager.getOrCreateFunction(
				BaseType::Function(
					evo::SmallVector<BaseType::Function::Param>{
						BaseType::Function::Param(iterator_type_id, BaseType::Function::Param::Kind::READ, false)
					},
					evo::SmallVector<TypeInfo::VoidableID>{TypeManager::getTypeBool()},
					evo::SmallVector<TypeInfo::VoidableID>(),
					false,
					false,
					false
				)
			);

			const sema::Func::ID at_end_func_id = this->sema_buffer.createFunc(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("atEnd"),
				std::string(),
				std::nullopt,
				at_end_type_id.funcID(),
				evo::SmallVector<sema::Func::Param>{sema::Func::Param(pthr_module_this_string, std::nullopt)},
				evo::SmallVector<Token::ID>(),
				evo::SmallVector<Token::ID>(),
				std::nullopt,
				1,
				false,
				false,
				true,
				false,
				false
			);

			this->sema_buffer.funcs[at_end_func_id].status = sema::Func::Status::INTERFACE_METHOD_NO_DEFAULT;

			iterator_type.methods.emplace_back(at_end_func_id);
		}



		//////////////////
		// IMutIteratorRT

		const BaseType::ID mut_iterator_rt_id = this->type_manager.createInterface(
			BaseType::Interface(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("IMutIteratorRT"),
				std::nullopt,
				std::nullopt,
				false,
				false
			)
		);

		pthr_module.createSymbol("IMutIteratorRT", mut_iterator_rt_id);

		{
			const TypeInfo::ID mut_iterator_type_id =
				this->type_manager.getOrCreateTypeInfo(TypeInfo(mut_iterator_rt_id));

			BaseType::Interface& mut_iterator_type = this->type_manager.getInterface(mut_iterator_rt_id.interfaceID());


			// func next = (this mut) #rt -> Void;
			const BaseType::ID next_type_id = this->type_manager.getOrCreateFunction(
				BaseType::Function(
					evo::SmallVector<BaseType::Function::Param>{
						BaseType::Function::Param(mut_iterator_type_id, BaseType::Function::Param::Kind::MUT, false)
					},
					evo::SmallVector<TypeInfo::VoidableID>{TypeInfo::VoidableID::Void()},
					evo::SmallVector<TypeInfo::VoidableID>(),
					false,
					false,
					false
				)
			);

			const sema::Func::ID next_func_id = this->sema_buffer.createFunc(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("next"),
				std::string(),
				std::nullopt,
				next_type_id.funcID(),
				evo::SmallVector<sema::Func::Param>{sema::Func::Param(pthr_module_this_string, std::nullopt)},
				evo::SmallVector<Token::ID>(),
				evo::SmallVector<Token::ID>(),
				std::nullopt,
				1,
				false,
				false,
				true,
				false,
				false
			);

			this->sema_buffer.funcs[next_func_id].status = sema::Func::Status::INTERFACE_METHOD_NO_DEFAULT;

			mut_iterator_type.methods.emplace_back(next_func_id);


			// func get = (this) #rt -> $$*mut;
			const TypeInfo::ID get_return_type = this->type_manager.getOrCreateTypeInfo(
				TypeInfo(
					this->type_manager.createTypeDeducer(BaseType::TypeDeducer(std::nullopt, std::nullopt)),
					evo::SmallVector<TypeInfo::Qualifier>{TypeInfo::Qualifier::createMutPtr()}
				)
			);

			const BaseType::ID get_type_id = this->type_manager.getOrCreateFunction(
				BaseType::Function(
					evo::SmallVector<BaseType::Function::Param>{
						BaseType::Function::Param(mut_iterator_type_id, BaseType::Function::Param::Kind::READ, false)
					},
					evo::SmallVector<TypeInfo::VoidableID>{get_return_type},
					evo::SmallVector<TypeInfo::VoidableID>(),
					false,
					false,
					false
				)
			);

			const sema::Func::ID get_func_id = this->sema_buffer.createFunc(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("get"),
				std::string(),
				std::nullopt,
				get_type_id.funcID(),
				evo::SmallVector<sema::Func::Param>{sema::Func::Param(pthr_module_this_string, std::nullopt)},
				evo::SmallVector<Token::ID>(),
				evo::SmallVector<Token::ID>(),
				std::nullopt,
				1,
				false,
				false,
				true,
				false,
				false
			);

			this->sema_buffer.funcs[get_func_id].status = sema::Func::Status::INTERFACE_METHOD_NO_DEFAULT;

			mut_iterator_type.methods.emplace_back(get_func_id);



			// func atEnd = (this) #rt -> Bool;
			const BaseType::ID at_end_type_id = this->type_manager.getOrCreateFunction(
				BaseType::Function(
					evo::SmallVector<BaseType::Function::Param>{
						BaseType::Function::Param(mut_iterator_type_id, BaseType::Function::Param::Kind::READ, false)
					},
					evo::SmallVector<TypeInfo::VoidableID>{TypeManager::getTypeBool()},
					evo::SmallVector<TypeInfo::VoidableID>(),
					false,
					false,
					false
				)
			);

			const sema::Func::ID at_end_func_id = this->sema_buffer.createFunc(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("atEnd"),
				std::string(),
				std::nullopt,
				at_end_type_id.funcID(),
				evo::SmallVector<sema::Func::Param>{sema::Func::Param(pthr_module_this_string, std::nullopt)},
				evo::SmallVector<Token::ID>(),
				evo::SmallVector<Token::ID>(),
				std::nullopt,
				1,
				false,
				false,
				true,
				false,
				false
			);

			this->sema_buffer.funcs[at_end_func_id].status = sema::Func::Status::INTERFACE_METHOD_NO_DEFAULT;

			mut_iterator_type.methods.emplace_back(at_end_func_id);
		}


		//////////////////
		// IIterableRT

		const BaseType::ID iterable_rt_id = this->type_manager.createInterface(
			BaseType::Interface(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("IIterableRT"),
				std::nullopt,
				std::nullopt,
				false,
				false
			)
		);

		pthr_module.createSymbol("IIterableRT", iterable_rt_id);

		{
			const TypeInfo::ID iterable_rt_type_id = this->type_manager.getOrCreateTypeInfo(TypeInfo(iterable_rt_id));

			BaseType::Interface& iterable_rt_type = this->type_manager.getInterface(iterable_rt_id.interfaceID());


			// func createIterator = (this) #rt -> impl($$:@pthr.IIterator);
			const BaseType::ID create_iterator_type_id = this->type_manager.getOrCreateFunction(
				BaseType::Function(
					evo::SmallVector<BaseType::Function::Param>{
						BaseType::Function::Param(iterable_rt_type_id, BaseType::Function::Param::Kind::READ, false)
					},
					evo::SmallVector<TypeInfo::VoidableID>{
						this->type_manager.getOrCreateTypeInfo(
							TypeInfo(
								this->type_manager.getOrCreateInterfaceMap(
									BaseType::InterfaceMap(
										anonymous_type_deducer_type_id, iterator_id.interfaceID()
									)
								)
							)
						)
					},
					evo::SmallVector<TypeInfo::VoidableID>(),
					false,
					false,
					false
				)
			);

			const sema::Func::ID create_iterator_func_id = this->sema_buffer.createFunc(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("createIterator"),
				std::string(),
				std::nullopt,
				create_iterator_type_id.funcID(),
				evo::SmallVector<sema::Func::Param>{sema::Func::Param(pthr_module_this_string, std::nullopt)},
				evo::SmallVector<Token::ID>(),
				evo::SmallVector<Token::ID>(),
				std::nullopt,
				1,
				false,
				false,
				true,
				false,
				false
			);

			this->sema_buffer.funcs[create_iterator_func_id].status = sema::Func::Status::INTERFACE_METHOD_NO_DEFAULT;

			iterable_rt_type.methods.emplace_back(create_iterator_func_id);



			// func createIterator = (this mut) #rt -> impl($$:@pthr.IMutIterator);
			const BaseType::ID create_mut_iterator_type_id = this->type_manager.getOrCreateFunction(
				BaseType::Function(
					evo::SmallVector<BaseType::Function::Param>{
						BaseType::Function::Param(iterable_rt_type_id, BaseType::Function::Param::Kind::MUT, false)
					},
					evo::SmallVector<TypeInfo::VoidableID>{
						this->type_manager.getOrCreateTypeInfo(
							TypeInfo(
								this->type_manager.getOrCreateInterfaceMap(
									BaseType::InterfaceMap(
										anonymous_type_deducer_type_id, mut_iterator_id.interfaceID()
									)
								)
							)
						)
					},
					evo::SmallVector<TypeInfo::VoidableID>(),
					false,
					false,
					false
				)
			);

			const sema::Func::ID create_mut_iterator_func_id = this->sema_buffer.createFunc(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("createIterator"),
				std::string(),
				std::nullopt,
				create_mut_iterator_type_id.funcID(),
				evo::SmallVector<sema::Func::Param>{sema::Func::Param(pthr_module_this_string, std::nullopt)},
				evo::SmallVector<Token::ID>(),
				evo::SmallVector<Token::ID>(),
				std::nullopt,
				1,
				false,
				false,
				true,
				false,
				false
			);

			this->sema_buffer.funcs[create_mut_iterator_func_id].status =
				sema::Func::Status::INTERFACE_METHOD_NO_DEFAULT;

			iterable_rt_type.methods.emplace_back(create_mut_iterator_func_id);
		}



		//////////////////
		// IIterableRefRT

		const BaseType::ID iterable_rt_ref_id = this->type_manager.createInterface(
			BaseType::Interface(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("IIterableRefRT"),
				std::nullopt,
				std::nullopt,
				false,
				false
			)
		);

		pthr_module.createSymbol("IIterableRefRT", iterable_rt_ref_id);

		{
			const TypeInfo::ID iterable_rt_ref_type_id =
				this->type_manager.getOrCreateTypeInfo(TypeInfo(iterable_rt_ref_id));

			BaseType::Interface& iterable_rt_ref_type =
				this->type_manager.getInterface(iterable_rt_ref_id.interfaceID());


			// func createIterator = (this) #rt -> impl($$:@pthr.IIterator);
			const BaseType::ID create_iterator_type_id = this->type_manager.getOrCreateFunction(
				BaseType::Function(
					evo::SmallVector<BaseType::Function::Param>{
						BaseType::Function::Param(iterable_rt_ref_type_id, BaseType::Function::Param::Kind::READ, false)
					},
					evo::SmallVector<TypeInfo::VoidableID>{
						this->type_manager.getOrCreateTypeInfo(
							TypeInfo(
								this->type_manager.getOrCreateInterfaceMap(
									BaseType::InterfaceMap(
										anonymous_type_deducer_type_id, iterator_id.interfaceID()
									)
								)
							)
						)
					},
					evo::SmallVector<TypeInfo::VoidableID>(),
					false,
					false,
					false
				)
			);

			const sema::Func::ID create_iterator_func_id = this->sema_buffer.createFunc(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("createIterator"),
				std::string(),
				std::nullopt,
				create_iterator_type_id.funcID(),
				evo::SmallVector<sema::Func::Param>{sema::Func::Param(pthr_module_this_string, std::nullopt)},
				evo::SmallVector<Token::ID>(),
				evo::SmallVector<Token::ID>(),
				std::nullopt,
				1,
				false,
				false,
				true,
				false,
				false
			);

			this->sema_buffer.funcs[create_iterator_func_id].status = sema::Func::Status::INTERFACE_METHOD_NO_DEFAULT;

			iterable_rt_ref_type.methods.emplace_back(create_iterator_func_id);
		}



		//////////////////
		// IIterableMutRefRT

		const BaseType::ID iterable_rt_mut_ref_id = this->type_manager.createInterface(
			BaseType::Interface(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("IIterableMutRefRT"),
				std::nullopt,
				std::nullopt,
				false,
				false
			)
		);

		pthr_module.createSymbol("IIterableMutRefRT", iterable_rt_mut_ref_id);

		{
			const TypeInfo::ID iterable_rt_mut_ref_type_id =
				this->type_manager.getOrCreateTypeInfo(TypeInfo(iterable_rt_mut_ref_id));

			BaseType::Interface& iterable_rt_mut_ref_type =
				this->type_manager.getInterface(iterable_rt_mut_ref_id.interfaceID());


			// func createIterator = (this) #rt -> impl($$:@pthr.IMutIterator);
			const BaseType::ID create_iterator_type_id = this->type_manager.getOrCreateFunction(
				BaseType::Function(
					evo::SmallVector<BaseType::Function::Param>{
						BaseType::Function::Param(
							iterable_rt_mut_ref_type_id, BaseType::Function::Param::Kind::READ, false
						)
					},
					evo::SmallVector<TypeInfo::VoidableID>{
						this->type_manager.getOrCreateTypeInfo(
							TypeInfo(
								this->type_manager.getOrCreateInterfaceMap(
									BaseType::InterfaceMap(
										anonymous_type_deducer_type_id, mut_iterator_id.interfaceID()
									)
								)
							)
						)
					},
					evo::SmallVector<TypeInfo::VoidableID>(),
					false,
					false,
					false
				)
			);

			const sema::Func::ID create_iterator_func_id = this->sema_buffer.createFunc(
				BuiltinModule::ID::PTHR,
				pthr_module.createString("createIterator"),
				std::string(),
				std::nullopt,
				create_iterator_type_id.funcID(),
				evo::SmallVector<sema::Func::Param>{sema::Func::Param(pthr_module_this_string, std::nullopt)},
				evo::SmallVector<Token::ID>(),
				evo::SmallVector<Token::ID>(),
				std::nullopt,
				1,
				false,
				false,
				true,
				false,
				false
			);

			this->sema_buffer.funcs[create_iterator_func_id].status = sema::Func::Status::INTERFACE_METHOD_NO_DEFAULT;

			iterable_rt_mut_ref_type.methods.emplace_back(create_iterator_func_id);
		}
	}



	auto Context::init_intrinsic_infos() -> void {
		this->init_builtin_modules();

		IntrinsicFunc::initLookupTableIfNeeded();
		TemplateIntrinsicFunc::initLookupTableIfNeeded();

		const auto create_func_type = [&](
			evo::SmallVector<BaseType::Function::Param>&& params,
			evo::SmallVector<TypeInfo::VoidableID>&& returns,
			evo::SmallVector<TypeInfo::VoidableID>&& error_returns
		) -> TypeInfo::ID {
			const BaseType::ID created_func_base_type = type_manager.getOrCreateFunction(
				BaseType::Function(std::move(params), std::move(returns), std::move(error_returns), false, false, false)
			);

			return type_manager.getOrCreateTypeInfo(TypeInfo(created_func_base_type));
		};


		const TypeInfo::ID no_params_return_void = create_func_type(
			evo::SmallVector<BaseType::Function::Param>{},
			evo::SmallVector<TypeInfo::VoidableID>{TypeInfo::VoidableID::Void()},
			evo::SmallVector<TypeInfo::VoidableID>{}
		);

		const TypeInfo::ID ui32_arg_return_void = create_func_type(
			evo::SmallVector<BaseType::Function::Param>{
				BaseType::Function::Param(TypeManager::getTypeUI32(), BaseType::Function::Param::Kind::READ, true)
			},
			evo::SmallVector<TypeInfo::VoidableID>{TypeInfo::VoidableID::Void()},
			evo::SmallVector<TypeInfo::VoidableID>{}
		);

		const TypeInfo::ID bool_arg_return_void = create_func_type(
			evo::SmallVector<BaseType::Function::Param>{
				BaseType::Function::Param(TypeManager::getTypeBool(), BaseType::Function::Param::Kind::READ, true)
			},
			evo::SmallVector<TypeInfo::VoidableID>{TypeInfo::VoidableID::Void()},
			evo::SmallVector<TypeInfo::VoidableID>{}
		);

		this->intrinsic_infos[size_t(evo::to_underlying(IntrinsicFunc::Kind::ABORT))] = IntrinsicFuncInfo{
			.typeID = no_params_return_void,
			.allowedInConstexpr = false, .allowedInComptime = false, .allowedInRuntime     = true,
			.allowedInCompile   = true,  .allowedInScript   = false, .allowedInBuildSystem = true,
		};
			
		this->intrinsic_infos[size_t(evo::to_underlying(IntrinsicFunc::Kind::BREAKPOINT))] = IntrinsicFuncInfo{
			.typeID = no_params_return_void,
			.allowedInConstexpr = false, .allowedInComptime = false, .allowedInRuntime     = true,
			.allowedInCompile   = true,  .allowedInScript   = false, .allowedInBuildSystem = true,
		};
			
		this->intrinsic_infos[size_t(evo::to_underlying(IntrinsicFunc::Kind::BUILD_SET_NUM_THREADS))] = 
		IntrinsicFuncInfo{
			.typeID = ui32_arg_return_void,
			.allowedInConstexpr = false, .allowedInComptime = false, .allowedInRuntime     = true,
			.allowedInCompile   = false, .allowedInScript   = false, .allowedInBuildSystem = true,
		};
			
		this->intrinsic_infos[size_t(evo::to_underlying(IntrinsicFunc::Kind::BUILD_SET_OUTPUT))] = IntrinsicFuncInfo{
			.typeID = ui32_arg_return_void,
			.allowedInConstexpr = false, .allowedInComptime = false, .allowedInRuntime     = true,
			.allowedInCompile   = false, .allowedInScript   = false, .allowedInBuildSystem = true,
		};


		{
			const BuiltinModule& builtin_module_build = this->source_manager[BuiltinModule::ID::BUILD];

			const TypeInfo::ID package_id = this->type_manager.getOrCreateTypeInfo(
				TypeInfo(builtin_module_build.getSymbol("PackageID")->as<BaseType::ID>())
			);

			const TypeInfo::ID created_func_type = create_func_type(
				evo::SmallVector<BaseType::Function::Param>{
					BaseType::Function::Param(package_id, BaseType::Function::Param::Kind::READ, true)
				},
				evo::SmallVector<TypeInfo::VoidableID>{TypeInfo::VoidableID::Void()},
				evo::SmallVector<TypeInfo::VoidableID>()
			);

			this->intrinsic_infos[size_t(evo::to_underlying(IntrinsicFunc::Kind::BUILD_SET_STD_LIB_PACKAGE))] = 
			IntrinsicFuncInfo{
				.typeID = created_func_type,
				.allowedInConstexpr = false, .allowedInComptime = false, .allowedInRuntime     = true,
				.allowedInCompile   = false, .allowedInScript   = false, .allowedInBuildSystem = true,
			};
		}

			



		const TypeInfo::ID string_type = this->type_manager.getOrCreateTypeInfo(
			TypeInfo(
				this->type_manager.getOrCreateArrayRef(
					BaseType::ArrayRef(
						TypeManager::getTypeChar(),
						evo::SmallVector<BaseType::ArrayRef::Dimension>{BaseType::ArrayRef::Dimension::ptr()},
						std::nullopt,
						false
					)
				)
			)
		);


		{
			const BuiltinModule& builtin_module_build = this->source_manager[BuiltinModule::ID::BUILD];

			const TypeInfo::ID package_warning_settings = this->type_manager.getOrCreateTypeInfo(
				TypeInfo(builtin_module_build.getSymbol("PackageWarningSettings")->as<BaseType::ID>())
			);

			const TypeInfo::ID package_id = this->type_manager.getOrCreateTypeInfo(
				TypeInfo(builtin_module_build.getSymbol("PackageID")->as<BaseType::ID>())
			);


			const BaseType::ID created_func_base_type = type_manager.getOrCreateFunction(
				BaseType::Function(
					evo::SmallVector<BaseType::Function::Param>{
						BaseType::Function::Param(string_type, BaseType::Function::Param::Kind::READ, false),
						BaseType::Function::Param(string_type, BaseType::Function::Param::Kind::READ, false),
						BaseType::Function::Param(package_warning_settings, BaseType::Function::Param::Kind::IN, false)
					},
					evo::SmallVector<TypeInfo::VoidableID>{package_id},
					evo::SmallVector<TypeInfo::VoidableID>(),
					false,
					false,
					false
				)
			);

			this->intrinsic_infos[size_t(evo::to_underlying(IntrinsicFunc::Kind::BUILD_CREATE_PACKAGE))] = 
			IntrinsicFuncInfo{
				.typeID = type_manager.getOrCreateTypeInfo(TypeInfo(created_func_base_type)),
				.allowedInConstexpr = false, .allowedInComptime = false, .allowedInRuntime     = true,
				.allowedInCompile   = false, .allowedInScript   = false, .allowedInBuildSystem = true,
			};
		}

		{
			const BuiltinModule& builtin_module_build = this->source_manager[BuiltinModule::ID::BUILD];

			const TypeInfo::ID package_id = this->type_manager.getOrCreateTypeInfo(
				TypeInfo(builtin_module_build.getSymbol("PackageID")->as<BaseType::ID>())
			);


			const BaseType::ID created_func_base_type = type_manager.getOrCreateFunction(
				BaseType::Function(
					evo::SmallVector<BaseType::Function::Param>{
						BaseType::Function::Param(string_type, BaseType::Function::Param::Kind::READ, false),
						BaseType::Function::Param(package_id, BaseType::Function::Param::Kind::READ, true)
					},
					evo::SmallVector<TypeInfo::VoidableID>{TypeInfo::VoidableID::Void()},
					evo::SmallVector<TypeInfo::VoidableID>(),
					false,
					false,
					false
				)
			);

			this->intrinsic_infos[size_t(evo::to_underlying(IntrinsicFunc::Kind::BUILD_ADD_SOURCE_FILE))] = 
			IntrinsicFuncInfo{
				.typeID = type_manager.getOrCreateTypeInfo(TypeInfo(created_func_base_type)),
				.allowedInConstexpr = false, .allowedInComptime = false, .allowedInRuntime     = true,
				.allowedInCompile   = false, .allowedInScript   = false, .allowedInBuildSystem = true,
			};
		}

		{
			const BuiltinModule& builtin_module_build = this->source_manager[BuiltinModule::ID::BUILD];

			const TypeInfo::ID package_id = this->type_manager.getOrCreateTypeInfo(
				TypeInfo(builtin_module_build.getSymbol("PackageID")->as<BaseType::ID>())
			);


			const BaseType::ID created_func_base_type = type_manager.getOrCreateFunction(
				BaseType::Function(
					evo::SmallVector<BaseType::Function::Param>{
						BaseType::Function::Param(string_type, BaseType::Function::Param::Kind::READ, false),
						BaseType::Function::Param(package_id, BaseType::Function::Param::Kind::READ, true),
						BaseType::Function::Param(
							TypeManager::getTypeBool(), BaseType::Function::Param::Kind::READ, true
						)
					},
					evo::SmallVector<TypeInfo::VoidableID>{TypeInfo::VoidableID::Void()},
					evo::SmallVector<TypeInfo::VoidableID>(),
					false,
					false,
					false
				)
			);

			this->intrinsic_infos[size_t(evo::to_underlying(IntrinsicFunc::Kind::BUILD_ADD_SOURCE_DIRECTORY))] = 
			IntrinsicFuncInfo{
				.typeID = type_manager.getOrCreateTypeInfo(TypeInfo(created_func_base_type)),
				.allowedInConstexpr = false, .allowedInComptime = false, .allowedInRuntime     = true,
				.allowedInCompile   = false, .allowedInScript   = false, .allowedInBuildSystem = true,
			};
		}

		{
			const BaseType::ID created_func_base_type = type_manager.getOrCreateFunction(
				BaseType::Function(
					evo::SmallVector<BaseType::Function::Param>{
						BaseType::Function::Param(string_type, BaseType::Function::Param::Kind::READ, false),
						BaseType::Function::Param(
							TypeManager::getTypeBool(), BaseType::Function::Param::Kind::READ, true
						)
					},
					evo::SmallVector<TypeInfo::VoidableID>{TypeInfo::VoidableID::Void()},
					evo::SmallVector<TypeInfo::VoidableID>(),
					false,
					false,
					false
				)
			);

			this->intrinsic_infos[size_t(evo::to_underlying(IntrinsicFunc::Kind::BUILD_ADD_C_HEADER_FILE))] = 
			IntrinsicFuncInfo{
				.typeID = type_manager.getOrCreateTypeInfo(TypeInfo(created_func_base_type)),
				.allowedInConstexpr = false, .allowedInComptime = false, .allowedInRuntime     = true,
				.allowedInCompile   = false, .allowedInScript   = false, .allowedInBuildSystem = true,
			};

			this->intrinsic_infos[size_t(evo::to_underlying(IntrinsicFunc::Kind::BUILD_ADD_CPP_HEADER_FILE))] = 
			IntrinsicFuncInfo{
				.typeID = type_manager.getOrCreateTypeInfo(TypeInfo(created_func_base_type)),
				.allowedInConstexpr = false, .allowedInComptime = false, .allowedInRuntime     = true,
				.allowedInCompile   = false, .allowedInScript   = false, .allowedInBuildSystem = true,
			};
		}



		//////////////////////////////////////////////////////////////////////
		// template intrinsic infos

		using TemplateParam = std::optional<TypeInfo::ID>;
		using Param = Context::TemplateIntrinsicFuncInfo::Param;
		using Return = Context::TemplateIntrinsicFuncInfo::ReturnParam;


		const auto get_template_intrinsic_info = [&](TemplateIntrinsicFunc::Kind kind) -> TemplateIntrinsicFuncInfo& {
			return this->template_intrinsic_infos[size_t(evo::to_underlying(kind))];
		};


		///////////////////////////////////
		// type traits

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::GET_TYPE_ID) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>(),
			.returns        = evo::SmallVector<Return>{TypeManager::getTypeTypeID()},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::ARRAY_ELEMENT_TYPE_ID) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>(),
			.returns        = evo::SmallVector<Return>{TypeManager::getTypeTypeID()},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::ARRAY_REF_ELEMENT_TYPE_ID) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>(),
			.returns        = evo::SmallVector<Return>{TypeManager::getTypeTypeID()},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};


		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::NUM_BYTES) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt, TypeManager::getTypeBool()},
			.params         = evo::SmallVector<Param>(),
			.returns        = evo::SmallVector<Return>{TypeManager::getTypeUSize()},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::NUM_BITS) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt, TypeManager::getTypeBool()},
			.params         = evo::SmallVector<Param>(),
			.returns        = evo::SmallVector<Return>{TypeManager::getTypeUSize()},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};



		///////////////////////////////////
		// type conversion

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::BIT_CAST) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt, std::nullopt},
			.params         = evo::SmallVector<Param>{Param(BaseType::Function::Param::Kind::READ, 0ul)},
			.returns        = evo::SmallVector<Return>{1ul},
			.allowedInConstexpr = false, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true,  .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::TRUNC) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt, std::nullopt},
			.params         = evo::SmallVector<Param>{Param(BaseType::Function::Param::Kind::READ, 0ul)},
			.returns        = evo::SmallVector<Return>{1ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::FTRUNC) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt, std::nullopt},
			.params         = evo::SmallVector<Param>{Param(BaseType::Function::Param::Kind::READ, 0ul)},
			.returns        = evo::SmallVector<Return>{1ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::SEXT) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt, std::nullopt},
			.params         = evo::SmallVector<Param>{Param(BaseType::Function::Param::Kind::READ, 0ul)},
			.returns        = evo::SmallVector<Return>{1ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::ZEXT) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt, std::nullopt},
			.params         = evo::SmallVector<Param>{Param(BaseType::Function::Param::Kind::READ, 0ul)},
			.returns        = evo::SmallVector<Return>{1ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::FEXT) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt, std::nullopt},
			.params         = evo::SmallVector<Param>{Param(BaseType::Function::Param::Kind::READ, 0ul)},
			.returns        = evo::SmallVector<Return>{1ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::I_TO_F) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt, std::nullopt},
			.params         = evo::SmallVector<Param>{Param(BaseType::Function::Param::Kind::READ, 0ul)},
			.returns        = evo::SmallVector<Return>{1ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::F_TO_I) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt, std::nullopt},
			.params         = evo::SmallVector<Param>{Param(BaseType::Function::Param::Kind::READ, 0ul)},
			.returns        = evo::SmallVector<Return>{1ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};


		///////////////////////////////////
		// arithmetic

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::ADD) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt, TypeManager::getTypeBool()},
			.params         = evo::SmallVector<Param>{
				Param(BaseType::Function::Param::Kind::READ, 0ul), Param(BaseType::Function::Param::Kind::READ, 0ul),
			},
			.returns        = evo::SmallVector<Return>{0ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::ADD_WRAP) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>{
				Param(BaseType::Function::Param::Kind::READ, 0ul), Param(BaseType::Function::Param::Kind::READ, 0ul),
			},
			.returns        = evo::SmallVector<Return>{0ul, TypeManager::getTypeBool()},
			.allowedInConstexpr = false, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true,  .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::ADD_SAT) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>{
				Param(BaseType::Function::Param::Kind::READ, 0ul), Param(BaseType::Function::Param::Kind::READ, 0ul),
			},
			.returns        = evo::SmallVector<Return>{0ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::FADD) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>{
				Param(BaseType::Function::Param::Kind::READ, 0ul), Param(BaseType::Function::Param::Kind::READ, 0ul),
			},
			.returns        = evo::SmallVector<Return>{0ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true,  .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::SUB) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt, TypeManager::getTypeBool()},
			.params         = evo::SmallVector<Param>{
				Param(BaseType::Function::Param::Kind::READ, 0ul), Param(BaseType::Function::Param::Kind::READ, 0ul),
			},
			.returns        = evo::SmallVector<Return>{0ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true,  .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::SUB_WRAP) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>{
				Param(BaseType::Function::Param::Kind::READ, 0ul), Param(BaseType::Function::Param::Kind::READ, 0ul),
			},
			.returns        = evo::SmallVector<Return>{0ul, TypeManager::getTypeBool()},
			.allowedInConstexpr = false, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true,  .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::SUB_SAT) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>{
				Param(BaseType::Function::Param::Kind::READ, 0ul), Param(BaseType::Function::Param::Kind::READ, 0ul),
			},
			.returns        = evo::SmallVector<Return>{0ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true,  .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::FSUB) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>{
				Param(BaseType::Function::Param::Kind::READ, 0ul), Param(BaseType::Function::Param::Kind::READ, 0ul),
			},
			.returns        = evo::SmallVector<Return>{0ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true,  .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::MUL) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt, TypeManager::getTypeBool()},
			.params         = evo::SmallVector<Param>{
				Param(BaseType::Function::Param::Kind::READ, 0ul), Param(BaseType::Function::Param::Kind::READ, 0ul),
			},
			.returns        = evo::SmallVector<Return>{0ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true,  .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::MUL_WRAP) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>{
				Param(BaseType::Function::Param::Kind::READ, 0ul), Param(BaseType::Function::Param::Kind::READ, 0ul),
			},
			.returns        = evo::SmallVector<Return>{0ul, TypeManager::getTypeBool()},
			.allowedInConstexpr = false, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true,  .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::MUL_SAT) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>{
				Param(BaseType::Function::Param::Kind::READ, 0ul), Param(BaseType::Function::Param::Kind::READ, 0ul),
			},
			.returns        = evo::SmallVector<Return>{0ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::FMUL) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>{
				Param(BaseType::Function::Param::Kind::READ, 0ul), Param(BaseType::Function::Param::Kind::READ, 0ul),
			},
			.returns        = evo::SmallVector<Return>{0ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::DIV) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt, TypeManager::getTypeBool()},
			.params         = evo::SmallVector<Param>{
				Param(BaseType::Function::Param::Kind::READ, 0ul), Param(BaseType::Function::Param::Kind::READ, 0ul),
			},
			.returns        = evo::SmallVector<Return>{0ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::FDIV) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>{
				Param(BaseType::Function::Param::Kind::READ, 0ul), Param(BaseType::Function::Param::Kind::READ, 0ul),
			},
			.returns        = evo::SmallVector<Return>{0ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::REM) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>{
				Param(BaseType::Function::Param::Kind::READ, 0ul), Param(BaseType::Function::Param::Kind::READ, 0ul),
			},
			.returns        = evo::SmallVector<Return>{0ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::FNEG) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>{Param(BaseType::Function::Param::Kind::READ, 0ul)},
			.returns        = evo::SmallVector<Return>{0ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};


		///////////////////////////////////
		// comparison

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::EQ) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>{
				Param(BaseType::Function::Param::Kind::READ, 0ul), Param(BaseType::Function::Param::Kind::READ, 0ul),
			},
			.returns        = evo::SmallVector<Return>{TypeManager::getTypeBool()},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::NEQ) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>{
				Param(BaseType::Function::Param::Kind::READ, 0ul), Param(BaseType::Function::Param::Kind::READ, 0ul),
			},
			.returns        = evo::SmallVector<Return>{TypeManager::getTypeBool()},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::LT) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>{
				Param(BaseType::Function::Param::Kind::READ, 0ul), Param(BaseType::Function::Param::Kind::READ, 0ul),
			},
			.returns        = evo::SmallVector<Return>{TypeManager::getTypeBool()},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::LTE) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>{
				Param(BaseType::Function::Param::Kind::READ, 0ul), Param(BaseType::Function::Param::Kind::READ, 0ul),
			},
			.returns        = evo::SmallVector<Return>{TypeManager::getTypeBool()},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::GT) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>{
				Param(BaseType::Function::Param::Kind::READ, 0ul), Param(BaseType::Function::Param::Kind::READ, 0ul),
			},
			.returns        = evo::SmallVector<Return>{TypeManager::getTypeBool()},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::GTE) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>{
				Param(BaseType::Function::Param::Kind::READ, 0ul), Param(BaseType::Function::Param::Kind::READ, 0ul),
			},
			.returns        = evo::SmallVector<Return>{TypeManager::getTypeBool()},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};


		///////////////////////////////////
		// logical

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::AND) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>{
				Param(BaseType::Function::Param::Kind::READ, 0ul), Param(BaseType::Function::Param::Kind::READ, 0ul),
			},
			.returns        = evo::SmallVector<Return>{0ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::OR) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>{
				Param(BaseType::Function::Param::Kind::READ, 0ul), Param(BaseType::Function::Param::Kind::READ, 0ul),
			},
			.returns        = evo::SmallVector<Return>{0ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::XOR) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>{
				Param(BaseType::Function::Param::Kind::READ, 0ul), Param(BaseType::Function::Param::Kind::READ, 0ul),
			},
			.returns        = evo::SmallVector<Return>{0ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::SHL) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt, std::nullopt, TypeManager::getTypeBool()},
			.params         = evo::SmallVector<Param>{
				Param(BaseType::Function::Param::Kind::READ, 0ul), Param(BaseType::Function::Param::Kind::READ, 1ul),
			},
			.returns        = evo::SmallVector<Return>{0ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::SHL_SAT) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt, std::nullopt},
			.params         = evo::SmallVector<Param>{
				Param(BaseType::Function::Param::Kind::READ, 0ul), Param(BaseType::Function::Param::Kind::READ, 1ul),
			},
			.returns        = evo::SmallVector<Return>{0ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::SHR) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt, std::nullopt, TypeManager::getTypeBool()},
			.params         = evo::SmallVector<Param>{
				Param(BaseType::Function::Param::Kind::READ, 0ul), Param(BaseType::Function::Param::Kind::READ, 1ul),
			},
			.returns        = evo::SmallVector<Return>{0ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::BIT_REVERSE) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>{Param(BaseType::Function::Param::Kind::READ, 0ul)},
			.returns        = evo::SmallVector<Return>{0ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::BYTE_SWAP) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>{Param(BaseType::Function::Param::Kind::READ, 0ul)},
			.returns        = evo::SmallVector<Return>{0ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::CTPOP) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>{Param(BaseType::Function::Param::Kind::READ, 0ul)},
			.returns        = evo::SmallVector<Return>{0ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::CTLZ) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>{Param(BaseType::Function::Param::Kind::READ, 0ul)},
			.returns        = evo::SmallVector<Return>{0ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};

		get_template_intrinsic_info(TemplateIntrinsicFunc::Kind::CTTZ) = TemplateIntrinsicFuncInfo{
			.templateParams = evo::SmallVector<TemplateParam>{std::nullopt},
			.params         = evo::SmallVector<Param>{Param(BaseType::Function::Param::Kind::READ, 0ul)},
			.returns        = evo::SmallVector<Return>{0ul},
			.allowedInConstexpr = true, .allowedInComptime = true, .allowedInRuntime     = true,
			.allowedInCompile   = true, .allowedInScript   = true, .allowedInBuildSystem = true,
		};
	}



	auto Context::getIntrinsicFuncInfo(IntrinsicFunc::Kind kind) const -> const IntrinsicFuncInfo& {
		return this->intrinsic_infos[size_t(evo::to_underlying(kind))];
	}

	auto Context::getTemplateIntrinsicFuncInfo(TemplateIntrinsicFunc::Kind kind) -> TemplateIntrinsicFuncInfo& {
		return this->template_intrinsic_infos[size_t(evo::to_underlying(kind))];
	}


	
}
