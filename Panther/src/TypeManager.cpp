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

#include "../include/source/SourceManager.h"

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
		return this->types[id];
	}

	auto TypeManager::getOrCreateTypeInfo(TypeInfo&& lookup_type_info) -> TypeInfo::ID {
		for(uint32_t i = 0; i < this->types.size(); i+=1){
			if(this->types[TypeInfo::ID(i)] == lookup_type_info){
				return TypeInfo::ID(i);
			}
		}

		return this->types.emplace_back(std::move(lookup_type_info));
	}


	auto TypeManager::printType(TypeInfo::VoidableID type_info_id, const SourceManager& source_manager) const 
	-> std::string {
		if(type_info_id.isVoid()) [[unlikely]] {
			return "Void";
		}else{
			return this->printType(type_info_id.asTypeID(), source_manager);
		}
	}

	auto TypeManager::printType(TypeInfo::ID type_info_id, const SourceManager& source_manager) const -> std::string {
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

				case BaseType::Kind::Array: {
					// TODO: fix this
					return "{ARRAY}";
				} break;

				case BaseType::Kind::Alias: {
					const BaseType::Alias::ID alias_id = type_info.baseTypeID().aliasID();
					const BaseType::Alias& alias = this->getAlias(alias_id);
					return this->printType(alias.aliasedType, source_manager);
				} break;

				case BaseType::Kind::Typedef: {
					const BaseType::Typedef::ID typedef_id = type_info.baseTypeID().typedefID();
					const BaseType::Typedef& typedef_info = this->getTypedef(typedef_id);

					return std::string(
						source_manager[typedef_info.sourceID].getTokenBuffer()[typedef_info.identTokenID].getString()
					);
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
		return this->functions[id];
	}

	auto TypeManager::getOrCreateFunction(BaseType::Function&& lookup_func) -> BaseType::ID {
		for(uint32_t i = 0; i < this->functions.size(); i+=1){
			if(this->functions[BaseType::Function::ID(i)] == lookup_func){
				return BaseType::ID(BaseType::Kind::Function, i);
			}
		}

		const BaseType::Function::ID new_function = this->functions.emplace_back(lookup_func);
		return BaseType::ID(BaseType::Kind::Function, new_function.get());
	}


	//////////////////////////////////////////////////////////////////////
	// array

	auto TypeManager::getArray(BaseType::Array::ID id) const -> const BaseType::Array& {
		return this->arrays[id];
	}

	auto TypeManager::getOrCreateArray(BaseType::Array&& lookup_func) -> BaseType::ID {
		for(uint32_t i = 0; i < this->arrays.size(); i+=1){
			if(this->arrays[BaseType::Array::ID(i)] == lookup_func){
				return BaseType::ID(BaseType::Kind::Array, i);
			}
		}

		const BaseType::Array::ID new_array = this->arrays.emplace_back(lookup_func);
		return BaseType::ID(BaseType::Kind::Array, new_array.get());
	}


	//////////////////////////////////////////////////////////////////////
	// primitive

	auto TypeManager::getPrimitive(BaseType::Primitive::ID id) const -> const BaseType::Primitive& {
		return this->primitives[id];
	}

	auto TypeManager::getOrCreatePrimitiveBaseType(Token::Kind kind) -> BaseType::ID {
		return this->get_or_create_primitive_base_type_impl(BaseType::Primitive(kind));
	}

	auto TypeManager::getOrCreatePrimitiveBaseType(Token::Kind kind, uint32_t bit_width) -> BaseType::ID {
		return this->get_or_create_primitive_base_type_impl(BaseType::Primitive(kind, bit_width));
	}

	auto TypeManager::get_or_create_primitive_base_type_impl(const BaseType::Primitive& lookup_type) -> BaseType::ID {
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
		return this->aliases[id];
	}


	auto TypeManager::getOrCreateAlias(BaseType::Alias&& lookup_type) -> BaseType::ID {
		for(uint32_t i = 0; i < this->aliases.size(); i+=1){
			if(this->aliases[BaseType::Alias::ID(i)] == lookup_type){
				return BaseType::ID(BaseType::Kind::Alias, i);
			}
		}

		const BaseType::Alias::ID new_alias = this->aliases.emplace_back(lookup_type);
		return BaseType::ID(BaseType::Kind::Alias, new_alias.get());
	}


	//////////////////////////////////////////////////////////////////////
	// typedefs

	auto TypeManager::getTypedef(BaseType::Typedef::ID id) const -> const BaseType::Typedef& {
		return this->typedefs[id];
	}


	auto TypeManager::getOrCreateTypedef(BaseType::Typedef&& lookup_type) -> BaseType::ID {
		for(uint32_t i = 0; i < this->typedefs.size(); i+=1){
			if(this->typedefs[BaseType::Typedef::ID(i)] == lookup_type){
				return BaseType::ID(BaseType::Kind::Typedef, i);
			}
		}

		const BaseType::Typedef::ID new_typedef = this->typedefs.emplace_back(lookup_type);
		return BaseType::ID(BaseType::Kind::Typedef, new_typedef.get());
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


	auto TypeManager::sizeOf(BaseType::ID id) const -> uint64_t {
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
						return 4;

					case Token::Kind::TypeCLong: case Token::Kind::TypeCULong:
						return this->getOS() == core::OS::Windows ? 4 : 8;

					case Token::Kind::TypeCLongLong: case Token::Kind::TypeCULongLong:
						return 8;

					case Token::Kind::TypeCLongDouble: return this->getOS() == core::OS::Windows ? 8 : 16;

					default: evo::debugFatalBreak("Unknown or unsupported built-in type");
				}
			} break;

			case BaseType::Kind::Function: {
				return this->sizeOfPtr();
			} break;

			case BaseType::Kind::Array: {
				const BaseType::Array& array = this->getArray(id.arrayID());
				const uint64_t elem_size = this->sizeOf(array.elementTypeID);

				if(array.terminator.has_value()){ return elem_size * array.lengths.back() + 1; }

				uint64_t output = elem_size;
				for(uint64_t length : array.lengths){
					output *= length;
				}
				return output;
			} break;

			case BaseType::Kind::Alias: {
				const BaseType::Alias& alias = this->getAlias(id.aliasID());
				evo::debugAssert(alias.aliasedType.isVoid() == false, "cannot get sizeof type `Void`");
				return this->sizeOf(alias.aliasedType.asTypeID());
			} break;

			case BaseType::Kind::Typedef: {
				const BaseType::Typedef& alias = this->getTypedef(id.typedefID());
				return this->sizeOf(alias.underlyingType);
			} break;

			case BaseType::Kind::Dummy: evo::debugFatalBreak("Dummy type should not be used");
		}

		evo::debugFatalBreak("Unknown or unsupported base-type kind");
	}

	auto TypeManager::sizeOfPtr() const -> uint64_t { return 8; }
	auto TypeManager::sizeOfGeneralRegister() const -> uint64_t { return 8; }


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
		return this->isPrimitive(id.asTypeID());
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
		return this->isIntegral(id.asTypeID());
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
		return this->isUnsignedIntegral(id.asTypeID());
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
		return this->isFloatingPoint(id.asTypeID());
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

	// auto TypeManager::isBuiltin(TypeInfo::VoidableID id) const -> bool {
	// 	if(id.isVoid()){ return true; }
	// 	return this->isBuiltin(id.asTypeID());
	// }

	// auto TypeManager::isBuiltin(TypeInfo::ID id) const -> bool {
	// 	const TypeInfo& type_info = this->getTypeInfo(id);

	// 	for(auto iter = type_info.qualifiers().rbegin(); iter != type_info.qualifiers().rend(); ++iter){
	// 		if(iter->isPtr){ return true; }
	// 	}

	// 	return this->isBuiltin(type_info.baseTypeID());
	// }

	// auto TypeManager::isBuiltin(BaseType::ID id) const -> bool {
	// 	switch(id.kind()){
	// 		case BaseType::Kind::Primitive: return true;
	// 		case BaseType::Kind::Function:  return false;
	// 		case BaseType::Kind::Array:     
	// 		case BaseType::Kind::Alias:     return this->isBuiltin(this->getAlias(id.aliasID()).aliasedType);
	// 		case BaseType::Kind::Typedef:   return false;

	// 		case BaseType::Kind::Dummy: evo::debugFatalBreak("Dummy type should not be used");
	// 	}

	// 	evo::debugFatalBreak("Unknown BaseType::Kind");
	// }


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
			case BaseType::Kind::Array: evo::resultError;
			case BaseType::Kind::Alias: {
				const BaseType::Alias& alias = this->getAlias(id.aliasID());
				if(alias.aliasedType.isVoid()){ return evo::resultError; }
				return this->getUnderlyingType(alias.aliasedType.asTypeID());
			} break;
			case BaseType::Kind::Typedef: {
				const BaseType::Typedef& typedef_info = this->getTypedef(id.typedefID());
				return this->getUnderlyingType(typedef_info.underlyingType);
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

			case Token::Kind::TypeCShort: case Token::Kind::TypeCInt: {
				return this->getOrCreateTypeInfo(
					TypeInfo(this->getOrCreatePrimitiveBaseType(Token::Kind::TypeI_N, 32))
				);
			} break;

			case Token::Kind::TypeCLong: case Token::Kind::TypeCLongLong: {
				const uint32_t size = this->_os == core::OS::Windows ? 32 : 64;
				return this->getOrCreateTypeInfo(
					TypeInfo(this->getOrCreatePrimitiveBaseType(Token::Kind::TypeI_N, size))
				);
			} break;

			case Token::Kind::TypeCUShort: case Token::Kind::TypeCUInt: {
				return this->getOrCreateTypeInfo(
					TypeInfo(this->getOrCreatePrimitiveBaseType(Token::Kind::TypeUI_N, 32))
				);
			} break;

			case Token::Kind::TypeCULong: case Token::Kind::TypeCULongLong: {
				const uint32_t size = this->_os == core::OS::Windows ? 32 : 64;
				return this->getOrCreateTypeInfo(
					TypeInfo(this->getOrCreatePrimitiveBaseType(Token::Kind::TypeUI_N, size))
				);
			} break;

			case Token::Kind::TypeCLongDouble: {
				if(this->getOS() == core::OS::Windows){
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



	///////////////////////////////////
	// min / max

	static auto calc_min_signed(size_t width) -> core::GenericInt {
		return core::GenericInt(unsigned(width), 1, false).ushl(core::GenericInt::create(unsigned(width) - 1)).result;
	}

	static auto calc_max_signed(size_t width) -> core::GenericInt {
		return calc_min_signed(unsigned(width)).usub(core::GenericInt(unsigned(width), 1)).result;
	}

	static auto calc_max_unsigned(size_t width) -> core::GenericInt {
		return core::GenericInt(unsigned(width), 0, false).usub(core::GenericInt(unsigned(width), 1, false)).result;
	}

	static auto float_data_from_exponent(unsigned width, int exponent, unsigned precision) -> core::GenericInt {
		return core::GenericInt(width, unsigned(exponent), false).ushl(core::GenericInt(width, precision)).result;
	}



	auto TypeManager::getMin(TypeInfo::ID id) const -> core::GenericValue {
		const TypeInfo& type_info = this->getTypeInfo(id);
		evo::debugAssert(type_info.qualifiers().empty(), "Can only get min of primitive");

		return this->getMin(type_info.baseTypeID());
	}

	auto TypeManager::getMin(BaseType::ID id) const -> core::GenericValue {
		const BaseType::Primitive& primitive = this->getPrimitive(id.primitiveID());
		switch(primitive.kind()){
			case Token::Kind::TypeInt:   return core::GenericValue(calc_min_signed(this->sizeOfGeneralRegister() * 8));
			case Token::Kind::TypeISize: return core::GenericValue(calc_min_signed(this->sizeOfPtr() * 8));
			case Token::Kind::TypeI_N:   return core::GenericValue(calc_min_signed(primitive.bitWidth()));

			case Token::Kind::TypeUInt:
				return core::GenericValue(core::GenericInt(unsigned(this->sizeOfGeneralRegister() * 8), 0));

			case Token::Kind::TypeUSize: 
				return core::GenericValue(core::GenericInt(unsigned(this->sizeOfPtr() * 8), 0));

			case Token::Kind::TypeUI_N:  return core::GenericValue(core::GenericInt(primitive.bitWidth(), 0));

			case Token::Kind::TypeF16:
				return core::GenericValue(core::GenericFloat::createF16(float_data_from_exponent(16, 15, 11)).neg());

			case Token::Kind::TypeBF16:
				return core::GenericValue(core::GenericFloat::createBF16(float_data_from_exponent(16, 127, 8)).neg());

			case Token::Kind::TypeF32:
				return core::GenericValue(core::GenericFloat::createF32(float_data_from_exponent(32, 127, 24)).neg());

			case Token::Kind::TypeF64:
				return core::GenericValue(core::GenericFloat::createF64(float_data_from_exponent(64, 1023, 53)).neg());

			case Token::Kind::TypeF80:
				return core::GenericValue(core::GenericFloat::createF80(float_data_from_exponent(80, 16383, 64)).neg());

			case Token::Kind::TypeF128:
				return core::GenericValue(
					core::GenericFloat::createF128(float_data_from_exponent(128, 16383, 113)).neg()
				);


			case Token::Kind::TypeByte:   return core::GenericValue(core::GenericInt(8, 0));
			case Token::Kind::TypeBool:   return core::GenericValue(false);
			case Token::Kind::TypeChar:   return core::GenericValue(calc_min_signed(8));

			case Token::Kind::TypeRawPtr:
				return core::GenericValue(core::GenericInt(unsigned(this->sizeOfPtr() * 8), 0));

			case Token::Kind::TypeTypeID: return core::GenericValue(core::GenericInt(32, 0));
			case Token::Kind::TypeCShort: return core::GenericValue(calc_min_signed(16));
			case Token::Kind::TypeCInt:   return core::GenericValue(calc_min_signed(32));
				
			case Token::Kind::TypeCLong:
				return core::GenericValue(calc_min_signed(this->_os == core::OS::Windows ? 32 : 64));

			case Token::Kind::TypeCLongLong: return core::GenericValue(calc_min_signed(32));
			case Token::Kind::TypeCUShort:   return core::GenericValue(core::GenericInt(16, 0));
			case Token::Kind::TypeCUInt:     return core::GenericValue(core::GenericInt(32, 0));

			case Token::Kind::TypeCULong:     
				 return core::GenericValue(core::GenericInt(this->_os == core::OS::Windows ? 32 : 64, 0));

			case Token::Kind::TypeCULongLong: return core::GenericValue(core::GenericInt(64, 0));

			case Token::Kind::TypeCLongDouble: {
				if(this->_os == core::OS::Windows){
					return core::GenericValue(
						core::GenericFloat::createF64(float_data_from_exponent(64, 1023, 53)).neg()
					);
				}else{
					return core::GenericValue(
						core::GenericFloat::createF128(float_data_from_exponent(128, 16383, 113)).neg()
					);
				}
			} break;

			default: evo::debugFatalBreak("Not a type");
		}
	}



	auto TypeManager::getNormalizedMin(TypeInfo::ID id) const -> core::GenericValue {
		const TypeInfo& type_info = this->getTypeInfo(id);
		evo::debugAssert(type_info.qualifiers().empty(), "Can only get min of primitive");

		return this->getNormalizedMin(type_info.baseTypeID());
	}

	auto TypeManager::getNormalizedMin(BaseType::ID id) const -> core::GenericValue {
		const BaseType::Primitive& primitive = this->getPrimitive(id.primitiveID());
		switch(primitive.kind()){
			case Token::Kind::TypeInt:   return core::GenericValue(calc_min_signed(this->sizeOfGeneralRegister() * 8));
			case Token::Kind::TypeISize: return core::GenericValue(calc_min_signed(this->sizeOfPtr() * 8));
			case Token::Kind::TypeI_N:   return core::GenericValue(calc_min_signed(primitive.bitWidth()));

			case Token::Kind::TypeUInt:
				return core::GenericValue(core::GenericInt(unsigned(this->sizeOfGeneralRegister() * 8), 0));

			case Token::Kind::TypeUSize:
				return core::GenericValue(core::GenericInt(unsigned(this->sizeOfPtr() * 8), 0));
				
			case Token::Kind::TypeUI_N: return core::GenericValue(core::GenericInt(primitive.bitWidth(), 0));

			case Token::Kind::TypeF16:
				return core::GenericValue(core::GenericFloat::createF16(float_data_from_exponent(16, 1, 11)));

			case Token::Kind::TypeBF16:
				return core::GenericValue(core::GenericFloat::createBF16(float_data_from_exponent(16, 1, 8)));

			case Token::Kind::TypeF32:
				return core::GenericValue(core::GenericFloat::createF32(float_data_from_exponent(32, 1, 24)));

			case Token::Kind::TypeF64:
				return core::GenericValue(core::GenericFloat::createF64(float_data_from_exponent(64, 1, 53)));

			case Token::Kind::TypeF80:
				return core::GenericValue(core::GenericFloat::createF80(float_data_from_exponent(80, 1, 64)));

			case Token::Kind::TypeF128:
				return core::GenericValue(core::GenericFloat::createF128(float_data_from_exponent(128, 1, 113)));


			case Token::Kind::TypeByte:   return core::GenericValue(core::GenericInt(8, 0));
			case Token::Kind::TypeBool:   return core::GenericValue(false);
			case Token::Kind::TypeChar:   return core::GenericValue(calc_min_signed(8));

			case Token::Kind::TypeRawPtr:
				return core::GenericValue(core::GenericInt(unsigned(this->sizeOfPtr() * 8), 0));

			case Token::Kind::TypeTypeID: return core::GenericValue(core::GenericInt(32, 0));
			case Token::Kind::TypeCShort: return core::GenericValue(calc_min_signed(16));
			case Token::Kind::TypeCInt:   return core::GenericValue(calc_min_signed(32));
				
			case Token::Kind::TypeCLong:
				return core::GenericValue(calc_min_signed(this->_os == core::OS::Windows ? 32 : 64));

			case Token::Kind::TypeCLongLong: return core::GenericValue(calc_min_signed(32));
			case Token::Kind::TypeCUShort:   return core::GenericValue(core::GenericInt(16, 0));
			case Token::Kind::TypeCUInt:     return core::GenericValue(core::GenericInt(32, 0));

			case Token::Kind::TypeCULong:     
				 return core::GenericValue(core::GenericInt(this->_os == core::OS::Windows ? 32 : 64, 0));

			case Token::Kind::TypeCULongLong: return core::GenericValue(core::GenericInt(64, 0));

			case Token::Kind::TypeCLongDouble: {
				if(this->_os == core::OS::Windows){
					return core::GenericValue(core::GenericFloat::createF64(float_data_from_exponent(64, 1, 53)));
				}else{
					return core::GenericValue(core::GenericFloat::createF128(float_data_from_exponent(128, 1, 113)));
				}
			} break;

			default: evo::debugFatalBreak("Not a type");
		}
	}



	auto TypeManager::getMax(TypeInfo::ID id) const -> core::GenericValue {
		const TypeInfo& type_info = this->getTypeInfo(id);
		evo::debugAssert(type_info.qualifiers().empty(), "Can only get max of primitive");

		return this->getMax(type_info.baseTypeID());
	}

	auto TypeManager::getMax(BaseType::ID id) const -> core::GenericValue {
		const BaseType::Primitive& primitive = this->getPrimitive(id.primitiveID());
		switch(primitive.kind()){
			case Token::Kind::TypeInt:   return core::GenericValue(calc_max_signed(this->sizeOfGeneralRegister() * 8));
			case Token::Kind::TypeISize: return core::GenericValue(calc_max_signed(this->sizeOfPtr() * 8));
			case Token::Kind::TypeI_N:   return core::GenericValue(calc_max_signed(primitive.bitWidth()));

			case Token::Kind::TypeUInt:
				return core::GenericValue(calc_max_unsigned(this->sizeOfGeneralRegister() * 8));

			case Token::Kind::TypeUSize: return core::GenericValue(calc_max_unsigned(this->sizeOfPtr() * 8));
			case Token::Kind::TypeUI_N:  return core::GenericValue(calc_max_unsigned(primitive.bitWidth()));

			case Token::Kind::TypeF16:
				return core::GenericValue(core::GenericFloat::createF16(float_data_from_exponent(16, 15, 11)));

			case Token::Kind::TypeBF16:
				return core::GenericValue(core::GenericFloat::createBF16(float_data_from_exponent(16, 127, 8)));

			case Token::Kind::TypeF32:
				return core::GenericValue(core::GenericFloat::createF32(float_data_from_exponent(32, 127, 24)));

			case Token::Kind::TypeF64:
				return core::GenericValue(core::GenericFloat::createF64(float_data_from_exponent(64, 1023, 53)));

			case Token::Kind::TypeF80:
				return core::GenericValue(core::GenericFloat::createF80(float_data_from_exponent(80, 16383, 64)));

			case Token::Kind::TypeF128:
				return core::GenericValue(core::GenericFloat::createF128(float_data_from_exponent(128, 16388, 113)));


			case Token::Kind::TypeByte:   return core::GenericValue(calc_max_unsigned(8));
			case Token::Kind::TypeBool:   return core::GenericValue(true);
			case Token::Kind::TypeChar:   return core::GenericValue(calc_max_signed(8));
			case Token::Kind::TypeRawPtr: return core::GenericValue(calc_max_unsigned(this->sizeOfPtr() * 8));
			case Token::Kind::TypeTypeID: return core::GenericValue(calc_max_unsigned(32));
			case Token::Kind::TypeCShort: return core::GenericValue(calc_max_signed(16));
			case Token::Kind::TypeCInt:   return core::GenericValue(calc_max_signed(32));

			case Token::Kind::TypeCLong:      
				return core::GenericValue(calc_max_signed(this->_os == core::OS::Windows ? 32 : 64));

			case Token::Kind::TypeCLongLong:  return core::GenericValue(calc_max_signed(32));
			case Token::Kind::TypeCUShort:    return core::GenericValue(calc_max_unsigned(16));
			case Token::Kind::TypeCUInt:      return core::GenericValue(calc_max_unsigned(32));

			case Token::Kind::TypeCULong:     
				return core::GenericValue(calc_max_unsigned(this->_os == core::OS::Windows ? 32 : 64));
				
			case Token::Kind::TypeCULongLong: return core::GenericValue(calc_max_unsigned(64));

			case Token::Kind::TypeCLongDouble: {
				if(this->_os == core::OS::Windows){
					return core::GenericValue(core::GenericFloat::createF64(float_data_from_exponent(64, 1023, 53)));
				}else{
					return core::GenericValue(
						core::GenericFloat::createF128(float_data_from_exponent(128, 16383, 113))
					);
				}
			} break;

			default: evo::debugFatalBreak("Not a type");
		}
	}


}