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

	// Create types through Module
	class Type{
		public:
			enum class Kind{
				Void,
				Signed,
				Unsigned,
				Float,
				BFloat,
				Ptr,
				Array,
				Struct,
				Function,
			};

		public:
			~Type() = default;

			Type(const Type&) = default;
			Type(Type&&) = default;

			auto operator=(const Type& rhs) -> Type& {
				std::construct_at(this, rhs);
				return *this;
			}

			auto operator=(Type&& rhs) -> Type& {
				std::construct_at(this, std::move(rhs));
				return *this;
			}

			EVO_NODISCARD auto operator==(const Type&) const -> bool = default;


			///////////////////////////////////
			// get values

			EVO_NODISCARD auto getKind() const -> Kind { return this->kind; }

			EVO_NODISCARD auto getWidth() const -> uint32_t {
				evo::debugAssert(this->hasWidth(), "This type does not have a width");
				return this->number;
			}


			///////////////////////////////////
			// property checking

			EVO_NODISCARD auto isIntegral() const -> bool {
				return this->kind == Kind::Signed || this->kind == Kind::Unsigned;
			}

			EVO_NODISCARD auto isFloat() const -> bool {
				return this->kind == Kind::Float || this->kind == Kind::BFloat;
			}

			EVO_NODISCARD auto isNumeric() const -> bool {
				return this->isIntegral() || this->isFloat();
			}

			EVO_NODISCARD auto isConstant() const -> bool {
				return this->isNumeric();
			}

			EVO_NODISCARD auto hasWidth() const -> bool {
				return this->kind == Kind::Signed || this->kind == Kind::Unsigned || this->kind == Kind::Float;
			}


		private:
			Type(Kind _kind, uint32_t _number) : kind(_kind), number(_number) {}
			Type(Kind _kind) : kind(_kind), number(0) {}
	
		private:
			Kind kind;
			uint32_t number; // might be width, might be ID, might be nothing

			friend class Module;
	};



	struct ArrayType{
		Type elemType;
		size_t length;
	};

	struct StructType{
		std::string name;
		evo::SmallVector<Type> members;
		bool isPacked;

		StructType(std::string&& _name, evo::SmallVector<Type> _members, bool is_packed) :
			name(std::move(_name)), members(std::move(_members)), isPacked(is_packed) {
			evo::debugAssert(isStandardName(this->name), "Invalid name for struct type ({})", this->name);
		}
	};


	struct FunctionType{
		evo::SmallVector<Type> parameters;
		CallingConvention callingConvention;
		Type returnType;
	};


}


