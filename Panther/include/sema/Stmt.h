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
			TRY_ELSE,
			TRY_ELSE_INTERFACE,
			INTERFACE_CALL,
			ASSIGN,
			MULTI_ASSIGN,
			RETURN,
			ERROR,
			UNREACHABLE,
			BREAK,
			CONTINUE,
			DELETE,
			BLOCK_SCOPE,
			CONDITIONAL,
			WHILE,
			FOR,
			FOR_UNROLL,
			SWITCH,
			DEFER,
		};

		explicit Stmt(VarID var_id)              : _kind(Kind::VAR),         value{.var_id = var_id}               {}
		explicit Stmt(FuncCallID func_call_id)   : _kind(Kind::FUNC_CALL),   value{.func_call_id = func_call_id}   {}
		explicit Stmt(TryElseID try_else_id)     : _kind(Kind::TRY_ELSE),    value{.try_else_id = try_else_id}     {}
		explicit Stmt(TryElseInterfaceID try_else_interface_id)
			: _kind(Kind::TRY_ELSE_INTERFACE), value{.try_else_interface_id = try_else_interface_id} {}
		explicit Stmt(InterfaceCallID interface_call_id) 
			: _kind(Kind::INTERFACE_CALL), value{.interface_call_id = interface_call_id} {}
		explicit Stmt(AssignID assign_id)        : _kind(Kind::ASSIGN),      value{.assign_id = assign_id}         {}
		explicit Stmt(MultiAssignID multi_assign_id)
			: _kind(Kind::MULTI_ASSIGN), value{.multi_assign_id = multi_assign_id} {}
		explicit Stmt(ReturnID return_id)        : _kind(Kind::RETURN),      value{.return_id = return_id}         {}
		explicit Stmt(ErrorID error_id)          : _kind(Kind::ERROR),       value{.error_id = error_id}           {}
		explicit Stmt(BreakID break_id)          : _kind(Kind::BREAK),       value{.break_id = break_id}           {}
		explicit Stmt(ContinueID continue_id)    : _kind(Kind::CONTINUE),    value{.continue_id = continue_id}     {}
		explicit Stmt(DeleteID delete_id)        : _kind(Kind::DELETE),      value{.delete_id = delete_id}         {}
		explicit Stmt(BlockScopeID block_scope_id)
			: _kind(Kind::BLOCK_SCOPE), value{.block_scope_id = block_scope_id} {}
		explicit Stmt(ConditionalID cond_id)     : _kind(Kind::CONDITIONAL), value{.cond_id = cond_id}             {}
		explicit Stmt(WhileID while_id)          : _kind(Kind::WHILE),       value{.while_id = while_id}           {}
		explicit Stmt(ForID for_id)              : _kind(Kind::FOR),         value{.for_id = for_id}               {}
		explicit Stmt(ForUnrollID for_unroll_id) : _kind(Kind::FOR_UNROLL),  value{.for_unroll_id = for_unroll_id} {}
		explicit Stmt(SwitchID switch_id)        : _kind(Kind::SWITCH),      value{.switch_id = switch_id}         {}
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

		EVO_NODISCARD auto tryElseID() const -> TryElseID {
			evo::debugAssert(this->kind() == Kind::TRY_ELSE, "not a try else");
			return this->value.try_else_id;
		}

		EVO_NODISCARD auto tryElseInterfaceID() const -> TryElseInterfaceID {
			evo::debugAssert(this->kind() == Kind::TRY_ELSE_INTERFACE, "not a try else interface");
			return this->value.try_else_interface_id;
		}

		EVO_NODISCARD auto interfaceCallID() const -> InterfaceCallID {
			evo::debugAssert(this->kind() == Kind::INTERFACE_CALL, "not an interface call");
			return this->value.interface_call_id;
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
			evo::debugAssert(this->kind() == Kind::RETURN, "not a return");
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

		EVO_NODISCARD auto breakID() const -> BreakID {
			evo::debugAssert(this->kind() == Kind::BREAK, "not a break");
			return this->value.break_id;
		}

		EVO_NODISCARD auto continueID() const -> ContinueID {
			evo::debugAssert(this->kind() == Kind::CONTINUE, "not a continue");
			return this->value.continue_id;
		}

		EVO_NODISCARD auto deleteID() const -> DeleteID {
			evo::debugAssert(this->kind() == Kind::DELETE, "not a delete");
			return this->value.delete_id;
		}

		EVO_NODISCARD auto blockScopeID() const -> BlockScopeID {
			evo::debugAssert(this->kind() == Kind::BLOCK_SCOPE, "not a block scope");
			return this->value.block_scope_id;
		}

		EVO_NODISCARD auto conditionalID() const -> ConditionalID {
			evo::debugAssert(this->kind() == Kind::CONDITIONAL, "not a conditional");
			return this->value.cond_id;
		}

		EVO_NODISCARD auto whileID() const -> WhileID {
			evo::debugAssert(this->kind() == Kind::WHILE, "not a while");
			return this->value.while_id;
		}

		EVO_NODISCARD auto forID() const -> ForID {
			evo::debugAssert(this->kind() == Kind::FOR, "not a for");
			return this->value.for_id;
		}

		EVO_NODISCARD auto forUnrollID() const -> ForUnrollID {
			evo::debugAssert(this->kind() == Kind::FOR_UNROLL, "not a for unroll");
			return this->value.for_unroll_id;
		}

		EVO_NODISCARD auto switchID() const -> SwitchID {
			evo::debugAssert(this->kind() == Kind::SWITCH, "not a switch");
			return this->value.switch_id;
		}

		EVO_NODISCARD auto deferID() const -> DeferID {
			evo::debugAssert(this->kind() == Kind::DEFER, "not a defer");
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
				TryElseID try_else_id;
				TryElseInterfaceID try_else_interface_id;
				InterfaceCallID interface_call_id;
				AssignID assign_id;
				MultiAssignID multi_assign_id;
				ReturnID return_id;
				ErrorID error_id;
				BreakID break_id;
				ContinueID continue_id;
				DeleteID delete_id;
				BlockScopeID block_scope_id;
				ConditionalID cond_id;
				WhileID while_id;
				ForID for_id;
				ForUnrollID for_unroll_id;
				SwitchID switch_id;
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

			EVO_NODISCARD auto front() const -> Stmt { return this->stmts.front(); }
			EVO_NODISCARD auto back() const -> Stmt { return this->stmts.back(); }
	
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
