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



		struct ID{
			EVO_NODISCARD auto kind() const -> Kind { return this->_kind; }


			template<class T>
			EVO_NODISCARD auto id() const -> T { static_assert(sizeof(T) == -1, "cannot get ID of this type"); }

			template<>
			EVO_NODISCARD auto id<Builtin::ID>() const -> Builtin::ID {
				evo::debugAssert(this->kind() == Kind::Builtin, "not a Builtin");
				return Builtin::ID(this->_id);
			}

			template<>
			EVO_NODISCARD auto id<Function::ID>() const -> Function::ID {
				evo::debugAssert(this->kind() == Kind::Function, "not a Function");
				return Function::ID(this->_id);
			}


			EVO_NODISCARD auto operator==(const ID& rhs) const -> bool {
				return this->_kind == rhs._kind && this->_id == rhs._id;
			};

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

			enum class QualifierFlag{
				Ptr,
				ReadOnly,
				Optional,
				_max,
			};

			using Qualifier = evo::Flags<QualifierFlag>;
			static_assert(sizeof(Qualifier) == 1);
			
		public:
			TypeInfo(const BaseType::ID& id) : base_type(id), _qualifiers() {};
			TypeInfo(const BaseType::ID& id, evo::SmallVector<Qualifier>&& qualifiers_list)
				: base_type(id), _qualifiers(std::move(qualifiers_list)) {};
			~TypeInfo() = default;


			EVO_NODISCARD auto baseTypeID() const -> BaseType::ID { return this->base_type; }
			EVO_NODISCARD auto qualifiers() const -> evo::ArrayProxy<Qualifier> { return this->_qualifiers; }

			EVO_NODISCARD auto operator==(const TypeInfo& rhs) const -> bool {
				return this->base_type == rhs.base_type && this->_qualifiers == rhs._qualifiers;
			};
	
		private:
			BaseType::ID base_type;
			evo::SmallVector<Qualifier> _qualifiers;
	};

	class TypeManager{
		public:
			TypeManager() = default;
			~TypeManager();

			auto initBuiltins() -> void; // single-threaded
			EVO_NODISCARD auto builtinsInitialized() const -> bool; // single-threaded

			EVO_NODISCARD auto getTypeInfo(TypeInfo::ID id) const -> const TypeInfo&;
			EVO_NODISCARD auto getOrCreateTypeInfo(TypeInfo&& lookup_type_info) -> TypeInfo::ID;
				
			EVO_NODISCARD auto printType(TypeInfo::VoidableID type_info_id) const -> std::string;
			EVO_NODISCARD auto printType(TypeInfo::ID type_info_id) const -> std::string;

			EVO_NODISCARD auto getFunction(BaseType::Function::ID id) const -> const BaseType::Function&;
			EVO_NODISCARD auto getOrCreateFunction(BaseType::Function lookup_func) -> BaseType::ID;

			EVO_NODISCARD auto getBuiltin(BaseType::Builtin::ID id) const -> const BaseType::Builtin&;
			EVO_NODISCARD auto getOrCreateBuiltinBaseType(Token::Kind kind) -> BaseType::ID;
			EVO_NODISCARD auto getOrCreateBuiltinBaseType(Token::Kind kind, uint32_t bit_width) -> BaseType::ID;

			// types of literals
			EVO_NODISCARD static auto getTypeBool()  -> TypeInfo::ID { return TypeInfo::ID(0); }
			EVO_NODISCARD static auto getTypeChar()  -> TypeInfo::ID { return TypeInfo::ID(1); }

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



namespace std{

	template<>
	class optional<pcit::panther::TypeInfo::ID> 
		: public pcit::core::Optional<pcit::panther::TypeInfo::ID, pcit::panther::TypeInfoIDOptInterface>{

		public:
			using pcit::core::Optional<pcit::panther::TypeInfo::ID, pcit::panther::TypeInfoIDOptInterface>::Optional;
			using pcit::core::Optional<pcit::panther::TypeInfo::ID, pcit::panther::TypeInfoIDOptInterface>::operator=;
	};
	
}