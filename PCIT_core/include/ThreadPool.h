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



namespace pcit::core{


	// Thread pool with a work stealing model

	template<class DATA>
	class ThreadPool{
		public:
			using WorkFunc = std::function<evo::Result<>(DATA&)>;

		public:
			ThreadPool() = default;

			#if defined(PCIT_CONFIG_DEBUG)
				~ThreadPool(){
					evo::debugAssert(
						this->isRunning() == false,
						"Attempted to run destructor of ThreadPool while it was still running"
					);
				}
			#else
				~ThreadPool() = default;
			#endif

			EVO_NODISCARD static auto optimalNumThreads() -> uint64_t {
				return std::thread::hardware_concurrency();
			}

			auto startup(unsigned num_threads = optimalNumThreads()) -> void {
				evo::debugAssert(this->isRunning() == false, "Already running");
				
				this->num_threads_still_running = num_threads;

				this->workers.reserve(num_threads);
				for(size_t i = 0; i < num_threads; i+=1){
					this->workers.emplace_back(this, i);
				}
			}

			auto shutdown() -> void {
				evo::debugAssert(this->isRunning(), "Should not shutdown if not running");

				for(Worker& worker : this->workers){
					worker.request_stop();
				}

				this->waitUntilNotRunning();
			}


			auto work(evo::SmallVector<DATA>&& run_data, WorkFunc&& func) -> void {
				evo::debugAssert(this->isRunning(), "Thread Pool not running");
				evo::debugAssert(this->isWorking() == false, "Thread Pool already working");

				this->worker_failed = false;
				this->data = std::move(run_data);
				this->work_func = func;	


				if(this->data.size() < this->workers.size()){
					this->num_threads_still_working = unsigned(this->data.size());

					for(size_t i = 0; i < this->data.size(); i+=1){
						this->workers[i].start_working(i, i);
					}

				}else{
					this->num_threads_still_working = this->getNumThreads();

					const size_t num_elements_per_worker = this->data.size() / this->workers.size();
					const size_t num_workers_with_extra = this->data.size() % this->workers.size();

					size_t last_end = 0;
					for(size_t i = 0; Worker& worker : this->workers){
						const bool should_add_extra = i < num_workers_with_extra;
						const size_t next_end = last_end + num_elements_per_worker - 1 + size_t(should_add_extra);

						worker.start_working(last_end, next_end);

						last_end = next_end + 1;
					
						i += 1;
					}
				}
			}


			EVO_NODISCARD auto isWorking() const -> bool { return this->num_threads_still_working != 0; }
			EVO_NODISCARD auto isRunning() const -> bool { return this->workers.empty() == false; }
			EVO_NODISCARD auto anyTaskFailed() const -> bool { return this->worker_failed; }
			EVO_NODISCARD auto getNumThreads() const -> unsigned { return unsigned(this->workers.size()); }


			auto waitUntilDoneWorking() -> evo::Result<> {
				while(this->isWorking()){
					std::this_thread::yield();
				}

				return evo::Result<>::fromBool(!this->anyTaskFailed());
			}

			auto waitUntilNotRunning() -> void {
				while(this->isRunning()){
					std::this_thread::yield();
				}
			}


		private:
			class Worker{
				public:
					enum class Mode{
						WAITING,
						WORKING,
						STEALING,
						STOPPING, // told to stop doing tasks, but not stop running
					};

				public:
					Worker(ThreadPool* _thread_pool, size_t _id)
						: thread_pool(_thread_pool), id(_id), thread([this](std::stop_token stop) -> void {
							while(stop.stop_requested() == false){
								if(this->mode == Mode::WAITING){
									std::this_thread::yield();
									continue;

								}else if(this->mode == Mode::STOPPING){
									this->mode = Mode::WAITING;
									this->thread_pool->signal_worker_finished_working();
									std::this_thread::yield();
									continue;
								}


								DATA* task = this->get_task();
								if(task == nullptr){
									this->mode = Mode::WAITING;
									std::this_thread::yield();
									continue;
								}

								const evo::Result<> task_result = this->thread_pool->work_func->operator()(*task);
								if(task_result.isError()){
									this->mode = Mode::STOPPING;
									this->thread_pool->signal_worker_failed();
								}
							}
							this->thread_pool->signal_worker_finished_running();
						}) 
					{
						this->thread.detach();
					}

					~Worker() = default;

					Worker(const Worker&) = delete;


					auto start_working(size_t new_start, size_t new_end) -> void {
						this->mode = Mode::WORKING;
						this->start_index = new_start;
						this->end_index = new_end;
					}


					auto request_stop() -> void {
						this->thread.request_stop();
					}

				public:
					Mode mode = Mode::WAITING;

				private:
					auto get_task() -> DATA* {
						evo::debugAssert(
							this->mode != Mode::WAITING, "Should not be getting task when in waiting mode"
						);

						if(this->mode == Mode::WORKING){
							this->work_lock.lock();

							if(this->start_index > this->end_index){
								this->mode = Mode::STEALING;
								this->work_lock.unlock();
								return this->search_for_task_to_steal();
							}

							DATA* next_task = &this->thread_pool->data[this->start_index];
							this->start_index += 1;
							this->work_lock.unlock();
							return next_task;

						}else if(this->mode == Mode::STOPPING){
							return nullptr;

						}else{
							evo::debugAssert(
								this->mode == Mode::STEALING, "Unsupported mode ({})", evo::to_underlying(this->mode)
							);

							return this->search_for_task_to_steal();
						}
					}


					auto search_for_task_to_steal() -> DATA* {
						evo::debugAssert(this->mode == Mode::STEALING, "Should only steal if in stealing mode");

						for(size_t i = 0; i < this->thread_pool->workers.size(); i+=1){
							if(i == this->id){ continue; }

							DATA* stolen_task = this->thread_pool->workers[i].attempt_to_steal_from();
							if(stolen_task != nullptr){ return stolen_task; }
						}

						this->mode = Mode::WAITING;
						this->thread_pool->signal_worker_finished_working();
						return nullptr;
					}

					auto attempt_to_steal_from() -> DATA* {
						const auto lock = std::lock_guard(this->work_lock);

						if(this->mode != Mode::WORKING){ return nullptr; }

						if(this->start_index >= this->end_index){ return nullptr; }

						DATA* stolen_task = &this->thread_pool->data[this->end_index];
						this->end_index -= 1;
						return stolen_task;
					}
					
			
				private:
					ThreadPool* thread_pool;
					const size_t id;
					std::jthread thread;

					evo::SpinLock work_lock{}; 
					size_t start_index = 0;
					size_t end_index = 0;
			};


			auto signal_worker_finished_working() -> void {
				if(this->num_threads_still_working.fetch_sub(1) == 1){
					this->data.clear();
					this->work_func.reset();
				}
			}

			auto signal_worker_failed() -> void {
				this->worker_failed = true;
				for(Worker& worker : this->workers){
					if(worker.mode != Worker::Mode::WAITING){ worker.mode = Worker::Mode::STOPPING; }
				}
			}


			auto signal_worker_finished_running() -> void {
				if(this->num_threads_still_running.fetch_sub(1) == 1){
					this->workers.clear();
				}
			}

		private:
			evo::SmallVector<DATA> data{};
			std::optional<WorkFunc> work_func{};
			bool worker_failed = false;

			evo::UnmovableVector<Worker, false> workers{};
			std::atomic<unsigned> num_threads_still_working = 0;
			std::atomic<unsigned> num_threads_still_running = 0;

			friend Worker;
	};


}


