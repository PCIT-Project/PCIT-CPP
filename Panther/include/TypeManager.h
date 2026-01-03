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
	class Context;


	struct EncapsulatingSymbolID{
		struct InterfaceImplInfo{
			TypeInfoID targetTypeID;
			BaseType::InterfaceID interfaceID;

			EVO_NODISCARD auto operator==(const InterfaceImplInfo&) const -> bool = default;
		};

		template<class T>
		EVO_NODISCARD auto is() const -> bool { return this->id.is<T>(); }

		template<class T>
		EVO_NODISCARD auto as() const -> const T& { return this->id.as<T>(); }

		template<class T>
		EVO_NODISCARD auto as() -> T& { return this->id.as<T>(); }


		auto visit(auto callable) const -> auto { return this->id.visit(callable); }
		auto visit(auto callable) -> auto { return this->id.visit(callable); }


		EVO_NODISCARD auto operator==(const EncapsulatingSymbolID&) const -> bool = default;


		evo::Variant<
			BaseType::StructID,
			BaseType::UnionID,
			BaseType::EnumID,
			BaseType::InterfaceID,
			sema::FuncID,
			InterfaceImplInfo
		> id;
	};


	//////////////////////////////////////////////////////////////////////
	// base type

	namespace BaseType{
		enum class Kind : uint32_t {
			DUMMY,

			PRIMITIVE,
			FUNCTION,
			ARRAY,
			ARRAY_DEDUCER,
			ARRAY_REF,
			ALIAS,
			DISTINCT_ALIAS,
			STRUCT,
			STRUCT_TEMPLATE,
			STRUCT_TEMPLATE_DEDUCER,
			UNION,
			ENUM,
			TYPE_DEDUCER,
			INTERFACE,
			POLY_INTERFACE_REF,
			INTERFACE_MAP,
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
				evo::debugAssert(this->kind() == Kind::ARRAY, "not an Array");
				return ArrayID(this->_id);
			}

			EVO_NODISCARD auto arrayDeducerID() const -> ArrayDeducerID {
				evo::debugAssert(this->kind() == Kind::ARRAY_DEDUCER, "not an ArrayDeducer");
				return ArrayDeducerID(this->_id);
			}

			EVO_NODISCARD auto arrayRefID() const -> ArrayRefID {
				evo::debugAssert(this->kind() == Kind::ARRAY_REF, "not an ArrayRef");
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

			EVO_NODISCARD auto structTemplateDeducerID() const -> StructTemplateDeducerID {
				evo::debugAssert(this->kind() == Kind::STRUCT_TEMPLATE_DEDUCER, "not a StructTemplateDeducer");
				return StructTemplateDeducerID(this->_id);
			}

			EVO_NODISCARD auto unionID() const -> UnionID {
				evo::debugAssert(this->kind() == Kind::UNION, "not a Union");
				return UnionID(this->_id);
			}

			EVO_NODISCARD auto enumID() const -> EnumID {
				evo::debugAssert(this->kind() == Kind::ENUM, "not a Enum");
				return EnumID(this->_id);
			}

			EVO_NODISCARD auto typeDeducerID() const -> TypeDeducerID {
				evo::debugAssert(this->kind() == Kind::TYPE_DEDUCER, "not a type deducer");
				return TypeDeducerID(this->_id);
			}

			EVO_NODISCARD auto interfaceID() const -> InterfaceID {
				evo::debugAssert(this->kind() == Kind::INTERFACE, "not an interface");
				return InterfaceID(this->_id);
			}

			EVO_NODISCARD auto polyInterfaceRefID() const -> PolyInterfaceRefID {
				evo::debugAssert(this->kind() == Kind::POLY_INTERFACE_REF, "not a poly interface ref");
				return PolyInterfaceRefID(this->_id);
			}

			EVO_NODISCARD auto interfaceMapID() const -> InterfaceMapID {
				evo::debugAssert(this->kind() == Kind::INTERFACE_MAP, "not an interface map");
				return InterfaceMapID(this->_id);
			}


			EVO_NODISCARD auto operator==(const ID&) const -> bool = default;


			EVO_NODISCARD static constexpr auto dummy() -> ID {
				return ID(Kind::DUMMY, std::numeric_limits<uint32_t>::max());
			}


			explicit ID(PrimitiveID id)             : _kind(Kind::PRIMITIVE),               _id(id.get()) {}
			explicit ID(FunctionID id)              : _kind(Kind::FUNCTION),                _id(id.get()) {}
			explicit ID(ArrayID id)                 : _kind(Kind::ARRAY),                   _id(id.get()) {}
			explicit ID(ArrayDeducerID id)          : _kind(Kind::ARRAY_DEDUCER),           _id(id.get()) {}
			explicit ID(ArrayRefID id)              : _kind(Kind::ARRAY_REF),               _id(id.get()) {}
			explicit ID(AliasID id)                 : _kind(Kind::ALIAS),                   _id(id.get()) {}
			explicit ID(DistinctAliasID id)         : _kind(Kind::DISTINCT_ALIAS),          _id(id.get()) {}
			explicit ID(StructID id)                : _kind(Kind::STRUCT),                  _id(id.get()) {}
			explicit ID(StructTemplateID id)        : _kind(Kind::STRUCT_TEMPLATE),         _id(id.get()) {}
			explicit ID(StructTemplateDeducerID id) : _kind(Kind::STRUCT_TEMPLATE_DEDUCER), _id(id.get()) {}
			explicit ID(UnionID id)                 : _kind(Kind::UNION),                   _id(id.get()) {}
			explicit ID(EnumID id)                  : _kind(Kind::ENUM),                    _id(id.get()) {}
			explicit ID(TypeDeducerID id)           : _kind(Kind::TYPE_DEDUCER),            _id(id.get()) {}
			explicit ID(InterfaceID id)             : _kind(Kind::INTERFACE),               _id(id.get()) {}
			explicit ID(PolyInterfaceRefID id)      : _kind(Kind::POLY_INTERFACE_REF),      _id(id.get()) {}
			explicit ID(InterfaceMapID id)          : _kind(Kind::INTERFACE_MAP),           _id(id.get()) {}



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
				enum class Kind{
					READ,
					MUT,
					IN,
					C,
				};

				TypeInfoID typeID;
				Kind kind;
				bool shouldCopy;

				EVO_NODISCARD auto operator==(const Param&) const -> bool = default;
			};

			evo::SmallVector<Param> params;
			evo::SmallVector<TypeInfoVoidableID> returnTypes;
			evo::SmallVector<TypeInfoVoidableID> errorTypes;
			bool hasNamedReturns;
			bool hasNamedErrorReturns;


			Function(
				evo::SmallVector<Param>&& _params,
				evo::SmallVector<TypeInfoVoidableID>&& returns_types,
				evo::SmallVector<TypeInfoVoidableID>&& error_types,
				bool has_named_returns,
				bool has_named_error_returns
			) : 
				params(std::move(_params)),
				returnTypes(std::move(returns_types)),
				errorTypes(std::move(error_types)),
				hasNamedReturns(has_named_returns),
				hasNamedErrorReturns(has_named_error_returns)
			{
				evo::debugAssert(
					this->hasNamedReturns == false || this->returnsVoid() == false,
					"Type `Void` cannot be named"
				);
				evo::debugAssert(
					this->hasNamedErrorReturns == false || this->hasErrorReturnParams(),
					"Cannot have named error returns without error returns"
				);
			}


			EVO_NODISCARD auto returnsVoid() const -> bool { return this->returnTypes[0].isVoid(); }

			EVO_NODISCARD auto hasErrorReturn() const -> bool { return !this->errorTypes.empty(); }
			EVO_NODISCARD auto hasErrorReturnParams() const -> bool {
				return this->hasErrorReturn() && this->errorTypes[0].isVoid() == false;
			}

			EVO_NODISCARD auto isImplicitRVO(const class TypeManager& type_manager) const -> bool; // only if Panther

			EVO_NODISCARD auto operator==(const Function&) const -> bool = default;
		};


		struct Array{
			using ID = ArrayID;
			
			TypeInfoID elementTypeID;
			evo::SmallVector<uint64_t> dimensions;
			std::optional<core::GenericValue> terminator; // core::GenericValue not sema::Expr so its comparable

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


		// Array type with deducer dimensions and/or terminator
		struct ArrayDeducer{
			using ID = ArrayDeducerID;

			using Dimension = evo::Variant<uint64_t, Token::ID>; // Token::ID if it is a deducer

			SourceID sourceID;
			TypeInfoID elementTypeID;
			evo::SmallVector<Dimension> dimensions;
			evo::Variant<std::monostate, core::GenericValue, Token::ID> terminator; // Token::ID if it is a dedcuer

			ArrayDeducer(
				SourceID source_id,
				TypeInfoID elem_type_id,
				evo::SmallVector<Dimension>&& _dimensions,
				evo::Variant<std::monostate, core::GenericValue, Token::ID>&& _terminator
			) : 
				sourceID(source_id),
				elementTypeID(elem_type_id),
				dimensions(std::move(_dimensions)),
				terminator(std::move(_terminator))
			{
				evo::debugAssert(
					!(this->dimensions.size() > 1 && this->terminator.is<std::monostate>() == false),
					"multi-dimensional arrays cannot be terminated"
				);
			}

			EVO_NODISCARD auto operator==(const ArrayDeducer& rhs) const -> bool { // IDK why default doesn't compile
				return this->sourceID == rhs.sourceID
					&& this->elementTypeID == rhs.elementTypeID
					&& this->dimensions == rhs.dimensions
					&& this->terminator == rhs.terminator;
			}
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
			bool isMut;


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
				bool is_mut
			) : 
				elementTypeID(elem_type_id),
				dimensions(std::move(_dimensions)),
				terminator(std::move(_terminator)),
				isMut(is_mut) 
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
			std::optional<EncapsulatingSymbolID> parent;
			TypeInfoID aliasedType;
			bool isPub; // meaningless if not pthr source type

			EVO_NODISCARD auto isPTHRSourceType() const -> bool { return this->sourceID.is<SourceID>(); }
			EVO_NODISCARD auto isClangType() const -> bool { return this->sourceID.is<ClangSourceID>(); }
			EVO_NODISCARD auto isBuiltinType() const -> bool { return this->sourceID.is<BuiltinModuleID>(); }
			EVO_NODISCARD auto getName(const class panther::SourceManager& source_manager) const -> std::string_view;

			EVO_NODISCARD auto operator==(const Alias& rhs) const -> bool {
				return this->sourceID == rhs.sourceID && this->name == rhs.name && this->parent == rhs.parent;
			}
		};


		struct DistinctAlias{
			using ID = DistinctAliasID;

			SourceID sourceID;
			Token::ID identTokenID;
			std::optional<EncapsulatingSymbolID> parent;
			TypeInfoID underlyingType;
			bool isPub;
			
			EVO_NODISCARD auto operator==(const DistinctAlias& rhs) const -> bool {
				return this->sourceID == rhs.sourceID
					&& this->identTokenID == rhs.identTokenID
					&& this->parent == rhs.parent;
			}
		};


		struct Struct{
			using ID = StructID;

			struct MemberVar{
				struct DefaultValue{
					sema::Expr value;
					bool isConstexpr;
				};

				AST::VarDef::Kind kind;
				evo::Variant<Token::ID, ClangSourceDeclInfoID, BuiltinModuleStringID> name;
				TypeInfoID typeID;
				std::optional<DefaultValue> defaultValue;
				bool isPriv;
			};

			struct DeletableOverload{
				std::optional<sema::FuncID> funcID;
				bool wasDeleted;

				DeletableOverload() : funcID(std::nullopt), wasDeleted(false) {}
				DeletableOverload(std::optional<sema::FuncID> func_id, bool was_deleted)
					: funcID(func_id), wasDeleted(was_deleted) {}
			};

			evo::Variant<SourceID, ClangSourceID, BuiltinModuleID> sourceID;
			evo::Variant<Token::ID, ClangSourceDeclInfoID, BuiltinModuleStringID> name;
			std::optional<EncapsulatingSymbolID> parent;
			std::optional<StructTemplateID> templateID = std::nullopt; // nullopt if not instantiated
			uint32_t instantiation = std::numeric_limits<uint32_t>::max(); // uint32_t max if not instantiation
			evo::SmallVector<MemberVar> memberVars; // make sure to take the lock (.memberVarsLock) when not defComplete
			evo::SmallVector<MemberVar*> memberVarsABI; // this is the order that members are for ABI
			SymbolProcNamespace* namespacedMembers; // nullptr if not pthr src type
			sema::ScopeLevel* scopeLevel; // nullptr if not pthr src type (although temporarily nullptr during creation)
			bool isPub; // meaningless if not pthr src type
			bool isOrdered; // TODO(FUTURE): is this needed here?
			bool isPacked;
			bool shouldLower = true; // may only be false if is builtin type // TODO(FUTURE): still in use?

			std::atomic<bool> defCompleted = false; // includes PIR lowering

			mutable evo::SpinLock memberVarsLock{}; // only needed before definition is completed

			evo::SmallVector<sema::FuncID> newInitOverloads{};
			mutable evo::SpinLock newInitOverloadsLock{}; // only needed before def completed

			evo::SmallVector<sema::FuncID> newAssignOverloads{};
			mutable evo::SpinLock newAssignOverloadsLock{}; // only needed before def completed

			std::atomic<std::optional<sema::FuncID>> deleteOverload{};
			std::atomic<DeletableOverload> copyInitOverload{};
			std::atomic<std::optional<sema::FuncID>> copyAssignOverload{}; // note: is nullopt if default from init
			std::atomic<DeletableOverload> moveInitOverload{};
			std::atomic<std::optional<sema::FuncID>> moveAssignOverload{}; // note: is nullopt if default from init

			std::unordered_map<TypeInfoID, sema::FuncID> operatorAsOverloads{};
			mutable evo::SpinLock operatorAsOverloadsLock{}; // only needed before def completed

			std::unordered_multimap<Token::Kind, sema::FuncID> infixOverloads{};
			mutable evo::SpinLock infixOverloadsLock{}; // only needed before def completed

			std::unordered_multimap<Token::Kind, sema::FuncID> prefixOverloads{};
			mutable evo::SpinLock prefixOverloadsLock{}; // only needed before def completed

			evo::SmallVector<sema::FuncID> indexerOverloads{};
			mutable evo::SpinLock indexerOverloadsLock{}; // only needed before def completed


			bool isDefaultInitializable = false;
			bool isTriviallyDefaultInitializable = false;
			bool isConstexprDefaultInitializable = false;
			bool isNoErrorDefaultInitializable = false;

			bool isTriviallyComparable = false;

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
					&& this->instantiation == rhs.instantiation
					&& this->parent == rhs.parent;
			}
		};

		static_assert(sizeof(Struct::DeletableOverload) <= 8, "Expected atomic of DeletableOverload to be lock free");


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


			SourceID sourceID;
			Token::ID identTokenID;
			std::optional<EncapsulatingSymbolID> parent;
			evo::SmallVector<Param> params;
			size_t minNumTemplateArgs; // TODO(PERF): make sure this optimization actually improves perf

			struct InstantiationInfo{
				Instantiation& instantiation;
				std::optional<uint32_t> instantiationID; // only has value if it needs to be compiled

				EVO_NODISCARD auto needsToBeCompiled() const -> bool { return this->instantiationID.has_value(); }
			};
			EVO_NODISCARD auto createOrLookupInstantiation(evo::SmallVector<Arg>&& args) -> InstantiationInfo;

			EVO_NODISCARD auto hasAnyDefaultParams() const -> bool {
				return this->minNumTemplateArgs != this->params.size();
			}


			EVO_NODISCARD auto getInstantiationArgs(uint32_t instantiation_id) const -> evo::SmallVector<Arg>;


			EVO_NODISCARD auto operator==(const StructTemplate& rhs) const -> bool {
				return this->sourceID == rhs.sourceID 
					&& this->identTokenID == rhs.identTokenID
					&& this->parent == rhs.parent
					&& this->params == rhs.params;
			}


			StructTemplate(
				SourceID source_id,
				Token::ID ident_token_id,
				std::optional<EncapsulatingSymbolID> _parent,
				evo::SmallVector<Param>&& _params,
				size_t min_num_template_args
			) : 
				sourceID(source_id), 
				identTokenID(ident_token_id),
				parent(_parent),
				params(std::move(_params)), 
				minNumTemplateArgs(min_num_template_args) 
			{}

			private:
				core::LinearStepAlloc<Instantiation, size_t> instantiations{};
				std::unordered_map<evo::SmallVector<Arg>, Instantiation&> instantiation_map{};
				mutable evo::SpinLock instantiation_lock{};
		};



		struct StructTemplateDeducer{ // args may be deducers or interfaces 
			using ID = StructTemplateDeducerID;

			SourceID instantiationSourceID;
			StructTemplate::ID structTemplateID;
			evo::SmallVector<StructTemplate::Arg> args;
		};



		struct Union{
			using ID = UnionID;

			struct Field{
				evo::Variant<Token::ID, ClangSourceDeclInfoID> location;
				TypeInfoVoidableID typeID;
			};
			
			evo::Variant<SourceID, ClangSourceID> sourceID;
			evo::Variant<Token::ID, ClangSourceDeclInfoID> location;
			std::optional<EncapsulatingSymbolID> parent;
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

			EVO_NODISCARD auto operator==(const Union& rhs) const -> bool {
				return this->sourceID == rhs.sourceID && this->location == rhs.location && this->parent == rhs.parent;
			}
		};


		struct Enum{
			using ID = EnumID;

			struct Enumerator{
				evo::Variant<Token::ID, ClangSourceDeclInfoID> location;
				core::GenericInt value;
			};
			
			evo::Variant<SourceID, ClangSourceID> sourceID;
			evo::Variant<Token::ID, ClangSourceDeclInfoID> location;
			std::optional<EncapsulatingSymbolID> parent;
			evo::SmallVector<Enumerator> enumerators;
			BaseType::Primitive::ID underlyingTypeID;
			SymbolProcNamespace* namespacedMembers; // nullptr if is clang type
			sema::ScopeLevel* scopeLevel; // nullopt if is clang type (although temporarily nullopt during creation)
			bool isPub; // meaningless if clang type

			std::atomic<bool> defCompleted = false;


			EVO_NODISCARD auto getName(const class panther::SourceManager& source_manager) const -> std::string_view;
			EVO_NODISCARD auto isClangType() const -> bool { return this->sourceID.is<ClangSourceID>(); }

			EVO_NODISCARD auto getEnumeratorName(
				const Enumerator& enumerator, const class panther::SourceManager& source_manager
			) const -> std::string_view;

			EVO_NODISCARD auto operator==(const Enum& rhs) const -> bool {
				return this->sourceID == rhs.sourceID && this->location == rhs.location && this->parent == rhs.parent;
			}
		};


		struct TypeDeducer{
			using ID = TypeDeducerID;

			std::optional<SourceID> sourceID; // nullopt if anonymous
			std::optional<Token::ID> identTokenID; // nullopt if anonymous

			EVO_NODISCARD auto isAnonymous() const -> bool {
				return this->sourceID.has_value() == false;
			}

			EVO_NODISCARD auto operator==(const TypeDeducer& rhs) const -> bool {
				return this->sourceID == rhs.sourceID && this->identTokenID == rhs.identTokenID;
			}
		};



		struct Interface{
			using ID = InterfaceID;

			struct Impl{
				const AST::InterfaceImpl& astInterfaceImpl;
				evo::SmallVector<sema::FuncID> methods{}; // may only access if instantiatingSymbolProc is nullopt 
				                                          // 	or waited on def
				
				// need to wait on def if not nullopt
				mutable std::atomic<std::optional<SymbolProcID>> instantiatingSymbolProc;
			};

			struct DeducerImpl{
				TypeInfoID deducerTypeID;
				AST::Node astInterfaceImpl;
				evo::SmallVector<SymbolProcID> methods{};

				SymbolProcID symbolProcID;

				DeducerImpl(TypeInfoID deducer_type_id, AST::Node ast_interface_impl, SymbolProcID symbol_proc_id) :
					deducerTypeID(deducer_type_id), astInterfaceImpl(ast_interface_impl), symbolProcID(symbol_proc_id) {
					evo::debugAssert(this->astInterfaceImpl.kind() == AST::Kind::INTERFACE_IMPL, "Incorrect node kind");
				}

				struct NeedsToBeCompiledResult{
					bool needsToBeCompiled;

					NeedsToBeCompiledResult(bool needs_to_be_compiled, std::atomic<bool>& _flag_ptr)
						: needsToBeCompiled(needs_to_be_compiled), flag_ptr(_flag_ptr) {}

					#if defined(PCIT_CONFIG_DEBUG)
						~NeedsToBeCompiledResult(){
							evo::debugAssert(this->flag_ptr.load() == true, "Needs to wait");
						}
					#endif

					auto waitForAddedToImpl() -> void {
						while(this->flag_ptr.load() == false){
							std::this_thread::yield();
						}
					}

					auto setAddedToImpl() -> void {
						this->flag_ptr = true;
					}

					private:
						std::atomic<bool>& flag_ptr;
				};

				// only returns `true` the first time called
				EVO_NODISCARD auto instantiationNeedsToBeCompiled(TypeInfoID instantiation_id) const
				-> NeedsToBeCompiledResult {
					const auto lock = std::scoped_lock(this->instantiations_lock);

					const auto instantiation_find = this->instantiations.find(instantiation_id);

					if(instantiation_find == this->instantiations.end()){ // new instantiation
						std::atomic<bool>& added_to_impl_flag = this->added_to_impl_flags.emplace_back(false);
						this->instantiations.emplace(instantiation_id, added_to_impl_flag);
						return NeedsToBeCompiledResult(true, added_to_impl_flag);

					}else{
						return NeedsToBeCompiledResult(false, instantiation_find->second);
					}
				}

				private:
					mutable std::unordered_map<TypeInfoID, std::atomic<bool>&> instantiations{};
					mutable evo::StepVector<std::atomic<bool>> added_to_impl_flags{}; 
					mutable evo::SpinLock instantiations_lock{};
			};
			
			evo::Variant<SourceID, BuiltinModuleID> sourceID;
			evo::Variant<Token::ID, BuiltinModuleStringID> name;
			std::optional<EncapsulatingSymbolID> parent;
			std::optional<SymbolProcID> symbolProcID; // nullopt if builtin
			bool isPub;
			bool isPolymorphic;

			evo::SmallVector<sema::FuncID> methods{};

			std::unordered_map<TypeInfoID, const Impl&> impls{};
			mutable evo::SpinLock implsLock{};

			evo::SmallVector<const DeducerImpl*> deducerImpls{}; // TODO(PERF): make StepVector?
			mutable evo::SpinLock deducerImplsLock{}; // only needed until `defCompleted == true`

			std::atomic<bool> defCompleted = false;

			EVO_NODISCARD auto getName(const class panther::SourceManager& source_manager) const -> std::string_view;

			EVO_NODISCARD auto operator==(const Interface& rhs) const -> bool {
				return this->sourceID == rhs.sourceID && this->name == rhs.name;
			}
		};


		struct PolyInterfaceRef{
			using ID = PolyInterfaceRefID;
			
			Interface::ID interfaceID;
			bool isMut;

			EVO_NODISCARD auto operator==(const PolyInterfaceRef&) const -> bool = default;
		};


		struct InterfaceMap{
			using ID = InterfaceMapID;
			
			TypeInfoID underlyingTypeID;
			Interface::ID interfaceID;

			EVO_NODISCARD auto operator==(const InterfaceMap&) const -> bool = default;
		};

	};


	class TypeInfo{
		public:
			using ID = TypeInfoID;
			using VoidableID = TypeInfoVoidableID;

			struct Qualifier{
				bool isPtr: 1;
				bool isMut: 1;
				bool isUninit: 1;
				bool isOptional: 1;

				Qualifier(bool is_ptr, bool is_mut, bool is_uninit, bool is_optional)
					: isPtr(is_ptr), isMut(is_mut), isUninit(is_uninit), isOptional(is_optional) {
					evo::debugAssert(is_ptr || is_optional, "must be pointer xor optional");
					evo::debugAssert(is_mut == false || is_ptr, "mut must be a pointer");
					evo::debugAssert(is_uninit == false || is_ptr, "uninit must be a pointer");
				}


				static auto createPtr()            -> Qualifier { return Qualifier(true, false, false, false); }
				static auto createMutPtr()         -> Qualifier { return Qualifier(true, true, false, false);  }
				static auto createUninitPtr()      -> Qualifier { return Qualifier(true, false, true, false);  }
				static auto createOptionalPtr()    -> Qualifier { return Qualifier(true, false, false, true);  }
				static auto createOptionalMutPtr() -> Qualifier { return Qualifier(true, true, false, true);   }
				static auto createOptional()       -> Qualifier { return Qualifier(false, false, false, true); }


				EVO_NODISCARD auto operator==(const Qualifier& rhs) const -> bool {
					return (std::bit_cast<uint8_t>(*this) & 0b1111) == (std::bit_cast<uint8_t>(rhs) & 0b1111);
				}
			};
			static_assert(sizeof(Qualifier) == 1, "sizeof(TypeInfo::Qualifier) != 1");
			
		public:
			explicit TypeInfo(const BaseType::ID& id) : base_type(id), _qualifiers() {};
			TypeInfo(const BaseType::ID& id, const evo::SmallVector<Qualifier>& qualifiers_list)
				: base_type(id), _qualifiers(qualifiers_list) {};
			TypeInfo(const BaseType::ID& id, evo::SmallVector<Qualifier>&& qualifiers_list)
				: base_type(id), _qualifiers(std::move(qualifiers_list)) {};
			~TypeInfo() = default;


			EVO_NODISCARD auto baseTypeID() const -> BaseType::ID { return this->base_type; }
			EVO_NODISCARD auto qualifiers() const -> evo::ArrayProxy<Qualifier> { return this->_qualifiers; }

			EVO_NODISCARD auto operator==(const TypeInfo&) const -> bool = default;

			EVO_NODISCARD auto isPointer() const -> bool {
				return this->qualifiers().empty() == false && this->qualifiers().back().isPtr;
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


			// TODO(PERF): better allocation of qualifiers vector
			EVO_NODISCARD auto copyWithPushedQualifier(Qualifier qualifier) const -> TypeInfo {
				TypeInfo copied_type = *this;
				copied_type._qualifiers.emplace_back(qualifier);
				return copied_type;
			}

			// TODO(PERF): better allocation of qualifiers vector
			EVO_NODISCARD auto copyWithPoppedQualifier() const -> TypeInfo {
				TypeInfo copied_type = *this;
				copied_type._qualifiers.pop_back();
				return copied_type;
			}

			// TODO(PERF): better allocation of qualifiers vector
			EVO_NODISCARD auto copyWithPoppedQualifier(size_t ammount_to_pop) const -> TypeInfo {
				evo::debugAssert(ammount_to_pop != 0, "Shouldn't be copying if popping 0");
				evo::debugAssert(this->_qualifiers.size() >= ammount_to_pop, "Too many qualifiers to pop");
				TypeInfo copied_type = *this;
				for(size_t i = 0; i < ammount_to_pop; i+=1){
					copied_type._qualifiers.pop_back();
				}
				return copied_type;
			}
	
		private:
			BaseType::ID base_type;
			evo::SmallVector<Qualifier> _qualifiers;
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
				
			EVO_NODISCARD auto printType(TypeInfo::VoidableID type_info_id, const class Context& context) const
				-> std::string;
			EVO_NODISCARD auto printType(TypeInfo::ID type_info_id, const class Context& context) const
				-> std::string;
			EVO_NODISCARD auto printType(BaseType::ID base_type_id, const class Context& context) const
				-> std::string;

			EVO_NODISCARD auto getFunction(BaseType::Function::ID id) const -> const BaseType::Function&;
			EVO_NODISCARD auto getFunction(BaseType::Function::ID id)       ->       BaseType::Function&;
			EVO_NODISCARD auto getOrCreateFunction(BaseType::Function&& lookup_func) -> BaseType::ID;

			EVO_NODISCARD auto getArray(BaseType::Array::ID id) const -> const BaseType::Array&;
			EVO_NODISCARD auto getOrCreateArray(BaseType::Array&& lookup_type) -> BaseType::ID;

			EVO_NODISCARD auto getArrayDeducer(BaseType::ArrayDeducer::ID id) const -> const BaseType::ArrayDeducer&;
			EVO_NODISCARD auto getOrCreateArrayDeducer(BaseType::ArrayDeducer&& lookup_type) -> BaseType::ID;

			EVO_NODISCARD auto getArrayRef(BaseType::ArrayRef::ID id) const -> const BaseType::ArrayRef&;
			EVO_NODISCARD auto getOrCreateArrayRef(BaseType::ArrayRef&& lookup_type) -> BaseType::ID;

			EVO_NODISCARD auto getPrimitive(BaseType::Primitive::ID id) const -> const BaseType::Primitive&;
			EVO_NODISCARD auto getOrCreatePrimitiveBaseType(Token::Kind kind) -> BaseType::ID;
			EVO_NODISCARD auto getOrCreatePrimitiveBaseType(Token::Kind kind, uint32_t bit_width) -> BaseType::ID;

			EVO_NODISCARD auto getAlias(BaseType::Alias::ID id) const -> const BaseType::Alias&;
			EVO_NODISCARD auto getAlias(BaseType::Alias::ID id)       ->       BaseType::Alias&;
			EVO_NODISCARD auto createAlias(BaseType::Alias&& new_type) -> BaseType::ID;

			EVO_NODISCARD auto getDistinctAlias(BaseType::DistinctAlias::ID id) const -> const BaseType::DistinctAlias&;
			EVO_NODISCARD auto getDistinctAlias(BaseType::DistinctAlias::ID id)       ->       BaseType::DistinctAlias&;
			EVO_NODISCARD auto createDistinctAlias(BaseType::DistinctAlias&& new_type) -> BaseType::ID;

			EVO_NODISCARD auto getStruct(BaseType::Struct::ID id) const -> const BaseType::Struct&;
			EVO_NODISCARD auto getStruct(BaseType::Struct::ID id)       ->       BaseType::Struct&;
			EVO_NODISCARD auto createStruct(BaseType::Struct&& new_type) -> BaseType::ID;
			EVO_NODISCARD auto getNumStructs() const -> size_t; // I don't love this design

			EVO_NODISCARD auto getStructTemplate(BaseType::StructTemplate::ID id) const
				-> const BaseType::StructTemplate&;
			EVO_NODISCARD auto getStructTemplate(BaseType::StructTemplate::ID id) -> BaseType::StructTemplate&;
			EVO_NODISCARD auto createStructTemplate(BaseType::StructTemplate&& new_type) -> BaseType::ID;

			EVO_NODISCARD auto getStructTemplateDeducer(BaseType::StructTemplateDeducer::ID id) const
				-> const BaseType::StructTemplateDeducer&;
			EVO_NODISCARD auto createStructTemplateDeducer(BaseType::StructTemplateDeducer&& new_type)
				-> BaseType::ID;

			EVO_NODISCARD auto getUnion(BaseType::Union::ID id) const -> const BaseType::Union&;
			EVO_NODISCARD auto getUnion(BaseType::Union::ID id)       ->       BaseType::Union&;
			EVO_NODISCARD auto createUnion(BaseType::Union&& new_type) -> BaseType::ID;
			EVO_NODISCARD auto getNumUnions() const -> size_t; // I don't love this design

			EVO_NODISCARD auto getEnum(BaseType::Enum::ID id) const -> const BaseType::Enum&;
			EVO_NODISCARD auto getEnum(BaseType::Enum::ID id)       ->       BaseType::Enum&;
			EVO_NODISCARD auto createEnum(BaseType::Enum&& new_type) -> BaseType::ID;

			EVO_NODISCARD auto getTypeDeducer(BaseType::TypeDeducer::ID id) const -> const BaseType::TypeDeducer&;
			EVO_NODISCARD auto createTypeDeducer(BaseType::TypeDeducer&& new_type) -> BaseType::ID;

			EVO_NODISCARD auto getInterface(BaseType::Interface::ID id) const -> const BaseType::Interface&;
			EVO_NODISCARD auto getInterface(BaseType::Interface::ID id)       ->       BaseType::Interface&;
			EVO_NODISCARD auto createInterface(BaseType::Interface&& new_type) -> BaseType::ID;
			EVO_NODISCARD auto getNumInterfaces() const -> size_t; // I don't love this design
			EVO_NODISCARD auto createInterfaceImpl(const AST::InterfaceImpl& ast_node) -> BaseType::Interface::Impl&;
			EVO_NODISCARD auto createInterfaceDeducerImpl(
				const TypeInfo::ID deducer_type_id, AST::Node ast_node, SymbolProcID symbol_proc_id
			) -> BaseType::Interface::DeducerImpl&;

			EVO_NODISCARD auto getPolyInterfaceRef(BaseType::PolyInterfaceRef::ID id) const
				-> const BaseType::PolyInterfaceRef&;
			EVO_NODISCARD auto getOrCreatePolyInterfaceRef(BaseType::PolyInterfaceRef&& lookup_type) -> BaseType::ID;

			EVO_NODISCARD auto getInterfaceMap(BaseType::InterfaceMap::ID id) const
				-> const BaseType::InterfaceMap&;
			EVO_NODISCARD auto getOrCreateInterfaceMap(BaseType::InterfaceMap&& lookup_type)
				-> BaseType::ID;

			
			
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


			EVO_NODISCARD auto isTypeDeducer(TypeInfo::VoidableID id) const -> bool;
			EVO_NODISCARD auto isTypeDeducer(TypeInfo::ID id) const -> bool;
			EVO_NODISCARD auto isTypeDeducer(BaseType::ID id) const -> bool;



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

			EVO_NODISCARD auto isConstexprDeletable(TypeInfo::ID id, const class SemaBuffer& sema_buffer) const -> bool;
			EVO_NODISCARD auto isConstexprDeletable(BaseType::ID id, const class SemaBuffer& sema_buffer) const -> bool;


			EVO_NODISCARD auto isCopyable(TypeInfo::ID id) const -> bool;
			EVO_NODISCARD auto isCopyable(BaseType::ID id) const -> bool;

			EVO_NODISCARD auto isTriviallyCopyable(TypeInfo::ID id) const -> bool;
			EVO_NODISCARD auto isTriviallyCopyable(BaseType::ID id) const -> bool;

			EVO_NODISCARD auto isConstexprCopyable(TypeInfo::ID id, const class SemaBuffer& sema_buffer) const -> bool;
			EVO_NODISCARD auto isConstexprCopyable(BaseType::ID id, const class SemaBuffer& sema_buffer) const -> bool;


			EVO_NODISCARD auto isMovable(TypeInfo::ID id) const -> bool;
			EVO_NODISCARD auto isMovable(BaseType::ID id) const -> bool;

			EVO_NODISCARD auto isTriviallyMovable(TypeInfo::ID id) const -> bool;
			EVO_NODISCARD auto isTriviallyMovable(BaseType::ID id) const -> bool;

			EVO_NODISCARD auto isConstexprMovable(TypeInfo::ID id, const class SemaBuffer& sema_buffer) const -> bool;
			EVO_NODISCARD auto isConstexprMovable(BaseType::ID id, const class SemaBuffer& sema_buffer) const -> bool;


			EVO_NODISCARD auto isTriviallyComparable(TypeInfo::ID id) const -> bool;
			EVO_NODISCARD auto isTriviallyComparable(BaseType::ID id) const -> bool;


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


			EVO_NODISCARD auto get_parent_name(
				std::optional<EncapsulatingSymbolID> parent,
				evo::Variant<SourceID, ClangSourceID, BuiltinModuleID> source_id,
				const class Context& context
			) const -> std::string;


			EVO_NODISCARD auto get_parent_name(
				std::optional<EncapsulatingSymbolID> parent,
				evo::Variant<SourceID, ClangSourceID> source_id,
				const class Context& context
			) const -> std::string {
				if(source_id.is<SourceID>()){
					return this->get_parent_name(
						parent,
						evo::Variant<SourceID, ClangSourceID, BuiltinModuleID>(source_id.as<SourceID>()),
						context
					);
				}else{
					return this->get_parent_name(
						parent,
						evo::Variant<SourceID, ClangSourceID, BuiltinModuleID>(source_id.as<ClangSourceID>()),
						context
					);
				}
			}

			EVO_NODISCARD auto get_parent_name(
				std::optional<EncapsulatingSymbolID> parent,
				evo::Variant<SourceID, BuiltinModuleID> source_id,
				const class Context& context
			) const -> std::string {
				if(source_id.is<SourceID>()){
					return this->get_parent_name(
						parent,
						evo::Variant<SourceID, ClangSourceID, BuiltinModuleID>(source_id.as<SourceID>()),
						context
					);
				}else{
					return this->get_parent_name(
						parent,
						evo::Variant<SourceID, ClangSourceID, BuiltinModuleID>(source_id.as<BuiltinModuleID>()),
						context
					);
				}
			}

			EVO_NODISCARD auto get_parent_name(
				std::optional<EncapsulatingSymbolID> parent, SourceID source_id, const class Context& context
			) const -> std::string {
				return this->get_parent_name(
					parent, evo::Variant<SourceID, ClangSourceID, BuiltinModuleID>(source_id), context
				);
			}

			EVO_NODISCARD auto get_parent_name(
				std::optional<EncapsulatingSymbolID> parent, ClangSourceID source_id, const class Context& context
			) const -> std::string {
				return this->get_parent_name(
					parent, evo::Variant<SourceID, ClangSourceID, BuiltinModuleID>(source_id), context
				);
			}

			EVO_NODISCARD auto get_parent_name(
				std::optional<EncapsulatingSymbolID> parent, BuiltinModuleID source_id, const class Context& context
			) const -> std::string {
				return this->get_parent_name(
					parent, evo::Variant<SourceID, ClangSourceID, BuiltinModuleID>(source_id), context
				);
			}



			EVO_NODISCARD auto get_func_name(sema::FuncID sema_func_id, const class Context& context) const
				-> std::string;


		private:
			core::Target _target;

			// TODO(PERF): improve lookup times
			core::LinearStepAlloc<BaseType::Primitive, BaseType::Primitive::ID> primitives{};
			mutable evo::SpinLock primitives_lock{};

			core::LinearStepAlloc<BaseType::Function, BaseType::Function::ID> functions{};
			mutable evo::SpinLock functions_lock{};

			core::LinearStepAlloc<BaseType::Array, BaseType::Array::ID> arrays{};
			mutable evo::SpinLock arrays_lock{};

			core::LinearStepAlloc<BaseType::ArrayDeducer, BaseType::ArrayDeducer::ID> array_deducers{};
			mutable evo::SpinLock array_deducers_lock{};

			core::LinearStepAlloc<BaseType::ArrayRef, BaseType::ArrayRef::ID> array_refs{};
			mutable evo::SpinLock array_refs_lock{};

			core::LinearStepAlloc<BaseType::Alias, BaseType::Alias::ID> aliases{};
			mutable evo::SpinLock aliases_lock{};

			core::LinearStepAlloc<BaseType::DistinctAlias, BaseType::DistinctAlias::ID> distinct_aliases{};
			mutable evo::SpinLock distinct_aliases_lock{};

			core::LinearStepAlloc<BaseType::Struct, BaseType::Struct::ID> structs{};
			mutable evo::SpinLock structs_lock{};

			core::LinearStepAlloc<BaseType::StructTemplate, BaseType::StructTemplate::ID> struct_templates{};
			mutable evo::SpinLock struct_templates_lock{};

			core::LinearStepAlloc<BaseType::StructTemplateDeducer, BaseType::StructTemplateDeducer::ID>
				struct_template_deducers{};
			mutable evo::SpinLock struct_template_deducers_lock{};

			core::LinearStepAlloc<BaseType::TypeDeducer, BaseType::TypeDeducer::ID> type_deducers{};
			mutable evo::SpinLock type_deducers_lock{};

			core::LinearStepAlloc<BaseType::Union, BaseType::Union::ID> unions{};
			mutable evo::SpinLock unions_lock{};

			core::LinearStepAlloc<BaseType::Enum, BaseType::Enum::ID> enums{};
			mutable evo::SpinLock enums_lock{};

			core::LinearStepAlloc<BaseType::Interface, BaseType::Interface::ID> interfaces{};
			mutable evo::SpinLock interfaces_lock{};
			core::SyncLinearStepAlloc<BaseType::Interface::Impl, uint64_t> interface_impls{};
			core::SyncLinearStepAlloc<BaseType::Interface::DeducerImpl, uint64_t> interface_deducer_impls{};

			core::LinearStepAlloc<BaseType::PolyInterfaceRef, BaseType::PolyInterfaceRef::ID> poly_interface_refs{};
			mutable evo::SpinLock poly_interface_refs_lock{};

			core::LinearStepAlloc<BaseType::InterfaceMap, BaseType::InterfaceMap::ID> interface_maps{};
			mutable evo::SpinLock interface_maps_lock{};

			core::LinearStepAlloc<TypeInfo, TypeInfo::ID> types{};
			mutable evo::SpinLock types_lock{};
	};

}
