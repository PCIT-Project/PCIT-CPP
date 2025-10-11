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

#include "../include/sema/SemaBuffer.h"


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




	auto BaseType::Alias::getName(const SourceManager& source_manager) const -> std::string_view {
		if(this->isPTHRSourceType()){
			const Source& source = source_manager[this->sourceID.as<Source::ID>()];
			return source.getTokenBuffer()[this->name.as<Token::ID>()].getString();

		}else if(this->isClangType()){
			const ClangSource& clang_source = source_manager[this->sourceID.as<ClangSource::ID>()];
			return clang_source.getDeclInfo(this->name.as<ClangSource::DeclInfoID>()).name;

		}else{
			const BuiltinModule& builtin_module = source_manager[this->sourceID.as<BuiltinModule::ID>()];
			return builtin_module.getString(this->name.as<BuiltinModule::StringID>());
		}
	}


	auto BaseType::Struct::getName(const SourceManager& source_manager) const -> std::string_view {
		if(this->isPTHRSourceType()){
			const Source& source = source_manager[this->sourceID.as<Source::ID>()];
			return source.getTokenBuffer()[this->name.as<Token::ID>()].getString();

		}else if(this->isClangType()){
			const ClangSource& clang_source = source_manager[this->sourceID.as<ClangSource::ID>()];
			return clang_source.getDeclInfo(this->name.as<ClangSource::DeclInfoID>()).name;

		}else{
			const BuiltinModule& builtin_module = source_manager[this->sourceID.as<BuiltinModule::ID>()];
			return builtin_module.getString(this->name.as<BuiltinModule::StringID>());
		}
	}


	auto BaseType::Struct::getMemberName(const MemberVar& member, const SourceManager& source_manager) const
	-> std::string_view {
		if(this->isPTHRSourceType()){
			const Source& source = source_manager[this->sourceID.as<Source::ID>()];
			return source.getTokenBuffer()[member.name.as<Token::ID>()].getString();

		}else if(this->isClangType()){
			const ClangSource& clang_source = source_manager[this->sourceID.as<ClangSource::ID>()];
			return clang_source.getDeclInfo(member.name.as<ClangSource::DeclInfoID>()).name;

		}else{
			const BuiltinModule& builtin_module = source_manager[this->sourceID.as<BuiltinModule::ID>()];
			return builtin_module.getString(member.name.as<BuiltinModule::StringID>());
		}
	}



	auto BaseType::Union::getName(const SourceManager& source_manager) const -> std::string_view {
		if(this->isClangType()){
			const ClangSource& clang_source = source_manager[this->sourceID.as<ClangSource::ID>()];
			return clang_source.getDeclInfo(this->location.as<ClangSource::DeclInfoID>()).name;
		}else{
			const Source& source = source_manager[this->sourceID.as<Source::ID>()];
			return source.getTokenBuffer()[this->location.as<Token::ID>()].getString();
		}
	}


	auto BaseType::Union::getFieldName(const Field& field, const SourceManager& source_manager) const
	-> std::string_view {
		if(this->isClangType()){
			const ClangSource& clang_source = source_manager[this->sourceID.as<ClangSource::ID>()];
			return clang_source.getDeclInfo(field.location.as<ClangSource::DeclInfoID>()).name;
		}else{
			const Source& source = source_manager[this->sourceID.as<Source::ID>()];
			return source.getTokenBuffer()[field.location.as<Token::ID>()].getString();
		}
	}



	auto BaseType::Enum::getName(const SourceManager& source_manager) const -> std::string_view {
		if(this->isClangType()){
			const ClangSource& clang_source = source_manager[this->sourceID.as<ClangSource::ID>()];
			return clang_source.getDeclInfo(this->location.as<ClangSource::DeclInfoID>()).name;
		}else{
			const Source& source = source_manager[this->sourceID.as<Source::ID>()];
			return source.getTokenBuffer()[this->location.as<Token::ID>()].getString();
		}
	}


	auto BaseType::Enum::getEnumeratorName(const Enumerator& enumerator, const SourceManager& source_manager) const
	-> std::string_view {
		if(this->isClangType()){
			const ClangSource& clang_source = source_manager[this->sourceID.as<ClangSource::ID>()];
			return clang_source.getDeclInfo(enumerator.location.as<ClangSource::DeclInfoID>()).name;
		}else{
			const Source& source = source_manager[this->sourceID.as<Source::ID>()];
			return source.getTokenBuffer()[enumerator.location.as<Token::ID>()].getString();
		}
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
		const BaseType::Primitive::ID type_f128    = this->primitives.emplace_back(Token::Kind::TYPE_F128);
		const BaseType::Primitive::ID type_byte    = this->primitives.emplace_back(Token::Kind::TYPE_BYTE);
		const BaseType::Primitive::ID type_bool    = this->primitives.emplace_back(Token::Kind::TYPE_BOOL);
		const BaseType::Primitive::ID type_char    = this->primitives.emplace_back(Token::Kind::TYPE_CHAR);
		const BaseType::Primitive::ID type_raw_ptr = this->primitives.emplace_back(Token::Kind::TYPE_RAWPTR);
		const BaseType::Primitive::ID type_type_id = this->primitives.emplace_back(Token::Kind::TYPE_TYPEID);

		this->primitives.emplace_back(Token::Kind::TYPE_C_WCHAR);
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
		this->types.emplace_back(TypeInfo(BaseType::ID(BaseType::Kind::PRIMITIVE, type_byte.get())));
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

		std::string type_str = this->printType(type_info.baseTypeID(), source_manager);

		bool is_first_qualifer = type_str.back() != '*'
			&& type_str.back() != '|'
			&& type_str.back() != '!'
			&& type_str.back() != '?';

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
			if(qualifier.isUninit){ type_str += '!'; }
			if(qualifier.isOptional){ type_str += '?'; }
		}

		return type_str;
	}


	auto TypeManager::printType(BaseType::ID base_type_id, const SourceManager& source_manager) const -> std::string {
		switch(base_type_id.kind()){
			case BaseType::Kind::DUMMY: evo::debugFatalBreak("Dummy type should not be used");

			case BaseType::Kind::PRIMITIVE: {
				const BaseType::Primitive::ID primitive_id = base_type_id.primitiveID();
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
				const BaseType::Array& array = this->getArray(base_type_id.arrayID());

				auto builder = std::string();
				builder += '[';

				builder += this->printType(array.elementTypeID, source_manager);

				builder += ':';

				for(size_t i = 0; uint64_t dimension : array.dimensions){
					builder += std::to_string(dimension);

					if(i + 1 < array.dimensions.size()){ builder += ","; }

					i += 1;
				}

				if(array.terminator.has_value()){
					if(this->isUnsignedIntegral(array.elementTypeID)){
						builder += ';';
						builder += 
							array.terminator->getInt(unsigned(this->numBits(array.elementTypeID))).toString(false);

					}else if(this->isSignedIntegral(array.elementTypeID)){
						builder += ';';
						builder += 
							array.terminator->getInt(unsigned(this->numBits(array.elementTypeID))).toString(true);

					}else if(this->isFloatingPoint(array.elementTypeID)){
						const BaseType::Primitive& primitive = this->getPrimitive(
							this->getTypeInfo(array.elementTypeID).baseTypeID().primitiveID()
						);

						builder += ';';

						switch(primitive.kind()){
							break; case Token::Kind::TYPE_F16:  builder += array.terminator->getF16().toString();
							break; case Token::Kind::TYPE_BF16: builder += array.terminator->getBF16().toString();
							break; case Token::Kind::TYPE_F32:  builder += array.terminator->getF32().toString();
							break; case Token::Kind::TYPE_F64:  builder += array.terminator->getF64().toString();
							break; case Token::Kind::TYPE_F80:  builder += array.terminator->getF80().toString();
							break; case Token::Kind::TYPE_F128: builder += array.terminator->getF128().toString();
							break; default: evo::debugFatalBreak("Unknown float type");
						}

					}else if(array.elementTypeID == TypeManager::getTypeBool()){
						builder += ';';
						builder += evo::boolStr(array.terminator->getBool());

					}else if(array.elementTypeID == TypeManager::getTypeChar()){
						builder += std::format(";'{}'", evo::printCharName(array.terminator->getChar()));
						
					}else{
						builder += ";<TERMINATOR>";
					}
				}

				builder += ']';

				return builder;
			} break;

			case BaseType::Kind::ARRAY_REF: {
				const BaseType::ArrayRef& array_ref = this->getArrayRef(base_type_id.arrayRefID());

				auto builder = std::string();
				builder += '[';

				builder += this->printType(array_ref.elementTypeID, source_manager);

				builder += ':';


				for(size_t i = 0; const BaseType::ArrayRef::Dimension& dimension : array_ref.dimensions){
					if(dimension.isPtr()){
						if(array_ref.isReadOnly){
							builder += "*|";
						}else{
							builder += "*";
						}
					}else{
						builder += std::to_string(dimension.length());
					}


					if(i + 1 < array_ref.dimensions.size()){
						builder += ",";
					}

					i += 1;
				}

				if(array_ref.terminator.has_value()){
					if(this->isUnsignedIntegral(array_ref.elementTypeID)){
						builder += ';';
						builder += array_ref.terminator->getInt(
							unsigned(this->numBits(array_ref.elementTypeID))
						).toString(false);

					}else if(this->isSignedIntegral(array_ref.elementTypeID)){
						builder += ';';
						builder += array_ref.terminator->getInt(
							unsigned(this->numBits(array_ref.elementTypeID))
						).toString(true);

					}else if(this->isFloatingPoint(array_ref.elementTypeID)){
						const BaseType::Primitive& primitive = this->getPrimitive(
							this->getTypeInfo(array_ref.elementTypeID).baseTypeID().primitiveID()
						);

						builder += ';';

						switch(primitive.kind()){
							break; case Token::Kind::TYPE_F16:
								builder += array_ref.terminator->getF16().toString();

							break; case Token::Kind::TYPE_BF16:
								builder += array_ref.terminator->getBF16().toString();

							break; case Token::Kind::TYPE_F32:
								builder += array_ref.terminator->getF32().toString();

							break; case Token::Kind::TYPE_F64:
								builder += array_ref.terminator->getF64().toString();

							break; case Token::Kind::TYPE_F80:
								builder += array_ref.terminator->getF80().toString();

							break; case Token::Kind::TYPE_F128:
								builder += array_ref.terminator->getF128().toString();

							break; default: evo::debugFatalBreak("Unknown float type");
						}

					}else if(array_ref.elementTypeID == TypeManager::getTypeBool()){
						builder += ';';
						builder += evo::boolStr(array_ref.terminator->getBool());

					}else if(array_ref.elementTypeID == TypeManager::getTypeChar()){
						builder += std::format(";'{}'", evo::printCharName(array_ref.terminator->getChar()));
						
					}else{
						builder += ";<TERMINATOR>";
					}
				}

				builder += ']';

				return builder;
			} break;

			case BaseType::Kind::ALIAS: {
				const BaseType::Alias::ID alias_id = base_type_id.aliasID();
				const BaseType::Alias& alias = this->getAlias(alias_id);

				return std::string(alias.getName(source_manager));
			} break;

			case BaseType::Kind::DISTINCT_ALIAS: {
				const BaseType::DistinctAlias::ID distinct_alias_id = base_type_id.distinctAliasID();
				const BaseType::DistinctAlias& distinct_alias_info = this->getDistinctAlias(distinct_alias_id);

				return std::string(
					source_manager[distinct_alias_info.sourceID]
						.getTokenBuffer()[distinct_alias_info.identTokenID]
						.getString()
				);
			} break;

			case BaseType::Kind::STRUCT: {
				const BaseType::Struct::ID struct_id = base_type_id.structID();
				const BaseType::Struct& struct_info = this->getStruct(struct_id);

				const std::string_view struct_name = struct_info.getName(source_manager);

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
						builder += evo::boolStr(template_arg.as<core::GenericValue>().getBool());

					}else if(*struct_template.params[i].typeID == TypeManager::getTypeChar()){
						builder += "'";
						builder += template_arg.as<core::GenericValue>().getChar();
						builder += "'";

					}else if(this->isUnsignedIntegral(*struct_template.params[i].typeID)){
						builder += template_arg.as<core::GenericValue>().getInt(
							unsigned(this->numBits(*struct_template.params[i].typeID))
						).toString(false);

					}else if(this->isIntegral(*struct_template.params[i].typeID)){
						builder += template_arg.as<core::GenericValue>().getInt(
							unsigned(this->numBits(*struct_template.params[i].typeID))
						).toString(true);

					}else if(this->isFloatingPoint(*struct_template.params[i].typeID)){
						const BaseType::Primitive& primitive = this->getPrimitive(
							this->getTypeInfo(*struct_template.params[i].typeID).baseTypeID().primitiveID()
						);

						const core::GenericValue& generic_value = template_arg.as<core::GenericValue>();

						switch(primitive.kind()){
							break; case Token::Kind::TYPE_F16:  builder += generic_value.getF16().toString();
							break; case Token::Kind::TYPE_BF16: builder += generic_value.getBF16().toString();
							break; case Token::Kind::TYPE_F32:  builder += generic_value.getF32().toString();
							break; case Token::Kind::TYPE_F64:  builder += generic_value.getF64().toString();
							break; case Token::Kind::TYPE_F80:  builder += generic_value.getF80().toString();
							break; case Token::Kind::TYPE_F128: builder += generic_value.getF128().toString();
							break; default: evo::debugFatalBreak("Unknown float type");
						}
						
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
				const BaseType::StructTemplate::ID struct_template_id = base_type_id.structTemplateID();
				const BaseType::StructTemplate& struct_template_info = this->getStructTemplate(struct_template_id);

				const TokenBuffer& token_buffer = source_manager[struct_template_info.sourceID].getTokenBuffer();
				return std::string(token_buffer[struct_template_info.identTokenID].getString());
			} break;

			case BaseType::Kind::UNION: {
				const BaseType::Union::ID union_id = base_type_id.unionID();
				const BaseType::Union& union_info = this->getUnion(union_id);

				return std::string(union_info.getName(source_manager));
			} break;

			case BaseType::Kind::ENUM: {
				const BaseType::Enum::ID enum_id = base_type_id.enumID();
				const BaseType::Enum& enum_info = this->getEnum(enum_id);

				return std::string(enum_info.getName(source_manager));
			} break;

			case BaseType::Kind::TYPE_DEDUCER: {
				const BaseType::TypeDeducer::ID type_deducer_id = base_type_id.typeDeducerID();
				const BaseType::TypeDeducer& type_deducer = this->getTypeDeducer(type_deducer_id);

				const Token& token =
					source_manager[type_deducer.sourceID].getTokenBuffer()[type_deducer.identTokenID];

				if(token.kind() == Token::Kind::TYPE_DEDUCER){
					return std::format("${}", token.getString());
				}else{
					evo::debugAssert(
						token.kind() == Token::Kind::ANONYMOUS_TYPE_DEDUCER, "Unknown type deducer kind"
					);
					return "$$";
				}
			} break;

			case BaseType::Kind::INTERFACE: {
				const BaseType::Interface::ID interface_id = base_type_id.interfaceID();
				const BaseType::Interface& interface_info = this->getInterface(interface_id);

				const TokenBuffer& token_buffer = source_manager[interface_info.sourceID].getTokenBuffer();
				return std::string(token_buffer[interface_info.identTokenID].getString());
			} break;

			case BaseType::Kind::INTERFACE_IMPL_INSTANTIATION: {
				const BaseType::InterfaceImplInstantiation& interface_impl_instantiation_info =
					this->getInterfaceImplInstantiation(base_type_id.interfaceImplInstantiationID());
				
				return std::format(
					"{}({})",
					this->printType(BaseType::ID(interface_impl_instantiation_info.interfacedID), source_manager),
					this->printType(interface_impl_instantiation_info.implInstantiationTypeID, source_manager)
				);
			} break;
		}

		evo::debugFatalBreak("Unknown or unsuport base-type kind");
	}



	//////////////////////////////////////////////////////////////////////
	// function

	auto TypeManager::getFunction(BaseType::Function::ID id) const -> const BaseType::Function& {
		const auto lock = std::scoped_lock(this->functions_lock);
		return this->functions[id];
	}

	auto TypeManager::getFunction(BaseType::Function::ID id) -> BaseType::Function& {
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
	// array ref

	auto TypeManager::getArrayRef(BaseType::ArrayRef::ID id) const -> const BaseType::ArrayRef& {
		const auto lock = std::scoped_lock(this->array_refs_lock);
		return this->array_refs[id];
	}

	auto TypeManager::getOrCreateArrayRef(BaseType::ArrayRef&& lookup_func) -> BaseType::ID {
		const auto lock = std::scoped_lock(this->array_refs_lock);

		for(uint32_t i = 0; i < this->array_refs.size(); i+=1){
			if(this->array_refs[BaseType::ArrayRef::ID(i)] == lookup_func){
				return BaseType::ID(BaseType::Kind::ARRAY_REF, i);
			}
		}

		const BaseType::ArrayRef::ID new_array_ref = this->array_refs.emplace_back(lookup_func);
		return BaseType::ID(BaseType::Kind::ARRAY_REF, new_array_ref.get());
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
			lookup_type.sourceID, lookup_type.name, lookup_type.aliasedType.load(), lookup_type.isPub
		);
		return BaseType::ID(BaseType::Kind::ALIAS, new_alias.get());
	}


	//////////////////////////////////////////////////////////////////////
	// distinct aliases

	auto TypeManager::getDistinctAlias(BaseType::DistinctAlias::ID id) const -> const BaseType::DistinctAlias& {
		const auto lock = std::scoped_lock(this->distinct_aliases_lock);
		return this->distinct_aliases[id];
	}

	auto TypeManager::getDistinctAlias(BaseType::DistinctAlias::ID id) -> BaseType::DistinctAlias& {
		const auto lock = std::scoped_lock(this->distinct_aliases_lock);
		return this->distinct_aliases[id];
	}


	auto TypeManager::getOrCreateDistinctAlias(BaseType::DistinctAlias&& lookup_type) -> BaseType::ID {
		const auto lock = std::scoped_lock(this->distinct_aliases_lock);

		for(uint32_t i = 0; i < this->distinct_aliases.size(); i+=1){
			if(this->distinct_aliases[BaseType::DistinctAlias::ID(i)] == lookup_type){
				return BaseType::ID(BaseType::Kind::DISTINCT_ALIAS, i);
			}
		}

		const BaseType::DistinctAlias::ID new_distinct_alias = this->distinct_aliases.emplace_back(
			lookup_type.sourceID, lookup_type.identTokenID, lookup_type.underlyingType.load(), lookup_type.isPub
		);
		return BaseType::ID(BaseType::Kind::DISTINCT_ALIAS, new_distinct_alias.get());
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


	auto TypeManager::getOrCreateStruct(BaseType::Struct&& lookup_type) -> BaseType::ID {
		const auto lock = std::scoped_lock(this->structs_lock);

		for(uint32_t i = 0; i < this->structs.size(); i+=1){
			if(this->structs[BaseType::Struct::ID(i)] == lookup_type){
				return BaseType::ID(BaseType::Kind::STRUCT, i);
			}
		}

		const BaseType::Struct::ID new_struct = this->structs.emplace_back(
			lookup_type.sourceID,
			lookup_type.name,
			lookup_type.templateID,
			lookup_type.instantiation,
			std::move(lookup_type.memberVars),
			std::move(lookup_type.memberVarsABI),
			lookup_type.namespacedMembers,
			lookup_type.scopeLevel,
			lookup_type.isPub,
			lookup_type.isOrdered,
			lookup_type.isPacked,
			lookup_type.shouldLower
		);
		return BaseType::ID(BaseType::Kind::STRUCT, new_struct.get());
	}


	auto TypeManager::getNumStructs() const -> size_t {
		const auto lock = std::scoped_lock(this->structs_lock);
		return this->structs.size();
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
	// union

	auto TypeManager::getUnion(BaseType::Union::ID id) const -> const BaseType::Union& {
		const auto lock = std::scoped_lock(this->unions_lock);
		return this->unions[id];
	}

	auto TypeManager::getUnion(BaseType::Union::ID id) -> BaseType::Union& {
		const auto lock = std::scoped_lock(this->unions_lock);
		return this->unions[id];
	}


	auto TypeManager::getOrCreateUnion(BaseType::Union&& lookup_type) -> BaseType::ID {
		const auto lock = std::scoped_lock(this->unions_lock);

		for(uint32_t i = 0; i < this->unions.size(); i+=1){
			if(this->unions[BaseType::Union::ID(i)] == lookup_type){
				return BaseType::ID(BaseType::Kind::UNION, i);
			}
		}

		const BaseType::Union::ID new_union = this->unions.emplace_back(
			lookup_type.sourceID,
			lookup_type.location,
			std::move(lookup_type.fields),
			lookup_type.namespacedMembers,
			lookup_type.scopeLevel,
			lookup_type.isPub,
			lookup_type.isUntagged
		);
		return BaseType::ID(BaseType::Kind::UNION, new_union.get());
	}


	auto TypeManager::getNumUnions() const -> size_t {
		const auto lock = std::scoped_lock(this->unions_lock);
		return this->unions.size();
	}


	//////////////////////////////////////////////////////////////////////
	// enum

	auto TypeManager::getEnum(BaseType::Enum::ID id) const -> const BaseType::Enum& {
		const auto lock = std::scoped_lock(this->enums_lock);
		return this->enums[id];
	}

	auto TypeManager::getEnum(BaseType::Enum::ID id) -> BaseType::Enum& {
		const auto lock = std::scoped_lock(this->enums_lock);
		return this->enums[id];
	}


	auto TypeManager::getOrCreateEnum(BaseType::Enum&& lookup_type) -> BaseType::ID {
		const auto lock = std::scoped_lock(this->enums_lock);

		for(uint32_t i = 0; i < this->enums.size(); i+=1){
			if(this->enums[BaseType::Enum::ID(i)] == lookup_type){
				return BaseType::ID(BaseType::Kind::UNION, i);
			}
		}

		const BaseType::Enum::ID new_enum = this->enums.emplace_back(
			lookup_type.sourceID,
			lookup_type.location,
			std::move(lookup_type.enumerators),
			lookup_type.underlyingTypeID,
			lookup_type.namespacedMembers,
			lookup_type.scopeLevel,
			lookup_type.isPub
		);
		return BaseType::ID(BaseType::Kind::ENUM, new_enum.get());
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
	// interfaces

	auto TypeManager::getInterface(BaseType::Interface::ID id) const -> const BaseType::Interface& {
		const auto lock = std::scoped_lock(this->interfaces_lock);
		return this->interfaces[id];
	}

	auto TypeManager::getInterface(BaseType::Interface::ID id) -> BaseType::Interface& {
		const auto lock = std::scoped_lock(this->interfaces_lock);
		return this->interfaces[id];
	}


	auto TypeManager::getOrCreateInterface(BaseType::Interface&& lookup_type) -> BaseType::ID {
		const auto lock = std::scoped_lock(this->interfaces_lock);

		for(uint32_t i = 0; i < this->interfaces.size(); i+=1){
			if(this->interfaces[BaseType::Interface::ID(i)] == lookup_type){
				return BaseType::ID(BaseType::Kind::INTERFACE, i);
			}
		}

		const BaseType::Interface::ID new_interface = this->interfaces.emplace_back(
			lookup_type.identTokenID,
			lookup_type.sourceID,
			lookup_type.symbolProcID,
			lookup_type.isPub
		);

		return BaseType::ID(BaseType::Kind::INTERFACE, new_interface.get());
	}


	auto TypeManager::getNumInterfaces() const -> size_t {
		const auto lock = std::scoped_lock(this->interfaces_lock);
		return this->interfaces.size();
	}


	auto TypeManager::createInterfaceImpl() -> BaseType::Interface::Impl& {
		return this->interface_impls[this->interface_impls.emplace_back()];
	}



	//////////////////////////////////////////////////////////////////////
	// interface impl instantiation

	auto TypeManager::getInterfaceImplInstantiation(BaseType::InterfaceImplInstantiation::ID id) const
	-> const BaseType::InterfaceImplInstantiation& {
		const auto lock = std::scoped_lock(this->interface_impl_instantiations_lock);
		return this->interface_impl_instantiations[id];
	}


	auto TypeManager::getOrCreateInterfaceImplInstantiation(BaseType::InterfaceImplInstantiation&& lookup_type)
	-> BaseType::ID {
		const auto lock = std::scoped_lock(this->interface_impl_instantiations_lock);

		for(uint32_t i = 0; i < this->interface_impl_instantiations.size(); i+=1){
			if(this->interface_impl_instantiations[BaseType::InterfaceImplInstantiation::ID(i)] == lookup_type){
				return BaseType::ID(BaseType::Kind::INTERFACE_IMPL_INSTANTIATION, i);
			}
		}

		const BaseType::InterfaceImplInstantiation::ID new_instantiation = 
			this->interface_impl_instantiations.emplace_back(std::move(lookup_type));
		return BaseType::ID(BaseType::Kind::INTERFACE_IMPL_INSTANTIATION, new_instantiation.get());
	}




	//////////////////////////////////////////////////////////////////////
	// misc

	auto TypeManager::isTypeDeducer(TypeInfo::ID id) const -> bool {
		const TypeInfo& type_info = this->getTypeInfo(id);

		switch(type_info.baseTypeID().kind()){
			case BaseType::Kind::ARRAY: {
				const BaseType::Array& array_type = this->getArray(type_info.baseTypeID().arrayID());
				return this->isTypeDeducer(array_type.elementTypeID);
			} break;

			case BaseType::Kind::ARRAY_REF: {
				const BaseType::ArrayRef& array_ref_type = this->getArrayRef(type_info.baseTypeID().arrayRefID());
				return this->isTypeDeducer(array_ref_type.elementTypeID);
			} break;

			case BaseType::Kind::TYPE_DEDUCER: {
				return true;
			} break;

			default: {
				return false;
			} break;
		}
	}




	//////////////////////////////////////////////////////////////////////
	// type traits

	// https://stackoverflow.com/a/1766566
	EVO_NODISCARD static constexpr auto ceil_to_multiple(size_t num, size_t multiple) -> size_t {
		return (num + (multiple - 1)) & ~(multiple - 1);
	}


	EVO_NODISCARD static constexpr auto add_padding_bytes_if_needed(size_t size, bool include_padding) -> size_t {
		if(include_padding == false){ return size; }

		switch(size){
			case 1: return 1;
			case 2: return 2;
			case 3: case 4: return 4;
			default: return ceil_to_multiple(size, 8);
		}
	}



	auto TypeManager::numBytes(TypeInfo::ID id, bool include_padding) const -> size_t {
		const TypeInfo& type_info = this->getTypeInfo(id);
		if(type_info.qualifiers().empty()){ return this->numBytes(type_info.baseTypeID()); }


		if(type_info.qualifiers().back().isOptional){
			if(type_info.qualifiers().back().isPtr){
				return this->numBytesOfPtr();
			}else{
				return add_padding_bytes_if_needed(this->numBytes(type_info.baseTypeID(), false) + 1, include_padding);
			}
			
		}else if(type_info.baseTypeID().kind() == BaseType::Kind::INTERFACE){
			return this->numBytesOfPtr() * 2;
		}else{
			return this->numBytesOfPtr();
		}
	}


	auto TypeManager::numBytes(BaseType::ID id, bool include_padding) const -> uint64_t {
		switch(id.kind()){
			case BaseType::Kind::DUMMY: evo::debugFatalBreak("Dummy type should not be used");

			case BaseType::Kind::PRIMITIVE: {
				const BaseType::Primitive& primitive = this->getPrimitive(id.primitiveID());

				switch(primitive.kind()){
					case Token::Kind::TYPE_INT: case Token::Kind::TYPE_UINT:
						return this->numBytesOfGeneralRegister();

					case Token::Kind::TYPE_ISIZE: case Token::Kind::TYPE_USIZE:
						return this->numBytesOfPtr();

					case Token::Kind::TYPE_I_N: case Token::Kind::TYPE_UI_N: {
						if(include_padding){
							if(primitive.bitWidth() <= 8){
								return 1;

							}else if(primitive.bitWidth() <= 16){
								return 2;

							}else if(primitive.bitWidth() <= 32){
								return 4;

							}else{
								return ceil_to_multiple(primitive.bitWidth(), 64) / 8;
							}
						}else{
							return ceil_to_multiple(primitive.bitWidth(), 8) / 8;
						}
					}

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
					case Token::Kind::TYPE_C_WCHAR:
						return this->getTarget().platform == core::Target::Platform::WINDOWS ? 2 : 4;

					case Token::Kind::TYPE_C_SHORT: case Token::Kind::TYPE_C_USHORT:
					    return 2;

					case Token::Kind::TYPE_C_INT: case Token::Kind::TYPE_C_UINT:
						return 4;

					case Token::Kind::TYPE_C_LONG: case Token::Kind::TYPE_C_ULONG:
						return this->getTarget().platform == core::Target::Platform::WINDOWS ? 4 : 8;

					case Token::Kind::TYPE_C_LONG_LONG: case Token::Kind::TYPE_C_ULONG_LONG:
						return 8;

					case Token::Kind::TYPE_C_LONG_DOUBLE:
						return this->getTarget().platform == core::Target::Platform::WINDOWS ? 8 : 16;

					default: evo::debugFatalBreak("Unknown or unsupported built-in type");
				}
			} break;

			case BaseType::Kind::FUNCTION: {
				return this->numBytesOfPtr();
			} break;

			case BaseType::Kind::ARRAY: {
				const BaseType::Array& array = this->getArray(id.arrayID());
				const uint64_t elem_size = this->numBytes(array.elementTypeID);

				if(array.terminator.has_value()){ return elem_size * array.dimensions.back() + 1; }

				uint64_t output = elem_size;
				for(uint64_t dimension : array.dimensions){
					output *= dimension;
				}
				return add_padding_bytes_if_needed(output, include_padding);
			} break;

			case BaseType::Kind::ARRAY_REF: {
				const BaseType::ArrayRef& arr_ref = this->getArrayRef(id.arrayRefID());

				unsigned num_words = 1;
				for(const BaseType::ArrayRef::Dimension& dimension : arr_ref.dimensions){
					if(dimension.isPtr()){ num_words += 1; }
				}

				return this->numBytesOfPtr() * num_words;
			} break;

			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias = this->getAlias(id.aliasID());
				evo::debugAssert(alias.aliasedType.load().has_value(), "Definition of alias was not completed");
				return this->numBytes(*alias.aliasedType.load(), include_padding);
			} break;

			case BaseType::Kind::DISTINCT_ALIAS: {
				const BaseType::DistinctAlias& type_def = this->getDistinctAlias(id.distinctAliasID());
				evo::debugAssert(
					type_def.underlyingType.load().has_value(), "Definition of distinct_alias was not completed"
				);
				return this->numBytes(*type_def.underlyingType.load(), include_padding);
			} break;

			case BaseType::Kind::STRUCT: {
				const BaseType::Struct& struct_info = this->getStruct(id.structID());
				
				size_t total = 0;

				for(const BaseType::Struct::MemberVar& member_var : struct_info.memberVars){
					total += this->numBytes(member_var.typeID, struct_info.isPacked);
				}

				return add_padding_bytes_if_needed(std::max(total, size_t(1)), include_padding);
			} break;

			case BaseType::Kind::STRUCT_TEMPLATE: {
				// TODO(FUTURE): handle this better?
				evo::debugFatalBreak("Cannot get size of Struct Template");
			} break;

			case BaseType::Kind::UNION: {
				const BaseType::Union& union_info = this->getUnion(id.unionID());


				size_t largest_size = [&]() -> size_t {
					if(union_info.fields[0].typeID.isVoid()){
						return 0;
					}else{
						return this->numBytes(union_info.fields[0].typeID.asTypeID(), include_padding);
					}
				}();

				for(size_t i = 1; i < union_info.fields.size(); i+=1){
					if(union_info.fields[i].typeID.isVoid()){ continue; }
					largest_size = 
						std::max(largest_size, this->numBytes(union_info.fields[i].typeID.asTypeID(), include_padding));
				}

				return add_padding_bytes_if_needed(std::max(largest_size, size_t(1)), include_padding);
			} break;

			case BaseType::Kind::ENUM: {
				const BaseType::Enum& enum_info = this->getEnum(id.enumID());
				return this->numBytes(BaseType::ID(enum_info.underlyingTypeID), include_padding);
			} break;

			case BaseType::Kind::TYPE_DEDUCER: {
				// TODO(FUTURE): handle this better?
				evo::debugFatalBreak("Cannot get size of type deducer");
			} break;

			case BaseType::Kind::INTERFACE: {
				// TODO(FUTURE): handle this better?
				evo::debugFatalBreak("Cannot get size of type deducer");
			} break;

			case BaseType::Kind::INTERFACE_IMPL_INSTANTIATION: {
				const BaseType::InterfaceImplInstantiation& interface_impl_instantiation_info =
					this->getInterfaceImplInstantiation(id.interfaceImplInstantiationID());
				
				return this->numBytes(interface_impl_instantiation_info.implInstantiationTypeID, true);
			} break;
		}

		evo::debugFatalBreak("Unknown or unsupported base-type kind");
	}

	auto TypeManager::numBytesOfPtr() const -> uint64_t { return 8; }
	auto TypeManager::numBytesOfGeneralRegister() const -> uint64_t { return 8; }


	///////////////////////////////////
	// numBits

	EVO_NODISCARD static constexpr auto add_padding_bits_if_needed(size_t size, bool include_padding) -> size_t {
		if(include_padding == false){ return size; }

		if(size <= 8){ return 8; }
		if(size <= 16){ return 16; }
		if(size <= 32){ return 32; }
		return ceil_to_multiple(size, 64);
	}

	auto TypeManager::numBits(TypeInfo::ID id, bool include_padding) const -> size_t {
		const TypeInfo& type_info = this->getTypeInfo(id);
		if(type_info.qualifiers().empty()){ return this->numBits(type_info.baseTypeID()); }

		if(type_info.qualifiers().back().isOptional){
			if(type_info.qualifiers().back().isPtr){
				return this->numBitsOfPtr();
			}else{
				return add_padding_bits_if_needed(
					this->numBytes(type_info.baseTypeID(), false) * 8 + 1, include_padding
				);
			}
			
		}else if(type_info.baseTypeID().kind() == BaseType::Kind::INTERFACE){
			return this->numBitsOfPtr() * 2;
		}else{
			return this->numBitsOfPtr();
		}
	}


	auto TypeManager::numBits(BaseType::ID id, bool include_padding) const -> uint64_t {
		switch(id.kind()){
			case BaseType::Kind::DUMMY: evo::debugFatalBreak("Dummy type should not be used");

			case BaseType::Kind::PRIMITIVE: {
				const BaseType::Primitive& primitive = this->getPrimitive(id.primitiveID());

				switch(primitive.kind()){
					case Token::Kind::TYPE_INT: case Token::Kind::TYPE_UINT:
						return this->numBitsOfGeneralRegister();

					case Token::Kind::TYPE_ISIZE: case Token::Kind::TYPE_USIZE:
						return this->numBitsOfPtr();

					case Token::Kind::TYPE_I_N: case Token::Kind::TYPE_UI_N:
						return add_padding_bits_if_needed(primitive.bitWidth(), include_padding);

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
					case Token::Kind::TYPE_C_WCHAR:
						return this->getTarget().platform == core::Target::Platform::WINDOWS ? 16 : 32;

					case Token::Kind::TYPE_C_SHORT: case Token::Kind::TYPE_C_USHORT:
					    return 16;

					case Token::Kind::TYPE_C_INT: case Token::Kind::TYPE_C_UINT:
						return 32;

					case Token::Kind::TYPE_C_LONG: case Token::Kind::TYPE_C_ULONG:
						return this->getTarget().platform == core::Target::Platform::WINDOWS ? 32 : 64;

					case Token::Kind::TYPE_C_LONG_LONG: case Token::Kind::TYPE_C_ULONG_LONG:
						return 64;

					case Token::Kind::TYPE_C_LONG_DOUBLE: {
						if(this->getTarget().platform == core::Target::Platform::WINDOWS){
							return 64;
						}
						
						return this->getTarget().architecture == core::Target::Architecture::X86_64 ? 80 : 128;
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

			case BaseType::Kind::ARRAY_REF: {
				const BaseType::ArrayRef& arr_ref = this->getArrayRef(id.arrayRefID());

				unsigned num_words = 1;
				for(const BaseType::ArrayRef::Dimension& dimension : arr_ref.dimensions){
					if(dimension.isPtr()){ num_words += 1; }
				}

				return this->numBitsOfPtr() * num_words;
			} break;

			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias = this->getAlias(id.aliasID());
				evo::debugAssert(alias.aliasedType.load().has_value(), "Definition of alias was not completed");
				return this->numBits(*alias.aliasedType.load(), include_padding);
			} break;

			case BaseType::Kind::DISTINCT_ALIAS: {
				const BaseType::DistinctAlias& type_def = this->getDistinctAlias(id.distinctAliasID());
				evo::debugAssert(
					type_def.underlyingType.load().has_value(), "Definition of distinct_alias was not completed"
				);
				return this->numBits(*type_def.underlyingType.load(), include_padding);
			} break;

			case BaseType::Kind::STRUCT: {
				return this->numBytes(id, include_padding) * 8;
			} break;

			case BaseType::Kind::STRUCT_TEMPLATE: {
				// TODO(FUTURE): handle this better?
				evo::debugAssert("Cannot get size of Struct Template");
			} break;

			case BaseType::Kind::UNION: {
				const BaseType::Union& union_info = this->getUnion(id.unionID());


				size_t largest_size = [&]() -> size_t {
					if(union_info.fields[0].typeID.isVoid()){
						return 0;
					}else{
						return this->numBits(union_info.fields[0].typeID.asTypeID(), include_padding);
					}
				}();

				for(size_t i = 1; i < union_info.fields.size(); i+=1){
					if(union_info.fields[i].typeID.isVoid()){ continue; }
					largest_size =
						std::max(largest_size, this->numBits(union_info.fields[i].typeID.asTypeID(), include_padding));
				}

				return std::max(largest_size, size_t(1));
			} break;

			case BaseType::Kind::ENUM: {
				const BaseType::Enum& enum_info = this->getEnum(id.enumID());
				return this->numBits(BaseType::ID(enum_info.underlyingTypeID), include_padding);
			} break;

			case BaseType::Kind::TYPE_DEDUCER: {
				// TODO(FUTURE): handle this better?
				evo::debugAssert("Cannot get size of type deducer");
			} break;

			case BaseType::Kind::INTERFACE: {
				// TODO(FUTURE): handle this better?
				evo::debugAssert("Cannot get size of type deducer");
			} break;

			case BaseType::Kind::INTERFACE_IMPL_INSTANTIATION: {
				const BaseType::InterfaceImplInstantiation& interface_impl_instantiation_info =
					this->getInterfaceImplInstantiation(id.interfaceImplInstantiationID());
				
				return this->numBits(interface_impl_instantiation_info.implInstantiationTypeID, include_padding);
			} break;
		}

		evo::debugFatalBreak("Unknown or unsupported base-type kind");
	}



	auto TypeManager::numBitsOfPtr() const -> uint64_t { return 64; }
	auto TypeManager::numBitsOfGeneralRegister() const -> uint64_t { return 64; }



	///////////////////////////////////
	// isTriviallySized

	auto TypeManager::isTriviallySized(TypeInfo::ID id) const -> bool {
		return this->numBytes(id) <= this->numBytesOfPtr();
	}

	auto TypeManager::isTriviallySized(BaseType::ID id) const -> bool {
		return this->numBytes(id) <= this->numBytesOfPtr();
	}





	///////////////////////////////////
	// isDefaultInitializable

	auto TypeManager::isDefaultInitializable(TypeInfo::ID id) const -> bool {
		const TypeInfo& type_info = this->getTypeInfo(id);

		if(type_info.qualifiers().empty()){ return this->isDefaultInitializable(type_info.baseTypeID()); }
		if(type_info.qualifiers().back().isOptional){ return true; }
		return false;
	}

	auto TypeManager::isDefaultInitializable(BaseType::ID id) const -> bool {
		switch(id.kind()){
			case BaseType::Kind::DUMMY: {
				evo::debugFatalBreak("Invalid type");
			} break;

			case BaseType::Kind::PRIMITIVE: {
				const BaseType::Primitive& primitive = this->getPrimitive(id.primitiveID());

				if(primitive.kind() == Token::Kind::TYPE_RAWPTR || primitive.kind() == Token::Kind::TYPE_TYPEID){
					return false;
				}

				return true;
			} break;

			case BaseType::Kind::FUNCTION: {
				return false;
			} break;

			case BaseType::Kind::ARRAY: {
				const BaseType::Array& array = this->getArray(id.arrayID());
				return this->isDefaultInitializable(array.elementTypeID);
			} break;

			case BaseType::Kind::ARRAY_REF: {
				return true;
			} break;

			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias = this->getAlias(id.aliasID());
				return this->isDefaultInitializable(*alias.aliasedType.load());
			} break;

			case BaseType::Kind::DISTINCT_ALIAS: {
				const BaseType::DistinctAlias& alias = this->getDistinctAlias(id.distinctAliasID());
				return this->isDefaultInitializable(*alias.underlyingType.load());
			} break;

			case BaseType::Kind::STRUCT: {
				const BaseType::Struct& struct_info = this->getStruct(id.structID());
				return struct_info.isDefaultInitializable;
			} break;

			case BaseType::Kind::UNION: {
				const BaseType::Union& union_info = this->getUnion(id.unionID());

				for(const BaseType::Union::Field& field : union_info.fields){
					if(field.typeID.isVoid()){ continue; }
					// Yes, this is the correct method
					if(this->isTriviallyDefaultInitializable(field.typeID.asTypeID()) == false){ return false; }
				}

				return true;
			} break;

			case BaseType::Kind::ENUM: {
				return false;
			} break;

			case BaseType::Kind::STRUCT_TEMPLATE: case BaseType::Kind::TYPE_DEDUCER: case BaseType::Kind::INTERFACE: {
				evo::debugFatalBreak("Invalid to check with this type");
			} break;

			case BaseType::Kind::INTERFACE_IMPL_INSTANTIATION: {
				const BaseType::InterfaceImplInstantiation& interface_impl_instantiation_info =
					this->getInterfaceImplInstantiation(id.interfaceImplInstantiationID());
				return this->isDefaultInitializable(interface_impl_instantiation_info.implInstantiationTypeID);
			} break;
		}
		evo::debugFatalBreak("Unkonwn or unsupported BaseType");
	}


	///////////////////////////////////
	// isNoErrorDefaultInitializable

	auto TypeManager::isNoErrorDefaultInitializable(TypeInfo::ID id) const -> bool {
		const TypeInfo& type_info = this->getTypeInfo(id);

		if(type_info.qualifiers().empty()){
			return this->isNoErrorDefaultInitializable(type_info.baseTypeID());
		}

		if(type_info.qualifiers().back().isOptional){ return true; }
		return false;
	}

	auto TypeManager::isNoErrorDefaultInitializable(BaseType::ID id) const -> bool {
		switch(id.kind()){
			case BaseType::Kind::DUMMY: {
				evo::debugFatalBreak("Invalid type");
			} break;

			case BaseType::Kind::PRIMITIVE: {
				const BaseType::Primitive& primitive = this->getPrimitive(id.primitiveID());

				if(primitive.kind() == Token::Kind::TYPE_RAWPTR || primitive.kind() == Token::Kind::TYPE_TYPEID){
					return false;
				}

				return true;
			} break;

			case BaseType::Kind::FUNCTION: {
				return false;
			} break;

			case BaseType::Kind::ARRAY: {
				const BaseType::Array& array = this->getArray(id.arrayID());
				return this->isNoErrorDefaultInitializable(array.elementTypeID);
			} break;

			case BaseType::Kind::ARRAY_REF: {
				return true;
			} break;

			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias = this->getAlias(id.aliasID());
				return this->isNoErrorDefaultInitializable(*alias.aliasedType.load());
			} break;

			case BaseType::Kind::DISTINCT_ALIAS: {
				const BaseType::DistinctAlias& alias = this->getDistinctAlias(id.distinctAliasID());
				return this->isNoErrorDefaultInitializable(*alias.underlyingType.load());
			} break;

			case BaseType::Kind::STRUCT: {
				const BaseType::Struct& struct_info = this->getStruct(id.structID());
				return struct_info.isNoErrorDefaultInitializable;
			} break;

			case BaseType::Kind::UNION: {
				const BaseType::Union& union_info = this->getUnion(id.unionID());

				for(const BaseType::Union::Field& field : union_info.fields){
					if(field.typeID.isVoid()){ continue; }
					// Yes, this is the correct method
					if(this->isTriviallyDefaultInitializable(field.typeID.asTypeID()) == false){ return false; }
				}

				return true;
			} break;

			case BaseType::Kind::ENUM: {
				return false;
			} break;

			case BaseType::Kind::STRUCT_TEMPLATE: case BaseType::Kind::TYPE_DEDUCER: case BaseType::Kind::INTERFACE: {
				evo::debugFatalBreak("Invalid to check with this type");
			} break;

			case BaseType::Kind::INTERFACE_IMPL_INSTANTIATION: {
				const BaseType::InterfaceImplInstantiation& interface_impl_instantiation_info =
					this->getInterfaceImplInstantiation(id.interfaceImplInstantiationID());
				return this->isNoErrorDefaultInitializable(interface_impl_instantiation_info.implInstantiationTypeID);
			} break;
		}
		evo::debugFatalBreak("Unkonwn or unsupported BaseType");
	}




	///////////////////////////////////
	// isConstexprDefaultInitializable

	auto TypeManager::isConstexprDefaultInitializable(TypeInfo::ID id) const -> bool {
		const TypeInfo& type_info = this->getTypeInfo(id);

		if(type_info.qualifiers().empty()){
			return this->isConstexprDefaultInitializable(type_info.baseTypeID());
		}

		if(type_info.qualifiers().back().isOptional){ return true; }
		return false;
	}

	auto TypeManager::isConstexprDefaultInitializable(BaseType::ID id) const -> bool {
		switch(id.kind()){
			case BaseType::Kind::DUMMY: {
				evo::debugFatalBreak("Invalid type");
			} break;

			case BaseType::Kind::PRIMITIVE: {
				const BaseType::Primitive& primitive = this->getPrimitive(id.primitiveID());

				if(primitive.kind() == Token::Kind::TYPE_RAWPTR || primitive.kind() == Token::Kind::TYPE_TYPEID){
					return false;
				}

				return true;
			} break;

			case BaseType::Kind::FUNCTION: {
				return false;
			} break;

			case BaseType::Kind::ARRAY: {
				const BaseType::Array& array = this->getArray(id.arrayID());
				return this->isConstexprDefaultInitializable(array.elementTypeID);
			} break;

			case BaseType::Kind::ARRAY_REF: {
				return true;
			} break;

			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias = this->getAlias(id.aliasID());
				return this->isConstexprDefaultInitializable(*alias.aliasedType.load());
			} break;

			case BaseType::Kind::DISTINCT_ALIAS: {
				const BaseType::DistinctAlias& alias = this->getDistinctAlias(id.distinctAliasID());
				return this->isConstexprDefaultInitializable(*alias.underlyingType.load());
			} break;

			case BaseType::Kind::STRUCT: {
				const BaseType::Struct& struct_info = this->getStruct(id.structID());
				return struct_info.isConstexprDefaultInitializable;
			} break;

			case BaseType::Kind::UNION: {
				const BaseType::Union& union_info = this->getUnion(id.unionID());

				for(const BaseType::Union::Field& field : union_info.fields){
					if(field.typeID.isVoid()){ continue; }
					// Yes, this is the correct method
					if(this->isTriviallyDefaultInitializable(field.typeID.asTypeID()) == false){ return false; }
				}

				return true;
			} break;

			case BaseType::Kind::ENUM: {
				return false;
			} break;

			case BaseType::Kind::STRUCT_TEMPLATE: case BaseType::Kind::TYPE_DEDUCER: case BaseType::Kind::INTERFACE: {
				evo::debugFatalBreak("Invalid to check with this type");
			} break;

			case BaseType::Kind::INTERFACE_IMPL_INSTANTIATION: {
				const BaseType::InterfaceImplInstantiation& interface_impl_instantiation_info =
					this->getInterfaceImplInstantiation(id.interfaceImplInstantiationID());
				return this->isConstexprDefaultInitializable(interface_impl_instantiation_info.implInstantiationTypeID);
			} break;
		}
		evo::debugFatalBreak("Unkonwn or unsupported BaseType");
	}




	///////////////////////////////////
	// isTriviallyDefaultInitializable

	auto TypeManager::isTriviallyDefaultInitializable(TypeInfo::ID id) const -> bool {
		const TypeInfo& type_info = this->getTypeInfo(id);

		if(type_info.qualifiers().empty() == false){ return false; }
		return this->isTriviallyDefaultInitializable(type_info.baseTypeID());
	}

	auto TypeManager::isTriviallyDefaultInitializable(BaseType::ID id) const -> bool {
		switch(id.kind()){
			case BaseType::Kind::DUMMY: {
				evo::debugFatalBreak("Invalid type");
			} break;

			case BaseType::Kind::PRIMITIVE: {
				const BaseType::Primitive& primitive = this->getPrimitive(id.primitiveID());

				if(primitive.kind() == Token::Kind::TYPE_RAWPTR || primitive.kind() == Token::Kind::TYPE_TYPEID){
					return false;
				}

				return true;
			} break;

			case BaseType::Kind::FUNCTION: {
				return false;
			} break;

			case BaseType::Kind::ARRAY: {
				const BaseType::Array& array = this->getArray(id.arrayID());
				return this->isTriviallyDefaultInitializable(array.elementTypeID);
			} break;

			case BaseType::Kind::ARRAY_REF: {
				return false;
			} break;

			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias = this->getAlias(id.aliasID());
				return this->isTriviallyDefaultInitializable(*alias.aliasedType.load());
			} break;

			case BaseType::Kind::DISTINCT_ALIAS: {
				const BaseType::DistinctAlias& alias = this->getDistinctAlias(id.distinctAliasID());
				return this->isTriviallyDefaultInitializable(*alias.underlyingType.load());
			} break;

			case BaseType::Kind::STRUCT: {
				const BaseType::Struct& struct_info = this->getStruct(id.structID());
				return struct_info.isTriviallyDefaultInitializable;
			} break;

			case BaseType::Kind::UNION: {
				const BaseType::Union& union_info = this->getUnion(id.unionID());

				for(const BaseType::Union::Field& field : union_info.fields){
					if(field.typeID.isVoid()){ continue; }
					if(this->isTriviallyDefaultInitializable(field.typeID.asTypeID()) == false){ return false; }
				}

				return true;
			} break;

			case BaseType::Kind::ENUM: {
				return false;
			} break;

			case BaseType::Kind::STRUCT_TEMPLATE: case BaseType::Kind::TYPE_DEDUCER: case BaseType::Kind::INTERFACE: {
				evo::debugFatalBreak("Invalid to check with this type");
			} break;

			case BaseType::Kind::INTERFACE_IMPL_INSTANTIATION: {
				const BaseType::InterfaceImplInstantiation& interface_impl_instantiation_info =
					this->getInterfaceImplInstantiation(id.interfaceImplInstantiationID());
				return this->isTriviallyDefaultInitializable(interface_impl_instantiation_info.implInstantiationTypeID);
			} break;
		}
		evo::debugFatalBreak("Unkonwn or unsupported BaseType");
	}


	///////////////////////////////////
	// isTriviallyDeletable

	auto TypeManager::isTriviallyDeletable(TypeInfo::ID id) const -> bool {
		const TypeInfo& type_info = this->getTypeInfo(id);

		for(const AST::Type::Qualifier& qualifier : type_info.qualifiers()){
			if(qualifier.isPtr){ return true; }
		}

		return this->isTriviallyDeletable(type_info.baseTypeID());
	}

	auto TypeManager::isTriviallyDeletable(BaseType::ID id) const -> bool {
		switch(id.kind()){
			case BaseType::Kind::DUMMY: {
				evo::debugFatalBreak("Invalid type");
			} break;

			case BaseType::Kind::PRIMITIVE: {
				return true;
			} break;

			case BaseType::Kind::FUNCTION: {
				return true;
			} break;

			case BaseType::Kind::ARRAY: {
				const BaseType::Array& array = this->getArray(id.arrayID());
				return this->isTriviallyDeletable(array.elementTypeID);
			} break;

			case BaseType::Kind::ARRAY_REF: {
				return true;
			} break;

			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias = this->getAlias(id.aliasID());
				return this->isTriviallyDeletable(*alias.aliasedType.load());
			} break;

			case BaseType::Kind::DISTINCT_ALIAS: {
				const BaseType::DistinctAlias& alias = this->getDistinctAlias(id.distinctAliasID());
				return this->isTriviallyDeletable(*alias.underlyingType.load());
			} break;

			case BaseType::Kind::STRUCT: {
				const BaseType::Struct& struct_info = this->getStruct(id.structID());
				return struct_info.deleteOverload.load().has_value() == false;
			} break;

			case BaseType::Kind::UNION: {
				const BaseType::Union& union_info = this->getUnion(id.unionID());

				if(union_info.isUntagged){ return true; }

				for(const BaseType::Union::Field& field : union_info.fields){
					if(field.typeID.isVoid()){ continue; }
					if(this->isTriviallyDeletable(field.typeID.asTypeID()) == false){ return false; }
				}

				return true;
			} break;

			case BaseType::Kind::ENUM: {
				return true;
			} break;

			case BaseType::Kind::STRUCT_TEMPLATE: case BaseType::Kind::TYPE_DEDUCER: case BaseType::Kind::INTERFACE: {
				evo::debugFatalBreak("Invalid to check with this type");
			} break;

			case BaseType::Kind::INTERFACE_IMPL_INSTANTIATION: {
				const BaseType::InterfaceImplInstantiation& interface_impl_instantiation_info =
					this->getInterfaceImplInstantiation(id.interfaceImplInstantiationID());
				return this->isTriviallyDeletable(interface_impl_instantiation_info.implInstantiationTypeID);
			} break;
		}
		evo::debugFatalBreak("Unkonwn or unsupported BaseType");
	}


	///////////////////////////////////
	// isConstexprDeletable

	auto TypeManager::isConstexprDeletable(TypeInfo::ID id, const SemaBuffer& sema_buffer) const -> bool {
		const TypeInfo& type_info = this->getTypeInfo(id);

		for(const AST::Type::Qualifier& qualifier : type_info.qualifiers()){
			if(qualifier.isPtr){ return true; }
		}

		return this->isConstexprDeletable(type_info.baseTypeID(), sema_buffer);
	}

	auto TypeManager::isConstexprDeletable(BaseType::ID id, const SemaBuffer& sema_buffer) const -> bool {
		switch(id.kind()){
			case BaseType::Kind::DUMMY: {
				evo::debugFatalBreak("Invalid type");
			} break;

			case BaseType::Kind::PRIMITIVE: {
				return true;
			} break;

			case BaseType::Kind::FUNCTION: {
				return true;
			} break;

			case BaseType::Kind::ARRAY: {
				const BaseType::Array& array = this->getArray(id.arrayID());
				return this->isConstexprDeletable(array.elementTypeID, sema_buffer);
			} break;

			case BaseType::Kind::ARRAY_REF: {
				return true;
			} break;

			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias = this->getAlias(id.aliasID());
				return this->isConstexprDeletable(*alias.aliasedType.load(), sema_buffer);
			} break;

			case BaseType::Kind::DISTINCT_ALIAS: {
				const BaseType::DistinctAlias& alias = this->getDistinctAlias(id.distinctAliasID());
				return this->isConstexprDeletable(*alias.underlyingType.load(), sema_buffer);
			} break;

			case BaseType::Kind::STRUCT: {
				const BaseType::Struct& struct_info = this->getStruct(id.structID());

				const std::optional<sema::Func::ID> delete_overload = struct_info.deleteOverload.load();
				if(delete_overload.has_value() == false){ return true; }
				return sema_buffer.getFunc(*delete_overload).isConstexpr;
			} break;

			case BaseType::Kind::UNION: {
				const BaseType::Union& union_info = this->getUnion(id.unionID());

				if(union_info.isUntagged){ return true; }

				for(const BaseType::Union::Field& field : union_info.fields){
					if(field.typeID.isVoid()){ continue; }
					if(this->isConstexprDeletable(field.typeID.asTypeID(), sema_buffer) == false){ return false; }
				}

				return true;
			} break;

			case BaseType::Kind::ENUM: {
				return true;
			} break;

			case BaseType::Kind::STRUCT_TEMPLATE: case BaseType::Kind::TYPE_DEDUCER: case BaseType::Kind::INTERFACE: {
				evo::debugFatalBreak("Invalid to check with this type");
			} break;

			case BaseType::Kind::INTERFACE_IMPL_INSTANTIATION: {
				const BaseType::InterfaceImplInstantiation& interface_impl_instantiation_info =
					this->getInterfaceImplInstantiation(id.interfaceImplInstantiationID());

				return this->isConstexprDeletable(
					interface_impl_instantiation_info.implInstantiationTypeID, sema_buffer
				);
			} break;
		}
		evo::debugFatalBreak("Unkonwn or unsupported BaseType");
	}


	///////////////////////////////////
	// isCopyable

	auto TypeManager::isCopyable(TypeInfo::ID id) const -> bool {
		const TypeInfo& type_info = this->getTypeInfo(id);

		for(const AST::Type::Qualifier& qualifier : type_info.qualifiers()){
			if(qualifier.isPtr){ return true; }
		}

		return this->isCopyable(type_info.baseTypeID());
	}

	auto TypeManager::isCopyable(BaseType::ID id) const -> bool {
		switch(id.kind()){
			case BaseType::Kind::DUMMY: {
				evo::debugFatalBreak("Invalid type");
			} break;

			case BaseType::Kind::PRIMITIVE: {
				return true;
			} break;

			case BaseType::Kind::FUNCTION: {
				return true;
			} break;

			case BaseType::Kind::ARRAY: {
				const BaseType::Array& array = this->getArray(id.arrayID());
				return this->isCopyable(array.elementTypeID);
			} break;

			case BaseType::Kind::ARRAY_REF: {
				return true;
			} break;

			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias = this->getAlias(id.aliasID());
				return this->isCopyable(*alias.aliasedType.load());
			} break;

			case BaseType::Kind::DISTINCT_ALIAS: {
				const BaseType::DistinctAlias& alias = this->getDistinctAlias(id.distinctAliasID());
				return this->isCopyable(*alias.underlyingType.load());
			} break;

			case BaseType::Kind::STRUCT: {
				const BaseType::Struct& struct_info = this->getStruct(id.structID());

				return !struct_info.copyInitOverload.load().wasDeleted;
			} break;

			case BaseType::Kind::UNION: {
				const BaseType::Union& union_info = this->getUnion(id.unionID());

				if(union_info.isUntagged){ return true; }

				for(const BaseType::Union::Field& field : union_info.fields){
					if(field.typeID.isVoid()){ continue; }
					if(this->isCopyable(field.typeID.asTypeID()) == false){ return false; }
				}

				return true;
			} break;

			case BaseType::Kind::ENUM: {
				return true;
			} break;

			case BaseType::Kind::STRUCT_TEMPLATE: case BaseType::Kind::TYPE_DEDUCER: case BaseType::Kind::INTERFACE: {
				evo::debugFatalBreak("Invalid to check with this type");
			} break;

			case BaseType::Kind::INTERFACE_IMPL_INSTANTIATION: {
				const BaseType::InterfaceImplInstantiation& interface_impl_instantiation_info =
					this->getInterfaceImplInstantiation(id.interfaceImplInstantiationID());
				return this->isCopyable(interface_impl_instantiation_info.implInstantiationTypeID);
			} break;
		}
		evo::debugFatalBreak("Unkonwn or unsupported BaseType");
	}


	///////////////////////////////////
	// isTriviallyCopyable

	auto TypeManager::isTriviallyCopyable(TypeInfo::ID id) const -> bool {
		const TypeInfo& type_info = this->getTypeInfo(id);

		for(const AST::Type::Qualifier& qualifier : type_info.qualifiers()){
			if(qualifier.isPtr){ return true; }
		}

		return this->isTriviallyCopyable(type_info.baseTypeID());
	}

	auto TypeManager::isTriviallyCopyable(BaseType::ID id) const -> bool {
		switch(id.kind()){
			case BaseType::Kind::DUMMY: {
				evo::debugFatalBreak("Invalid type");
			} break;

			case BaseType::Kind::PRIMITIVE: {
				return true;
			} break;

			case BaseType::Kind::FUNCTION: {
				return true;
			} break;

			case BaseType::Kind::ARRAY: {
				const BaseType::Array& array = this->getArray(id.arrayID());
				return this->isTriviallyCopyable(array.elementTypeID);
			} break;

			case BaseType::Kind::ARRAY_REF: {
				return true;
			} break;

			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias = this->getAlias(id.aliasID());
				return this->isTriviallyCopyable(*alias.aliasedType.load());
			} break;

			case BaseType::Kind::DISTINCT_ALIAS: {
				const BaseType::DistinctAlias& alias = this->getDistinctAlias(id.distinctAliasID());
				return this->isTriviallyCopyable(*alias.underlyingType.load());
			} break;

			case BaseType::Kind::STRUCT: {
				const BaseType::Struct& struct_info = this->getStruct(id.structID());

				const BaseType::Struct::DeletableOverload delete_overload = struct_info.copyInitOverload.load();
				return delete_overload.wasDeleted == false && delete_overload.funcID.has_value() == false;
			} break;

			case BaseType::Kind::UNION: {
				const BaseType::Union& union_info = this->getUnion(id.unionID());

				if(union_info.isUntagged){ return true; }

				for(const BaseType::Union::Field& field : union_info.fields){
					if(field.typeID.isVoid()){ continue; }
					if(this->isTriviallyCopyable(field.typeID.asTypeID()) == false){ return false; }
				}

				return true;
			} break;

			case BaseType::Kind::ENUM: {
				return true;
			} break;

			case BaseType::Kind::STRUCT_TEMPLATE: case BaseType::Kind::TYPE_DEDUCER: case BaseType::Kind::INTERFACE: {
				evo::debugFatalBreak("Invalid to check with this type");
			} break;

			case BaseType::Kind::INTERFACE_IMPL_INSTANTIATION: {
				const BaseType::InterfaceImplInstantiation& interface_impl_instantiation_info =
					this->getInterfaceImplInstantiation(id.interfaceImplInstantiationID());
				return this->isTriviallyCopyable(interface_impl_instantiation_info.implInstantiationTypeID);
			} break;
		}
		evo::debugFatalBreak("Unkonwn or unsupported BaseType");
	}


	///////////////////////////////////
	// isConstexprCopyable

	auto TypeManager::isConstexprCopyable(TypeInfo::ID id, const SemaBuffer& sema_buffer) const -> bool {
		const TypeInfo& type_info = this->getTypeInfo(id);

		for(const AST::Type::Qualifier& qualifier : type_info.qualifiers()){
			if(qualifier.isPtr){ return true; }
		}

		return this->isConstexprCopyable(type_info.baseTypeID(), sema_buffer);
	}

	auto TypeManager::isConstexprCopyable(BaseType::ID id, const SemaBuffer& sema_buffer) const -> bool {
		switch(id.kind()){
			case BaseType::Kind::DUMMY: {
				evo::debugFatalBreak("Invalid type");
			} break;

			case BaseType::Kind::PRIMITIVE: {
				return true;
			} break;

			case BaseType::Kind::FUNCTION: {
				return true;
			} break;

			case BaseType::Kind::ARRAY: {
				const BaseType::Array& array = this->getArray(id.arrayID());
				return this->isConstexprCopyable(array.elementTypeID, sema_buffer);
			} break;

			case BaseType::Kind::ARRAY_REF: {
				return true;
			} break;

			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias = this->getAlias(id.aliasID());
				return this->isConstexprCopyable(*alias.aliasedType.load(), sema_buffer);
			} break;

			case BaseType::Kind::DISTINCT_ALIAS: {
				const BaseType::DistinctAlias& alias = this->getDistinctAlias(id.distinctAliasID());
				return this->isConstexprCopyable(*alias.underlyingType.load(), sema_buffer);
			} break;

			case BaseType::Kind::STRUCT: {
				const BaseType::Struct& struct_info = this->getStruct(id.structID());

				const BaseType::Struct::DeletableOverload copy_overload = struct_info.copyInitOverload.load();
				if(copy_overload.wasDeleted){ return false; }
				if(copy_overload.funcID.has_value() == false){ return true; }
				return sema_buffer.getFunc(*copy_overload.funcID).isConstexpr;
			} break;

			case BaseType::Kind::UNION: {
				const BaseType::Union& union_info = this->getUnion(id.unionID());

				if(union_info.isUntagged){ return true; }

				for(const BaseType::Union::Field& field : union_info.fields){
					if(field.typeID.isVoid()){ continue; }
					if(this->isConstexprCopyable(field.typeID.asTypeID(), sema_buffer) == false){ return false; }
				}

				return true;
			} break;

			case BaseType::Kind::ENUM: {
				return true;
			} break;

			case BaseType::Kind::STRUCT_TEMPLATE: case BaseType::Kind::TYPE_DEDUCER: case BaseType::Kind::INTERFACE: {
				evo::debugFatalBreak("Invalid to check with this type");
			} break;

			case BaseType::Kind::INTERFACE_IMPL_INSTANTIATION: {
				const BaseType::InterfaceImplInstantiation& interface_impl_instantiation_info =
					this->getInterfaceImplInstantiation(id.interfaceImplInstantiationID());

				return this->isConstexprCopyable(
					interface_impl_instantiation_info.implInstantiationTypeID, sema_buffer
				);
			} break;
		}
		evo::debugFatalBreak("Unkonwn or unsupported BaseType");
	}


	///////////////////////////////////
	// isMovable

	auto TypeManager::isMovable(TypeInfo::ID id) const -> bool {
		const TypeInfo& type_info = this->getTypeInfo(id);

		for(const AST::Type::Qualifier& qualifier : type_info.qualifiers()){
			if(qualifier.isPtr){ return true; }
		}

		return this->isMovable(type_info.baseTypeID());
	}

	auto TypeManager::isMovable(BaseType::ID id) const -> bool {
		switch(id.kind()){
			case BaseType::Kind::DUMMY: {
				evo::debugFatalBreak("Invalid type");
			} break;

			case BaseType::Kind::PRIMITIVE: {
				return true;
			} break;

			case BaseType::Kind::FUNCTION: {
				return true;
			} break;

			case BaseType::Kind::ARRAY: {
				const BaseType::Array& array = this->getArray(id.arrayID());
				return this->isMovable(array.elementTypeID);
			} break;

			case BaseType::Kind::ARRAY_REF: {
				return true;
			} break;

			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias = this->getAlias(id.aliasID());
				return this->isMovable(*alias.aliasedType.load());
			} break;

			case BaseType::Kind::DISTINCT_ALIAS: {
				const BaseType::DistinctAlias& alias = this->getDistinctAlias(id.distinctAliasID());
				return this->isMovable(*alias.underlyingType.load());
			} break;

			case BaseType::Kind::STRUCT: {
				const BaseType::Struct& struct_info = this->getStruct(id.structID());

				return !struct_info.moveInitOverload.load().wasDeleted;
			} break;

			case BaseType::Kind::UNION: {
				const BaseType::Union& union_info = this->getUnion(id.unionID());

				if(union_info.isUntagged){ return true; }

				for(const BaseType::Union::Field& field : union_info.fields){
					if(field.typeID.isVoid()){ continue; }
					if(this->isMovable(field.typeID.asTypeID()) == false){ return false; }
				}

				return true;
			} break;

			case BaseType::Kind::ENUM: {
				return true;
			} break;

			case BaseType::Kind::STRUCT_TEMPLATE: case BaseType::Kind::TYPE_DEDUCER: case BaseType::Kind::INTERFACE: {
				evo::debugFatalBreak("Invalid to check with this type");
			} break;

			case BaseType::Kind::INTERFACE_IMPL_INSTANTIATION: {
				const BaseType::InterfaceImplInstantiation& interface_impl_instantiation_info =
					this->getInterfaceImplInstantiation(id.interfaceImplInstantiationID());
				return this->isMovable(interface_impl_instantiation_info.implInstantiationTypeID);
			} break;
		}
		evo::debugFatalBreak("Unkonwn or unsupported BaseType");
	}


	///////////////////////////////////
	// isTriviallyMovable

	auto TypeManager::isTriviallyMovable(TypeInfo::ID id) const -> bool {
		const TypeInfo& type_info = this->getTypeInfo(id);

		for(const AST::Type::Qualifier& qualifier : type_info.qualifiers()){
			if(qualifier.isPtr){ return true; }
		}

		return this->isTriviallyMovable(type_info.baseTypeID());
	}

	auto TypeManager::isTriviallyMovable(BaseType::ID id) const -> bool {
		switch(id.kind()){
			case BaseType::Kind::DUMMY: {
				evo::debugFatalBreak("Invalid type");
			} break;

			case BaseType::Kind::PRIMITIVE: {
				return true;
			} break;

			case BaseType::Kind::FUNCTION: {
				return true;
			} break;

			case BaseType::Kind::ARRAY: {
				const BaseType::Array& array = this->getArray(id.arrayID());
				return this->isTriviallyMovable(array.elementTypeID);
			} break;

			case BaseType::Kind::ARRAY_REF: {
				return true;
			} break;

			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias = this->getAlias(id.aliasID());
				return this->isTriviallyMovable(*alias.aliasedType.load());
			} break;

			case BaseType::Kind::DISTINCT_ALIAS: {
				const BaseType::DistinctAlias& alias = this->getDistinctAlias(id.distinctAliasID());
				return this->isTriviallyMovable(*alias.underlyingType.load());
			} break;

			case BaseType::Kind::STRUCT: {
				const BaseType::Struct& struct_info = this->getStruct(id.structID());

				const BaseType::Struct::DeletableOverload delete_overload = struct_info.moveInitOverload.load();
				return delete_overload.wasDeleted == false && delete_overload.funcID.has_value() == false;
			} break;

			case BaseType::Kind::UNION: {
				const BaseType::Union& union_info = this->getUnion(id.unionID());

				if(union_info.isUntagged){ return true; }

				for(const BaseType::Union::Field& field : union_info.fields){
					if(field.typeID.isVoid()){ continue; }
					if(this->isTriviallyMovable(field.typeID.asTypeID()) == false){ return false; }
				}

				return true;
			} break;

			case BaseType::Kind::ENUM: {
				return true;
			} break;

			case BaseType::Kind::STRUCT_TEMPLATE: case BaseType::Kind::TYPE_DEDUCER: case BaseType::Kind::INTERFACE: {
				evo::debugFatalBreak("Invalid to check with this type");
			} break;

			case BaseType::Kind::INTERFACE_IMPL_INSTANTIATION: {
				const BaseType::InterfaceImplInstantiation& interface_impl_instantiation_info =
					this->getInterfaceImplInstantiation(id.interfaceImplInstantiationID());
				return this->isTriviallyMovable(interface_impl_instantiation_info.implInstantiationTypeID);
			} break;
		}
		evo::debugFatalBreak("Unkonwn or unsupported BaseType");
	}


	///////////////////////////////////
	// isConstexprMovable

	auto TypeManager::isConstexprMovable(TypeInfo::ID id, const SemaBuffer& sema_buffer) const -> bool {
		const TypeInfo& type_info = this->getTypeInfo(id);

		for(const AST::Type::Qualifier& qualifier : type_info.qualifiers()){
			if(qualifier.isPtr){ return true; }
		}

		return this->isConstexprMovable(type_info.baseTypeID(), sema_buffer);
	}

	auto TypeManager::isConstexprMovable(BaseType::ID id, const SemaBuffer& sema_buffer) const -> bool {
		switch(id.kind()){
			case BaseType::Kind::DUMMY: {
				evo::debugFatalBreak("Invalid type");
			} break;

			case BaseType::Kind::PRIMITIVE: {
				return true;
			} break;

			case BaseType::Kind::FUNCTION: {
				return true;
			} break;

			case BaseType::Kind::ARRAY: {
				const BaseType::Array& array = this->getArray(id.arrayID());
				return this->isConstexprMovable(array.elementTypeID, sema_buffer);
			} break;

			case BaseType::Kind::ARRAY_REF: {
				return true;
			} break;

			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias = this->getAlias(id.aliasID());
				return this->isConstexprMovable(*alias.aliasedType.load(), sema_buffer);
			} break;

			case BaseType::Kind::DISTINCT_ALIAS: {
				const BaseType::DistinctAlias& alias = this->getDistinctAlias(id.distinctAliasID());
				return this->isConstexprMovable(*alias.underlyingType.load(), sema_buffer);
			} break;

			case BaseType::Kind::STRUCT: {
				const BaseType::Struct& struct_info = this->getStruct(id.structID());

				const BaseType::Struct::DeletableOverload move_overload = struct_info.moveInitOverload.load();
				if(move_overload.wasDeleted){ return false; }
				if(move_overload.funcID.has_value() == false){ return true; }
				return sema_buffer.getFunc(*move_overload.funcID).isConstexpr;
			} break;

			case BaseType::Kind::UNION: {
				const BaseType::Union& union_info = this->getUnion(id.unionID());

				if(union_info.isUntagged){ return true; }

				for(const BaseType::Union::Field& field : union_info.fields){
					if(field.typeID.isVoid()){ continue; }
					if(this->isConstexprMovable(field.typeID.asTypeID(), sema_buffer) == false){ return false; }
				}

				return true;
			} break;

			case BaseType::Kind::ENUM: {
				return true;
			} break;

			case BaseType::Kind::STRUCT_TEMPLATE: case BaseType::Kind::TYPE_DEDUCER: case BaseType::Kind::INTERFACE: {
				evo::debugFatalBreak("Invalid to check with this type");
			} break;

			case BaseType::Kind::INTERFACE_IMPL_INSTANTIATION: {
				const BaseType::InterfaceImplInstantiation& interface_impl_instantiation_info =
					this->getInterfaceImplInstantiation(id.interfaceImplInstantiationID());

				return this->isConstexprMovable(interface_impl_instantiation_info.implInstantiationTypeID, sema_buffer);
			} break;
		}
		evo::debugFatalBreak("Unkonwn or unsupported BaseType");
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
			case Token::Kind::TYPE_C_WCHAR:       return false;
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
			case Token::Kind::TYPE_C_WCHAR:       return false;
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
			case Token::Kind::TYPE_C_WCHAR:       return false;
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
			case Token::Kind::TYPE_C_WCHAR:       return false;
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
		
		if(type_info.qualifiers().back().isPtr){
			if(type_info.baseTypeID().kind() == BaseType::Kind::INTERFACE){
				return id;

			}else{
				return TypeManager::getTypeRawPtr();
			}
		}

		return id;
	}

	// TODO(PERF): optimize this function
	auto TypeManager::getUnderlyingType(BaseType::ID id) -> TypeInfo::ID {
		switch(id.kind()){
			case BaseType::Kind::DUMMY:     evo::debugFatalBreak("Dummy type should not be used");
			case BaseType::Kind::PRIMITIVE: break;
			case BaseType::Kind::FUNCTION:  return this->getOrCreateTypeInfo(TypeInfo(id));
			case BaseType::Kind::ARRAY:     return this->getOrCreateTypeInfo(TypeInfo(id));
			case BaseType::Kind::ARRAY_REF: return this->getOrCreateTypeInfo(TypeInfo(id));
			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias = this->getAlias(id.aliasID());
				evo::debugAssert(alias.aliasedType.load().has_value(), "Definition of alias was not completed");
				return this->getUnderlyingType(*alias.aliasedType.load());
			} break;
			case BaseType::Kind::DISTINCT_ALIAS: {
				const BaseType::DistinctAlias& distinct_alias_info = this->getDistinctAlias(id.distinctAliasID());
				evo::debugAssert(
					distinct_alias_info.underlyingType.load().has_value(),
					"Definition of distinct_alias was not completed"
				);
				return this->getUnderlyingType(*distinct_alias_info.underlyingType.load());
			} break;
			case BaseType::Kind::STRUCT:          return this->getOrCreateTypeInfo(TypeInfo(id));
			case BaseType::Kind::STRUCT_TEMPLATE: evo::debugFatalBreak("Cannot get underlying type of this kind");
			case BaseType::Kind::UNION:           return this->getOrCreateTypeInfo(TypeInfo(id));
			case BaseType::Kind::ENUM: {
				const BaseType::Enum& enum_type = this->getEnum(id.enumID());
				return this->getUnderlyingType(BaseType::ID(enum_type.underlyingTypeID));
			} break;
			case BaseType::Kind::TYPE_DEDUCER:    evo::debugFatalBreak("Cannot get underlying type of this kind");
			case BaseType::Kind::INTERFACE:       evo::debugFatalBreak("Cannot get underlying type of this kind");

			case BaseType::Kind::INTERFACE_IMPL_INSTANTIATION: {
				const BaseType::InterfaceImplInstantiation& interface_impl_instantiation_info =
					this->getInterfaceImplInstantiation(id.interfaceImplInstantiationID());
				return this->getUnderlyingType(interface_impl_instantiation_info.implInstantiationTypeID);
			} break;
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
					TypeInfo(this->getOrCreatePrimitiveBaseType(
						Token::Kind::TYPE_I_N, uint32_t(this->numBytesOfPtr()) * 8
					))
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

			case Token::Kind::TYPE_C_WCHAR: {
				if(this->getTarget().platform == core::Target::Platform::WINDOWS){
					return this->getOrCreateTypeInfo(
						TypeInfo(this->getOrCreatePrimitiveBaseType(Token::Kind::TYPE_UI_N, 16))
					);
				}else{
					return this->getOrCreateTypeInfo(
						TypeInfo(this->getOrCreatePrimitiveBaseType(Token::Kind::TYPE_UI_N, 32))
					);
				}
			} break;

			case Token::Kind::TYPE_C_SHORT: case Token::Kind::TYPE_C_INT: {
				return this->getOrCreateTypeInfo(
					TypeInfo(this->getOrCreatePrimitiveBaseType(Token::Kind::TYPE_I_N, 32))
				);
			} break;

			case Token::Kind::TYPE_C_LONG: case Token::Kind::TYPE_C_LONG_LONG: {
				const uint32_t size = this->getTarget().platform == core::Target::Platform::WINDOWS ? 32 : 64;
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
				const uint32_t size = this->getTarget().platform == core::Target::Platform::WINDOWS ? 32 : 64;
				return this->getOrCreateTypeInfo(
					TypeInfo(this->getOrCreatePrimitiveBaseType(Token::Kind::TYPE_UI_N, size))
				);
			} break;

			case Token::Kind::TYPE_C_LONG_DOUBLE: {
				if(this->getTarget().platform == core::Target::Platform::WINDOWS){
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
			case Token::Kind::TYPE_INT: return core::GenericValue(calc_min_signed(this->numBytesOfGeneralRegister()*8));
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

			case Token::Kind::TYPE_C_WCHAR:
				return core::GenericValue(
					core::GenericInt(this->getTarget().platform == core::Target::Platform::WINDOWS ? 16 : 32, 0)
				);

			case Token::Kind::TYPE_C_SHORT: return core::GenericValue(calc_min_signed(16));
			case Token::Kind::TYPE_C_INT:   return core::GenericValue(calc_min_signed(32));
				
			case Token::Kind::TYPE_C_LONG:
				return core::GenericValue(
					calc_min_signed(this->getTarget().platform == core::Target::Platform::WINDOWS ? 32 : 64)
				);

			case Token::Kind::TYPE_C_LONG_LONG: return core::GenericValue(calc_min_signed(32));
			case Token::Kind::TYPE_C_USHORT:   return core::GenericValue(core::GenericInt(16, 0));
			case Token::Kind::TYPE_C_UINT:     return core::GenericValue(core::GenericInt(32, 0));

			case Token::Kind::TYPE_C_ULONG:     
				return core::GenericValue(
					core::GenericInt(this->getTarget().platform == core::Target::Platform::WINDOWS ? 32 : 64, 0)
				);

			case Token::Kind::TYPE_C_ULONG_LONG: return core::GenericValue(core::GenericInt(64, 0));

			case Token::Kind::TYPE_C_LONG_DOUBLE: {
				if(this->getTarget().platform == core::Target::Platform::WINDOWS){
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
			case Token::Kind::TYPE_INT: return core::GenericValue(calc_min_signed(this->numBytesOfGeneralRegister()*8));
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

			case Token::Kind::TYPE_C_WCHAR:
				return core::GenericValue(
					core::GenericInt(this->getTarget().platform == core::Target::Platform::WINDOWS ? 16 : 32, 0)
				);

			case Token::Kind::TYPE_C_SHORT: return core::GenericValue(calc_min_signed(16));
			case Token::Kind::TYPE_C_INT:   return core::GenericValue(calc_min_signed(32));
				
			case Token::Kind::TYPE_C_LONG:
				return core::GenericValue(
					calc_min_signed(this->getTarget().platform == core::Target::Platform::WINDOWS ? 32 : 64)
				);

			case Token::Kind::TYPE_C_LONG_LONG: return core::GenericValue(calc_min_signed(32));
			case Token::Kind::TYPE_C_USHORT:   return core::GenericValue(core::GenericInt(16, 0));
			case Token::Kind::TYPE_C_UINT:     return core::GenericValue(core::GenericInt(32, 0));

			case Token::Kind::TYPE_C_ULONG:     
				return core::GenericValue(
					core::GenericInt(this->getTarget().platform == core::Target::Platform::WINDOWS ? 32 : 64, 0)
				);

			case Token::Kind::TYPE_C_ULONG_LONG: return core::GenericValue(core::GenericInt(64, 0));

			case Token::Kind::TYPE_C_LONG_DOUBLE: {
				if(this->getTarget().platform == core::Target::Platform::WINDOWS){
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
			case Token::Kind::TYPE_INT: return core::GenericValue(calc_max_signed(this->numBytesOfGeneralRegister()*8));
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

			case Token::Kind::TYPE_C_WCHAR:
				return core::GenericValue(
					calc_max_unsigned(this->getTarget().platform == core::Target::Platform::WINDOWS ? 16 : 32)
				);

			case Token::Kind::TYPE_C_SHORT: return core::GenericValue(calc_max_signed(16));
			case Token::Kind::TYPE_C_INT:   return core::GenericValue(calc_max_signed(32));

			case Token::Kind::TYPE_C_LONG:      
				return core::GenericValue(
					calc_max_signed(this->getTarget().platform == core::Target::Platform::WINDOWS ? 32 : 64)
				);

			case Token::Kind::TYPE_C_LONG_LONG:  return core::GenericValue(calc_max_signed(32));
			case Token::Kind::TYPE_C_USHORT:    return core::GenericValue(calc_max_unsigned(16));
			case Token::Kind::TYPE_C_UINT:      return core::GenericValue(calc_max_unsigned(32));

			case Token::Kind::TYPE_C_ULONG:
				return core::GenericValue(
					calc_max_unsigned(this->getTarget().platform == core::Target::Platform::WINDOWS ? 32 : 64)
				);
				
			case Token::Kind::TYPE_C_ULONG_LONG: return core::GenericValue(calc_max_unsigned(64));

			case Token::Kind::TYPE_C_LONG_DOUBLE: {
				if(this->getTarget().platform == core::Target::Platform::WINDOWS){
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