////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <queue>
#include <memory>
#include <mutex>
#include <filesystem>
namespace fs = std::filesystem;

#include <Evo.h>
#include <PCIT_core.h>

#include "./SourceManager.h"
#include "./diagnostics.h"
#include "./TypeManager.h"
#include "./ScopeManager.h"
#include "./intrinsics.h"
#include "../src/ComptimeExecutor.h"


namespace pcit::panther{

	class Context{
		public:
			using DiagnosticCallback = std::function<void(const Context&, const Diagnostic&)>;

			struct Config{
				fs::path basePath;

				core::OS os             = core::getCurrentOS();
				core::Architecture arch = core::getCurrentArchitecture();
				
				bool addSourceLocations = true;
				bool checkedMath        = true;

				unsigned numThreads     = 0; // 0 for single-threaded
				unsigned maxNumErrors   = 1;
				bool mayRecover         = true;
			};

			enum class LookupSourceIDError{
				EmptyPath,
				SameAsCaller,
				NotOneOfSources,
				DoesntExist,
			};

		public:
			Context(core::Printer& _printer, DiagnosticCallback diagnostic_callback, const Config& _config);
			~Context();

			Context(const Context&) = delete;
			auto operator=(const Context&) = delete;

			EVO_NODISCARD static auto optimalNumThreads() -> unsigned;

			EVO_NODISCARD auto isSingleThreaded() const -> bool { return this->config.numThreads == 0; }
			EVO_NODISCARD auto isMultiThreaded() const -> bool { return this->config.numThreads != 0; }


			EVO_NODISCARD auto threadsRunning() const -> bool;
			auto startupThreads() -> void;
			auto shutdownThreads() -> void;
			auto waitForAllTasks() -> void;


			// Loads a number of files in a multithreaded. 
			// TODO: make a task of the grouped parallelizable tasks merged into one
			auto loadFiles(evo::ArrayProxy<fs::path> file_paths) -> void;
			auto tokenizeLoadedFiles() -> void;
			auto parseLoadedFiles() -> void;
			auto semanticAnalysisLoadedFiles() -> void;

			EVO_NODISCARD auto printPIR() -> bool;
			EVO_NODISCARD auto printLLVMIR(bool add_runtime) -> evo::Result<std::string>;
			EVO_NODISCARD auto run() -> evo::Result<uint8_t>;

			EVO_NODISCARD auto lookupSourceID(const std::string_view src_path) -> evo::Result<Source::ID>;
			EVO_NODISCARD auto lookupRelativeSourceID(const fs::path& file_location, const std::string_view src_path)
				-> evo::Expected<Source::ID, LookupSourceIDError>;
			
			EVO_NODISCARD auto setEntry(const ASG::Func::LinkID& entry_id) -> bool; // returns false if already set
			EVO_NODISCARD auto getEntry() const -> std::optional<ASG::Func::LinkID>;

			EVO_NODISCARD auto errored() const -> bool { return this->num_errors != 0; }
			EVO_NODISCARD auto hasHitFailCondition() const -> bool { return this->hit_fail_condition; }

			EVO_NODISCARD auto getSourceManager()       ->       SourceManager& { return this->src_manager; }
			EVO_NODISCARD auto getSourceManager() const -> const SourceManager& { return this->src_manager; }

			EVO_NODISCARD auto getTypeManager()       ->       TypeManager& { return this->type_manager; }
			EVO_NODISCARD auto getTypeManager() const -> const TypeManager& { return this->type_manager; }

			EVO_NODISCARD auto getConfig() const -> const Config& { return this->config; }

			EVO_NODISCARD auto getIntrinsic(Intrinsic::Kind kind) const -> const Intrinsic&;
			EVO_NODISCARD auto getTemplatedIntrinsic(TemplatedIntrinsic::Kind kind) const -> const TemplatedIntrinsic&;


			///////////////////////////////////
			// internal use only

			auto emitFatal(auto&&... args) -> void {
				this->num_errors = this->config.maxNumErrors;
				hit_fail_condition = true;
				this->emit_diagnostic_internal(Diagnostic::Level::Fatal, std::forward<decltype(args)>(args)...);
				this->notify_task_errored();
			}

			auto emitError(auto&&... args) -> void {
				this->num_errors += 1;
				if(this->num_errors <= this->config.maxNumErrors){
					this->emit_diagnostic_internal(Diagnostic::Level::Error, std::forward<decltype(args)>(args)...);
				}
				this->notify_task_errored();
			}

			auto emitWarning(auto&&... args) -> void {
				this->emit_diagnostic_internal(Diagnostic::Level::Warning, std::forward<decltype(args)>(args)...);
			}


			auto emitDebug([[maybe_unused]] std::string_view message) -> void {
				#if defined(PCIT_BUILD_DEBUG)
					const auto lock_guard = std::lock_guard(this->callback_mutex);
					evo::log::debug(message);
				#endif
			}

			template<class... Args>
			auto emitDebug([[maybe_unused]] std::format_string<Args...> fmt, [[maybe_unused]] Args&&... args)
			-> void {
				#if defined(PCIT_BUILD_DEBUG)
					const auto lock_guard = std::lock_guard(this->callback_mutex);
					evo::log::debug(fmt, std::forward<decltype(args)>(args)...);
				#endif
			}

			auto emitTrace([[maybe_unused]] std::string_view message) -> void {
				#if defined(PCIT_BUILD_DEBUG)
					const auto lock_guard = std::lock_guard(this->callback_mutex);
					evo::log::trace(message);
				#endif
			}

			template<class... Args>
			auto emitTrace([[maybe_unused]] std::format_string<Args...> fmt, [[maybe_unused]] Args&&... args)
			-> void {
				#if defined(PCIT_BUILD_DEBUG)
					const auto lock_guard = std::lock_guard(this->callback_mutex);
					evo::log::trace(fmt, std::forward<decltype(args)>(args)...);
				#endif
			}

			EVO_NODISCARD auto getScopeManager() const -> const ScopeManager& { return this->scope_manager; }
			EVO_NODISCARD auto getScopeManager()       ->       ScopeManager& { return this->scope_manager; }

		private:
			auto wait_for_all_current_tasks() -> void;

			auto emit_diagnostic_internal(auto&&... args) -> void {
				auto diagnostic = Diagnostic(std::forward<decltype(args)>(args)...);
				this->emit_diagnostic_impl(diagnostic);
			}

			auto emit_diagnostic_impl(const Diagnostic& diagnostic) -> void;
			auto notify_task_errored() -> void;
			auto consume_tasks_single_threaded() -> void;


			auto init_intrinsics() -> void;
	
		private:
			Config config;
			core::Printer& printer;

			SourceManager src_manager;
			mutable core::SpinLock src_manager_mutex{};

			TypeManager type_manager;

			DiagnosticCallback callback;
			mutable core::SpinLock callback_mutex{};

			ScopeManager scope_manager{};

			std::optional<ASG::Func::LinkID> entry{};
			mutable core::SpinLock entry_mutex{};

			std::array<Intrinsic, size_t(Intrinsic::Kind::_max_)> intrinsics{};
			std::array<TemplatedIntrinsic, size_t(TemplatedIntrinsic::Kind::_max_)> templated_intrinsics{};

			ComptimeExecutor comptime_executor{*this, this->printer};

			friend class SemanticAnalyzer; // to access comptime_executor


			///////////////////////////////////
			// threading

			bool task_group_running = false;
			bool multiple_task_stages_left = false;
			std::atomic<unsigned> num_errors = 0;
			std::atomic<unsigned> num_threads_running = 0;
			std::atomic<bool> hit_fail_condition = false;
			std::atomic_flag shutting_down_threads{};

			struct LoadFileTask{
				fs::path path;
				int num;
			};

			struct TokenizeFileTask{
				Source::ID source_id;
			};

			struct ParseFileTask{
				Source::ID source_id;
			};

			struct LoadTokenizeAndParseFileTask{
				fs::path path;
				int num;
			};

			struct SemaGlobalDeclsTask{
				Source::ID source_id;
			};

			struct SemaGlobalStmtsComptimeTask{
				Source::ID source_id;
			};

			struct SemaGlobalStmtsRuntimeTask{
				Source::ID source_id;
			};

			using Task = evo::Variant<
				LoadFileTask,
				TokenizeFileTask,
				ParseFileTask,
				LoadTokenizeAndParseFileTask,
				SemaGlobalDeclsTask,
				SemaGlobalStmtsComptimeTask,
				SemaGlobalStmtsRuntimeTask
			>;

			std::queue<std::unique_ptr<Task>> tasks{};
			core::SpinLock tasks_mutex{};


			class Worker{
				public:
					Worker() : context(nullptr) {}

					Worker(Context* _context) : context(_context) {}
					~Worker() = default;

					Worker(const Worker&) = delete;
					Worker(Worker&& rhs) 
						: context(std::exchange(rhs.context, nullptr)),
						  is_working(rhs.is_working),
						  thread(std::move(rhs.thread)) {}


					auto done() -> void;
					auto get_task() -> void;
					auto get_task_single_threaded() -> void;

					EVO_NODISCARD auto isWorking() const -> bool { return this->is_working; }

					EVO_NODISCARD auto getThread()       ->       std::jthread& { return this->thread; }
					EVO_NODISCARD auto getThread() const -> const std::jthread& { return this->thread; }

				private:
					auto run_task(const Task& task) -> void;
					auto run_load_file(const LoadFileTask& task) -> void;
					auto run_tokenize_file(const TokenizeFileTask& task) -> void;
					auto run_parse_file(const ParseFileTask& task) -> void;
					auto run_sema_global_decls(const SemaGlobalDeclsTask& task) -> void;
					auto run_sema_global_comptime_stmts(const SemaGlobalStmtsComptimeTask& task) -> void;
					auto run_sema_global_runtime_stmts(const SemaGlobalStmtsRuntimeTask& task) -> void;


				private:
					Context* context;
					bool is_working = false;
					std::jthread thread{};
			};


			evo::SmallVector<Worker> workers{};
	};


}