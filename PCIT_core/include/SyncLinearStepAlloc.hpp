////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <mutex>

#include <Evo.hpp>

#include "./LinearStepAlloc.hpp"


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


			[[nodiscard]] auto size() const -> size_t {
				const auto lock = std::scoped_lock(this->spin_lock);
				return this->linear_step_alloc.size();
			}

			[[nodiscard]] auto empty() const -> bool {
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


			[[nodiscard]] auto begin()        ->      Iter { return this->linear_step_alloc.begin();  }
			[[nodiscard]] auto begin()  const -> ConstIter { return this->linear_step_alloc.begin();  }
			[[nodiscard]] auto cbegin() const -> ConstIter { return this->linear_step_alloc.cbegin(); }

			[[nodiscard]] auto end() -> Iter {
				const auto lock = std::scoped_lock(this->spin_lock);
				return this->linear_step_alloc.end(); 
			}

			[[nodiscard]] auto end()  const -> ConstIter {
				const auto lock = std::scoped_lock(this->spin_lock);
				return this->linear_step_alloc.end(); 
			}

			[[nodiscard]] auto cend() const -> ConstIter {
				const auto lock = std::scoped_lock(this->spin_lock);
				return this->linear_step_alloc.cend();
			}



			[[nodiscard]] auto rbegin() -> Iter {
				const auto lock = std::scoped_lock(this->spin_lock);
				return this->linear_step_alloc.rbegin(); 
			}

			[[nodiscard]] auto rbegin()  const -> ConstIter {
				const auto lock = std::scoped_lock(this->spin_lock);
				return this->linear_step_alloc.rbegin(); 
			}

			[[nodiscard]] auto crbegin() const -> ConstIter {
				const auto lock = std::scoped_lock(this->spin_lock);
				return this->linear_step_alloc.crbegin();
			}

			[[nodiscard]] auto rend()        ->      Iter { return this->linear_step_alloc.rend();  }
			[[nodiscard]] auto rend()  const -> ConstIter { return this->linear_step_alloc.rend();  }
			[[nodiscard]] auto crend() const -> ConstIter { return this->linear_step_alloc.crend(); }


			[[nodiscard]] auto getSpinLock() const -> evo::SpinLock& {
				return this->spin_lock;
			}
	
		private:
			LinearStepAlloc<T, ID, STARTING_POW_OF_2> linear_step_alloc{};
			mutable evo::SpinLock spin_lock{};
	};


}


