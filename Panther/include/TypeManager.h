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

#include "./source/source_data.h"
#include "./tokens/Token.h"
#include "./AST/AST.h"
#include "./strings.h"


namespace pcit::panther{

	class TypeManager;


	//////////////////////////////////////////////////////////////////////
	// forward decls

	// is aliased as TypeInfo::ID
	struct TypeInfoID : public core::UniqueID<uint32_t, struct TypeInfoID> {
		using core::UniqueID<uint32_t, TypeInfoID>::UniqueID;
	};

	struct TypeInfoIDOptInterface{
		static constexpr auto init(TypeInfoID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const TypeInfoID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};



	// is aliased as TypeInfo::VoidableID
	class TypeInfoVoidableID{
		public:
			TypeInfoVoidableID(TypeInfoID type_id) : id(type_id) {};
			~TypeInfoVoidableID() = default;

			EVO_NODISCARD static auto Void() -> TypeInfoVoidableID { return TypeInfoVoidableID(); };

			EVO_NODISCARD auto operator==(const TypeInfoVoidableID& rhs) const -> bool {
				return this->id == rhs.id;
			};

			EVO_NODISCARD auto asTypeID() const -> const TypeInfoID& {
				evo::debugAssert(this->isVoid() == false, "type is void");
				return this->id;
			};

			EVO_NODISCARD auto asTypeID() -> TypeInfoID& {
				evo::debugAssert(this->isVoid() == false, "type is void");
				return this->id;
			};


			EVO_NODISCARD auto isVoid() const -> bool {
				return this->id.get() == std::numeric_limits<TypeInfoID::Base>::max();
			};

		private:
			TypeInfoVoidableID() : id(std::numeric_limits<TypeInfoID::Base>::max()) {};
	
		private:
			TypeInfoID id;
	};


	//////////////////////////////////////////////////////////////////////
	// base type


	namespace BaseType{
		enum class Kind : uint8_t {
			Dummy,

			Primitive,
			Function,
			Alias,
			Typedef,
		};


		struct Primitive{
			struct ID : public core::UniqueID<uint32_t, struct ID> { using core::UniqueID<uint32_t, ID>::UniqueID; };

			EVO_NODISCARD auto kind() const -> Token::Kind { return this->_kind; }

			EVO_NODISCARD auto bitWidth() const -> uint32_t {
				evo::debugAssert(
					this->_kind == Token::Kind::TypeI_N || this->_kind == Token::Kind::TypeUI_N,
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
					this->_kind != Token::Kind::TypeI_N && this->_kind != Token::Kind::TypeUI_N,
					"This type requires a bit-width"
				);
			};

			Primitive(Token::Kind tok_kind, uint32_t _bit_width) : _kind(tok_kind), bit_width(_bit_width) {
				evo::debugAssert(
					this->_kind == Token::Kind::TypeI_N || this->_kind == Token::Kind::TypeUI_N,
					"This type does not have a bit-width"
				);
			};

			private:
				Token::Kind _kind;
				uint32_t bit_width;
		};

		struct Function{
			struct ID : public core::UniqueID<uint32_t, struct ID> { using core::UniqueID<uint32_t, ID>::UniqueID; };

			struct Param{
				evo::Variant<Token::ID, strings::StringCode> ident;
				TypeInfoID typeID;
				AST::FuncDecl::Param::Kind kind;
				bool mustLabel:1;
				bool optimizeWithCopy:1;

				EVO_NODISCARD auto operator==(const Param&) const -> bool = default;
			};

			struct ReturnParam{
				std::optional<Token::ID> ident;
				TypeInfoVoidableID typeID;

				EVO_NODISCARD auto operator==(const ReturnParam&) const -> bool = default;
			};

			evo::SmallVector<Param> params;
			evo::SmallVector<ReturnParam> returnParams;
			bool isRuntime;


			EVO_NODISCARD auto hasNamedReturns() const -> bool { return this->returnParams[0].ident.has_value(); }
			EVO_NODISCARD auto returnsVoid() const -> bool { return this->returnParams[0].typeID.isVoid(); }

			EVO_NODISCARD auto operator==(const Function&) const -> bool = default;
		};


		struct Alias{
			struct ID : public core::UniqueID<uint32_t, struct ID> { using core::UniqueID<uint32_t, ID>::UniqueID; };

			SourceID sourceID;
			Token::ID identTokenID;
			TypeInfoVoidableID aliasedType;
			
			EVO_NODISCARD auto operator==(const Alias&) const -> bool = default;
		};


		struct Typedef{
			struct ID : public core::UniqueID<uint32_t, struct ID> { using core::UniqueID<uint32_t, ID>::UniqueID; };

			SourceID sourceID;
			Token::ID identTokenID;
			TypeInfoID underlyingType;
			
			EVO_NODISCARD auto operator==(const Typedef&) const -> bool = default;
		};



		struct ID{
			EVO_NODISCARD auto kind() const -> Kind { return this->_kind; }

			EVO_NODISCARD auto primitiveID() const -> Primitive::ID {
				evo::debugAssert(this->kind() == Kind::Primitive, "not a Primitive");
				return Primitive::ID(this->_id);
			}

			EVO_NODISCARD auto funcID() const -> Function::ID {
				evo::debugAssert(this->kind() == Kind::Function, "not a Function");
				return Function::ID(this->_id);
			}

			EVO_NODISCARD auto aliasID() const -> Alias::ID {
				evo::debugAssert(this->kind() == Kind::Alias, "not an Alias");
				return Alias::ID(this->_id);
			}

			EVO_NODISCARD auto typedefID() const -> Typedef::ID {
				evo::debugAssert(this->kind() == Kind::Typedef, "not an Typedef");
				return Typedef::ID(this->_id);
			}


			EVO_NODISCARD auto operator==(const ID&) const -> bool = default;


			EVO_NODISCARD static constexpr auto dummy() -> ID {
				return ID(Kind::Dummy, std::numeric_limits<uint32_t>::max());
			}


			explicit ID(Primitive::ID id) : _kind(Kind::Primitive), _id(id.get()) {}
			explicit ID(Function::ID id)  : _kind(Kind::Function),  _id(id.get()) {}
			explicit ID(Alias::ID id)     : _kind(Kind::Alias),     _id(id.get()) {}
			explicit ID(Typedef::ID id)   : _kind(Kind::Typedef),   _id(id.get()) {}

			private:
				constexpr ID(Kind base_type_kind, uint32_t base_type_id) : _kind(base_type_kind), _id(base_type_id) {};

			private:
				Kind _kind;
				uint32_t _id;

				friend TypeManager;
				friend struct IDOptInterface;
		};


		struct IDOptInterface{
			static constexpr auto init(ID* id) -> void {
				new(id) ID(Kind::Dummy, std::numeric_limits<uint32_t>::max());
			}

			static constexpr auto has_value(const BaseType::ID& id) -> bool {
				return id._kind != Kind::Dummy;
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
			~TypeInfo() = default;


			EVO_NODISCARD auto baseTypeID() const -> BaseType::ID { return this->base_type; }
			EVO_NODISCARD auto qualifiers() const -> evo::ArrayProxy<AST::Type::Qualifier> { return this->_qualifiers; }

			EVO_NODISCARD auto operator==(const TypeInfo&) const -> bool = default;

			EVO_NODISCARD auto isPointer() const -> bool {
				return this->qualifiers().empty() == false && this->qualifiers().back().isPtr;
			}

			EVO_NODISCARD auto isOptionalNotPointer() const -> bool {
				return this->qualifiers().empty() == false      && 
				       this->qualifiers().back().isPtr == false &&
				       this->qualifiers().back().isOptional;
			}
	
		private:
			BaseType::ID base_type;
			evo::SmallVector<AST::Type::Qualifier> _qualifiers;
	};

	class TypeManager{
		public:
			TypeManager(core::OS target_os, core::Architecture target_arch) : _os(target_os), _arch(target_arch) {};
			~TypeManager() = default;


			auto initPrimitives() -> void; // single-threaded
			EVO_NODISCARD auto primitivesInitialized() const -> bool; // single-threaded


			EVO_NODISCARD auto getOS() const -> core::OS { return this->_os; }
			EVO_NODISCARD auto getArchitecture() const -> core::Architecture { return this->_arch; }


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

			EVO_NODISCARD auto getPrimitive(BaseType::Primitive::ID id) const -> const BaseType::Primitive&;
			EVO_NODISCARD auto getOrCreatePrimitiveBaseType(Token::Kind kind) -> BaseType::ID;
			EVO_NODISCARD auto getOrCreatePrimitiveBaseType(Token::Kind kind, uint32_t bit_width) -> BaseType::ID;

			EVO_NODISCARD auto getAlias(BaseType::Alias::ID id) const -> const BaseType::Alias&;
			EVO_NODISCARD auto getOrCreateAlias(BaseType::Alias&& lookup_type) -> BaseType::ID;

			EVO_NODISCARD auto getTypedef(BaseType::Typedef::ID id) const -> const BaseType::Typedef&;
			EVO_NODISCARD auto getOrCreateTypedef(BaseType::Typedef&& lookup_type) -> BaseType::ID;


			EVO_NODISCARD static auto getTypeBool()   -> TypeInfo::ID { return TypeInfo::ID(0); }
			EVO_NODISCARD static auto getTypeChar()   -> TypeInfo::ID { return TypeInfo::ID(1); }
			EVO_NODISCARD static auto getTypeUI8()    -> TypeInfo::ID { return TypeInfo::ID(2); }
			EVO_NODISCARD static auto getTypeUSize()  -> TypeInfo::ID { return TypeInfo::ID(3); }
			EVO_NODISCARD static auto getTypeTypeID() -> TypeInfo::ID { return TypeInfo::ID(4); }
			EVO_NODISCARD static auto getTypeRawPtr() -> TypeInfo::ID { return TypeInfo::ID(5); }


			///////////////////////////////////
			// type traits

			EVO_NODISCARD auto sizeOf(TypeInfo::ID id) const -> size_t;
			EVO_NODISCARD auto sizeOf(BaseType::ID id) const -> size_t;

			EVO_NODISCARD auto sizeOfPtr() const -> size_t;
			EVO_NODISCARD auto sizeOfGeneralRegister() const -> size_t;

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

			EVO_NODISCARD auto isFloatingPoint(TypeInfo::VoidableID id) const -> bool;
			EVO_NODISCARD auto isFloatingPoint(TypeInfo::ID id) const -> bool;
			EVO_NODISCARD auto isFloatingPoint(BaseType::ID id) const -> bool;

			EVO_NODISCARD auto isBuiltin(TypeInfo::VoidableID id) const -> bool;
			EVO_NODISCARD auto isBuiltin(TypeInfo::ID id) const -> bool;
			EVO_NODISCARD auto isBuiltin(BaseType::ID id) const -> bool;

			EVO_NODISCARD auto getUnderlyingType(TypeInfo::ID id) -> evo::Result<TypeInfo::ID>;
			EVO_NODISCARD auto getUnderlyingType(BaseType::ID id) -> evo::Result<TypeInfo::ID>;

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
			core::OS _os;
			core::Architecture _arch;

			// TODO: improve lookup times
			core::SyncLinearStepAlloc<BaseType::Primitive, BaseType::Primitive::ID> primitives{};
			core::SyncLinearStepAlloc<BaseType::Function, BaseType::Function::ID> functions{};
			core::SyncLinearStepAlloc<BaseType::Alias, BaseType::Alias::ID> aliases{};
			core::SyncLinearStepAlloc<BaseType::Typedef, BaseType::Typedef::ID> typedefs{};
			core::SyncLinearStepAlloc<TypeInfo, TypeInfo::ID> types{};
	};

}



namespace std{

	template<>
	class optional<pcit::panther::TypeInfo::ID> 
		: public pcit::core::Optional<pcit::panther::TypeInfo::ID, pcit::panther::TypeInfoIDOptInterface>{

		public:
			using pcit::core::Optional<pcit::panther::TypeInfo::ID, pcit::panther::TypeInfoIDOptInterface>::Optional;
			using pcit::core::Optional<pcit::panther::TypeInfo::ID, pcit::panther::TypeInfoIDOptInterface>::operator=;
	};


	template<>
	class optional<pcit::panther::BaseType::ID> 
		: public pcit::core::Optional<pcit::panther::BaseType::ID, pcit::panther::BaseType::IDOptInterface>{

		public:
			using pcit::core::Optional<pcit::panther::BaseType::ID, pcit::panther::BaseType::IDOptInterface>::Optional;
			using pcit::core::Optional<pcit::panther::BaseType::ID, pcit::panther::BaseType::IDOptInterface>::operator=;
	};
	
}
