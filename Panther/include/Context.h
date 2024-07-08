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
#include <filesystem>
namespace fs = std::filesystem;

#include <Evo.h>
#include <PCIT_core.h>

#include "./SourceManager.h"
#include "./diagnostics.h"


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
			Context(DiagnosticCallback diagnostic_callback, const Config& _config) noexcept;
			~Context() noexcept;

			Context(const Context&) = delete;
			auto operator=(const Context&) = delete;

			// Returns 0 if number is unknown
			EVO_NODISCARD static auto optimalNumThreads() noexcept -> evo::uint;

			EVO_NODISCARD auto isSingleThreaded() const noexcept -> bool { return this->config.numThreads == 0; };
			EVO_NODISCARD auto isMultiThreaded() const noexcept -> bool { return this->config.numThreads != 0; };


			EVO_NODISCARD auto threadsRunning() const noexcept -> bool;
			auto startupThreads() noexcept -> void;
			auto shutdownThreads() noexcept -> void;
			auto waitForAllTasks() noexcept -> void;


			// Loads a number of files in a multithreaded. 
			// TODO: Make sure the following comment is actually true
			// No other files should be loaded before or after calling this function. If you want more granular control,
			// 		load files through the Source Manager directly, (Call this->getSourceManager())
			auto loadFiles(evo::ArrayProxy<fs::path> file_paths) noexcept -> void;

			auto tokenizeLoadedFiles() noexcept -> void;

			auto parseLoadedFiles() noexcept -> void;

			




			EVO_NODISCARD auto errored() const noexcept -> bool { return this->num_errors != 0; };
			EVO_NODISCARD auto hasHitFailCondition() const noexcept -> bool { return this->hit_fail_condition; };

			EVO_NODISCARD auto getSourceManager()       noexcept ->       SourceManager& { return this->src_manager; };
			EVO_NODISCARD auto getSourceManager() const noexcept -> const SourceManager& { return this->src_manager; };

			EVO_NODISCARD auto getConfig() const noexcept -> const Config& { return this->config; };



			///////////////////////////////////
			// internal use only

			auto emitFatal(auto&&... args) noexcept -> void {
				this->num_errors += 1;
				if(this->num_errors <= this->config.maxNumErrors){
					this->emit_diagnostic_internal(Diagnostic::Level::Fatal, std::forward<decltype(args)>(args)...);
				}
				this->notify_task_errored();
			};

			auto emitError(auto&&... args) noexcept -> void {
				this->num_errors += 1;
				if(this->num_errors <= this->config.maxNumErrors){
					this->emit_diagnostic_internal(Diagnostic::Level::Error, std::forward<decltype(args)>(args)...);
				}
				this->notify_task_errored();
			};

			auto emitWarning(auto&&... args) noexcept -> void {
				this->emit_diagnostic_internal(Diagnostic::Level::Warning, std::forward<decltype(args)>(args)...);
			};


			auto emitDebug([[maybe_unused]] std::string_view message) noexcept -> void {
				#if defined(PCIT_BUILD_DEBUG)
					const auto lock_guard = std::lock_guard(this->callback_mutex);
					evo::log::debug(message);
				#endif
			};

			template<class... Args>
			auto emitDebug([[maybe_unused]] std::format_string<Args...> fmt, [[maybe_unused]] Args&&... args) noexcept
			-> void {
				#if defined(PCIT_BUILD_DEBUG)
					const auto lock_guard = std::lock_guard(this->callback_mutex);
					evo::log::debug(fmt, std::forward<decltype(args)>(args)...);
				#endif
			};

			auto emitTrace([[maybe_unused]] std::string_view message) noexcept -> void {
				#if defined(PCIT_BUILD_DEBUG)
					const auto lock_guard = std::lock_guard(this->callback_mutex);
					evo::log::trace(message);
				#endif
			};

			template<class... Args>
			auto emitTrace([[maybe_unused]] std::format_string<Args...> fmt, [[maybe_unused]] Args&&... args) noexcept
			-> void {
				#if defined(PCIT_BUILD_DEBUG)
					const auto lock_guard = std::lock_guard(this->callback_mutex);
					evo::log::trace(fmt, std::forward<decltype(args)>(args)...);
				#endif
			};
		private:
			auto emit_diagnostic_internal(auto&&... args) noexcept -> void {
				auto diagnostic = Diagnostic(std::forward<decltype(args)>(args)...);
				this->emit_diagnostic_impl(diagnostic);
			};

			auto emit_diagnostic_impl(const Diagnostic& diagnostic) noexcept -> void;
			auto notify_task_errored() noexcept -> void;
			auto consume_tasks_single_threaded() noexcept -> void;
	
		private:
			Config config;

			SourceManager src_manager;
			std::mutex src_manager_mutex{};


			DiagnosticCallback callback;
			std::mutex callback_mutex{};


			///////////////////////////////////
			// threading

			bool task_group_running = false;
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

			using Task = evo::Variant<LoadFileTask, TokenizeFileTask, ParseFileTask>;

			std::queue<std::unique_ptr<Task>> tasks{};
			std::mutex tasks_mutex{};


			class Worker{
				public:
					Worker() noexcept : context(nullptr) {};

					Worker(Context* _context) noexcept : context(_context) {};
					~Worker() = default;

					Worker(const Worker&) = delete;
					Worker(Worker&& rhs) noexcept 
						: context(std::exchange(rhs.context, nullptr)),
						  is_working(rhs.is_working),
						  thread(std::move(rhs.thread)) {};


					auto done() noexcept -> void;
					auto get_task() noexcept -> void;
					auto get_task_single_threaded() noexcept -> void;

					EVO_NODISCARD auto isWorking() const noexcept -> bool { return this->is_working; };

					EVO_NODISCARD auto getThread()       noexcept ->       std::jthread& { return this->thread; };
					EVO_NODISCARD auto getThread() const noexcept -> const std::jthread& { return this->thread; };

				private:
					auto run_task(const Task& task) noexcept -> void;
					auto run_load_file(const LoadFileTask& task) noexcept -> void;
					auto run_tokenize_file(const TokenizeFileTask& task) noexcept -> void;
					auto run_parse_file(const ParseFileTask& task) noexcept -> void;

				private:
					Context* context;
					bool is_working = false;
					std::jthread thread{};
			};


			std::vector<Worker> workers{};
	};


};