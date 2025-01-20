////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <filesystem>

#include <Evo.h>
#include <PCIT_core.h>

#include "./source/SourceManager.h"
#include "./Diagnostic.h"
#include "./DG/DGBuffer.h"


namespace pcit::panther{


	class Context{
		public:
			using DiagnosticCallback = std::function<void(Context&, const Diagnostic&)>;

			struct Config{
				enum class Mode{
					Compile,
					Scripting,
					BuildSystem = Scripting,
				};

				Mode mode;
				core::OS os;
				core::Architecture architecture;

				uint32_t maxNumErrors = std::numeric_limits<uint32_t>::max();
				uint32_t numThreads = 0; // 0 means single-threaded

				EVO_NODISCARD auto isMultiThreaded() const -> bool { return this->numThreads > 0; }
			};


			enum class AddSourceResult{
				Success,
				DoesntExist,
				NotDirectory,
				StdNotAbsolute,
			};

		public:
			Context(const DiagnosticCallback& diagnostic_callback, const Config& config)
				: _diagnostic_callback(diagnostic_callback), _config(config) {
				evo::debugAssert(config.os != core::OS::Unknown, "OS must be known");
				evo::debugAssert(config.architecture != core::Architecture::Unknown, "Architecture must be known");
			}

			Context(DiagnosticCallback&& diagnostic_callback, const Config& config)
				: _diagnostic_callback(std::move(diagnostic_callback)), _config(config) {
				evo::debugAssert(config.os != core::OS::Unknown, "OS must be known");
				evo::debugAssert(config.architecture != core::Architecture::Unknown, "Architecture must be known");
			}

			~Context() = default;


			EVO_NODISCARD static auto optimalNumThreads() -> unsigned;


			EVO_NODISCARD auto hasHitFailCondition() const -> bool {
				return this->num_errors >= this->_config.maxNumErrors;
			}

			EVO_NODISCARD auto mayAddSourceFile() const -> bool {
				return this->_config.mode == Config::Mode::Scripting || this->source_manager.size() > 0;
			}


			///////////////////////////////////
			// getters

			EVO_NODISCARD auto getNumErrors() const -> unsigned { return this->num_errors.load(); }

			EVO_NODISCARD auto getSourceManager() const -> const SourceManager& { return this->source_manager; }
			EVO_NODISCARD auto getSourceManager()       ->       SourceManager& { return this->source_manager; }

			EVO_NODISCARD auto getConfig() const -> const Config& { return this->_config; }


			///////////////////////////////////
			// build targets

			auto tokenize() -> bool;
			auto parse() -> bool; 
			auto analyzeDependencies() -> bool;
			auto analyzeSemantics() -> bool;



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
				this->num_errors = this->_config.maxNumErrors;
				this->emit_diagnostic_impl(Diagnostic(Diagnostic::Level::Fatal, std::forward<decltype(args)>(args)...));
			}

			auto emitError(auto&&... args) -> void {
				this->num_errors += 1;
				if(this->hasHitFailCondition() == false){
					this->emit_diagnostic_impl(
						Diagnostic(Diagnostic::Level::Error, std::forward<decltype(args)>(args)...)
					);
				}
			}

			auto emitWarning(auto&&... args) -> void {
				this->emit_diagnostic_impl(
					Diagnostic(Diagnostic::Level::Warning, std::forward<decltype(args)>(args)...)
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

			#if defined(PCIT_CONFIG_DEBUG)
				EVO_NODISCARD auto getDGBuffer() const -> const DGBuffer& { return this->dg_buffer; }
			#endif

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
			
			auto dependency_analysis_impl(
				std::filesystem::path&& path, Source::CompilationConfig::ID compilation_config_id
			) -> evo::Result<Source::ID>;


			auto emit_diagnostic_impl(const Diagnostic& diagnostic) -> void;


			struct FileToLoad{
				std::filesystem::path path;
				Source::CompilationConfig::ID compilation_config_id;
			};

			struct SemaDecl{    DG::Node::ID dg_node_id; };
			struct SemaDef{     DG::Node::ID dg_node_id; };
			struct SemaDeclDef{ DG::Node::ID dg_node_id; };

			using Task = evo::Variant<FileToLoad, SemaDecl, SemaDef, SemaDeclDef>;

			auto add_task_to_work_manager(auto&&... args) -> void {
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
	
		private:
			const Config& _config;

			DiagnosticCallback _diagnostic_callback;
			mutable core::SpinLock diagnostic_callback_mutex{};

			std::atomic<unsigned> num_errors = 0;
			bool added_std_lib = false;

			std::vector<FileToLoad> files_to_load{};


			// Only used for semantic analysis
			// std::monostate is used as uninitialized state
			evo::Variant<std::monostate, core::ThreadQueue<Task>, core::SingleThreadedWorkQueue<Task>> work_manager{};

			SourceManager source_manager{};
			DGBuffer dg_buffer{};

			friend class DependencyAnalysis;
			friend class SemanticAnalyzer;
	};

	
}