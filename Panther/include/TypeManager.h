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
#include "./strings.h"
#include "../src/symbol_proc/symbol_proc_ids.h"
#include "./sema/Expr.h"
#include "../src/sema/ScopeLevel.h"
#include "./type_ids.h"





namespace pcit::panther{
	

	//////////////////////////////////////////////////////////////////////
	// base type

	namespace BaseType{
		enum class Kind : uint32_t {
			DUMMY,

			PRIMITIVE,
			FUNCTION,
			ARRAY,
			ALIAS,
			TYPEDEF,
			STRUCT,
			STRUCT_TEMPLATE,
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

			EVO_NODISCARD auto aliasID() const -> AliasID {
				evo::debugAssert(this->kind() == Kind::ALIAS, "not an Alias");
				return AliasID(this->_id);
			}

			EVO_NODISCARD auto typedefID() const -> TypedefID {
				evo::debugAssert(this->kind() == Kind::TYPEDEF, "not a Typedef");
				return TypedefID(this->_id);
			}

			EVO_NODISCARD auto structID() const -> StructID {
				evo::debugAssert(this->kind() == Kind::STRUCT, "not a Struct");
				return StructID(this->_id);
			}

			EVO_NODISCARD auto structTemplateID() const -> StructTemplateID {
				evo::debugAssert(this->kind() == Kind::STRUCT_TEMPLATE, "not a StructTemplate");
				return StructTemplateID(this->_id);
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
			explicit ID(AliasID id)          : _kind(Kind::ALIAS),           _id(id.get()) {}
			explicit ID(TypedefID id)        : _kind(Kind::TYPEDEF),         _id(id.get()) {}
			explicit ID(StructID id)         : _kind(Kind::STRUCT),          _id(id.get()) {}
			explicit ID(StructTemplateID id) : _kind(Kind::STRUCT_TEMPLATE), _id(id.get()) {}
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
			evo::SmallVector<uint64_t> lengths;
			std::optional<core::GenericValue> terminator;

			EVO_NODISCARD auto isArrayPtr() const -> bool { return this->lengths.empty(); }

			Array(
				TypeInfoID elem_type_id,
				evo::SmallVector<uint64_t>&& _lengths,
				std::optional<core::GenericValue> _terminator
			) : elementTypeID(elem_type_id), lengths(_lengths), terminator(_terminator) {
				evo::debugAssert(
					!(this->lengths.size() > 1 && this->terminator.has_value()),
					"multi-dimensional arrays cannot be terminated"
				);
			}

			EVO_NODISCARD auto operator==(const Array&) const -> bool = default;
		};


		static_assert(std::atomic<std::optional<TypeInfoID>>::is_always_lock_free);

		struct Alias{
			using ID = AliasID;

			SourceID sourceID;
			Token::ID identTokenID;
			std::atomic<std::optional<TypeInfoID>> aliasedType; // nullopt if only has decl completed
			bool isPub;

			EVO_NODISCARD auto defCompleted() const -> bool { return this->aliasedType.load().has_value(); }
			
			EVO_NODISCARD auto operator==(const Alias& rhs) const -> bool {
				return this->sourceID == rhs.sourceID && this->identTokenID == rhs.identTokenID;
			}
		};


		struct Typedef{
			using ID = TypedefID;

			SourceID sourceID;
			Token::ID identTokenID;
			std::atomic<std::optional<TypeInfoID>> underlyingType; // nullopt if only has decl completed
			bool isPub;

			EVO_NODISCARD auto defCompleted() const -> bool { return this->underlyingType.load().has_value(); }
			
			EVO_NODISCARD auto operator==(const Typedef& rhs) const -> bool {
				return this->sourceID == rhs.sourceID && this->identTokenID == rhs.identTokenID;
			}
		};


		struct Struct{
			using ID = StructID;

			struct MemberVar{
				AST::VarDecl::Kind kind;
				Token::ID identTokenID;
				TypeInfoID typeID;
				std::optional<sema::Expr> defaultValue;
			};

			SourceID sourceID;
			Token::ID identTokenID;
			std::optional<StructTemplateID> templateID; // nullopt if not instantiated
			uint32_t instantiation = std::numeric_limits<uint32_t>::max(); // uin32_t max if not instantiation
			evo::SmallVector<MemberVar> memberVars; // make sure to take the lock (.memberVarsLock)
			evo::SmallVector<MemberVar*> memberVarsABI; // this is the order that members are for ABI
			SymbolProcNamespace& namespacedMembers;
			sema::ScopeLevel* scopeLevel; // is pointer because it needs to be set after construction (so never nullptr)
			bool isPub;
			bool isOrdered;
			bool isPacked;

			std::optional<pir::Type> constexprJITType{}; // if nullopt and `defCompleted == true`, means no member vars

			std::atomic<bool> defCompleted = false;

			mutable core::SpinLock memberVarsLock{}; // only needed before definition is completed


			std::unordered_map<TypeInfoID, sema::FuncID> operatorAsOverloads{};
			mutable core::SpinLock operatorAsOverloadsLock{};

			EVO_NODISCARD auto operator==(const Struct& rhs) const -> bool {
				return this->sourceID == rhs.sourceID
					&& this->identTokenID == rhs.identTokenID
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

			EVO_NODISCARD auto isOptionalNotPointer() const -> bool {
				return this->qualifiers().empty() == false
					&& this->qualifiers().back().isPtr == false
					&& this->qualifiers().back().isOptional;
			}
	
		private:
			BaseType::ID base_type;
			evo::SmallVector<AST::Type::Qualifier> _qualifiers;
	};

	class TypeManager{
		public:
			TypeManager(core::Platform target_platform) : platform(target_platform) {};
			~TypeManager() = default;


			auto initPrimitives() -> void; // single-threaded
			EVO_NODISCARD auto primitivesInitialized() const -> bool; // single-threaded


			EVO_NODISCARD auto getPlatform() const -> const core::Platform& { return this->platform; }


			EVO_NODISCARD auto getTypeInfo(TypeInfo::ID id) const -> const TypeInfo&;
			EVO_NODISCARD auto getOrCreateTypeInfo(TypeInfo&& lookup_type_info) -> TypeInfo::ID;
				
			EVO_NODISCARD auto printType(
				TypeInfo::VoidableID type_info_id, const class SourceManager& source_manager
			) const -> std::string;
			EVO_NODISCARD auto printType(
				TypeInfo::ID type_info_id, const class SourceManager& source_manager
			) const -> std::string;

			EVO_NODISCARD auto getFunction(BaseType::Function::ID id) const -> const BaseType::Function&;
			EVO_NODISCARD auto getOrCreateFunction(BaseType::Function&& lookup_func) -> BaseType::ID;

			EVO_NODISCARD auto getArray(BaseType::Array::ID id) const -> const BaseType::Array&;
			EVO_NODISCARD auto getOrCreateArray(BaseType::Array&& lookup_type) -> BaseType::ID;

			EVO_NODISCARD auto getPrimitive(BaseType::Primitive::ID id) const -> const BaseType::Primitive&;
			EVO_NODISCARD auto getOrCreatePrimitiveBaseType(Token::Kind kind) -> BaseType::ID;
			EVO_NODISCARD auto getOrCreatePrimitiveBaseType(Token::Kind kind, uint32_t bit_width) -> BaseType::ID;

			EVO_NODISCARD auto getAlias(BaseType::Alias::ID id) const -> const BaseType::Alias&;
			EVO_NODISCARD auto getAlias(BaseType::Alias::ID id)       ->       BaseType::Alias&;
			EVO_NODISCARD auto getOrCreateAlias(BaseType::Alias&& lookup_type) -> BaseType::ID;

			EVO_NODISCARD auto getTypedef(BaseType::Typedef::ID id) const -> const BaseType::Typedef&;
			EVO_NODISCARD auto getTypedef(BaseType::Typedef::ID id)       ->       BaseType::Typedef&;
			EVO_NODISCARD auto getOrCreateTypedef(BaseType::Typedef&& lookup_type) -> BaseType::ID;

			EVO_NODISCARD auto getStruct(BaseType::Struct::ID id) const -> const BaseType::Struct&;
			EVO_NODISCARD auto getStruct(BaseType::Struct::ID id)       ->       BaseType::Struct&;
			EVO_NODISCARD auto getOrCreateStruct(BaseType::Struct&& lookup_type) -> BaseType::ID;
			EVO_NODISCARD auto getNumStructs() const -> size_t; // I don't love this design

			EVO_NODISCARD auto getStructTemplate(BaseType::StructTemplate::ID id) const
				-> const BaseType::StructTemplate&;
			EVO_NODISCARD auto getStructTemplate(BaseType::StructTemplate::ID id) -> BaseType::StructTemplate&;
			EVO_NODISCARD auto getOrCreateStructTemplate(BaseType::StructTemplate&& lookup_type) -> BaseType::ID;

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


			EVO_NODISCARD auto isTypeDeducer(TypeInfo::ID id) const -> bool;


			///////////////////////////////////
			// type traits

			EVO_NODISCARD auto numBytes(TypeInfo::ID id) const -> uint64_t;
			EVO_NODISCARD auto numBytes(BaseType::ID id) const -> uint64_t;
			EVO_NODISCARD auto numBytesOfPtr() const -> uint64_t;
			EVO_NODISCARD auto numBytesOfGeneralRegister() const -> uint64_t;

			EVO_NODISCARD auto numBits(TypeInfo::ID id) const -> uint64_t;
			EVO_NODISCARD auto numBits(BaseType::ID id) const -> uint64_t;
			EVO_NODISCARD auto numBitsOfPtr() const -> uint64_t;
			EVO_NODISCARD auto numBitsOfGeneralRegister() const -> uint64_t;

			EVO_NODISCARD auto isTriviallySized(TypeInfo::ID id) const -> bool;
			EVO_NODISCARD auto isTriviallySized(BaseType::ID id) const -> bool;

			EVO_NODISCARD auto isTriviallyCopyable(TypeInfo::ID id) const -> bool;
			EVO_NODISCARD auto isTriviallyCopyable(BaseType::ID id) const -> bool;

			EVO_NODISCARD auto isTriviallyDestructable(TypeInfo::ID id) const -> bool;
			EVO_NODISCARD auto isTriviallyDestructable(BaseType::ID id) const -> bool;

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
			core::Platform platform;

			// TODO(PERF): improve lookup times
			core::LinearStepAlloc<BaseType::Primitive, BaseType::Primitive::ID> primitives{};
			mutable core::SpinLock primitives_lock{};

			core::LinearStepAlloc<BaseType::Function, BaseType::Function::ID> functions{};
			mutable core::SpinLock functions_lock{};

			core::LinearStepAlloc<BaseType::Array, BaseType::Array::ID> arrays{};
			mutable core::SpinLock arrays_lock{};

			core::LinearStepAlloc<BaseType::Alias, BaseType::Alias::ID> aliases{};
			mutable core::SpinLock aliases_lock{};

			core::LinearStepAlloc<BaseType::Typedef, BaseType::Typedef::ID> typedefs{};
			mutable core::SpinLock typedefs_lock{};

			core::LinearStepAlloc<BaseType::Struct, BaseType::Struct::ID> structs{};
			mutable core::SpinLock structs_lock{};

			core::LinearStepAlloc<BaseType::StructTemplate, BaseType::StructTemplate::ID> struct_templates{};
			mutable core::SpinLock struct_templates_lock{};

			core::LinearStepAlloc<BaseType::TypeDeducer, BaseType::TypeDeducer::ID> type_deducers{};
			mutable core::SpinLock type_deducers_lock{};

			core::LinearStepAlloc<BaseType::Interface, BaseType::Interface::ID> interfaces{};
			mutable core::SpinLock interfaces_lock{};
			core::SyncLinearStepAlloc<BaseType::Interface::Impl, uint64_t> interface_impls{};

			core::LinearStepAlloc<TypeInfo, TypeInfo::ID> types{};
			mutable core::SpinLock types_lock{};
	};

}
