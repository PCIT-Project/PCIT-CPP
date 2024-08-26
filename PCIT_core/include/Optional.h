//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


/*

A nice, easy interface to overload std::optional
This does not create a C++ standard compliant version of std::optional for all given T, so be careful

example of INTERFACE (Fill in T, and follow comments):

	struct Interface{
		static constexpr auto init(T* t) -> void {
			// initialize T to nullopt state
		}

		static constexpr auto has_value(const T& t) -> bool {
			// check if T is not in the nullopt state
		}
	};

example usage (fill in T and INTERFACE):

	template<>
	class std::optional<T> : public pcit::core::Optional<T, INTERFACE>{
		public:
			using pcit::core::Optional<T, INTERFACE>::Optional;
			using pcit::core::Optional<T, INTERFACE>::operator=;
	};

*/


#pragma once


#include <Evo.h>

namespace pcit::core{

	
	template<class T, class INTERFACE>
	class Optional{
		public:
			using value_type = T;

		public:
			constexpr Optional() noexcept : _value{} { INTERFACE::init(&this->_value.held); }
			constexpr Optional(std::nullopt_t) noexcept : _value{} { INTERFACE::init(&this->_value.held); }

			constexpr Optional(const Optional& rhs) = default;
			constexpr Optional(Optional&& rhs) = default;



			// template<class U>
			// constexpr Optional(const optional<U>& other) = default;

			// template<class U>
			// constexpr Optional(optional<U>&& other) = default;


			template<class... Args>
			constexpr explicit Optional(std::in_place_t, Args&&... args) : value{} {
				std::construct_at(&this->_value.held, std::forward<Args...>(args)...);
			}

			template<class U = T>
			constexpr Optional(U&& value) : _value{} {
				std::construct_at(&this->_value.held, std::forward<U>(value));
			}

			constexpr ~Optional() requires(std::is_trivially_destructible_v<T> == false) {
				if(this->has_value()){
					std::destroy_at(&this->_value.held);
				}
			}

			constexpr ~Optional() requires(std::is_trivially_destructible_v<T>) = default;


			constexpr auto operator=(const Optional&) -> Optional& = default;
			constexpr auto operator=(Optional&&) -> Optional& = default;


			constexpr auto operator=(std::nullopt_t) noexcept -> Optional& {
				this->reset();
				return *this;
			}

			constexpr auto operator=(const T& rhs) noexcept -> Optional& {
				this->_value.held = rhs;
				return *this;
			}

			constexpr auto operator=(T&& rhs) noexcept -> Optional& {
				this->_value.held = std::move(rhs);
				return *this;
			}



			///////////////////////////////////
			// observers

			constexpr auto operator->() const  noexcept -> const T*  {
				evo::debugAssert(this->has_value(), "optional does not contain value");
				return &this->_value.held;
			}
			constexpr auto operator->()        noexcept ->       T*  {
				evo::debugAssert(this->has_value(), "optional does not contain value");
				return &this->_value.held;
			}

			constexpr auto operator*() const&  noexcept -> const T&  {
				evo::debugAssert(this->has_value(), "optional does not contain value");
				return this->_value.held;
			}
			constexpr auto operator*()      &  noexcept ->       T&  {
				evo::debugAssert(this->has_value(), "optional does not contain value");
				return this->_value.held;
			}

			constexpr auto operator*() const&& noexcept -> const T&& {
				evo::debugAssert(this->has_value(), "optional does not contain value");
				return std::move(this->_value.held);
			}
			constexpr auto operator*()      && noexcept ->       T&& {
				evo::debugAssert(this->has_value(), "optional does not contain value");
				return std::move(this->_value.held);
			}

			constexpr auto has_value() const -> bool { return INTERFACE::has_value(this->_value.held); }
			constexpr explicit operator bool() const noexcept { return this->has_value(); }



			constexpr auto value() const& -> const T& {
				if(this->has_value() == false){
					throw std::bad_optional_access();
				}

				return this->_value.held;
			}

			constexpr auto value() & -> T& {
				if(this->has_value() == false){
					throw std::bad_optional_access();
				}

				return this->_value.held;
			}

			constexpr auto value() const&& -> const T&& {
				if(this->has_value() == false){
					throw std::bad_optional_access();
				}

				return std::move(this->_value.held);
			}

			constexpr auto value() && -> T&& {
				if(this->has_value() == false){
					throw std::bad_optional_access();
				}

				return std::move(this->_value.held);
			}


			template<class U>
			constexpr auto value_or(U&& default_value) const& -> T {
				if(this->has_value()){
					return this->_value.held;
				}else{
					return std::forward<U>(default_value);
				}
			}

			template<class U>
			constexpr auto value_or(U&& default_value) const&& -> T {
				if(this->has_value()){
					return std::move(this->_value.held);
				}else{
					return std::forward<U>(default_value);
				}
			}


			///////////////////////////////////
			// modifiers

			constexpr auto swap(Optional& rhs) noexcept -> void {
				const T saved_value = this->_value.held;
				this->_value.held = rhs._value.held;
				rhs._value.held = saved_value;
			}

			constexpr auto reset() noexcept -> void {
				if constexpr(std::is_trivially_destructible_v<T> == false){
					std::destroy_at(&this->_value.held);
				}

				INTERFACE::init(&this->_value.held);
			}

			template<class... Args>
			constexpr auto emplace(Args&&... args) -> T& {
				std::construct_at(&this->_value.held, std::forward<Args...>(args)...);
				return this->_value.held;
			}



		private:
			union{
				evo::byte dummy[1];
				T held;
			} _value;
	};


}


