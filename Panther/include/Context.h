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
				NumThreads(uint32_t num_threads) : num(num_threads){
					evo::debugAssert(num_threads != 0, "For single threaded, use NumThreads::single()");
				}

				EVO_NODISCARD auto isSingle() const -> bool { return this->num == 0; }
				EVO_NODISCARD auto isMulti() const -> bool { return this->num != 0; }
				EVO_NODISCARD auto getNum() const -> uint32_t {
					evo::debugAssert(this->isSingle() == false, "Cannot get num threads when is single-threaded");
					return num;
				}

				EVO_NODISCARD static auto single() -> NumThreads { return evo::bitCast<NumThreads>(uint32_t(0)); }
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
				core::Platform platform;

				uint32_t maxNumErrors = std::numeric_limits<uint32_t>::max();
				NumThreads numThreads = NumThreads::single();
			};

			struct BuildSystemConfig{
				enum class Output : uint32_t {
					PRINT_TOKENS       = 0,
					PRINT_AST          = 1,
					BUILD_SYMBOL_PROCS = 2,
					ANALYZE_SEMANTICS  = 3,
					PRINT_PIR          = 4,
					PRINT_ASSEMBLY     = 5,
					RUN                = 6,
				};


				Output output         = Output::RUN;
				NumThreads numThreads = NumThreads::single();
				bool useStdLib        = true;
			};

			enum class AddSourceResult{
				SUCCESS,
				DOESNT_EXIST,
				NOT_DIRECTORY,
				// STD_NOT_ABSOLUTE,
			};

		public:
			Context(const DiagnosticCallback& diagnostic_callback, const Config& config)
				: _diagnostic_callback(diagnostic_callback),
				_config(config),
				type_manager(config.platform),
				constexpr_pir_module(evo::copy(config.title), config.platform),
				constexpr_sema_to_pir_data(SemaToPIR::Data::Config{
					#if defined(PCIT_CONFIG_DEBUG)
						.useReadableNames = true,
					#else
						.useReadableNames = false,
					#endif
					.checkedMath        = true,
					.isJIT              = true,
					.addSourceLocations = true,
				})
			{
				evo::debugAssert(config.platform.os != core::Platform::OS::UNKNOWN, "OS must be known");
				evo::debugAssert(
					config.platform.arch != core::Platform::Architecture::UNKNOWN, "Architecture must be known"
				);
			}

			Context(DiagnosticCallback&& diagnostic_callback, const Config& config)
				: _diagnostic_callback(std::move(diagnostic_callback)),
				_config(config),
				type_manager(config.platform),
				constexpr_pir_module(evo::copy(config.title), config.platform),
				constexpr_sema_to_pir_data(SemaToPIR::Data::Config{
					#if defined(PCIT_CONFIG_DEBUG)
						.useReadableNames = true,
					#else
						.useReadableNames = false,
					#endif
					.checkedMath        = true,
					.isJIT              = true,
					.addSourceLocations = true,
				})
			{
				evo::debugAssert(config.platform.os != core::Platform::OS::UNKNOWN, "OS must be known");
				evo::debugAssert(
					config.platform.arch != core::Platform::Architecture::UNKNOWN, "Architecture must be known"
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
			auto analyzeSemantics() -> evo::Result<>;

			auto lowerToAndPrintPIR(core::Printer& printer) -> void;
			auto lowerToASM() -> evo::Result<std::string>;

			EVO_NODISCARD auto runEntry() -> evo::Result<uint8_t>;



			///////////////////////////////////
			// adding sources

			EVO_NODISCARD auto addSourceFile(
				const std::filesystem::path& path, Source::CompilationConfig::ID compilation_config
			) -> AddSourceResult;

			EVO_NODISCARD auto addSourceDirectory(
				const std::filesystem::path& path, Source::CompilationConfig::ID compilation_config
			) -> AddSourceResult;

			EVO_NODISCARD auto addSourceDirectoryRecursive(
				const std::filesystem::path& path, Source::CompilationConfig::ID compilation_config
			) -> AddSourceResult;

			EVO_NODISCARD auto addStdLib(const std::filesystem::path& directory) -> AddSourceResult;



			///////////////////////////////////
			// emitting diagnostics

			auto emitFatal(auto&&... args) -> void {
				this->num_errors += 1;
				if(this->encountered_fatal.exchange(true) == true){ return; }
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


			///////////////////////////////////
			// internal debug

			auto trace([[maybe_unused]] std::string_view message) -> void {
				#if defined(PCIT_BUILD_DEBUG)
					const auto lock_guard = std::lock_guard(this->diagnostic_callback_mutex);
					evo::log::trace(message);
				#endif
			}

			template<class... Args>
			auto trace([[maybe_unused]] std::format_string<Args...> fmt, [[maybe_unused]] Args&&... args)
			-> void {
				#if defined(PCIT_BUILD_DEBUG)
					const auto lock_guard = std::lock_guard(this->diagnostic_callback_mutex);
					evo::log::trace(fmt, std::forward<decltype(args)>(args)...);
				#endif
			}


		private:
			EVO_NODISCARD auto load_source(
				std::filesystem::path&& path, Source::CompilationConfig::ID compilation_config_id
			) -> evo::Result<Source::ID>;

			auto tokenize_impl(
				std::filesystem::path&& path, Source::CompilationConfig::ID compilation_config_id
			) -> void;

			auto parse_impl(
				std::filesystem::path&& path, Source::CompilationConfig::ID compilation_config_id
			) -> void;
			
			auto build_symbol_procs_impl(
				std::filesystem::path&& path, Source::CompilationConfig::ID compilation_config_id
			) -> evo::Result<Source::ID>;


			enum class LookupSourceIDError{
				EMPTY_PATH,
				SAME_AS_CALLER,
				NOT_ONE_OF_SOURCES,
				DOESNT_EXIST,
				FAILED_DURING_ANALYSIS_OF_NEWLY_LOADED,
			};
			EVO_NODISCARD auto lookupSourceID(std::string_view lookup_path, const Source& calling_source)
				-> evo::Expected<Source::ID, LookupSourceIDError>;

			auto emit_diagnostic_impl(const Diagnostic& diagnostic) -> void;


			struct FileToLoad{
				std::filesystem::path path;
				Source::CompilationConfig::ID compilation_config_id;
			};

			// TODO: needed anymore?
			using Task = evo::Variant<FileToLoad, SymbolProc::ID>;

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



			struct IntrinsicFuncInfo{
				TypeInfoID typeID;

				bool allowedInConstexpr;
				bool allowedInRuntime;

				bool allowedInCompile;
				bool allowedInScript;
				bool allowedInBuildSystem;
			};

			auto initIntrinsicInfos() -> void;
			EVO_NODISCARD auto getIntrinsicFuncInfo(IntrinsicFunc::Kind kind) const -> const IntrinsicFuncInfo&;

	
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

			std::array<IntrinsicFuncInfo, evo::to_underlying(IntrinsicFunc::Kind::_max_)> intrinsic_infos{};

			pir::Module constexpr_pir_module;
			SemaToPIR::Data constexpr_sema_to_pir_data;
			pir::JITEngine constexpr_jit_engine{};

			friend class SymbolProcBuilder;
			friend class SemanticAnalyzer;
			friend class SymbolProc;
			friend class SemaToPIR;
	};

	
}