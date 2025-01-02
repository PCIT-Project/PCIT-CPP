////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>


namespace pcit::core{

	// Step allocator that guarantees that adding new elements does not invalidate pointers
	// Allows lookups with an ID
	// Allows for construction of types that have private constructors iff they are friends with LinearStepAlloc

	// Note: ID must inherit from UniqueID, or be an integral


	// picks either the size of a cache-line or 4 elements (whichever makes more sense for the size of the type)
	template<class T>
	EVO_NODISCARD consteval auto get_optimal_starting_pow_of_2_for_linear_step_alloc() -> size_t {
		const size_t num_bits = std::bit_ceil(std::hardware_destructive_interference_size / sizeof(T));
		return std::max(size_t(2), size_t(std::countr_zero(num_bits)));
	}


	template<class T, class ID, size_t STARTING_POW_OF_2 = get_optimal_starting_pow_of_2_for_linear_step_alloc<T>()>
	class LinearStepAlloc{
		public:
			LinearStepAlloc() = default;
			~LinearStepAlloc() {
				if(this->buffers.empty()){ return; }

				if(this->buffers.size() > 1){
					for(size_t i = 0; i < this->buffers.size() - 1; i+=1){
						std::destroy_n(this->buffers[i], size_t(1) << (i + STARTING_POW_OF_2));
						std::free(this->buffers[i]);
					}
				}

				std::destroy_n(this->buffers.back(), this->current_end_of_buffer);
				std::free(this->buffers.back());
			};

			LinearStepAlloc(const LinearStepAlloc&) = delete;
			LinearStepAlloc(LinearStepAlloc&&) = delete;


			auto emplace_back(auto&&... args) -> ID {
				if(this->need_to_create_new_buffer()){ this->create_new_buffer(); }

				T* new_elem_ptr = &this->buffers.back()[this->current_end_of_buffer];

				// using placement new to allow for construction of type that have private constructors but are friends
				::new (static_cast<void*>(new_elem_ptr)) T(std::forward<decltype(args)>(args)...);

				EVO_DEFER([&](){ this->current_end_of_buffer += 1; });

				return ID(uint32_t(this->size_when_not_fully_deallocated()));
			}

			
			auto operator[](const ID& id) const -> const T& {
				const BufferAndElemIndex indices = this->get_buffer_and_elem_index(id);
				return this->buffers[indices.buffer_index][indices.elem_index];
			}

			auto operator[](const ID& id) -> T& {
				const BufferAndElemIndex indices = this->get_buffer_and_elem_index(id);
				return this->buffers[indices.buffer_index][indices.elem_index];
			}


			EVO_NODISCARD auto size() const -> size_t {
				if(this->buffers.empty()){ return 0; }
				return this->size_when_not_fully_deallocated();
			}

			EVO_NODISCARD auto empty() const -> bool { return this->size() == 0; }



			EVO_NODISCARD auto clear() -> void {
				std::destroy_at(this);
				std::construct_at(this);
			}



			//////////////////////////////////////////////////////////////////////
			// iterators

			class Iter{
			    public:
			    	using difference_type   = std::ptrdiff_t;
			    	using value_type        = T;
			    	using pointer           = const T*;
			    	using reference         = const T&;

			        Iter() : index(), parent(nullptr) {};
			        Iter(const ID& _index, LinearStepAlloc& _parent) : index(_index), parent(&_parent) {};
			        ~Iter() = default;

			        Iter(const Iter&) = default;
			        auto operator=(const Iter& rhs) -> Iter& {
			        	std::construct_at(this, rhs);
			        	return *this;
			        };
			        Iter(Iter&&) = default;
			        auto operator=(Iter&& rhs) -> Iter& {
			        	std::construct_at(this, std::move(rhs));
			        	return *this;
			        };


			        auto operator++() -> Iter& {
			            if constexpr(std::is_integral_v<ID>){
			            	this->index += 1;
			            }else{
			            	this->index = ID(this->index.get() + 1);
			            }
			            return *this;
			        }

			        auto operator++(int) -> Iter {
			            Iter iterator = *this;
			            ++(*this);
			            return iterator;
			        }

			        auto operator--() -> Iter& {
			            if constexpr(std::is_integral_v<ID>){
			            	this->index -= 1;
			            }else{
			            	this->index = ID(this->index.get() - 1);
			            }
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

			    private:
			    	ID index;
			        LinearStepAlloc* parent;
			};


			class ConstIter{
			    public:
			    	using difference_type   = std::ptrdiff_t;
			    	using value_type        = T;
			    	using pointer           = const T*;
			    	using reference         = const T&;

			        ConstIter() : index(), parent(nullptr) {};
			        ConstIter(const ID& _index, const LinearStepAlloc& _parent) : index(_index), parent(&_parent) {};
			        ~ConstIter() = default;

			        ConstIter(const ConstIter&) = default;
			        auto operator=(const ConstIter& rhs) -> ConstIter& {
			        	std::construct_at(this, rhs);
			        	return *this;
			        };
			        ConstIter(ConstIter&&) = default;
			        auto operator=(ConstIter&& rhs) -> ConstIter& {
			        	std::construct_at(this, std::move(rhs));
			        	return *this;
			        };


			        auto operator++() -> ConstIter& {
			            if constexpr(std::is_integral_v<ID>){
			            	this->index += 1;
			            }else{
			            	this->index = ID(this->index.get() + 1);
			            }
			            return *this;
			        }

			        auto operator++(int) -> ConstIter {
			            ConstIter iterator = *this;
			            ++(*this);
			            return iterator;
			        }

			        auto operator--() -> ConstIter& {
			            if constexpr(std::is_integral_v<ID>){
			            	this->index -= 1;
			            }else{
			            	this->index = ID(this->index.get() - 1);
			            }
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

			    private:
			    	ID index;
			        const LinearStepAlloc* parent;
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

			EVO_NODISCARD auto rend()        ->      Iter { return Iter(ID(~0ul), *this);      }
			EVO_NODISCARD auto rend()  const -> ConstIter { return ConstIter(ID(~0ul), *this); }
			EVO_NODISCARD auto crend() const -> ConstIter { return ConstIter(ID(~0ul), *this); }



		private:
			EVO_NODISCARD auto size_when_not_fully_deallocated() const -> size_t {
				return (size_t(1) << (this->buffers.size() - 1 + STARTING_POW_OF_2)) - (1 << STARTING_POW_OF_2)
					+ this->current_end_of_buffer;
			}


			EVO_NODISCARD auto current_buffer_max_size() const -> size_t {
				evo::debugAssert(this->buffers.empty() == false, "no buffer to get max size of");
				return 1ull << (this->buffers.size() - 1 + STARTING_POW_OF_2);
			}

			EVO_NODISCARD auto need_to_create_new_buffer() const -> bool {
				if(this->buffers.empty()){ return true; }
				return this->current_end_of_buffer >= this->current_buffer_max_size();
			}

			EVO_NODISCARD auto create_new_buffer() -> void {
				const size_t size_of_new_buffer = size_t(1) << (this->buffers.size() + STARTING_POW_OF_2);

				this->buffers.emplace_back((T*)std::malloc(sizeof(T) * size_of_new_buffer));
				this->current_end_of_buffer = 0;
			}

			struct BufferAndElemIndex{
				size_t buffer_index;
				size_t elem_index;
			};
			EVO_NODISCARD auto get_buffer_and_elem_index(const ID& id) const -> BufferAndElemIndex {
				const uint64_t index = [&](){
					if constexpr(std::is_integral_v<ID>){
						return uint64_t(id) + (1 << STARTING_POW_OF_2);
					}else{
						return uint64_t(id.get()) + (1 << STARTING_POW_OF_2);
					}
				}();
				const uint64_t buffer_index = (sizeof(uint64_t) * 8) - std::countl_zero(index) - 1;
				const uint64_t elem_index = index & ~(1ull << buffer_index);
				// Note: doing and/not is equivalent (performance-wise) to subtracting

				return BufferAndElemIndex(size_t(buffer_index) - STARTING_POW_OF_2, size_t(elem_index));
			}

	
		private:
			evo::SmallVector<T*> buffers{};
			uint32_t current_end_of_buffer = 0;
	};


}


