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
#include "./ASG_IDs.h"
#include "./ScopeManager.h"


namespace pcit::panther{
	class Source;
}


namespace pcit::panther::ASG{


	//////////////////////////////////////////////////////////////////////
	// expressions

	struct LiteralInt{
		using ID = LiteralIntID;

		// TODO: change to BaseType::ID?
		std::optional<TypeInfo::ID> typeID; // nullopt if type is unknown (needs to be set before usage)
		uint64_t value;
	};

	struct LiteralFloat{
		using ID = LiteralFloatID;
	

		// TODO: change to BaseType::ID?
		std::optional<TypeInfo::ID> typeID; // nullopt if type is unknown (needs to be set before usage)
		float64_t value;
	};

	struct LiteralBool{
		using ID = LiteralBoolID;

		bool value;
	};

	struct LiteralChar{
		using ID = LiteralCharID;

		char value;
	};


	namespace Copy{
		using ID = CopyID;
	}


	struct Expr{
		enum class Kind{
			LiteralInt,
			LiteralFloat,
			LiteralBool,
			LiteralChar,

			Copy,

			Var,
			Func,
		};


		explicit Expr(LiteralInt::ID int_id) : _kind(Kind::LiteralInt), value{.literal_int = int_id} {};
		explicit Expr(LiteralFloat::ID float_id) : _kind(Kind::LiteralFloat), value{.literal_float = float_id} {};
		explicit Expr(LiteralBool::ID bool_id) : _kind(Kind::LiteralBool), value{.literal_bool = bool_id} {};
		explicit Expr(LiteralChar::ID char_id) : _kind(Kind::LiteralChar), value{.literal_char = char_id} {};

		explicit Expr(Copy::ID copy_id) : _kind(Kind::Copy), value{.copy = copy_id} {};

		explicit Expr(VarLinkID var_id) : _kind(Kind::Var), value{.var = var_id} {};
		explicit Expr(FuncLinkID func_id) : _kind(Kind::Func), value{.func = func_id} {};


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


		EVO_NODISCARD auto copyID() const -> Copy::ID {
			evo::debugAssert(this->kind() == Kind::Copy, "not a copy");
			return this->value.copy;
		}


		EVO_NODISCARD auto varLinkID() const -> VarLinkID {
			evo::debugAssert(this->kind() == Kind::Var, "not a var");
			return this->value.var;
		}

		EVO_NODISCARD auto funcLinkID() const -> FuncLinkID {
			evo::debugAssert(this->kind() == Kind::Func, "not a func");
			return this->value.func;
		}


		private:
			Kind _kind;

			union {
				LiteralInt::ID literal_int;
				LiteralFloat::ID literal_float;
				LiteralBool::ID literal_bool;
				LiteralChar::ID literal_char;

				Copy::ID copy;

				VarLinkID var;
				FuncLinkID func; // TODO: figure out how to shrink this / something else to allow Expr to be size 8
			} value;
	};



	//////////////////////////////////////////////////////////////////////
	// statements



	struct FuncCall{
		using ID = FuncCallID;

		FuncLinkID target;
	};


	struct Assign{
		using ID = AssignID;

		Expr lhs;
		Expr rhs;
	};

	struct Return{
		using ID = ReturnID;

		std::optional<Expr> value; // nullopt means `return;`
	};


	struct Stmt{
		enum class Kind{
			Var,
			FuncCall,
			Assign,
			Return,
		};

		explicit Stmt(VarID var_id) : _kind(Kind::Var), value{.var_id = var_id} {}
		explicit Stmt(FuncCall::ID func_call_id) : _kind(Kind::FuncCall), value{.func_call_id = func_call_id} {}
		explicit Stmt(Assign::ID assign_id) : _kind(Kind::Assign), value{.assign_id = assign_id} {}
		explicit Stmt(Return::ID return_id) : _kind(Kind::Return), value{.return_id = return_id} {}


		EVO_NODISCARD auto kind() const -> Kind { return this->_kind; }

		EVO_NODISCARD auto varID() const -> VarID {
			evo::debugAssert(this->kind() == Kind::Var, "not a var");
			return this->value.var_id;
		}

		EVO_NODISCARD auto funcCallID() const -> FuncCall::ID {
			evo::debugAssert(this->kind() == Kind::FuncCall, "not a func call");
			return this->value.func_call_id;
		}

		EVO_NODISCARD auto assignID() const -> Assign::ID {
			evo::debugAssert(this->kind() == Kind::Assign, "not an assign");
			return this->value.assign_id;
		}

		EVO_NODISCARD auto returnID() const -> Return::ID {
			evo::debugAssert(this->kind() == Kind::Return, "not an return");
			return this->value.return_id;
		}


		private:
			Kind _kind;
			union {
				VarID var_id;
				FuncCall::ID func_call_id;
				Assign::ID assign_id;
				Return::ID return_id;
			} value;
	};

	static_assert(sizeof(Stmt) == 8, "sizeof(pcit::panther::ASG::Stmt) != 8");





	struct Func{
		using ID = FuncID;
		using LinkID = FuncLinkID;

		struct InstanceID{
			InstanceID() : id(std::numeric_limits<uint32_t>::max()) {}
			InstanceID(uint32_t instance_id) : id(instance_id) {}

			EVO_NODISCARD auto get() const -> uint32_t {
				evo::debugAssert(this->has_value(), "cannot get instance value as it doesn't have one");
				return this->id;
			}

			EVO_NODISCARD auto has_value() const -> bool {
				return this->id != std::numeric_limits<uint32_t>::max();
			}

			private:
				uint32_t id;
		};

		AST::Node name;
		BaseType::ID baseTypeID;
		Parent parent;
		InstanceID instanceID;
		bool isPub: 1;
		bool isTerminated: 1 = false;
		evo::SmallVector<Stmt> stmts{};
	};


	struct TemplatedFunc{
		using ID = TemplatedFuncID;

		struct LinkID{
			LinkID(SourceID source_id, ID templated_func_id)
				: _source_id(source_id), _templated_func_id(templated_func_id) {}

			EVO_NODISCARD auto sourceID() const -> SourceID { return this->_source_id; }
			EVO_NODISCARD auto templatedFuncID() const -> ID { return this->_templated_func_id; }

			EVO_NODISCARD auto operator==(const LinkID& rhs) const -> bool {
				return this->_source_id == rhs._source_id && this->_templated_func_id == rhs._templated_func_id;
			}

			EVO_NODISCARD auto operator!=(const LinkID& rhs) const -> bool {
				return this->_source_id != rhs._source_id || this->_templated_func_id != rhs._templated_func_id;
			}
			
			private:
				SourceID _source_id;
				ID _templated_func_id;
		};


		struct TemplateParam{
			Token::ID ident;
			std::optional<TypeInfo::ID> typeID; // nullopt means type "Type"
		};

		const AST::FuncDecl& funcDecl;
		Parent parent;
		evo::SmallVector<TemplateParam> templateParams;
		ScopeManager::Scope scope;
		bool isPub: 1;

		TemplatedFunc(
			const AST::FuncDecl& func_decl,
			Parent _parent,
			evo::SmallVector<TemplateParam>&& template_params,
			const ScopeManager::Scope& _scope,
			bool is_pub
		) : 
			funcDecl(func_decl),
			parent(_parent),
			templateParams(std::move(template_params)),
			scope(_scope),
			isPub(is_pub)
		{}


		struct LookupInfo{
			bool needToGenerate;
			ASG::Func::InstanceID instanceID;

			LookupInfo(
				bool _need_to_generate,
				ASG::Func::InstanceID instance_id,
				std::atomic<std::optional<Func::ID>>& func_id
			) : needToGenerate(_need_to_generate), instanceID(instance_id), id(func_id) {}

			auto waitForAndGetID() const -> Func::ID {
				while(this->id.load().has_value() == false){}
				return *this->id.load(); 
			};

			auto store(Func::ID func_id) -> void {
				this->id.store(func_id);
			}

			private:
				std::atomic<std::optional<Func::ID>>& id;
		};

		using Arg = std::variant<TypeInfo::VoidableID, uint64_t, double, char, bool>;
		EVO_NODISCARD auto lookupInstance(evo::SmallVector<Arg>&& args) -> LookupInfo;

		private:
			struct Instatiation{
				std::atomic<std::optional<Func::ID>> id;
				evo::SmallVector<Arg> args;
			};
			// TODO: speedup lookup?
			// TODO: better allocation?
			evo::SmallVector<std::unique_ptr<Instatiation>> instantiations{};
			mutable std::mutex instance_lock{};
	};


	struct Var{
		using ID = VarID;
		using LinkID = VarLinkID;

		AST::VarDecl::Kind kind;
		Token::ID ident;
		TypeInfo::ID typeID;
		Expr expr;
	};

}
