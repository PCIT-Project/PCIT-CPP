////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


/*

A nice, easy interface to overload std::optional

HOW TO USE:

	1) specialize pcit::core::OptionalInterface for type T (where T is your custom type)
	
		namespace pcit::core{

			template<>
			struct OptionalInterface<T>{
				static constexpr auto init(T* t) -> void {
					// initialize T to nullopt state
				}

				static constexpr auto has_value(const T& t) -> bool {
					// return true if T is not in the nullopt state
				}
			};

		}


	1.5) if you need the OptionalInterface to access private stuff of your T, then you can add
		 "friend struct pcit::core::OptionalInterface<T>" to your T


	2) specialize std::optional
	
		namespace std{

			template<>
			class optional<T> : public pcit::core::Optional<T>{
				public:
					using pcit::core::Optional<T>::Optional;
					using pcit::core::Optional<T>::operator=;
			};

		}

*/


#pragma once


#include <Evo.h>

namespace pcit::core{


	template<class T>
	struct OptionalInterface{
		static constexpr auto init(T* t) -> void {
			static_assert(false, "No specialization for pcit::core::OptionalInterface for this type exists");
		}

		static constexpr auto has_value(const T& t) -> bool {
			static_assert(false, "No specialization for pcit::core::OptionalInterface for this type exists");
		}
	};




	
	template<class T>
	class Optional{
		public:
			using value_type = T;

		public:
			constexpr Optional() noexcept : _data{} { OptionalInterface<T>::init(&this->data()); }
			constexpr Optional(std::nullopt_t) noexcept : _data{} { OptionalInterface<T>::init(&this->data()); }


			constexpr Optional(const Optional& rhs) requires(std::is_copy_constructible_v<T> == false) = delete;

			constexpr Optional(const Optional& rhs)
				requires(std::is_copy_constructible_v<T> && std::is_trivially_copy_constructible_v<T>) = default;

			constexpr Optional(const Optional& rhs)
				requires(std::is_copy_constructible_v<T> && std::is_trivially_copy_constructible_v<T> == false) 
				: _data{} {

				if constexpr(std::is_trivially_destructible<T>() == false){
					if(this->has_value()){
						std::destroy_at(this);
					}
				}

				if(rhs.has_value()){
					std::construct_at(&this->data(), rhs.data());
				}else{
					std::construct_at(this);
				}
			}


			constexpr Optional(Optional&& rhs) requires(std::is_move_constructible_v<T> == false) = delete;

			constexpr Optional(Optional&& rhs)
				requires(std::is_move_constructible_v<T> && std::is_trivially_move_constructible_v<T>) = default;

			constexpr Optional(Optional&& rhs)
				requires(std::is_move_constructible_v<T> && std::is_trivially_move_constructible_v<T> == false) 
				: _data{} {

				if constexpr(std::is_trivially_destructible<T>() == false){
					if(this->has_value()){
						std::destroy_at(this);
					}
				}

				if(rhs.has_value()){
					std::construct_at(&this->data(), std::move(rhs.data()));
				}else{
					std::construct_at(this);
				}
			}



			// template<class U>
			// constexpr Optional(const optional<U>& other) = default;

			// template<class U>
			// constexpr Optional(optional<U>&& other) = default;


			template<class... Args>
			constexpr explicit Optional(std::in_place_t, Args&&... args) : _data{} {
				std::construct_at(&this->data(), std::forward<Args...>(args)...);
			}

			template<class U = T>
			constexpr Optional(U&& value) : _data{} {
				std::construct_at(&this->data(), std::forward<U>(value));
			}

			constexpr ~Optional() requires(std::is_trivially_destructible_v<T> == false) {
				if(this->has_value()){
					std::destroy_at(&this->data());
				}
			}

			constexpr ~Optional() requires(std::is_trivially_destructible_v<T>) = default;


			constexpr auto operator=(const Optional& rhs) -> Optional& = default;

			constexpr auto operator=(Optional&& rhs) -> Optional& = default;



			constexpr auto operator=(std::nullopt_t) noexcept -> Optional& {
				this->reset();
				return *this;
			}

			constexpr auto operator=(const T& rhs) noexcept -> Optional& {
				if constexpr(std::is_trivially_destructible<T>() == false){
					std::destroy_at(this);
				}

				std::construct_at(this, rhs);

				return *this;
			}

			constexpr auto operator=(T&& rhs) noexcept -> Optional& {
				if constexpr(std::is_trivially_destructible<T>() == false){
					std::destroy_at(this);
				}

				std::construct_at(this, std::move(rhs));
				
				return *this;
			}



			EVO_NODISCARD constexpr auto operator==(const Optional& rhs) const -> bool {
				if(this->has_value() != rhs.has_value()){ return false; }
				if(this->has_value() == false){ return true; }
				return this->value() == rhs.value();
			}


			EVO_NODISCARD constexpr auto operator==(const T& rhs) const -> bool {
				if(this->has_value() == false){ return false; }
				return this->value() == rhs;
			}


			///////////////////////////////////
			// observers

			EVO_NODISCARD constexpr auto operator->() const noexcept -> const T* {
				evo::debugAssert(this->has_value(), "optional does not contain value");
				return &this->data();
			}
			EVO_NODISCARD constexpr auto operator->() noexcept -> T* {
				evo::debugAssert(this->has_value(), "optional does not contain value");
				return &this->data();
			}

			EVO_NODISCARD constexpr auto operator*() const& noexcept -> const T& {
				evo::debugAssert(this->has_value(), "optional does not contain value");
				return this->data();
			}
			EVO_NODISCARD constexpr auto operator*() & noexcept -> T& {
				evo::debugAssert(this->has_value(), "optional does not contain value");
				return this->data();
			}
			EVO_NODISCARD constexpr auto operator*() && noexcept -> T&& {
				evo::debugAssert(this->has_value(), "optional does not contain value");
				return std::move(this->data());
			}

			EVO_NODISCARD constexpr auto has_value() const -> bool {
				return OptionalInterface<T>::has_value(this->data());
			}

			EVO_NODISCARD constexpr explicit operator bool() const noexcept { return this->has_value(); }



			EVO_NODISCARD constexpr auto value() const& -> const T& {
				// if(this->has_value() == false){ throw std::bad_optional_access(); }
				evo::Assert(this->has_value(), "optional does not contain value");

				return this->data();
			}

			EVO_NODISCARD constexpr auto value() & -> T& {
				// if(this->has_value() == false){ throw std::bad_optional_access(); }
				evo::Assert(this->has_value(), "optional does not contain value");

				return this->data();
			}

			EVO_NODISCARD constexpr auto value() && -> T&& {
				// if(this->has_value() == false){ throw std::bad_optional_access(); }
				evo::Assert(this->has_value(), "optional does not contain value");

				return std::move(this->data());
			}


			template<class U>
			EVO_NODISCARD constexpr auto value_or(U&& default_value) const& -> T {
				if(this->has_value()){
					return this->data();
				}else{
					return std::forward<U>(default_value);
				}
			}

			template<class U>
			EVO_NODISCARD constexpr auto value_or(U&& default_value) const&& -> T {
				if(this->has_value()){
					return std::move(this->data());
				}else{
					return std::forward<U>(default_value);
				}
			}


			///////////////////////////////////
			// modifiers

			constexpr auto swap(Optional& rhs) noexcept -> void {
				if(this->has_value() && rhs.has_value()){
					std::swap(this->data(), rhs.data());

				}else{
					Optional temp_holder = std::move(rhs);
					rhs = std::move(this->data());
					this->data() = std::move(temp_holder);
				}
			}

			constexpr auto reset() noexcept -> void {
				if constexpr(std::is_trivially_destructible<T>() == false){
					std::destroy_at(this);
				}

				OptionalInterface<T>::init(&this->data());
			}

			template<class... Args>
			constexpr auto emplace(Args&&... args) -> T& {
				if constexpr(std::is_trivially_destructible<T>() == false){
					std::destroy_at(this);
				}

				std::construct_at(&this->data(), std::forward<Args...>(args)...);
				return this->data();
			}


		private:
			EVO_NODISCARD constexpr auto data() const& -> const T&  { return evo::bitCast<T>(this->_data); }
			EVO_NODISCARD constexpr auto data()      & ->       T&  { return evo::bitCast<T>(this->_data); }
			EVO_NODISCARD constexpr auto data()     && ->       T&& { return evo::bitCast<T>(this->_data); }

		private:
			evo::byte _data[sizeof(T)];
	};


	template<class T>
	EVO_NODISCARD constexpr auto operator==(const T& lhs, const Optional<T>& rhs) -> bool {
		return rhs == lhs;
	}
}


