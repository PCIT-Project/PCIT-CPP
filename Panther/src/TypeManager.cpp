////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/TypeManager.h"

#include <ranges>

#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

#include "../include/source/SourceManager.h"
#include "../include/Context.h"
#include "../include/sema/SemaBuffer.h"


namespace pcit::panther{

	//////////////////////////////////////////////////////////////////////
	// base type

	auto BaseType::Function::isImplicitRVO(const TypeManager& type_manager) const -> bool {
		if(this->returnsVoid()){ return false; }
		if(this->hasNamedReturns){ return false; }

		const TypeInfo::ID ret_type_id = this->returnTypes[0].asTypeID();

		return type_manager.isTriviallySized(ret_type_id) == false
			|| type_manager.isTriviallyCopyable(ret_type_id) == false
			|| type_manager.isTriviallyMovable(ret_type_id) == false;
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

	auto BaseType::DistinctAlias::getName(const SourceManager& source_manager) const -> std::string_view {
		if(this->isPTHRSourceType()){
			const Source& source = source_manager[this->sourceID.as<Source::ID>()];
			return source.getTokenBuffer()[this->name.as<Token::ID>()].getString();

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



	auto BaseType::StructTemplate::createOrLookupInstantiation(evo::SmallVector<Arg>&& args) -> InstantiationInfo {
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
			return clang_source.getDeclInfo(this->name.as<ClangSource::DeclInfoID>()).name;

		}else if(this->isPTHRSourceType()){
			const Source& source = source_manager[this->sourceID.as<Source::ID>()];
			return source.getTokenBuffer()[this->name.as<Token::ID>()].getString();

		}else{
			const BuiltinModule& builtin_module = source_manager[this->sourceID.as<BuiltinModule::ID>()];
			return builtin_module.getString(this->name.as<BuiltinModule::StringID>());
		}
	}


	auto BaseType::Enum::getEnumeratorName(const Enumerator& enumerator, const SourceManager& source_manager) const
	-> std::string_view {
		if(this->isClangType()){
			const ClangSource& clang_source = source_manager[this->sourceID.as<ClangSource::ID>()];
			return clang_source.getDeclInfo(enumerator.name.as<ClangSource::DeclInfoID>()).name;

		}else if(this->isPTHRSourceType()){
			const Source& source = source_manager[this->sourceID.as<Source::ID>()];
			return source.getTokenBuffer()[enumerator.name.as<Token::ID>()].getString();

		}else{
			const BuiltinModule& builtin_module = source_manager[this->sourceID.as<BuiltinModule::ID>()];
			return builtin_module.getString(enumerator.name.as<BuiltinModule::StringID>());
		}
	}


	auto BaseType::Interface::getName(const SourceManager& source_manager) const -> std::string_view {
		if(this->sourceID.is<SourceID>()){
			const Source& source = source_manager[this->sourceID.as<Source::ID>()];
			return source.getTokenBuffer()[this->name.as<Token::ID>()].getString();

		}else{
			const BuiltinModule& builtin_module = source_manager[this->sourceID.as<BuiltinModule::ID>()];
			return builtin_module.getString(this->name.as<BuiltinModule::StringID>());
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
		const BaseType::Primitive::ID type_f80     = this->primitives.emplace_back(Token::Kind::TYPE_F80);
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
		this->types.emplace_back(TypeInfo(BaseType::ID(BaseType::Kind::PRIMITIVE, type_f80.get())));
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


	auto TypeManager::printType(TypeInfo::VoidableID type_info_id, const Context& context) const 
	-> std::string {
		if(type_info_id.isVoid()) [[unlikely]] {
			return "Void";
		}else{
			return this->printType(type_info_id.asTypeID(), context);
		}
	}

	auto TypeManager::printType(TypeInfo::ID type_info_id, const Context& context) const -> std::string {
		const TypeInfo& type_info = this->getTypeInfo(type_info_id);

		std::string type_str = this->printType(type_info.baseTypeID(), context);

		bool is_first_qualifier = type_str.back() != '*'
			&& type_str.ends_with("*mut")
			&& type_str.back() != '!'
			&& type_str.back() != '?';

		for(const TypeInfo::Qualifier& qualifier : type_info.qualifiers()){
			if(type_info.qualifiers().size() > 1){
				if(is_first_qualifier){
					is_first_qualifier = false;
				}else{
					type_str += ' ';
				}
			}

			if(qualifier.isPtr){ type_str += '*'; }
			if(qualifier.isMut){ type_str += "mut"; }
			if(qualifier.isUninit){ type_str += '!'; }
			if(qualifier.isOptional){ type_str += '?'; }
		}

		return type_str;
	}


	auto TypeManager::printType(BaseType::ID base_type_id, const Context& context) const -> std::string {
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

				builder += this->printType(array.elementTypeID, context);

				builder += ':';

				for(size_t i = 0; uint64_t dimension : array.dimensions){
					builder += std::to_string(dimension);

					if(i + 1 < array.dimensions.size()){ builder += ","; }

					i += 1;
				}

				if(array.terminator.has_value()){
					if(this->isUnsignedIntegral(array.elementTypeID)){
						builder += ';';
						builder += array.terminator->getInt(
							unsigned(this->numBits(array.elementTypeID))
						).toString(false);

					}else if(this->isSignedIntegral(array.elementTypeID)){
						builder += ';';
						builder += array.terminator->getInt(
							unsigned(this->numBits(array.elementTypeID))
						).toString(true);

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

			case BaseType::Kind::ARRAY_DEDUCER: {
				const BaseType::ArrayDeducer& array_deducer = this->getArrayDeducer(base_type_id.arrayDeducerID());

				auto builder = std::string();
				builder += '[';

				builder += this->printType(array_deducer.elementTypeID, context);

				builder += ':';

				for(size_t i = 0; const BaseType::ArrayDeducer::Dimension& dimension : array_deducer.dimensions){
					if(dimension.is<uint64_t>()){
						builder += std::to_string(dimension.as<uint64_t>());
						
					}else{
						const Token& token = context.getSourceManager()[array_deducer.sourceID]
							.getTokenBuffer()[dimension.as<Token::ID>()];

						if(token.kind() == Token::Kind::DEDUCER){
							builder += std::format("${}", token.getString());
						}else{
							evo::debugAssert(
								token.kind() == Token::Kind::ANONYMOUS_DEDUCER, "Unknown type deducer kind"
							);
							builder += "$$";
						}
					}

					if(i + 1 < array_deducer.dimensions.size()){ builder += ","; }

					i += 1;
				}

				if(array_deducer.terminator.is<core::GenericValue>()){
					if(this->isUnsignedIntegral(array_deducer.elementTypeID)){
						builder += ';';
						builder += array_deducer.terminator.as<core::GenericValue>().getInt(
							unsigned(this->numBits(array_deducer.elementTypeID))
						).toString(false);

					}else if(this->isSignedIntegral(array_deducer.elementTypeID)){
						builder += ';';
						builder += array_deducer.terminator.as<core::GenericValue>().getInt(
							unsigned(this->numBits(array_deducer.elementTypeID))
						).toString(true);

					}else if(this->isFloatingPoint(array_deducer.elementTypeID)){
						const BaseType::Primitive& primitive = this->getPrimitive(
							this->getTypeInfo(array_deducer.elementTypeID).baseTypeID().primitiveID()
						);

						builder += ';';

						switch(primitive.kind()){
							break; case Token::Kind::TYPE_F16:
								builder += array_deducer.terminator.as<core::GenericValue>().getF16().toString();

							break; case Token::Kind::TYPE_BF16:
								builder += array_deducer.terminator.as<core::GenericValue>().getBF16().toString();

							break; case Token::Kind::TYPE_F32:
								builder += array_deducer.terminator.as<core::GenericValue>().getF32().toString();

							break; case Token::Kind::TYPE_F64:
								builder += array_deducer.terminator.as<core::GenericValue>().getF64().toString();

							break; case Token::Kind::TYPE_F80:
								builder += array_deducer.terminator.as<core::GenericValue>().getF80().toString();

							break; case Token::Kind::TYPE_F128:
								builder += array_deducer.terminator.as<core::GenericValue>().getF128().toString();

							break; default: evo::debugFatalBreak("Unknown float type");
						}

					}else if(array_deducer.elementTypeID == TypeManager::getTypeBool()){
						builder += ';';
						builder += evo::boolStr(array_deducer.terminator.as<core::GenericValue>().getBool());

					}else if(array_deducer.elementTypeID == TypeManager::getTypeChar()){
						builder += std::format(
							";'{}'", evo::printCharName(array_deducer.terminator.as<core::GenericValue>().getChar())
						);
						
					}else{
						builder += ";<TERMINATOR>";
					}

				}else if(array_deducer.terminator.is<Token::ID>()){
					const Token& token = context.getSourceManager()[array_deducer.sourceID]
						.getTokenBuffer()[array_deducer.terminator.as<Token::ID>()];

					if(token.kind() == Token::Kind::DEDUCER){
						return std::format("${}", token.getString());
					}else{
						evo::debugAssert(
							token.kind() == Token::Kind::ANONYMOUS_DEDUCER, "Unknown type deducer kind"
						);
						return "$$";
					}
				}

				builder += ']';

				return builder;
			} break;

			case BaseType::Kind::ARRAY_REF: {
				const BaseType::ArrayRef& array_ref = this->getArrayRef(base_type_id.arrayRefID());

				auto builder = std::string();
				builder += '[';

				builder += this->printType(array_ref.elementTypeID, context);

				builder += ':';


				if(array_ref.isMut){
					builder += "*mut";
				}else{
					builder += "*";
				}


				if(array_ref.dimensions.size() != 1 || array_ref.dimensions[0].isPtr() == false){
					builder += '(';

					for(size_t i = 0; const BaseType::ArrayRef::Dimension& dimension : array_ref.dimensions){
						if(dimension.isPtr()){
							builder += "*";
						}else{
							builder += std::to_string(dimension.length());
						}


						if(i + 1 < array_ref.dimensions.size()){
							builder += ",";
						}

						i += 1;
					}

					builder += ')';
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

			case BaseType::Kind::ARRAY_REF_DEDUCER: {
				const BaseType::ArrayRefDeducer& array_ref_deducer =
					this->getArrayRefDeducer(base_type_id.arrayRefDeducerID());

				auto builder = std::string();
				builder += '[';

				builder += this->printType(array_ref_deducer.elementTypeID, context);

				builder += ':';


				if(array_ref_deducer.isMut){
					builder += "*mut";
				}else{
					builder += "*";
				}


				if(array_ref_deducer.dimensions.size() != 1 || array_ref_deducer.dimensions[0].isPtr() == false){
					builder += '(';

					for(
						size_t i = 0;
						const BaseType::ArrayRefDeducer::Dimension& dimension : array_ref_deducer.dimensions
					){
						if(dimension.isPtr()){
							builder += "*";

						}else if(dimension.isDeducer()){
							const Token& token = context.getSourceManager()[array_ref_deducer.sourceID]
								.getTokenBuffer()[dimension.deducer()];

							if(token.kind() == Token::Kind::DEDUCER){
								builder += std::format("${}", token.getString());
							}else{
								evo::debugAssert(
									token.kind() == Token::Kind::ANONYMOUS_DEDUCER, "Unknown type deducer kind"
								);
								builder += "$$";
							}

						}else{
							builder += std::to_string(dimension.length());
						}


						if(i + 1 < array_ref_deducer.dimensions.size()){
							builder += ",";
						}

						i += 1;
					}

					builder += ')';
				}



				if(array_ref_deducer.terminator.is<core::GenericValue>()){
					if(this->isUnsignedIntegral(array_ref_deducer.elementTypeID)){
						builder += ';';
						builder += array_ref_deducer.terminator.as<core::GenericValue>().getInt(
							unsigned(this->numBits(array_ref_deducer.elementTypeID))
						).toString(false);

					}else if(this->isSignedIntegral(array_ref_deducer.elementTypeID)){
						builder += ';';
						builder += array_ref_deducer.terminator.as<core::GenericValue>().getInt(
							unsigned(this->numBits(array_ref_deducer.elementTypeID))
						).toString(true);

					}else if(this->isFloatingPoint(array_ref_deducer.elementTypeID)){
						const BaseType::Primitive& primitive = this->getPrimitive(
							this->getTypeInfo(array_ref_deducer.elementTypeID).baseTypeID().primitiveID()
						);

						builder += ';';

						switch(primitive.kind()){
							break; case Token::Kind::TYPE_F16:
								builder += array_ref_deducer.terminator.as<core::GenericValue>().getF16().toString();

							break; case Token::Kind::TYPE_BF16:
								builder += array_ref_deducer.terminator.as<core::GenericValue>().getBF16().toString();

							break; case Token::Kind::TYPE_F32:
								builder += array_ref_deducer.terminator.as<core::GenericValue>().getF32().toString();

							break; case Token::Kind::TYPE_F64:
								builder += array_ref_deducer.terminator.as<core::GenericValue>().getF64().toString();

							break; case Token::Kind::TYPE_F80:
								builder += array_ref_deducer.terminator.as<core::GenericValue>().getF80().toString();

							break; case Token::Kind::TYPE_F128:
								builder += array_ref_deducer.terminator.as<core::GenericValue>().getF128().toString();

							break; default: evo::debugFatalBreak("Unknown float type");
						}

					}else if(array_ref_deducer.elementTypeID == TypeManager::getTypeBool()){
						builder += ';';
						builder += evo::boolStr(array_ref_deducer.terminator.as<core::GenericValue>().getBool());

					}else if(array_ref_deducer.elementTypeID == TypeManager::getTypeChar()){
						builder += std::format(
							";'{}'", evo::printCharName(array_ref_deducer.terminator.as<core::GenericValue>().getChar())
						);
						
					}else{
						builder += ";<TERMINATOR>";
					}

				}else if(array_ref_deducer.terminator.is<Token::ID>()){
					const Token& token = context.getSourceManager()[array_ref_deducer.sourceID]
						.getTokenBuffer()[array_ref_deducer.terminator.as<Token::ID>()];

					if(token.kind() == Token::Kind::DEDUCER){
						return std::format("${}", token.getString());
					}else{
						evo::debugAssert(
							token.kind() == Token::Kind::ANONYMOUS_DEDUCER, "Unknown type deducer kind"
						);
						return "$$";
					}
				}

				builder += ']';

				return builder;
			} break;

			case BaseType::Kind::ALIAS: {
				const BaseType::Alias::ID alias_id = base_type_id.aliasID();
				const BaseType::Alias& alias_type = this->getAlias(alias_id);

				return this->get_parent_name(alias_type.parent, alias_type.sourceID, context) +
					std::string(alias_type.getName(context.getSourceManager()));
			} break;

			case BaseType::Kind::DISTINCT_ALIAS: {
				const BaseType::DistinctAlias::ID distinct_alias_id = base_type_id.distinctAliasID();
				const BaseType::DistinctAlias& distinct_alias_type = this->getDistinctAlias(distinct_alias_id);


				return this->get_parent_name(distinct_alias_type.parent, distinct_alias_type.sourceID, context) +
					std::string(distinct_alias_type.getName(context.getSourceManager()));
			} break;

			case BaseType::Kind::STRUCT: {
				const BaseType::Struct::ID struct_id = base_type_id.structID();
				const BaseType::Struct& struct_info = this->getStruct(struct_id);

				const std::string_view struct_name = struct_info.getName(context.getSourceManager());

				auto builder = this->get_parent_name(struct_info.parent, struct_info.sourceID, context);

				builder += struct_name;

				if(struct_info.templateID.has_value() == false){
					return builder;
				}

				const BaseType::StructTemplate& struct_template = this->getStructTemplate(*struct_info.templateID);

				builder += "<{";

				const evo::SmallVector<BaseType::StructTemplate::Arg> template_args =
					struct_template.getInstantiationArgs(struct_info.instantiation);


				// TODO(PERF): 
				for(size_t i = 0; const BaseType::StructTemplate::Arg& template_arg : template_args){
					if(template_arg.is<TypeInfo::VoidableID>()){
						builder += this->printType(template_arg.as<TypeInfo::VoidableID>(), context);

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

				const TokenBuffer& token_buffer =
					context.getSourceManager()[struct_template_info.sourceID].getTokenBuffer();

				return this->get_parent_name(struct_template_info.parent, struct_template_info.sourceID, context) +
					std::string(token_buffer[struct_template_info.identTokenID].getString());
			} break;

			case BaseType::Kind::STRUCT_TEMPLATE_DEDUCER: {
				const BaseType::StructTemplateDeducer& struct_template_deduce_info = this->getStructTemplateDeducer(
					base_type_id.structTemplateDeducerID()
				);

				const BaseType::StructTemplate& struct_template = this->getStructTemplate(
					struct_template_deduce_info.structTemplateID
				);


				std::string builder = this->get_parent_name(struct_template.parent, struct_template.sourceID, context);
				builder += this->printType(BaseType::ID(struct_template_deduce_info.structTemplateID), context);
				builder += "<{";


				// TODO(PERF): 
				for(size_t i = 0; const BaseType::StructTemplate::Arg& template_arg : struct_template_deduce_info.args){
					if(template_arg.is<TypeInfo::VoidableID>()){
						builder += this->printType(template_arg.as<TypeInfo::VoidableID>(), context);

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


					if(i + 1 < struct_template_deduce_info.args.size()){
						builder += ", ";
					}
				
					i += 1;
				}

				builder += "}>";

				return builder;
			} break;

			case BaseType::Kind::UNION: {
				const BaseType::Union::ID union_id = base_type_id.unionID();
				const BaseType::Union& union_type = this->getUnion(union_id);

				return this->get_parent_name(union_type.parent, union_type.sourceID, context)
					+ std::string(union_type.getName(context.getSourceManager()));
			} break;

			case BaseType::Kind::ENUM: {
				const BaseType::Enum::ID enum_id = base_type_id.enumID();
				const BaseType::Enum& enum_type = this->getEnum(enum_id);

				return this->get_parent_name(enum_type.parent, enum_type.sourceID, context)
					+ std::string(enum_type.getName(context.getSourceManager()));
			} break;

			case BaseType::Kind::TYPE_DEDUCER: {
				const BaseType::TypeDeducer::ID type_deducer_id = base_type_id.typeDeducerID();
				const BaseType::TypeDeducer& type_deducer = this->getTypeDeducer(type_deducer_id);

				if(type_deducer.sourceID.has_value() == false){
					return "$$";
				}

				const Token& token =
					context.getSourceManager()[*type_deducer.sourceID].getTokenBuffer()[*type_deducer.identTokenID];

				if(token.kind() == Token::Kind::DEDUCER){
					return std::format("${}", token.getString());
				}else{
					evo::debugAssert(
						token.kind() == Token::Kind::ANONYMOUS_DEDUCER, "Unknown type deducer kind"
					);
					return "$$";
				}
			} break;

			case BaseType::Kind::INTERFACE: {
				const BaseType::Interface::ID interface_id = base_type_id.interfaceID();
				const BaseType::Interface& interface_type = this->getInterface(interface_id);

				return this->get_parent_name(interface_type.parent, interface_type.sourceID, context)
					+ std::string(interface_type.getName(context.getSourceManager()));
			} break;

			case BaseType::Kind::POLY_INTERFACE_REF: {
				const BaseType::PolyInterfaceRef::ID poly_interface_ref_id = base_type_id.polyInterfaceRefID();
				const BaseType::PolyInterfaceRef& poly_interface_ref_info =
					this->getPolyInterfaceRef(poly_interface_ref_id);
					
				return std::format(
					"impl({}:{})",
					poly_interface_ref_info.isMut ? "*mut" : "*",
					this->printType(BaseType::ID(poly_interface_ref_info.interfaceID), context)
				);
			} break;


			case BaseType::Kind::INTERFACE_MAP: {
				const BaseType::InterfaceMap& interface_map_info = this->getInterfaceMap(base_type_id.interfaceMapID());
				
				return std::format(
					"impl({}:{})",
					this->printType(interface_map_info.underlyingTypeID, context),
					this->printType(BaseType::ID(interface_map_info.interfaceID), context)
				);
			} break;
		}

		evo::debugFatalBreak("Unknown or unsuport base-type kind");
	}



	template<TypeManager::SpecialMember SPECIAL_MEMBER, TypeManager::SpecialMemberProp SPECIAL_MEMBER_PROP>
	auto TypeManager::special_member_prop_check(TypeInfo::ID id, const SemaBuffer* sema_buffer) const -> bool {
		if constexpr(SPECIAL_MEMBER == SpecialMember::DEFAULT_NEW){

		}else if constexpr(SPECIAL_MEMBER == SpecialMember::DELETE){
			static_assert(SPECIAL_MEMBER_PROP != SpecialMemberProp::AT_ALL, "invalid special member prop check");
			static_assert(SPECIAL_MEMBER_PROP != SpecialMemberProp::NO_ERROR, "invalid special member prop check");
			static_assert(SPECIAL_MEMBER_PROP != SpecialMemberProp::SAFE, "invalid special member prop check");
			
		}else if constexpr(SPECIAL_MEMBER == SpecialMember::COPY){
			
		}else if constexpr(SPECIAL_MEMBER == SpecialMember::MOVE){
			
		}else{
			static_assert(false, "Unknown special member");
		}

		if constexpr(
			SPECIAL_MEMBER_PROP != SpecialMemberProp::AT_ALL
			&& SPECIAL_MEMBER_PROP != SpecialMemberProp::COMPTIME
			&& SPECIAL_MEMBER_PROP != SpecialMemberProp::NO_ERROR
			&& SPECIAL_MEMBER_PROP != SpecialMemberProp::SAFE
			&& SPECIAL_MEMBER_PROP != SpecialMemberProp::TRIVIAL
		){
			static_assert(false, "Unknown special member prop");
		}


		const TypeInfo& type_info = this->getTypeInfo(id);

		if constexpr(SPECIAL_MEMBER == SpecialMember::DEFAULT_NEW){
			if constexpr(
				SPECIAL_MEMBER_PROP == SpecialMemberProp::AT_ALL
				|| SPECIAL_MEMBER_PROP == SpecialMemberProp::NO_ERROR
				|| SPECIAL_MEMBER_PROP == SpecialMemberProp::COMPTIME
				|| SPECIAL_MEMBER_PROP == SpecialMemberProp::SAFE
			){
				if(type_info.qualifiers().empty()){
					return this->special_member_prop_check<SPECIAL_MEMBER, SPECIAL_MEMBER_PROP>(
						type_info.baseTypeID(), sema_buffer
					);
				}

				if(type_info.qualifiers().back().isOptional){ return true; }
				return false;

			}else if constexpr(SPECIAL_MEMBER_PROP == SpecialMemberProp::TRIVIAL){
				if(type_info.qualifiers().empty() == false){ return false; }
				return this->isTriviallyDefaultInitializable(type_info.baseTypeID());
			}

		}else if constexpr(SPECIAL_MEMBER == SpecialMember::DELETE){
			for(const TypeInfo::Qualifier& qualifier : type_info.qualifiers()){
				if(qualifier.isPtr){ return true; }
			}

			return this->special_member_prop_check<SPECIAL_MEMBER, SPECIAL_MEMBER_PROP>(
				type_info.baseTypeID(), sema_buffer
			);

		}else if constexpr(SPECIAL_MEMBER == SpecialMember::COPY || SPECIAL_MEMBER == SpecialMember::MOVE){
			for(const TypeInfo::Qualifier& qualifier : type_info.qualifiers()){
				if(qualifier.isPtr){ return true; }
			}

			return this->special_member_prop_check<SPECIAL_MEMBER, SPECIAL_MEMBER_PROP>(
				type_info.baseTypeID(), sema_buffer
			);
		}
	}


	template<TypeManager::SpecialMember SPECIAL_MEMBER, TypeManager::SpecialMemberProp SPECIAL_MEMBER_PROP>
	auto TypeManager::special_member_prop_check(BaseType::ID id, const SemaBuffer* sema_buffer) const -> bool {
		if constexpr(SPECIAL_MEMBER == SpecialMember::DEFAULT_NEW){

		}else if constexpr(SPECIAL_MEMBER == SpecialMember::DELETE){
			static_assert(SPECIAL_MEMBER_PROP != SpecialMemberProp::AT_ALL, "invalid special member prop check");
			static_assert(SPECIAL_MEMBER_PROP != SpecialMemberProp::NO_ERROR, "invalid special member prop check");
			static_assert(SPECIAL_MEMBER_PROP != SpecialMemberProp::SAFE, "invalid special member prop check");
			
		}else if constexpr(SPECIAL_MEMBER == SpecialMember::COPY){
			
		}else if constexpr(SPECIAL_MEMBER == SpecialMember::MOVE){
			
		}else{
			static_assert(false, "Unknown special member");
		}

		if constexpr(
			SPECIAL_MEMBER_PROP != SpecialMemberProp::AT_ALL
			&& SPECIAL_MEMBER_PROP != SpecialMemberProp::COMPTIME
			&& SPECIAL_MEMBER_PROP != SpecialMemberProp::NO_ERROR
			&& SPECIAL_MEMBER_PROP != SpecialMemberProp::SAFE
			&& SPECIAL_MEMBER_PROP != SpecialMemberProp::TRIVIAL
		){
			static_assert(false, "Unknown special member prop");
		}


		switch(id.kind()){
			case BaseType::Kind::DUMMY: {
				evo::debugFatalBreak("Invalid type");
			} break;

			case BaseType::Kind::PRIMITIVE: {
				const BaseType::Primitive& primitive = this->getPrimitive(id.primitiveID());

				if constexpr(SPECIAL_MEMBER == SpecialMember::DEFAULT_NEW){
					if(primitive.kind() == Token::Kind::TYPE_RAWPTR || primitive.kind() == Token::Kind::TYPE_TYPEID){
						return false;
					}

					return true;

				}else{
					return true;
				}
			} break;

			case BaseType::Kind::FUNCTION: {
				return false;
			} break;

			case BaseType::Kind::ARRAY: {
				const BaseType::Array& array = this->getArray(id.arrayID());
				return this->special_member_prop_check<SPECIAL_MEMBER, SPECIAL_MEMBER_PROP>(
					array.elementTypeID, sema_buffer
				);
			} break;

			case BaseType::Kind::ARRAY_REF: {
				return true;
			} break;

			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias = this->getAlias(id.aliasID());
				return this->special_member_prop_check<SPECIAL_MEMBER, SPECIAL_MEMBER_PROP>(
					alias.aliasedType, sema_buffer
				);
			} break;

			case BaseType::Kind::DISTINCT_ALIAS: {
				const BaseType::DistinctAlias& alias = this->getDistinctAlias(id.distinctAliasID());
				return this->special_member_prop_check<SPECIAL_MEMBER, SPECIAL_MEMBER_PROP>(
					alias.underlyingType, sema_buffer
				);
			} break;

			case BaseType::Kind::STRUCT: {
				const BaseType::Struct& struct_info = this->getStruct(id.structID());

				if constexpr(SPECIAL_MEMBER == SpecialMember::DEFAULT_NEW){
					if constexpr(SPECIAL_MEMBER_PROP == SpecialMemberProp::AT_ALL){
						return struct_info.isDefaultInitializable;

					}else if constexpr(SPECIAL_MEMBER_PROP == SpecialMemberProp::NO_ERROR){
						return struct_info.isNoErrorDefaultInitializable;

					}else if constexpr(SPECIAL_MEMBER_PROP == SpecialMemberProp::SAFE){
						return struct_info.isSafeDefaultInitializable;

					}else if constexpr(SPECIAL_MEMBER_PROP == SpecialMemberProp::COMPTIME){
						return struct_info.isComptimeDefaultInitializable;

					}else if constexpr(SPECIAL_MEMBER_PROP == SpecialMemberProp::TRIVIAL){
						return struct_info.isTriviallyDefaultInitializable;
					}

				}else if constexpr(SPECIAL_MEMBER == SpecialMember::DELETE){
					if constexpr(SPECIAL_MEMBER_PROP == SpecialMemberProp::TRIVIAL){
						return struct_info.deleteOverload.load().has_value() == false;

					}else if constexpr(SPECIAL_MEMBER_PROP == SpecialMemberProp::COMPTIME){
						const std::optional<sema::Func::ID> delete_overload = struct_info.deleteOverload.load();
						if(delete_overload.has_value() == false){ return true; }
						return sema_buffer->getFunc(*delete_overload).isComptime;
					}

				}else if constexpr(SPECIAL_MEMBER == SpecialMember::COPY){
					if constexpr(SPECIAL_MEMBER_PROP == SpecialMemberProp::AT_ALL){
						return !struct_info.copyInitOverload.load().wasDeleted;

					}else if constexpr(SPECIAL_MEMBER_PROP == SpecialMemberProp::NO_ERROR){
						static_assert(false, "Unimplemented");

					}else if constexpr(SPECIAL_MEMBER_PROP == SpecialMemberProp::SAFE){
						const BaseType::Struct::DeletableOverload copy_overload = struct_info.copyInitOverload.load();
						if(copy_overload.wasDeleted){ return false; }
						if(copy_overload.funcID.has_value() == false){ return true; }

						const sema::Func& copy_overload_sema_func = sema_buffer->getFunc(*copy_overload.funcID);
						return this->getFunction(copy_overload_sema_func.typeID).isUnsafe == false;

					}else if constexpr(SPECIAL_MEMBER_PROP == SpecialMemberProp::COMPTIME){
						const BaseType::Struct::DeletableOverload copy_overload = struct_info.copyInitOverload.load();
						if(copy_overload.wasDeleted){ return false; }
						if(copy_overload.funcID.has_value() == false){ return true; }
						return sema_buffer->getFunc(*copy_overload.funcID).isComptime;

					}else if constexpr(SPECIAL_MEMBER_PROP == SpecialMemberProp::TRIVIAL){
						const BaseType::Struct::DeletableOverload delete_overload = struct_info.copyInitOverload.load();
						return delete_overload.wasDeleted == false && delete_overload.funcID.has_value() == false;
					}

				}else if constexpr(SPECIAL_MEMBER == SpecialMember::MOVE){
					if constexpr(SPECIAL_MEMBER_PROP == SpecialMemberProp::AT_ALL){
						return !struct_info.moveInitOverload.load().wasDeleted;

					}else if constexpr(SPECIAL_MEMBER_PROP == SpecialMemberProp::NO_ERROR){
						static_assert(false, "Unimplemented");

					}else if constexpr(SPECIAL_MEMBER_PROP == SpecialMemberProp::SAFE){
						const BaseType::Struct::DeletableOverload move_overload = struct_info.moveInitOverload.load();
						if(move_overload.wasDeleted){ return false; }
						if(move_overload.funcID.has_value() == false){ return true; }

						const sema::Func& move_overload_sema_func = sema_buffer->getFunc(*move_overload.funcID);
						return this->getFunction(move_overload_sema_func.typeID).isUnsafe == false;

					}else if constexpr(SPECIAL_MEMBER_PROP == SpecialMemberProp::COMPTIME){
						const BaseType::Struct::DeletableOverload move_overload = struct_info.moveInitOverload.load();
						if(move_overload.wasDeleted){ return false; }
						if(move_overload.funcID.has_value() == false){ return true; }
						return sema_buffer->getFunc(*move_overload.funcID).isComptime;

					}else if constexpr(SPECIAL_MEMBER_PROP == SpecialMemberProp::TRIVIAL){
						const BaseType::Struct::DeletableOverload delete_overload = struct_info.moveInitOverload.load();
						return delete_overload.wasDeleted == false && delete_overload.funcID.has_value() == false;
					}
				}
			} break;

			case BaseType::Kind::UNION: {
				const BaseType::Union& union_info = this->getUnion(id.unionID());

				if(union_info.isUntagged){
					if constexpr(SPECIAL_MEMBER == SpecialMember::DEFAULT_NEW){
						for(const BaseType::Union::Field& field : union_info.fields){
							// Yes, this is the correct method
							if(this->isTriviallyDefaultInitializable(field.typeID.asTypeID()) == false){ return false; }
						}	
					}

					return true;

				}else{
					if constexpr(SPECIAL_MEMBER == SpecialMember::DEFAULT_NEW){
						if constexpr(SPECIAL_MEMBER_PROP == SpecialMemberProp::TRIVIAL){
							return false;

						}else{
							const TypeInfo::VoidableID default_field_type = union_info.fields[0].typeID;
							if(default_field_type.isVoid()){ return true; }

							return this->special_member_prop_check<SpecialMember::DEFAULT_NEW, SPECIAL_MEMBER_PROP>(
								default_field_type.asTypeID(), sema_buffer
							);
						}

					}else{
						for(const BaseType::Union::Field& field : union_info.fields){
							if(field.typeID.isVoid()){ continue; }

							if(this->special_member_prop_check<SPECIAL_MEMBER, SPECIAL_MEMBER_PROP>(
								field.typeID.asTypeID(), sema_buffer
							) == false){
								return false;
							}
						}

						return true;
					}
				}
			} break;

			case BaseType::Kind::ENUM: {
				return true;
			} break;

			case BaseType::Kind::POLY_INTERFACE_REF: {
				if constexpr(SPECIAL_MEMBER == SpecialMember::DEFAULT_NEW){
					return false;
				}else{
					return true;
				}
			} break;

			case BaseType::Kind::ARRAY_DEDUCER:   case BaseType::Kind::ARRAY_REF_DEDUCER:
			case BaseType::Kind::STRUCT_TEMPLATE: case BaseType::Kind::STRUCT_TEMPLATE_DEDUCER:
			case BaseType::Kind::TYPE_DEDUCER:    case BaseType::Kind::INTERFACE: {
				evo::debugFatalBreak("Invalid to check with this type");
			} break;

			case BaseType::Kind::INTERFACE_MAP: {
				const BaseType::InterfaceMap& interface_map_info = this->getInterfaceMap(id.interfaceMapID());

				return this->special_member_prop_check<SPECIAL_MEMBER, SPECIAL_MEMBER_PROP>(
					interface_map_info.underlyingTypeID, sema_buffer
				);
			} break;
		}
		evo::debugFatalBreak("Unknown or unsupported BaseType");
	}



	auto TypeManager::is_safe_byte_bit_cast_base_type(BaseType::ID id) const -> bool {
		switch(id.kind()){
			case BaseType::Kind::PRIMITIVE: {
				return this->getPrimitive(id.primitiveID()).kind() == Token::Kind::TYPE_BYTE;
			} break;

			case BaseType::Kind::ARRAY: {
				const BaseType::Array& array_type = this->getArray(id.arrayID());

				const TypeInfo& elem_type = this->getTypeInfo(array_type.elementTypeID);
				if(elem_type.qualifiers().size() > 0){ return false; }

				return this->is_safe_byte_bit_cast_base_type(elem_type.baseTypeID());
			} break;

			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias_type = this->getAlias(id.aliasID());

				const TypeInfo& aliased_type = this->getTypeInfo(alias_type.aliasedType);
				if(aliased_type.qualifiers().size() > 0){ return false; }

				return this->is_safe_byte_bit_cast_base_type(aliased_type.baseTypeID());
			} break;

			default: {
				return false;
			} break;
		}
	}



	auto TypeManager::get_parent_name(
		std::optional<EncapsulatingSymbolID> parent,
		evo::Variant<SourceID, ClangSourceID, BuiltinModuleID> source_id,
		const Context& context
	) const -> std::string {
		auto output = std::string();

		if(parent.has_value()){
			parent->visit([&](const auto& parent_id) -> void {
				using ParentIDType = std::decay_t<decltype(parent_id)>;

				if constexpr(
					std::is_same<ParentIDType, BaseType::StructID>()
					|| std::is_same<ParentIDType, BaseType::UnionID>()
					|| std::is_same<ParentIDType, BaseType::EnumID>()
					|| std::is_same<ParentIDType, BaseType::InterfaceID>()
				){
					output = this->printType(BaseType::ID(parent_id), context);
					output += ".";

				}else if constexpr(std::is_same<ParentIDType, sema::FuncID>()){
					output = this->get_func_name(parent_id, context);
					output += ".";

				}else if constexpr(std::is_same<ParentIDType, EncapsulatingSymbolID::InterfaceImplInfo>()){
					output = this->printType(BaseType::ID(parent_id.interfaceID), context);
					output += std::format(".impl_{}.", parent_id.targetTypeID.get());

				}else{
					static_assert(false, "Unknown encapsulating symbol");
				}
			});

		}else{
			if(source_id.is<Source::ID>()){
				const Source& parent_source = context.getSourceManager()[source_id.as<Source::ID>()];
				const Source::Package& parent_package =
					context.getSourceManager().getPackage(parent_source.getPackageID());

				output = parent_package.name;
				output += ".";

			}else if(source_id.is<BuiltinModule::ID>()){
				switch(source_id.as<BuiltinModule::ID>()){
					break; case BuiltinModule::ID::PTHR:  output = "@pthr.";
					break; case BuiltinModule::ID::BUILD: output = "@build.";
				}

			}else{
				evo::debugAssert(source_id.is<ClangSource::ID>(), "Unknown source ID");

				const ClangSource& clang_source = context.getSourceManager()[source_id.as<ClangSource::ID>()];

				if(clang_source.isCPP()){
					output += "{CPP}.";
				}else{
					output += "{C}.";
				}
			}
		}

		return output;
	}


	auto TypeManager::get_func_name(sema::FuncID sema_func_id, const Context& context) const -> std::string {
		const sema::Func& sema_func = context.getSemaBuffer().getFunc(sema_func_id);

		auto output = this->get_parent_name(sema_func.parent, sema_func.sourceID, context);
		output += sema_func.getName(context.getSourceManager());

		return output;
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

	auto TypeManager::getOrCreateFunction(BaseType::Function&& lookup_type) -> BaseType::ID {
		const auto lock = std::scoped_lock(this->functions_lock);

		for(uint32_t i = 0; i < this->functions.size(); i+=1){
			if(this->functions[BaseType::Function::ID(i)] == lookup_type){
				return BaseType::ID(BaseType::Kind::FUNCTION, i);
			}
		}

		const BaseType::Function::ID new_function = this->functions.emplace_back(lookup_type);
		return BaseType::ID(BaseType::Kind::FUNCTION, new_function.get());
	}


	//////////////////////////////////////////////////////////////////////
	// array

	auto TypeManager::getArray(BaseType::Array::ID id) const -> const BaseType::Array& {
		const auto lock = std::scoped_lock(this->arrays_lock);
		return this->arrays[id];
	}

	auto TypeManager::getOrCreateArray(BaseType::Array&& lookup_type) -> BaseType::ID {
		const auto lock = std::scoped_lock(this->arrays_lock);

		for(uint32_t i = 0; i < this->arrays.size(); i+=1){
			if(this->arrays[BaseType::Array::ID(i)] == lookup_type){
				return BaseType::ID(BaseType::Kind::ARRAY, i);
			}
		}

		const BaseType::Array::ID new_array = this->arrays.emplace_back(lookup_type);
		return BaseType::ID(BaseType::Kind::ARRAY, new_array.get());
	}


	//////////////////////////////////////////////////////////////////////
	// array deducer

	auto TypeManager::getArrayDeducer(BaseType::ArrayDeducer::ID id) const -> const BaseType::ArrayDeducer& {
		const auto lock = std::scoped_lock(this->array_deducers_lock);
		return this->array_deducers[id];
	}

	auto TypeManager::getOrCreateArrayDeducer(BaseType::ArrayDeducer&& lookup_type) -> BaseType::ID {
		const auto lock = std::scoped_lock(this->array_deducers_lock);

		for(uint32_t i = 0; i < this->array_deducers.size(); i+=1){
			if(this->array_deducers[BaseType::ArrayDeducer::ID(i)] == lookup_type){
				return BaseType::ID(BaseType::Kind::ARRAY_DEDUCER, i);
			}
		}

		const BaseType::ArrayDeducer::ID new_array_deducer = this->array_deducers.emplace_back(lookup_type);
		return BaseType::ID(BaseType::Kind::ARRAY_DEDUCER, new_array_deducer.get());
	}


	//////////////////////////////////////////////////////////////////////
	// array ref

	auto TypeManager::getArrayRef(BaseType::ArrayRef::ID id) const -> const BaseType::ArrayRef& {
		const auto lock = std::scoped_lock(this->array_refs_lock);
		return this->array_refs[id];
	}

	auto TypeManager::getOrCreateArrayRef(BaseType::ArrayRef&& lookup_type) -> BaseType::ID {
		const auto lock = std::scoped_lock(this->array_refs_lock);

		for(uint32_t i = 0; i < this->array_refs.size(); i+=1){
			if(this->array_refs[BaseType::ArrayRef::ID(i)] == lookup_type){
				return BaseType::ID(BaseType::Kind::ARRAY_REF, i);
			}
		}

		const BaseType::ArrayRef::ID new_array_ref = this->array_refs.emplace_back(lookup_type);
		return BaseType::ID(BaseType::Kind::ARRAY_REF, new_array_ref.get());
	}


	//////////////////////////////////////////////////////////////////////
	// array ref deducer

	auto TypeManager::getArrayRefDeducer(BaseType::ArrayRefDeducer::ID id) const -> const BaseType::ArrayRefDeducer& {
		const auto lock = std::scoped_lock(this->array_ref_deducers_lock);
		return this->array_ref_deducers[id];
	}

	auto TypeManager::getOrCreateArrayRefDeducer(BaseType::ArrayRefDeducer&& lookup_type) -> BaseType::ID {
		const auto lock = std::scoped_lock(this->array_ref_deducers_lock);

		for(uint32_t i = 0; i < this->array_ref_deducers.size(); i+=1){
			if(this->array_ref_deducers[BaseType::ArrayRefDeducer::ID(i)] == lookup_type){
				return BaseType::ID(BaseType::Kind::ARRAY_REF_DEDUCER, i);
			}
		}

		const BaseType::ArrayRefDeducer::ID new_array_ref_deducer = this->array_ref_deducers.emplace_back(lookup_type);
		return BaseType::ID(BaseType::Kind::ARRAY_REF_DEDUCER, new_array_ref_deducer.get());
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


	auto TypeManager::createAlias(BaseType::Alias&& new_type) -> BaseType::ID {
		this->aliases_lock.lock();

		const BaseType::Alias::ID new_alias = this->aliases.emplace_back(
			new_type.sourceID, new_type.name, new_type.parent, new_type.aliasedType, new_type.isPub
		);

		this->aliases_lock.unlock();

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


	auto TypeManager::createDistinctAlias(BaseType::DistinctAlias&& new_type) -> BaseType::ID {
		this->distinct_aliases_lock.lock();

		const BaseType::DistinctAlias::ID new_distinct_alias = this->distinct_aliases.emplace_back(
			new_type.sourceID,
			new_type.name,
			new_type.parent,
			new_type.underlyingType,
			new_type.isPub
		);

		this->distinct_aliases_lock.unlock();

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


	auto TypeManager::createStruct(BaseType::Struct&& new_type) -> BaseType::ID {
		this->structs_lock.lock();

		const BaseType::Struct::ID new_struct = this->structs.emplace_back(
			new_type.sourceID,
			new_type.name,
			new_type.parent,
			new_type.templateID,
			new_type.instantiation,
			std::move(new_type.memberVars),
			std::move(new_type.memberVarsABI),
			new_type.namespacedMembers,
			new_type.scopeLevel,
			new_type.isPub,
			new_type.isOrdered,
			new_type.isPacked,
			new_type.shouldLower
		);

		this->structs_lock.unlock();

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



	auto TypeManager::createStructTemplate(BaseType::StructTemplate&& new_type) -> BaseType::ID {
		this->struct_templates_lock.lock();

		const BaseType::StructTemplate::ID new_struct = this->struct_templates.emplace_back(
			new_type.sourceID,
			new_type.identTokenID,
			new_type.parent,
			std::move(new_type.params),
			new_type.minNumTemplateArgs
		);

		this->struct_templates_lock.unlock();

		return BaseType::ID(BaseType::Kind::STRUCT_TEMPLATE, new_struct.get());
	}


	//////////////////////////////////////////////////////////////////////
	// struct template deducers

	auto TypeManager::getStructTemplateDeducer(BaseType::StructTemplateDeducer::ID id) const
	-> const BaseType::StructTemplateDeducer& {
		const auto lock = std::scoped_lock(this->struct_template_deducers_lock);
		return this->struct_template_deducers[id];
	}


	auto TypeManager::createStructTemplateDeducer(BaseType::StructTemplateDeducer&& new_type) -> BaseType::ID {
		this->struct_template_deducers_lock.lock();

		const BaseType::StructTemplateDeducer::ID new_struct =
			this->struct_template_deducers.emplace_back(std::move(new_type));

		this->struct_template_deducers_lock.unlock();

		return BaseType::ID(BaseType::Kind::STRUCT_TEMPLATE_DEDUCER, new_struct.get());
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


	auto TypeManager::createUnion(BaseType::Union&& new_type) -> BaseType::ID {
		this->unions_lock.lock();

		const BaseType::Union::ID new_union = this->unions.emplace_back(
			new_type.sourceID,
			new_type.location,
			new_type.parent,
			std::move(new_type.fields),
			new_type.namespacedMembers,
			new_type.scopeLevel,
			new_type.isPub,
			new_type.isUntagged
		);

		this->unions_lock.unlock();

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


	auto TypeManager::createEnum(BaseType::Enum&& new_type) -> BaseType::ID {
		this->enums_lock.lock();

		const BaseType::Enum::ID new_enum = this->enums.emplace_back(
			new_type.sourceID,
			new_type.name,
			new_type.parent,
			std::move(new_type.enumerators),
			new_type.underlyingTypeID,
			new_type.namespacedMembers,
			new_type.scopeLevel,
			new_type.isPub
		);

		this->enums_lock.unlock();

		return BaseType::ID(BaseType::Kind::ENUM, new_enum.get());
	}



	//////////////////////////////////////////////////////////////////////
	// type deducer

	auto TypeManager::getTypeDeducer(BaseType::TypeDeducer::ID id) const -> const BaseType::TypeDeducer& {
		const auto lock = std::scoped_lock(this->type_deducers_lock);
		return this->type_deducers[id];
	}


	auto TypeManager::createTypeDeducer(BaseType::TypeDeducer&& new_type) -> BaseType::ID {
		const auto lock = std::scoped_lock(this->type_deducers_lock);

		const BaseType::TypeDeducer::ID new_type_deducer = this->type_deducers.emplace_back(std::move(new_type));
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


	auto TypeManager::createInterface(BaseType::Interface&& new_type) -> BaseType::ID {
		this->interfaces_lock.lock();

		const BaseType::Interface::ID new_interface = this->interfaces.emplace_back(
			new_type.sourceID,
			new_type.name,
			new_type.parent,
			new_type.symbolProcID,
			new_type.isPub,
			new_type.isPolymorphic
		);

		this->interfaces_lock.unlock();

		return BaseType::ID(BaseType::Kind::INTERFACE, new_interface.get());
	}


	auto TypeManager::getNumInterfaces() const -> size_t {
		const auto lock = std::scoped_lock(this->interfaces_lock);
		return this->interfaces.size();
	}


	auto TypeManager::createInterfaceImpl(const AST::InterfaceImpl& ast_node) -> BaseType::Interface::Impl& {
		return this->interface_impls[this->interface_impls.emplace_back(ast_node)];
	}

	auto TypeManager::createInterfaceDeducerImpl(
		const TypeInfo::ID deducer_type_id, AST::Node ast_node, SymbolProcID symbol_proc_id
	) -> BaseType::Interface::DeducerImpl& {
		return this->interface_deducer_impls[
			this->interface_deducer_impls.emplace_back(deducer_type_id, ast_node, symbol_proc_id)
		];
	}



	//////////////////////////////////////////////////////////////////////
	// poly interface ref

	auto TypeManager::getPolyInterfaceRef(BaseType::PolyInterfaceRef::ID id) const
	-> const BaseType::PolyInterfaceRef& {
		const auto lock = std::scoped_lock(this->poly_interface_refs_lock);
		return this->poly_interface_refs[id];
	}


	auto TypeManager::getOrCreatePolyInterfaceRef(BaseType::PolyInterfaceRef&& lookup_type) -> BaseType::ID {
		const auto lock = std::scoped_lock(this->poly_interface_refs_lock);

		for(uint32_t i = 0; i < this->poly_interface_refs.size(); i+=1){
			if(this->poly_interface_refs[BaseType::PolyInterfaceRef::ID(i)] == lookup_type){
				return BaseType::ID(BaseType::Kind::POLY_INTERFACE_REF, i);
			}
		}

		const BaseType::PolyInterfaceRef::ID new_poly_interface_ref =
			this->poly_interface_refs.emplace_back(std::move(lookup_type));
		return BaseType::ID(BaseType::Kind::POLY_INTERFACE_REF, new_poly_interface_ref.get());
	}




	//////////////////////////////////////////////////////////////////////
	// interface map

	auto TypeManager::getInterfaceMap(BaseType::InterfaceMap::ID id) const -> const BaseType::InterfaceMap& {
		const auto lock = std::scoped_lock(this->interface_maps_lock);
		return this->interface_maps[id];
	}


	auto TypeManager::getOrCreateInterfaceMap(BaseType::InterfaceMap&& lookup_type)
	-> BaseType::ID {
		const auto lock = std::scoped_lock(this->interface_maps_lock);
		for(uint32_t i = 0; i < this->interface_maps.size(); i+=1){
			if(this->interface_maps[BaseType::InterfaceMap::ID(i)] == lookup_type){
				return BaseType::ID(BaseType::Kind::INTERFACE_MAP, i);
			}
		}

		const BaseType::InterfaceMap::ID new_instantiation = this->interface_maps.emplace_back(std::move(lookup_type));
		return BaseType::ID(BaseType::Kind::INTERFACE_MAP, new_instantiation.get());
	}




	//////////////////////////////////////////////////////////////////////
	// misc

	///////////////////////////////////
	// isTypeDeducer

	auto TypeManager::isTypeDeducer(TypeInfo::VoidableID id) const -> bool {
		if(id.isVoid()){ return false; }
		return this->isTypeDeducer(id.asTypeID());
	}

	auto TypeManager::isTypeDeducer(TypeInfo::ID id) const -> bool {
		return this->isTypeDeducer(this->getTypeInfo(id).baseTypeID());
	}

	auto TypeManager::isTypeDeducer(BaseType::ID id) const -> bool {
		switch(id.kind()){
			case BaseType::Kind::ARRAY_DEDUCER: {
				return true;
			} break;

			case BaseType::Kind::ARRAY_REF_DEDUCER: {
				return true;
			} break;

			case BaseType::Kind::STRUCT: {
				const BaseType::Struct& struct_type = this->getStruct(id.structID());

				if(struct_type.templateID.has_value() == false){ return false; }

				const BaseType::StructTemplate& struct_template =
					this->getStructTemplate(*struct_type.templateID);

				const evo::SmallVector<BaseType::StructTemplate::Arg> template_args =
					struct_template.getInstantiationArgs(struct_type.instantiation);

				for(const BaseType::StructTemplate::Arg& template_arg : template_args){
					if(template_arg.is<TypeInfo::VoidableID>() == false){ continue; }
					if(template_arg.as<TypeInfo::VoidableID>().isVoid()){ continue; }
					if(this->isTypeDeducer(template_arg.as<TypeInfo::VoidableID>().asTypeID()) == false){ continue; }
				}

				return false;
			} break;

			case BaseType::Kind::STRUCT_TEMPLATE_DEDUCER: {
				return true;
			} break;

			case BaseType::Kind::TYPE_DEDUCER: {
				return true;
			} break;

			case BaseType::Kind::INTERFACE_MAP: {
				const BaseType::InterfaceMap& interface_map_type = this->getInterfaceMap(id.interfaceMapID());
				return this->isTypeDeducer(interface_map_type.underlyingTypeID);
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
		if(type_info.qualifiers().empty()){ return this->numBytes(type_info.baseTypeID(), include_padding); }

		if(type_info.qualifiers().back().isPtr){
			return this->numBytesOfPtr();
		}else{
			return add_padding_bytes_if_needed(this->numBytes(type_info.baseTypeID(), false) + 1, include_padding);
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

				if(array.terminator.has_value()){ return elem_size * (array.dimensions.back() + 1); }

				uint64_t output = elem_size;
				for(uint64_t dimension : array.dimensions){
					output *= dimension;
				}
				return add_padding_bytes_if_needed(output, include_padding);
			} break;

			case BaseType::Kind::ARRAY_DEDUCER: {
				// TODO(FUTURE): handle this better?
				evo::debugFatalBreak("Cannot get size of array deducer");
			} break;

			case BaseType::Kind::ARRAY_REF: {
				const BaseType::ArrayRef& arr_ref = this->getArrayRef(id.arrayRefID());

				unsigned num_words = 1;
				for(const BaseType::ArrayRef::Dimension& dimension : arr_ref.dimensions){
					if(dimension.isPtr()){ num_words += 1; }
				}

				return this->numBytesOfPtr() * num_words;
			} break;

			case BaseType::Kind::ARRAY_REF_DEDUCER: {
				// TODO(FUTURE): handle this better?
				evo::debugFatalBreak("Cannot get size of array ref deducer");
			} break;

			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias = this->getAlias(id.aliasID());
				return this->numBytes(alias.aliasedType, include_padding);
			} break;

			case BaseType::Kind::DISTINCT_ALIAS: {
				const BaseType::DistinctAlias& distinct_alias = this->getDistinctAlias(id.distinctAliasID());
				return this->numBytes(distinct_alias.underlyingType, include_padding);
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

			case BaseType::Kind::STRUCT_TEMPLATE_DEDUCER: {
				// TODO(FUTURE): handle this better?
				evo::debugFatalBreak("Cannot get size of Struct Template Deducer");
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
				evo::debugFatalBreak("Cannot get size of interface");
			} break;

			case BaseType::Kind::POLY_INTERFACE_REF: {
				return this->numBytesOfPtr() * 2;
			} break;

			case BaseType::Kind::INTERFACE_MAP: {
				const BaseType::InterfaceMap& interface_map_info = this->getInterfaceMap(id.interfaceMapID());
				return this->numBytes(interface_map_info.underlyingTypeID, true);
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
		if(type_info.qualifiers().empty()){ return this->numBits(type_info.baseTypeID(), include_padding); }

		if(type_info.qualifiers().back().isPtr){
			return this->numBitsOfPtr();
		}else{
			return add_padding_bits_if_needed(
				this->numBytes(type_info.baseTypeID(), false) * 8 + 1, include_padding
			);
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

			case BaseType::Kind::ARRAY_DEDUCER: {
				// TODO(FUTURE): handle this better?
				evo::debugFatalBreak("Cannot get size of array deducer");
			} break;

			case BaseType::Kind::ARRAY_REF: {
				const BaseType::ArrayRef& arr_ref = this->getArrayRef(id.arrayRefID());

				unsigned num_words = 1;
				for(const BaseType::ArrayRef::Dimension& dimension : arr_ref.dimensions){
					if(dimension.isPtr()){ num_words += 1; }
				}

				return this->numBitsOfPtr() * num_words;
			} break;

			case BaseType::Kind::ARRAY_REF_DEDUCER: {
				// TODO(FUTURE): handle this better?
				evo::debugFatalBreak("Cannot get size of array ref deducer");
			} break;

			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias = this->getAlias(id.aliasID());
				return this->numBits(alias.aliasedType, include_padding);
			} break;

			case BaseType::Kind::DISTINCT_ALIAS: {
				const BaseType::DistinctAlias& distinct_alias = this->getDistinctAlias(id.distinctAliasID());
				return this->numBits(distinct_alias.underlyingType, include_padding);
			} break;

			case BaseType::Kind::STRUCT: {
				return this->numBytes(id, include_padding) * 8;
			} break;

			case BaseType::Kind::STRUCT_TEMPLATE: {
				// TODO(FUTURE): handle this better?
				evo::debugAssert("Cannot get size of Struct Template");
			} break;

			case BaseType::Kind::STRUCT_TEMPLATE_DEDUCER: {
				// TODO(FUTURE): handle this better?
				evo::debugAssert("Cannot get size of Struct Template Deducer");
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
				evo::debugAssert("Cannot get size of interface");
			} break;

			case BaseType::Kind::POLY_INTERFACE_REF: {
				return this->numBitsOfPtr() * 2;
			} break;

			case BaseType::Kind::INTERFACE_MAP: {
				const BaseType::InterfaceMap& interface_map_info = this->getInterfaceMap(id.interfaceMapID());
				return this->numBits(interface_map_info.underlyingTypeID, include_padding);
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
	// maxAtomicBytes


	auto TypeManager::maxAtomicNumBytes() const -> uint64_t {
		return 8;

		// aarch64    = 16
		// aarch64_be = 16
		// arm        = 4
		// armeb      = 4

		// riscv32    = 4
		// riscv64    = 8

		// spirv32    = 8
		// spirv64    = 8

		// wasm32     = 4
		// wasm64     = 8

		// x86        = 4
		// x86_64     = 8
	}


	auto TypeManager::maxAtomicNumBits() const -> uint64_t {
		return 8;

		// aarch64    = 128
		// aarch64_be = 128
		// arm        = 32
		// armeb      = 32

		// riscv32    = 32
		// riscv64    = 64

		// spirv32    = 64
		// spirv64    = 64

		// wasm32     = 32
		// wasm64     = 64

		// x86        = 32
		// x86_64     = 64
	}




	///////////////////////////////////
	// isDefaultInitializable

	auto TypeManager::isDefaultInitializable(TypeInfo::ID id) const -> bool {
		return this->special_member_prop_check<SpecialMember::DEFAULT_NEW, SpecialMemberProp::AT_ALL>(id, nullptr);
	}

	auto TypeManager::isDefaultInitializable(BaseType::ID id) const -> bool {
		return this->special_member_prop_check<SpecialMember::DEFAULT_NEW, SpecialMemberProp::AT_ALL>(id, nullptr);
	}


	///////////////////////////////////
	// isNoErrorDefaultInitializable

	auto TypeManager::isNoErrorDefaultInitializable(TypeInfo::ID id) const -> bool {
		return this->special_member_prop_check<SpecialMember::DEFAULT_NEW, SpecialMemberProp::NO_ERROR>(id, nullptr);
	}

	auto TypeManager::isNoErrorDefaultInitializable(BaseType::ID id) const -> bool {
		return this->special_member_prop_check<SpecialMember::DEFAULT_NEW, SpecialMemberProp::NO_ERROR>(id, nullptr);
	}




	///////////////////////////////////
	// isComptimeDefaultInitializable

	auto TypeManager::isComptimeDefaultInitializable(TypeInfo::ID id) const -> bool {
		return this->special_member_prop_check<SpecialMember::DEFAULT_NEW, SpecialMemberProp::COMPTIME>(id, nullptr);
	}

	auto TypeManager::isComptimeDefaultInitializable(BaseType::ID id) const -> bool {
		return this->special_member_prop_check<SpecialMember::DEFAULT_NEW, SpecialMemberProp::COMPTIME>(id, nullptr);
	}




	///////////////////////////////////
	// isTriviallyDefaultInitializable

	auto TypeManager::isTriviallyDefaultInitializable(TypeInfo::ID id) const -> bool {
		return this->special_member_prop_check<SpecialMember::DEFAULT_NEW, SpecialMemberProp::TRIVIAL>(id, nullptr);
	}

	auto TypeManager::isTriviallyDefaultInitializable(BaseType::ID id) const -> bool {
		return this->special_member_prop_check<SpecialMember::DEFAULT_NEW, SpecialMemberProp::TRIVIAL>(id, nullptr);
	}


	///////////////////////////////////
	// isSafeDefaultInitializable

	auto TypeManager::isSafeDefaultInitializable(TypeInfo::ID id) const -> bool {
		return this->special_member_prop_check<SpecialMember::DEFAULT_NEW, SpecialMemberProp::SAFE>(id, nullptr);
	}

	auto TypeManager::isSafeDefaultInitializable(BaseType::ID id) const -> bool {
		return this->special_member_prop_check<SpecialMember::DEFAULT_NEW, SpecialMemberProp::SAFE>(id, nullptr);
	}


	///////////////////////////////////
	// isTriviallyDeletable

	auto TypeManager::isTriviallyDeletable(TypeInfo::ID id) const -> bool {
		return this->special_member_prop_check<SpecialMember::DELETE, SpecialMemberProp::TRIVIAL>(id, nullptr);
	}

	auto TypeManager::isTriviallyDeletable(BaseType::ID id) const -> bool {
		return this->special_member_prop_check<SpecialMember::DELETE, SpecialMemberProp::TRIVIAL>(id, nullptr);
	}


	///////////////////////////////////
	// isComptimeDeletable

	auto TypeManager::isComptimeDeletable(TypeInfo::ID id, const SemaBuffer& sema_buffer) const -> bool {
		return this->special_member_prop_check<SpecialMember::DELETE, SpecialMemberProp::COMPTIME>(id, &sema_buffer);
	}

	auto TypeManager::isComptimeDeletable(BaseType::ID id, const SemaBuffer& sema_buffer) const -> bool {
		return this->special_member_prop_check<SpecialMember::DELETE, SpecialMemberProp::COMPTIME>(id, &sema_buffer);
	}


	///////////////////////////////////
	// isCopyable

	auto TypeManager::isCopyable(TypeInfo::ID id) const -> bool {
		return this->special_member_prop_check<SpecialMember::COPY, SpecialMemberProp::AT_ALL>(id, nullptr);
	}

	auto TypeManager::isCopyable(BaseType::ID id) const -> bool {
		return this->special_member_prop_check<SpecialMember::COPY, SpecialMemberProp::AT_ALL>(id, nullptr);
	}


	///////////////////////////////////
	// isTriviallyCopyable

	auto TypeManager::isTriviallyCopyable(TypeInfo::ID id) const -> bool {
		return this->special_member_prop_check<SpecialMember::COPY, SpecialMemberProp::TRIVIAL>(id, nullptr);
	}

	auto TypeManager::isTriviallyCopyable(BaseType::ID id) const -> bool {
		return this->special_member_prop_check<SpecialMember::COPY, SpecialMemberProp::TRIVIAL>(id, nullptr);
	}


	///////////////////////////////////
	// isComptimeCopyable

	auto TypeManager::isComptimeCopyable(TypeInfo::ID id, const SemaBuffer& sema_buffer) const -> bool {
		return this->special_member_prop_check<SpecialMember::COPY, SpecialMemberProp::COMPTIME>(id, &sema_buffer);
	}

	auto TypeManager::isComptimeCopyable(BaseType::ID id, const SemaBuffer& sema_buffer) const -> bool {
		return this->special_member_prop_check<SpecialMember::COPY, SpecialMemberProp::COMPTIME>(id, &sema_buffer);
	}


	///////////////////////////////////
	// isSafeCopyable

	auto TypeManager::isSafeCopyable(TypeInfo::ID id, const SemaBuffer& sema_buffer) const -> bool {
		return this->special_member_prop_check<SpecialMember::COPY, SpecialMemberProp::SAFE>(id, &sema_buffer);
	}

	auto TypeManager::isSafeCopyable(BaseType::ID id, const SemaBuffer& sema_buffer) const -> bool {
		return this->special_member_prop_check<SpecialMember::COPY, SpecialMemberProp::SAFE>(id, &sema_buffer);
	}



	///////////////////////////////////
	// isMovable

	auto TypeManager::isMovable(TypeInfo::ID id) const -> bool {
		return this->special_member_prop_check<SpecialMember::MOVE, SpecialMemberProp::AT_ALL>(id, nullptr);
	}

	auto TypeManager::isMovable(BaseType::ID id) const -> bool {
		return this->special_member_prop_check<SpecialMember::MOVE, SpecialMemberProp::AT_ALL>(id, nullptr);
	}


	///////////////////////////////////
	// isTriviallyMovable

	auto TypeManager::isTriviallyMovable(TypeInfo::ID id) const -> bool {
		return this->special_member_prop_check<SpecialMember::MOVE, SpecialMemberProp::TRIVIAL>(id, nullptr);
	}

	auto TypeManager::isTriviallyMovable(BaseType::ID id) const -> bool {
		return this->special_member_prop_check<SpecialMember::MOVE, SpecialMemberProp::TRIVIAL>(id, nullptr);
	}


	///////////////////////////////////
	// isComptimeMovable

	auto TypeManager::isComptimeMovable(TypeInfo::ID id, const SemaBuffer& sema_buffer) const -> bool {
		return this->special_member_prop_check<SpecialMember::MOVE, SpecialMemberProp::COMPTIME>(id, &sema_buffer);
	}

	auto TypeManager::isComptimeMovable(BaseType::ID id, const SemaBuffer& sema_buffer) const -> bool {
		return this->special_member_prop_check<SpecialMember::MOVE, SpecialMemberProp::COMPTIME>(id, &sema_buffer);
	}


	///////////////////////////////////
	// isSafeMovable

	auto TypeManager::isSafeMovable(TypeInfo::ID id, const SemaBuffer& sema_buffer) const -> bool {
		return this->special_member_prop_check<SpecialMember::MOVE, SpecialMemberProp::SAFE>(id, &sema_buffer);
	}

	auto TypeManager::isSafeMovable(BaseType::ID id, const SemaBuffer& sema_buffer) const -> bool {
		return this->special_member_prop_check<SpecialMember::MOVE, SpecialMemberProp::SAFE>(id, &sema_buffer);
	}





	///////////////////////////////////
	// isTriviallyComparable


	auto TypeManager::isTriviallyComparable(TypeInfo::ID id) const -> bool {
		const TypeInfo& type_info = this->getTypeInfo(id);
		if(type_info.qualifiers().empty() == false){ return type_info.qualifiers().back().isPtr; }

		return this->isTriviallyComparable(type_info.baseTypeID());
	}


	auto TypeManager::isTriviallyComparable(BaseType::ID id) const -> bool {
		switch(id.kind()){
			case BaseType::Kind::DUMMY: evo::debugFatalBreak("Invalid type");

			case BaseType::Kind::PRIMITIVE: {
				return !this->isFloatingPoint(id);
			} break;

			case BaseType::Kind::FUNCTION: {
				return true;
			} break;

			case BaseType::Kind::ARRAY: {
				const BaseType::Array& array_type = this->getArray(id.arrayID());
				if(this->isTriviallyComparable(array_type.elementTypeID) == false){ return false; }
				return this->numBits(array_type.elementTypeID, true) == this->numBits(array_type.elementTypeID, false);
			} break;

			case BaseType::Kind::ARRAY_REF: {
				return false;
			} break;

			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias_type = this->getAlias(id.aliasID());
				return this->isTriviallyComparable(alias_type.aliasedType);
			} break;

			case BaseType::Kind::DISTINCT_ALIAS: {
				const BaseType::DistinctAlias& distinct_alias_type = this->getDistinctAlias(id.distinctAliasID());
				return this->isTriviallyComparable(distinct_alias_type.underlyingType);
			} break;

			case BaseType::Kind::STRUCT: {
				const BaseType::Struct& struct_type = this->getStruct(id.structID());
				return struct_type.isTriviallyComparable;
			} break;

			case BaseType::Kind::UNION: {
				return false;
			} break;

			case BaseType::Kind::ENUM: {
				return true;
			} break;

			case BaseType::Kind::POLY_INTERFACE_REF: {
				return false;
			} break;

			case BaseType::Kind::ARRAY_DEDUCER:   case BaseType::Kind::ARRAY_REF_DEDUCER:
			case BaseType::Kind::STRUCT_TEMPLATE: case BaseType::Kind::STRUCT_TEMPLATE_DEDUCER:
			case BaseType::Kind::TYPE_DEDUCER:    case BaseType::Kind::INTERFACE:
			case BaseType::Kind::INTERFACE_MAP: {
				evo::debugFatalBreak("Invalid type to compare");
			} break;
		}

		evo::debugFatalBreak("Unknown base type kind");
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
			case Token::Kind::TYPE_BYTE:          return true;
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
			case Token::Kind::TYPE_BYTE:          return true;
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
	// isPointer


	auto TypeManager::isPointer(TypeInfo::VoidableID id) const -> bool {
		if(id.isVoid()){ return false; }
		return this->isPointer(id.asTypeID());
	}


	auto TypeManager::isPointer(TypeInfo::ID id) const -> bool {
		const TypeInfo& type_info = this->getTypeInfo(id);

		if(type_info.qualifiers().empty()){
			return this->isPointer(type_info.baseTypeID());
		}else{
			return type_info.isPointer();
		}
	}


	auto TypeManager::isPointer(BaseType::ID id) const -> bool {
		if(id.kind() != BaseType::Kind::PRIMITIVE){ return false; }
		return this->getPrimitive(id.primitiveID()).kind() == Token::Kind::TYPE_RAWPTR;
	}

	


	///////////////////////////////////
	// getUnderlyingType

	auto TypeManager::getUnderlyingType(TypeInfo::ID id) -> TypeInfo::ID {
		const TypeInfo& type_info = this->getTypeInfo(id);

		if(type_info.qualifiers().empty()){ return this->getUnderlyingType(type_info.baseTypeID()); }
		
		if(type_info.qualifiers().back().isPtr){
			return TypeManager::getTypeRawPtr();
		}else{
			return id;
		}
	}

	// TODO(PERF): optimize this function
	auto TypeManager::getUnderlyingType(BaseType::ID id) -> TypeInfo::ID {
		switch(id.kind()){
			case BaseType::Kind::DUMMY:             evo::debugFatalBreak("Dummy type should not be used");
			case BaseType::Kind::PRIMITIVE:         break;
			case BaseType::Kind::FUNCTION:          return this->getOrCreateTypeInfo(TypeInfo(id));
			case BaseType::Kind::ARRAY:             return this->getOrCreateTypeInfo(TypeInfo(id));
			case BaseType::Kind::ARRAY_DEDUCER:     evo::debugFatalBreak("Cannot get underlying type of this kind");
			case BaseType::Kind::ARRAY_REF:         return this->getOrCreateTypeInfo(TypeInfo(id));
			case BaseType::Kind::ARRAY_REF_DEDUCER: evo::debugFatalBreak("Cannot get underlying type of this kind");
			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias = this->getAlias(id.aliasID());
				return this->getUnderlyingType(alias.aliasedType);
			} break;
			case BaseType::Kind::DISTINCT_ALIAS: {
				const BaseType::DistinctAlias& distinct_alias_info = this->getDistinctAlias(id.distinctAliasID());
				return this->getUnderlyingType(distinct_alias_info.underlyingType);
			} break;
			case BaseType::Kind::STRUCT:          return this->getOrCreateTypeInfo(TypeInfo(id));
			case BaseType::Kind::STRUCT_TEMPLATE: evo::debugFatalBreak("Cannot get underlying type of this kind");
			case BaseType::Kind::STRUCT_TEMPLATE_DEDUCER:
				evo::debugFatalBreak("Cannot get underlying type of this kind");
			case BaseType::Kind::UNION:           return this->getOrCreateTypeInfo(TypeInfo(id));
			case BaseType::Kind::ENUM: {
				const BaseType::Enum& enum_type = this->getEnum(id.enumID());
				return this->getUnderlyingType(BaseType::ID(enum_type.underlyingTypeID));
			} break;
			case BaseType::Kind::TYPE_DEDUCER:       evo::debugFatalBreak("Cannot get underlying type of this kind");
			case BaseType::Kind::INTERFACE:          evo::debugFatalBreak("Cannot get underlying type of this kind");
			case BaseType::Kind::POLY_INTERFACE_REF: return this->getOrCreateTypeInfo(TypeInfo(id));
			case BaseType::Kind::INTERFACE_MAP: {
				const BaseType::InterfaceMap& interface_map_info = this->getInterfaceMap(id.interfaceMapID());
				return this->getUnderlyingType(interface_map_info.underlyingTypeID);
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




	template<bool DECAY_DISTINCT_ALIAS, bool DECAY_INTERFACE_MAP>
	auto TypeManager::decay_type_impl(TypeInfo::ID type_id) -> TypeInfo::ID {
		BaseType::ID base_type_id = BaseType::ID::dummy();
		auto qualifiers = evo::SmallVector<TypeInfo::Qualifier>();

		bool should_continue = true;
		while(should_continue){
			const TypeInfo& type_info = this->getTypeInfo(type_id);

			base_type_id = type_info.baseTypeID();

			for(const TypeInfo::Qualifier& qualifier : type_info.qualifiers() | std::views::reverse){
				qualifiers.insert(qualifiers.begin(), qualifier);
			}


			switch(base_type_id.kind()){
				case BaseType::Kind::ARRAY: {
					const BaseType::Array& array_type = this->getArray(type_info.baseTypeID().arrayID());

					base_type_id = this->getOrCreateArray(
						BaseType::Array(
							this->decay_type_impl<DECAY_DISTINCT_ALIAS, DECAY_INTERFACE_MAP>(
								array_type.elementTypeID
							),
							evo::copy(array_type.dimensions),
							evo::copy(array_type.terminator)
						)
					);

					should_continue = false;
				} break;

				case BaseType::Kind::ARRAY_REF: {
					const BaseType::ArrayRef& array_ref_type =
						this->getArrayRef(type_info.baseTypeID().arrayRefID());

					base_type_id = this->getOrCreateArrayRef(
						BaseType::ArrayRef(
							this->decay_type_impl<DECAY_DISTINCT_ALIAS, DECAY_INTERFACE_MAP>(
								array_ref_type.elementTypeID
							),
							evo::copy(array_ref_type.dimensions),
							evo::copy(array_ref_type.terminator),
							array_ref_type.isMut
						)
					);

					should_continue = false;
				} break;

				case BaseType::Kind::ALIAS: {
					const BaseType::Alias& alias = this->getAlias(type_info.baseTypeID().aliasID());
					type_id = alias.aliasedType;
				} break;

				case BaseType::Kind::DISTINCT_ALIAS: {
					if constexpr(DECAY_DISTINCT_ALIAS){
						const BaseType::DistinctAlias& distinct_alias = 
							this->getDistinctAlias(type_info.baseTypeID().distinctAliasID());

						type_id = distinct_alias.underlyingType;

					}else{
						should_continue = false;
					}
				} break;

				case BaseType::Kind::INTERFACE_MAP: {
					if constexpr(DECAY_INTERFACE_MAP){
						const BaseType::InterfaceMap& interface_map = 
							this->getInterfaceMap(type_info.baseTypeID().interfaceMapID());

						type_id = interface_map.underlyingTypeID;

					}else{
						should_continue = false;
					}
				} break;

				default: {
					should_continue = false;
				} break;
			}
		}

		return this->getOrCreateTypeInfo(TypeInfo(base_type_id, std::move(qualifiers)));
	}


	auto TypeManager::decay_type_false_false(TypeInfo::ID type_id) -> TypeInfo::ID {
		return this->decay_type_impl<false, false>(type_id);
	}

	auto TypeManager::decay_type_false_true(TypeInfo::ID type_id) -> TypeInfo::ID {
		return this->decay_type_impl<false, true>(type_id);
	}

	auto TypeManager::decay_type_true_false(TypeInfo::ID type_id) -> TypeInfo::ID {
		return this->decay_type_impl<true, false>(type_id);
	}

	auto TypeManager::decay_type_true_true(TypeInfo::ID type_id) -> TypeInfo::ID {
		return this->decay_type_impl<true, true>(type_id);
	}



}