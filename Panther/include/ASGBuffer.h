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
				return this->funcs.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getFunc(ASG::Func::ID id) const -> const ASG::Func& { return this->funcs[id]; }


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
				return this->templated_funcs.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getTemplatedFunc(ASG::TemplatedFunc::ID id) const -> const ASG::TemplatedFunc& {
				return this->templated_funcs[id];
			}


			///////////////////////////////////
			// vars

			EVO_NODISCARD auto createVar(auto&&... args) -> ASG::Var::ID {
				return this->vars.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getVar(ASG::Var::ID id) const -> const ASG::Var& { return this->vars[id]; }

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
				return this->params.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getParam(ASG::Param::ID id) const -> const ASG::Param& {
				return this->params[id];
			}


			///////////////////////////////////
			// return params

			EVO_NODISCARD auto createReturnParam(auto&&... args) -> ASG::ReturnParam::ID {
				return this->return_params.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getReturnParam(ASG::ReturnParam::ID id) const -> const ASG::ReturnParam& {
				return this->return_params[id];
			}



			///////////////////////////////////
			// func calls

			EVO_NODISCARD auto createFuncCall(auto&&... args) -> ASG::FuncCall::ID {
				return this->func_calls.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getFuncCall(ASG::FuncCall::ID id) const -> const ASG::FuncCall& {
				return this->func_calls[id];
			}


			///////////////////////////////////
			// assignments

			EVO_NODISCARD auto createAssign(auto&&... args) -> ASG::Assign::ID {
				return this->assigns.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getAssign(ASG::Assign::ID id) const -> const ASG::Assign& {
				return this->assigns[id];
			}


			///////////////////////////////////
			// multi-assign

			EVO_NODISCARD auto createMultiAssign(auto&&... args) -> ASG::MultiAssign::ID {
				return this->multi_assigns.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getMultiAssign(ASG::MultiAssign::ID id) const -> const ASG::MultiAssign& {
				return this->multi_assigns[id];
			}


			///////////////////////////////////
			// returns

			EVO_NODISCARD auto createReturn(auto&&... args) -> ASG::Return::ID {
				return this->returns.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getReturn(ASG::Return::ID id) const -> const ASG::Return& {
				return this->returns[id];
			}


			///////////////////////////////////
			// conditionals

			EVO_NODISCARD auto createConditional(auto&&... args) -> ASG::Conditional::ID {
				return this->conds.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getConditional(ASG::Conditional::ID id) const -> const ASG::Conditional& {
				return this->conds[id];
			}


			///////////////////////////////////
			// copies

			EVO_NODISCARD auto createCopy(auto&&... args) -> ASG::Copy::ID {
				return ASG::Copy::ID(this->misc_exprs.emplace_back(std::forward<decltype(args)>(args)...));
			}

			EVO_NODISCARD auto getCopy(ASG::Copy::ID id) const -> const ASG::Expr& {
				return this->misc_exprs[id.get()];
			}


			///////////////////////////////////
			// moves

			EVO_NODISCARD auto createMove(auto&&... args) -> ASG::Move::ID {
				return ASG::Move::ID(this->misc_exprs.emplace_back(std::forward<decltype(args)>(args)...));
			}

			EVO_NODISCARD auto getMove(ASG::Move::ID id) const -> const ASG::Expr& {
				return this->misc_exprs[id.get()];
			}


			///////////////////////////////////
			// address ofs

			EVO_NODISCARD auto createAddrOf(auto&&... args) -> ASG::AddrOf::ID {
				return ASG::AddrOf::ID(this->misc_exprs.emplace_back(std::forward<decltype(args)>(args)...));
			}

			EVO_NODISCARD auto getAddrOf(ASG::AddrOf::ID id) const -> const ASG::Expr& {
				return this->misc_exprs[id.get()];
			}

			///////////////////////////////////
			// dereferences

			EVO_NODISCARD auto createDeref(auto&&... args) -> ASG::Deref::ID {
				return this->derefs.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getDeref(ASG::Deref::ID id) const -> const ASG::Deref& {
				return this->derefs[id];
			}


			///////////////////////////////////
			// template intrinsic instantiations

			EVO_NODISCARD auto createTemplatedIntrinsicInstantiation(auto&&... args)
			-> ASG::TemplatedIntrinsicInstantiation::ID {
				return this->templated_intrinsic_instantiations.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getTemplatedIntrinsicInstantiation(ASG::TemplatedIntrinsicInstantiation::ID id) const
			-> const ASG::TemplatedIntrinsicInstantiation& {
				return this->templated_intrinsic_instantiations[id];
			}


			///////////////////////////////////
			// ints

			EVO_NODISCARD auto createLiteralInt(uint64_t integer, std::optional<TypeInfo::ID> type_info_id)
			-> ASG::LiteralInt::ID {
				return this->literal_ints.emplace_back(type_info_id, integer);
			}

			EVO_NODISCARD auto getLiteralInt(ASG::LiteralInt::ID id) const -> const ASG::LiteralInt& {
				return this->literal_ints[id];
			}


			///////////////////////////////////
			// floats

			EVO_NODISCARD auto createLiteralFloat(float64_t floating_point, std::optional<TypeInfo::ID> type_info_id)
			-> ASG::LiteralFloat::ID {
				return this->literal_floats.emplace_back(type_info_id, floating_point);
			}

			EVO_NODISCARD auto getLiteralFloat(ASG::LiteralFloat::ID id) const -> const ASG::LiteralFloat& {
				return this->literal_floats[id];
			}


			///////////////////////////////////
			// bools

			EVO_NODISCARD auto createLiteralBool(bool boolean) -> ASG::LiteralBool::ID {
				return this->literal_bools.emplace_back(boolean);
			}

			EVO_NODISCARD auto getLiteralBool(ASG::LiteralBool::ID id) const -> const ASG::LiteralBool& {
				return this->literal_bools[id];
			}


			///////////////////////////////////
			// chars

			EVO_NODISCARD auto createLiteralChar(char character) -> ASG::LiteralChar::ID {
				return this->literal_chars.emplace_back(character);
			}

			EVO_NODISCARD auto getLiteralChar(ASG::LiteralChar::ID id) const -> const ASG::LiteralChar& {
				return this->literal_chars[id];
			}


			///////////////////////////////////
			// uninit

			EVO_NODISCARD auto createUninit(Token::ID uninit_token_id) -> ASG::Uninit::ID {
				return ASG::Uninit::ID(this->misc_tokens.emplace_back(uninit_token_id));
			}

			EVO_NODISCARD auto getUninit(ASG::Uninit::ID id) const -> Token::ID {
				return this->misc_tokens[id.get()];
			}


			///////////////////////////////////
			// zeroinit

			EVO_NODISCARD auto createZeroinit(Token::ID zeroinit_token_id) -> ASG::Zeroinit::ID {
				return ASG::Zeroinit::ID(this->misc_tokens.emplace_back(zeroinit_token_id));
			}

			EVO_NODISCARD auto getZeroinit(ASG::Zeroinit::ID id) const -> Token::ID {
				return this->misc_tokens[id.get()];
			}

	
		private:
			core::LinearStepAlloc<ASG::Func, ASG::Func::ID> funcs{};
			core::LinearStepAlloc<ASG::TemplatedFunc, ASG::TemplatedFunc::ID> templated_funcs{};
			core::LinearStepAlloc<ASG::Var, ASG::Var::ID> vars{};
			core::LinearStepAlloc<ASG::Param, ASG::Param::ID> params{};
			core::LinearStepAlloc<ASG::ReturnParam, ASG::ReturnParam::ID> return_params{};

			core::LinearStepAlloc<ASG::FuncCall, ASG::FuncCall::ID> func_calls{};
			core::LinearStepAlloc<ASG::Assign, ASG::Assign::ID> assigns{};
			core::LinearStepAlloc<ASG::MultiAssign, ASG::MultiAssign::ID> multi_assigns{};
			core::LinearStepAlloc<ASG::Return, ASG::Return::ID> returns{};
			core::LinearStepAlloc<ASG::Conditional, ASG::Conditional::ID> conds{};

			core::LinearStepAlloc<ASG::Expr, uint32_t> misc_exprs{};
			core::LinearStepAlloc<ASG::Deref, ASG::Deref::ID> derefs{};

			core::LinearStepAlloc<
				ASG::TemplatedIntrinsicInstantiation, ASG::TemplatedIntrinsicInstantiation::ID
			> templated_intrinsic_instantiations{};

			core::LinearStepAlloc<ASG::LiteralInt, ASG::LiteralInt::ID> literal_ints{};
			core::LinearStepAlloc<ASG::LiteralFloat, ASG::LiteralFloat::ID> literal_floats{};
			core::LinearStepAlloc<ASG::LiteralBool, ASG::LiteralBool::ID> literal_bools{};
			core::LinearStepAlloc<ASG::LiteralChar, ASG::LiteralChar::ID> literal_chars{};

			core::LinearStepAlloc<Token::ID, uint32_t> misc_tokens{};

			friend class SemanticAnalyzer;
	};


}
