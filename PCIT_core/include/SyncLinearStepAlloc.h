////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <mutex>

#include <Evo.h>

#include "./LinearStepAlloc.h"


namespace pcit::core{

	template<class T, class ID, size_t STARTING_POW_OF_2 = get_optimal_start_pow_of_2_for_linear_step_alloc<T>()>
	class SyncLinearStepAlloc{
		public:
			SyncLinearStepAlloc() = default;
			~SyncLinearStepAlloc() = default;

			SyncLinearStepAlloc(const SyncLinearStepAlloc& rhs) = delete;
			SyncLinearStepAlloc(SyncLinearStepAlloc&& rhs) = default;



			auto emplace_back(auto&&... args) -> ID {
				const auto lock = std::scoped_lock(this->spin_lock);
				return this->linear_step_alloc.emplace_back(std::forward<decltype(args)>(args)...);
			}

			
			auto operator[](const ID& id) const -> const T& {
				const auto lock = std::scoped_lock(this->spin_lock);
				return this->linear_step_alloc[id];
			}

			auto operator[](const ID& id) -> T& {
				const auto lock = std::scoped_lock(this->spin_lock);
				return this->linear_step_alloc[id];	
			}


			EVO_NODISCARD auto size() const -> size_t {
				const auto lock = std::scoped_lock(this->spin_lock);
				return this->linear_step_alloc.size();
			}

			EVO_NODISCARD auto empty() const -> bool {
				const auto lock = std::scoped_lock(this->spin_lock);
				return this->linear_step_alloc.empty();
			}



			auto clear() -> void {
				const auto lock = std::scoped_lock(this->spin_lock);
				this->linear_step_alloc.clear();
			}



			//////////////////////////////////////////////////////////////////////
			// iterators

			using Iter = LinearStepAlloc<T, ID, STARTING_POW_OF_2>::Iter;
			using ConstIter = LinearStepAlloc<T, ID, STARTING_POW_OF_2>::ConstIter;


			EVO_NODISCARD auto begin()        ->      Iter { return this->linear_step_alloc.begin();  }
			EVO_NODISCARD auto begin()  const -> ConstIter { return this->linear_step_alloc.begin();  }
			EVO_NODISCARD auto cbegin() const -> ConstIter { return this->linear_step_alloc.cbegin(); }

			EVO_NODISCARD auto end() -> Iter {
				const auto lock = std::scoped_lock(this->spin_lock);
				return this->linear_step_alloc.end(); 
			}

			EVO_NODISCARD auto end()  const -> ConstIter {
				const auto lock = std::scoped_lock(this->spin_lock);
				return this->linear_step_alloc.end(); 
			}

			EVO_NODISCARD auto cend() const -> ConstIter {
				const auto lock = std::scoped_lock(this->spin_lock);
				return this->linear_step_alloc.cend();
			}



			EVO_NODISCARD auto rbegin() -> Iter {
				const auto lock = std::scoped_lock(this->spin_lock);
				return this->linear_step_alloc.rbegin(); 
			}

			EVO_NODISCARD auto rbegin()  const -> ConstIter {
				const auto lock = std::scoped_lock(this->spin_lock);
				return this->linear_step_alloc.rbegin(); 
			}

			EVO_NODISCARD auto crbegin() const -> ConstIter {
				const auto lock = std::scoped_lock(this->spin_lock);
				return this->linear_step_alloc.crbegin();
			}

			EVO_NODISCARD auto rend()        ->      Iter { return this->linear_step_alloc.rend();  }
			EVO_NODISCARD auto rend()  const -> ConstIter { return this->linear_step_alloc.rend();  }
			EVO_NODISCARD auto crend() const -> ConstIter { return this->linear_step_alloc.crend(); }


			EVO_NODISCARD auto getSpinLock() const -> evo::SpinLock& {
				return this->spin_lock;
			}
	
		private:
			LinearStepAlloc<T, ID, STARTING_POW_OF_2> linear_step_alloc{};
			mutable evo::SpinLock spin_lock{};
	};


}


