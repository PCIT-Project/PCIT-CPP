////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>

#include <atomic>
#include <thread>



namespace pcit::core{

	// adapted from: https://www.youtube.com/watch?v=rmGJc9PXpuE
	template<int INITIAL_WAIT_THRESHOLD = 8>
	class SpinLockBase{
		public:
			SpinLockBase() = default;
			~SpinLockBase() = default;

			auto lock() -> void {
				int wait_threshold = INITIAL_WAIT_THRESHOLD;
				int wait_iter = 0;

				while(this->try_lock() == false){
					if(wait_iter == wait_threshold){
						std::this_thread::yield();

						wait_iter = 0;
						if(wait_threshold < 0){ wait_threshold -= 1; }
					}else{
						wait_iter += 1;
					}
				}
			}

			auto try_lock() -> bool {
				return this->flag.load(std::memory_order_relaxed) == 0
					&& this->flag.exchange(1, std::memory_order_acquire) == 0;
			}

			auto unlock() -> void {
				this->flag.store(0, std::memory_order_release);
			}

			auto unlock_shared() -> void { this->unlock(); }
			auto lock_shared() -> void { this->lock(); }
	
		private:
			std::atomic<uint32_t> flag = 0; // surprisingly, this was measured to be faster than uint64_t
	};


	using SpinLock = SpinLockBase<>;


}


