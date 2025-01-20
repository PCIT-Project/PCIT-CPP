////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>

#include <thread>
#include <deque>

namespace pcit::core{

	template<class TASK>
	class ThreadQueue{
		public:
			using WorkFunc = std::function<bool(TASK&)>;

		public:
			ThreadQueue(WorkFunc&& work_func) : _work_func(std::move(work_func)) {}
			ThreadQueue(const WorkFunc& work_func) : _work_func(work_func) {}

			#if defined(PCIT_CONFIG_DEBUG)
				~ThreadQueue(){
					evo::debugAssert(
						this->isRunning() == false,
						"Attempted to run destructor of ThreadQueue while it was still running"
					);
				}
			#else
				~ThreadQueue() = default;
			#endif


			EVO_NODISCARD static auto optimalNumThreads() -> uint32_t {
				return uint32_t(std::thread::hardware_concurrency());
			}


			auto startup(uint32_t num_threads = optimalNumThreads()) -> void {
				evo::debugAssert(this->isRunning() == false, "Already running");

				this->priv.num_workers_running = num_threads;
				this->priv.task_failed = false;

				this->priv.workers.reserve(num_threads);
				for(size_t i = 0; i < num_threads; i+=1){
					this->priv.workers.emplace_back(*this);
				}
			}


			auto shutdown() -> void {
				evo::debugAssert(this->isRunning(), "Should not shutdown if not running");

				for(Worker& worker : this->priv.workers){
					worker.request_stop();
				}

				this->waitUntilNotRunning();
			}


			auto addTask(TASK&& task) -> void {
				// evo::debugAssert(this->isRunning(), "Thread Queue not running");

				const auto add_lock = std::lock_guard(this->priv.add_tasks_lock);
				const auto task_lock = std::lock_guard(this->priv.tasks_lock);
				this->priv.tasks.emplace_front(std::move(task));
			}

			auto addTask(const TASK& task) -> void {
				// evo::debugAssert(this->isRunning(), "Thread Queue not running");

				const auto add_lock = std::lock_guard(this->priv.add_tasks_lock);
				const auto task_lock = std::lock_guard(this->priv.tasks_lock);
				this->priv.tasks.emplace_front(task);
			}

			auto addTask(auto&&... args) -> void {
				// evo::debugAssert(this->isRunning(), "Thread Queue not running");

				const auto add_lock = std::lock_guard(this->priv.add_tasks_lock);
				const auto task_lock = std::lock_guard(this->priv.tasks_lock);
				this->priv.tasks.emplace_front(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto isWorking() const -> bool { 
				if(this->priv.tasks.empty()){ return false; }

				const auto add_lock = std::lock_guard(this->priv.add_tasks_lock);
				if(this->priv.tasks.empty()){ return false; }

				for(const Worker& worker : this->priv.workers){
					if(worker.is_working()){ return true; }
				}

				return false;
			}

			EVO_NODISCARD auto isRunning() const -> bool { return this->priv.workers.empty() == false; }
			EVO_NODISCARD auto taskFailed() const -> bool { return this->priv.task_failed; }


			// returns if all tasks ran successfully
			auto waitUntilDoneWorking() -> bool {
				while(this->isWorking()){
					std::this_thread::yield();
				}

				return !this->taskFailed();
			}

			auto waitUntilNotRunning() -> void {
				while(this->isRunning()){
					std::this_thread::yield();
				}
			}


		private:
			class Worker{
				public:
					Worker(ThreadQueue& _thread_queue) 
						: thread_queue(_thread_queue), thread([this](std::stop_token stop) -> void {
							while(stop.stop_requested() == false){
								std::optional<TASK> task = this->thread_queue.get_task();
								if(task.has_value() == false){
									this->_is_working = false;
									std::this_thread::yield();
									continue;
								}

								this->_is_working = true;

								const bool work_res = this->thread_queue._work_func(*task);
								if(work_res == false){
									this->thread_queue.signal_task_failed();
									this->_is_working = false;
									std::this_thread::yield();
								}
							}

							this->thread_queue.signal_worker_shutdown();
						})
					{
						this->thread.detach();
					}

					~Worker() = default;


					EVO_NODISCARD auto is_working() const -> bool { return this->_is_working; }

					auto request_stop() -> void {
						this->thread.request_stop();
					}
			
				private:
					ThreadQueue& thread_queue;
					std::jthread thread;

					bool _is_working = true;
			};


			auto get_task() -> std::optional<TASK> {
				if(this->priv.tasks.empty()){ return std::nullopt; }

				const auto lock = std::lock_guard(this->priv.tasks_lock);
				if(this->priv.tasks.empty()){ return std::nullopt; }

				const TASK task = this->priv.tasks.back();
				this->priv.tasks.pop_back();
				return task;
			}


			auto signal_task_failed() -> void {
				const auto lock = std::lock_guard(this->priv.tasks_lock);

				this->priv.tasks.clear();
				this->priv.task_failed = true;
			}

			auto signal_worker_shutdown() -> void {
				if(this->priv.num_workers_running.fetch_sub(1) == 1){
					this->priv.workers.clear();
				}
			}

	
		private:
			// this is to keep members private from workers while allowing workers to access private member funcs
			struct /* priv */ {
				private:
					evo::UnmovableVector<Worker, false> workers{};

					std::deque<TASK> tasks{};
					mutable core::SpinLock tasks_lock{};
					mutable core::SpinLock add_tasks_lock{};

					std::atomic<uint32_t> num_workers_running = 0;
					bool task_failed = false;

					friend ThreadQueue;
			} priv;

			WorkFunc _work_func;

			friend Worker;
	};


}


