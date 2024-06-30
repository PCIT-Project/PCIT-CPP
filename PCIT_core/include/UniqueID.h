//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0      //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>

namespace pcit::core{


	template<class BaseT, class SelfReference>
	class UniqueID{
		public:
			using ThisT = UniqueID<BaseT, SelfReference>;

			template<class T>
			class IteratorImpl{
			    public:
			        using difference_type   = std::ptrdiff_t;
			        using value_type        = T;

			        constexpr IteratorImpl(const ThisT& _id) noexcept : id(_id) {};
			        constexpr ~IteratorImpl() = default;


			        constexpr auto operator++() noexcept -> IteratorImpl& {
			            BaseT* id_ptr = (BaseT*)&this->id;
			            (*id_ptr)++;
			            return *this;
			        };

			        constexpr auto operator++(int) noexcept -> IteratorImpl {
			            IteratorImpl iterator = *this;
			            ++(*this);
			            return iterator;
			        };


			        constexpr auto operator--() noexcept -> IteratorImpl& {
			            BaseT* id_ptr = (BaseT*)&this->id;
			            (*id_ptr)--;
			            return *this;
			        };

			        constexpr auto operator--(int) noexcept -> IteratorImpl {
			            IteratorImpl iterator = *this;
			            --(*this);
			            return iterator;
			        };

			        EVO_NODISCARD constexpr auto operator*() const noexcept -> const T& {
			        	return *((const T*)&this->id);
			        };

			        EVO_NODISCARD constexpr auto operator->() const noexcept -> const T* {
			        	return (const T*)&this->id;
			        };

			        EVO_NODISCARD constexpr auto operator==(const IteratorImpl& rhs) const noexcept -> bool {
			        	return this->id == rhs.id;
			        };
			        EVO_NODISCARD constexpr auto operator!=(const IteratorImpl& rhs) const noexcept -> bool {
			        	return this->id != rhs.id;
			        };
			
			    private:
			        ThisT id;
			};

		public:
			explicit constexpr UniqueID(const BaseT& id) noexcept : internal_id(id) {};
			constexpr ~UniqueID() noexcept = default;


			EVO_NODISCARD constexpr auto get() const noexcept -> BaseT { return this->internal_id; };
	
		private:
			BaseT internal_id;
	};


	template<class BaseT, class SelfReference>
	class UniqueComparableID{
		public:
			using ThisT = UniqueComparableID<BaseT, SelfReference>;

			template<class T>
			class IteratorImpl{
			    public:
			        using difference_type   = std::ptrdiff_t;
			        using value_type        = T;

			        constexpr IteratorImpl(const ThisT& _id) noexcept : id(_id) {};
			        constexpr ~IteratorImpl() = default;


			        constexpr auto operator++() noexcept -> IteratorImpl& {
			            BaseT* id_ptr = (BaseT*)&this->id;
			            (*id_ptr)++;
			            return *this;
			        };

			        constexpr auto operator++(int) noexcept -> IteratorImpl {
			            IteratorImpl iterator = *this;
			            ++(*this);
			            return iterator;
			        };


			        constexpr auto operator--() noexcept -> IteratorImpl& {
			            BaseT* id_ptr = (BaseT*)&this->id;
			            (*id_ptr)--;
			            return *this;
			        };

			        constexpr auto operator--(int) noexcept -> IteratorImpl {
			            IteratorImpl iterator = *this;
			            --(*this);
			            return iterator;
			        };

			        EVO_NODISCARD constexpr auto operator*() const noexcept -> const T& {
			        	return *((const T*)&this->id);
			        };

			        EVO_NODISCARD constexpr auto operator->() const noexcept -> const T* {
			        	return (const T*)&this->id;
			        };

			        EVO_NODISCARD constexpr auto operator==(const IteratorImpl& rhs) const noexcept -> bool {
			        	return this->id == rhs.id;
			        };
			        EVO_NODISCARD constexpr auto operator!=(const IteratorImpl& rhs) const noexcept -> bool {
			        	return this->id != rhs.id;
			        };
			
			    private:
			        ThisT id;
			};

		public:
			explicit constexpr UniqueComparableID(const BaseT& id) noexcept : internal_id(id) {};
			constexpr ~UniqueComparableID() noexcept = default;


			EVO_NODISCARD constexpr auto operator==(const ThisT& rhs) const noexcept -> bool {
				return this->get() == rhs.get();
			};

			EVO_NODISCARD constexpr auto operator!=(const ThisT& rhs) const noexcept -> bool {
				return this->get() != rhs.get();
			};


			EVO_NODISCARD constexpr auto get() const noexcept -> BaseT { return this->internal_id; };
	
		private:
			BaseT internal_id;
	};

	
	

};