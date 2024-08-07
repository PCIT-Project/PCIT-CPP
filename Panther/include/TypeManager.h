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

#include "./Token.h"

namespace pcit::panther{

	struct BaseType{
		enum class Kind{
			Builtin,
		};

		// TODO: privatize members, constructor
		struct ID{
			Kind kind;
			uint32_t id;
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
	};


	class Type{
		public:
			class ID : public core::UniqueComparableID<uint32_t, class ID> {
				public:
					using core::UniqueComparableID<uint32_t, ID>::UniqueComparableID;
			};

			class VoidableID{
				public:
					VoidableID(ID type_id) : id(type_id) {};
					~VoidableID() = default;

					EVO_NODISCARD static auto Void() -> VoidableID { return VoidableID(); };

					EVO_NODISCARD auto operator==(const VoidableID& rhs) const -> bool {
						return this->id == rhs.id;
					};

					EVO_NODISCARD auto typeID() const -> const ID& {
						evo::debugAssert(this->isVoid() == false, "type is void");
						return this->id;
					};

					EVO_NODISCARD auto typeID() -> ID& {
						evo::debugAssert(this->isVoid() == false, "type is void");
						return this->id;
					};


					EVO_NODISCARD auto isVoid() const -> bool {
						return this->id.get() == std::numeric_limits<ID::Base>::max();
					};

				private:
					VoidableID() : id(std::numeric_limits<ID::Base>::max()) {};
			
				private:
					ID id;
			};


			enum class QualifierFlag{
				Ptr,
				ReadOnly,
				Optional,
				_max,
			};

			using Qualifier = evo::Flags<QualifierFlag>;
			static_assert(sizeof(Qualifier) == 1);
			
		public:
			Type(BaseType::ID id) : base_type(id), qualifiers() {};
			Type(BaseType::ID id, evo::SmallVector<Qualifier>&& _qualifiers)
				: base_type(id), qualifiers(std::move(_qualifiers)) {};
			~Type() = default;

			EVO_NODISCARD auto getBaseTypeID() const -> BaseType::ID { return this->base_type; }
			EVO_NODISCARD auto getQualifiers() const -> evo::ArrayProxy<Qualifier> { return this->qualifiers; }
	
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

			EVO_NODISCARD auto getType(Type::ID id) const -> const Type&;

			EVO_NODISCARD auto getBuiltin(BaseType::Builtin::ID id) const -> const BaseType::Builtin&;
			EVO_NODISCARD auto getOrCreateBuiltinBaseType(Token::Kind kind) -> BaseType::ID;
			EVO_NODISCARD auto getOrCreateBuiltinBaseType(Token::Kind kind, uint32_t bit_width) -> BaseType::ID;

		private:
			EVO_NODISCARD auto get_or_create_builtin_base_type_impl(const BaseType::Builtin& lookup_type)
				-> BaseType::ID;

		private:
			// TODO: better allocation methods (custom allocator instead of new/delete)?
			std::vector<BaseType::Builtin*> builtins{};
			mutable std::shared_mutex builtins_mutex{};

			std::vector<Type*> types{};
			mutable std::shared_mutex types_mutex{};
	};



}