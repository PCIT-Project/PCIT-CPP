////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>

#include <stack>
#include <shared_mutex>

#include "./LinearStepAlloc.h"



namespace pcit::core{


	// Step allocator that guarantees that adding new elements does not invalidate pointers
	// Allows lookups with an ID

	// Note: ID must inherit from UniqueID, or be an integral


	template<class T, class ID, size_t STARTING_POW_OF_2 = get_optimal_starting_pow_of_2_for_linear_step_alloc<T>()>
	class StepAlloc{
		public:
			StepAlloc() = default;
			~StepAlloc() = default;

			StepAlloc(const StepAlloc&) = delete;
			StepAlloc(StepAlloc&&) = delete;
			

			EVO_NODISCARD auto emplace_back(auto&&... args) -> ID {
				const auto lock = std::unique_lock(this->mutex);

				if(this->erased_elems.empty() == false){
					const ID new_elem_id = this->erased_elems.top();
					this->erased_elems.pop();

					this->linear_step_alloc[new_elem_id].emplace(std::forward<decltype(args)>(args)...);
					return new_elem_id;
				}

				return this->linear_step_alloc.emplace_back(std::in_place, std::forward<decltype(args)>(args)...);
			}


			auto erase(ID id) -> void {
				const auto lock = std::unique_lock(this->mutex);

				this->linear_step_alloc[id].reset();
				this->erased_elems.push(id);

				if(this->erased_elems.size() == this->linear_step_alloc.size()){
					// if all elements were erased, reset
					this->clear();
				}
			}


			auto operator[](const ID& id) const -> const T& {
				const auto lock = std::shared_lock(this->mutex);

				return *this->linear_step_alloc[id];
			}

			auto operator[](const ID& id) -> T& {
				const auto lock = std::shared_lock(this->mutex);

				return *this->linear_step_alloc[id];
			}


			EVO_NODISCARD auto size() const -> size_t { return this->linear_step_alloc.size(); }
			EVO_NODISCARD auto empty() const -> bool { return this->linear_step_alloc.empty(); }


			EVO_NODISCARD auto clear() -> void {
				const auto lock = std::unique_lock(this->mutex);

				std::destroy_at(this);
				std::construct_at(this);
			}


			//////////////////////////////////////////////////////////////////////
			// iterators

			class Iter{
			    public:
			        Iter(const ID& _index, StepAlloc& _parent) : index(_index), parent(_parent) {};
			        ~Iter() = default;


			        auto operator++() -> Iter& {
			            do{
			            	if constexpr(std::is_integral_v<ID>){
			            		this->index += 1;
			            	}else{
			            		this->index = ID(this->index.get() + 1);
			            	}
			            }while(
			            	this->parent.linear_step_alloc[this->index].has_value() == false &&
			            	this->index != ID(uint32_t(this->parent.size()))
			            );

			            return *this;
			        }

			        auto operator++(int) -> Iter {
			            Iter iterator = *this;
			            ++(*this);
			            return iterator;
			        }

			        auto operator--() -> Iter& {
			            do{
			            	if constexpr(std::is_integral_v<ID>){
			            		this->index -= 1;
			            	}else{
			            		this->index = ID(this->index.get() - 1);
			            	}
			            }while(
			            	this->parent.linear_step_alloc[this->index].has_value() == false &&
			            	this->index != ID(0)
			            );

			            return *this;
			        }

			        auto operator--(int) -> Iter {
			            Iter iterator = *this;
			            --(*this);
			            return iterator;
			        }


			        EVO_NODISCARD auto operator*() const -> T& { return this->parent[this->index]; }
			        EVO_NODISCARD auto operator->() const -> T* { return &this->parent[this->index]; }

			        EVO_NODISCARD auto operator==(const Iter& rhs) const -> bool {
			        	return this->index == rhs.index;
			        }
			        EVO_NODISCARD auto operator!=(const Iter& rhs) const -> bool {
			        	return this->index != rhs.index;
			        }


			    private:
			    	ID index;
			        StepAlloc& parent;
			};


			class ConstIter{
			    public:
			        ConstIter(const ID& _index, const StepAlloc& _parent) : index(_index), parent(_parent) {};
			        ~ConstIter() = default;


			        auto operator++() -> ConstIter& {
			            do{
			            	if constexpr(std::is_integral_v<ID>){
			            		this->index += 1;
			            	}else{
			            		this->index = ID(this->index.get() + 1);
			            	}
			            }while(
			            	this->parent.linear_step_alloc[this->index].has_value() == false &&
			            	this->index != ID(uint32_t(this->parent.size()))
			            );

			            return *this;
			        }

			        auto operator++(int) -> ConstIter {
			            ConstIter iterator = *this;
			            ++(*this);
			            return iterator;
			        }

			        auto operator--() -> ConstIter& {
			            do{
			            	if constexpr(std::is_integral_v<ID>){
			            		this->index -= 1;
			            	}else{
			            		this->index = ID(this->index.get() - 1);
			            	}
			            }while(
			            	this->parent.linear_step_alloc[this->index].has_value() == false &&
			            	this->index != ID(0)
			            );

			            return *this;
			        }

			        auto operator--(int) -> ConstIter {
			            ConstIter iterator = *this;
			            --(*this);
			            return iterator;
			        }


			        EVO_NODISCARD auto operator*() const -> const T& { return this->parent[this->index]; }
			        EVO_NODISCARD auto operator->() const -> const T* { return &this->parent[this->index]; }

			        EVO_NODISCARD auto operator==(const ConstIter& rhs) const -> bool {
			        	return this->index == rhs.index;
			        }
			        EVO_NODISCARD auto operator!=(const ConstIter& rhs) const -> bool {
			        	return this->index != rhs.index;
			        }


			    private:
			    	ID index;
			        const StepAlloc& parent;
			};




			EVO_NODISCARD auto begin()        ->     Iter { return Iter(ID(0), *this);       }
			EVO_NODISCARD auto begin()  const -> ConstIter { return ConstIter(ID(0), *this); }
			EVO_NODISCARD auto cbegin() const -> ConstIter { return ConstIter(ID(0), *this); }

			EVO_NODISCARD auto end()        ->      Iter { return Iter(ID(uint32_t(this->size())), *this);      }
			EVO_NODISCARD auto end()  const -> ConstIter { return ConstIter(ID(uint32_t(this->size())), *this); }
			EVO_NODISCARD auto cend() const -> ConstIter { return ConstIter(ID(uint32_t(this->size())), *this); }


			EVO_NODISCARD auto rbegin()        ->      Iter { return Iter(ID(uint32_t(this->size() - 1)), *this);      }
			EVO_NODISCARD auto rbegin()  const -> ConstIter { return ConstIter(ID(uint32_t(this->size() - 1)), *this); }
			EVO_NODISCARD auto crbegin() const -> ConstIter { return ConstIter(ID(uint32_t(this->size() - 1)), *this); }

			EVO_NODISCARD auto rend()        ->      Iter { return Iter(ID(~uint32_t(0)), *this);      }
			EVO_NODISCARD auto rend()  const -> ConstIter { return ConstIter(ID(~uint32_t(0)), *this); }
			EVO_NODISCARD auto crend() const -> ConstIter { return ConstIter(ID(~uint32_t(0)), *this); }


	
		private:
			LinearStepAlloc<std::optional<T>, ID, STARTING_POW_OF_2> linear_step_alloc{};
			std::stack<ID> erased_elems{};
			mutable core::SpinLock mutex{};

			friend Iter;
	};


}


