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
				evo::uint maxNumErrors = 0;
			};

		public:
			// pass 0 for _num_threads for single threaded
			Context(DiagnosticCallback diagnostic_callback, const Config& _config) noexcept;
			~Context() noexcept;

			// Returns 0 if number is unknown
			EVO_NODISCARD static auto optimalNumThreads() noexcept -> evo::uint;

			EVO_NODISCARD auto isSingleThreaded() const noexcept -> bool { return this->config.numThreads == 0; };
			EVO_NODISCARD auto isMultiThreaded() const noexcept -> bool { return this->config.numThreads != 0; };


			EVO_NODISCARD auto threadsRunning() const noexcept -> bool;
			auto startupThreads() noexcept -> void;
			auto shutdownThreads() noexcept -> void;
			auto waitForAllTasks() noexcept -> void;


			// Loads a number of files in a multithreaded. 
			// No other files should be loaded before or after calling this function. If you want more granular control,
			// 		load files through the Source Manager directly, (Call this->getSourceManager())
			auto loadFiles(evo::ArrayProxy<fs::path> file_paths) noexcept -> void;

			

			auto emitDiagnostic(auto&&... args) noexcept -> void {
				auto diagnostic = Diagnostic(std::forward<decltype(args)>(args)...);
				this->emit_diagnostic_impl(diagnostic);
			};


			EVO_NODISCARD auto errored() const noexcept -> bool { return this->num_errors != 0; };
			EVO_NODISCARD auto hitFailCondition() const noexcept -> bool { return this->hit_fail_condition; };

			EVO_NODISCARD auto getSourceManager()       noexcept ->       SourceManager& { return this->src_manager; };
			EVO_NODISCARD auto getSourceManager() const noexcept -> const SourceManager& { return this->src_manager; };

			EVO_NODISCARD auto getConfig() const noexcept -> const Config& { return this->config; };



			///////////////////////////////////
			// internal use only

			auto debug([[maybe_unused]] std::string_view message) noexcept -> void {
				#if defined(PCIT_BUILD_DEBUG)
					const auto lock_guard = std::lock_guard(this->callback_mutex);
					evo::log::debug(message);
				#endif
			};

			template<class... Args>
			auto debug([[maybe_unused]] std::format_string<Args...> fmt, [[maybe_unused]] Args&&... args) noexcept
			-> void {
				#if defined(PCIT_BUILD_DEBUG)
					const auto lock_guard = std::lock_guard(this->callback_mutex);
					evo::log::debug(fmt, std::forward<decltype(args)>(args)...);
				#endif
			};

			auto trace([[maybe_unused]] std::string_view message) noexcept -> void {
				#if defined(PCIT_BUILD_DEBUG)
					const auto lock_guard = std::lock_guard(this->callback_mutex);
					evo::log::trace(message);
				#endif
			};

			template<class... Args>
			auto trace([[maybe_unused]] std::format_string<Args...> fmt, [[maybe_unused]] Args&&... args) noexcept
			-> void {
				#if defined(PCIT_BUILD_DEBUG)
					const auto lock_guard = std::lock_guard(this->callback_mutex);
					evo::log::trace(fmt, std::forward<decltype(args)>(args)...);
				#endif
			};
		private:
			auto emit_diagnostic_impl(const Diagnostic& diagnostic) noexcept -> void;
			auto notify_task_failed() noexcept -> void;
			auto consume_tasks_single_threaded() noexcept -> void;
	
		private:
			Config config;

			SourceManager src_manager;
			std::mutex src_manager_mutex{};


			DiagnosticCallback callback;
			std::mutex callback_mutex{};


			///////////////////////////////////
			// threading

			std::atomic<evo::uint> num_errors = 0;
			std::atomic<evo::uint> num_threads_running = 0;
			std::atomic<bool> hit_fail_condition = false;
			std::atomic<bool> shutting_down_threads = false;

			struct LoadFileTask{
				fs::path path;
				int num;
			};

			using Task = evo::Variant<LoadFileTask>;

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
					auto run_load_file(const LoadFileTask& task) noexcept -> bool;

				private:
					Context* context;
					bool is_working = false;
					std::jthread thread{};
			};


			std::vector<Worker> workers{};
	};


};