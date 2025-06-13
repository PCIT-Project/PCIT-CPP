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
			VAR,
			FUNC_CALL,
			ASSIGN,
			MULTI_ASSIGN,
			RETURN,
			ERROR,
			UNREACHABLE,
			CONDITIONAL,
			WHILE,
			DEFER,
		};

		explicit Stmt(VarID var_id)              : _kind(Kind::VAR),          value{.var_id = var_id}               {}
		explicit Stmt(FuncCallID func_call_id)   : _kind(Kind::FUNC_CALL),    value{.func_call_id = func_call_id}   {}
		explicit Stmt(AssignID assign_id)        : _kind(Kind::ASSIGN),       value{.assign_id = assign_id}         {}
		explicit Stmt(MultiAssignID multi_assign_id)
			: _kind(Kind::MULTI_ASSIGN), value{.multi_assign_id = multi_assign_id} {}
		explicit Stmt(ReturnID return_id)        : _kind(Kind::RETURN),      value{.return_id = return_id}         {}
		explicit Stmt(ErrorID error_id)          : _kind(Kind::ERROR),       value{.error_id = error_id}           {}
		explicit Stmt(ConditionalID cond_id)     : _kind(Kind::CONDITIONAL), value{.cond_id = cond_id}             {}
		explicit Stmt(WhileID while_id)          : _kind(Kind::WHILE),       value{.while_id = while_id}           {}
		explicit Stmt(DeferID defer_id)          : _kind(Kind::DEFER),       value{.defer_id = defer_id}           {}

		static auto createUnreachable(Token::ID token_id) -> Stmt { return Stmt(token_id, Kind::UNREACHABLE); }


		EVO_NODISCARD auto kind() const -> Kind { return this->_kind; }

		EVO_NODISCARD auto varID() const -> VarID {
			evo::debugAssert(this->kind() == Kind::VAR, "not a var");
			return this->value.var_id;
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

		EVO_NODISCARD auto deferID() const -> DeferID {
			evo::debugAssert(this->kind() == Kind::DEFER, "not an defer");
			return this->value.defer_id;
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
				ErrorID error_id;
				ConditionalID cond_id;
				WhileID while_id;
				DeferID defer_id;
			} value;
	};

	static_assert(sizeof(Stmt) == 8, "sizeof(pcit::panther::sema::Stmt) != 8");



	class StmtBlock{
		enum class TerminatedKind{
			NONE, // only used for optional interface

			TERMINATED,
			LABEL_TERMINATED, // for example, through `return->label 12;`
			NOT_TERMINATED,
		};

		public:
			StmtBlock() : stmts(), terminated_kind(TerminatedKind::NOT_TERMINATED){}

			// TODO(FUTURE): is this constructor needed?
			// StmtBlock(evo::SmallVector<Stmt>&& statements, bool is_terminated = false)
			// 	: stmts(std::move(statements)), terminated_kind(is_terminated) {}

			~StmtBlock() = default;

			
			EVO_NODISCARD auto isTerminated() const -> bool {
				return this->terminated_kind != TerminatedKind::NOT_TERMINATED;
			}

			EVO_NODISCARD auto isLabelTerminated() const -> bool {
				return this->terminated_kind == TerminatedKind::LABEL_TERMINATED;
			}
			

			auto setTerminated() -> void {
				evo::debugAssert(this->isTerminated() == false, "already terminated");
				this->terminated_kind = TerminatedKind::TERMINATED;
			}

			auto setLabelTerminated() -> void {
				evo::debugAssert(this->isTerminated() == false, "already terminated");
				this->terminated_kind = TerminatedKind::LABEL_TERMINATED;
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
			TerminatedKind terminated_kind;

			friend struct core::OptionalInterface<StmtBlock>;
	};



}


namespace pcit::core{

	template<>
	struct OptionalInterface<panther::sema::StmtBlock>{
		static constexpr auto init(panther::sema::StmtBlock* block) -> void {
			block->terminated_kind = panther::sema::StmtBlock::TerminatedKind::NONE;
		}

		static constexpr auto has_value(const panther::sema::StmtBlock& block) -> bool {
			return block.terminated_kind != panther::sema::StmtBlock::TerminatedKind::NONE;
		}
	};

}



namespace std{


	template<>
	class optional<pcit::panther::sema::StmtBlock> : public pcit::core::Optional<pcit::panther::sema::StmtBlock>{
		public:
			using pcit::core::Optional<pcit::panther::sema::StmtBlock>::Optional;
			using pcit::core::Optional<pcit::panther::sema::StmtBlock>::operator=;
	};

	
}
