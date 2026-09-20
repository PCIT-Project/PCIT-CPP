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
				RangeInfo& new_range_info = this->range_infos.emplace_back(
					first, last, std::forward<decltype(value_args)>(value_args)...
				);

				if(this->range_infos.size() == 1){ return; }

				RangeInfo* target_range_info = &this->range_infos.front();
				while(true){
					RangeInfo** next_target_range_info_ptr = [&]() -> RangeInfo** {
						if(new_range_info.first < target_range_info->first){
							return &target_range_info->less_than;
						}else{
							return &target_range_info->greater_than_or_equal_to;
						}
					}();

					if(*next_target_range_info_ptr == nullptr){
						*next_target_range_info_ptr = &new_range_info;
						return;
					}

					target_range_info = *next_target_range_info_ptr;
				}
			}

			struct CLookupResult{
				const Value& value;
				RangeBound offset;
			};
			[[nodiscard]] auto lookup(RangeBound value) const -> std::optional<CLookupResult> {
				if(this->range_infos.empty()){ return std::nullopt; }

				const RangeInfo* target_range_info = &this->range_infos.front();
				while(target_range_info != nullptr){
					if(value >= target_range_info->first){
						if(value <= target_range_info->last){
							return CLookupResult(target_range_info->value, value - target_range_info->first);
						}

						target_range_info = target_range_info->greater_than_or_equal_to;
						
					}else{ // value < target_range_info->first
						target_range_info = target_range_info->less_than;
					}
				}

				return std::nullopt;
			}


			struct LookupResult{
				Value& value;
				RangeBound offset;
			};
			[[nodiscard]] auto lookup(RangeBound value) -> std::optional<LookupResult> {
				if(this->range_infos.empty()){ return std::nullopt; }

				RangeInfo* target_range_info = &this->range_infos.front();
				while(target_range_info != nullptr){
					if(value >= target_range_info->first){
						if(value <= target_range_info->last){
							return CLookupResult(target_range_info->value, value - target_range_info->first);
						}

						target_range_info = target_range_info->greater_than_or_equal_to;
						
					}else{ // value < target_range_info->first
						target_range_info = target_range_info->less_than;
					}
				}

				return std::nullopt;
			}

	
		private:
			struct RangeInfo{
				Value value;

				RangeBound first;
				RangeBound last;

				RangeInfo* less_than = nullptr;
				RangeInfo* greater_than_or_equal_to = nullptr;

				RangeInfo(RangeBound _first, RangeBound _last, auto&&... value_args)
					: first(_first), last(_last), value(std::forward<decltype(value_args)>(value_args)...) {}
			};

			evo::StepVector<RangeInfo> range_infos{};
	};


}


