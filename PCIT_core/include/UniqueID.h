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



	template<class BaseT, class SelfReference>
	struct UniqueID{
		public:
			using ThisT = UniqueID<BaseT, SelfReference>;
			using Base = BaseT;

			class Iterator{
			    public:
			        using difference_type   = std::ptrdiff_t;
			        using value_type        = SelfReference;

			        constexpr Iterator(const ThisT& _id) : id(_id) {}
			        constexpr ~Iterator() = default;


			        constexpr auto operator++() -> Iterator& {
			            BaseT* id_ptr = (BaseT*)&this->id;
			            (*id_ptr)++;
			            return *this;
			        }

			        constexpr auto operator++(int) -> Iterator {
			            Iterator iterator = *this;
			            ++(*this);
			            return iterator;
			        }


			        constexpr auto operator--() -> Iterator& {
			            BaseT* id_ptr = (BaseT*)&this->id;
			            (*id_ptr)--;
			            return *this;
			        }

			        constexpr auto operator--(int) -> Iterator {
			            Iterator iterator = *this;
			            --(*this);
			            return iterator;
			        }

			        EVO_NODISCARD constexpr auto operator*() const -> const SelfReference& {
			        	return *((const SelfReference*)&this->id);
			        }

			        EVO_NODISCARD constexpr auto operator->() const -> const SelfReference* {
			        	return (const SelfReference*)&this->id;
			        }

			        EVO_NODISCARD constexpr auto operator==(const Iterator& rhs) const -> bool {
			        	return this->id == rhs.id;
			        }
			        EVO_NODISCARD constexpr auto operator!=(const Iterator& rhs) const -> bool {
			        	return this->id != rhs.id;
			        }
			
			    private:
			        ThisT id;
			};

		public:
			explicit constexpr UniqueID(const BaseT& id) : internal_id(id) {}
			constexpr ~UniqueID() = default;

			static constexpr auto dummy() -> SelfReference { return SelfReference(); }


			EVO_NODISCARD constexpr auto operator==(const ThisT& rhs) const -> bool {
				return this->get() == rhs.get();
			}

			EVO_NODISCARD constexpr auto operator!=(const ThisT& rhs) const -> bool {
				return this->get() != rhs.get();
			}


			EVO_NODISCARD constexpr auto get() const -> BaseT { return this->internal_id; }


		protected:
			#if defined(PCIT_CONFIG_DEBUG)
				constexpr UniqueID() : internal_id(std::numeric_limits<BaseT>::max()) {};
			#else
				constexpr UniqueID() : internal_id() {};
			#endif
	
		private:
			BaseT internal_id;
	};

	
	

}