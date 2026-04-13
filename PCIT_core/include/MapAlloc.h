////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>

#include <unordered_map>
#include <mutex>


namespace pcit::core{


	template<
		class KEY, class VALUE, size_t STARTING_POW_OF_2 = evo::get_optimal_start_pow_of_2_for_step_vector<VALUE>()
	>
	class MapAlloc{
		private:
			struct InternalValue{
				std::optional<VALUE> value = std::nullopt;
				std::atomic<bool> flag = false;
			};

		public:
			struct ValueHandle{
				EVO_NODISCARD auto needsToBeSet() const -> bool { return this->needs_to_be_set; }

				auto getValue() -> VALUE& {
					evo::debugAssert(this->needsToBeSet() == false, "Cannot get value if it doesn't need to be set");

					while(this->internal_value.flag.load() == false){
						std::this_thread::yield();
					}

					return *this->internal_value.value;
				}

				auto emplaceValue(auto&&... args) -> VALUE& {
					evo::debugAssert(this->needsToBeSet(), "Cannot emplace value if it needs to be set");

					this->internal_value.value.emplace(std::forward<decltype(args)>(args)...);
					this->internal_value.flag = true;
					return *this->internal_value.value;
				}


				ValueHandle(InternalValue& _internal_value, bool _needs_to_be_set)
					: internal_value(_internal_value), needs_to_be_set(_needs_to_be_set) {}

				private:
					InternalValue& internal_value;
					bool needs_to_be_set;
			};


		public:
			MapAlloc() = default;
			~MapAlloc() = default;

			auto get(const KEY& key) -> ValueHandle {
				const auto lock = std::scoped_lock(this->spin_lock);

				const auto find = this->map.find(key);

				if(find != this->map.end()){
					return ValueHandle(*find->second, false);
				}else{
					InternalValue& new_internal_value = this->allocator.emplace_back();
					this->map.emplace(key, &new_internal_value);
					return ValueHandle(new_internal_value, true);
				}
			}
	

		private:
			std::unordered_map<KEY, InternalValue*> map{};
			evo::StepVector<InternalValue> allocator{};
			mutable evo::SpinLock spin_lock{};
	};


}


