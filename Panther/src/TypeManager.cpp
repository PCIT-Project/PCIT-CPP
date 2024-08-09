//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "../include/TypeManager.h"

namespace pcit::panther{

	
	TypeManager::~TypeManager(){
		for(BaseType::Builtin* builtin : this->builtins){
			delete builtin;
		}

		for(TypeInfo* type_info : this->types){
			delete type_info;
		}
	}
	

	auto TypeManager::initBuiltins() -> void {
		evo::debugAssert(this->builtinsInitialized() == false, "builtins already initialized");

		this->builtins.reserve(32);

		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeInt));
		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeISize));
		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeUInt));
		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeUSize));
		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeF16));
		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeF32));
		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeF64));
		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeF80));
		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeF128));
		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeByte));
		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeBool));
		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeChar));
		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeRawPtr));

		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeCShort));
		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeCUShort));
		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeCInt));
		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeCUInt));
		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeCLong));
		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeCULong));
		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeCLongLong));
		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeCULongLong));
		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeCLongDouble));

		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeI_N, 8));
		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeI_N, 16));
		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeI_N, 32));
		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeI_N, 64));

		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeUI_N, 8));
		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeUI_N, 16));
		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeUI_N, 32));
		this->builtins.emplace_back(new BaseType::Builtin(Token::Kind::TypeUI_N, 64));
	}

	auto TypeManager::builtinsInitialized() const -> bool {
		return !this->builtins.empty();
	}


	//////////////////////////////////////////////////////////////////////
	// type

	auto TypeManager::getTypeInfo(TypeInfo::ID id) const -> const TypeInfo& {
		const auto lock = std::shared_lock(this->types_mutex);
		return *this->types[id.get()];
	}

	auto TypeManager::getOrCreateTypeInfo(TypeInfo&& lookup_type_info) -> TypeInfo::ID {
		const auto lock = std::unique_lock(this->types_mutex);

		for(uint32_t i = 0; const TypeInfo* type_info : this->types){
			if(*type_info == lookup_type_info){
				return TypeInfo::ID(i);
			}
		
			i += 1;
		}

		const auto new_type_id = TypeInfo::ID(uint32_t(this->types.size()));
		this->types.emplace_back(new TypeInfo(std::forward<TypeInfo>(lookup_type_info)));
		return new_type_id;
	}


	//////////////////////////////////////////////////////////////////////
	// function

	auto TypeManager::getFunction(BaseType::Function::ID id) const -> const BaseType::Function& {
		const auto lock = std::shared_lock(this->functions_mutex);
		return *this->functions[id.get()];
	}

	auto TypeManager::getOrCreateFunction(BaseType::Function lookup_func) -> BaseType::ID {
		const auto lock = std::unique_lock(this->functions_mutex);
		
		for(uint32_t i = 0; const BaseType::Function* function : this->functions){
			if(*function == lookup_func){
				return BaseType::ID(BaseType::Kind::Function, i);
			}
		
			i += 1;
		}

		const auto new_id = BaseType::ID(BaseType::Kind::Function, uint32_t(this->functions.size()));
		this->functions.emplace_back(new BaseType::Function(lookup_func));
		return new_id;
	}


	//////////////////////////////////////////////////////////////////////
	// builtin

	auto TypeManager::getBuiltin(BaseType::Builtin::ID id) const -> const BaseType::Builtin& {
		const auto lock = std::shared_lock(this->builtins_mutex);
		return *this->builtins[id.get()];
	}

	auto TypeManager::getOrCreateBuiltinBaseType(Token::Kind kind) -> BaseType::ID {
		return this->get_or_create_builtin_base_type_impl(BaseType::Builtin(kind));
	}

	auto TypeManager::getOrCreateBuiltinBaseType(Token::Kind kind, uint32_t bit_width) -> BaseType::ID {
		return this->get_or_create_builtin_base_type_impl(BaseType::Builtin(kind, bit_width));
	}

	auto TypeManager::get_or_create_builtin_base_type_impl(const BaseType::Builtin& lookup_type) -> BaseType::ID {
		const auto lock = std::unique_lock(this->builtins_mutex);

		for(uint32_t i = 0; const BaseType::Builtin* builtin_type : this->builtins){
			if(*builtin_type == lookup_type){
				return BaseType::ID(BaseType::Kind::Builtin, i);
			}
		
			i += 1;
		}

		const auto new_id = BaseType::ID(BaseType::Kind::Builtin, uint32_t(this->builtins.size()));
		this->builtins.emplace_back(new BaseType::Builtin(lookup_type));
		return new_id;
	}


}