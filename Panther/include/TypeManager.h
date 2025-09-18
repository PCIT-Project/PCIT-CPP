////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <shared_mutex>

#include <Evo.h>
#include <PCIT_core.h>
#include <PIR.h>

#include "./source/source_data.h"
#include "./tokens/Token.h"
#include "./AST/AST.h"
#include "../src/symbol_proc/symbol_proc_ids.h"
#include "./sema/Expr.h"
#include "../src/sema/ScopeLevel.h"
#include "./type_ids.h"





namespace pcit::panther{

	class SourceManager;


	//////////////////////////////////////////////////////////////////////
	// base type

	namespace BaseType{
		enum class Kind : uint32_t {
			DUMMY,

			PRIMITIVE,
			FUNCTION,
			ARRAY,
			ARRAY_REF,
			ALIAS,
			DISTINCT_ALIAS,
			STRUCT,
			STRUCT_TEMPLATE,
			UNION,
			TYPE_DEDUCER,
			INTERFACE,
		};


		struct ID{
			EVO_NODISCARD auto kind() const -> Kind { return this->_kind; }

			EVO_NODISCARD auto primitiveID() const -> PrimitiveID {
				evo::debugAssert(this->kind() == Kind::PRIMITIVE, "not a Primitive");
				return PrimitiveID(this->_id);
			}

			EVO_NODISCARD auto funcID() const -> FunctionID {
				evo::debugAssert(this->kind() == Kind::FUNCTION, "not a Function");
				return FunctionID(this->_id);
			}

			EVO_NODISCARD auto arrayID() const -> ArrayID {
				evo::debugAssert(this->kind() == Kind::ARRAY, "not a Array");
				return ArrayID(this->_id);
			}

			EVO_NODISCARD auto arrayRefID() const -> ArrayRefID {
				evo::debugAssert(this->kind() == Kind::ARRAY_REF, "not a ArrayRef");
				return ArrayRefID(this->_id);
			}

			EVO_NODISCARD auto aliasID() const -> AliasID {
				evo::debugAssert(this->kind() == Kind::ALIAS, "not an Alias");
				return AliasID(this->_id);
			}

			EVO_NODISCARD auto distinctAliasID() const -> DistinctAliasID {
				evo::debugAssert(this->kind() == Kind::DISTINCT_ALIAS, "not a DistinctAlias");
				return DistinctAliasID(this->_id);
			}

			EVO_NODISCARD auto structID() const -> StructID {
				evo::debugAssert(this->kind() == Kind::STRUCT, "not a Struct");
				return StructID(this->_id);
			}

			EVO_NODISCARD auto structTemplateID() const -> StructTemplateID {
				evo::debugAssert(this->kind() == Kind::STRUCT_TEMPLATE, "not a StructTemplate");
				return StructTemplateID(this->_id);
			}

			EVO_NODISCARD auto unionID() const -> UnionID {
				evo::debugAssert(this->kind() == Kind::UNION, "not a Union");
				return UnionID(this->_id);
			}

			EVO_NODISCARD auto typeDeducerID() const -> TypeDeducerID {
				evo::debugAssert(this->kind() == Kind::TYPE_DEDUCER, "not a type deducer");
				return TypeDeducerID(this->_id);
			}

			EVO_NODISCARD auto interfaceID() const -> InterfaceID {
				evo::debugAssert(this->kind() == Kind::INTERFACE, "not an interface");
				return InterfaceID(this->_id);
			}


			EVO_NODISCARD auto operator==(const ID&) const -> bool = default;


			EVO_NODISCARD static constexpr auto dummy() -> ID {
				return ID(Kind::DUMMY, std::numeric_limits<uint32_t>::max());
			}


			explicit ID(PrimitiveID id)      : _kind(Kind::PRIMITIVE),       _id(id.get()) {}
			explicit ID(FunctionID id)       : _kind(Kind::FUNCTION),        _id(id.get()) {}
			explicit ID(ArrayID id)          : _kind(Kind::ARRAY),           _id(id.get()) {}
			explicit ID(ArrayRefID id)       : _kind(Kind::ARRAY_REF),       _id(id.get()) {}
			explicit ID(AliasID id)          : _kind(Kind::ALIAS),           _id(id.get()) {}
			explicit ID(DistinctAliasID id)  : _kind(Kind::DISTINCT_ALIAS),  _id(id.get()) {}
			explicit ID(StructID id)         : _kind(Kind::STRUCT),          _id(id.get()) {}
			explicit ID(StructTemplateID id) : _kind(Kind::STRUCT_TEMPLATE), _id(id.get()) {}
			explicit ID(UnionID id)          : _kind(Kind::UNION),           _id(id.get()) {}
			explicit ID(TypeDeducerID id)    : _kind(Kind::TYPE_DEDUCER),    _id(id.get()) {}
			explicit ID(InterfaceID id)      : _kind(Kind::INTERFACE),       _id(id.get()) {}



			EVO_NODISCARD auto hash() const -> size_t {
				return std::hash<uint64_t>{}(evo::bitCast<uint64_t>(*this));
			}


			private:
				constexpr ID(Kind base_type_kind, uint32_t base_type_id) : _kind(base_type_kind), _id(base_type_id) {};

			private:
				Kind _kind;
				uint32_t _id;

				friend TypeManager;
				friend struct core::OptionalInterface<ID>;
		};

	}

}

namespace pcit::core{

	template<>
	struct OptionalInterface<panther::BaseType::ID>{
		static constexpr auto init(panther::BaseType::ID* id) -> void {
			new(id) panther::BaseType::ID(panther::BaseType::Kind::DUMMY, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::BaseType::ID& id) -> bool {
			return id._kind != panther::BaseType::Kind::DUMMY;
		}
	};

}


namespace std{


	template<>
	class optional<pcit::panther::BaseType::ID> : public pcit::core::Optional<pcit::panther::BaseType::ID>{
		public:
			using pcit::core::Optional<pcit::panther::BaseType::ID>::Optional;
			using pcit::core::Optional<pcit::panther::BaseType::ID>::operator=;
	};


	template<>
	struct hash<pcit::panther::BaseType::ID>{
		auto operator()(const pcit::panther::BaseType::ID& id) const noexcept -> size_t {
			return id.hash();
		};
	};

	
}




namespace pcit::panther{
	
	namespace BaseType{

		struct Primitive{
			using ID = PrimitiveID;

			EVO_NODISCARD auto kind() const -> Token::Kind { return this->_kind; }

			EVO_NODISCARD auto bitWidth() const -> uint32_t {
				evo::debugAssert(
					this->_kind == Token::Kind::TYPE_I_N || this->_kind == Token::Kind::TYPE_UI_N,
					"This type does not have a bit-width"
				);

				return this->bit_width;
			}

			EVO_NODISCARD auto operator==(const Primitive& rhs) const -> bool {
				return this->_kind == rhs._kind && this->bit_width == rhs.bit_width;
			}

			EVO_NODISCARD auto operator!=(const Primitive& rhs) const -> bool {
				return this->_kind != rhs._kind || this->bit_width != rhs.bit_width;
			}



			Primitive(Token::Kind tok_kind) : _kind(tok_kind), bit_width(0) {
				evo::debugAssert(
					this->_kind != Token::Kind::TYPE_I_N && this->_kind != Token::Kind::TYPE_UI_N,
					"This type requires a bit-width"
				);
			};

			Primitive(Token::Kind tok_kind, uint32_t _bit_width) : _kind(tok_kind), bit_width(_bit_width) {
				evo::debugAssert(
					this->_kind == Token::Kind::TYPE_I_N || this->_kind == Token::Kind::TYPE_UI_N,
					"This type does not have a bit-width"
				);
			};

			private:
				Token::Kind _kind;
				uint32_t bit_width;
		};

		struct Function{
			using ID = FunctionID;

			struct Param{
				TypeInfoID typeID;
				AST::FuncDecl::Param::Kind kind;
				bool shouldCopy;

				EVO_NODISCARD auto operator==(const Param&) const -> bool = default;
			};

			struct ReturnParam{
				std::optional<Token::ID> ident;
				TypeInfoVoidableID typeID;

				EVO_NODISCARD auto operator==(const ReturnParam& rhs) const -> bool {
					return this->ident.has_value() == rhs.ident.has_value() && this->typeID == rhs.typeID;
				}
			};

			evo::SmallVector<Param> params;
			evo::SmallVector<ReturnParam> returnParams;
			evo::SmallVector<ReturnParam> errorParams;


			EVO_NODISCARD auto hasNamedReturns() const -> bool { return this->returnParams[0].ident.has_value(); }
			EVO_NODISCARD auto returnsVoid() const -> bool { return this->returnParams[0].typeID.isVoid(); }

			EVO_NODISCARD auto hasErrorReturn() const -> bool { return !this->errorParams.empty(); }
			EVO_NODISCARD auto hasErrorReturnParams() const -> bool {
				return this->hasErrorReturn() && this->errorParams[0].typeID.isVoid() == false;
			}
			EVO_NODISCARD auto hasNamedErrorReturns() const -> bool {
				return this->hasErrorReturnParams() && this->errorParams[0].ident.has_value();
			}

			EVO_NODISCARD auto operator==(const Function&) const -> bool = default;
		};


		struct Array{
			using ID = ArrayID;
			
			TypeInfoID elementTypeID;
			evo::SmallVector<uint64_t> dimensions;
			std::optional<core::GenericValue> terminator;

			Array(
				TypeInfoID elem_type_id,
				evo::SmallVector<uint64_t>&& _dimensions,
				std::optional<core::GenericValue>&& _terminator
			) : elementTypeID(elem_type_id), dimensions(std::move(_dimensions)), terminator(std::move(_terminator)) {
				evo::debugAssert(
					!(this->dimensions.size() > 1 && this->terminator.has_value()),
					"multi-dimensional arrays cannot be terminated"
				);
			}

			EVO_NODISCARD auto operator==(const Array&) const -> bool = default;
		};


		struct ArrayRef{
			using ID = ArrayRefID;

			struct Dimension{
				explicit Dimension(uint64_t dimension_length) : _length(dimension_length) {}
				EVO_NODISCARD static auto ptr() -> Dimension { return Dimension(); }


				EVO_NODISCARD auto isPtr() const -> bool {
					return this->_length == std::numeric_limits<uint64_t>::max();
				}

				EVO_NODISCARD auto length() const -> uint64_t {
					evo::debugAssert(this->isPtr() == false, "Dimension is a ptr, so has no length");
					return this->_length;
				}


				EVO_NODISCARD auto operator==(const Dimension&) const -> bool = default;
				
				
				private:
					Dimension() : _length(std::numeric_limits<uint64_t>::max()) {}

					uint64_t _length;
			};


			
			TypeInfoID elementTypeID;
			evo::SmallVector<Dimension> dimensions;
			std::optional<core::GenericValue> terminator;
			bool isReadOnly;


			EVO_NODISCARD auto getNumRefPtrs() const -> size_t {
				size_t output = 0;
				for(const Dimension& dimension : this->dimensions){
					if(dimension.isPtr()){ output += 1; }
				}
				return output;
			}


			ArrayRef(
				TypeInfoID elem_type_id,
				evo::SmallVector<Dimension>&& _dimensions,
				std::optional<core::GenericValue>&& _terminator,
				bool is_read_only
			) : 
				elementTypeID(elem_type_id),
				dimensions(std::move(_dimensions)),
				terminator(std::move(_terminator)),
				isReadOnly(is_read_only) 
			{
				evo::debugAssert(
					!(this->dimensions.size() > 1 && this->terminator.has_value()),
					"multi-dimensional arrays cannot be terminated"
				);
			}

			EVO_NODISCARD auto operator==(const ArrayRef&) const -> bool = default;
		};


		static_assert(std::atomic<std::optional<TypeInfoID>>::is_always_lock_free);

		struct Alias{
			using ID = AliasID;

			evo::Variant<SourceID, ClangSourceID, BuiltinModuleID> sourceID;
			evo::Variant<Token::ID, ClangSourceDeclInfoID, BuiltinModuleStringID> name;
			std::atomic<std::optional<TypeInfoID>> aliasedType; // nullopt if only has decl completed
			bool isPub; // meaningless if not pthr source type

			EVO_NODISCARD auto defCompleted() const -> bool { return this->aliasedType.load().has_value(); }
			
			EVO_NODISCARD auto isPTHRSourceType() const -> bool { return this->sourceID.is<SourceID>(); }
			EVO_NODISCARD auto isClangType() const -> bool { return this->sourceID.is<ClangSourceID>(); }
			EVO_NODISCARD auto isBuiltinType() const -> bool { return this->sourceID.is<BuiltinModuleID>(); }
			EVO_NODISCARD auto getName(const class panther::SourceManager& source_manager) const -> std::string_view;

			EVO_NODISCARD auto operator==(const Alias& rhs) const -> bool {
				return this->sourceID == rhs.sourceID && this->name == rhs.name;
			}
		};


		struct DistinctAlias{
			using ID = DistinctAliasID;

			SourceID sourceID;
			Token::ID identTokenID;
			std::atomic<std::optional<TypeInfoID>> underlyingType; // nullopt if only has decl completed
			bool isPub;

			EVO_NODISCARD auto defCompleted() const -> bool { return this->underlyingType.load().has_value(); }
			
			EVO_NODISCARD auto operator==(const DistinctAlias& rhs) const -> bool {
				return this->sourceID == rhs.sourceID && this->identTokenID == rhs.identTokenID;
			}
		};


		struct Struct{
			using ID = StructID;

			struct MemberVar{
				AST::VarDecl::Kind kind;
				evo::Variant<Token::ID, ClangSourceDeclInfoID, BuiltinModuleStringID> name;
				TypeInfoID typeID;
				std::optional<sema::Expr> defaultValue;
			};

			evo::Variant<SourceID, ClangSourceID, BuiltinModuleID> sourceID;
			evo::Variant<Token::ID, ClangSourceDeclInfoID, BuiltinModuleStringID> name;
			std::optional<StructTemplateID> templateID = std::nullopt; // nullopt if not instantiated
			uint32_t instantiation = std::numeric_limits<uint32_t>::max(); // uin32_t max if not instantiation
			evo::SmallVector<MemberVar> memberVars; // make sure to take the lock (.memberVarsLock) when not defComplete
			evo::SmallVector<MemberVar*> memberVarsABI; // this is the order that members are for ABI
			SymbolProcNamespace* namespacedMembers; // nullptr if not pthr src type
			sema::ScopeLevel* scopeLevel; // nullptr if not pthr src type (although temporarily nullptr during creation)
			bool isPub; // meaningless if not pthr src type
			bool isOrdered; // TODO(FUTURE): is this needed here?
			bool isPacked;
			bool shouldLower = true; // may only be false if is builtin type

			std::atomic<bool> defCompleted = false; // includes PIR lowering

			mutable core::SpinLock memberVarsLock{}; // only needed before definition is completed

			evo::SmallVector<sema::FuncID> newInitOverloads{};
			mutable core::SpinLock newInitOverloadsLock{}; // only needed before def completed

			evo::SmallVector<sema::FuncID> newReassignOverloads{};
			mutable core::SpinLock newReassignOverloadsLock{}; // only needed before def completed

			std::unordered_map<TypeInfoID, sema::FuncID> operatorAsOverloads{};
			mutable core::SpinLock operatorAsOverloadsLock{}; // only needed before def completed

			bool isDefaultInitializable = false;
			bool isTriviallyDefaultInitializable = false;
			bool isConstexprDefaultInitializable = false;
			bool isNoErrorDefaultInitializable = false;

			EVO_NODISCARD auto isPTHRSourceType() const -> bool { return this->sourceID.is<SourceID>(); }
			EVO_NODISCARD auto isClangType() const -> bool { return this->sourceID.is<ClangSourceID>(); }
			EVO_NODISCARD auto isBuiltinType() const -> bool { return this->sourceID.is<BuiltinModuleID>(); }
			EVO_NODISCARD auto getName(const class panther::SourceManager& source_manager) const -> std::string_view;

			EVO_NODISCARD auto getMemberName(
				const MemberVar& member, const class panther::SourceManager& source_manager
			) const -> std::string_view;

			EVO_NODISCARD auto operator==(const Struct& rhs) const -> bool {
				return this->sourceID == rhs.sourceID
					&& this->name == rhs.name
					&& this->instantiation == rhs.instantiation;
			}
		};


		struct StructTemplate{
			using ID = StructTemplateID;
			using Arg = evo::Variant<TypeInfoVoidableID, core::GenericValue>;

			struct Instantiation{
				std::atomic<std::optional<SymbolProcID>> symbolProcID{}; // nullopt means its being generated
				std::optional<BaseType::Struct::ID> structID{}; // nullopt means it's being worked on
				std::atomic<bool> errored = false;

				Instantiation() = default;
				Instantiation(const Instantiation&) = delete;
			};

			struct Param{
				const AST::Type& astType;
				std::optional<TypeInfoID> typeID;
				evo::Variant<std::monostate, sema::Expr, TypeInfoVoidableID> defaultValue; // monostate if no default

				EVO_NODISCARD auto isType() const -> bool { return this->typeID.has_value() == false; }
				EVO_NODISCARD auto isExpr() const -> bool { return this->typeID.has_value(); }

				EVO_NODISCARD auto operator==(const Param& rhs) const -> bool { return this->typeID == rhs.typeID; };
			};


			SourceID& sourceID;
			Token::ID identTokenID;
			evo::SmallVector<Param> params;
			size_t minNumTemplateArgs; // TODO(PERF): make sure this optimization actually improves perf

			struct InstantiationInfo{
				Instantiation& instantiation;
				std::optional<uint32_t> instantiationID; // only has value if it needs to be compiled

				EVO_NODISCARD auto needsToBeCompiled() const -> bool { return this->instantiationID.has_value(); }
			};
			EVO_NODISCARD auto lookupInstantiation(evo::SmallVector<Arg>&& args) -> InstantiationInfo;

			EVO_NODISCARD auto hasAnyDefaultParams() const -> bool {
				return this->minNumTemplateArgs != this->params.size();
			}


			EVO_NODISCARD auto getInstantiationArgs(uint32_t instantiation_id) const -> evo::SmallVector<Arg>;


			auto operator==(const StructTemplate& rhs) const -> bool {
				return this->sourceID == rhs.sourceID 
					&& this->identTokenID == rhs.identTokenID
					&& this->params == rhs.params;
			}


			StructTemplate(
				SourceID source_id,
				Token::ID ident_token_id,
				evo::SmallVector<Param>&& _params,
				size_t min_num_template_args
			) : 
				sourceID(source_id), 
				identTokenID(ident_token_id),
				params(std::move(_params)), 
				minNumTemplateArgs(min_num_template_args) 
			{}

			private:
				core::LinearStepAlloc<Instantiation, size_t> instantiations{};
				std::unordered_map<evo::SmallVector<Arg>, Instantiation&> instantiation_map{};
				mutable core::SpinLock instantiation_lock{};
		};



		struct Union{
			using ID = UnionID;

			struct Field{
				evo::Variant<Token::ID, ClangSourceDeclInfoID> location;
				TypeInfoVoidableID typeID;
			};
			
			evo::Variant<SourceID, ClangSourceID> sourceID;
			evo::Variant<Token::ID, ClangSourceDeclInfoID> location;
			evo::SmallVector<Field> fields;
			SymbolProcNamespace* namespacedMembers; // nullptr if is clang type
			sema::ScopeLevel* scopeLevel; // nullopt if is clang type (although temporarily nullopt during creation)
			bool isPub; // meaningless if clang type
			bool isUntagged;

			std::atomic<bool> defCompleted = false;


			EVO_NODISCARD auto getName(const class panther::SourceManager& source_manager) const -> std::string_view;
			EVO_NODISCARD auto isClangType() const -> bool { return this->sourceID.is<ClangSourceID>(); }

			EVO_NODISCARD auto getFieldName(
				const Field& field, const class panther::SourceManager& source_manager
			) const -> std::string_view;

			auto operator==(const Union& rhs) const -> bool {
				return this->sourceID == rhs.sourceID && this->location == rhs.location;
			}
		};



		// TODO(FUTURE): is `.sourceID` ever actually used (outside of operator==),
		// 		and can this `identTokenID` be "inlined" into ID?
		struct TypeDeducer{
			using ID = TypeDeducerID;

			Token::ID identTokenID;
			SourceID sourceID;

			auto operator==(const TypeDeducer& rhs) const -> bool {
				return this->identTokenID == rhs.identTokenID && this->sourceID == rhs.sourceID;
			}
		};



		struct Interface{
			using ID = InterfaceID;

			struct Impl{
				evo::SmallVector<sema::FuncID> methods;
			};
			
			Token::ID identTokenID;
			SourceID sourceID;
			SymbolProcID symbolProcID;
			bool isPub;

			evo::SmallVector<sema::FuncID> methods{};

			std::unordered_map<BaseType::ID, const Impl&> impls{};
			mutable core::SpinLock implsLock{};

			std::atomic<bool> defCompleted = false;

			auto operator==(const Interface& rhs) const -> bool {
				return this->identTokenID == rhs.identTokenID && this->sourceID == rhs.sourceID;
			}
		};

	};


	class TypeInfo{
		public:
			using ID = TypeInfoID;
			using VoidableID = TypeInfoVoidableID;
			
		public:
			explicit TypeInfo(const BaseType::ID& id) : base_type(id), _qualifiers() {};
			TypeInfo(const BaseType::ID& id, const evo::SmallVector<AST::Type::Qualifier>& qualifiers_list)
				: base_type(id), _qualifiers(qualifiers_list) {};
			TypeInfo(const BaseType::ID& id, evo::SmallVector<AST::Type::Qualifier>&& qualifiers_list)
				: base_type(id), _qualifiers(std::move(qualifiers_list)) {};
			~TypeInfo() = default;


			EVO_NODISCARD auto baseTypeID() const -> BaseType::ID { return this->base_type; }
			EVO_NODISCARD auto qualifiers() const -> evo::ArrayProxy<AST::Type::Qualifier> { return this->_qualifiers; }

			EVO_NODISCARD auto operator==(const TypeInfo&) const -> bool = default;

			EVO_NODISCARD auto isPointer() const -> bool {
				return this->qualifiers().empty() == false && this->qualifiers().back().isPtr;
			}

			EVO_NODISCARD auto isNormalPointer() const -> bool {
				return this->isPointer() && this->base_type.kind() != BaseType::Kind::INTERFACE;
			}

			EVO_NODISCARD auto isInterfacePointer() const -> bool {
				return this->_qualifiers.size() == 1
					&& this->isPointer()
					&& this->base_type.kind() == BaseType::Kind::INTERFACE;
			}

			EVO_NODISCARD auto isInterface() const -> bool {
				return this->_qualifiers.empty() && this->base_type.kind() == BaseType::Kind::INTERFACE;
			}


			EVO_NODISCARD auto isOptional() const -> bool {
				return this->qualifiers().empty() == false && this->qualifiers().back().isOptional;
			}

			EVO_NODISCARD auto isOptionalNotPointer() const -> bool {
				return this->qualifiers().empty() == false
					&& this->qualifiers().back().isPtr == false
					&& this->qualifiers().back().isOptional;
			}

			EVO_NODISCARD auto isUninitPointer() const -> bool {
				return this->qualifiers().empty() == false && this->qualifiers().back().isUninit;
			}


			EVO_NODISCARD auto copyWithPushedQualifier(AST::Type::Qualifier qualifier) const -> TypeInfo {
				TypeInfo copied_type = *this;
				copied_type._qualifiers.emplace_back(qualifier);
				return copied_type;
			}

			EVO_NODISCARD auto copyWithPoppedQualifier() const -> TypeInfo {
				TypeInfo copied_type = *this;
				copied_type._qualifiers.pop_back();
				return copied_type;
			}
	
		private:
			BaseType::ID base_type;
			evo::SmallVector<AST::Type::Qualifier> _qualifiers;
	};

	class TypeManager{
		public:
			TypeManager(core::Target target) : _target(target) {};
			~TypeManager() = default;


			auto initPrimitives() -> void; // single-threaded
			EVO_NODISCARD auto primitivesInitialized() const -> bool; // single-threaded


			EVO_NODISCARD auto getTarget() const -> const core::Target& { return this->_target; }


			EVO_NODISCARD auto getTypeInfo(TypeInfo::ID id) const -> const TypeInfo&;
			EVO_NODISCARD auto getOrCreateTypeInfo(TypeInfo&& lookup_type_info) -> TypeInfo::ID;
				
			EVO_NODISCARD auto printType(
				TypeInfo::VoidableID type_info_id, const class SourceManager& source_manager
			) const -> std::string;
			EVO_NODISCARD auto printType(
				TypeInfo::ID type_info_id, const class SourceManager& source_manager
			) const -> std::string;

			EVO_NODISCARD auto getFunction(BaseType::Function::ID id) const -> const BaseType::Function&;
			EVO_NODISCARD auto getFunction(BaseType::Function::ID id)       ->       BaseType::Function&;
			EVO_NODISCARD auto getOrCreateFunction(BaseType::Function&& lookup_func) -> BaseType::ID;

			EVO_NODISCARD auto getArray(BaseType::Array::ID id) const -> const BaseType::Array&;
			EVO_NODISCARD auto getOrCreateArray(BaseType::Array&& lookup_type) -> BaseType::ID;

			EVO_NODISCARD auto getArrayRef(BaseType::ArrayRef::ID id) const -> const BaseType::ArrayRef&;
			EVO_NODISCARD auto getOrCreateArrayRef(BaseType::ArrayRef&& lookup_type) -> BaseType::ID;

			EVO_NODISCARD auto getPrimitive(BaseType::Primitive::ID id) const -> const BaseType::Primitive&;
			EVO_NODISCARD auto getOrCreatePrimitiveBaseType(Token::Kind kind) -> BaseType::ID;
			EVO_NODISCARD auto getOrCreatePrimitiveBaseType(Token::Kind kind, uint32_t bit_width) -> BaseType::ID;

			EVO_NODISCARD auto getAlias(BaseType::Alias::ID id) const -> const BaseType::Alias&;
			EVO_NODISCARD auto getAlias(BaseType::Alias::ID id)       ->       BaseType::Alias&;
			EVO_NODISCARD auto getOrCreateAlias(BaseType::Alias&& lookup_type) -> BaseType::ID;

			EVO_NODISCARD auto getDistinctAlias(BaseType::DistinctAlias::ID id) const -> const BaseType::DistinctAlias&;
			EVO_NODISCARD auto getDistinctAlias(BaseType::DistinctAlias::ID id)       ->       BaseType::DistinctAlias&;
			EVO_NODISCARD auto getOrCreateDistinctAlias(BaseType::DistinctAlias&& lookup_type) -> BaseType::ID;

			EVO_NODISCARD auto getStruct(BaseType::Struct::ID id) const -> const BaseType::Struct&;
			EVO_NODISCARD auto getStruct(BaseType::Struct::ID id)       ->       BaseType::Struct&;
			EVO_NODISCARD auto getOrCreateStruct(BaseType::Struct&& lookup_type) -> BaseType::ID;
			EVO_NODISCARD auto getNumStructs() const -> size_t; // I don't love this design

			EVO_NODISCARD auto getStructTemplate(BaseType::StructTemplate::ID id) const
				-> const BaseType::StructTemplate&;
			EVO_NODISCARD auto getStructTemplate(BaseType::StructTemplate::ID id) -> BaseType::StructTemplate&;
			EVO_NODISCARD auto getOrCreateStructTemplate(BaseType::StructTemplate&& lookup_type) -> BaseType::ID;

			EVO_NODISCARD auto getUnion(BaseType::Union::ID id) const -> const BaseType::Union&;
			EVO_NODISCARD auto getUnion(BaseType::Union::ID id)       ->       BaseType::Union&;
			EVO_NODISCARD auto getOrCreateUnion(BaseType::Union&& lookup_type) -> BaseType::ID;
			EVO_NODISCARD auto getNumUnions() const -> size_t; // I don't love this design

			EVO_NODISCARD auto getTypeDeducer(BaseType::TypeDeducer::ID id) const -> const BaseType::TypeDeducer&;
			EVO_NODISCARD auto getOrCreateTypeDeducer(BaseType::TypeDeducer&& lookup_type) -> BaseType::ID;

			EVO_NODISCARD auto getInterface(BaseType::Interface::ID id) const -> const BaseType::Interface&;
			EVO_NODISCARD auto getInterface(BaseType::Interface::ID id)       ->       BaseType::Interface&;
			EVO_NODISCARD auto getOrCreateInterface(BaseType::Interface&& lookup_type) -> BaseType::ID;
			EVO_NODISCARD auto getNumInterfaces() const -> size_t; // I don't love this design
			EVO_NODISCARD auto createInterfaceImpl() -> BaseType::Interface::Impl&;
			
			
			EVO_NODISCARD static auto getTypeBool()   -> TypeInfo::ID { return TypeInfo::ID(0);  }
			EVO_NODISCARD static auto getTypeChar()   -> TypeInfo::ID { return TypeInfo::ID(1);  }
			EVO_NODISCARD static auto getTypeUI8()    -> TypeInfo::ID { return TypeInfo::ID(2);  }
			EVO_NODISCARD static auto getTypeUI16()   -> TypeInfo::ID { return TypeInfo::ID(3);  }
			EVO_NODISCARD static auto getTypeUI32()   -> TypeInfo::ID { return TypeInfo::ID(4);  }
			EVO_NODISCARD static auto getTypeUI64()   -> TypeInfo::ID { return TypeInfo::ID(5);  }
			EVO_NODISCARD static auto getTypeUSize()  -> TypeInfo::ID { return TypeInfo::ID(6);  }
			EVO_NODISCARD static auto getTypeTypeID() -> TypeInfo::ID { return TypeInfo::ID(7);  }
			EVO_NODISCARD static auto getTypeRawPtr() -> TypeInfo::ID { return TypeInfo::ID(8);  }
			EVO_NODISCARD static auto getTypeI256()   -> TypeInfo::ID { return TypeInfo::ID(9);  }
			EVO_NODISCARD static auto getTypeF128()   -> TypeInfo::ID { return TypeInfo::ID(10); } 
			EVO_NODISCARD static auto getTypeByte()   -> TypeInfo::ID { return TypeInfo::ID(11); } 


			EVO_NODISCARD auto isTypeDeducer(TypeInfo::ID id) const -> bool;


			///////////////////////////////////
			// type traits

			//////////////////
			// size

			EVO_NODISCARD auto numBytes(TypeInfo::ID id, bool include_padding = true) const -> uint64_t;
			EVO_NODISCARD auto numBytes(BaseType::ID id, bool include_padding = true) const -> uint64_t;
			EVO_NODISCARD auto numBytesOfPtr() const -> uint64_t;
			EVO_NODISCARD auto numBytesOfGeneralRegister() const -> uint64_t;

			EVO_NODISCARD auto numBits(TypeInfo::ID id, bool include_padding = true) const -> uint64_t;
			EVO_NODISCARD auto numBits(BaseType::ID id, bool include_padding = true) const -> uint64_t;
			EVO_NODISCARD auto numBitsOfPtr() const -> uint64_t;
			EVO_NODISCARD auto numBitsOfGeneralRegister() const -> uint64_t;

			EVO_NODISCARD auto isTriviallySized(TypeInfo::ID id) const -> bool;
			EVO_NODISCARD auto isTriviallySized(BaseType::ID id) const -> bool;


			//////////////////
			// operations

			EVO_NODISCARD auto isDefaultInitializable(TypeInfo::ID id) const -> bool;
			EVO_NODISCARD auto isDefaultInitializable(BaseType::ID id) const -> bool;

			EVO_NODISCARD auto isConstexprDefaultInitializable(TypeInfo::ID id) const -> bool;
			EVO_NODISCARD auto isConstexprDefaultInitializable(BaseType::ID id) const -> bool;

			EVO_NODISCARD auto isNoErrorDefaultInitializable(TypeInfo::ID id) const -> bool;
			EVO_NODISCARD auto isNoErrorDefaultInitializable(BaseType::ID id) const -> bool;

			EVO_NODISCARD auto isTriviallyDefaultInitializable(TypeInfo::ID id) const -> bool;
			EVO_NODISCARD auto isTriviallyDefaultInitializable(BaseType::ID id) const -> bool;

			EVO_NODISCARD auto isTriviallyDeletable(TypeInfo::ID id) const -> bool;
			EVO_NODISCARD auto isTriviallyDeletable(BaseType::ID id) const -> bool;

			EVO_NODISCARD auto isCopyable(TypeInfo::ID id) const -> bool;
			EVO_NODISCARD auto isCopyable(BaseType::ID id) const -> bool;

			EVO_NODISCARD auto isTriviallyCopyable(TypeInfo::ID id) const -> bool;
			EVO_NODISCARD auto isTriviallyCopyable(BaseType::ID id) const -> bool;

			EVO_NODISCARD auto isMovable(TypeInfo::ID id) const -> bool;
			EVO_NODISCARD auto isMovable(BaseType::ID id) const -> bool;

			EVO_NODISCARD auto isTriviallyMovable(TypeInfo::ID id) const -> bool;
			EVO_NODISCARD auto isTriviallyMovable(BaseType::ID id) const -> bool;


			//////////////////
			// primitive type categories

			EVO_NODISCARD auto isPrimitive(TypeInfo::VoidableID id) const -> bool;
			EVO_NODISCARD auto isPrimitive(TypeInfo::ID id) const -> bool;
			EVO_NODISCARD auto isPrimitive(BaseType::ID id) const -> bool;

			EVO_NODISCARD auto isIntegral(TypeInfo::VoidableID id) const -> bool;
			EVO_NODISCARD auto isIntegral(TypeInfo::ID id) const -> bool;
			EVO_NODISCARD auto isIntegral(BaseType::ID id) const -> bool;

			EVO_NODISCARD auto isUnsignedIntegral(TypeInfo::VoidableID id) const -> bool;
			EVO_NODISCARD auto isUnsignedIntegral(TypeInfo::ID id) const -> bool;
			EVO_NODISCARD auto isUnsignedIntegral(BaseType::ID id) const -> bool;

			EVO_NODISCARD auto isSignedIntegral(TypeInfo::VoidableID id) const -> bool;
			EVO_NODISCARD auto isSignedIntegral(TypeInfo::ID id) const -> bool;
			EVO_NODISCARD auto isSignedIntegral(BaseType::ID id) const -> bool;

			EVO_NODISCARD auto isFloatingPoint(TypeInfo::VoidableID id) const -> bool;
			EVO_NODISCARD auto isFloatingPoint(TypeInfo::ID id) const -> bool;
			EVO_NODISCARD auto isFloatingPoint(BaseType::ID id) const -> bool;

			EVO_NODISCARD auto getUnderlyingType(TypeInfo::ID id) -> TypeInfo::ID; // yes, this operation is not const
			EVO_NODISCARD auto getUnderlyingType(BaseType::ID id) -> TypeInfo::ID; // yes, this operation is not const


			//////////////////
			// numeric limits

			EVO_NODISCARD auto getMin(TypeInfo::ID id) const -> core::GenericValue;
			EVO_NODISCARD auto getMin(BaseType::ID id) const -> core::GenericValue;

			EVO_NODISCARD auto getNormalizedMin(TypeInfo::ID id) const -> core::GenericValue;
			EVO_NODISCARD auto getNormalizedMin(BaseType::ID id) const -> core::GenericValue;

			EVO_NODISCARD auto getMax(TypeInfo::ID id) const -> core::GenericValue;
			EVO_NODISCARD auto getMax(BaseType::ID id) const -> core::GenericValue;


		private:
			EVO_NODISCARD auto get_or_create_primitive_base_type_impl(const BaseType::Primitive& lookup_type)
				-> BaseType::ID;


		private:
			core::Target _target;

			// TODO(PERF): improve lookup times
			core::LinearStepAlloc<BaseType::Primitive, BaseType::Primitive::ID> primitives{};
			mutable core::SpinLock primitives_lock{};

			core::LinearStepAlloc<BaseType::Function, BaseType::Function::ID> functions{};
			mutable core::SpinLock functions_lock{};

			core::LinearStepAlloc<BaseType::Array, BaseType::Array::ID> arrays{};
			mutable core::SpinLock arrays_lock{};

			core::LinearStepAlloc<BaseType::ArrayRef, BaseType::ArrayRef::ID> array_refs{};
			mutable core::SpinLock array_refs_lock{};

			core::LinearStepAlloc<BaseType::Alias, BaseType::Alias::ID> aliases{};
			mutable core::SpinLock aliases_lock{};

			core::LinearStepAlloc<BaseType::DistinctAlias, BaseType::DistinctAlias::ID> distinct_aliases{};
			mutable core::SpinLock distinct_aliases_lock{};

			core::LinearStepAlloc<BaseType::Struct, BaseType::Struct::ID> structs{};
			mutable core::SpinLock structs_lock{};

			core::LinearStepAlloc<BaseType::StructTemplate, BaseType::StructTemplate::ID> struct_templates{};
			mutable core::SpinLock struct_templates_lock{};

			core::LinearStepAlloc<BaseType::TypeDeducer, BaseType::TypeDeducer::ID> type_deducers{};
			mutable core::SpinLock type_deducers_lock{};

			core::LinearStepAlloc<BaseType::Union, BaseType::Union::ID> unions{};
			mutable core::SpinLock unions_lock{};

			core::LinearStepAlloc<BaseType::Interface, BaseType::Interface::ID> interfaces{};
			mutable core::SpinLock interfaces_lock{};
			core::SyncLinearStepAlloc<BaseType::Interface::Impl, uint64_t> interface_impls{};

			core::LinearStepAlloc<TypeInfo, TypeInfo::ID> types{};
			mutable core::SpinLock types_lock{};
	};

}
