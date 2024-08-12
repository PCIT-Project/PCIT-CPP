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

namespace pcit::panther{


	//////////////////////////////////////////////////////////////////////
	// forward decls

	// is aliased as TypeInfo::ID
	class TypeInfoID : public core::UniqueID<uint32_t, class TypeInfoID> {
		public:
			using core::UniqueID<uint32_t, TypeInfoID>::UniqueID;
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

	struct BaseType{
		enum class Kind{
			Builtin,
			Function,
		};

		struct ID{
			EVO_NODISCARD auto operator==(const ID& rhs) const -> bool {
				return this->kind == rhs.kind && this->id == rhs.id;
			};

			private:
				ID(Kind _kind, uint32_t _id) : kind(_kind), id(_id) {};

			private:
				Kind kind;
				uint32_t id;

				friend class TypeManager;
		};


		struct Builtin{
			class ID : public core::UniqueID<uint32_t, class ID> {
				public:
					using core::UniqueID<uint32_t, ID>::UniqueID;
			};

			EVO_NODISCARD auto getKind() const -> Token::Kind { return this->kind; }

			EVO_NODISCARD auto getBitWidth() -> uint32_t {
				evo::debugAssert(
					this->kind == Token::Kind::TypeI_N || this->kind == Token::Kind::TypeUI_N,
					"This type does not have a bit-width"
				);

				return this->bit_width;
			}

			EVO_NODISCARD auto operator==(const Builtin& rhs) const -> bool {
				return this->kind == rhs.kind && this->bit_width == rhs.bit_width;
			}

			EVO_NODISCARD auto operator!=(const Builtin& rhs) const -> bool {
				return this->kind != rhs.kind || this->bit_width != rhs.bit_width;
			}

			private:
				Builtin(Token::Kind _kind) : kind(_kind), bit_width(0) {
					evo::debugAssert(
						this->kind != Token::Kind::TypeI_N && this->kind != Token::Kind::TypeUI_N,
						"This type requires a bit-width"
					);
				};

				Builtin(Token::Kind _kind, uint32_t _bit_width) : kind(_kind), bit_width(_bit_width) {
					evo::debugAssert(
						this->kind == Token::Kind::TypeI_N || this->kind == Token::Kind::TypeUI_N,
						"This type does not have a bit-width"
					);
				};

				Builtin(const Builtin& rhs) = default;

				Token::Kind kind;
				uint32_t bit_width;

				friend class TypeManager;
		};

		struct Function{
			class ID : public core::UniqueID<uint32_t, class ID> {
				public:
					using core::UniqueID<uint32_t, ID>::UniqueID;
			};

			struct ReturnParam{
				std::optional<Token::ID> ident;
				TypeInfoVoidableID typeID;

				EVO_NODISCARD auto operator==(const ReturnParam& rhs) const -> bool {
					return this->ident == rhs.ident && this->typeID == rhs.typeID;
				}
			};

			Function(SourceID _source_id, evo::SmallVector<ReturnParam>&& return_params_in)
				: source_id(_source_id), return_params(std::move(return_params_in)) {};

			EVO_NODISCARD auto getSourceID() const -> SourceID { return this->source_id; }
			EVO_NODISCARD auto getReturnParams() const -> evo::ArrayProxy<ReturnParam> { return this->return_params; }

			EVO_NODISCARD auto operator==(const Function& rhs) const -> bool {
				return this->source_id == rhs.source_id && this->return_params == rhs.return_params;
			}

			private:
				SourceID source_id;
				evo::SmallVector<ReturnParam> return_params;
		};
	};


	class TypeInfo{
		public:
			using ID = TypeInfoID;
			using VoidableID = TypeInfoVoidableID;

			enum class QualifierFlag{
				Ptr,
				ReadOnly,
				Optional,
				_max,
			};

			using Qualifier = evo::Flags<QualifierFlag>;
			static_assert(sizeof(Qualifier) == 1);
			
		public:
			TypeInfo(BaseType::ID id) : base_type(id), qualifiers() {};
			TypeInfo(BaseType::ID id, evo::SmallVector<Qualifier>&& _qualifiers)
				: base_type(id), qualifiers(std::move(_qualifiers)) {};
			~TypeInfo() = default;


			EVO_NODISCARD auto getBaseTypeID() const -> BaseType::ID { return this->base_type; }
			EVO_NODISCARD auto getQualifiers() const -> evo::ArrayProxy<Qualifier> { return this->qualifiers; }

			EVO_NODISCARD auto operator==(const TypeInfo& rhs) const -> bool {
				return this->base_type == rhs.base_type && this->qualifiers == rhs.qualifiers;
			};
	
		private:
			BaseType::ID base_type;
			evo::SmallVector<Qualifier> qualifiers;
	};

	class TypeManager{
		public:
			TypeManager() = default;
			~TypeManager();

			auto initBuiltins() -> void; // single-threaded
			EVO_NODISCARD auto builtinsInitialized() const -> bool; // single-threaded

			EVO_NODISCARD auto getTypeInfo(TypeInfo::ID id) const -> const TypeInfo&;
			EVO_NODISCARD auto getOrCreateTypeInfo(TypeInfo&& lookup_type_info) -> TypeInfo::ID;

			EVO_NODISCARD auto getFunction(BaseType::Function::ID id) const -> const BaseType::Function&;
			EVO_NODISCARD auto getOrCreateFunction(BaseType::Function lookup_func) -> BaseType::ID;

			EVO_NODISCARD auto getBuiltin(BaseType::Builtin::ID id) const -> const BaseType::Builtin&;
			EVO_NODISCARD auto getOrCreateBuiltinBaseType(Token::Kind kind) -> BaseType::ID;
			EVO_NODISCARD auto getOrCreateBuiltinBaseType(Token::Kind kind, uint32_t bit_width) -> BaseType::ID;

		private:
			EVO_NODISCARD auto get_or_create_builtin_base_type_impl(const BaseType::Builtin& lookup_type)
				-> BaseType::ID;

		private:
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