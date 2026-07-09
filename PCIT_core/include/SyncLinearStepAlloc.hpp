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

			//////////////////////////////////////////////////////////////////////
			// iterators

			class Iter{
			    public:
			    	using difference_type   = std::ptrdiff_t;
			    	using value_type        = T;
			    	using pointer           = const T*;
			    	using reference         = const T&;

			        Iter() : index(), parent(nullptr) {};
			        Iter(const ID& _index, SyncLinearStepAlloc& _parent) : index(_index), parent(&_parent) {};
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


			        [[nodiscard]] auto operator*() const -> T& { return this->parent->operator[](this->index); }
			        [[nodiscard]] auto operator->() const -> T* { return &this->parent->operator[](this->index); }

			        [[nodiscard]] auto operator==(const Iter& rhs) const -> bool {
			        	return this->index == rhs.index;
			        }
			        [[nodiscard]] auto operator!=(const Iter& rhs) const -> bool {
			        	return this->index != rhs.index;
			        }

			    private:
			    	ID index;
			        SyncLinearStepAlloc* parent;
			};


			class ConstIter{
			    public:
			    	using difference_type   = std::ptrdiff_t;
			    	using value_type        = T;
			    	using pointer           = const T*;
			    	using reference         = const T&;

			        ConstIter() : index(), parent(nullptr) {};
			        ConstIter(const ID& _index, const SyncLinearStepAlloc& _parent) : index(_index), parent(&_parent) {};
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


			        [[nodiscard]] auto operator*() const -> const T& { return this->parent->operator[](this->index); }
			        [[nodiscard]] auto operator->() const -> const T* { return &this->parent->operator[](this->index); }

			        [[nodiscard]] auto operator==(const ConstIter& rhs) const -> bool {
			        	return this->index == rhs.index;
			        }
			        [[nodiscard]] auto operator!=(const ConstIter& rhs) const -> bool {
			        	return this->index != rhs.index;
			        }

			    private:
			    	ID index;
			        const SyncLinearStepAlloc* parent;
			};


			[[nodiscard]] auto begin()        ->      Iter { return Iter(ID(0), *this);      }
			[[nodiscard]] auto begin()  const -> ConstIter { return ConstIter(ID(0), *this); }
			[[nodiscard]] auto cbegin() const -> ConstIter { return ConstIter(ID(0), *this); }

			[[nodiscard]] auto end()        ->      Iter { return Iter(ID(uint32_t(this->size())), *this);      }
			[[nodiscard]] auto end()  const -> ConstIter { return ConstIter(ID(uint32_t(this->size())), *this); }
			[[nodiscard]] auto cend() const -> ConstIter { return ConstIter(ID(uint32_t(this->size())), *this); }


			[[nodiscard]] auto rbegin()        ->      Iter { return Iter(ID(uint32_t(this->size() - 1)), *this);      }
			[[nodiscard]] auto rbegin()  const -> ConstIter { return ConstIter(ID(uint32_t(this->size() - 1)), *this); }
			[[nodiscard]] auto crbegin() const -> ConstIter { return ConstIter(ID(uint32_t(this->size() - 1)), *this); }

			[[nodiscard]] auto rend()        ->      Iter { return Iter(ID(~0ul), *this);      }
			[[nodiscard]] auto rend()  const -> ConstIter { return ConstIter(ID(~0ul), *this); }
			[[nodiscard]] auto crend() const -> ConstIter { return ConstIter(ID(~0ul), *this); }


			[[nodiscard]] auto getSpinLock() const -> evo::SpinLock& {
				return this->spin_lock;
			}
	
		private:
			LinearStepAlloc<T, ID, STARTING_POW_OF_2> linear_step_alloc{};
			mutable evo::SpinLock spin_lock{};
	};


}


