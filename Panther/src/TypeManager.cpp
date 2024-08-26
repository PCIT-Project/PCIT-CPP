//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "../include/TypeManager.h"

#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


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


		this->types.reserve(4); // TODO: optimize this number

		this->types.emplace_back(new TypeInfo(BaseType::ID(BaseType::Kind::Builtin, 10))); // literal bool
		this->types.emplace_back(new TypeInfo(BaseType::ID(BaseType::Kind::Builtin, 11))); // literal character
		this->types.emplace_back(new TypeInfo(BaseType::ID(BaseType::Kind::Builtin, 26))); // UI8
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


	auto TypeManager::printType(TypeInfo::VoidableID type_info_id) const -> std::string {
		if(type_info_id.isVoid()) [[unlikely]] {
			return "Void";
		}else{
			return this->printType(type_info_id.typeID());
		}
	}

	auto TypeManager::printType(TypeInfo::ID type_info_id) const -> std::string {
		const TypeInfo& type_info = this->getTypeInfo(type_info_id);

		auto get_base_str = [&]() -> std::string {
			switch(type_info.baseTypeID().kind()){
				case BaseType::Kind::Builtin: {
					const BaseType::Builtin::ID builtin_id = type_info.baseTypeID().builtinID();
					const BaseType::Builtin& builtin = this->getBuiltin(builtin_id);

					if(builtin.kind() == Token::Kind::TypeI_N){
						return std::format("I{}", builtin.bitWidth());

					}else if(builtin.kind() == Token::Kind::TypeUI_N){
						return std::format("UI{}", builtin.bitWidth());

					}else{
						return std::string(Token::printKind(builtin.kind()));
					}
				} break;

				case BaseType::Kind::Function: {
					return "{FUNCTION}";
				} break;
			}

			evo::debugFatalBreak("Unknown or unsuport base-type kind");
		};


		std::string type_str = get_base_str();

		bool is_first_qualifer = true;
		for(const AST::Type::Qualifier& qualifier : type_info.qualifiers()){
			if(type_info.qualifiers().size() > 1){
				if(is_first_qualifer){
					is_first_qualifer = false;
				}else{
					type_str += ' ';
				}
			}

			if(qualifier.isPtr){ type_str += '*'; }
			if(qualifier.isReadOnly){ type_str += '|'; }
			if(qualifier.isOptional){ type_str += '?'; }
		}

		return type_str;
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



	//////////////////////////////////////////////////////////////////////
	// type traits

	// https://stackoverflow.com/a/1766566
	static constexpr auto round_up_to_nearest_multiple_of_8(size_t num) -> size_t {
		return (num + (8 - 1)) & ~(8 - 1);
	}

	auto TypeManager::sizeOf(TypeInfo::ID id) const -> size_t {
		const TypeInfo& type_info = this->getTypeInfo(id);
		if(type_info.qualifiers().empty()){ return this->sizeOf(type_info.baseTypeID()); }

		evo::debugAssert(
			type_info.qualifiers().back().isPtr || !type_info.qualifiers().back().isOptional,
			"optionals are not supported yet"
		);

		return this->sizeOfPtr();
	}


	auto TypeManager::sizeOf(BaseType::ID id) const -> size_t {
		switch(id.kind()){
			case BaseType::Kind::Builtin: {
				const BaseType::Builtin& builtin = this->getBuiltin(id.builtinID());

				switch(builtin.kind()){
					case Token::Kind::TypeInt: case Token::Kind::TypeUInt:
						return this->sizeOfGeneralRegister();

					case Token::Kind::TypeISize: case Token::Kind::TypeUSize:
						return this->sizeOfPtr();

					case Token::Kind::TypeI_N: case Token::Kind::TypeUI_N:
						return round_up_to_nearest_multiple_of_8(builtin.bitWidth()) / 8;

					case Token::Kind::TypeF16:    return 2;
					case Token::Kind::TypeBF16:   return 2;
					case Token::Kind::TypeF32:    return 4;
					case Token::Kind::TypeF64:    return 8;
					case Token::Kind::TypeF80:    return 16;
					case Token::Kind::TypeF128:   return 16;
					case Token::Kind::TypeByte:   return 1;
					case Token::Kind::TypeBool:   return 1;
					case Token::Kind::TypeChar:   return 1;
					case Token::Kind::TypeRawPtr: return this->sizeOfPtr();

					// https://en.cppreference.com/w/cpp/language/types
					case Token::Kind::TypeCShort: case Token::Kind::TypeCUShort:
					    return 2;

					case Token::Kind::TypeCInt: case Token::Kind::TypeCUInt:
						return this->platform() == core::Platform::Windows ? 4 : 8;

					case Token::Kind::TypeCLong: case Token::Kind::TypeCULong:
						return 4;

					case Token::Kind::TypeCLongLong: case Token::Kind::TypeCULongLong:
						return 8;

					case Token::Kind::TypeCLongDouble: return this->platform() == core::Platform::Windows ? 8 : 16;

					default: evo::debugFatalBreak("Unknown or unsupported built-in type");
				}
			} break;

			case BaseType::Kind::Function: {
				return this->sizeOfPtr();
			} break;
		}

		evo::debugFatalBreak("Unknown or unsupported base-type kind");
	}

	auto TypeManager::sizeOfPtr() const -> size_t { return 8; }
	auto TypeManager::sizeOfGeneralRegister() const -> size_t { return 8; }


	auto TypeManager::isTriviallyCopyable(TypeInfo::ID) const -> bool {
		return true;
	}

	auto TypeManager::isTriviallyCopyable(BaseType::ID) const -> bool {
		return true;
	}

}