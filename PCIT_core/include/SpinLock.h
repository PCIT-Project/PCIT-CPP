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

#if defined(EVO_ARCH_X86_64) || defined(EVO_ARCH_X86_32)
	#include <emmintrin.h>

#elif defined(EVO_ARCH_ARM)
	#include <arm_acle.h>
#endif



namespace pcit::core{

	// https://www.youtube.com/watch?v=rmGJc9PXpuE
	class SpinLock{
		public:
			SpinLock() = default;
			~SpinLock() = default;

			auto lock() -> void {
				for(size_t i = 0; !this->try_lock(); i+=1){
					if(i == 8){
						i = 0;
						std::this_thread::yield();
					}
				}
			}

			auto try_lock() -> bool {
				return !this->flag.load(std::memory_order_relaxed) && 
					!this->flag.exchange(1, std::memory_order_acquire);
			}

			auto unlock() -> void {
				this->flag.store(0, std::memory_order_release);
			}

			auto unlock_shared() -> void { this->unlock(); }
			auto lock_shared() -> void { this->lock(); }
	
		private:
			std::atomic<uint32_t> flag = 0;
	};


}


