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

	//////////////////////////////////////////////////////////////////////
	// base type


	auto BaseType::StructTemplate::lookupInstantiation(evo::SmallVector<Arg>&& args) -> InstantiationInfo {
		const auto lock = std::scoped_lock(this->instantiation_lock);

		auto find = this->instantiation_map.find(args);
		if(find == this->instantiation_map.end()){
			const uint32_t instantiation_id = uint32_t(this->instantiations.size());
			Instantiation& new_instantiation = this->instantiations[this->instantiations.emplace_back()];
			this->instantiation_map.emplace(std::move(args), new_instantiation);
			return InstantiationInfo(new_instantiation, instantiation_id);

		}else{
			return InstantiationInfo(find->second, std::nullopt);
		}
	}



	auto BaseType::StructTemplate::getInstantiationArgs(uint32_t instantiation_id) const -> evo::SmallVector<Arg> {
		const auto lock = std::scoped_lock(this->instantiation_lock);

		const Instantiation* instantiation_ptr = &this->instantiations[instantiation_id];

		for(const auto& instantiation_map_item : this->instantiation_map){
			if(&instantiation_map_item.second == instantiation_ptr){
				return instantiation_map_item.first;
			}
		}

		evo::debugFatalBreak("Didn't find instantiation: {}", instantiation_id);
	}




	//////////////////////////////////////////////////////////////////////
	// type manager


	auto TypeManager::initPrimitives() -> void {
		evo::debugAssert(this->primitivesInitialized() == false, "primitives already initialized");

		this->primitives.emplace_back(Token::Kind::TYPE_INT);
		this->primitives.emplace_back(Token::Kind::TYPE_ISIZE);
		this->primitives.emplace_back(Token::Kind::TYPE_UINT);
		const BaseType::Primitive::ID type_usize = this->primitives.emplace_back(Token::Kind::TYPE_USIZE);
		this->primitives.emplace_back(Token::Kind::TYPE_F16);
		this->primitives.emplace_back(Token::Kind::TYPE_F32);
		this->primitives.emplace_back(Token::Kind::TYPE_F64);
		this->primitives.emplace_back(Token::Kind::TYPE_F80);
		const BaseType::Primitive::ID type_f128  =  this->primitives.emplace_back(Token::Kind::TYPE_F128);
		this->primitives.emplace_back(Token::Kind::TYPE_BYTE);
		const BaseType::Primitive::ID type_bool    = this->primitives.emplace_back(Token::Kind::TYPE_BOOL);
		const BaseType::Primitive::ID type_char    = this->primitives.emplace_back(Token::Kind::TYPE_CHAR);
		const BaseType::Primitive::ID type_raw_ptr = this->primitives.emplace_back(Token::Kind::TYPE_RAWPTR);
		const BaseType::Primitive::ID type_type_id = this->primitives.emplace_back(Token::Kind::TYPE_TYPEID);

		this->primitives.emplace_back(Token::Kind::TYPE_C_SHORT);
		this->primitives.emplace_back(Token::Kind::TYPE_C_USHORT);
		this->primitives.emplace_back(Token::Kind::TYPE_C_INT);
		this->primitives.emplace_back(Token::Kind::TYPE_C_UINT);
		this->primitives.emplace_back(Token::Kind::TYPE_C_LONG);
		this->primitives.emplace_back(Token::Kind::TYPE_C_ULONG);
		this->primitives.emplace_back(Token::Kind::TYPE_C_LONG_LONG);
		this->primitives.emplace_back(Token::Kind::TYPE_C_ULONG_LONG);
		this->primitives.emplace_back(Token::Kind::TYPE_C_LONG_DOUBLE);

		this->primitives.emplace_back(Token::Kind::TYPE_I_N, 8);
		this->primitives.emplace_back(Token::Kind::TYPE_I_N, 16);
		this->primitives.emplace_back(Token::Kind::TYPE_I_N, 32);
		this->primitives.emplace_back(Token::Kind::TYPE_I_N, 64);

		const BaseType::Primitive::ID type_ui8  = this->primitives.emplace_back(Token::Kind::TYPE_UI_N, 8);
		const BaseType::Primitive::ID type_ui16 = this->primitives.emplace_back(Token::Kind::TYPE_UI_N, 16);
		const BaseType::Primitive::ID type_ui32 = this->primitives.emplace_back(Token::Kind::TYPE_UI_N, 32);
		const BaseType::Primitive::ID type_ui64 = this->primitives.emplace_back(Token::Kind::TYPE_UI_N, 64);

		const BaseType::Primitive::ID type_i256 = this->primitives.emplace_back(Token::Kind::TYPE_I_N, 256);


		this->types.emplace_back(TypeInfo(BaseType::ID(BaseType::Kind::PRIMITIVE, type_bool.get())));
		this->types.emplace_back(TypeInfo(BaseType::ID(BaseType::Kind::PRIMITIVE, type_char.get())));
		this->types.emplace_back(TypeInfo(BaseType::ID(BaseType::Kind::PRIMITIVE, type_ui8.get())));
		this->types.emplace_back(TypeInfo(BaseType::ID(BaseType::Kind::PRIMITIVE, type_ui16.get())));
		this->types.emplace_back(TypeInfo(BaseType::ID(BaseType::Kind::PRIMITIVE, type_ui32.get())));
		this->types.emplace_back(TypeInfo(BaseType::ID(BaseType::Kind::PRIMITIVE, type_ui64.get())));
		this->types.emplace_back(TypeInfo(BaseType::ID(BaseType::Kind::PRIMITIVE, type_usize.get())));
		this->types.emplace_back(TypeInfo(BaseType::ID(BaseType::Kind::PRIMITIVE, type_type_id.get())));
		this->types.emplace_back(TypeInfo(BaseType::ID(BaseType::Kind::PRIMITIVE, type_raw_ptr.get())));
		this->types.emplace_back(TypeInfo(BaseType::ID(BaseType::Kind::PRIMITIVE, type_i256.get())));
		this->types.emplace_back(TypeInfo(BaseType::ID(BaseType::Kind::PRIMITIVE, type_f128.get())));
	}

	auto TypeManager::primitivesInitialized() const -> bool {
		const auto lock = std::scoped_lock(this->primitives_lock);
		return !this->primitives.empty();
	}


	//////////////////////////////////////////////////////////////////////
	// type

	auto TypeManager::getTypeInfo(TypeInfo::ID id) const -> const TypeInfo& {
		const auto lock = std::scoped_lock(this->types_lock);
		evo::debugAssert(id.get() != std::numeric_limits<uint32_t>::max(), "Attempted to lookup the Void TypeInfo");
		evo::debugAssert(
			id.isTemplateDeclInstantiation() == false, "Attempted to lookup the TemplateDeclInstantiation TypeInfo"
		);
		return this->types[id];
	}

	auto TypeManager::getOrCreateTypeInfo(TypeInfo&& lookup_type_info) -> TypeInfo::ID {
		const auto lock = std::scoped_lock(this->types_lock);

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
				case BaseType::Kind::PRIMITIVE: {
					const BaseType::Primitive::ID primitive_id = type_info.baseTypeID().primitiveID();
					const BaseType::Primitive& primitive = this->getPrimitive(primitive_id);

					if(primitive.kind() == Token::Kind::TYPE_I_N){
						return std::format("I{}", primitive.bitWidth());

					}else if(primitive.kind() == Token::Kind::TYPE_UI_N){
						return std::format("UI{}", primitive.bitWidth());

					}else{
						return std::string(Token::printKind(primitive.kind()));
					}
				} break;

				case BaseType::Kind::FUNCTION: {
					// TODO(FEATURE): fix this
					return "{FUNCTION}";
				} break;

				case BaseType::Kind::ARRAY: {
					// TODO(FEATURE): fix this
					return "{ARRAY}";
				} break;

				case BaseType::Kind::ALIAS: {
					const BaseType::Alias::ID alias_id = type_info.baseTypeID().aliasID();
					const BaseType::Alias& alias = this->getAlias(alias_id);

					return std::string(source_manager[alias.sourceID].getTokenBuffer()[alias.identTokenID].getString());
				} break;

				case BaseType::Kind::TYPEDEF: {
					const BaseType::Typedef::ID typedef_id = type_info.baseTypeID().typedefID();
					const BaseType::Typedef& typedef_info = this->getTypedef(typedef_id);

					return std::string(
						source_manager[typedef_info.sourceID].getTokenBuffer()[typedef_info.identTokenID].getString()
					);
				} break;

				case BaseType::Kind::STRUCT: {
					const BaseType::Struct::ID struct_id = type_info.baseTypeID().structID();
					const BaseType::Struct& struct_info = this->getStruct(struct_id);

					const std::string_view struct_name =
						source_manager[struct_info.sourceID].getTokenBuffer()[struct_info.identTokenID].getString();

					if(struct_info.templateID.has_value() == false){
						return std::string(struct_name);
					}

					const BaseType::StructTemplate& struct_template = this->getStructTemplate(*struct_info.templateID);

					auto builder = std::string();
					builder += struct_name;

					builder += "<{";

					const evo::SmallVector<BaseType::StructTemplate::Arg> template_args =
						struct_template.getInstantiationArgs(struct_info.instantiation);


					// TODO(PERF): 
					for(size_t i = 0; const BaseType::StructTemplate::Arg& template_arg : template_args){
						if(template_arg.is<TypeInfo::VoidableID>()){
							builder += this->printType(template_arg.as<TypeInfo::VoidableID>(), source_manager);

						}else if(*struct_template.params[i].typeID == TypeManager::getTypeBool()){
							builder += evo::boolStr(template_arg.as<core::GenericValue>().as<bool>());

						}else if(*struct_template.params[i].typeID == TypeManager::getTypeChar()){
							builder += "'";
							builder += char(template_arg.as<core::GenericValue>().as<core::GenericInt>());
							builder += "'";

						}else if(this->isUnsignedIntegral(*struct_template.params[i].typeID)){
							builder += template_arg.as<core::GenericValue>().as<core::GenericInt>().toString(false);

						}else if(this->isIntegral(*struct_template.params[i].typeID)){
							builder += template_arg.as<core::GenericValue>().as<core::GenericInt>().toString(true);

						}else if(this->isFloatingPoint(*struct_template.params[i].typeID)){
							builder += template_arg.as<core::GenericValue>().as<core::GenericFloat>().toString();
							
						}else{
							builder += "<EXPR>";
						}


						if(i + 1 < template_args.size()){
							builder += ", ";
						}
					
						i += 1;
					}

					builder += "}>";

					return builder;

				} break;

				case BaseType::Kind::STRUCT_TEMPLATE: {
					const BaseType::StructTemplate::ID struct_template_id = type_info.baseTypeID().structTemplateID();
					const BaseType::StructTemplate& struct_template_info = this->getStructTemplate(struct_template_id);

					const TokenBuffer& token_buffer = source_manager[struct_template_info.sourceID].getTokenBuffer();
					return std::string(token_buffer[struct_template_info.identTokenID].getString());
				} break;

				case BaseType::Kind::TYPE_DEDUCER: {
					const BaseType::TypeDeducer::ID type_deducer_id = type_info.baseTypeID().typeDeducerID();
					const BaseType::TypeDeducer& type_deducer = this->getTypeDeducer(type_deducer_id);

					const Token& token = source_manager[type_deducer.sourceID].getTokenBuffer()[type_deducer.tokenID];

					if(token.kind() == Token::Kind::TYPE_DEDUCER){
						return std::format("${}", token.getString());
					}else{
						evo::debugAssert(
							token.kind() == Token::Kind::ANONYMOUS_TYPE_DEDUCER, "Unknown type deducer kind"
						);
						return "$$";
					}
				} break;

				case BaseType::Kind::DUMMY: evo::debugFatalBreak("Dummy type should not be used");
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
		const auto lock = std::scoped_lock(this->functions_lock);
		return this->functions[id];
	}

	auto TypeManager::getOrCreateFunction(BaseType::Function&& lookup_func) -> BaseType::ID {
		const auto lock = std::scoped_lock(this->functions_lock);

		for(uint32_t i = 0; i < this->functions.size(); i+=1){
			if(this->functions[BaseType::Function::ID(i)] == lookup_func){
				return BaseType::ID(BaseType::Kind::FUNCTION, i);
			}
		}

		const BaseType::Function::ID new_function = this->functions.emplace_back(lookup_func);
		return BaseType::ID(BaseType::Kind::FUNCTION, new_function.get());
	}


	//////////////////////////////////////////////////////////////////////
	// array

	auto TypeManager::getArray(BaseType::Array::ID id) const -> const BaseType::Array& {
		const auto lock = std::scoped_lock(this->arrays_lock);
		return this->arrays[id];
	}

	auto TypeManager::getOrCreateArray(BaseType::Array&& lookup_func) -> BaseType::ID {
		const auto lock = std::scoped_lock(this->arrays_lock);

		for(uint32_t i = 0; i < this->arrays.size(); i+=1){
			if(this->arrays[BaseType::Array::ID(i)] == lookup_func){
				return BaseType::ID(BaseType::Kind::ARRAY, i);
			}
		}

		const BaseType::Array::ID new_array = this->arrays.emplace_back(lookup_func);
		return BaseType::ID(BaseType::Kind::ARRAY, new_array.get());
	}


	//////////////////////////////////////////////////////////////////////
	// primitive

	auto TypeManager::getPrimitive(BaseType::Primitive::ID id) const -> const BaseType::Primitive& {
		const auto lock = std::scoped_lock(this->primitives_lock);
		return this->primitives[id];
	}

	auto TypeManager::getOrCreatePrimitiveBaseType(Token::Kind kind) -> BaseType::ID {
		return this->get_or_create_primitive_base_type_impl(BaseType::Primitive(kind));
	}

	auto TypeManager::getOrCreatePrimitiveBaseType(Token::Kind kind, uint32_t bit_width) -> BaseType::ID {
		return this->get_or_create_primitive_base_type_impl(BaseType::Primitive(kind, bit_width));
	}

	auto TypeManager::get_or_create_primitive_base_type_impl(const BaseType::Primitive& lookup_type) -> BaseType::ID {
		const auto lock = std::scoped_lock(this->primitives_lock);

		for(uint32_t i = 0; i < this->primitives.size(); i+=1){
			if(this->primitives[BaseType::Primitive::ID(i)] == lookup_type){
				return BaseType::ID(BaseType::Kind::PRIMITIVE, i);
			}
		}

		const BaseType::Primitive::ID new_primitive = this->primitives.emplace_back(lookup_type);
		return BaseType::ID(BaseType::Kind::PRIMITIVE, new_primitive.get());
	}


	//////////////////////////////////////////////////////////////////////
	// aliases

	auto TypeManager::getAlias(BaseType::Alias::ID id) const -> const BaseType::Alias& {
		const auto lock = std::scoped_lock(this->aliases_lock);
		return this->aliases[id];
	}

	auto TypeManager::getAlias(BaseType::Alias::ID id) -> BaseType::Alias& {
		const auto lock = std::scoped_lock(this->aliases_lock);
		return this->aliases[id];
	}


	auto TypeManager::getOrCreateAlias(BaseType::Alias&& lookup_type) -> BaseType::ID {
		const auto lock = std::scoped_lock(this->aliases_lock);

		for(uint32_t i = 0; i < this->aliases.size(); i+=1){
			if(this->aliases[BaseType::Alias::ID(i)] == lookup_type){
				return BaseType::ID(BaseType::Kind::ALIAS, i);
			}
		}

		const BaseType::Alias::ID new_alias = this->aliases.emplace_back(
			lookup_type.sourceID, lookup_type.identTokenID, lookup_type.aliasedType.load(), lookup_type.isPub
		);
		return BaseType::ID(BaseType::Kind::ALIAS, new_alias.get());
	}


	//////////////////////////////////////////////////////////////////////
	// typedefs

	auto TypeManager::getTypedef(BaseType::Typedef::ID id) const -> const BaseType::Typedef& {
		const auto lock = std::scoped_lock(this->typedefs_lock);
		return this->typedefs[id];
	}

	auto TypeManager::getTypedef(BaseType::Typedef::ID id) -> BaseType::Typedef& {
		const auto lock = std::scoped_lock(this->typedefs_lock);
		return this->typedefs[id];
	}


	auto TypeManager::getOrCreateTypedef(BaseType::Typedef&& lookup_type) -> BaseType::ID {
		const auto lock = std::scoped_lock(this->typedefs_lock);

		for(uint32_t i = 0; i < this->typedefs.size(); i+=1){
			if(this->typedefs[BaseType::Typedef::ID(i)] == lookup_type){
				return BaseType::ID(BaseType::Kind::TYPEDEF, i);
			}
		}

		const BaseType::Typedef::ID new_typedef = this->typedefs.emplace_back(
			lookup_type.sourceID, lookup_type.identTokenID, lookup_type.underlyingType.load(), lookup_type.isPub
		);
		return BaseType::ID(BaseType::Kind::TYPEDEF, new_typedef.get());
	}


	//////////////////////////////////////////////////////////////////////
	// structs

	auto TypeManager::getStruct(BaseType::Struct::ID id) const -> const BaseType::Struct& {
		const auto lock = std::scoped_lock(this->structs_lock);
		return this->structs[id];
	}


	auto TypeManager::getStruct(BaseType::Struct::ID id) -> BaseType::Struct& {
		const auto lock = std::scoped_lock(this->structs_lock);
		return this->structs[id];
	}

	auto TypeManager::getNumStructs() const -> size_t {
		const auto lock = std::scoped_lock(this->structs_lock);
		return this->structs.size();
	}


	auto TypeManager::getOrCreateStruct(BaseType::Struct&& lookup_type) -> BaseType::ID {
		const auto lock = std::scoped_lock(this->structs_lock);

		for(uint32_t i = 0; i < this->structs.size(); i+=1){
			if(this->structs[BaseType::Struct::ID(i)] == lookup_type){
				return BaseType::ID(BaseType::Kind::STRUCT, i);
			}
		}

		const BaseType::Struct::ID new_struct = this->structs.emplace_back(
			lookup_type.sourceID,
			lookup_type.identTokenID,
			lookup_type.templateID,
			lookup_type.instantiation,
			std::move(lookup_type.memberVars),
			std::move(lookup_type.memberVarsABI),
			lookup_type.namespacedMembers,
			lookup_type.scopeLevel,
			lookup_type.isPub,
			lookup_type.isOrdered,
			lookup_type.isPacked
		);
		return BaseType::ID(BaseType::Kind::STRUCT, new_struct.get());
	}


	//////////////////////////////////////////////////////////////////////
	// struct templates

	auto TypeManager::getStructTemplate(BaseType::StructTemplate::ID id) const -> const BaseType::StructTemplate& {
		const auto lock = std::scoped_lock(this->struct_templates_lock);
		return this->struct_templates[id];
	}

	auto TypeManager::getStructTemplate(BaseType::StructTemplate::ID id) -> BaseType::StructTemplate& {
		const auto lock = std::scoped_lock(this->struct_templates_lock);
		return this->struct_templates[id];
	}



	auto TypeManager::getOrCreateStructTemplate(BaseType::StructTemplate&& lookup_type) -> BaseType::ID {
		const auto lock = std::scoped_lock(this->struct_templates_lock);

		for(uint32_t i = 0; i < this->struct_templates.size(); i+=1){
			if(this->struct_templates[BaseType::StructTemplate::ID(i)] == lookup_type){
				return BaseType::ID(BaseType::Kind::STRUCT_TEMPLATE, i);
			}
		}

		const BaseType::StructTemplate::ID new_struct = this->struct_templates.emplace_back(
			lookup_type.sourceID,
			lookup_type.identTokenID,
			std::move(lookup_type.params),
			lookup_type.minNumTemplateArgs
		);
		return BaseType::ID(BaseType::Kind::STRUCT_TEMPLATE, new_struct.get());
	}


	//////////////////////////////////////////////////////////////////////
	// type deducer

	auto TypeManager::getTypeDeducer(BaseType::TypeDeducer::ID id) const -> const BaseType::TypeDeducer& {
		const auto lock = std::scoped_lock(this->type_deducers_lock);
		return this->type_deducers[id];
	}


	auto TypeManager::getOrCreateTypeDeducer(BaseType::TypeDeducer&& lookup_type) -> BaseType::ID {
		const auto lock = std::scoped_lock(this->type_deducers_lock);

		for(uint32_t i = 0; i < this->type_deducers.size(); i+=1){
			if(this->type_deducers[BaseType::TypeDeducer::ID(i)] == lookup_type){
				return BaseType::ID(BaseType::Kind::TYPE_DEDUCER, i);
			}
		}

		const BaseType::TypeDeducer::ID new_type_deducer = this->type_deducers.emplace_back(std::move(lookup_type));
		return BaseType::ID(BaseType::Kind::TYPE_DEDUCER, new_type_deducer.get());
	}


	//////////////////////////////////////////////////////////////////////
	// type traits

	// https://stackoverflow.com/a/1766566
	static constexpr auto round_up_to_nearest_multiple_of_8(size_t num) -> size_t {
		return (num + (8 - 1)) & ~(8 - 1);
	}

	auto TypeManager::numBytes(TypeInfo::ID id) const -> size_t {
		const TypeInfo& type_info = this->getTypeInfo(id);
		if(type_info.qualifiers().empty()){ return this->numBytes(type_info.baseTypeID()); }

		evo::debugAssert(
			type_info.qualifiers().back().isPtr || !type_info.qualifiers().back().isOptional,
			"optionals are not supported yet"
		);

		return this->numBytesOfPtr();
	}


	auto TypeManager::numBytes(BaseType::ID id) const -> uint64_t {
		switch(id.kind()){
			case BaseType::Kind::PRIMITIVE: {
				const BaseType::Primitive& primitive = this->getPrimitive(id.primitiveID());

				switch(primitive.kind()){
					case Token::Kind::TYPE_INT: case Token::Kind::TYPE_UINT:
						return this->numBytesOfGeneralRegister();

					case Token::Kind::TYPE_ISIZE: case Token::Kind::TYPE_USIZE:
						return this->numBytesOfPtr();

					case Token::Kind::TYPE_I_N: case Token::Kind::TYPE_UI_N:
						return round_up_to_nearest_multiple_of_8(
							round_up_to_nearest_multiple_of_8(primitive.bitWidth()) / 8
						);

					case Token::Kind::TYPE_F16:    return 2;
					case Token::Kind::TYPE_BF16:   return 2;
					case Token::Kind::TYPE_F32:    return 4;
					case Token::Kind::TYPE_F64:    return 8;
					case Token::Kind::TYPE_F80:    return 16;
					case Token::Kind::TYPE_F128:   return 16;
					case Token::Kind::TYPE_BYTE:   return 1;
					case Token::Kind::TYPE_BOOL:   return 1;
					case Token::Kind::TYPE_CHAR:   return 1;
					case Token::Kind::TYPE_RAWPTR: return this->numBytesOfPtr();
					case Token::Kind::TYPE_TYPEID: return 4;

					// https://en.cppreference.com/w/cpp/language/types
					case Token::Kind::TYPE_C_SHORT: case Token::Kind::TYPE_C_USHORT:
					    return 2;

					case Token::Kind::TYPE_C_INT: case Token::Kind::TYPE_C_UINT:
						return 4;

					case Token::Kind::TYPE_C_LONG: case Token::Kind::TYPE_C_ULONG:
						return this->platform.os == core::Platform::OS::WINDOWS ? 4 : 8;

					case Token::Kind::TYPE_C_LONG_LONG: case Token::Kind::TYPE_C_ULONG_LONG:
						return 8;

					case Token::Kind::TYPE_C_LONG_DOUBLE:
						return this->platform.os == core::Platform::OS::WINDOWS ? 8 : 16;

					default: evo::debugFatalBreak("Unknown or unsupported built-in type");
				}
			} break;

			case BaseType::Kind::FUNCTION: {
				return this->numBytesOfPtr();
			} break;

			case BaseType::Kind::ARRAY: {
				const BaseType::Array& array = this->getArray(id.arrayID());
				const uint64_t elem_size = this->numBytes(array.elementTypeID);

				if(array.terminator.has_value()){ return elem_size * array.lengths.back() + 1; }

				uint64_t output = elem_size;
				for(uint64_t length : array.lengths){
					output *= length;
				}
				return output;
			} break;

			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias = this->getAlias(id.aliasID());
				evo::debugAssert(alias.aliasedType.load().has_value(), "Definition of alias was not completed");
				return this->numBytes(*alias.aliasedType.load());
			} break;

			case BaseType::Kind::TYPEDEF: {
				const BaseType::Typedef& type_def = this->getTypedef(id.typedefID());
				evo::debugAssert(type_def.underlyingType.load().has_value(), "Definition of typedef was not completed");
				return this->numBytes(*type_def.underlyingType.load());
			} break;

			case BaseType::Kind::STRUCT: {
				const BaseType::Struct& struct_info = this->getStruct(id.structID());
				
				size_t total = 0;

				for(const BaseType::Struct::MemberVar& member_var : struct_info.memberVars){
					total += this->numBytes(member_var.typeID);
				}

				return total;
			} break;

			case BaseType::Kind::STRUCT_TEMPLATE: {
				// TODO(FUTURE): handle this better?
				evo::debugAssert("Cannot get size of Struct Template");
			} break;

			case BaseType::Kind::TYPE_DEDUCER: {
				// TODO(FUTURE): handle this better?
				evo::debugAssert("Cannot get size of type deducer");
			} break;

			case BaseType::Kind::DUMMY: evo::debugFatalBreak("Dummy type should not be used");
		}

		evo::debugFatalBreak("Unknown or unsupported base-type kind");
	}

	auto TypeManager::numBytesOfPtr() const -> uint64_t { return 8; }
	auto TypeManager::numBytesOfGeneralRegister() const -> uint64_t { return 8; }


	///////////////////////////////////
	// numBits

	auto TypeManager::numBits(TypeInfo::ID id) const -> size_t {
		const TypeInfo& type_info = this->getTypeInfo(id);
		if(type_info.qualifiers().empty()){ return this->numBits(type_info.baseTypeID()); }

		evo::debugAssert(type_info.isOptionalNotPointer(), "optionals are not supported yet");

		return this->numBitsOfPtr();
	}


	auto TypeManager::numBits(BaseType::ID id) const -> uint64_t {
		switch(id.kind()){
			case BaseType::Kind::PRIMITIVE: {
				const BaseType::Primitive& primitive = this->getPrimitive(id.primitiveID());

				switch(primitive.kind()){
					case Token::Kind::TYPE_INT: case Token::Kind::TYPE_UINT:
						return this->numBitsOfGeneralRegister();

					case Token::Kind::TYPE_ISIZE: case Token::Kind::TYPE_USIZE:
						return this->numBitsOfPtr();

					case Token::Kind::TYPE_I_N: case Token::Kind::TYPE_UI_N:
						return primitive.bitWidth();

					case Token::Kind::TYPE_F16:    return 16;
					case Token::Kind::TYPE_BF16:   return 16;
					case Token::Kind::TYPE_F32:    return 32;
					case Token::Kind::TYPE_F64:    return 64;
					case Token::Kind::TYPE_F80:    return 80;
					case Token::Kind::TYPE_F128:   return 128;
					case Token::Kind::TYPE_BYTE:   return 8;
					case Token::Kind::TYPE_BOOL:   return 1;
					case Token::Kind::TYPE_CHAR:   return 8;
					case Token::Kind::TYPE_RAWPTR: return this->numBitsOfPtr();
					case Token::Kind::TYPE_TYPEID: return 32;

					// https://en.cppreference.com/w/cpp/language/types
					case Token::Kind::TYPE_C_SHORT: case Token::Kind::TYPE_C_USHORT:
					    return 16;

					case Token::Kind::TYPE_C_INT: case Token::Kind::TYPE_C_UINT:
						return 32;

					case Token::Kind::TYPE_C_LONG: case Token::Kind::TYPE_C_ULONG:
						return this->platform.os == core::Platform::OS::WINDOWS ? 32 : 64;

					case Token::Kind::TYPE_C_LONG_LONG: case Token::Kind::TYPE_C_ULONG_LONG:
						return 64;

					case Token::Kind::TYPE_C_LONG_DOUBLE: {
						if(this->platform.os == core::Platform::OS::WINDOWS){
							return 64;
						}
						
						return this->platform.arch == core::Platform::Architecture::X86_64 ? 80 : 128;
					} break;

					default: evo::debugFatalBreak("Unknown or unsupported built-in type");
				}
			} break;

			case BaseType::Kind::FUNCTION: {
				return this->numBitsOfPtr();
			} break;

			case BaseType::Kind::ARRAY: {
				return this->numBytes(id) * 8;
			} break;

			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias = this->getAlias(id.aliasID());
				evo::debugAssert(alias.aliasedType.load().has_value(), "Definition of alias was not completed");
				return this->numBits(*alias.aliasedType.load());
			} break;

			case BaseType::Kind::TYPEDEF: {
				const BaseType::Typedef& type_def = this->getTypedef(id.typedefID());
				evo::debugAssert(type_def.underlyingType.load().has_value(), "Definition of typedef was not completed");
				return this->numBits(*type_def.underlyingType.load());
			} break;

			case BaseType::Kind::STRUCT: {
				return this->numBytes(id) * 8;
			} break;

			case BaseType::Kind::STRUCT_TEMPLATE: {
				// TODO(FUTURE): handle this better?
				evo::debugAssert("Cannot get size of Struct Template");
			} break;

			case BaseType::Kind::TYPE_DEDUCER: {
				// TODO(FUTURE): handle this better?
				evo::debugAssert("Cannot get size of type deducer");
			} break;

			case BaseType::Kind::DUMMY: evo::debugFatalBreak("Dummy type should not be used");
		}

		evo::debugFatalBreak("Unknown or unsupported base-type kind");
	}



	auto TypeManager::numBitsOfPtr() const -> uint64_t { return 64; }
	auto TypeManager::numBitsOfGeneralRegister() const -> uint64_t { return 64; }



	///////////////////////////////////
	// isTriviallySized

	auto TypeManager::isTriviallySized(TypeInfo::ID id) const -> bool {
		const TypeInfo& type_info = this->getTypeInfo(id);
		if(type_info.isPointer()){ return true; }
		if(type_info.qualifiers().back().isOptional){ evo::unimplemented("Trivially Copyable of optionals"); }
		return this->isTriviallySized(type_info.baseTypeID());
	}

	auto TypeManager::isTriviallySized(BaseType::ID id) const -> bool {
		return this->numBytes(id) <= this->numBytesOfPtr();
	}


	///////////////////////////////////
	// isTriviallyCopyable

	auto TypeManager::isTriviallyCopyable(TypeInfo::ID id) const -> bool {
		const TypeInfo& type_info = this->getTypeInfo(id);
		if(type_info.isPointer()){ return true; }
		if(type_info.qualifiers().back().isOptional){ evo::unimplemented("Trivially Copyable of optionals"); }
		return this->isTriviallyCopyable(type_info.baseTypeID());
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
		return id.kind() == BaseType::Kind::PRIMITIVE;
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
		if(id.kind() != BaseType::Kind::PRIMITIVE){ return false; }

		const BaseType::Primitive& primitive = this->getPrimitive(id.primitiveID());

		switch(primitive.kind()){
			case Token::Kind::TYPE_INT:           return true;
			case Token::Kind::TYPE_ISIZE:         return true;
			case Token::Kind::TYPE_I_N:           return true;
			case Token::Kind::TYPE_UINT:          return true;
			case Token::Kind::TYPE_USIZE:         return true;
			case Token::Kind::TYPE_UI_N:          return true;
			case Token::Kind::TYPE_F16:           return false;
			case Token::Kind::TYPE_BF16:          return false;
			case Token::Kind::TYPE_F32:           return false;
			case Token::Kind::TYPE_F64:           return false;
			case Token::Kind::TYPE_F80:           return false;
			case Token::Kind::TYPE_F128:          return false;
			case Token::Kind::TYPE_BYTE:          return false;
			case Token::Kind::TYPE_BOOL:          return false;
			case Token::Kind::TYPE_CHAR:          return false;
			case Token::Kind::TYPE_RAWPTR:        return false;
			case Token::Kind::TYPE_TYPEID:        return false;
			case Token::Kind::TYPE_C_SHORT:       return true;
			case Token::Kind::TYPE_C_USHORT:      return true;
			case Token::Kind::TYPE_C_INT:         return true;
			case Token::Kind::TYPE_C_UINT:        return true;
			case Token::Kind::TYPE_C_LONG:        return true;
			case Token::Kind::TYPE_C_ULONG:       return true;
			case Token::Kind::TYPE_C_LONG_LONG:   return true;
			case Token::Kind::TYPE_C_ULONG_LONG:  return true;
			case Token::Kind::TYPE_C_LONG_DOUBLE: return false;
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
		if(id.kind() != BaseType::Kind::PRIMITIVE){ return false; }

		const BaseType::Primitive& primitive = this->getPrimitive(id.primitiveID());

		switch(primitive.kind()){
			case Token::Kind::TYPE_INT:           return false;
			case Token::Kind::TYPE_ISIZE:         return false;
			case Token::Kind::TYPE_I_N:           return false;
			case Token::Kind::TYPE_UINT:          return true;
			case Token::Kind::TYPE_USIZE:         return true;
			case Token::Kind::TYPE_UI_N:          return true;
			case Token::Kind::TYPE_F16:           return false;
			case Token::Kind::TYPE_BF16:          return false;
			case Token::Kind::TYPE_F32:           return false;
			case Token::Kind::TYPE_F64:           return false;
			case Token::Kind::TYPE_F80:           return false;
			case Token::Kind::TYPE_F128:          return false;
			case Token::Kind::TYPE_BYTE:          return false;
			case Token::Kind::TYPE_BOOL:          return false;
			case Token::Kind::TYPE_CHAR:          return false;
			case Token::Kind::TYPE_RAWPTR:        return false;
			case Token::Kind::TYPE_TYPEID:        return false;
			case Token::Kind::TYPE_C_SHORT:       return false;
			case Token::Kind::TYPE_C_USHORT:      return true;
			case Token::Kind::TYPE_C_INT:         return false;
			case Token::Kind::TYPE_C_UINT:        return true;
			case Token::Kind::TYPE_C_LONG:        return false;
			case Token::Kind::TYPE_C_ULONG:       return true;
			case Token::Kind::TYPE_C_LONG_LONG:   return false;
			case Token::Kind::TYPE_C_ULONG_LONG:  return true;
			case Token::Kind::TYPE_C_LONG_DOUBLE: return false;
			default: evo::debugFatalBreak("Not a type");
		}
	}


	///////////////////////////////////
	// isSignedIntegral

	auto TypeManager::isSignedIntegral(TypeInfo::VoidableID id) const -> bool {
		if(id.isVoid()){ return false; }
		return this->isSignedIntegral(id.asTypeID());
	}

	auto TypeManager::isSignedIntegral(TypeInfo::ID id) const -> bool {
		const TypeInfo& type_info = this->getTypeInfo(id);

		if(type_info.qualifiers().empty()){ return this->isSignedIntegral(type_info.baseTypeID()); }

		return false;
	}


	auto TypeManager::isSignedIntegral(BaseType::ID id) const -> bool {
		if(id.kind() != BaseType::Kind::PRIMITIVE){ return false; }

		const BaseType::Primitive& primitive = this->getPrimitive(id.primitiveID());

		switch(primitive.kind()){
			case Token::Kind::TYPE_INT:           return true;
			case Token::Kind::TYPE_ISIZE:         return true;
			case Token::Kind::TYPE_I_N:           return true;
			case Token::Kind::TYPE_UINT:          return false;
			case Token::Kind::TYPE_USIZE:         return false;
			case Token::Kind::TYPE_UI_N:          return false;
			case Token::Kind::TYPE_F16:           return false;
			case Token::Kind::TYPE_BF16:          return false;
			case Token::Kind::TYPE_F32:           return false;
			case Token::Kind::TYPE_F64:           return false;
			case Token::Kind::TYPE_F80:           return false;
			case Token::Kind::TYPE_F128:          return false;
			case Token::Kind::TYPE_BYTE:          return false;
			case Token::Kind::TYPE_BOOL:          return false;
			case Token::Kind::TYPE_CHAR:          return false;
			case Token::Kind::TYPE_RAWPTR:        return false;
			case Token::Kind::TYPE_TYPEID:        return false;
			case Token::Kind::TYPE_C_SHORT:       return true;
			case Token::Kind::TYPE_C_USHORT:      return false;
			case Token::Kind::TYPE_C_INT:         return true;
			case Token::Kind::TYPE_C_UINT:        return false;
			case Token::Kind::TYPE_C_LONG:        return true;
			case Token::Kind::TYPE_C_ULONG:       return false;
			case Token::Kind::TYPE_C_LONG_LONG:   return true;
			case Token::Kind::TYPE_C_ULONG_LONG:  return false;
			case Token::Kind::TYPE_C_LONG_DOUBLE: return false;
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
		if(id.kind() != BaseType::Kind::PRIMITIVE){ return false; }

		const BaseType::Primitive& primitive = this->getPrimitive(id.primitiveID());
		switch(primitive.kind()){
			case Token::Kind::TYPE_INT:           return false;
			case Token::Kind::TYPE_ISIZE:         return false;
			case Token::Kind::TYPE_I_N:           return false;
			case Token::Kind::TYPE_UINT:          return false;
			case Token::Kind::TYPE_USIZE:         return false;
			case Token::Kind::TYPE_UI_N:          return false;
			case Token::Kind::TYPE_F16:           return true;
			case Token::Kind::TYPE_BF16:          return true;
			case Token::Kind::TYPE_F32:           return true;
			case Token::Kind::TYPE_F64:           return true;
			case Token::Kind::TYPE_F80:           return true;
			case Token::Kind::TYPE_F128:          return true;
			case Token::Kind::TYPE_BYTE:          return false;
			case Token::Kind::TYPE_BOOL:          return false;
			case Token::Kind::TYPE_CHAR:          return false;
			case Token::Kind::TYPE_RAWPTR:        return false;
			case Token::Kind::TYPE_TYPEID:        return false;
			case Token::Kind::TYPE_C_SHORT:       return false;
			case Token::Kind::TYPE_C_USHORT:      return false;
			case Token::Kind::TYPE_C_INT:         return false;
			case Token::Kind::TYPE_C_UINT:        return false;
			case Token::Kind::TYPE_C_LONG:        return false;
			case Token::Kind::TYPE_C_ULONG:       return false;
			case Token::Kind::TYPE_C_LONG_LONG:   return false;
			case Token::Kind::TYPE_C_ULONG_LONG:  return false;
			case Token::Kind::TYPE_C_LONG_DOUBLE: return true;
			default: evo::debugFatalBreak("Not a type");
		}
	}
	
	///////////////////////////////////
	// getUnderlyingType

	auto TypeManager::getUnderlyingType(TypeInfo::ID id) -> TypeInfo::ID {
		const TypeInfo& type_info = this->getTypeInfo(id);

		if(type_info.qualifiers().empty()){ return this->getUnderlyingType(type_info.baseTypeID()); }
		
		if(type_info.qualifiers().back().isPtr){ return TypeManager::getTypeRawPtr(); }

		return id;
	}

	// TODO(PERF): optimize this function
	auto TypeManager::getUnderlyingType(BaseType::ID id) -> TypeInfo::ID {
		switch(id.kind()){
			case BaseType::Kind::DUMMY:     evo::debugFatalBreak("Dummy type should not be used");
			case BaseType::Kind::PRIMITIVE: break;
			case BaseType::Kind::FUNCTION:  return this->getOrCreateTypeInfo(TypeInfo(id));
			case BaseType::Kind::ARRAY:     return this->getOrCreateTypeInfo(TypeInfo(id));
			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias = this->getAlias(id.aliasID());
				evo::debugAssert(alias.aliasedType.load().has_value(), "Definition of alias was not completed");
				return this->getUnderlyingType(*alias.aliasedType.load());
			} break;
			case BaseType::Kind::TYPEDEF: {
				const BaseType::Typedef& typedef_info = this->getTypedef(id.typedefID());
				evo::debugAssert(
					typedef_info.underlyingType.load().has_value(), "Definition of typedef was not completed"
				);
				return this->getUnderlyingType(*typedef_info.underlyingType.load());
			} break;
			case BaseType::Kind::STRUCT:          return this->getOrCreateTypeInfo(TypeInfo(id));
			case BaseType::Kind::STRUCT_TEMPLATE: evo::debugFatalBreak("Cannot get underlying type of this kind");
			case BaseType::Kind::TYPE_DEDUCER:    evo::debugFatalBreak("Cannot get underlying type of this kind");
		}

		const BaseType::Primitive& primitive = this->getPrimitive(id.primitiveID());
		switch(primitive.kind()){
			case Token::Kind::TYPE_INT:{
				return this->getOrCreateTypeInfo(
					TypeInfo(
						this->getOrCreatePrimitiveBaseType(
							Token::Kind::TYPE_I_N, uint32_t(this->numBytesOfGeneralRegister()) * 8
						)
					)
				);
			} break;

			case Token::Kind::TYPE_ISIZE:{
				return this->getOrCreateTypeInfo(
					TypeInfo(this->getOrCreatePrimitiveBaseType(Token::Kind::TYPE_I_N, uint32_t(this->numBytesOfPtr()) * 8))
				);
			} break;

			case Token::Kind::TYPE_I_N: {
				return this->getOrCreateTypeInfo(TypeInfo(id));
			} break;

			case Token::Kind::TYPE_UINT: {
				return this->getOrCreateTypeInfo(
					TypeInfo(
						this->getOrCreatePrimitiveBaseType(
							Token::Kind::TYPE_UI_N, uint32_t(this->numBytesOfGeneralRegister()) * 8
						)
					)
				);
			} break;

			case Token::Kind::TYPE_USIZE: {
				return this->getOrCreateTypeInfo(
					TypeInfo(
						this->getOrCreatePrimitiveBaseType(Token::Kind::TYPE_UI_N, uint32_t(this->numBytesOfPtr()) * 8)
					)
				);
			} break;

			case Token::Kind::TYPE_UI_N: {
				return this->getOrCreateTypeInfo(TypeInfo(id));
			} break;

			case Token::Kind::TYPE_F16: {
				return this->getOrCreateTypeInfo(TypeInfo(id));
			} break;

			case Token::Kind::TYPE_BF16: {
				return this->getOrCreateTypeInfo(TypeInfo(id));
			} break;

			case Token::Kind::TYPE_F32: {
				return this->getOrCreateTypeInfo(TypeInfo(id));
			} break;

			case Token::Kind::TYPE_F64: {
				return this->getOrCreateTypeInfo(TypeInfo(id));
			} break;

			case Token::Kind::TYPE_F80: {
				return this->getOrCreateTypeInfo(TypeInfo(id));
			} break;

			case Token::Kind::TYPE_F128: {
				return this->getOrCreateTypeInfo(TypeInfo(id));
			} break;

			case Token::Kind::TYPE_BYTE: {
				return this->getOrCreateTypeInfo(
					TypeInfo(this->getOrCreatePrimitiveBaseType(Token::Kind::TYPE_UI_N, 8))
				);
			} break;

			case Token::Kind::TYPE_BOOL: {
				return this->getOrCreateTypeInfo(TypeInfo(id));
			} break;

			case Token::Kind::TYPE_CHAR: {
				return this->getOrCreateTypeInfo(
					TypeInfo(this->getOrCreatePrimitiveBaseType(Token::Kind::TYPE_UI_N, 8))
				);
			} break;

			case Token::Kind::TYPE_RAWPTR: {
				return TypeManager::getTypeRawPtr();
			} break;

			case Token::Kind::TYPE_TYPEID: {
				return this->getOrCreateTypeInfo(
					TypeInfo(this->getOrCreatePrimitiveBaseType(Token::Kind::TYPE_UI_N, 32))
				);
			} break;

			case Token::Kind::TYPE_C_SHORT: case Token::Kind::TYPE_C_INT: {
				return this->getOrCreateTypeInfo(
					TypeInfo(this->getOrCreatePrimitiveBaseType(Token::Kind::TYPE_I_N, 32))
				);
			} break;

			case Token::Kind::TYPE_C_LONG: case Token::Kind::TYPE_C_LONG_LONG: {
				const uint32_t size = this->platform.os == core::Platform::OS::WINDOWS ? 32 : 64;
				return this->getOrCreateTypeInfo(
					TypeInfo(this->getOrCreatePrimitiveBaseType(Token::Kind::TYPE_I_N, size))
				);
			} break;

			case Token::Kind::TYPE_C_USHORT: case Token::Kind::TYPE_C_UINT: {
				return this->getOrCreateTypeInfo(
					TypeInfo(this->getOrCreatePrimitiveBaseType(Token::Kind::TYPE_UI_N, 32))
				);
			} break;

			case Token::Kind::TYPE_C_ULONG: case Token::Kind::TYPE_C_ULONG_LONG: {
				const uint32_t size = this->platform.os == core::Platform::OS::WINDOWS ? 32 : 64;
				return this->getOrCreateTypeInfo(
					TypeInfo(this->getOrCreatePrimitiveBaseType(Token::Kind::TYPE_UI_N, size))
				);
			} break;

			case Token::Kind::TYPE_C_LONG_DOUBLE: {
				if(this->platform.os == core::Platform::OS::WINDOWS){
					return this->getOrCreateTypeInfo(
						TypeInfo(this->getOrCreatePrimitiveBaseType(Token::Kind::TYPE_F64))
					);
				}else{
					return this->getOrCreateTypeInfo(
						TypeInfo(this->getOrCreatePrimitiveBaseType(Token::Kind::TYPE_F128))
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

	static auto calc_float_max(unsigned width, int exponent, unsigned precision) -> core::GenericInt {
		return float_data_from_exponent(width, exponent, precision).bitwiseOr(
			core::GenericInt(width, 1)
				.ushl(core::GenericInt(width, precision - 1)).result
				.usub(core::GenericInt(width, 1)).result
		);
	}



	auto TypeManager::getMin(TypeInfo::ID id) const -> core::GenericValue {
		const TypeInfo& type_info = this->getTypeInfo(id);
		evo::debugAssert(type_info.qualifiers().empty(), "Can only get min of primitive");

		return this->getMin(type_info.baseTypeID());
	}

	auto TypeManager::getMin(BaseType::ID id) const -> core::GenericValue {
		const BaseType::Primitive& primitive = this->getPrimitive(id.primitiveID());
		switch(primitive.kind()){
			case Token::Kind::TYPE_INT:   return core::GenericValue(calc_min_signed(this->numBytesOfGeneralRegister() * 8));
			case Token::Kind::TYPE_ISIZE: return core::GenericValue(calc_min_signed(this->numBytesOfPtr() * 8));
			case Token::Kind::TYPE_I_N:   return core::GenericValue(calc_min_signed(primitive.bitWidth()));

			case Token::Kind::TYPE_UINT:
				return core::GenericValue(core::GenericInt(unsigned(this->numBytesOfGeneralRegister() * 8), 0));

			case Token::Kind::TYPE_USIZE: 
				return core::GenericValue(core::GenericInt(unsigned(this->numBytesOfPtr() * 8), 0));

			case Token::Kind::TYPE_UI_N:  return core::GenericValue(core::GenericInt(primitive.bitWidth(), 0));

			case Token::Kind::TYPE_F16:
				return core::GenericValue(core::GenericFloat::createF16(float_data_from_exponent(16, 15, 11)).neg());

			case Token::Kind::TYPE_BF16:
				return core::GenericValue(core::GenericFloat::createBF16(float_data_from_exponent(16, 127, 8)).neg());

			case Token::Kind::TYPE_F32:
				return core::GenericValue(core::GenericFloat::createF32(float_data_from_exponent(32, 127, 24)).neg());

			case Token::Kind::TYPE_F64:
				return core::GenericValue(core::GenericFloat::createF64(float_data_from_exponent(64, 1023, 53)).neg());

			case Token::Kind::TYPE_F80:
				return core::GenericValue(core::GenericFloat::createF80(float_data_from_exponent(80, 16383, 64)).neg());

			case Token::Kind::TYPE_F128:
				return core::GenericValue(
					core::GenericFloat::createF128(float_data_from_exponent(128, 16383, 113)).neg()
				);


			case Token::Kind::TYPE_BYTE:   return core::GenericValue(core::GenericInt(8, 0));
			case Token::Kind::TYPE_BOOL:   return core::GenericValue(false);
			case Token::Kind::TYPE_CHAR:   return core::GenericValue(calc_min_signed(8));

			case Token::Kind::TYPE_RAWPTR:
				return core::GenericValue(core::GenericInt(unsigned(this->numBytesOfPtr() * 8), 0));

			case Token::Kind::TYPE_TYPEID: return core::GenericValue(core::GenericInt(32, 0));
			case Token::Kind::TYPE_C_SHORT: return core::GenericValue(calc_min_signed(16));
			case Token::Kind::TYPE_C_INT:   return core::GenericValue(calc_min_signed(32));
				
			case Token::Kind::TYPE_C_LONG:
				return core::GenericValue(calc_min_signed(this->platform.os == core::Platform::OS::WINDOWS ? 32 : 64));

			case Token::Kind::TYPE_C_LONG_LONG: return core::GenericValue(calc_min_signed(32));
			case Token::Kind::TYPE_C_USHORT:   return core::GenericValue(core::GenericInt(16, 0));
			case Token::Kind::TYPE_C_UINT:     return core::GenericValue(core::GenericInt(32, 0));

			case Token::Kind::TYPE_C_ULONG:     
				return core::GenericValue(
					core::GenericInt(this->platform.os == core::Platform::OS::WINDOWS ? 32 : 64, 0)
				);

			case Token::Kind::TYPE_C_ULONG_LONG: return core::GenericValue(core::GenericInt(64, 0));

			case Token::Kind::TYPE_C_LONG_DOUBLE: {
				if(this->platform.os == core::Platform::OS::WINDOWS){
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
			case Token::Kind::TYPE_INT:   return core::GenericValue(calc_min_signed(this->numBytesOfGeneralRegister() * 8));
			case Token::Kind::TYPE_ISIZE: return core::GenericValue(calc_min_signed(this->numBytesOfPtr() * 8));
			case Token::Kind::TYPE_I_N:   return core::GenericValue(calc_min_signed(primitive.bitWidth()));

			case Token::Kind::TYPE_UINT:
				return core::GenericValue(core::GenericInt(unsigned(this->numBytesOfGeneralRegister() * 8), 0));

			case Token::Kind::TYPE_USIZE:
				return core::GenericValue(core::GenericInt(unsigned(this->numBytesOfPtr() * 8), 0));
				
			case Token::Kind::TYPE_UI_N: return core::GenericValue(core::GenericInt(primitive.bitWidth(), 0));

			case Token::Kind::TYPE_F16:
				return core::GenericValue(core::GenericFloat::createF16(float_data_from_exponent(16, 1, 11)));

			case Token::Kind::TYPE_BF16:
				return core::GenericValue(core::GenericFloat::createBF16(float_data_from_exponent(16, 1, 8)));

			case Token::Kind::TYPE_F32:
				return core::GenericValue(core::GenericFloat::createF32(float_data_from_exponent(32, 1, 24)));

			case Token::Kind::TYPE_F64:
				return core::GenericValue(core::GenericFloat::createF64(float_data_from_exponent(64, 1, 53)));

			case Token::Kind::TYPE_F80:
				return core::GenericValue(core::GenericFloat::createF80(float_data_from_exponent(80, 1, 64)));

			case Token::Kind::TYPE_F128:
				return core::GenericValue(core::GenericFloat::createF128(float_data_from_exponent(128, 1, 113)));


			case Token::Kind::TYPE_BYTE:   return core::GenericValue(core::GenericInt(8, 0));
			case Token::Kind::TYPE_BOOL:   return core::GenericValue(false);
			case Token::Kind::TYPE_CHAR:   return core::GenericValue(calc_min_signed(8));

			case Token::Kind::TYPE_RAWPTR:
				return core::GenericValue(core::GenericInt(unsigned(this->numBytesOfPtr() * 8), 0));

			case Token::Kind::TYPE_TYPEID: return core::GenericValue(core::GenericInt(32, 0));
			case Token::Kind::TYPE_C_SHORT: return core::GenericValue(calc_min_signed(16));
			case Token::Kind::TYPE_C_INT:   return core::GenericValue(calc_min_signed(32));
				
			case Token::Kind::TYPE_C_LONG:
				return core::GenericValue(calc_min_signed(this->platform.os == core::Platform::OS::WINDOWS ? 32 : 64));

			case Token::Kind::TYPE_C_LONG_LONG: return core::GenericValue(calc_min_signed(32));
			case Token::Kind::TYPE_C_USHORT:   return core::GenericValue(core::GenericInt(16, 0));
			case Token::Kind::TYPE_C_UINT:     return core::GenericValue(core::GenericInt(32, 0));

			case Token::Kind::TYPE_C_ULONG:     
				return core::GenericValue(
					core::GenericInt(this->platform.os == core::Platform::OS::WINDOWS ? 32 : 64, 0)
				);

			case Token::Kind::TYPE_C_ULONG_LONG: return core::GenericValue(core::GenericInt(64, 0));

			case Token::Kind::TYPE_C_LONG_DOUBLE: {
				if(this->platform.os == core::Platform::OS::WINDOWS){
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
			case Token::Kind::TYPE_INT:   return core::GenericValue(calc_max_signed(this->numBytesOfGeneralRegister() * 8));
			case Token::Kind::TYPE_ISIZE: return core::GenericValue(calc_max_signed(this->numBytesOfPtr() * 8));
			case Token::Kind::TYPE_I_N:   return core::GenericValue(calc_max_signed(primitive.bitWidth()));

			case Token::Kind::TYPE_UINT:
				return core::GenericValue(calc_max_unsigned(this->numBytesOfGeneralRegister() * 8));

			case Token::Kind::TYPE_USIZE: return core::GenericValue(calc_max_unsigned(this->numBytesOfPtr() * 8));
			case Token::Kind::TYPE_UI_N:  return core::GenericValue(calc_max_unsigned(primitive.bitWidth()));

			case Token::Kind::TYPE_F16:
				return core::GenericValue(core::GenericFloat::createF16(calc_float_max(16, 15, 11)));

			case Token::Kind::TYPE_BF16:
				return core::GenericValue(core::GenericFloat::createBF16(calc_float_max(16, 127, 8)));

			case Token::Kind::TYPE_F32:
				return core::GenericValue(core::GenericFloat::createF32(calc_float_max(32, 127, 24)));

			case Token::Kind::TYPE_F64:
				return core::GenericValue(core::GenericFloat::createF64(calc_float_max(64, 1023, 53)));

			case Token::Kind::TYPE_F80:
				// TODO(FUTURE): is this correct? 
				// 		Doing it the correct way seems to make `llvm::APFloat::toString` print "NaN"
				return core::GenericValue(core::GenericFloat::createF128(calc_float_max(128, 16383, 113)).asF80());
				// return core::GenericValue(core::GenericFloat::createF80(calc_float_max(80, 16383, 64)));

			case Token::Kind::TYPE_F128:
				return core::GenericValue(core::GenericFloat::createF128(calc_float_max(128, 16383, 113)));


			case Token::Kind::TYPE_BYTE:   return core::GenericValue(calc_max_unsigned(8));
			case Token::Kind::TYPE_BOOL:   return core::GenericValue(true);
			case Token::Kind::TYPE_CHAR:   return core::GenericValue(calc_max_signed(8));
			case Token::Kind::TYPE_RAWPTR: return core::GenericValue(calc_max_unsigned(this->numBytesOfPtr() * 8));
			case Token::Kind::TYPE_TYPEID: return core::GenericValue(calc_max_unsigned(32));
			case Token::Kind::TYPE_C_SHORT: return core::GenericValue(calc_max_signed(16));
			case Token::Kind::TYPE_C_INT:   return core::GenericValue(calc_max_signed(32));

			case Token::Kind::TYPE_C_LONG:      
				return core::GenericValue(calc_max_signed(this->platform.os == core::Platform::OS::WINDOWS ? 32 : 64));

			case Token::Kind::TYPE_C_LONG_LONG:  return core::GenericValue(calc_max_signed(32));
			case Token::Kind::TYPE_C_USHORT:    return core::GenericValue(calc_max_unsigned(16));
			case Token::Kind::TYPE_C_UINT:      return core::GenericValue(calc_max_unsigned(32));

			case Token::Kind::TYPE_C_ULONG:     
				return core::GenericValue(
					calc_max_unsigned(this->platform.os == core::Platform::OS::WINDOWS ? 32 : 64)
				);
				
			case Token::Kind::TYPE_C_ULONG_LONG: return core::GenericValue(calc_max_unsigned(64));

			case Token::Kind::TYPE_C_LONG_DOUBLE: {
				if(this->platform.os == core::Platform::OS::WINDOWS){
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