////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>
#include <PCIT_core.h>

#include "../../src/sema/ScopeManager.h"
#include "./sema.h"


namespace pcit::panther{

	class SemaBuffer{
		public:
			SemaBuffer() = default;
			~SemaBuffer() = default;

			///////////////////////////////////
			// funcs

			EVO_NODISCARD auto createFunc(auto&&... args) -> sema::Func::ID {
				return this->funcs.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getFunc(sema::Func::ID id) const -> const sema::Func& { return this->funcs[id]; }


			EVO_NODISCARD auto getFuncs() const -> core::IterRange<sema::Func::ID::Iterator> {
				return core::IterRange<sema::Func::ID::Iterator>(
					sema::Func::ID::Iterator(sema::Func::ID(0)),
					sema::Func::ID::Iterator(sema::Func::ID(uint32_t(this->funcs.size())))
				);
			};

			EVO_NODISCARD auto numFuncs() const -> size_t { return this->funcs.size(); }


			///////////////////////////////////
			// templated funcs

			EVO_NODISCARD auto createTemplatedFunc(auto&&... args) -> sema::TemplatedFunc::ID {
				return this->templated_funcs.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getTemplatedFunc(sema::TemplatedFunc::ID id) const -> const sema::TemplatedFunc& {
				return this->templated_funcs[id];
			}


			///////////////////////////////////
			// templated structs

			EVO_NODISCARD auto createTemplatedStruct(auto&&... args) -> sema::TemplatedStruct::ID {
				return this->templated_structs.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getTemplatedStruct(sema::TemplatedStruct::ID id) const -> const sema::TemplatedStruct& {
				return this->templated_structs[id];
			}


			///////////////////////////////////
			// vars

			EVO_NODISCARD auto createVar(auto&&... args) -> sema::Var::ID {
				return this->vars.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getVar(sema::Var::ID id) const -> const sema::Var& {
				return this->vars[id];
			}

			EVO_NODISCARD auto getVars() const -> core::IterRange<sema::Var::ID::Iterator> {
				return core::IterRange<sema::Var::ID::Iterator>(
					sema::Var::ID::Iterator(sema::Var::ID(0)),
					sema::Var::ID::Iterator(sema::Var::ID(uint32_t(this->vars.size())))
				);
			};

			EVO_NODISCARD auto numVars() const -> size_t { return this->vars.size(); }


			///////////////////////////////////
			// global vars

			EVO_NODISCARD auto createGlobalVar(auto&&... args) -> sema::GlobalVar::ID {
				return this->global_vars.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getGlobalVar(sema::GlobalVar::ID id) const -> const sema::GlobalVar& {
				return this->global_vars[id];
			}

			EVO_NODISCARD auto getGlobalVars() const -> core::IterRange<sema::GlobalVar::ID::Iterator> {
				return core::IterRange<sema::GlobalVar::ID::Iterator>(
					sema::GlobalVar::ID::Iterator(sema::GlobalVar::ID(0)),
					sema::GlobalVar::ID::Iterator(sema::GlobalVar::ID(uint32_t(this->global_vars.size())))
				);
			};

			EVO_NODISCARD auto numGlobalVars() const -> size_t { return this->global_vars.size(); }


			///////////////////////////////////
			// params

			EVO_NODISCARD auto createParam(auto&&... args) -> sema::Param::ID {
				return this->params.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getParam(sema::Param::ID id) const -> const sema::Param& {
				return this->params[id];
			}


			///////////////////////////////////
			// return params

			EVO_NODISCARD auto createReturnParam(auto&&... args) -> sema::ReturnParam::ID {
				return this->return_params.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getReturnParam(sema::ReturnParam::ID id) const -> const sema::ReturnParam& {
				return this->return_params[id];
			}


			///////////////////////////////////
			// error return params

			EVO_NODISCARD auto createErrorReturnParam(auto&&... args) -> sema::ErrorReturnParam::ID {
				return this->error_return_params.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getErrorReturnParam(sema::ErrorReturnParam::ID id) const
			-> const sema::ErrorReturnParam& {
				return this->error_return_params[id];
			}


			///////////////////////////////////
			// block expr outputs

			EVO_NODISCARD auto createBlockExprOutput(auto&&... args) -> sema::BlockExprOutput::ID {
				return this->block_expr_outputs.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getBlockExprOutput(sema::BlockExprOutput::ID id) const -> const sema::BlockExprOutput& {
				return this->block_expr_outputs[id];
			}


			///////////////////////////////////
			// except param

			EVO_NODISCARD auto createExceptParam(auto&&... args) -> sema::ExceptParam::ID {
				return this->except_params.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getExceptParam(sema::ExceptParam::ID id) const -> const sema::ExceptParam& {
				return this->except_params[id];
			}


			///////////////////////////////////
			// func calls

			EVO_NODISCARD auto createFuncCall(auto&&... args) -> sema::FuncCall::ID {
				return this->func_calls.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getFuncCall(sema::FuncCall::ID id) const -> const sema::FuncCall& {
				return this->func_calls[id];
			}


			///////////////////////////////////
			// assignments

			EVO_NODISCARD auto createAssign(auto&&... args) -> sema::Assign::ID {
				return this->assigns.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getAssign(sema::Assign::ID id) const -> const sema::Assign& {
				return this->assigns[id];
			}


			///////////////////////////////////
			// multi-assign

			EVO_NODISCARD auto createMultiAssign(auto&&... args) -> sema::MultiAssign::ID {
				return this->multi_assigns.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getMultiAssign(sema::MultiAssign::ID id) const -> const sema::MultiAssign& {
				return this->multi_assigns[id];
			}


			///////////////////////////////////
			// returns

			EVO_NODISCARD auto createReturn(auto&&... args) -> sema::Return::ID {
				return this->returns.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getReturn(sema::Return::ID id) const -> const sema::Return& {
				return this->returns[id];
			}


			///////////////////////////////////
			// errors

			EVO_NODISCARD auto createError(auto&&... args) -> sema::Error::ID {
				return this->errors.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getError(sema::Error::ID id) const -> const sema::Error& {
				return this->errors[id];
			}


			///////////////////////////////////
			// breaks

			EVO_NODISCARD auto createBreak(auto&&... args) -> sema::Break::ID {
				return this->breaks.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getBreak(sema::Break::ID id) const -> const sema::Break& {
				return this->breaks[id];
			}


			///////////////////////////////////
			// continues

			EVO_NODISCARD auto createContinue(auto&&... args) -> sema::Continue::ID {
				return this->continues.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getContinue(sema::Continue::ID id) const -> const sema::Continue& {
				return this->continues[id];
			}


			///////////////////////////////////
			// conditionals

			EVO_NODISCARD auto createConditional(auto&&... args) -> sema::Conditional::ID {
				return this->conds.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getConditional(sema::Conditional::ID id) const -> const sema::Conditional& {
				return this->conds[id];
			}


			///////////////////////////////////
			// whiles

			EVO_NODISCARD auto createWhile(auto&&... args) -> sema::While::ID {
				return this->whiles.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getWhile(sema::While::ID id) const -> const sema::While& {
				return this->whiles[id];
			}


			///////////////////////////////////
			// defers

			EVO_NODISCARD auto createDefer(auto&&... args) -> sema::Defer::ID {
				return this->defers.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getDefer(sema::Defer::ID id) const -> const sema::Defer& {
				return this->defers[id];
			}


			///////////////////////////////////
			// copies

			EVO_NODISCARD auto createCopy(auto&&... args) -> sema::Copy::ID {
				return sema::Copy::ID(this->misc_exprs.emplace_back(std::forward<decltype(args)>(args)...));
			}

			EVO_NODISCARD auto getCopy(sema::Copy::ID id) const -> const sema::Expr& {
				return this->misc_exprs[id.get()];
			}


			///////////////////////////////////
			// moves

			EVO_NODISCARD auto createMove(auto&&... args) -> sema::Move::ID {
				return sema::Move::ID(this->misc_exprs.emplace_back(std::forward<decltype(args)>(args)...));
			}

			EVO_NODISCARD auto getMove(sema::Move::ID id) const -> const sema::Expr& {
				return this->misc_exprs[id.get()];
			}


			///////////////////////////////////
			// forwards

			EVO_NODISCARD auto createForward(auto&&... args) -> sema::Forward::ID {
				return sema::Forward::ID(this->misc_exprs.emplace_back(std::forward<decltype(args)>(args)...));
			}

			EVO_NODISCARD auto getForward(sema::Forward::ID id) const -> const sema::Expr& {
				return this->misc_exprs[id.get()];
			}


			///////////////////////////////////
			// address ofs

			EVO_NODISCARD auto createAddrOf(auto&&... args) -> sema::AddrOf::ID {
				return sema::AddrOf::ID(this->misc_exprs.emplace_back(std::forward<decltype(args)>(args)...));
			}

			EVO_NODISCARD auto getAddrOf(sema::AddrOf::ID id) const -> const sema::Expr& {
				return this->misc_exprs[id.get()];
			}


			///////////////////////////////////
			// dereferences

			EVO_NODISCARD auto createDeref(auto&&... args) -> sema::Deref::ID {
				return this->derefs.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getDeref(sema::Deref::ID id) const -> const sema::Deref& {
				return this->derefs[id];
			}


			///////////////////////////////////
			// accessors

			EVO_NODISCARD auto createAccessor(auto&&... args) -> sema::Accessor::ID {
				return this->accessors.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getAccessor(sema::Accessor::ID id) const -> const sema::Accessor& {
				return this->accessors[id];
			}


			///////////////////////////////////
			// ptr accessors

			EVO_NODISCARD auto createPtrAccessor(auto&&... args) -> sema::PtrAccessor::ID {
				return this->ptr_accessors.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getPtrAccessor(sema::PtrAccessor::ID id) const -> const sema::PtrAccessor& {
				return this->ptr_accessors[id];
			}


			///////////////////////////////////
			// try/else

			EVO_NODISCARD auto createTryElse(auto&&... args) -> sema::TryElse::ID {
				return this->try_elses.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getTryElse(sema::TryElse::ID id) const -> const sema::TryElse& {
				return this->try_elses[id];
			}


			///////////////////////////////////
			// block expr

			EVO_NODISCARD auto createBlockExpr(auto&&... args) -> sema::BlockExpr::ID {
				return this->block_exprs.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getBlockExpr(sema::BlockExpr::ID id) const -> const sema::BlockExpr& {
				return this->block_exprs[id];
			}


			///////////////////////////////////
			// fake term info

			EVO_NODISCARD auto createFakeTermInfo(auto&&... args) -> sema::FakeTermInfo::ID {
				return this->fake_term_infos.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getFakeTermInfo(sema::FakeTermInfo::ID id) const -> const sema::FakeTermInfo& {
				return this->fake_term_infos[id];
			}


			///////////////////////////////////
			// make interface ptr

			EVO_NODISCARD auto createMakeInterfacePtr(auto&&... args) -> sema::MakeInterfacePtr::ID {
				return this->make_interface_ptrs.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getMakeInterfacePtr(sema::MakeInterfacePtr::ID id) const
			-> const sema::MakeInterfacePtr& {
				return this->make_interface_ptrs[id];
			}


			///////////////////////////////////
			// interface call

			EVO_NODISCARD auto createInterfaceCall(auto&&... args) -> sema::InterfaceCall::ID {
				return this->interface_calls.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getInterfaceCall(sema::InterfaceCall::ID id) const -> const sema::InterfaceCall& {
				return this->interface_calls[id];
			}


			///////////////////////////////////
			// indexer

			EVO_NODISCARD auto createIndexer(auto&&... args) -> sema::Indexer::ID {
				return this->indexers.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getIndexer(sema::Indexer::ID id) const -> const sema::Indexer& {
				return this->indexers[id];
			}


			///////////////////////////////////
			// pointer indexer

			EVO_NODISCARD auto createPtrIndexer(auto&&... args) -> sema::PtrIndexer::ID {
				return this->ptr_indexers.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto getPtrIndexer(sema::PtrIndexer::ID id) const -> const sema::PtrIndexer& {
				return this->ptr_indexers[id];
			}


			///////////////////////////////////
			// template intrinsic instantiations

			EVO_NODISCARD auto createTemplateIntrinsicFuncInstantiation(auto&&... args)
			-> sema::TemplateIntrinsicFuncInstantiation::ID {
				return this->templated_intrinsic_func_instantiations.emplace_back(
					std::forward<decltype(args)>(args)...
				);
			}

			EVO_NODISCARD auto getTemplateIntrinsicFuncInstantiation(
				sema::TemplateIntrinsicFuncInstantiation::ID id
			) const -> const sema::TemplateIntrinsicFuncInstantiation& {
				return this->templated_intrinsic_func_instantiations[id];
			}


			///////////////////////////////////
			// ints

			EVO_NODISCARD auto createIntValue(core::GenericInt integer, std::optional<BaseType::ID> type_info_id)
			-> sema::IntValue::ID {
				return this->int_values.emplace_back(integer, type_info_id);
			}

			EVO_NODISCARD auto getIntValue(sema::IntValue::ID id) const -> const sema::IntValue& {
				return this->int_values[id];
			}


			///////////////////////////////////
			// floats

			EVO_NODISCARD auto createFloatValue(
				core::GenericFloat floating_point, std::optional<BaseType::ID> type_info_id
			) -> sema::FloatValue::ID {
				return this->float_values.emplace_back(floating_point, type_info_id);
			}

			EVO_NODISCARD auto getFloatValue(sema::FloatValue::ID id) const -> const sema::FloatValue& {
				return this->float_values[id];
			}


			///////////////////////////////////
			// bools

			EVO_NODISCARD auto createBoolValue(bool boolean) -> sema::BoolValue::ID {
				return this->bool_values.emplace_back(boolean);
			}

			EVO_NODISCARD auto getBoolValue(sema::BoolValue::ID id) const -> const sema::BoolValue& {
				return this->bool_values[id];
			}

			///////////////////////////////////
			// strings

			EVO_NODISCARD auto createStringValue(std::string&& value) -> sema::StringValue::ID {
				return this->string_values.emplace_back(std::move(value));
			}

			EVO_NODISCARD auto createStringValue(const std::string& value) -> sema::StringValue::ID {
				return this->string_values.emplace_back(value);
			}

			EVO_NODISCARD auto getStringValue(sema::StringValue::ID id) const -> const sema::StringValue& {
				return this->string_values[id];
			}


			///////////////////////////////////
			// aggregates

			EVO_NODISCARD auto createAggregateValue(evo::SmallVector<sema::Expr>&& values, BaseType::ID typeID)
			-> sema::AggregateValue::ID {
				return this->aggregate_values.emplace_back(std::move(values), typeID);
			}

			EVO_NODISCARD auto createAggregateValue(
				const evo::SmallVector<sema::Expr>& values, BaseType::ID typeID
			) -> sema::AggregateValue::ID {
				return this->aggregate_values.emplace_back(values, typeID);
			}

			EVO_NODISCARD auto getAggregateValue(sema::AggregateValue::ID id) const -> const sema::AggregateValue& {
				return this->aggregate_values[id];
			}


			///////////////////////////////////
			// chars

			EVO_NODISCARD auto createCharValue(char character) -> sema::CharValue::ID {
				return this->char_values.emplace_back(character);
			}

			EVO_NODISCARD auto getCharValue(sema::CharValue::ID id) const -> const sema::CharValue& {
				return this->char_values[id];
			}


			///////////////////////////////////
			// uninit

			EVO_NODISCARD auto createUninit(Token::ID uninit_token_id) -> sema::Uninit::ID {
				return sema::Uninit::ID(this->misc_tokens.emplace_back(uninit_token_id));
			}

			EVO_NODISCARD auto getUninit(sema::Uninit::ID id) const -> Token::ID {
				return this->misc_tokens[id.get()];
			}


			///////////////////////////////////
			// zeroinit

			EVO_NODISCARD auto createZeroinit(Token::ID zeroinit_token_id) -> sema::Zeroinit::ID {
				return sema::Zeroinit::ID(this->misc_tokens.emplace_back(zeroinit_token_id));
			}

			EVO_NODISCARD auto getZeroinit(sema::Zeroinit::ID id) const -> Token::ID {
				return this->misc_tokens[id.get()];
			}

	
		private:
			core::SyncLinearStepAlloc<sema::Func, sema::Func::ID> funcs{};
			core::SyncLinearStepAlloc<sema::TemplatedFunc, sema::TemplatedFunc::ID> templated_funcs{};
			core::SyncLinearStepAlloc<sema::TemplatedStruct, sema::TemplatedStruct::ID> templated_structs{};
			core::SyncLinearStepAlloc<sema::Var, sema::Var::ID> vars{};
			core::SyncLinearStepAlloc<sema::GlobalVar, sema::GlobalVar::ID> global_vars{};
			core::SyncLinearStepAlloc<sema::Param, sema::Param::ID> params{};
			core::SyncLinearStepAlloc<sema::ReturnParam, sema::ReturnParam::ID> return_params{};
			core::SyncLinearStepAlloc<sema::ErrorReturnParam, sema::ErrorReturnParam::ID> error_return_params{};
			core::SyncLinearStepAlloc<sema::BlockExprOutput, sema::BlockExprOutput::ID> block_expr_outputs{};
			core::SyncLinearStepAlloc<sema::ExceptParam, sema::ExceptParam::ID> except_params{};

			core::SyncLinearStepAlloc<sema::FuncCall, sema::FuncCall::ID> func_calls{};
			core::SyncLinearStepAlloc<sema::Assign, sema::Assign::ID> assigns{};
			core::SyncLinearStepAlloc<sema::MultiAssign, sema::MultiAssign::ID> multi_assigns{};
			core::SyncLinearStepAlloc<sema::Return, sema::Return::ID> returns{};
			core::SyncLinearStepAlloc<sema::Error, sema::Error::ID> errors{};
			core::SyncLinearStepAlloc<sema::Break, sema::Break::ID> breaks{};
			core::SyncLinearStepAlloc<sema::Continue, sema::Continue::ID> continues{};
			core::SyncLinearStepAlloc<sema::Conditional, sema::Conditional::ID> conds{};
			core::SyncLinearStepAlloc<sema::While, sema::While::ID> whiles{};
			core::SyncLinearStepAlloc<sema::Defer, sema::Defer::ID> defers{};

			core::SyncLinearStepAlloc<sema::Expr, uint32_t> misc_exprs{};
			core::SyncLinearStepAlloc<sema::Deref, sema::Deref::ID> derefs{};
			core::SyncLinearStepAlloc<sema::Accessor, sema::Accessor::ID> accessors{};
			core::SyncLinearStepAlloc<sema::PtrAccessor, sema::PtrAccessor::ID> ptr_accessors{};
			core::SyncLinearStepAlloc<sema::TryElse, sema::TryElse::ID> try_elses{};
			core::SyncLinearStepAlloc<sema::BlockExpr, sema::BlockExpr::ID> block_exprs{};
			core::SyncLinearStepAlloc<sema::FakeTermInfo, sema::FakeTermInfo::ID> fake_term_infos{};
			core::SyncLinearStepAlloc<sema::MakeInterfacePtr, sema::MakeInterfacePtr::ID> make_interface_ptrs{};
			core::SyncLinearStepAlloc<sema::InterfaceCall, sema::InterfaceCall::ID> interface_calls{};
			core::SyncLinearStepAlloc<sema::Indexer, sema::Indexer::ID> indexers{};
			core::SyncLinearStepAlloc<sema::PtrIndexer, sema::PtrIndexer::ID> ptr_indexers{};

			core::SyncLinearStepAlloc<
				sema::TemplateIntrinsicFuncInstantiation, sema::TemplateIntrinsicFuncInstantiation::ID
			> templated_intrinsic_func_instantiations{};

			core::SyncLinearStepAlloc<sema::IntValue, sema::IntValue::ID> int_values{};
			core::SyncLinearStepAlloc<sema::FloatValue, sema::FloatValue::ID> float_values{};
			core::SyncLinearStepAlloc<sema::BoolValue, sema::BoolValue::ID> bool_values{};
			core::SyncLinearStepAlloc<sema::StringValue, sema::StringValue::ID> string_values{};
			core::SyncLinearStepAlloc<sema::AggregateValue, sema::AggregateValue::ID> aggregate_values{};
			core::SyncLinearStepAlloc<sema::CharValue, sema::CharValue::ID> char_values{};

			core::SyncLinearStepAlloc<Token::ID, uint32_t> misc_tokens{};


			sema::ScopeManager scope_manager{};


			friend class Source;
			friend class Context;
			friend class SemanticAnalyzer;
	};


}
