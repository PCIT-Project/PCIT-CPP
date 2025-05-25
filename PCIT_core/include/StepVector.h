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



	// picks either the size of a cache-line or 4 elements (whichever makes more sense for the size of the type)
	template<class T>
	EVO_NODISCARD consteval auto get_optimal_start_pow_of_2_for_step_vector() -> size_t {
		const size_t num_bits = std::bit_ceil(std::hardware_destructive_interference_size / sizeof(T));
		return std::max(size_t(2), size_t(std::countr_zero(num_bits)));
	}


	template<class T, size_t STARTING_POW_OF_2 = get_optimal_start_pow_of_2_for_step_vector<T>()>
	class StepVector{
		public:
			StepVector() = default;
			~StepVector() {
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

			StepVector(const StepVector&) = delete;
			StepVector(StepVector&&) = delete;


			auto emplace_back(auto&&... args) -> T& {
				if(this->need_to_create_new_buffer()){ this->create_new_buffer(); }

				T* new_elem_ptr = &this->buffers.back()[this->current_end_of_buffer];

				// using placement new to allow for construction of type that have private constructors but are friends
				::new (static_cast<void*>(new_elem_ptr)) T(std::forward<decltype(args)>(args)...);

				EVO_DEFER([&](){ this->current_end_of_buffer += 1; });

				return *new_elem_ptr;
			}

			auto pop_back() -> void {
				std::destroy_at(this->back());

				this->current_end_of_buffer -= 1;

				if(this->current_end_of_buffer == 0){
					std::free(this->buffers.back());
					this->buffers.pop_back();
				}
			}


			auto resize(size_t new_size) -> void {
				const size_t current_size = this->size();

				if(new_size > current_size){
					for(size_t i = 0; i < new_size - current_size; i+=1){
						this->emplace_back();
					}

				}else{
					for(size_t i = 0; i < current_size - new_size; i+=1){
						this->pop_back();
					}
				}
			}


			
			auto operator[](size_t index) const -> const T& {
				const BufferAndElemIndex indices = this->get_buffer_and_elem_index(index);
				return this->buffers[indices.buffer_index][indices.elem_index];
			}

			auto operator[](size_t index) -> T& {
				const BufferAndElemIndex indices = this->get_buffer_and_elem_index(index);
				return this->buffers[indices.buffer_index][indices.elem_index];
			}


			EVO_NODISCARD auto size() const -> size_t {
				if(this->buffers.empty()){ return 0; }
				return this->size_when_not_fully_deallocated();
			}

			EVO_NODISCARD auto empty() const -> bool { return this->size() == 0; }



			auto clear() -> void {
				std::destroy_at(this);
				std::construct_at(this);
			}



			EVO_NODISCARD auto front() const -> const T& { return this->buffers[0][0]; }
			EVO_NODISCARD auto front()       ->       T& { return this->buffers[0][0]; }

			EVO_NODISCARD auto back() const -> const T& { return this->buffers.back()[this->current_end_of_buffer-1]; }
			EVO_NODISCARD auto back()       ->       T& { return this->buffers.back()[this->current_end_of_buffer-1]; }



			//////////////////////////////////////////////////////////////////////
			// iterators

			class Iter{
			    public:
			    	using difference_type   = std::ptrdiff_t;
			    	using value_type        = T;
			    	using pointer           = const T*;
			    	using reference         = const T&;

			        Iter() : index(), parent(nullptr) {};
			        Iter(size_t _index, StepVector& _parent) : index(_index), parent(&_parent) {};
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
			            this->index += 1;
			            return *this;
			        }

			        auto operator++(int) -> Iter {
			            Iter iterator = *this;
			            ++(*this);
			            return iterator;
			        }

			        auto operator--() -> Iter& {
			            this->index -= 1;
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
			    	size_t index;
			        StepVector* parent;
			};


			class ConstIter{
			    public:
			    	using difference_type   = std::ptrdiff_t;
			    	using value_type        = T;
			    	using pointer           = const T*;
			    	using reference         = const T&;

			        ConstIter() : index(), parent(nullptr) {};
			        ConstIter(size_t _index, const StepVector& _parent) : index(_index), parent(&_parent) {};
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
						this->index += 1;
			            return *this;
			        }

			        auto operator++(int) -> ConstIter {
			            ConstIter iterator = *this;
			            ++(*this);
			            return iterator;
			        }

			        auto operator--() -> ConstIter& {
			            this->index -= 1;
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
			    	size_t index;
			        const StepVector* parent;
			};


			EVO_NODISCARD auto begin()        ->      Iter { return Iter(0, *this);      }
			EVO_NODISCARD auto begin()  const -> ConstIter { return ConstIter(0, *this); }
			EVO_NODISCARD auto cbegin() const -> ConstIter { return ConstIter(0, *this); }

			EVO_NODISCARD auto end()        ->      Iter { return Iter(this->size(), *this);      }
			EVO_NODISCARD auto end()  const -> ConstIter { return ConstIter(this->size(), *this); }
			EVO_NODISCARD auto cend() const -> ConstIter { return ConstIter(this->size(), *this); }


			// TODO(FUTURE): reverse iterators?


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
			EVO_NODISCARD auto get_buffer_and_elem_index(size_t index) const -> BufferAndElemIndex {
				const uint64_t converted_index = uint64_t(index) + (1 << STARTING_POW_OF_2);
				
				const uint64_t buffer_index = std::bit_width(converted_index) - 1;
				const uint64_t elem_index = converted_index & ~(1ull << buffer_index);

				return BufferAndElemIndex(size_t(buffer_index) - STARTING_POW_OF_2, size_t(elem_index));
			}

	
		private:
			evo::SmallVector<T*> buffers{};
			uint32_t current_end_of_buffer = 0;
	};


}


