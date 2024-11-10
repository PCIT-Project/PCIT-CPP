////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/TypeManager.h"

#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::panther{


	auto TypeManager::initPrimitives() -> void {
		evo::debugAssert(this->primitivesInitialized() == false, "primitives already initialized");

		this->primitives.emplace_back(Token::Kind::TypeInt);
		this->primitives.emplace_back(Token::Kind::TypeISize);
		this->primitives.emplace_back(Token::Kind::TypeUInt);
		const BaseType::Primitive::ID type_usize = this->primitives.emplace_back(Token::Kind::TypeUSize);
		this->primitives.emplace_back(Token::Kind::TypeF16);
		this->primitives.emplace_back(Token::Kind::TypeF32);
		this->primitives.emplace_back(Token::Kind::TypeF64);
		this->primitives.emplace_back(Token::Kind::TypeF80);
		this->primitives.emplace_back(Token::Kind::TypeF128);
		this->primitives.emplace_back(Token::Kind::TypeByte);
		const BaseType::Primitive::ID type_bool = this->primitives.emplace_back(Token::Kind::TypeBool);
		const BaseType::Primitive::ID type_char = this->primitives.emplace_back(Token::Kind::TypeChar);
		const BaseType::Primitive::ID type_raw_ptr = this->primitives.emplace_back(Token::Kind::TypeRawPtr);
		const BaseType::Primitive::ID type_type_id = this->primitives.emplace_back(Token::Kind::TypeTypeID);

		this->primitives.emplace_back(Token::Kind::TypeCShort);
		this->primitives.emplace_back(Token::Kind::TypeCUShort);
		this->primitives.emplace_back(Token::Kind::TypeCInt);
		this->primitives.emplace_back(Token::Kind::TypeCUInt);
		this->primitives.emplace_back(Token::Kind::TypeCLong);
		this->primitives.emplace_back(Token::Kind::TypeCULong);
		this->primitives.emplace_back(Token::Kind::TypeCLongLong);
		this->primitives.emplace_back(Token::Kind::TypeCULongLong);
		this->primitives.emplace_back(Token::Kind::TypeCLongDouble);

		this->primitives.emplace_back(Token::Kind::TypeI_N, 8);
		this->primitives.emplace_back(Token::Kind::TypeI_N, 16);
		this->primitives.emplace_back(Token::Kind::TypeI_N, 32);
		this->primitives.emplace_back(Token::Kind::TypeI_N, 64);

		const BaseType::Primitive::ID type_ui8 = this->primitives.emplace_back(Token::Kind::TypeUI_N, 8);
		this->primitives.emplace_back(Token::Kind::TypeUI_N, 16);
		this->primitives.emplace_back(Token::Kind::TypeUI_N, 32);
		this->primitives.emplace_back(Token::Kind::TypeUI_N, 64);

		this->types.emplace_back(TypeInfo(BaseType::ID(BaseType::Kind::Primitive, type_bool.get())));
		this->types.emplace_back(TypeInfo(BaseType::ID(BaseType::Kind::Primitive, type_char.get())));
		this->types.emplace_back(TypeInfo(BaseType::ID(BaseType::Kind::Primitive, type_ui8.get())));
		this->types.emplace_back(TypeInfo(BaseType::ID(BaseType::Kind::Primitive, type_usize.get())));
		this->types.emplace_back(TypeInfo(BaseType::ID(BaseType::Kind::Primitive, type_type_id.get())));
		this->types.emplace_back(TypeInfo(BaseType::ID(BaseType::Kind::Primitive, type_raw_ptr.get())));
	}

	auto TypeManager::primitivesInitialized() const -> bool {
		return !this->primitives.empty();
	}


	//////////////////////////////////////////////////////////////////////
	// type

	auto TypeManager::getTypeInfo(TypeInfo::ID id) const -> const TypeInfo& {
		const auto lock = std::shared_lock(this->types_mutex);
		return this->types[id];
	}

	auto TypeManager::getOrCreateTypeInfo(TypeInfo&& lookup_type_info) -> TypeInfo::ID {
		const auto lock = std::unique_lock(this->types_mutex);

		for(uint32_t i = 0; i < this->types.size(); i+=1){
			if(this->types[TypeInfo::ID(i)] == lookup_type_info){
				return TypeInfo::ID(i);
			}
		}

		return this->types.emplace_back(std::move(lookup_type_info));
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
				case BaseType::Kind::Primitive: {
					const BaseType::Primitive::ID primitive_id = type_info.baseTypeID().primitiveID();
					const BaseType::Primitive& primitive = this->getPrimitive(primitive_id);

					if(primitive.kind() == Token::Kind::TypeI_N){
						return std::format("I{}", primitive.bitWidth());

					}else if(primitive.kind() == Token::Kind::TypeUI_N){
						return std::format("UI{}", primitive.bitWidth());

					}else{
						return std::string(Token::printKind(primitive.kind()));
					}
				} break;

				case BaseType::Kind::Function: {
					// TODO: fix this
					return "{FUNCTION}";
				} break;

				case BaseType::Kind::Alias: {
					const BaseType::Alias::ID alias_id = type_info.baseTypeID().aliasID();
					const BaseType::Alias& alias = this->getAlias(alias_id);
					return this->printType(alias.aliasedType);
				} break;

				case BaseType::Kind::Dummy: evo::debugFatalBreak("Dummy type should not be used");
			}

			evo::debugFatalBreak("Unknown or unsuport base-type kind");
		};


		std::string type_str = get_base_str();

		bool is_first_qualifer = type_str.back() != '*' && type_str.back() != '|' && type_str.back() != '?';
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
		return this->functions[id];
	}

	auto TypeManager::getOrCreateFunction(BaseType::Function&& lookup_func) -> BaseType::ID {
		const auto lock = std::unique_lock(this->functions_mutex);

		for(uint32_t i = 0; i < this->functions.size(); i+=1){
			if(this->functions[BaseType::Function::ID(i)] == lookup_func){
				return BaseType::ID(BaseType::Kind::Function, i);
			}
		}

		const BaseType::Function::ID new_function = this->functions.emplace_back(lookup_func);
		return BaseType::ID(BaseType::Kind::Function, new_function.get());
	}


	//////////////////////////////////////////////////////////////////////
	// primitive

	auto TypeManager::getPrimitive(BaseType::Primitive::ID id) const -> const BaseType::Primitive& {
		const auto lock = std::shared_lock(this->primitives_mutex);
		return this->primitives[id];
	}

	auto TypeManager::getOrCreatePrimitiveBaseType(Token::Kind kind) -> BaseType::ID {
		return this->get_or_create_primitive_base_type_impl(BaseType::Primitive(kind));
	}

	auto TypeManager::getOrCreatePrimitiveBaseType(Token::Kind kind, uint32_t bit_width) -> BaseType::ID {
		return this->get_or_create_primitive_base_type_impl(BaseType::Primitive(kind, bit_width));
	}

	auto TypeManager::get_or_create_primitive_base_type_impl(const BaseType::Primitive& lookup_type) -> BaseType::ID {
		const auto lock = std::unique_lock(this->primitives_mutex);

		for(uint32_t i = 0; i < this->primitives.size(); i+=1){
			if(this->primitives[BaseType::Primitive::ID(i)] == lookup_type){
				return BaseType::ID(BaseType::Kind::Primitive, i);
			}
		}

		const BaseType::Primitive::ID new_primitive = this->primitives.emplace_back(lookup_type);
		return BaseType::ID(BaseType::Kind::Primitive, new_primitive.get());
	}


	//////////////////////////////////////////////////////////////////////
	// aliases

	auto TypeManager::getAlias(BaseType::Alias::ID id) const -> const BaseType::Alias& {
		const auto lock = std::shared_lock(this->aliases_mutex);
		return this->aliases[id];
	}


	auto TypeManager::getOrCreateAlias(BaseType::Alias&& lookup_type) -> BaseType::ID {
		const auto lock = std::shared_lock(this->aliases_mutex);
		
		for(uint32_t i = 0; i < this->aliases.size(); i+=1){
			if(this->aliases[BaseType::Alias::ID(i)] == lookup_type){
				return BaseType::ID(BaseType::Kind::Alias, i);
			}
		}

		const BaseType::Alias::ID new_alias = this->aliases.emplace_back(lookup_type);
		return BaseType::ID(BaseType::Kind::Alias, new_alias.get());
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
			case BaseType::Kind::Primitive: {
				const BaseType::Primitive& primitive = this->getPrimitive(id.primitiveID());

				switch(primitive.kind()){
					case Token::Kind::TypeInt: case Token::Kind::TypeUInt:
						return this->sizeOfGeneralRegister();

					case Token::Kind::TypeISize: case Token::Kind::TypeUSize:
						return this->sizeOfPtr();

					case Token::Kind::TypeI_N: case Token::Kind::TypeUI_N:
						return round_up_to_nearest_multiple_of_8(primitive.bitWidth()) / 8;

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
					case Token::Kind::TypeTypeID: return 4;

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

			case BaseType::Kind::Alias: {
				const BaseType::Alias& alias = this->getAlias(id.aliasID());
				evo::debugAssert(alias.aliasedType.isVoid() == false, "cannot get sizeof type `Void`");
				return this->sizeOf(alias.aliasedType.typeID());
			} break;

			case BaseType::Kind::Dummy: evo::debugFatalBreak("Dummy type should not be used");
		}

		evo::debugFatalBreak("Unknown or unsupported base-type kind");
	}

	auto TypeManager::sizeOfPtr() const -> size_t { return 8; }
	auto TypeManager::sizeOfGeneralRegister() const -> size_t { return 8; }


	///////////////////////////////////
	// isTriviallyCopyable

	auto TypeManager::isTriviallyCopyable(TypeInfo::ID id) const -> bool {
		const TypeInfo& type_info = this->getTypeInfo(id);
		if(type_info.qualifiers().empty()){ return this->isTriviallyCopyable(type_info.baseTypeID()); }
		return true;
	}

	auto TypeManager::isTriviallyCopyable(BaseType::ID) const -> bool {
		return true;
	}


	///////////////////////////////////
	// isTriviallyDestructable

	auto TypeManager::isTriviallyDestructable(TypeInfo::ID id) const -> bool {
		const TypeInfo& type_info = this->getTypeInfo(id);
		if(type_info.qualifiers().empty()){ return this->isPrimitive(type_info.baseTypeID()); }
		return true;
	}

	auto TypeManager::isTriviallyDestructable(BaseType::ID) const -> bool {
		return true;
	}


	///////////////////////////////////
	// isPrimitive

	auto TypeManager::isPrimitive(TypeInfo::VoidableID id) const -> bool {
		if(id.isVoid()){ return false; }
		return this->isPrimitive(id.typeID());
	}

	auto TypeManager::isPrimitive(TypeInfo::ID id) const -> bool {
		const TypeInfo& type_info = this->getTypeInfo(id);
		if(type_info.qualifiers().empty()){ return this->isPrimitive(type_info.baseTypeID()); }
		return false;
	}

	auto TypeManager::isPrimitive(BaseType::ID id) const -> bool {
		return id.kind() == BaseType::Kind::Primitive;
	}


	///////////////////////////////////
	// isIntegral

	auto TypeManager::isIntegral(TypeInfo::VoidableID id) const -> bool {
		if(id.isVoid()){ return false; }
		return this->isIntegral(id.typeID());
	}

	auto TypeManager::isIntegral(TypeInfo::ID id) const -> bool {
		const TypeInfo& type_info = this->getTypeInfo(id);

		if(type_info.qualifiers().empty()){ return this->isIntegral(type_info.baseTypeID()); }

		return false;
	}


	auto TypeManager::isIntegral(BaseType::ID id) const -> bool {
		if(id.kind() != BaseType::Kind::Primitive){ return false; }

		const BaseType::Primitive& primitive = this->getPrimitive(id.primitiveID());

		switch(primitive.kind()){
			case Token::Kind::TypeInt:         return true;
			case Token::Kind::TypeISize:       return true;
			case Token::Kind::TypeI_N:         return true;
			case Token::Kind::TypeUInt:        return true;
			case Token::Kind::TypeUSize:       return true;
			case Token::Kind::TypeUI_N:        return true;
			case Token::Kind::TypeF16:         return false;
			case Token::Kind::TypeBF16:        return false;
			case Token::Kind::TypeF32:         return false;
			case Token::Kind::TypeF64:         return false;
			case Token::Kind::TypeF80:         return false;
			case Token::Kind::TypeF128:        return false;
			case Token::Kind::TypeByte:        return false;
			case Token::Kind::TypeBool:        return false;
			case Token::Kind::TypeChar:        return false;
			case Token::Kind::TypeRawPtr:      return false;
			case Token::Kind::TypeTypeID:      return false;
			case Token::Kind::TypeCShort:      return true;
			case Token::Kind::TypeCUShort:     return true;
			case Token::Kind::TypeCInt:        return true;
			case Token::Kind::TypeCUInt:       return true;
			case Token::Kind::TypeCLong:       return true;
			case Token::Kind::TypeCULong:      return true;
			case Token::Kind::TypeCLongLong:   return true;
			case Token::Kind::TypeCULongLong:  return true;
			case Token::Kind::TypeCLongDouble: return false;
			default: evo::debugFatalBreak("Not a type");
		}
	}


	///////////////////////////////////
	// isUnsignedIntegral

	auto TypeManager::isUnsignedIntegral(TypeInfo::VoidableID id) const -> bool {
		if(id.isVoid()){ return false; }
		return this->isUnsignedIntegral(id.typeID());
	}

	auto TypeManager::isUnsignedIntegral(TypeInfo::ID id) const -> bool {
		const TypeInfo& type_info = this->getTypeInfo(id);

		if(type_info.qualifiers().empty()){ return this->isUnsignedIntegral(type_info.baseTypeID()); }

		return false;
	}


	auto TypeManager::isUnsignedIntegral(BaseType::ID id) const -> bool {
		if(id.kind() != BaseType::Kind::Primitive){ return false; }

		const BaseType::Primitive& primitive = this->getPrimitive(id.primitiveID());

		switch(primitive.kind()){
			case Token::Kind::TypeInt:         return false;
			case Token::Kind::TypeISize:       return false;
			case Token::Kind::TypeI_N:         return false;
			case Token::Kind::TypeUInt:        return true;
			case Token::Kind::TypeUSize:       return true;
			case Token::Kind::TypeUI_N:        return true;
			case Token::Kind::TypeF16:         return false;
			case Token::Kind::TypeBF16:        return false;
			case Token::Kind::TypeF32:         return false;
			case Token::Kind::TypeF64:         return false;
			case Token::Kind::TypeF80:         return false;
			case Token::Kind::TypeF128:        return false;
			case Token::Kind::TypeByte:        return false;
			case Token::Kind::TypeBool:        return false;
			case Token::Kind::TypeChar:        return false;
			case Token::Kind::TypeRawPtr:      return false;
			case Token::Kind::TypeTypeID:      return false;
			case Token::Kind::TypeCShort:      return false;
			case Token::Kind::TypeCUShort:     return true;
			case Token::Kind::TypeCInt:        return false;
			case Token::Kind::TypeCUInt:       return true;
			case Token::Kind::TypeCLong:       return false;
			case Token::Kind::TypeCULong:      return true;
			case Token::Kind::TypeCLongLong:   return false;
			case Token::Kind::TypeCULongLong:  return true;
			case Token::Kind::TypeCLongDouble: return false;
			default: evo::debugFatalBreak("Not a type");
		}
	}


	///////////////////////////////////
	// isFloatingPoint

	auto TypeManager::isFloatingPoint(TypeInfo::VoidableID id) const -> bool {
		if(id.isVoid()){ return false; }
		return this->isFloatingPoint(id.typeID());
	}


	auto TypeManager::isFloatingPoint(TypeInfo::ID id) const -> bool {
		const TypeInfo& type_info = this->getTypeInfo(id);

		if(type_info.qualifiers().empty()){ return this->isFloatingPoint(type_info.baseTypeID()); }

		return false;
	}


	auto TypeManager::isFloatingPoint(BaseType::ID id) const -> bool {
		if(id.kind() != BaseType::Kind::Primitive){ return false; }

		const BaseType::Primitive& primitive = this->getPrimitive(id.primitiveID());
		switch(primitive.kind()){
			case Token::Kind::TypeInt:         return false;
			case Token::Kind::TypeISize:       return false;
			case Token::Kind::TypeI_N:         return false;
			case Token::Kind::TypeUInt:        return false;
			case Token::Kind::TypeUSize:       return false;
			case Token::Kind::TypeUI_N:        return false;
			case Token::Kind::TypeF16:         return true;
			case Token::Kind::TypeBF16:        return true;
			case Token::Kind::TypeF32:         return true;
			case Token::Kind::TypeF64:         return true;
			case Token::Kind::TypeF80:         return true;
			case Token::Kind::TypeF128:        return true;
			case Token::Kind::TypeByte:        return false;
			case Token::Kind::TypeBool:        return false;
			case Token::Kind::TypeChar:        return false;
			case Token::Kind::TypeRawPtr:      return false;
			case Token::Kind::TypeTypeID:      return false;
			case Token::Kind::TypeCShort:      return false;
			case Token::Kind::TypeCUShort:     return false;
			case Token::Kind::TypeCInt:        return false;
			case Token::Kind::TypeCUInt:       return false;
			case Token::Kind::TypeCLong:       return false;
			case Token::Kind::TypeCULong:      return false;
			case Token::Kind::TypeCLongLong:   return false;
			case Token::Kind::TypeCULongLong:  return false;
			case Token::Kind::TypeCLongDouble: return true;
			default: evo::debugFatalBreak("Not a type");
		}
	}


	///////////////////////////////////
	// isBuiltin

	auto TypeManager::isBuiltin(TypeInfo::VoidableID id) const -> bool {
		if(id.isVoid()){ return true; }
		return this->isBuiltin(id.typeID());
	}

	auto TypeManager::isBuiltin(TypeInfo::ID id) const -> bool {
		const TypeInfo& type_info = this->getTypeInfo(id);

		for(auto iter = type_info.qualifiers().rbegin(); iter != type_info.qualifiers().rend(); ++iter){
			if(iter->isPtr){ return true; }
		}

		return this->isBuiltin(type_info.baseTypeID());
	}

	auto TypeManager::isBuiltin(BaseType::ID id) const -> bool {
		switch(id.kind()){
			case BaseType::Kind::Primitive: return true;
			case BaseType::Kind::Function:  return false;
			case BaseType::Kind::Alias:     return this->isBuiltin(this->getAlias(id.aliasID()).aliasedType);

			case BaseType::Kind::Dummy: evo::debugFatalBreak("Dummy type should not be used");
		}

		evo::debugFatalBreak("Unknown BaseType::Kind");
	}


	///////////////////////////////////
	// getUnderlyingType

	auto TypeManager::getUnderlyingType(TypeInfo::ID id) -> evo::Result<TypeInfo::ID> {
		const TypeInfo& type_info = this->getTypeInfo(id);

		if(type_info.qualifiers().empty()){ return this->getUnderlyingType(type_info.baseTypeID()); }
		
		if(type_info.qualifiers().back().isPtr){ return TypeManager::getTypeRawPtr(); }

		return evo::resultError;
	}

	// TODO: optimize this function
	auto TypeManager::getUnderlyingType(BaseType::ID id) -> evo::Result<TypeInfo::ID> {
		switch(id.kind()){
			case BaseType::Kind::Dummy: evo::debugFatalBreak("Dummy type should not be used");
			case BaseType::Kind::Primitive: break;
			case BaseType::Kind::Function: evo::resultError;
			case BaseType::Kind::Alias: {
				const BaseType::Alias& alias = this->getAlias(id.aliasID());
				if(alias.aliasedType.isVoid()){ return evo::resultError; }
				return this->getUnderlyingType(alias.aliasedType.typeID());
			} break;
		}

		const BaseType::Primitive& primitive = this->getPrimitive(id.primitiveID());
		switch(primitive.kind()){
			case Token::Kind::TypeInt:{
				return this->getOrCreateTypeInfo(
					TypeInfo(
						this->getOrCreatePrimitiveBaseType(
							Token::Kind::TypeI_N, uint32_t(this->sizeOfGeneralRegister())
						)
					)
				);
			} break;

			case Token::Kind::TypeISize:{
				return this->getOrCreateTypeInfo(
					TypeInfo(this->getOrCreatePrimitiveBaseType(Token::Kind::TypeI_N, uint32_t(this->sizeOfPtr())))
				);
			} break;

			case Token::Kind::TypeI_N: {
				return this->getOrCreateTypeInfo(TypeInfo(id));
			} break;

			case Token::Kind::TypeUInt: {
				return this->getOrCreateTypeInfo(
					TypeInfo(
						this->getOrCreatePrimitiveBaseType(
							Token::Kind::TypeUI_N, uint32_t(this->sizeOfGeneralRegister())
						)
					)
				);
			} break;

			case Token::Kind::TypeUSize: {
				return this->getOrCreateTypeInfo(
					TypeInfo(this->getOrCreatePrimitiveBaseType(Token::Kind::TypeUI_N, uint32_t(this->sizeOfPtr())))
				);
			} break;

			case Token::Kind::TypeUI_N: {
				return this->getOrCreateTypeInfo(TypeInfo(id));
			} break;

			case Token::Kind::TypeF16: {
				return this->getOrCreateTypeInfo(TypeInfo(id));
			} break;

			case Token::Kind::TypeBF16: {
				return this->getOrCreateTypeInfo(TypeInfo(id));
			} break;

			case Token::Kind::TypeF32: {
				return this->getOrCreateTypeInfo(TypeInfo(id));
			} break;

			case Token::Kind::TypeF64: {
				return this->getOrCreateTypeInfo(TypeInfo(id));
			} break;

			case Token::Kind::TypeF80: {
				return this->getOrCreateTypeInfo(TypeInfo(id));
			} break;

			case Token::Kind::TypeF128: {
				return this->getOrCreateTypeInfo(TypeInfo(id));
			} break;

			case Token::Kind::TypeByte: {
				return this->getOrCreateTypeInfo(
					TypeInfo(this->getOrCreatePrimitiveBaseType(Token::Kind::TypeUI_N, 8))
				);
			} break;

			case Token::Kind::TypeBool: {
				return this->getOrCreateTypeInfo(
					TypeInfo(this->getOrCreatePrimitiveBaseType(Token::Kind::TypeUI_N, 1))
				);
			} break;

			case Token::Kind::TypeChar: {
				return this->getOrCreateTypeInfo(
					TypeInfo(this->getOrCreatePrimitiveBaseType(Token::Kind::TypeUI_N, 8))
				);
			} break;

			case Token::Kind::TypeRawPtr: {
				return TypeManager::getTypeRawPtr();
			} break;

			case Token::Kind::TypeTypeID: {
				return this->getOrCreateTypeInfo(
					TypeInfo(this->getOrCreatePrimitiveBaseType(Token::Kind::TypeUI_N, 32))
				);
			} break;

			case Token::Kind::TypeCShort: case Token::Kind::TypeCInt: case Token::Kind::TypeCLong:
			case Token::Kind::TypeCLongLong: {
				return this->getOrCreateTypeInfo(
					TypeInfo(this->getOrCreatePrimitiveBaseType(Token::Kind::TypeI_N, uint32_t(this->sizeOf(id))))
				);
			} break;

			case Token::Kind::TypeCUShort: case Token::Kind::TypeCUInt: case Token::Kind::TypeCULong:
			case Token::Kind::TypeCULongLong: {
				return this->getOrCreateTypeInfo(
					TypeInfo(this->getOrCreatePrimitiveBaseType(Token::Kind::TypeUI_N, uint32_t(this->sizeOf(id))))
				);
			} break;

			case Token::Kind::TypeCLongDouble: {
				if(this->platform() == core::Platform::Windows){
					return this->getOrCreateTypeInfo(
						TypeInfo(this->getOrCreatePrimitiveBaseType(Token::Kind::TypeF64))
					);
				}else{
					return this->getOrCreateTypeInfo(
						TypeInfo(this->getOrCreatePrimitiveBaseType(Token::Kind::TypeF128))
					);
				}
			} break;

			default: evo::debugFatalBreak("Not a type");
		}
	}


}