//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once

#include <shared_mutex>

#include <Evo.h>
#include <PCIT_core.h>

#include "./source_data.h"
#include "./Token.h"
#include "./AST.h"

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

			EVO_NODISCARD auto typeID() const -> const TypeInfoID& {
				evo::debugAssert(this->isVoid() == false, "type is void");
				return this->id;
			};

			EVO_NODISCARD auto typeID() -> TypeInfoID& {
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
		enum class Kind{
			Builtin,
			Function,
		};


		struct Builtin{
			struct ID : public core::UniqueID<uint32_t, struct ID> { using core::UniqueID<uint32_t, ID>::UniqueID; };

			EVO_NODISCARD auto kind() const -> Token::Kind { return this->_kind; }

			EVO_NODISCARD auto bitWidth() const -> uint32_t {
				evo::debugAssert(
					this->_kind == Token::Kind::TypeI_N || this->_kind == Token::Kind::TypeUI_N,
					"This type does not have a bit-width"
				);

				return this->bit_width;
			}

			EVO_NODISCARD auto operator==(const Builtin& rhs) const -> bool {
				return this->_kind == rhs._kind && this->bit_width == rhs.bit_width;
			}

			EVO_NODISCARD auto operator!=(const Builtin& rhs) const -> bool {
				return this->_kind != rhs._kind || this->bit_width != rhs.bit_width;
			}

			private:
				Builtin(Token::Kind tok_kind) : _kind(tok_kind), bit_width(0) {
					evo::debugAssert(
						this->_kind != Token::Kind::TypeI_N && this->_kind != Token::Kind::TypeUI_N,
						"This type requires a bit-width"
					);
				};

				Builtin(Token::Kind tok_kind, uint32_t _bit_width) : _kind(tok_kind), bit_width(_bit_width) {
					evo::debugAssert(
						this->_kind == Token::Kind::TypeI_N || this->_kind == Token::Kind::TypeUI_N,
						"This type does not have a bit-width"
					);
				};

				Builtin(const Builtin& rhs) = default;

				Token::Kind _kind;
				uint32_t bit_width;

				friend TypeManager;
		};

		struct Function{
			struct ID : public core::UniqueID<uint32_t, struct ID> { using core::UniqueID<uint32_t, ID>::UniqueID; };

			struct Param{
				Token::ID ident;
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

			Function(
				SourceID _source_id, evo::SmallVector<Param>&& params_in, evo::SmallVector<ReturnParam>&& _return_params
			) : source_id(_source_id), _params(std::move(params_in)), return_params(std::move(_return_params)) {};

			EVO_NODISCARD auto getSourceID() const -> SourceID { return this->source_id; }
			EVO_NODISCARD auto params() const -> evo::ArrayProxy<Param> { return this->_params; }
			EVO_NODISCARD auto returnParams() const -> evo::ArrayProxy<ReturnParam> { return this->return_params; }


			EVO_NODISCARD auto hasNamedReturns() const -> bool { return this->return_params[0].ident.has_value(); }
			EVO_NODISCARD auto returnsVoid() const -> bool { return this->return_params[0].typeID.isVoid(); }

			EVO_NODISCARD auto operator==(const Function&) const -> bool = default;

			private:
				SourceID source_id;
				evo::SmallVector<Param> _params;
				evo::SmallVector<ReturnParam> return_params;
		};



		struct ID{
			EVO_NODISCARD auto kind() const -> Kind { return this->_kind; }

			EVO_NODISCARD auto builtinID() const -> Builtin::ID {
				evo::debugAssert(this->kind() == Kind::Builtin, "not a Builtin");
				return Builtin::ID(this->_id);
			}

			EVO_NODISCARD auto funcID() const -> Function::ID {
				evo::debugAssert(this->kind() == Kind::Function, "not a Function");
				return Function::ID(this->_id);
			}

			EVO_NODISCARD auto operator==(const ID&) const -> bool = default;

			private:
				ID(Kind base_type_kind, uint32_t base_type_id) : _kind(base_type_kind), _id(base_type_id) {};

			private:
				Kind _kind;
				uint32_t _id;

				friend TypeManager;
		};
	};


	class TypeInfo{
		public:
			using ID = TypeInfoID;
			using VoidableID = TypeInfoVoidableID;
			
		public:
			TypeInfo(const BaseType::ID& id) : base_type(id), _qualifiers() {};
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
			TypeManager(core::Platform target_platform, core::Architecture target_arch)
				: _platform(target_platform), _architecture(target_arch) {};
			~TypeManager();


			auto initBuiltins() -> void; // single-threaded
			EVO_NODISCARD auto builtinsInitialized() const -> bool; // single-threaded


			EVO_NODISCARD auto platform() const -> core::Platform { return this->_platform; }
			EVO_NODISCARD auto architecture() const -> core::Architecture { return this->_architecture; }


			EVO_NODISCARD auto getTypeInfo(TypeInfo::ID id) const -> const TypeInfo&;
			EVO_NODISCARD auto getOrCreateTypeInfo(TypeInfo&& lookup_type_info) -> TypeInfo::ID;
				
			EVO_NODISCARD auto printType(TypeInfo::VoidableID type_info_id) const -> std::string;
			EVO_NODISCARD auto printType(TypeInfo::ID type_info_id) const -> std::string;

			EVO_NODISCARD auto getFunction(BaseType::Function::ID id) const -> const BaseType::Function&;
			EVO_NODISCARD auto getOrCreateFunction(BaseType::Function lookup_func) -> BaseType::ID;

			EVO_NODISCARD auto getBuiltin(BaseType::Builtin::ID id) const -> const BaseType::Builtin&;
			EVO_NODISCARD auto getOrCreateBuiltinBaseType(Token::Kind kind) -> BaseType::ID;
			EVO_NODISCARD auto getOrCreateBuiltinBaseType(Token::Kind kind, uint32_t bit_width) -> BaseType::ID;

			// types needed by semantic analyzer to check againt
			EVO_NODISCARD static auto getTypeBool() -> TypeInfo::ID { return TypeInfo::ID(0); }
			EVO_NODISCARD static auto getTypeChar() -> TypeInfo::ID { return TypeInfo::ID(1); }
			EVO_NODISCARD static auto getTypeUI8()  -> TypeInfo::ID { return TypeInfo::ID(2); }

			// type traits
			EVO_NODISCARD auto sizeOf(TypeInfo::ID id) const -> size_t;
			EVO_NODISCARD auto sizeOf(BaseType::ID id) const -> size_t;

			EVO_NODISCARD auto sizeOfPtr() const -> size_t;
			EVO_NODISCARD auto sizeOfGeneralRegister() const -> size_t;

			EVO_NODISCARD auto isTriviallyCopyable(TypeInfo::ID id) const -> bool;
			EVO_NODISCARD auto isTriviallyCopyable(BaseType::ID id) const -> bool;

		private:
			EVO_NODISCARD auto get_or_create_builtin_base_type_impl(const BaseType::Builtin& lookup_type)
				-> BaseType::ID;


		private:
			core::Platform _platform;
			core::Architecture _architecture;


			// TODO: improve lookup times
			// TODO: better allocation methods (custom allocator instead of new/delete)?

			std::vector<BaseType::Builtin*> builtins{};
			mutable std::shared_mutex builtins_mutex{};

			std::vector<BaseType::Function*> functions{};
			mutable std::shared_mutex functions_mutex{};

			std::vector<TypeInfo*> types{};
			mutable std::shared_mutex types_mutex{};
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
	
}
