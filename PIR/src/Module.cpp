////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/Module.h"

#include "../include/ReaderAgent.h"

#include <unordered_set>


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

namespace pcit::pir{


	auto Module::getExprType(const Expr& expr) const -> Type {
		evo::debugAssert(
			expr.isConstant(),
			"Module can only get value of expr that is a constant. Use Function::getExprType() instead "
			"(where Function is the function the expr is from"
		);

		switch(expr.getKind()){
			case Expr::Kind::GlobalValue: return this->createPtrType();
			case Expr::Kind::Number:      return ReaderAgent(*this).getNumber(expr).type;
			default: evo::unreachable();
		}
	}


	#if defined(PCIT_CONFIG_DEBUG)
		auto Module::check_param_names(evo::ArrayProxy<Parameter> params) const -> void {
			auto names_seen = std::unordered_set<std::string_view>();

			for(const Parameter& param : params){
				evo::debugAssert(param.getName().empty() == false, "Parameter must have name");
				evo::debugAssert(
					isStandardName(param.getName()), "Invalid name for parameter ({})", param.getName()
				);
				evo::debugAssert(names_seen.contains(param.getName()) == false, "Parameter name already used");

				names_seen.emplace(param.getName());
			}
		}

		auto Module::check_global_name_reusue(std::string_view global_name) const -> void {
			for(const Function& func : this->functions){
				evo::debugAssert(func.getName() != global_name, "global \"{}\" already used", global_name);
			}

			for(const FunctionDecl& func_decl : this->function_decls){
				evo::debugAssert(func_decl.name != global_name, "global \"{}\" already used", global_name);
			}

			for(const GlobalVar& global_var : this->global_vars){
				evo::debugAssert(global_var.name != global_name, "global \"{}\" already used", global_name);
			}
		}
	#endif


	//////////////////////////////////////////////////////////////////////
	// type traits

	static constexpr auto round_up_to_nearest_multiple(size_t num, size_t multiple) -> size_t {
		return (num + (multiple - 1)) & ~(multiple - 1);
	}


	auto Module::sizeOfPtr() const -> size_t {
		return 8;
	}

	auto Module::alignmentOfPtr() const -> size_t {
		return 8;
	}

	auto Module::sizeOfGeneralRegister() const -> size_t {
		return 8;
	}


	auto Module::getSize(const Type& type) const -> size_t {
		switch(type.getKind()){
			case Type::Kind::Void: evo::debugFatalBreak("Cannot get size of Void");

			case Type::Kind::Signed: return round_up_to_nearest_multiple(type.getWidth(), 8) / 8;
			case Type::Kind::Unsigned: return round_up_to_nearest_multiple(type.getWidth(), 8) / 8;

			case Type::Kind::Float: {
				switch(type.getWidth()){
					case 16: return 2;
					case 32: return 4;
					case 64: return 8;
					case 80: return 16;
					case 128: return 16;
				}
			} break;

			case Type::Kind::BFloat: return 2;
			case Type::Kind::Ptr: return this->sizeOfPtr();

			case Type::Kind::Array: {
				const ArrayType& array_type = this->getArrayType(type);
				return this->getSize(array_type.elemType) * array_type.length;
			} break;

			case Type::Kind::Struct: {
				const StructType& struct_type = this->getStructType(type);

				size_t size = 0;

				for(const Type& member : struct_type.members){
					if(struct_type.isPacked){
						size += this->getSize(member);
					}else{
						size += this->getSize(member);
						size = round_up_to_nearest_multiple(size, this->getAlignment(member));
					}
				}

				return round_up_to_nearest_multiple(size, this->getAlignment(type));
			} break;

			case Type::Kind::Function: return this->sizeOfPtr();
		}

		evo::unreachable();
	}


	auto Module::getAlignment(const Type& type) const -> size_t {
		switch(type.getKind()){
			case Type::Kind::Void: evo::debugFatalBreak("Cannot get size of Void");

			case Type::Kind::Signed:
				return std::min<size_t>(round_up_to_nearest_multiple(type.getWidth(), 8) / 8, this->sizeOfPtr());

			case Type::Kind::Unsigned:
				return std::min<size_t>(round_up_to_nearest_multiple(type.getWidth(), 8) / 8, this->sizeOfPtr());

			case Type::Kind::Float: {
				switch(type.getWidth()){
					case 16: return 2;
					case 32: return 4;
					case 64: return 8;
					case 80: return 8;
					case 128: return 8;
				}
			} break;

			case Type::Kind::BFloat: return 2;
			case Type::Kind::Ptr: return this->sizeOfPtr();

			case Type::Kind::Array: {
				const ArrayType& array_type = this->getArrayType(type);
				return this->getAlignment(array_type.elemType);
			} break;

			case Type::Kind::Struct: {
				const StructType& struct_type = this->getStructType(type);

				size_t max_align = 0;

				for(const Type& member : struct_type.members){
					max_align = std::max(max_align, this->getAlignment(member));
				}

				return max_align;
			} break;

			case Type::Kind::Function: return this->sizeOfPtr();
		}

		evo::unreachable();
	}

}