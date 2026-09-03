////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <type_traits>
#include <Evo.hpp>
#include "./StepAlloc.hpp"

namespace pcit::core{


	template<class RangeBound, class Value>
	class RangeMap{
		static_assert(std::is_integral_v<RangeBound>, "RangeBound must be integral");

		public:
			RangeMap() = default;
			~RangeMap() = default;


			auto emplace(RangeBound first, RangeBound last, auto&&... value_args) -> void {
				evo::debugAssert(first <= last, "first must be <= last");

				///////////////////////////////////
				// find location

				size_t target_index = 0;

				if(this->range_infos_bst.empty() == false){
					while(true){
						if(target_index >= this->range_infos_bst.size()){
							if(this->range_infos_bst.empty()){
								this->range_infos_bst.resize(3);
							}else{
								this->range_infos_bst.resize(this->range_infos_bst.size() * 2 + 1);
							}
							break;
						}

						std::optional<RangeInfo>& target = this->range_infos_bst[target_index];

						if(target.has_value() == false){ break; }

						// <  : left
						// >= : right
						target_index = (2 * target_index) + 1 + size_t(first >= target->first);
					}

				}else{
					this->range_infos_bst.resize(3);
				}


				///////////////////////////////////
				// emplace

				const uint32_t value_index = this->values_step_alloc.emplace_back(
					std::forward<decltype(value_args)>(value_args)...
				);
				this->range_infos_bst[target_index].emplace(first, last, value_index);
			}


			struct CLookupResult{
				const Value& value;
				RangeBound offset;
			};
			[[nodiscard]] auto lookup(RangeBound value) const -> std::optional<CLookupResult> {
				size_t target_index = 0;

				while(true){
					if(target_index >= this->range_infos_bst.size()){ return std::nullopt; }

					const std::optional<RangeInfo>& target = this->range_infos_bst[target_index];

					if(target.has_value() == false){ return std::nullopt; }

					if(value >= target->first){
						if(value <= target->last){
							return CLookupResult{
								.value = this->values_step_alloc[target->value_index],
								.offset = value - target->first,
							};

						}else{ // go right
							target_index = 2 * target_index + 2;
						}

					}else{ // go left
						target_index = 2 * target_index + 1;
					}
				}
			}

			struct LookupResult{
				Value& value;
				RangeBound offset;
			};
			[[nodiscard]] auto lookup(RangeBound value) -> std::optional<LookupResult> {
				size_t target_index = 0;

				while(true){
					if(target_index >= this->range_infos_bst.size()){ return std::nullopt; }

					std::optional<RangeInfo>& target = this->range_infos_bst[target_index];

					if(target.has_value() == false){ return std::nullopt; }

					if(value >= target->first && value <= target->last){
						return LookupResult{
							.value = this->values_step_alloc[target->value_index],
							.offset = value - target->first,
						};
					}

					target_index = (2 * target_index) + 1 + size_t(value > target->first);
				}
			}

	
		private:
			struct RangeInfo{
				RangeBound first;
				RangeBound last;
				uint32_t value_index;
			};
			evo::SmallVector<std::optional<RangeInfo>> range_infos_bst{};

			core::StepAlloc<Value, uint32_t> values_step_alloc{};
	};


}


