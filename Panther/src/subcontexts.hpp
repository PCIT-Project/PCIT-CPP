////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <Evo.hpp>


namespace pcit::panther{

	class ContextComptimeContext{
		public:
			struct Data{
				Data() = default;
				Data(const Data&) = delete;

				std::unordered_map<void*, bool> allocations_currently_allocated{};
				size_t num_allocations_allocated = 0;

				auto add_allocation(void* ptr) -> void {
					this->num_allocations_allocated += 1;

					using MapIter = std::unordered_map<void*, bool>::iterator;
					std::pair<MapIter, bool> emplace_res =
						this->allocations_currently_allocated.emplace(ptr, true);

					if(emplace_res.second == false){
						emplace_res.first->second = true;
					}
				}

				auto reset() -> void {
					this->allocations_currently_allocated.clear();
					this->num_allocations_allocated = 0;
				}
			};

		public:
			ContextComptimeContext() = default;
			~ContextComptimeContext() = default;

			auto add_thread_data_if_needed() -> void {
				const std::thread::id current_thread_id = std::this_thread::get_id();
				
				const auto lock = std::scoped_lock(this->spin_lock);

				if(this->data_map.contains(current_thread_id)){ return; }
				this->data_map.emplace(current_thread_id, &this->data_alloc.emplace_back());
			}

			auto get_data() -> Data& {
				const auto lock = std::scoped_lock(this->spin_lock);
				
				evo::debugAssert(
					this->data_map.contains(std::this_thread::get_id()), "This thread id wasn't added"
				);

				return *this->data_map.at(std::this_thread::get_id());
			}
	
		private:
			evo::StepVector<Data> data_alloc{};
			std::unordered_map<std::thread::id, Data*> data_map{};
			mutable evo::SpinLock spin_lock{};
	};


	class PantherVMContext{
		public:
			PantherVMContext() = default;
			~PantherVMContext() {
				evo::debugAssert(this->allocations_currently_allocated.empty(), "Allocations weren't cleared");
			}


			auto mark_alloc(void* ptr, size_t size) -> void {
				const auto lock = std::scoped_lock(this->allocations_currently_allocated_lock);
				this->allocations_currently_allocated.emplace(ptr, AllocationInfo(size, true));
			}

			enum class MarkDeallocError : uint32_t {
				NOT_ALLOCATED = 0,
				ALREADY_DEALLOCATED = 1,
			};
			[[nodiscard]] auto mark_dealloc(void* ptr) -> evo::Expected<void, MarkDeallocError> {
				const auto lock = std::scoped_lock(this->allocations_currently_allocated_lock);

				const auto alloc_find = this->allocations_currently_allocated.find(ptr);

				if(alloc_find == this->allocations_currently_allocated.end()){
					return evo::Unexpected(MarkDeallocError::NOT_ALLOCATED);
				}

				if(alloc_find->second.is_allocated == false){
					return evo::Unexpected(MarkDeallocError::ALREADY_DEALLOCATED);
				}

				alloc_find->second.is_allocated = false;
				return {};
			}


			// To be run after execution
			auto clear_current_allocations(const std::function<void(void*, size_t)>& callback) -> void {
				for(const auto& [key, value] : this->allocations_currently_allocated){
					if(value.is_allocated){ callback(key, value.size); }
				}

				this->allocations_currently_allocated.clear();
			}

	
		private:
			struct AllocationInfo{
				size_t size;
				bool is_allocated;
			};


			std::unordered_map<void*, AllocationInfo> allocations_currently_allocated{};
			evo::SpinLock allocations_currently_allocated_lock{};
	};

	

	
}
