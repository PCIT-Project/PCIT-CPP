////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.hpp>

#include <thread>



namespace pcit::core{


	// Thread pool with a work stealing model

	template<class DATA>
	class ThreadPool{
		public:
			using WorkFunc = std::function<evo::Result<>(DATA&)>;

		public:
			ThreadPool() = default;
			~ThreadPool() = default;


			auto startup(size_t num_threads) -> void {
				evo::debugAssert(this->isRunning() == false, "Already running");

				this->workers.reserve(num_threads);

				for(size_t i = 0; i < num_threads; i+=1){
					this->workers.emplace_back(*this);
				}

				this->errored      = false;
				this->num_stealing = 0;
				this->num_working  = 0;
				this->num_running  = uint32_t(num_threads);
			}



			[[nodiscard]] static auto optimalNumThreads() -> uint64_t {
				return std::thread::hardware_concurrency();
			}

			auto work(evo::SmallVector<DATA>&& tasks, WorkFunc&& func) -> void {
				evo::debugAssert(this->isRunning(), "Not running");
				evo::debugAssert(this->isWorking() == false, "Already working");
				evo::debugAssert(tasks.empty() == false, "Shouldn't give 0 tasks");

				this->data = std::move(tasks);
				this->work_func = std::move(func);

				this->errored      = false;
				this->num_stealing = 0;


				if(this->workers.size() > this->data.size()){
					for(size_t i = 0; i < this->data.size(); i+=1){
						this->workers[i].start_working(i, i+1);
					}

					this->max_num_working = uint32_t(this->data.size());
					
				}else{
					const size_t num_tasks_per_thread = this->data.size() / this->workers.size();
					const size_t num_remaining_tasks = this->data.size() % this->workers.size();
					
					size_t task_index = 0;
					for(size_t i = 0; Worker& worker : this->workers){
						const size_t num_tasks_for_worker = num_tasks_per_thread + size_t(i < num_remaining_tasks);
						worker.start_working(task_index, task_index + num_tasks_for_worker);

						task_index += num_tasks_for_worker;
					
						i += 1;
					}

					this->max_num_working = uint32_t(this->workers.size());
				}

				// setting `this->num_working` MUST be the last line executed in this function 
				// as it starts the workers working
				this->num_working = this->max_num_working;
			}


			auto waitUntilDoneWorking() -> evo::Result<> {
				while(this->isWorking()){
					std::this_thread::yield();
				}

				return evo::Result<>::fromBool(!this->errored.load());
			}

			auto waitUntilDoneRunning() -> void {
				while(this->isRunning()){
					std::this_thread::yield();
				}
			}


			auto shutdown() -> void {
				evo::debugAssert(this->isRunning(), "Not running");

				for(Worker& worker : this->workers){
					worker.request_stop();
				}

				this->waitUntilDoneRunning();
			}


			[[nodiscard]] auto isRunning() const -> bool { return this->num_running > 0; }
			[[nodiscard]] auto isWorking() const -> bool { return this->num_working > 0; }


		private:
			class Worker{
				public:
					Worker(ThreadPool& _thread_pool)
						: thread_pool(_thread_pool), mode(Mode::WAITING), thread([this](std::stop_token stop) -> void {
							this->thread_func(stop);
						}) 
					{
						this->thread.detach();
					}

					~Worker() = default;

					auto request_stop() -> void { this->thread.request_stop(); }

					auto start_working(size_t start, size_t end) -> void {
						evo::debugAssert(this->mode == Mode::WAITING, "Can only start working if currently waiting");
						this->current_index = start;
						this->end_index     = end;
						this->mode          = Mode::READY_TO_START;
					}

					auto tell_to_stop_working() -> void {
						Mode expected_mode = Mode::WORKING;
						if(this->mode.compare_exchange_strong(expected_mode, Mode::TOLD_TO_STOP_WORKING)){ return; }

						expected_mode = Mode::STEALING;
						this->mode.compare_exchange_strong(expected_mode, Mode::TOLD_TO_STOP_WORKING);
					}

				private:
					[[nodiscard]] auto is_done_working() -> bool {
						evo::debugAssert(this->index_lock.try_lock() == false, "Lock must be taken to call this");
						return this->current_index >= this->end_index;
					}


					auto stop_working() -> void {
						const Mode old_mode = this->mode.exchange(Mode::WAITING);

						if(old_mode != Mode::WAITING){
							this->thread_pool.signal_worker_stopped_working();
						}
					}

					auto task_errored() -> void { 
						this->stop_working();
						this->thread_pool.signal_worker_errored();
					}



					[[nodiscard]] auto get_task() -> DATA* {
						const auto lock = std::scoped_lock(this->index_lock);

						if(this->is_done_working()){ return nullptr; }

						EVO_DEFER([&](){ this->current_index += 1; });
						return &this->thread_pool.data[this->current_index];
					}

					[[nodiscard]] auto externally_steal_task() -> DATA* {
						const auto lock = std::scoped_lock(this->index_lock);

						if(this->is_done_working()){ return nullptr; }

						this->end_index -= 1; // yes, decriment is done BEFORE getting index of stolen task
						return &this->thread_pool.data[this->end_index];
					}



					enum class Mode{
						WAITING,
						READY_TO_START,
						WORKING,
						STEALING,
						TOLD_TO_STOP_WORKING,
					};


					auto thread_func(std::stop_token stop) -> void {
						while(stop.stop_requested() == false){
							switch(this->mode.load()){
								case Mode::WAITING: {
									std::this_thread::yield();
								} break;

								case Mode::READY_TO_START: {
									if(this->thread_pool.isWorking()){
										this->mode = Mode::WORKING;
									}else{
										std::this_thread::yield();
									}
								} break;

								case Mode::WORKING: {
									DATA* task = this->get_task();
									if(task == nullptr){
										this->mode = Mode::STEALING;
										this->thread_pool.num_stealing += 1;
										continue;
									}

									if(this->thread_pool.work_func(*task).isError()){
										this->task_errored();
									}
								} break;

								case Mode::STEALING: {
									size_t target_worker_index = 0;
									size_t num_workers_failed_to_steal_from_in_a_row = 0;

									while(this->thread_pool.num_stealing < this->thread_pool.max_num_working){
										EVO_DEFER([&](){
											target_worker_index += 1;
											if(target_worker_index >= this->thread_pool.max_num_working){
												target_worker_index = 0;
											}
										});

										Worker& target_worker = this->thread_pool.workers[target_worker_index];
										if(target_worker.mode != Mode::WORKING){ continue; }

										DATA* task = target_worker.externally_steal_task();
										if(task == nullptr){
											num_workers_failed_to_steal_from_in_a_row += 1;
											if(
												num_workers_failed_to_steal_from_in_a_row
												>= this->thread_pool.max_num_working
											){
												this->stop_working();
											}
											continue;
										}

										num_workers_failed_to_steal_from_in_a_row = 0;

										if(this->thread_pool.work_func(*task).isError()){
											this->task_errored();
										}
									}

									this->stop_working();
								} break;

								case Mode::TOLD_TO_STOP_WORKING: {
									this->stop_working();
								} break;
							}
						}

						this->thread_pool.signal_worker_stopped_running();
					}
			
				private:
					ThreadPool& thread_pool;
					std::atomic<Mode> mode;
					std::jthread thread;

					evo::SpinLock index_lock{};
					size_t current_index = 0;
					size_t end_index = 0;
			};

			auto signal_worker_stopped_running() -> void {
				#if defined(PCIT_CONFIG_DEBUG)
					evo::debugAssert(this->num_running.fetch_sub(1) > 0, "Too many signals to stop running");
				#else
					this->num_running -= 1;
				#endif
			}

			auto signal_worker_stopped_working() -> void {
				#if defined(PCIT_CONFIG_DEBUG)
					evo::debugAssert(this->num_working.fetch_sub(1) > 0, "Too many signals to stop working");
				#else
					this->num_working -= 1;
				#endif
			}

			auto signal_worker_errored() -> void {
				if(this->errored.exchange(true)){ return; }

				for(Worker& worker : this->workers){
					worker.tell_to_stop_working();
				}
			}


		private:
			evo::SmallVector<DATA> data{};
			WorkFunc work_func{};
			evo::UnmovableVector<Worker, false> workers{};

			uint32_t max_num_working = 0;

			std::atomic<uint32_t> num_running  = 0;
			std::atomic<uint32_t> num_working  = 0;
			std::atomic<uint32_t> num_stealing = 0;
			std::atomic<bool> errored          = false;

	};



}


