////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <filesystem>
#include <unordered_set>

#include <Evo.h>
#include <PCIT_core.h>
#include <PIR.h>

#include "./source/SourceManager.h"
#include "./Diagnostic.h"
#include "./TypeManager.h"
#include "../../src/symbol_proc/SymbolProcManager.h"
#include "./sema/SemaBuffer.h"
#include "../../src/sema_to_pir/SemaToPIR.h"


namespace pcit::panther{


	class Context{
		public:
			using DiagnosticCallback = std::function<void(Context&, const Diagnostic&)>;

			struct NumThreads{
				NumThreads(uint32_t num_threads) : num(num_threads){} // 0 is single-threaded

				EVO_NODISCARD auto isSingle() const -> bool { return this->num == 0; }
				EVO_NODISCARD auto isMulti() const -> bool { return this->num != 0; }
				EVO_NODISCARD auto getNum() const -> uint32_t {
					evo::debugAssert(this->isSingle() == false, "Cannot get num threads when is single-threaded");
					return num;
				}

				EVO_NODISCARD static auto single() -> NumThreads { return NumThreads(0); }
				EVO_NODISCARD static auto optimalMulti() -> NumThreads;

				private:
					uint32_t num;
			};


			struct Config{
				enum class Mode{
					COMPILE,
					SCRIPTING,
					BUILD_SYSTEM,
				};

				Mode mode;
				std::string title;
				core::Target target;
				std::filesystem::path workingDirectory;

				uint32_t maxNumErrors = std::numeric_limits<uint32_t>::max();
				NumThreads numThreads = NumThreads::single();
			};

			struct BuildSystemConfig{
				enum class Output : uint32_t {
					TOKENS              = 0,
					AST                 = 1,
					BUILD_SYMBOL_PROCS  = 2,
					SEMANTIC_ANALYSIS   = 3,
					PIR                 = 4,
					LLVMIR              = 5,
					ASSEMBLY            = 6,
					OBJECT              = 7,
					RUN                 = 8,
					CONSOLE_EXECUTABLE  = 9,
					WINDOWED_EXECUTABLE = 10,
				};

				struct PantherFile{
					std::string path;
					Source::ProjectConfig::ID projectID; // is index into projectConfigs
				};

				struct CLangFile{
					std::string path;
					bool addIncludesToPubApi;
					bool isCPP;
					bool isHeader;
				};


				Output output         = Output::RUN;
				NumThreads numThreads = NumThreads::single();
				bool useStdLib        = true;

				evo::StepVector<Source::ProjectConfig> projectConfigs{};
				evo::StepVector<PantherFile> sourceFiles{};
				evo::StepVector<CLangFile> cLangFiles{};
			};

			enum class AddSourceResult{
				SUCCESS,
				DOESNT_EXIST,
				NOT_DIRECTORY,
			};

		public:
			Context(const DiagnosticCallback& diagnostic_callback, const Config& config)
				: _diagnostic_callback(diagnostic_callback),
				_config(config),
				type_manager(config.target),
				constexpr_pir_module("<Panther-constexpr>", config.target),
				constexpr_sema_to_pir_data(SemaToPIR::Data::Config{
					#if defined(PCIT_CONFIG_DEBUG)
						.useReadableNames = true,
					#else
						.useReadableNames = false,
					#endif
					.checkedMath          = true,
					.isJIT                = true,
					.addSourceLocations   = true,
					.useDebugUnreachables = true,
				})
			{
				evo::debugAssert(config.target.platform != core::Target::Platform::UNKNOWN, "Platform must be known");
				evo::debugAssert(
					config.target.architecture != core::Target::Architecture::UNKNOWN, "Architecture must be known"
				);
			}

			Context(DiagnosticCallback&& diagnostic_callback, const Config& config)
				: _diagnostic_callback(std::move(diagnostic_callback)),
				_config(config),
				type_manager(config.target),
				constexpr_pir_module("<Panther-constexpr>", config.target),
				constexpr_sema_to_pir_data(SemaToPIR::Data::Config{
					#if defined(PCIT_CONFIG_DEBUG)
						.useReadableNames = true,
					#else
						.useReadableNames = false,
					#endif
					.checkedMath          = true,
					.isJIT                = true,
					.addSourceLocations   = true,
					.useDebugUnreachables = true,
				})
			{
				evo::debugAssert(config.target.platform != core::Target::Platform::UNKNOWN, "Platform must be known");
				evo::debugAssert(
					config.target.architecture != core::Target::Architecture::UNKNOWN, "Architecture must be known"
				);
			}

			~Context();

			
			EVO_NODISCARD auto getBuildSystemConfig() const -> const BuildSystemConfig& {
				return this->build_system_config;
			}


			EVO_NODISCARD static auto optimalNumThreads() -> unsigned;


			EVO_NODISCARD auto hasHitFailCondition() const -> bool {
				return this->num_errors >= this->_config.maxNumErrors || this->encountered_fatal;
			}

			EVO_NODISCARD auto mayAddSourceFile() const -> bool {
				return this->_config.mode == Config::Mode::SCRIPTING 
					|| this->_config.mode == Config::Mode::BUILD_SYSTEM
					|| this->started_any_target == false;
			}


			///////////////////////////////////
			// getters

			EVO_NODISCARD auto getNumErrors() const -> unsigned { return this->num_errors.load(); }

			EVO_NODISCARD auto getSourceManager() const -> const SourceManager& { return this->source_manager; }
			EVO_NODISCARD auto getSourceManager()       ->       SourceManager& { return this->source_manager; }

			EVO_NODISCARD auto getTypeManager() const -> const TypeManager& { return this->type_manager; }

			EVO_NODISCARD auto getSemaBuffer() const -> const SemaBuffer& { return this->sema_buffer; }

			EVO_NODISCARD auto getConfig() const -> const Config& { return this->_config; }


			///////////////////////////////////
			// build targets

			auto tokenize() -> evo::Result<>;
			auto parse() -> evo::Result<>;
			auto buildSymbolProcs() -> evo::Result<>;
			auto analyzeSemantics() -> evo::Result<>; // autmatically calls `compileOtherLangHeaders`

			auto compileOtherLangHeaders() -> evo::Result<>; // done automatically by `analyzeSemantics`


			enum class EntryKind{
				NONE,
				CONSOLE_EXECUTABLE,
				WINDOWED_EXECUTABLE,
			};

			// call analyzeSemantics before any of these
			EVO_NODISCARD auto lowerToPIR(EntryKind entry_kind, pir::Module& module) -> evo::Result<>;
			EVO_NODISCARD auto runEntry(bool allow_default_symbol_linking = false) -> evo::Result<uint8_t>;

			EVO_NODISCARD auto lowerToLLVMIR(pir::Module& module) -> evo::Result<std::string>;
			EVO_NODISCARD auto lowerToAssembly(pir::Module& module) -> evo::Result<std::string>;
			EVO_NODISCARD auto lowerToObject(pir::Module& module) -> evo::Result<std::vector<evo::byte>>;




			///////////////////////////////////
			// adding sources

			EVO_NODISCARD auto addSourceFile(
				const std::filesystem::path& path, Source::ProjectConfig::ID project_config
			) -> AddSourceResult;

			EVO_NODISCARD auto addSourceDirectory(
				const std::filesystem::path& path, Source::ProjectConfig::ID project_config
			) -> AddSourceResult;

			EVO_NODISCARD auto addSourceDirectoryRecursive(
				const std::filesystem::path& path, Source::ProjectConfig::ID project_config
			) -> AddSourceResult;

			EVO_NODISCARD auto addStdLib(const std::filesystem::path& directory) -> AddSourceResult;

			EVO_NODISCARD auto addCHeaderFile(const std::filesystem::path& path, bool add_includes_to_pub_api)
				-> AddSourceResult;
			EVO_NODISCARD auto addCPPHeaderFile(const std::filesystem::path& path, bool add_includes_to_pub_api)
				-> AddSourceResult;



			///////////////////////////////////
			// emitting diagnostics

			auto emitFatal(auto&&... args) -> void {
				this->num_errors += 1;
				if(this->encountered_fatal.exchange(true)){ return; }
				this->emit_diagnostic_impl(Diagnostic(Diagnostic::Level::FATAL, std::forward<decltype(args)>(args)...));
				this->clear_work_queue_if_needed();
			}

			auto emitError(auto&&... args) -> void {
				this->num_errors += 1;
				if(this->hasHitFailCondition() == false){
					this->emit_diagnostic_impl(
						Diagnostic(Diagnostic::Level::ERROR, std::forward<decltype(args)>(args)...)
					);
				}else{
					this->clear_work_queue_if_needed();
				}
			}

			auto emitWarning(auto&&... args) -> void {
				this->emit_diagnostic_impl(
					Diagnostic(Diagnostic::Level::WARNING, std::forward<decltype(args)>(args)...)
				);
			}


		private:
			EVO_NODISCARD auto load_source(
				std::filesystem::path&& path, Source::ProjectConfig::ID project_config_id
			) -> evo::Result<Source::ID>;

			auto tokenize_impl(
				std::filesystem::path&& path, Source::ProjectConfig::ID project_config_id
			) -> void;

			auto parse_impl(
				std::filesystem::path&& path, Source::ProjectConfig::ID project_config_id
			) -> void;
			
			auto build_symbol_procs_impl(
				std::filesystem::path&& path, Source::ProjectConfig::ID project_config_id
			) -> evo::Result<Source::ID>;

			auto analyze_clang_header_impl(std::filesystem::path&& path, bool add_includes_to_pub_api, bool is_cpp)
				-> void;


			enum class LookupSourceIDError{
				EMPTY_PATH,
				SAME_AS_CALLER,
				NOT_ONE_OF_SOURCES,
				DOESNT_EXIST,
				FAILED_DURING_ANALYSIS_OF_NEWLY_LOADED,
				WRONG_LANGUAGE,
			};
			EVO_NODISCARD auto lookupSourceID(std::string_view lookup_path, const Source& calling_source)
				-> evo::Expected<Source::ID, LookupSourceIDError>;
			EVO_NODISCARD auto lookupClangSourceID(
				std::string_view lookup_path,
				const Source& calling_source,
				bool is_cpp
			) -> evo::Expected<ClangSource::ID, LookupSourceIDError>;

			auto emit_diagnostic_impl(const Diagnostic& diagnostic) -> void;


			struct FileToLoad{
				std::filesystem::path path;
				Source::ProjectConfig::ID project_config_id;
			};

			struct CHeaderToLoad{
				std::filesystem::path path;
				bool add_includes_to_pub_api;
			};

			struct CPPHeaderToLoad{
				std::filesystem::path path;
				bool add_includes_to_pub_api;
			};

			using Task = evo::Variant<FileToLoad, CHeaderToLoad, CPPHeaderToLoad, SymbolProc::ID>;

			auto add_task_to_work_manager(auto&&... args) -> void {
				if(this->hasHitFailCondition()){ return; }

				this->work_manager.visit([&](auto& work_manager) -> void {
					using WorkManager = std::decay_t<decltype(work_manager)>;

					if constexpr(std::is_same<WorkManager, std::monostate>()){
						evo::debugFatalBreak("Cannot add task to work manager as none is running");

					}else if constexpr(std::is_same<WorkManager, core::ThreadQueue<Task>>()){
						work_manager.addTask(std::forward<decltype(args)>(args)...);

					}else if constexpr(std::is_same<WorkManager, core::SingleThreadedWorkQueue<Task>>()){
						work_manager.addTask(std::forward<decltype(args)>(args)...);

					}else{
						static_assert(false, "Unsupported work manager");
					}
				});
			}

			auto clear_work_queue_if_needed() -> void {
				this->work_manager.visit([&](auto& work_manager) -> void {
					using WorkManager = std::decay_t<decltype(work_manager)>;

					if constexpr(std::is_same<WorkManager, std::monostate>()){
						return;

					}else if constexpr(std::is_same<WorkManager, core::ThreadQueue<Task>>()){
						work_manager.forceClearQueue();

					}else if constexpr(std::is_same<WorkManager, core::SingleThreadedWorkQueue<Task>>()){
						work_manager.forceClearQueue();

					}else{
						static_assert(false, "Unsupported work manager");
					}
				});
			}


			auto jit_engine_result_emit_diagnositc(const evo::SmallVector<std::string>& messages) -> void;

			EVO_NODISCARD auto register_build_system_jit_funcs(pir::JITEngine& jit_engine) -> evo::Result<>;



			auto init_builtin_modules() -> void;
			
			auto init_intrinsic_infos() -> void;

			struct IntrinsicFuncInfo{
				TypeInfoID typeID;

				bool allowedInConstexpr;
				bool allowedInComptime;
				bool allowedInRuntime;

				bool allowedInCompile;
				bool allowedInScript;
				bool allowedInBuildSystem;
			};
			EVO_NODISCARD auto getIntrinsicFuncInfo(IntrinsicFunc::Kind kind) const -> const IntrinsicFuncInfo&;


			struct TemplateIntrinsicFuncInfo{
				struct Param{
					AST::FuncDecl::Param::Kind kind;
					evo::Variant<TypeInfo::ID, uint32_t> type; // uint32_t is the index of the templateParam
				};

				using ReturnParam = evo::Variant<TypeInfo::VoidableID, uint32_t>; // uint32_t is the index 
																				  //   of the templateParam

				evo::SmallVector<std::optional<TypeInfo::ID>> templateParams; // nullopt means it's a `Type` param
				evo::SmallVector<Param> params;
				evo::SmallVector<ReturnParam> returns;

				bool allowedInConstexpr;
				bool allowedInComptime;
				bool allowedInRuntime;

				bool allowedInCompile;
				bool allowedInScript;
				bool allowedInBuildSystem;

				
				EVO_NODISCARD auto getTypeInstantiation(
					evo::ArrayProxy<std::optional<TypeInfo::VoidableID>> template_args// nullopt if is an expr argument
				) const -> BaseType::Function {
					auto instantiated_params = evo::SmallVector<BaseType::Function::Param>();
					instantiated_params.reserve(this->params.size());
					for(const Param& param : this->params){
						const TypeInfo::ID param_type = param.type.visit([&](const auto& param_type) -> TypeInfo::ID {
							if constexpr(std::is_same<std::decay_t<decltype(param_type)>, TypeInfo::ID>()){
								return param_type;
							}else{
								return template_args[param_type]->asTypeID();
							}
						});

						instantiated_params.emplace_back(param_type, param.kind, false);
					}

					auto instantiated_returns = evo::SmallVector<BaseType::Function::ReturnParam>();
					instantiated_returns.reserve(this->returns.size());
					for(const ReturnParam& return_param : this->returns){
						const TypeInfo::VoidableID return_type = return_param.visit([&](const auto& return_data){
							if constexpr(std::is_same<std::decay_t<decltype(return_data)>, TypeInfo::VoidableID>()){
								return return_data;
							}else{
								return *template_args[return_data];
							}
						});

						instantiated_returns.emplace_back(std::nullopt, return_type);
					}

					return BaseType::Function(std::move(instantiated_params), std::move(instantiated_returns), {});
				}
			};
			EVO_NODISCARD auto getTemplateIntrinsicFuncInfo(TemplateIntrinsicFunc::Kind kind)
				-> TemplateIntrinsicFuncInfo&;


	
		private:
			const Config& _config;
			BuildSystemConfig build_system_config{};

			DiagnosticCallback _diagnostic_callback;
			mutable core::SpinLock diagnostic_callback_mutex{};

			std::atomic<unsigned> num_errors = 0;
			std::atomic<bool> encountered_fatal = false;
			bool added_std_lib = false;
			bool started_any_target = false;

			std::vector<FileToLoad> files_to_load{};
			std::vector<CHeaderToLoad> c_headers_to_load{};
			std::vector<CPPHeaderToLoad> cpp_headers_to_load{};

			std::unordered_set<std::filesystem::path> current_dynamic_file_load{};
			mutable core::SpinLock current_dynamic_file_load_lock{};



			// Only used for semantic analysis
			// std::monostate is used as uninitialized state
			evo::Variant<std::monostate, core::ThreadQueue<Task>, core::SingleThreadedWorkQueue<Task>> work_manager{};

			SourceManager source_manager{};
			TypeManager type_manager;
			SymbolProcManager symbol_proc_manager{};
			SemaBuffer sema_buffer{};

			std::optional<sema::Func::ID> entry{};

			std::array<IntrinsicFuncInfo, evo::to_underlying(IntrinsicFunc::Kind::_MAX_)> intrinsic_infos{};
			std::array<
				TemplateIntrinsicFuncInfo, evo::to_underlying(TemplateIntrinsicFunc::Kind::_MAX_)
			> template_intrinsic_infos{};

			pir::Module constexpr_pir_module;
			SemaToPIR::Data constexpr_sema_to_pir_data;
			pir::JITEngine constexpr_jit_engine{};

			friend class SymbolProcBuilder;
			friend class SemanticAnalyzer;
			friend class SymbolProc;
			friend class SemaToPIR;
	};

	
}