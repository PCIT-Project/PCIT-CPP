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

#include <Evo.hpp>
#include <PCIT_core.hpp>
#include <PIR.hpp>

#include "./source/SourceManager.hpp"
#include "./Diagnostic.hpp"
#include "./TypeManager.hpp"
#include "../../src/symbol_proc/SymbolProcManager.hpp"
#include "./sema/SemaBuffer.hpp"
#include "../../src/sema_to_pir/SemaToPIR.hpp"


namespace pcit::panther{


	class Context{
		public:
			using DiagnosticCallback = std::function<void(Context&, const Diagnostic&)>;

			struct NumThreads{
				NumThreads(uint32_t num_threads) : num(num_threads){} // 0 is single-threaded

				[[nodiscard]] auto isSingle() const -> bool { return this->num == 0; }
				[[nodiscard]] auto isMulti() const -> bool { return this->num != 0; }
				[[nodiscard]] auto getNum() const -> uint32_t {
					evo::debugAssert(this->isSingle() == false, "Cannot get num threads when is single-threaded");
					return num;
				}

				[[nodiscard]] static auto single() -> NumThreads { return NumThreads(0); }
				[[nodiscard]] static auto optimalMulti() -> NumThreads;

				private:
					uint32_t num;
			};


			struct Config{
				enum class Mode : uint32_t {
					COMPILE     = 0,
					COMPILE_RUN = 1,
					SCRIPT      = 2,
					BUILD       = 3,
				};

				enum class WindowsSubsystem : uint32_t {
					CONSOLE = 0, // console application
					WINDOWS = 1, // application doesn't require a console

					// less used
					// BOOT_APPLICATION        = 2, // application that runs in the Windows boot environment
					// EFI_APPLICATION         = 3, // Extensible Firmare Interface subsystem
					// EFI_BOOT_SERVICE_DRIVER = 4, // Extensible Firmare Interface subsystem
					// EFI_ROM                 = 5, // Extensible Firmare Interface subsystem
					// EFI_RUNTIME_DRIVER      = 6, // Extensible Firmare Interface subsystem
					// NATIVE                  = 7, // kernel mode drivers for Windows NT
					// POSIX_IN_WINDOWS_NT     = 8, // application that runs with the POSIX subsystem in Windows NT
				};

				std::string title;
				core::Target target;
				Mode mode;
				std::optional<WindowsSubsystem> windowsSubsystem;
				pir::OptMode optMode;
				std::filesystem::path compilerExecutablePath;
				std::filesystem::path workingDirectory;

				bool includeDebugInfo = true;
				
				uint32_t maxNumErrors = std::numeric_limits<uint32_t>::max();
				NumThreads numThreads = NumThreads::single();
			};

			struct PantherBuildConfig{
				struct StringRef{
					const char* data;
					size_t size;

					[[nodiscard]] operator std::string_view() const { return std::string_view(this->data, this->size); }
					[[nodiscard]] operator std::string() const { return std::string(this->data, this->size); }
				};

				template<class T>
				struct Optional{
					[[nodiscard]] auto operator*() const -> const T& {
						evo::debugAssert(this->has_value(), "Optional doesn't have value");
						return this->data.value;
					}
					[[nodiscard]] auto operator->() const -> const T* {
						evo::debugAssert(this->has_value(), "Optional doesn't have value");
						return &this->data.value;
					}

					[[nodiscard]] auto has_value() const -> bool { return this->_has_value; }


					[[nodiscard]] operator std::optional<T>() {
						if(this->has_value()){
							return std::optional<T>(this->data.value);
						}else{
							return std::nullopt;
						}
					}

					
					private:
						union{
							std::byte dummy;
							std::remove_cv_t<T> value;
						} data;
						bool _has_value;
				};


				struct Output{
					enum class Tag : uint8_t {
						TOKENS            = 0,
						AST               = 1,
						SEMANTIC_ANALYSIS = 2,
						PIR               = 3,
						LLVMIR            = 4,
						ASSEMBLY          = 5,
						OBJECT            = 6,
						RUN               = 7,
						EXECUTABLE        = 8,
					};

					struct TokensData{
						Optional<StringRef> path;
					};

					struct ASTData{
						Optional<StringRef> path;
					};

					struct PIRData{
						Optional<StringRef> path;
					};

					struct LLVMIRData{
						Optional<StringRef> path;
					};

					struct AssemblyData{
						Optional<StringRef> path;
					};

					struct ObjectData{
						StringRef path;
					};

					struct ExecutableData{
						StringRef path;
						StringRef objectPath;
						evo::ArrayProxy<StringRef> linkPaths;
					};


					[[nodiscard]] auto getTag() const -> Tag { return this->tag; }

					[[nodiscard]] auto tokensData() const -> const TokensData& {
						evo::debugAssert(this->tag == Tag::TOKENS, "Not an tokens output");
						return this->data.tokens;
					}

					[[nodiscard]] auto astData() const -> const ASTData& {
						evo::debugAssert(this->tag == Tag::AST, "Not an ast output");
						return this->data.ast;
					}

					[[nodiscard]] auto pirData() const -> const PIRData& {
						evo::debugAssert(this->tag == Tag::PIR, "Not an pir output");
						return this->data.pir;
					}

					[[nodiscard]] auto llvmirData() const -> const LLVMIRData& {
						evo::debugAssert(this->tag == Tag::LLVMIR, "Not an llvmir output");
						return this->data.llvmir;
					}

					[[nodiscard]] auto assemblyData() const -> const AssemblyData& {
						evo::debugAssert(this->tag == Tag::ASSEMBLY, "Not an assembly output");
						return this->data.assembly;
					}

					[[nodiscard]] auto objectData() const -> const ObjectData& {
						evo::debugAssert(this->tag == Tag::OBJECT, "Not an object output");
						return this->data.object;
					}

					[[nodiscard]] auto executableData() const -> const ExecutableData& {
						evo::debugAssert(this->tag == Tag::EXECUTABLE, "Not an executable output");
						return this->data.executable;
					}


					private:
						union Data{
							std::byte dummy;
							TokensData tokens;
							ASTData ast;
							PIRData pir;
							LLVMIRData llvmir;
							AssemblyData assembly;
							ObjectData object;
							ExecutableData executable;
						};

						Data data;
						Tag tag;
				};


				struct Package{
					struct Directory{
						StringRef path;
						bool isRecursive;
					};


					struct Option{
						struct Value{
							enum class Tag : uint8_t {
								BOOLEAN = 0,
								UI8     = 1,
								UI16    = 2,
								UI32    = 3,
								UI64    = 4,
								I8      = 5,
								I16     = 6,
								I32     = 7,
								I64     = 8,
								F32     = 9,
								F64     = 10,
								STRING  = 11,
							};

							[[nodiscard]] auto getTag() const -> Tag { return this->tag; }

							[[nodiscard]] auto boolValue() const -> bool {
								evo::debugAssert(this->tag == Tag::BOOLEAN, "Not a bool value");
								return this->data.boolean;
							}

							[[nodiscard]] auto ui8Value() const -> uint8_t {
								evo::debugAssert(this->tag == Tag::UI8, "Not a ui8 value");
								return this->data.ui8;
							}

							[[nodiscard]] auto ui16Value() const -> uint16_t {
								evo::debugAssert(this->tag == Tag::UI16, "Not a ui16 value");
								return this->data.ui16;
							}

							[[nodiscard]] auto ui32Value() const -> uint32_t {
								evo::debugAssert(this->tag == Tag::UI32, "Not a ui32 value");
								return this->data.ui32;
							}

							[[nodiscard]] auto ui64Value() const -> uint64_t {
								evo::debugAssert(this->tag == Tag::UI64, "Not a ui64 value");
								return this->data.ui64;
							}

							[[nodiscard]] auto i8Value() const -> int8_t {
								evo::debugAssert(this->tag == Tag::I8, "Not a i8 value");
								return this->data.i8;
							}

							[[nodiscard]] auto i16Value() const -> int16_t {
								evo::debugAssert(this->tag == Tag::I16, "Not a i16 value");
								return this->data.i16;
							}

							[[nodiscard]] auto i32Value() const -> int32_t {
								evo::debugAssert(this->tag == Tag::I32, "Not a i32 value");
								return this->data.i32;
							}

							[[nodiscard]] auto i64Value() const -> int64_t {
								evo::debugAssert(this->tag == Tag::I64, "Not a i64 value");
								return this->data.i64;
							}

							[[nodiscard]] auto f32Value() const -> float32_t {
								evo::debugAssert(this->tag == Tag::F32, "Not a f32 value");
								return this->data.f32;
							}

							[[nodiscard]] auto f64Value() const -> float64_t {
								evo::debugAssert(this->tag == Tag::F64, "Not a f64 value");
								return this->data.f64;
							}

							[[nodiscard]] auto stringValue() const -> std::string_view {
								evo::debugAssert(this->tag == Tag::STRING, "Not a string value");
								return static_cast<std::string_view>(this->data.string);
							}


							private:
								union Data{
									bool boolean;
									uint8_t ui8;
									uint16_t ui16;
									uint32_t ui32;
									uint64_t ui64;
									int8_t i8;
									int16_t i16;
									int32_t i32;
									int64_t i64;
									float32_t f32;
									float64_t f64;
									StringRef string;
								};

								Data data;
								Tag tag;
						};

						StringRef name;
						Value value;
					};

					StringRef path;
					StringRef name;
					Source::Package::Warns warns;
					evo::ArrayProxy<Option> options;
					evo::ArrayProxy<StringRef> sourceFiles;
					evo::ArrayProxy<Directory> sourceDirectories;
				};

				struct CFamilyHeader{
					StringRef path;
					evo::ArrayProxy<StringRef> systemIncludeDirectories;
					evo::ArrayProxy<StringRef> includeDirectories;
					bool isCPP;
					bool addIncludesToPubApi;
				};



				StringRef title;
				Output output;
				core::Target::Architecture architecture;
				core::Target::Platform platform;
				Optional<Config::WindowsSubsystem> windowsSubsystem;
				pir::OptMode optMode;
				NumThreads numThreads;
				bool addDebugInfo;

				evo::ArrayProxy<Package> packages;
				evo::ArrayProxy<CFamilyHeader> cFamilyHeaders;
			};



			enum class AddSourceResult{
				SUCCESS,
				DOESNT_EXIST,
				NOT_FILE,
				NOT_DIRECTORY,
			};

		public:
			Context(const DiagnosticCallback& diagnostic_callback, const Config& config)
				: _diagnostic_callback(diagnostic_callback),
				_config(config),
				type_manager(config.target),
				pir_module(std::string(config.title), config.target),
				sema_to_pir_data(SemaToPIR::Data::Config{
					.includeDebugInfo     = this->_config.includeDebugInfo,
					.useReadableNames     = true,
					.checkedMath          = true,
					.useDebugUnreachables = true, // TODO(FUTURE): when an option, add check in sema for `unreachable`
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
				pir_module(std::string(config.title), config.target),
				sema_to_pir_data(SemaToPIR::Data::Config{
					.includeDebugInfo     = this->_config.includeDebugInfo,
					.useReadableNames     = true,
					.checkedMath          = true,
					.useDebugUnreachables = true, // TODO(FUTURE): when an option, add check in sema for `unreachable`
				})
			{
				evo::debugAssert(config.target.platform != core::Target::Platform::UNKNOWN, "Platform must be known");
				evo::debugAssert(
					config.target.architecture != core::Target::Architecture::UNKNOWN, "Architecture must be known"
				);
			}



			[[nodiscard]] static auto optimalNumThreads() -> unsigned;


			[[nodiscard]] auto hasHitFailCondition() const -> bool {
				return this->num_errors >= this->_config.maxNumErrors || this->encountered_fatal;
			}

			[[nodiscard]] auto mayAddSourceFile() const -> bool {
				return this->_config.mode == Config::Mode::SCRIPT
					|| this->_config.mode == Config::Mode::BUILD
					|| this->started_any_target == false;
			}


			///////////////////////////////////
			// getters

			[[nodiscard]] auto getPIRModule() const -> const pir::Module& { return this->pir_module; }
			[[nodiscard]] auto getPIRModule()       ->       pir::Module& { return this->pir_module; }

			[[nodiscard]] auto getNumErrors() const -> unsigned { return this->num_errors.load(); }

			[[nodiscard]] auto getSourceManager() const -> const SourceManager& { return this->source_manager; }
			[[nodiscard]] auto getSourceManager()       ->       SourceManager& { return this->source_manager; }

			[[nodiscard]] auto getTypeManager() const -> const TypeManager& { return this->type_manager; }
			[[nodiscard]] auto getTypeManager()       ->       TypeManager& { return this->type_manager; }

			[[nodiscard]] auto getSemaBuffer() const -> const SemaBuffer& { return this->sema_buffer; }

			[[nodiscard]] auto getConfig() const -> const Config& { return this->_config; }


			///////////////////////////////////
			// build targets

			auto tokenize() -> evo::Result<>;
			auto parse() -> evo::Result<>;
			auto buildSymbolProcs() -> evo::Result<>;
			auto analyzeSemantics() -> evo::Result<>; // autmatically calls `compileOtherLangHeaders`

			auto compileOtherLangHeaders() -> evo::Result<>; // done automatically by `analyzeSemantics`


			//////////////////
			// call analyzeSemantics before any of these

			[[nodiscard]] auto lowerToPIR() -> evo::Result<>;

			[[nodiscard]] auto lowerToLLVMIR() -> evo::Result<std::string>;
			[[nodiscard]] auto lowerToAssembly() -> evo::Result<std::string>;
			[[nodiscard]] auto lowerToObject() -> evo::Result<std::vector<evo::byte>>;

			[[nodiscard]] auto runEntry(bool allow_default_symbol_linking = false) -> evo::Result<uint8_t>;


			using CreatePantherBuildCallback = std::function<evo::Result<void>(PantherBuildConfig&)>;
			[[nodiscard]] auto runBuildSystem(
				const CreatePantherBuildCallback& create_panther_build_callback,
				bool allow_default_symbol_linking = false
			) -> evo::Result<uint8_t>;



			///////////////////////////////////
			// adding sources

			[[nodiscard]] auto addSourceFile(
				const std::filesystem::path& path, Source::Package::ID package_id
			) -> AddSourceResult;

			[[nodiscard]] auto addSourceDirectory(
				const std::filesystem::path& path, Source::Package::ID package_id
			) -> AddSourceResult;

			[[nodiscard]] auto addSourceDirectoryRecursive(
				const std::filesystem::path& path, Source::Package::ID package_id
			) -> AddSourceResult;

			[[nodiscard]] auto addStdLib(Source::Package::ID package_id) -> void;

			[[nodiscard]] auto addCHeaderFile(
				const std::filesystem::path& path,
				evo::SmallVector<std::string>&& system_include_directories,
				evo::SmallVector<std::string>&& include_directories,
				bool add_includes_to_pub_api
			) -> AddSourceResult;
			[[nodiscard]] auto addCPPHeaderFile(
				const std::filesystem::path& path,
				evo::SmallVector<std::string>&& system_include_directories,
				evo::SmallVector<std::string>&& include_directories,
				bool add_includes_to_pub_api
			) -> AddSourceResult;



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
			[[nodiscard]] auto load_source(
				std::filesystem::path&& path, Source::Package::ID package_id
			) -> evo::Result<Source::ID>;

			auto tokenize_impl(
				std::filesystem::path&& path, Source::Package::ID package_id
			) -> void;

			auto parse_impl(
				std::filesystem::path&& path, Source::Package::ID package_id
			) -> void;
			
			auto build_symbol_procs_impl(
				std::filesystem::path&& path, Source::Package::ID package_id
			) -> evo::Result<Source::ID>;

			auto analyze_c_family_header_impl(
				std::filesystem::path&& path,
				evo::SmallVector<std::string>&& system_include_directories,
				evo::SmallVector<std::string>&& include_directories,
				bool add_includes_to_pub_api,
				bool is_cpp
			) -> void;


			enum class LookupSourceIDError{
				EMPTY_PATH,
				SAME_AS_CALLER,
				NOT_ONE_OF_SOURCES,
				DOESNT_EXIST,
				FAILED_DURING_ANALYSIS_OF_NEWLY_LOADED,
				WRONG_LANGUAGE,
			};
			[[nodiscard]] auto lookupSourceID(std::string_view lookup_path, const Source& calling_source)
				-> evo::Expected<Source::ID, LookupSourceIDError>;
			[[nodiscard]] auto lookupCFamilySourceID(
				std::string_view lookup_path,
				const Source& calling_source,
				bool is_cpp
			) -> evo::Expected<CFamilySource::ID, LookupSourceIDError>;

			auto emit_diagnostic_impl(const Diagnostic& diagnostic) -> void;


			struct FileToLoad{
				std::filesystem::path path;
				Source::Package::ID package_id;

				FileToLoad(const std::filesystem::path& _path, Source::Package::ID _package_id)
					: path(_path), package_id(_package_id) { 
					evo::debugAssert(this->path.empty() == false, "Cannot load empty path");
				}
				FileToLoad(std::filesystem::path&& _path, Source::Package::ID _package_id)
					: path(std::move(_path)), package_id(_package_id) { 
					evo::debugAssert(this->path.empty() == false, "Cannot load empty path");
				}
			};

			struct CHeaderToLoad{
				std::filesystem::path path;
				evo::SmallVector<std::string> system_include_directories;
				evo::SmallVector<std::string> include_directories;
				bool add_includes_to_pub_api;
			};

			struct CPPHeaderToLoad{
				std::filesystem::path path;
				evo::SmallVector<std::string> system_include_directories;
				evo::SmallVector<std::string> include_directories;
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

			auto add_task_to_work_manager(SymbolProc::ID symbol_proc_id) -> void {
				if(this->hasHitFailCondition()){ return; }

				const SymbolProc& symbol_proc = this->symbol_proc_manager.getSymbolProc(symbol_proc_id);

				evo::debugAssert(
					symbol_proc.status == SymbolProc::Status::IN_QUEUE, "Invalid status to add to work manager"
				);

				this->work_manager.visit([&](auto& work_manager) -> void {
					using WorkManager = std::decay_t<decltype(work_manager)>;

					if constexpr(std::is_same<WorkManager, std::monostate>()){
						evo::debugFatalBreak("Cannot add task to work manager as none is running");

					}else if constexpr(std::is_same<WorkManager, core::ThreadQueue<Task>>()){
						if(symbol_proc.isPriority()){
							work_manager.addPriorityTask(symbol_proc_id);
						}else{
							work_manager.addTask(symbol_proc_id);
						}

					}else if constexpr(std::is_same<WorkManager, core::SingleThreadedWorkQueue<Task>>()){
						if(symbol_proc.isPriority()){
							work_manager.addPriorityTask(symbol_proc_id);
						}else{
							work_manager.addTask(symbol_proc_id);
						}

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


			auto jit_engine_result_emit_diagnostic(const evo::SmallVector<std::string>& messages) -> void;



			auto init_builtin_modules() -> void;
			auto init_comptime_execution_engine_funcs() -> void;
			
			auto init_intrinsic_infos() -> void;

			struct IntrinsicFuncInfo{
				TypeInfoID typeID;

				bool allowedInComptime;
				bool allowedInRuntime;

				bool allowedInCompile;
				bool allowedInScript;
				bool allowedInBuild;
			};
			[[nodiscard]] auto getIntrinsicFuncInfo(IntrinsicFunc::Kind kind) const -> const IntrinsicFuncInfo&;


			struct TemplateIntrinsicFuncInfo{
				struct TemplateParam{
					[[nodiscard]] static auto createType() -> TemplateParam {
						return TemplateParam(std::nullopt);
					}

					[[nodiscard]] static auto createExpr(TypeInfo::ID expr_type) -> TemplateParam {
						return TemplateParam(expr_type);
					}

					TemplateParam(TypeInfo::ID _expr_type) : expr_type(_expr_type) {}


					[[nodiscard]] auto isType() const -> bool { return this->expr_type.has_value() == false; }
					[[nodiscard]] auto isExpr() const -> bool { return this->expr_type.has_value(); }

					[[nodiscard]] auto getExprType() const -> TypeInfo::ID {
						evo::debugAssert(this->isExpr(), "not an expr template param");
						return *this->expr_type;
					}

					private:
						TemplateParam(std::optional<TypeInfo::ID> _expr_type)
							: expr_type(_expr_type) {}

						std::optional<TypeInfo::ID> expr_type;
				};

				struct Param{
					BaseType::Function::Param::Kind kind;
					evo::Variant<TypeInfo::ID, uint32_t> type; // uint32_t is the index of the templateParam
				};

				using ReturnParam = evo::Variant<TypeInfo::VoidableID, uint32_t>; // uint32_t is the index 
																				  //   of the templateParam

				evo::SmallVector<TemplateParam> templateParams;
				evo::SmallVector<Param> params;
				evo::SmallVector<ReturnParam> returns;

				bool allowedInComptime;
				bool allowedInRuntime;

				bool allowedInCompile;
				bool allowedInScript;
				bool allowedInBuild;

				
				[[nodiscard]] auto getTypeInstantiation(
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

					auto instantiated_return_types = evo::SmallVector<TypeInfo::VoidableID>();
					instantiated_return_types.reserve(this->returns.size());
					for(const ReturnParam& return_param : this->returns){
						const TypeInfo::VoidableID return_type = return_param.visit([&](const auto& return_data){
							if constexpr(std::is_same<std::decay_t<decltype(return_data)>, TypeInfo::VoidableID>()){
								return return_data;
							}else{
								return *template_args[return_data];
							}
						});

						instantiated_return_types.emplace_back(return_type);
					}

					return BaseType::Function(
						std::move(instantiated_params),
						std::move(instantiated_return_types),
						evo::SmallVector<TypeInfo::VoidableID>(),
						BaseType::Function::Attributes{
							.isComptime        = this->allowedInComptime,
							.isRuntime         = this->allowedInRuntime,
							.isUnsafe          = false,
							.isNoReturn        = false,
							.callingConvention = pir::CallingConvention::DEFAULT,
							.abi               = BaseType::Function::ABI::PANTHER,
						},
						false
					);
				}
			};
			[[nodiscard]] auto getTemplateIntrinsicFuncInfo(TemplateIntrinsicFunc::Kind kind)
				-> TemplateIntrinsicFuncInfo&;


			[[nodiscard]] auto create_builtin_struct(
				BuiltinModule::ID builtin_module_id,
				std::string_view name,
				evo::SmallVector<BaseType::Struct::MemberVar>&& members
			) -> BaseType::Struct::ID;

	
		private:
			const Config& _config;

			DiagnosticCallback _diagnostic_callback;
			mutable evo::SpinLock diagnostic_callback_mutex{};

			std::atomic<unsigned> num_errors = 0;
			std::atomic<bool> encountered_fatal = false;
			bool added_std_lib = false;
			bool started_any_target = false;

			std::vector<FileToLoad> files_to_load{};
			std::vector<CHeaderToLoad> c_headers_to_load{};
			std::vector<CPPHeaderToLoad> cpp_headers_to_load{};

			std::unordered_set<std::filesystem::path> current_dynamic_file_load{};
			mutable evo::SpinLock current_dynamic_file_load_lock{};

			std::unordered_map<std::filesystem::path, LookupSourceIDError> current_dynamic_file_load_failed{};
			mutable evo::SpinLock current_dynamic_file_load_failed_lock{};



			// Only used for semantic analysis
			// std::monostate is used as uninitialized state
			evo::Variant<std::monostate, core::ThreadQueue<Task>, core::SingleThreadedWorkQueue<Task>> work_manager{};

			SourceManager source_manager{};
			TypeManager type_manager;
			SymbolProcManager symbol_proc_manager{};
			SemaBuffer sema_buffer{};

			std::atomic<std::optional<sema::Func::ID>> entry{};
			std::optional<sema::Func::ID> panic{}; 
			std::atomic<bool> expecting_entry = false;
			std::atomic<bool> expecting_panic = false;

			std::array<IntrinsicFuncInfo, size_t(IntrinsicFunc::Kind::_LAST_) + 1> intrinsic_infos{};
			std::array<
				TemplateIntrinsicFuncInfo, size_t(TemplateIntrinsicFunc::Kind::_LAST_) + 1
			> template_intrinsic_infos{};

			pir::Module pir_module;
			SemaToPIR::Data sema_to_pir_data;
			pir::ExecutionEngine comptime_execution_engine{pir_module};


			const CreatePantherBuildCallback* _create_panther_build_callback = nullptr;


			friend class SymbolProcBuilder;
			friend class SemanticAnalyzer;
			friend class SymbolProc;
			friend class SymbolProcManager;
			friend class SemaToPIR;
	};

	
}