////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>

#include <PCIT_core.h>

#include "./misc.h"
#include "./enums.h"

namespace pcit::pir{

	//////////////////////////////////////////////////////////////////////
	// 
	// Create types through Module
	// Lookup extra info about types in Module as well
	// 

	class Type{
		public:
			enum class Kind : uint32_t {
				VOID,
				INTEGER,
				BOOL,
				FLOAT,
				BFLOAT,
				PTR,
				ARRAY,
				STRUCT,
				FUNCTION,
			};

		public:
			~Type() = default;

			Type(const Type&) = default;
			Type(Type&&) = default;

			auto operator=(const Type& rhs) -> Type& = default;
			auto operator=(Type&& rhs) -> Type& = default;

			EVO_NODISCARD auto operator==(const Type&) const -> bool = default;


			///////////////////////////////////
			// get values

			EVO_NODISCARD auto kind() const -> Kind { return this->_kind; }

			EVO_NODISCARD auto getWidth() const -> uint32_t {
				evo::debugAssert(this->hasWidth(), "This type does not have a width");
				return this->number;
			}


			///////////////////////////////////
			// property checking


			EVO_NODISCARD auto isFloat() const -> bool {
				return this->_kind == Kind::FLOAT || this->_kind == Kind::BFLOAT;
			}

			EVO_NODISCARD auto isNumeric() const -> bool {
				return this->_kind == Kind::INTEGER || this->isFloat();
			}

			EVO_NODISCARD auto isAggregate() const -> bool {
				return this->_kind == Kind::ARRAY || this->_kind == Kind::STRUCT;
			}

			EVO_NODISCARD auto isPrimitive() const -> bool {
				return this->_kind == Kind::INTEGER
					|| this->_kind == Kind::BOOL
					|| this->_kind == Kind::FLOAT
					|| this->_kind == Kind::BFLOAT
					|| this->_kind == Kind::PTR;
			}

			EVO_NODISCARD auto hasWidth() const -> bool {
				return this->_kind == Kind::INTEGER || this->_kind == Kind::FLOAT;
			}


		private:
			constexpr Type(Kind type_kind, uint32_t _number) : _kind(type_kind), number(_number) {}
			constexpr Type(Kind type_kind) : _kind(type_kind), number(0) {}
	
		private:
			Kind _kind;
			uint32_t number; // might be width, might be ID, might be nothing

			friend class Module;
			friend struct core::OptionalInterface<Type>;
	};

	static_assert(sizeof(Type) == sizeof(uint64_t), "Unexpected size for pir::Type");
	static_assert(std::is_trivially_copyable<Type>(), "pir::Type is not trivially copyable");



	struct ArrayType{
		Type elemType;
		uint64_t length;

		EVO_NODISCARD auto isString() const -> bool {
			return this->elemType.kind() == Type::Kind::INTEGER && this->elemType.getWidth() == 8;
		}
	};
 
	struct StructType{
		std::string name;
		evo::SmallVector<Type> members;
		bool isPacked;
	};


	struct FunctionType{
		evo::SmallVector<Type> parameters;
		CallingConvention callingConvention;
		Type returnType;
	};


}



namespace pcit::core{

	template<>
	struct OptionalInterface<pir::Type>{
		static constexpr auto init(pir::Type* t) -> void {
			new(t) pir::Type(pir::Type::Kind::VOID, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const pir::Type& t) -> bool {
			return t.number != std::numeric_limits<uint32_t>::max();
		}
	};

}



namespace std{
	
	template<>
	struct hash<pcit::pir::Type>{
		auto operator()(const pcit::pir::Type& type) const noexcept -> size_t {
			return std::hash<uint64_t>{}(evo::bitCast<uint64_t>(type));
		};
	};


	template<>
	class optional<pcit::pir::Type> : public pcit::core::Optional<pcit::pir::Type>{
		public:
			using pcit::core::Optional<pcit::pir::Type>::Optional;
			using pcit::core::Optional<pcit::pir::Type>::operator=;
	};
	
}


