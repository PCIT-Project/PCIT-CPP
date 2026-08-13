////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.hpp>
#include <PCIT_core.hpp>



namespace pcit::pir{

	
	class ExecutionEnginePtrMap{
		public:
			ExecutionEnginePtrMap() = default;
			~ExecutionEnginePtrMap() = default;


			auto lookupPtr(uint32_t ptr_key) -> std::optional<void*> {
				const auto find = this->map.find_left(ptr_key);
				if(find != this->map.end()){ return find->second; }
				return std::nullopt;
			}


			auto getOrCreateKey(void* ptr) -> uint32_t {
				const auto find = this->map.find_right(ptr);
				if(find != this->map.end()){ return find->first; }

				const uint32_t new_key = uint32_t(this->map.size());
				this->map.emplace(new_key, ptr);
				return new_key;
			}



			auto reset() -> void {
				this->map.clear();
			}


		private:
			evo::Bimap<uint32_t, void*> map{};
	};


}


