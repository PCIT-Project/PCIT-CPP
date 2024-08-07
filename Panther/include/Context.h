//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


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
#include "../src/sema/ScopeManager.h"


namespace pcit::panther{


	class Context{
		public:
			using DiagnosticCallback = std::function<void(const Context&, const Diagnostic&)>;

			struct Config{
				evo::uint numThreads   = 0;
				evo::uint maxNumErrors = 1;
			};

		public:
			// pass 0 for _num_threads for single threaded
			Context(DiagnosticCallback diagnostic_callback, const Config& _config);
			~Context();

			Context(const Context&) = delete;
			auto operator=(const Context&) = delete;

			// Returns 0 if number is unknown
			EVO_NODISCARD static auto optimalNumThreads() -> evo::uint;

			EVO_NODISCARD auto isSingleThreaded() const -> bool { return this->config.numThreads == 0; }
			EVO_NODISCARD auto isMultiThreaded() const -> bool { return this->config.numThreads != 0; }


			EVO_NODISCARD auto threadsRunning() const -> bool;
			auto startupThreads() -> void;
			auto shutdownThreads() -> void;
			auto waitForAllTasks() -> void;


			// Loads a number of files in a multithreaded. 
			// TODO: Make sure the following comment is actually true
			// No other files should be loaded before or after calling this function. If you want more granular control,
			// 		load files through the Source Manager directly, (Call this->getSourceManager())
			auto loadFiles(evo::ArrayProxy<fs::path> file_paths) -> void;
			auto tokenizeLoadedFiles() -> void;
			auto parseLoadedFiles() -> void;
			auto semanticAnalysisLoadedFiles() -> void;

			

			EVO_NODISCARD auto errored() const -> bool { return this->num_errors != 0; }
			EVO_NODISCARD auto hasHitFailCondition() const -> bool { return this->hit_fail_condition; }

			EVO_NODISCARD auto getSourceManager()       ->       SourceManager& { return this->src_manager; }
			EVO_NODISCARD auto getSourceManager() const -> const SourceManager& { return this->src_manager; }

			EVO_NODISCARD auto getTypeManager()       ->       TypeManager& { return this->type_manager; }
			EVO_NODISCARD auto getTypeManager() const -> const TypeManager& { return this->type_manager; }

			EVO_NODISCARD auto getConfig() const -> const Config& { return this->config; }



			///////////////////////////////////
			// internal use only

			auto emitFatal(auto&&... args) -> void {
				this->num_errors += 1;
				if(this->num_errors <= this->config.maxNumErrors){
					this->emit_diagnostic_internal(Diagnostic::Level::Fatal, std::forward<decltype(args)>(args)...);
				}
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

			EVO_NODISCARD auto getScopeManager() const -> const sema::ScopeManager& { return this->scope_manager; }
			EVO_NODISCARD auto getScopeManager()       ->       sema::ScopeManager& { return this->scope_manager; }

		private:
			auto wait_for_all_current_tasks() -> void;

			auto emit_diagnostic_internal(auto&&... args) -> void {
				auto diagnostic = Diagnostic(std::forward<decltype(args)>(args)...);
				this->emit_diagnostic_impl(diagnostic);
			}

			auto emit_diagnostic_impl(const Diagnostic& diagnostic) -> void;
			auto notify_task_errored() -> void;
			auto consume_tasks_single_threaded() -> void;
	
		private:
			Config config;

			SourceManager src_manager;
			std::mutex src_manager_mutex{};

			TypeManager type_manager{};

			DiagnosticCallback callback;
			std::mutex callback_mutex{};

			sema::ScopeManager scope_manager{};


			///////////////////////////////////
			// threading

			bool task_group_running = false;
			bool multiple_task_stages_left = false;
			std::atomic<evo::uint> num_errors = 0;
			std::atomic<evo::uint> num_threads_running = 0;
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

			struct SemaGlobalDeclsTask{
				Source::ID source_id;
			};

			using Task = evo::Variant<LoadFileTask, TokenizeFileTask, ParseFileTask, SemaGlobalDeclsTask>;

			std::queue<std::unique_ptr<Task>> tasks{};
			std::mutex tasks_mutex{};


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

				private:
					Context* context;
					bool is_working = false;
					std::jthread thread{};
			};


			evo::SmallVector<Worker> workers{};
	};


}