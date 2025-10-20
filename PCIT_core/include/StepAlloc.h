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
#include <mutex>

#include "./LinearStepAlloc.h"



namespace pcit::core{


	// Step allocator that guarantees that adding new elements does not invalidate pointers
	// Allows lookups with an ID

	// Note: ID must inherit from UniqueID, or be an integral


	template<class T>
	EVO_NODISCARD consteval auto get_optimal_start_pow_of_2_for_step_alloc() -> size_t {
		const size_t num_bits = std::bit_ceil(std::hardware_destructive_interference_size / sizeof(std::optional<T>));
		return std::max(size_t(2), size_t(std::countr_zero(num_bits)));
	}


	template<class T, class ID, size_t STARTING_POW_OF_2 = get_optimal_start_pow_of_2_for_step_alloc<T>()>
	class StepAlloc{
		public:
			StepAlloc() = default;
			~StepAlloc() = default;

			StepAlloc(const StepAlloc& rhs) = delete;
			StepAlloc(StepAlloc&& rhs) = default;

			

			EVO_NODISCARD auto emplace_back(auto&&... args) -> ID {
				const auto lock = std::scoped_lock(this->mutex);

				if(this->erased_elems.empty() == false){
					const ID new_elem_id = this->erased_elems.top();
					this->erased_elems.pop();

					this->linear_step_alloc[new_elem_id].emplace(std::forward<decltype(args)>(args)...);
					return new_elem_id;
				}

				return this->linear_step_alloc.emplace_back(std::in_place, std::forward<decltype(args)>(args)...);
			}


			auto erase(ID id) -> void {
				const auto lock = std::scoped_lock(this->mutex);

				this->linear_step_alloc[id].reset();
				this->erased_elems.push(id);

				if(this->erased_elems.size() == this->linear_step_alloc.size()){
					// if all elements were erased, reset
					this->clear_without_lock();
				}
			}


			auto operator[](const ID& id) const -> const T& {
				const auto lock = std::scoped_lock(this->mutex);
				if constexpr(std::is_integral_v<ID>){
					evo::debugAssert(this->linear_step_alloc[id].has_value(), "This element ({}) was erased", id);
				}else{
					evo::debugAssert(this->linear_step_alloc[id].has_value(), "This element ({}) was erased", id.get());
				}

				return *this->linear_step_alloc[id];
			}

			auto operator[](const ID& id) -> T& {
				const auto lock = std::scoped_lock(this->mutex);
				if constexpr(std::is_integral_v<ID>){
					evo::debugAssert(this->linear_step_alloc[id].has_value(), "This element ({}) was erased", id);
				}else{
					evo::debugAssert(this->linear_step_alloc[id].has_value(), "This element ({}) was erased", id.get());
				}

				return *this->linear_step_alloc[id];
			}


			EVO_NODISCARD auto size() const -> size_t { return this->linear_step_alloc.size(); }
			EVO_NODISCARD auto empty() const -> bool { return this->linear_step_alloc.empty(); }


			EVO_NODISCARD auto clear() -> void {
				const auto lock = std::scoped_lock(this->mutex);

				this->clear_without_lock();
			}


			//////////////////////////////////////////////////////////////////////
			// iterators

			class Iter{
			    public:
			    	using difference_type   = std::ptrdiff_t;
			    	using value_type        = T;
			    	using pointer           = const T*;
			    	using reference         = const T&;

			    	Iter() : index(ID(0)), parent(nullptr) {}
			        Iter(const ID& _index, StepAlloc& _parent) : index(_index), parent(&_parent) {}
			        ~Iter() = default;

			        Iter(const Iter&) = default;
			        auto operator=(const Iter& rhs) -> Iter& {
			        	std::construct_at(this, rhs);
			        	return *this;
			        }

			        Iter(Iter&&) = default;
			        auto operator=(Iter&& rhs) -> Iter& {
			        	std::construct_at(this, std::move(rhs));
			        	return *this;
			        }


			        auto operator++() -> Iter& {
			            do{
			            	if constexpr(std::is_integral_v<ID>){
			            		this->index += 1;
			            	}else{
			            		this->index = ID(this->index.get() + 1);
			            	}
			            }while(
			            	this->parent->linear_step_alloc[this->index].has_value() == false &&
			            	this->index != ID(uint32_t(this->parent->size()))
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
			            	this->parent->linear_step_alloc[this->index].has_value() == false &&
			            	this->index != ID(0)
			            );

			            return *this;
			        }

			        auto operator--(int) -> Iter {
			            Iter iterator = *this;
			            --(*this);
			            return iterator;
			        }


			        EVO_NODISCARD auto operator*() const -> T& { return this->parent->operator[](this->index); }
			        EVO_NODISCARD auto operator->() const -> T* { return &this->parent->operator[](this->index); }

			        EVO_NODISCARD auto operator==(const Iter& rhs) const -> bool {
			        	return this->index == rhs.index;
			        }
			        EVO_NODISCARD auto operator!=(const Iter& rhs) const -> bool {
			        	return this->index != rhs.index;
			        }


			        EVO_NODISCARD auto getID() const -> ID { return this->index; }
			        EVO_NODISCARD auto getParent() const -> StepAlloc& {
			        	evo::debugAssert(this->parent != nullptr, "Iterator was default-constructed; no parent set");
			        	return *this->parent;
			        }

			    private:
			    	ID index;
			        StepAlloc* parent;
			};

			static_assert(std::bidirectional_iterator<Iter>);


			class ConstIter{
			    public:
			    	using difference_type   = std::ptrdiff_t;
			    	using value_type        = T;
			    	using pointer           = const T*;
			    	using reference         = const T&;

			    	ConstIter() : index(ID(0)), parent(nullptr) {}
			        ConstIter(const ID& _index, const StepAlloc& _parent) : index(_index), parent(&_parent) {}
			        ~ConstIter() = default;

			        ConstIter(const ConstIter&) = default;
			        auto operator=(const ConstIter& rhs) -> ConstIter& {
			        	std::construct_at(this, rhs);
			        	return *this;
			        }

			        ConstIter(ConstIter&&) = default;
			        auto operator=(ConstIter&& rhs) -> ConstIter& {
			        	std::construct_at(this, std::move(rhs));
			        	return *this;
			        }


			        auto operator++() -> ConstIter& {
			            do{
			            	if constexpr(std::is_integral_v<ID>){
			            		this->index += 1;
			            	}else{
			            		this->index = ID(this->index.get() + 1);
			            	}
			            }while(
			            	this->index != ID(uint32_t(this->parent->size())) &&
			            	this->parent->linear_step_alloc[this->index].has_value() == false
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
			            	this->parent->linear_step_alloc[this->index].has_value() == false &&
			            	this->index != ID(0)
			            );

			            return *this;
			        }

			        auto operator--(int) -> ConstIter {
			            ConstIter iterator = *this;
			            --(*this);
			            return iterator;
			        }


			        EVO_NODISCARD auto operator*() const -> const T& { return this->parent->operator[](this->index); }
			        EVO_NODISCARD auto operator->() const -> const T* { return &this->parent->operator[](this->index); }

			        EVO_NODISCARD auto operator==(const ConstIter& rhs) const -> bool {
			        	return this->index == rhs.index;
			        }
			        EVO_NODISCARD auto operator!=(const ConstIter& rhs) const -> bool {
			        	return this->index != rhs.index;
			        }


			        EVO_NODISCARD auto getID() const -> ID { return this->index; }
			        EVO_NODISCARD auto getParent() const -> const StepAlloc& {
			        	evo::debugAssert(this->parent != nullptr, "Iterator was default-constructed; no parent set");
			        	return *this->parent;
			        }

			    private:
			    	ID index;
			        const StepAlloc* parent;
			};

			static_assert(std::bidirectional_iterator<ConstIter>);




			EVO_NODISCARD auto begin()        ->      Iter { return Iter(ID(0), *this);      }
			EVO_NODISCARD auto begin()  const -> ConstIter { return ConstIter(ID(0), *this); }
			EVO_NODISCARD auto cbegin() const -> ConstIter { return ConstIter(ID(0), *this); }

			EVO_NODISCARD auto end()        ->      Iter { return Iter(ID(uint32_t(this->size())), *this);      }
			EVO_NODISCARD auto end()  const -> ConstIter { return ConstIter(ID(uint32_t(this->size())), *this); }
			EVO_NODISCARD auto cend() const -> ConstIter { return ConstIter(ID(uint32_t(this->size())), *this); }


		private:
			auto clear_without_lock() -> void {
				std::destroy_at(this);
				std::construct_at(this);
			}
	
		private:
			LinearStepAlloc<std::optional<T>, ID, STARTING_POW_OF_2> linear_step_alloc{};
			std::stack<ID> erased_elems{};
			mutable evo::SpinLock mutex{};
	};

}


