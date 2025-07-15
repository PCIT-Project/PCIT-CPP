////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Evo.h>


#include "./GenericInt.h"
#include "./GenericFloat.h"


namespace pcit::core{

	//////////////////////////////////////////////////////////////////////
	// 
	// GenericValue
	// 	 There is purposely no way to check which value type is held as if GenericValue is being used you should know
	// 	 which type there is held
	// 
	//////////////////////////////////////////////////////////////////////

	class GenericValue{
		public:
			GenericValue()                                              : value(std::monostate()) {}
			explicit GenericValue(GenericInt&& val)                     : value(std::move(val))   {}
			explicit GenericValue(GenericFloat&& val)                   : value(std::move(val))   {}
			explicit GenericValue(bool val)                             : value(std::move(val))   {}
			explicit GenericValue(std::string val)                      : value(std::move(val))   {}
			explicit GenericValue(evo::SmallVector<GenericValue>&& val) : value(std::move(val))   {}
			~GenericValue() = default;


			GenericValue(char character) = delete; // use GenericInt::create<char> instead


			template<class T>
			EVO_NODISCARD auto as() const& -> const T& { static_assert(sizeof(T) == 0, "Not a valid type"); }

			template<class T>
			EVO_NODISCARD auto as() & -> T& { static_assert(sizeof(T) == 0, "Not a valid type"); }

			template<class T>
			EVO_NODISCARD auto as() && -> T&& { static_assert(sizeof(T) == 0, "Not a valid type"); }


			template<>
			EVO_NODISCARD auto as<GenericInt>() const& -> const GenericInt& {
				evo::debugAssert(this->value.is<GenericInt>(), "Generic Value is not this type");
				return this->value.as<GenericInt>();
			}
			template<>
			EVO_NODISCARD auto as<GenericInt>() & -> GenericInt& {
				evo::debugAssert(this->value.is<GenericInt>(), "Generic Value is not this type");
				return this->value.as<GenericInt>();
			}
			template<>
			EVO_NODISCARD auto as<GenericInt>() && -> GenericInt&& {
				evo::debugAssert(this->value.is<GenericInt>(), "Generic Value is not this type");
				return std::move(this->value.as<GenericInt>());
			}


			template<>
			EVO_NODISCARD auto as<GenericFloat>() const& -> const GenericFloat& {
				evo::debugAssert(this->value.is<GenericFloat>(), "Generic Value is not this type");
				return this->value.as<GenericFloat>();
			}
			template<>
			EVO_NODISCARD auto as<GenericFloat>() & -> GenericFloat& {
				evo::debugAssert(this->value.is<GenericFloat>(), "Generic Value is not this type");
				return this->value.as<GenericFloat>();
			}
			template<>
			EVO_NODISCARD auto as<GenericFloat>() && -> GenericFloat&& {
				evo::debugAssert(this->value.is<GenericFloat>(), "Generic Value is not this type");
				return std::move(this->value.as<GenericFloat>());
			}


			template<>
			EVO_NODISCARD auto as<bool>() const& -> const bool& {
				evo::debugAssert(this->value.is<bool>(), "Generic Value is not this type");
				return this->value.as<bool>();
			}
			template<>
			EVO_NODISCARD auto as<bool>() & -> bool& {
				evo::debugAssert(this->value.is<bool>(), "Generic Value is not this type");
				return this->value.as<bool>();
			}
			template<>
			EVO_NODISCARD auto as<bool>() && -> bool&& {
				evo::debugAssert(this->value.is<bool>(), "Generic Value is not this type");
				return std::move(this->value.as<bool>());
			}


			template<>
			EVO_NODISCARD auto as<std::string>() const& -> const std::string& {
				evo::debugAssert(this->value.is<std::string>(), "Generic Value is not this type");
				return this->value.as<std::string>();
			}
			template<>
			EVO_NODISCARD auto as<std::string>() & -> std::string& {
				evo::debugAssert(this->value.is<std::string>(), "Generic Value is not this type");
				return this->value.as<std::string>();
			}
			template<>
			EVO_NODISCARD auto as<std::string>() && -> std::string&& {
				evo::debugAssert(this->value.is<std::string>(), "Generic Value is not this type");
				return std::move(this->value.as<std::string>());
			}


			template<>
			EVO_NODISCARD auto as<evo::SmallVector<GenericValue>>() const& -> const evo::SmallVector<GenericValue>& {
				evo::debugAssert(this->value.is<evo::SmallVector<GenericValue>>(), "Generic Value is not this type");
				return this->value.as<evo::SmallVector<GenericValue>>();
			}
			template<>
			EVO_NODISCARD auto as<evo::SmallVector<GenericValue>>() & -> evo::SmallVector<GenericValue>& {
				evo::debugAssert(this->value.is<evo::SmallVector<GenericValue>>(), "Generic Value is not this type");
				return this->value.as<evo::SmallVector<GenericValue>>();
			}
			template<>
			EVO_NODISCARD auto as<evo::SmallVector<GenericValue>>() && -> evo::SmallVector<GenericValue>&& {
				evo::debugAssert(this->value.is<evo::SmallVector<GenericValue>>(), "Generic Value is not this type");
				return std::move(this->value.as<evo::SmallVector<GenericValue>>());
			}




			EVO_NODISCARD auto operator==(const GenericValue& rhs) const -> bool = default;


			EVO_NODISCARD auto toString() const -> std::string {
				return this->value.visit([](const auto& value) -> std::string {
					using ValueT = std::decay_t<decltype(value)>;

					if constexpr(std::is_same<ValueT, std::monostate>()){
						return "void";

					}else if constexpr(std::is_same<ValueT, GenericInt>()){
						return value.toString(false);

					}else if constexpr(std::is_same<ValueT, GenericFloat>()){
						return value.toString();
						
					}else if constexpr(std::is_same<ValueT, bool>()){
						return evo::boolStr(value);
						
					}else if constexpr(std::is_same<ValueT, std::string>()){
						return value;
						
					}else if constexpr(std::is_same<ValueT, evo::SmallVector<GenericValue>>()){
						auto output = std::string();

						output += "{";

						for(size_t i = 0; const GenericValue& member : value){
							output += member.toString();

							i += 1;

							if(i < value.size()){
								output += ", ";
							}
						}

						output += "}";

						return output;
						
					}else{
						static_assert(false, "Unsupported value type");
					}
				});
			}



			EVO_NODISCARD auto hash() const -> size_t {
				return this->value.visit([](const auto& value) -> size_t {
					using ValueT = std::decay_t<decltype(value)>;

					if constexpr(std::is_same<ValueT, std::monostate>()){
						return 0;

					}else if constexpr(std::is_same<ValueT, GenericInt>()){
						return std::hash<size_t>{}(static_cast<size_t>(value));

					}else if constexpr(std::is_same<ValueT, GenericFloat>()){
						return std::hash<evo::float64_t>{}(static_cast<evo::float64_t>(value));
						
					}else if constexpr(std::is_same<ValueT, bool>()){
						return std::hash<bool>{}(value);
						
					}else if constexpr(std::is_same<ValueT, std::string>()){
						return std::hash<std::string>{}(value);
						
					}else if constexpr(std::is_same<ValueT, evo::SmallVector<GenericValue>>()){
						size_t hash_value = 0;

						for(const GenericValue& member : value){
							hash_value = evo::hashCombine(hash_value, member.hash());
						}

						return hash_value;
						
					}else{
						static_assert(false, "Unsupported value type");
					}
				});
			}

	
		private:
			evo::Variant<
				std::monostate, GenericInt, GenericFloat, bool, std::string, evo::SmallVector<GenericValue>
			> value;
	};


}


namespace std{

	template<>
	struct formatter<pcit::core::GenericValue>{
	    constexpr auto parse(format_parse_context& ctx) const -> auto {
	        return ctx.begin();
	    }

	    auto format(const pcit::core::GenericValue& value, format_context& ctx) const -> auto {
	        return format_to(ctx.out(), "{}", value.toString());
	    }
	};


	template<>
	struct hash<pcit::core::GenericValue>{
		auto operator()(const pcit::core::GenericValue& generic_value) const noexcept -> size_t {
			return generic_value.hash();
		};
	};

	
}
