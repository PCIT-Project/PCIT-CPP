//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>
#include <PCIT_core.h>

#include "./AST.h"
#include "./TypeManager.h"


namespace pcit::panther::ASG{

	// All IDs defined here are used as indexes into ASGBuffer (found in Source)


	//////////////////////////////////////////////////////////////////////
	// forward declarations

	class FuncID : public core::UniqueID<uint32_t, class FuncID> {
		public:
			using core::UniqueID<uint32_t, FuncID>::UniqueID;
	};

	using Parent = evo::Variant<std::monostate, FuncID>;


	class VarID : public core::UniqueID<uint32_t, class VarID> {
		public:
			using core::UniqueID<uint32_t, VarID>::UniqueID;
	};


	//////////////////////////////////////////////////////////////////////
	// expressions

	struct LiteralInt{
		class ID : public core::UniqueID<uint32_t, class ID> {
			public:
				using core::UniqueID<uint32_t, ID>::UniqueID;
		};


		uint64_t value;
		std::optional<TypeInfo::ID> typeID; // TODO: change to BaseType::ID?
	};

	struct LiteralFloat{
		class ID : public core::UniqueID<uint32_t, class ID> {
			public:
				using core::UniqueID<uint32_t, ID>::UniqueID;
		};

		
		float64_t value;
		std::optional<TypeInfo::ID> typeID; // TODO: change to BaseType::ID?
	};

	struct LiteralBool{
		class ID : public core::UniqueID<uint32_t, class ID> {
			public:
				using core::UniqueID<uint32_t, ID>::UniqueID;
		};

		bool value;
	};

	struct LiteralChar{
		class ID : public core::UniqueID<uint32_t, class ID> {
			public:
				using core::UniqueID<uint32_t, ID>::UniqueID;
		};

		char value;
	};




	struct Expr{
		enum class Kind{
			LiteralInt,
			LiteralFloat,
			LiteralBool,
			LiteralChar,

			Var,
		};


		explicit Expr(LiteralInt::ID int_id) : _kind(Kind::LiteralInt), value{.literal_int = int_id} {};
		explicit Expr(LiteralFloat::ID float_id) : _kind(Kind::LiteralFloat), value{.literal_float = float_id} {};
		explicit Expr(LiteralBool::ID bool_id) : _kind(Kind::LiteralBool), value{.literal_bool = bool_id} {};
		explicit Expr(LiteralChar::ID char_id) : _kind(Kind::LiteralChar), value{.literal_char = char_id} {};
		explicit Expr(VarID var_id) : _kind(Kind::Var), value{.var = var_id} {};


		EVO_NODISCARD auto kind() const -> Kind { return this->_kind; }


		EVO_NODISCARD auto literalIntID() const -> LiteralInt::ID {
			evo::debugAssert(this->kind() == Kind::LiteralInt, "not a LiteralInt");
			return this->value.literal_int;
		}
		EVO_NODISCARD auto literalFloatID() const -> LiteralFloat::ID {
			evo::debugAssert(this->kind() == Kind::LiteralFloat, "not a LiteralFloat");
			return this->value.literal_float;
		}
		EVO_NODISCARD auto literalBoolID() const -> LiteralBool::ID {
			evo::debugAssert(this->kind() == Kind::LiteralBool, "not a LiteralBool");
			return this->value.literal_bool;
		}
		EVO_NODISCARD auto literalCharID() const -> LiteralChar::ID {
			evo::debugAssert(this->kind() == Kind::LiteralChar, "not a LiteralChar");
			return this->value.literal_char;
		}

		EVO_NODISCARD auto varID() const -> VarID {
			evo::debugAssert(this->kind() == Kind::Var, "not a var");
			return this->value.var;
		}

		private:
			Kind _kind;

			union {
				LiteralInt::ID literal_int;
				LiteralFloat::ID literal_float;
				LiteralBool::ID literal_bool;
				LiteralChar::ID literal_char;

				VarID var;
			} value;

	};

	static_assert(sizeof(Expr) == 8, "sizeof(pcit::panther::ASG::Expr) is different than expected");


	//////////////////////////////////////////////////////////////////////
	// statements


	struct Stmt{
		enum class Kind{
			Var,
		};


		explicit Stmt(VarID var_id) : _kind(Kind::Var), value{.var_id = var_id} {};


		EVO_NODISCARD auto kind() const -> Kind { return this->_kind; }

		EVO_NODISCARD auto varID() const -> VarID {
			evo::debugAssert(this->kind() == Kind::Var, "not a var");
			return this->value.var_id;
		}


		private:
			Kind _kind;
			union {
				VarID var_id;
			} value;
	};

	static_assert(sizeof(Stmt) == 8, "sizeof(pcit::panther::ASG::Stmt) is different than expected");





	struct Func{
		using ID = FuncID;

		struct LinkID{
			LinkID(SourceID source_id, ID func_id) : _source_id(source_id), _func_id(func_id) {}

			EVO_NODISCARD auto sourceID() const -> SourceID { return this->_source_id; }
			EVO_NODISCARD auto funcID() const -> ID { return this->_func_id; }

			EVO_NODISCARD auto operator==(const LinkID& rhs) const -> bool {
				return this->_source_id == rhs._source_id && this->_func_id == rhs._func_id;
			}

			EVO_NODISCARD auto operator!=(const LinkID& rhs) const -> bool {
				return this->_source_id != rhs._source_id || this->_func_id != rhs._func_id;
			}
			
			private:
				SourceID _source_id;
				ID _func_id;
		};



		AST::Node name;
		BaseType::ID baseTypeID;
		Parent parent;
		evo::SmallVector<Stmt> stmts{};
	};


	struct Var{
		using ID = VarID;

		struct LinkID{
			LinkID(SourceID source_id, ID var_id) : _source_id(source_id), _var_id(var_id) {}

			EVO_NODISCARD auto sourceID() const -> SourceID { return this->_source_id; }
			EVO_NODISCARD auto varID() const -> ID { return this->_var_id; }

			EVO_NODISCARD auto operator==(const LinkID& rhs) const -> bool {
				return this->_source_id == rhs._source_id && this->_var_id == rhs._var_id;
			}

			EVO_NODISCARD auto operator!=(const LinkID& rhs) const -> bool {
				return this->_source_id != rhs._source_id || this->_var_id != rhs._var_id;
			}
			
			private:
				SourceID _source_id;
				ID _var_id;
		};



		Token::ID ident;
		TypeInfo::ID typeID;
		Expr expr;
	};

}


template<>
struct std::hash<pcit::panther::ASG::Func::LinkID>{
	auto operator()(const pcit::panther::ASG::Func::LinkID& link_id) const noexcept -> size_t {
		auto hasher = std::hash<uint32_t>{};
		return evo::hashCombine(hasher(link_id.sourceID().get()), hasher(link_id.funcID().get()));
	};
};

