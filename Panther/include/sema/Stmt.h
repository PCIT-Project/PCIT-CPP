////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
//                                                                  //
// This file is needed to prevent circular dependencies             //
//                                                                  //
//////////////////////////////////////////////////////////////////////



#pragma once


#include <Evo.h>
#include <PCIT_core.h>

#include "./sema_ids.h"
#include "../tokens/Token.h"



namespace pcit::panther::sema{

	struct Stmt{
		enum class Kind : uint32_t {
			GLOBAL_VAR,
			FUNC_CALL,
			ASSIGN,
			MULTI_ASSIGN,
			RETURN,
			ERROR,
			UNREACHABLE,
			CONDITIONAL,
			WHILE,
		};

		explicit Stmt(GlobalVarID global_var_id) : _kind(Kind::GLOBAL_VAR),   value{.global_var_id = global_var_id} {}
		explicit Stmt(FuncCallID func_call_id)   : _kind(Kind::FUNC_CALL),    value{.func_call_id = func_call_id}   {}
		explicit Stmt(AssignID assign_id)        : _kind(Kind::ASSIGN),       value{.assign_id = assign_id}         {}
		explicit Stmt(MultiAssignID multi_assign_id)
			: _kind(Kind::MULTI_ASSIGN), value{.multi_assign_id = multi_assign_id} {}
		explicit Stmt(ReturnID return_id)        : _kind(Kind::RETURN),      value{.return_id = return_id}         {}
		explicit Stmt(ErrorID error_id)          : _kind(Kind::ERROR),       value{.error_id = error_id}           {}
		explicit Stmt(ConditionalID cond_id)     : _kind(Kind::CONDITIONAL), value{.cond_id = cond_id}             {}
		explicit Stmt(WhileID while_id)          : _kind(Kind::WHILE),       value{.while_id = while_id}           {}

		static auto createUnreachable(Token::ID token_id) -> Stmt { return Stmt(token_id, Kind::UNREACHABLE); }


		EVO_NODISCARD auto kind() const -> Kind { return this->_kind; }

		EVO_NODISCARD auto globalVarID() const -> GlobalVarID {
			evo::debugAssert(this->kind() == Kind::GLOBAL_VAR, "not a global_var");
			return this->value.global_var_id;
		}

		EVO_NODISCARD auto funcCallID() const -> FuncCallID {
			evo::debugAssert(this->kind() == Kind::FUNC_CALL, "not a func call");
			return this->value.func_call_id;
		}

		EVO_NODISCARD auto assignID() const -> AssignID {
			evo::debugAssert(this->kind() == Kind::ASSIGN, "not an assign");
			return this->value.assign_id;
		}

		EVO_NODISCARD auto multiAssignID() const -> MultiAssignID {
			evo::debugAssert(this->kind() == Kind::MULTI_ASSIGN, "not an assign");
			return this->value.multi_assign_id;
		}

		EVO_NODISCARD auto returnID() const -> ReturnID {
			evo::debugAssert(this->kind() == Kind::RETURN, "not an return");
			return this->value.return_id;
		}

		EVO_NODISCARD auto errorID() const -> ErrorID {
			evo::debugAssert(this->kind() == Kind::ERROR, "not an error");
			return this->value.error_id;
		}

		EVO_NODISCARD auto unreachableID() const -> Token::ID {
			evo::debugAssert(this->kind() == Kind::UNREACHABLE, "not an unreachable");
			return this->value.token_id;
		}

		EVO_NODISCARD auto conditionalID() const -> ConditionalID {
			evo::debugAssert(this->kind() == Kind::CONDITIONAL, "not an conditional");
			return this->value.cond_id;
		}

		EVO_NODISCARD auto whileID() const -> WhileID {
			evo::debugAssert(this->kind() == Kind::WHILE, "not an while");
			return this->value.while_id;
		}


		private:
			Stmt(Token::ID token_id, Kind stmt_kind) : _kind(stmt_kind), value{.token_id = token_id} {}


		private:
			Kind _kind;
			union {
				Token::ID token_id;
				GlobalVarID global_var_id;
				FuncCallID func_call_id;
				AssignID assign_id;
				MultiAssignID multi_assign_id;
				ReturnID return_id;
				ErrorID error_id;
				ConditionalID cond_id;
				WhileID while_id;
			} value;
	};

	static_assert(sizeof(Stmt) == 8, "sizeof(pcit::panther::sema::Stmt) != 8");



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
