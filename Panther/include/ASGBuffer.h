//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once

#include <deque>

#include <Evo.h>
#include <PCIT_core.h>

#include "./ASG.h"


namespace pcit::panther{

	class ASGBuffer{
		public:
			ASGBuffer() = default;
			~ASGBuffer() = default;

			///////////////////////////////////
			// funcs

			EVO_NODISCARD auto createFunc(auto&&... args) -> ASG::Func::ID {
				const auto created_id = ASG::Func::ID(uint32_t(this->funcs.size()));
				this->funcs.emplace_back(std::forward<decltype(args)>(args)...);
				return created_id;
			}

			EVO_NODISCARD auto getFunc(ASG::Func::ID id) const -> const ASG::Func& { return this->funcs[id.get()]; }


			EVO_NODISCARD auto getFuncs() const -> core::IterRange<ASG::Func::ID::Iterator> {
				return core::IterRange<ASG::Func::ID::Iterator>(
					ASG::Func::ID::Iterator(ASG::Func::ID(0)),
					ASG::Func::ID::Iterator(ASG::Func::ID(uint32_t(this->funcs.size())))
				);
			};

			EVO_NODISCARD auto numFuncs() const -> size_t { return this->funcs.size(); }


			///////////////////////////////////
			// templated func calls

			EVO_NODISCARD auto createTemplatedFunc(auto&&... args) -> ASG::TemplatedFunc::ID {
				const auto created_id = ASG::TemplatedFunc::ID(uint32_t(this->templated_funcs.size()));
				this->templated_funcs.emplace_back(
					std::make_unique<ASG::TemplatedFunc>(std::forward<decltype(args)>(args)...)
				);
				return created_id;
			}

			EVO_NODISCARD auto getTemplatedFunc(ASG::TemplatedFunc::ID id) const -> const ASG::TemplatedFunc& {
				return *this->templated_funcs[id.get()];
			}


			///////////////////////////////////
			// vars

			EVO_NODISCARD auto createVar(auto&&... args) -> ASG::Var::ID {
				const auto created_id = ASG::Var::ID(uint32_t(this->vars.size()));
				this->vars.emplace_back(std::forward<decltype(args)>(args)...);
				return created_id;
			}

			EVO_NODISCARD auto getVar(ASG::Var::ID id) const -> const ASG::Var& { return this->vars[id.get()]; }

			EVO_NODISCARD auto getVars() const -> core::IterRange<ASG::Var::ID::Iterator> {
				return core::IterRange<ASG::Var::ID::Iterator>(
					ASG::Var::ID::Iterator(ASG::Var::ID(0)),
					ASG::Var::ID::Iterator(ASG::Var::ID(uint32_t(this->vars.size())))
				);
			};

			EVO_NODISCARD auto numVars() const -> size_t { return this->vars.size(); }


			///////////////////////////////////
			// params

			EVO_NODISCARD auto createParam(auto&&... args) -> ASG::Param::ID {
				const auto created_id = ASG::Param::ID(uint32_t(this->params.size()));
				this->params.emplace_back(std::forward<decltype(args)>(args)...);
				return created_id;
			}

			EVO_NODISCARD auto getParam(ASG::Param::ID id) const -> const ASG::Param& {
				return this->params[id.get()];
			}


			///////////////////////////////////
			// return params

			EVO_NODISCARD auto createReturnParam(auto&&... args) -> ASG::ReturnParam::ID {
				const auto created_id = ASG::ReturnParam::ID(uint32_t(this->return_params.size()));
				this->return_params.emplace_back(std::forward<decltype(args)>(args)...);
				return created_id;
			}

			EVO_NODISCARD auto getReturnParam(ASG::ReturnParam::ID id) const -> const ASG::ReturnParam& {
				return this->return_params[id.get()];
			}



			///////////////////////////////////
			// func calls

			EVO_NODISCARD auto createFuncCall(auto&&... args) -> ASG::FuncCall::ID {
				const auto created_id = ASG::FuncCall::ID(uint32_t(this->func_calls.size()));
				this->func_calls.emplace_back(std::forward<decltype(args)>(args)...);
				return created_id;
			}

			EVO_NODISCARD auto getFuncCall(ASG::FuncCall::ID id) const -> const ASG::FuncCall& {
				return this->func_calls[id.get()];
			}


			///////////////////////////////////
			// assignments

			EVO_NODISCARD auto createAssign(auto&&... args) -> ASG::Assign::ID {
				const auto created_id = ASG::Assign::ID(uint32_t(this->assigns.size()));
				this->assigns.emplace_back(std::forward<decltype(args)>(args)...);
				return created_id;
			}

			EVO_NODISCARD auto getAssign(ASG::Assign::ID id) const -> const ASG::Assign& {
				return this->assigns[id.get()];
			}


			///////////////////////////////////
			// multi-assign

			EVO_NODISCARD auto createMultiAssign(auto&&... args) -> ASG::MultiAssign::ID {
				const auto created_id = ASG::MultiAssign::ID(uint32_t(this->multi_assigns.size()));
				this->multi_assigns.emplace_back(std::forward<decltype(args)>(args)...);
				return created_id;
			}

			EVO_NODISCARD auto getMultiAssign(ASG::MultiAssign::ID id) const -> const ASG::MultiAssign& {
				return this->multi_assigns[id.get()];
			}


			///////////////////////////////////
			// returns

			EVO_NODISCARD auto createReturn(auto&&... args) -> ASG::Return::ID {
				const auto created_id = ASG::Return::ID(uint32_t(this->returns.size()));
				this->returns.emplace_back(std::forward<decltype(args)>(args)...);
				return created_id;
			}

			EVO_NODISCARD auto getReturn(ASG::Return::ID id) const -> const ASG::Return& {
				return this->returns[id.get()];
			}


			///////////////////////////////////
			// conditionals

			EVO_NODISCARD auto createConditional(auto&&... args) -> ASG::Conditional::ID {
				const auto created_id = ASG::Conditional::ID(uint32_t(this->conds.size()));
				this->conds.emplace_back(std::forward<decltype(args)>(args)...);
				return created_id;
			}

			EVO_NODISCARD auto getConditional(ASG::Conditional::ID id) const -> const ASG::Conditional& {
				return this->conds[id.get()];
			}


			///////////////////////////////////
			// copies

			EVO_NODISCARD auto createCopy(auto&&... args) -> ASG::Copy::ID {
				const auto created_id = ASG::Copy::ID(uint32_t(this->misc_exprs.size()));
				this->misc_exprs.emplace_back(std::forward<decltype(args)>(args)...);
				return created_id;
			}

			EVO_NODISCARD auto getCopy(ASG::Copy::ID id) const -> const ASG::Expr& {
				return this->misc_exprs[id.get()];
			}


			///////////////////////////////////
			// moves

			EVO_NODISCARD auto createMove(auto&&... args) -> ASG::Move::ID {
				const auto created_id = ASG::Move::ID(uint32_t(this->misc_exprs.size()));
				this->misc_exprs.emplace_back(std::forward<decltype(args)>(args)...);
				return created_id;
			}

			EVO_NODISCARD auto getMove(ASG::Move::ID id) const -> const ASG::Expr& {
				return this->misc_exprs[id.get()];
			}


			///////////////////////////////////
			// address ofs

			EVO_NODISCARD auto createAddrOf(auto&&... args) -> ASG::AddrOf::ID {
				const auto created_id = ASG::AddrOf::ID(uint32_t(this->misc_exprs.size()));
				this->misc_exprs.emplace_back(std::forward<decltype(args)>(args)...);
				return created_id;
			}

			EVO_NODISCARD auto getAddrOf(ASG::AddrOf::ID id) const -> const ASG::Expr& {
				return this->misc_exprs[id.get()];
			}

			///////////////////////////////////
			// dereferences

			EVO_NODISCARD auto createDeref(auto&&... args) -> ASG::Deref::ID {
				const auto created_id = ASG::Deref::ID(uint32_t(this->derefs.size()));
				this->derefs.emplace_back(std::forward<decltype(args)>(args)...);
				return created_id;
			}

			EVO_NODISCARD auto getDeref(ASG::Deref::ID id) const -> const ASG::Deref& {
				return this->derefs[id.get()];
			}


			///////////////////////////////////
			// ints

			EVO_NODISCARD auto createLiteralInt(uint64_t integer, std::optional<TypeInfo::ID> type_info_id)
			-> ASG::LiteralInt::ID {
				const auto created_id = ASG::LiteralInt::ID(uint32_t(this->literal_ints.size()));
				this->literal_ints.emplace_back(type_info_id, integer);
				return created_id;
			}

			EVO_NODISCARD auto getLiteralInt(ASG::LiteralInt::ID id) const -> const ASG::LiteralInt& {
				return this->literal_ints[id.get()];
			}


			///////////////////////////////////
			// floats

			EVO_NODISCARD auto createLiteralFloat(float64_t floating_point, std::optional<TypeInfo::ID> type_info_id)
			-> ASG::LiteralFloat::ID {
				const auto created_id = ASG::LiteralFloat::ID(uint32_t(this->literal_floats.size()));
				this->literal_floats.emplace_back(type_info_id, floating_point);
				return created_id;
			}

			EVO_NODISCARD auto getLiteralFloat(ASG::LiteralFloat::ID id) const -> const ASG::LiteralFloat& {
				return this->literal_floats[id.get()];
			}


			///////////////////////////////////
			// bools

			EVO_NODISCARD auto createLiteralBool(bool boolean) -> ASG::LiteralBool::ID {
				const auto created_id = ASG::LiteralBool::ID(uint32_t(this->literal_bools.size()));
				this->literal_bools.emplace_back(boolean);
				return created_id;
			}

			EVO_NODISCARD auto getLiteralBool(ASG::LiteralBool::ID id) const -> const ASG::LiteralBool& {
				return this->literal_bools[id.get()];
			}


			///////////////////////////////////
			// chars

			EVO_NODISCARD auto createLiteralChar(char character) -> ASG::LiteralChar::ID {
				const auto created_id = ASG::LiteralChar::ID(uint32_t(this->literal_chars.size()));
				this->literal_chars.emplace_back(character);
				return created_id;
			}

			EVO_NODISCARD auto getLiteralChar(ASG::LiteralChar::ID id) const -> const ASG::LiteralChar& {
				return this->literal_chars[id.get()];
			}


			///////////////////////////////////
			// uninit

			EVO_NODISCARD auto createUninit(Token::ID uninit_token_id) -> ASG::Uninit::ID {
				const auto created_id = ASG::Uninit::ID(uint32_t(this->misc_tokens.size()));
				this->misc_tokens.emplace_back(uninit_token_id);
				return created_id;
			}

			EVO_NODISCARD auto getUninit(ASG::Uninit::ID id) const -> Token::ID {
				return this->misc_tokens[id.get()];
			}


			///////////////////////////////////
			// zeroinit

			EVO_NODISCARD auto createZeroinit(Token::ID zeroinit_token_id) -> ASG::Zeroinit::ID {
				const auto created_id = ASG::Zeroinit::ID(uint32_t(this->misc_tokens.size()));
				this->misc_tokens.emplace_back(zeroinit_token_id);
				return created_id;
			}

			EVO_NODISCARD auto getZeroinit(ASG::Zeroinit::ID id) const -> Token::ID {
				return this->misc_tokens[id.get()];
			}

	
		private:
			evo::SmallVector<ASG::Func> funcs{};
			evo::SmallVector<std::unique_ptr<ASG::TemplatedFunc>> templated_funcs{};
			evo::SmallVector<ASG::Var> vars{};
			evo::SmallVector<ASG::Param> params{};
			evo::SmallVector<ASG::ReturnParam> return_params{};

			evo::SmallVector<ASG::FuncCall> func_calls{};
			evo::SmallVector<ASG::Assign> assigns{};
			evo::SmallVector<ASG::MultiAssign> multi_assigns{};
			evo::SmallVector<ASG::Return> returns{};
			evo::SmallVector<ASG::Conditional> conds{};

			evo::SmallVector<ASG::Expr> misc_exprs{};
			evo::SmallVector<ASG::Deref> derefs{};

			evo::SmallVector<ASG::LiteralInt> literal_ints{};
			evo::SmallVector<ASG::LiteralFloat> literal_floats{};
			evo::SmallVector<ASG::LiteralBool> literal_bools{}; // switch to bool for std::vector<bool> optimization?
			evo::SmallVector<ASG::LiteralChar> literal_chars{};

			evo::SmallVector<Token::ID> misc_tokens{};

			friend class SemanticAnalyzer;
	};


}
