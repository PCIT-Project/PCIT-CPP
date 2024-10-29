//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
//                                                                  //
// This file is needed to prevent circular dependencies             //
//                                                                  //
//////////////////////////////////////////////////////////////////////



#pragma once


#include <Evo.h>
#include <PCIT_core.h>

#include "./ASG_IDs.h"
#include "./Token.h"



namespace pcit::panther::ASG{

	struct Stmt{
		enum class Kind : uint32_t {
			Var,
			FuncCall,
			Assign,
			MultiAssign,
			Return,
			Unreachable,
			Conditional,
			While,
		};

		explicit Stmt(VarID var_id)            : _kind(Kind::Var),        value{.var_id = var_id}             {}
		explicit Stmt(FuncCallID func_call_id) : _kind(Kind::FuncCall),   value{.func_call_id = func_call_id} {}
		explicit Stmt(AssignID assign_id)      : _kind(Kind::Assign),     value{.assign_id = assign_id}       {}
		explicit Stmt(MultiAssignID multi_assign_id)
			: _kind(Kind::MultiAssign), value{.multi_assign_id = multi_assign_id} {}
		explicit Stmt(ReturnID return_id)      : _kind(Kind::Return),      value{.return_id = return_id}      {}
		explicit Stmt(ConditionalID cond_id)   : _kind(Kind::Conditional), value{.cond_id = cond_id}          {}
		explicit Stmt(WhileID while_id)        : _kind(Kind::While),       value{.while_id = while_id}        {}

		static auto createUnreachable(Token::ID token_id) -> Stmt { return Stmt(token_id, Kind::Unreachable); }


		EVO_NODISCARD auto kind() const -> Kind { return this->_kind; }

		EVO_NODISCARD auto varID() const -> VarID {
			evo::debugAssert(this->kind() == Kind::Var, "not a var");
			return this->value.var_id;
		}

		EVO_NODISCARD auto funcCallID() const -> FuncCallID {
			evo::debugAssert(this->kind() == Kind::FuncCall, "not a func call");
			return this->value.func_call_id;
		}

		EVO_NODISCARD auto assignID() const -> AssignID {
			evo::debugAssert(this->kind() == Kind::Assign, "not an assign");
			return this->value.assign_id;
		}

		EVO_NODISCARD auto multiAssignID() const -> MultiAssignID {
			evo::debugAssert(this->kind() == Kind::MultiAssign, "not an assign");
			return this->value.multi_assign_id;
		}

		EVO_NODISCARD auto returnID() const -> ReturnID {
			evo::debugAssert(this->kind() == Kind::Return, "not an return");
			return this->value.return_id;
		}

		EVO_NODISCARD auto unreachableID() const -> Token::ID {
			evo::debugAssert(this->kind() == Kind::Unreachable, "not an unreachable");
			return this->value.token_id;
		}

		EVO_NODISCARD auto conditionalID() const -> ConditionalID {
			evo::debugAssert(this->kind() == Kind::Conditional, "not an conditional");
			return this->value.cond_id;
		}

		EVO_NODISCARD auto whileID() const -> WhileID {
			evo::debugAssert(this->kind() == Kind::While, "not an while");
			return this->value.while_id;
		}


		private:
			Stmt(Token::ID token_id, Kind stmt_kind) : _kind(stmt_kind), value{.token_id = token_id} {}


		private:
			Kind _kind;
			union {
				Token::ID token_id;
				VarID var_id;
				FuncCallID func_call_id;
				AssignID assign_id;
				MultiAssignID multi_assign_id;
				ReturnID return_id;
				ConditionalID cond_id;
				WhileID while_id;
			} value;
	};

	static_assert(sizeof(Stmt) == 8, "sizeof(pcit::panther::ASG::Stmt) != 8");



	class StmtBlock{
		public:
			StmtBlock() : stmts(), _is_terminated(false){}

			StmtBlock(evo::SmallVector<Stmt>&& statements, bool is_terminated = false)
				: stmts(std::move(statements)), _is_terminated(is_terminated) {}

			~StmtBlock() = default;


			EVO_NODISCARD auto isTerminated() const -> bool { return this->_is_terminated; }
			auto setTerminated() -> void {
				evo::debugAssert(this->isTerminated() == false, "already terminated");
				this->_is_terminated = true;
			}


			auto emplace_back(auto&&... args) -> Stmt& {
				return this->stmts.emplace_back(std::forward<decltype(args)>(args)...);
			}


			EVO_NODISCARD auto begin()        -> evo::SmallVector<Stmt>::iterator       { return this->stmts.begin(); };
			EVO_NODISCARD auto begin()  const -> evo::SmallVector<Stmt>::const_iterator { return this->stmts.begin(); };
			EVO_NODISCARD auto cbegin() const -> evo::SmallVector<Stmt>::const_iterator { return this->stmts.begin(); };

			EVO_NODISCARD auto end()        -> evo::SmallVector<Stmt>::iterator       { return this->stmts.end(); };
			EVO_NODISCARD auto end()  const -> evo::SmallVector<Stmt>::const_iterator { return this->stmts.end(); };
			EVO_NODISCARD auto cend() const -> evo::SmallVector<Stmt>::const_iterator { return this->stmts.end(); };

			EVO_NODISCARD auto empty() const -> bool { return this->stmts.empty(); }
			EVO_NODISCARD auto size() const -> size_t { return this->stmts.size(); }
	
		private:
			evo::SmallVector<Stmt> stmts;
			bool _is_terminated;
	};


}
