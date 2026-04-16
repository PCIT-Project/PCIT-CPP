////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.hpp>
#include <PCIT_core.hpp>

#include "../../src/sema/ScopeManager.hpp"
#include "./sema.hpp"


namespace pcit::panther{

	class SemaBuffer{
		public:
			SemaBuffer() = default;
			~SemaBuffer() = default;

			///////////////////////////////////
			// funcs

			[[nodiscard]] auto createFunc(auto&&... args) -> sema::Func::ID {
				return this->funcs.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getFunc(sema::Func::ID id) const -> const sema::Func& { return this->funcs[id]; }


			[[nodiscard]] auto getFuncs() const -> evo::IterRange<sema::Func::ID::Iterator> {
				return evo::IterRange<sema::Func::ID::Iterator>(
					sema::Func::ID::Iterator(sema::Func::ID(0)),
					sema::Func::ID::Iterator(sema::Func::ID(uint32_t(this->funcs.size())))
				);
			};

			[[nodiscard]] auto numFuncs() const -> size_t { return this->funcs.size(); }


			///////////////////////////////////
			// func alias

			[[nodiscard]] auto createFuncAlias(auto&&... args) -> sema::FuncAlias::ID {
				return this->func_aliases.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getFuncAlias(sema::FuncAlias::ID id) const -> const sema::FuncAlias& {
				return this->func_aliases[id];
			}


			///////////////////////////////////
			// templated funcs

			[[nodiscard]] auto createTemplatedFunc(auto&&... args) -> sema::TemplatedFunc::ID {
				return this->templated_funcs.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getTemplatedFunc(sema::TemplatedFunc::ID id) const -> const sema::TemplatedFunc& {
				return this->templated_funcs[id];
			}


			///////////////////////////////////
			// templated structs

			[[nodiscard]] auto createTemplatedStruct(auto&&... args) -> sema::TemplatedStruct::ID {
				return this->templated_structs.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getTemplatedStruct(sema::TemplatedStruct::ID id) const -> const sema::TemplatedStruct& {
				return this->templated_structs[id];
			}


			///////////////////////////////////
			// struct template alias

			[[nodiscard]] auto createStructTemplateAlias(auto&&... args) -> sema::StructTemplateAlias::ID {
				return this->struct_template_aliases.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getStructTemplateAlias(sema::StructTemplateAlias::ID id) const
			-> const sema::StructTemplateAlias& {
				return this->struct_template_aliases[id];
			}


			///////////////////////////////////
			// vars

			[[nodiscard]] auto createVar(auto&&... args) -> sema::Var::ID {
				return this->vars.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getVar(sema::Var::ID id) const -> const sema::Var& {
				return this->vars[id];
			}

			[[nodiscard]] auto getVars() const -> evo::IterRange<sema::Var::ID::Iterator> {
				return evo::IterRange<sema::Var::ID::Iterator>(
					sema::Var::ID::Iterator(sema::Var::ID(0)),
					sema::Var::ID::Iterator(sema::Var::ID(uint32_t(this->vars.size())))
				);
			};

			[[nodiscard]] auto numVars() const -> size_t { return this->vars.size(); }


			///////////////////////////////////
			// global vars

			[[nodiscard]] auto createGlobalVar(auto&&... args) -> sema::GlobalVar::ID {
				return this->global_vars.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getGlobalVar(sema::GlobalVar::ID id) const -> const sema::GlobalVar& {
				return this->global_vars[id];
			}

			[[nodiscard]] auto getGlobalVars() const -> evo::IterRange<sema::GlobalVar::ID::Iterator> {
				return evo::IterRange<sema::GlobalVar::ID::Iterator>(
					sema::GlobalVar::ID::Iterator(sema::GlobalVar::ID(0)),
					sema::GlobalVar::ID::Iterator(sema::GlobalVar::ID(uint32_t(this->global_vars.size())))
				);
			};

			[[nodiscard]] auto numGlobalVars() const -> size_t { return this->global_vars.size(); }


			///////////////////////////////////
			// params

			[[nodiscard]] auto createParam(auto&&... args) -> sema::Param::ID {
				return this->params.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getParam(sema::Param::ID id) const -> const sema::Param& {
				return this->params[id];
			}


			///////////////////////////////////
			// variadic params

			[[nodiscard]] auto createVariadicParam(auto&&... args) -> sema::VariadicParam::ID {
				return this->variadic_params.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getVariadicParam(sema::VariadicParam::ID id) const -> const sema::VariadicParam& {
				return this->variadic_params[id];
			}


			///////////////////////////////////
			// return params

			[[nodiscard]] auto createReturnParam(auto&&... args) -> sema::ReturnParam::ID {
				return this->return_params.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getReturnParam(sema::ReturnParam::ID id) const -> const sema::ReturnParam& {
				return this->return_params[id];
			}


			///////////////////////////////////
			// error return params

			[[nodiscard]] auto createErrorReturnParam(auto&&... args) -> sema::ErrorReturnParam::ID {
				return this->error_return_params.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getErrorReturnParam(sema::ErrorReturnParam::ID id) const
			-> const sema::ErrorReturnParam& {
				return this->error_return_params[id];
			}


			///////////////////////////////////
			// block expr outputs

			[[nodiscard]] auto createBlockExprOutput(auto&&... args) -> sema::BlockExprOutput::ID {
				return this->block_expr_outputs.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getBlockExprOutput(sema::BlockExprOutput::ID id) const -> const sema::BlockExprOutput& {
				return this->block_expr_outputs[id];
			}


			///////////////////////////////////
			// except param

			[[nodiscard]] auto createExceptParam(auto&&... args) -> sema::ExceptParam::ID {
				return this->except_params.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getExceptParam(sema::ExceptParam::ID id) const -> const sema::ExceptParam& {
				return this->except_params[id];
			}


			///////////////////////////////////
			// for param

			[[nodiscard]] auto createForParam(auto&&... args) -> sema::ForParam::ID {
				return this->for_params.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getForParam(sema::ForParam::ID id) const -> const sema::ForParam& {
				return this->for_params[id];
			}


			///////////////////////////////////
			// func calls

			[[nodiscard]] auto createFuncCall(auto&&... args) -> sema::FuncCall::ID {
				return this->func_calls.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getFuncCall(sema::FuncCall::ID id) const -> const sema::FuncCall& {
				return this->func_calls[id];
			}


			///////////////////////////////////
			// try else

			[[nodiscard]] auto createTryElse(auto&&... args) -> sema::TryElse::ID {
				return this->try_elses.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getTryElse(sema::TryElse::ID id) const -> const sema::TryElse& {
				return this->try_elses[id];
			}


			///////////////////////////////////
			// try else interface

			[[nodiscard]] auto createTryElseInterface(auto&&... args) -> sema::TryElseInterface::ID {
				return this->try_else_interfaces.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getTryElseInterface(sema::TryElseInterface::ID id) const
			-> const sema::TryElseInterface& {
				return this->try_else_interfaces[id];
			}


			///////////////////////////////////
			// assignments

			[[nodiscard]] auto createAssign(auto&&... args) -> sema::Assign::ID {
				return this->assigns.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getAssign(sema::Assign::ID id) const -> const sema::Assign& {
				return this->assigns[id];
			}


			///////////////////////////////////
			// multi-assign

			[[nodiscard]] auto createMultiAssign(auto&&... args) -> sema::MultiAssign::ID {
				return this->multi_assigns.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getMultiAssign(sema::MultiAssign::ID id) const -> const sema::MultiAssign& {
				return this->multi_assigns[id];
			}


			///////////////////////////////////
			// returns

			[[nodiscard]] auto createReturn(auto&&... args) -> sema::Return::ID {
				return this->returns.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getReturn(sema::Return::ID id) const -> const sema::Return& {
				return this->returns[id];
			}


			///////////////////////////////////
			// errors

			[[nodiscard]] auto createError(auto&&... args) -> sema::Error::ID {
				return this->errors.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getError(sema::Error::ID id) const -> const sema::Error& {
				return this->errors[id];
			}


			///////////////////////////////////
			// breaks

			[[nodiscard]] auto createBreak(auto&&... args) -> sema::Break::ID {
				return this->breaks.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getBreak(sema::Break::ID id) const -> const sema::Break& {
				return this->breaks[id];
			}


			///////////////////////////////////
			// continues

			[[nodiscard]] auto createContinue(auto&&... args) -> sema::Continue::ID {
				return this->continues.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getContinue(sema::Continue::ID id) const -> const sema::Continue& {
				return this->continues[id];
			}


			///////////////////////////////////
			// deletes

			[[nodiscard]] auto createDelete(auto&&... args) -> sema::Delete::ID {
				return this->deletes.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getDelete(sema::Delete::ID id) const -> const sema::Delete& {
				return this->deletes[id];
			}


			///////////////////////////////////
			// block scopes

			[[nodiscard]] auto createBlockScope(auto&&... args) -> sema::BlockScope::ID {
				return this->block_scopes.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getBlockScope(sema::BlockScope::ID id) const -> const sema::BlockScope& {
				return this->block_scopes[id];
			}


			///////////////////////////////////
			// conditionals

			[[nodiscard]] auto createConditional(auto&&... args) -> sema::Conditional::ID {
				return this->conds.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getConditional(sema::Conditional::ID id) const -> const sema::Conditional& {
				return this->conds[id];
			}


			///////////////////////////////////
			// whiles

			[[nodiscard]] auto createWhile(auto&&... args) -> sema::While::ID {
				return this->whiles.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getWhile(sema::While::ID id) const -> const sema::While& {
				return this->whiles[id];
			}


			///////////////////////////////////
			// fors

			[[nodiscard]] auto createFor(auto&&... args) -> sema::For::ID {
				return this->fors.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getFor(sema::For::ID id) const -> const sema::For& {
				return this->fors[id];
			}


			///////////////////////////////////
			// for unrolls

			[[nodiscard]] auto createForUnroll(auto&&... args) -> sema::ForUnroll::ID {
				return this->for_unrolls.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getForUnroll(sema::ForUnroll::ID id) const -> const sema::ForUnroll& {
				return this->for_unrolls[id];
			}


			///////////////////////////////////
			// switches

			[[nodiscard]] auto createSwitch(auto&&... args) -> sema::Switch::ID {
				return this->switches.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getSwitch(sema::Switch::ID id) const -> const sema::Switch& {
				return this->switches[id];
			}


			///////////////////////////////////
			// defers

			[[nodiscard]] auto createDefer(auto&&... args) -> sema::Defer::ID {
				return this->defers.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getDefer(sema::Defer::ID id) const -> const sema::Defer& {
				return this->defers[id];
			}


			///////////////////////////////////
			// lifetime start

			[[nodiscard]] auto createLifetimeStart(auto&&... args) -> sema::LifetimeStart::ID {
				return this->lifetime_starts.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getLifetimeStart(sema::LifetimeStart::ID id) const -> const sema::LifetimeStart& {
				return this->lifetime_starts[id];
			}


			///////////////////////////////////
			// lifetime end

			[[nodiscard]] auto createLifetimeEnd(auto&&... args) -> sema::LifetimeEnd::ID {
				return this->lifetime_ends.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getLifetimeEnd(sema::LifetimeEnd::ID id) const -> const sema::LifetimeEnd& {
				return this->lifetime_ends[id];
			}


			///////////////////////////////////
			// unused expr

			[[nodiscard]] auto createUnusedExpr(auto&&... args) -> sema::UnusedExpr::ID {
				return this->unused_exprs.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getUnusedExpr(sema::UnusedExpr::ID id) const -> const sema::UnusedExpr& {
				return this->unused_exprs[id];
			}


			///////////////////////////////////
			// copies

			[[nodiscard]] auto createCopy(auto&&... args) -> sema::Copy::ID {
				return this->copies.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getCopy(sema::Copy::ID id) const -> const sema::Copy& {
				return this->copies[id];
			}


			///////////////////////////////////
			// moves

			[[nodiscard]] auto createMove(auto&&... args) -> sema::Move::ID {
				return this->moves.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getMove(sema::Move::ID id) const -> const sema::Move& {
				return this->moves[id];
			}


			///////////////////////////////////
			// forwards

			[[nodiscard]] auto createForward(auto&&... args) -> sema::Forward::ID {
				return this->forwards.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getForward(sema::Forward::ID id) const -> const sema::Forward& {
				return this->forwards[id];
			}


			///////////////////////////////////
			// address ofs

			[[nodiscard]] auto createAddrOf(auto&&... args) -> sema::AddrOf::ID {
				return sema::AddrOf::ID(this->misc_exprs.emplace_back(std::forward<decltype(args)>(args)...));
			}

			[[nodiscard]] auto getAddrOf(sema::AddrOf::ID id) const -> const sema::Expr& {
				return this->misc_exprs[id.get()];
			}


			///////////////////////////////////
			// optional null check

			[[nodiscard]] auto createConversionToOptional(auto&&... args)
			-> sema::ConversionToOptional::ID {
				return this->conversion_to_optionals.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getConversionToOptional(sema::ConversionToOptional::ID id) const
			-> const sema::ConversionToOptional& {
				return this->conversion_to_optionals[id];
			}


			///////////////////////////////////
			// optional null check

			[[nodiscard]] auto createOptionalNullCheck(auto&&... args) -> sema::OptionalNullCheck::ID {
				return this->optional_null_checks.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getOptionalNullCheck(sema::OptionalNullCheck::ID id) const
			-> const sema::OptionalNullCheck& {
				return this->optional_null_checks[id];
			}


			///////////////////////////////////
			// optional extract

			[[nodiscard]] auto createOptionalExtract(auto&&... args) -> sema::OptionalExtract::ID {
				return this->optional_extracts.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getOptionalExtract(sema::OptionalExtract::ID id) const
			-> const sema::OptionalExtract& {
				return this->optional_extracts[id];
			}



			///////////////////////////////////
			// dereferences

			[[nodiscard]] auto createDeref(auto&&... args) -> sema::Deref::ID {
				return this->derefs.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getDeref(sema::Deref::ID id) const -> const sema::Deref& {
				return this->derefs[id];
			}


			///////////////////////////////////
			// unwraps

			[[nodiscard]] auto createUnwrap(auto&&... args) -> sema::Unwrap::ID {
				return this->unwraps.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getUnwrap(sema::Unwrap::ID id) const -> const sema::Unwrap& {
				return this->unwraps[id];
			}


			///////////////////////////////////
			// accessors

			[[nodiscard]] auto createAccessor(auto&&... args) -> sema::Accessor::ID {
				return this->accessors.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getAccessor(sema::Accessor::ID id) const -> const sema::Accessor& {
				return this->accessors[id];
			}


			///////////////////////////////////
			// union accessors

			[[nodiscard]] auto createUnionAccessor(auto&&... args) -> sema::UnionAccessor::ID {
				return this->union_accessors.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getUnionAccessor(sema::UnionAccessor::ID id) const -> const sema::UnionAccessor& {
				return this->union_accessors[id];
			}


			///////////////////////////////////
			// logical and

			[[nodiscard]] auto createLogicalAnd(auto&&... args) -> sema::LogicalAnd::ID {
				return this->logical_ands.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getLogicalAnd(sema::LogicalAnd::ID id) const -> const sema::LogicalAnd& {
				return this->logical_ands[id];
			}


			///////////////////////////////////
			// logical or

			[[nodiscard]] auto createLogicalOr(auto&&... args) -> sema::LogicalOr::ID {
				return this->logical_ors.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getLogicalOr(sema::LogicalOr::ID id) const -> const sema::LogicalOr& {
				return this->logical_ors[id];
			}


			///////////////////////////////////
			// try/else expr

			[[nodiscard]] auto createTryElseExpr(auto&&... args) -> sema::TryElseExpr::ID {
				return this->try_else_exprs.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getTryElseExpr(sema::TryElseExpr::ID id) const -> const sema::TryElseExpr& {
				return this->try_else_exprs[id];
			}


			///////////////////////////////////
			// try/else interface expr

			[[nodiscard]] auto createTryElseInterfaceExpr(auto&&... args) -> sema::TryElseInterfaceExpr::ID {
				return this->try_else_interface_exprs.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getTryElseInterfaceExpr(sema::TryElseInterfaceExpr::ID id) const
			-> const sema::TryElseInterfaceExpr& {
				return this->try_else_interface_exprs[id];
			}


			///////////////////////////////////
			// block expr

			[[nodiscard]] auto createBlockExpr(auto&&... args) -> sema::BlockExpr::ID {
				return this->block_exprs.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getBlockExpr(sema::BlockExpr::ID id) const -> const sema::BlockExpr& {
				return this->block_exprs[id];
			}


			///////////////////////////////////
			// fake term info

			[[nodiscard]] auto createFakeTermInfo(auto&&... args) -> sema::FakeTermInfo::ID {
				return this->fake_term_infos.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getFakeTermInfo(sema::FakeTermInfo::ID id) const -> const sema::FakeTermInfo& {
				return this->fake_term_infos[id];
			}


			///////////////////////////////////
			// make interface ptr

			[[nodiscard]] auto createMakeInterfacePtr(auto&&... args) -> sema::MakeInterfacePtr::ID {
				return this->make_interface_ptrs.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getMakeInterfacePtr(sema::MakeInterfacePtr::ID id) const
			-> const sema::MakeInterfacePtr& {
				return this->make_interface_ptrs[id];
			}


			///////////////////////////////////
			// interface ptr extract this

			[[nodiscard]] auto createInterfacePtrExtractThis(auto&&... args) -> sema::InterfacePtrExtractThis::ID {
				return this->interface_ptr_extract_thises.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getInterfacePtrExtractThis(sema::InterfacePtrExtractThis::ID id) const
			-> const sema::InterfacePtrExtractThis& {
				return this->interface_ptr_extract_thises[id];
			}


			///////////////////////////////////
			// interface call

			[[nodiscard]] auto createInterfaceCall(auto&&... args) -> sema::InterfaceCall::ID {
				return this->interface_calls.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getInterfaceCall(sema::InterfaceCall::ID id) const -> const sema::InterfaceCall& {
				return this->interface_calls[id];
			}


			///////////////////////////////////
			// indexer

			[[nodiscard]] auto createIndexer(auto&&... args) -> sema::Indexer::ID {
				return this->indexers.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getIndexer(sema::Indexer::ID id) const -> const sema::Indexer& {
				return this->indexers[id];
			}


			///////////////////////////////////
			// default init

			[[nodiscard]] auto createDefaultNew(auto&&... args) -> sema::DefaultNew::ID {
				return this->default_news.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getDefaultNew(sema::DefaultNew::ID id) const
			-> const sema::DefaultNew& {
				return this->default_news[id];
			}


			///////////////////////////////////
			// init array ref

			[[nodiscard]] auto createInitArrayRef(auto&&... args) -> sema::InitArrayRef::ID {
				return this->init_array_ref.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getInitArrayRef(sema::InitArrayRef::ID id) const -> const sema::InitArrayRef& {
				return this->init_array_ref[id];
			}


			///////////////////////////////////
			// array ref indexer

			[[nodiscard]] auto createArrayRefIndexer(auto&&... args) -> sema::ArrayRefIndexer::ID {
				return this->array_ref_indexers.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getArrayRefIndexer(sema::ArrayRefIndexer::ID id) const -> const sema::ArrayRefIndexer& {
				return this->array_ref_indexers[id];
			}


			///////////////////////////////////
			// array ref size

			[[nodiscard]] auto createArrayRefSize(auto&&... args) -> sema::ArrayRefSize::ID {
				return this->array_ref_size.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getArrayRefSize(sema::ArrayRefSize::ID id) const -> const sema::ArrayRefSize& {
				return this->array_ref_size[id];
			}


			///////////////////////////////////
			// array ref dimensions

			[[nodiscard]] auto createArrayRefDimensions(auto&&... args) -> sema::ArrayRefDimensions::ID {
				return this->array_ref_dimensions.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getArrayRefDimensions(sema::ArrayRefDimensions::ID id) const
			-> const sema::ArrayRefDimensions& {
				return this->array_ref_dimensions[id];
			}


			///////////////////////////////////
			// array ref data

			[[nodiscard]] auto createArrayRefData(auto&&... args) -> sema::ArrayRefData::ID {
				return this->array_ref_data.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getArrayRefData(sema::ArrayRefData::ID id) const -> const sema::ArrayRefData& {
				return this->array_ref_data[id];
			}


			///////////////////////////////////
			// union designated init new

			[[nodiscard]] auto createUnionDesignatedInitNew(auto&&... args) -> sema::UnionDesignatedInitNew::ID {
				return this->union_designated_init_news.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getUnionDesignatedInitNew(sema::UnionDesignatedInitNew::ID id) const
			-> const sema::UnionDesignatedInitNew& {
				return this->union_designated_init_news[id];
			}


			///////////////////////////////////
			// union tag cmp

			[[nodiscard]] auto createUnionTagCmp(auto&&... args) -> sema::UnionTagCmp::ID {
				return this->union_tag_cmps.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getUnionTagCmp(sema::UnionTagCmp::ID id) const -> const sema::UnionTagCmp& {
				return this->union_tag_cmps[id];
			}


			///////////////////////////////////
			// same type cmp

			[[nodiscard]] auto createSameTypeCmp(auto&&... args) -> sema::SameTypeCmp::ID {
				return this->same_type_cmps.emplace_back(std::forward<decltype(args)>(args)...);
			}

			[[nodiscard]] auto getSameTypeCmp(sema::SameTypeCmp::ID id) const -> const sema::SameTypeCmp& {
				return this->same_type_cmps[id];
			}


			///////////////////////////////////
			// template intrinsic instantiations

			[[nodiscard]] auto createTemplateIntrinsicFuncInstantiation(auto&&... args)
			-> sema::TemplateIntrinsicFuncInstantiation::ID {
				return this->templated_intrinsic_func_instantiations.emplace_back(
					std::forward<decltype(args)>(args)...
				);
			}

			[[nodiscard]] auto getTemplateIntrinsicFuncInstantiation(
				sema::TemplateIntrinsicFuncInstantiation::ID id
			) const -> const sema::TemplateIntrinsicFuncInstantiation& {
				return this->templated_intrinsic_func_instantiations[id];
			}


			///////////////////////////////////
			// ints

			[[nodiscard]] auto createIntValue(core::GenericInt integer, std::optional<BaseType::ID> type_info_id)
			-> sema::IntValue::ID {
				return this->int_values.emplace_back(integer, type_info_id);
			}

			[[nodiscard]] auto getIntValue(sema::IntValue::ID id) const -> const sema::IntValue& {
				return this->int_values[id];
			}


			///////////////////////////////////
			// floats

			[[nodiscard]] auto createFloatValue(
				core::GenericFloat floating_point, std::optional<BaseType::ID> type_info_id
			) -> sema::FloatValue::ID {
				return this->float_values.emplace_back(floating_point, type_info_id);
			}

			[[nodiscard]] auto getFloatValue(sema::FloatValue::ID id) const -> const sema::FloatValue& {
				return this->float_values[id];
			}


			///////////////////////////////////
			// bools

			[[nodiscard]] auto createBoolValue(bool boolean) -> sema::BoolValue::ID {
				return this->bool_values.emplace_back(boolean);
			}

			[[nodiscard]] auto getBoolValue(sema::BoolValue::ID id) const -> const sema::BoolValue& {
				return this->bool_values[id];
			}

			///////////////////////////////////
			// strings

			[[nodiscard]] auto createStringValue(std::string&& value) -> sema::StringValue::ID {
				return this->string_values.emplace_back(std::move(value));
			}

			[[nodiscard]] auto createStringValue(const std::string& value) -> sema::StringValue::ID {
				return this->string_values.emplace_back(value);
			}

			[[nodiscard]] auto getStringValue(sema::StringValue::ID id) const -> const sema::StringValue& {
				return this->string_values[id];
			}


			///////////////////////////////////
			// aggregates

			[[nodiscard]] auto createAggregateValue(evo::SmallVector<sema::Expr>&& values, BaseType::ID typeID)
			-> sema::AggregateValue::ID {
				return this->aggregate_values.emplace_back(std::move(values), typeID);
			}

			[[nodiscard]] auto createAggregateValue(
				const evo::SmallVector<sema::Expr>& values, BaseType::ID typeID
			) -> sema::AggregateValue::ID {
				return this->aggregate_values.emplace_back(values, typeID);
			}

			[[nodiscard]] auto getAggregateValue(sema::AggregateValue::ID id) const -> const sema::AggregateValue& {
				return this->aggregate_values[id];
			}


			///////////////////////////////////
			// chars

			[[nodiscard]] auto createCharValue(char character) -> sema::CharValue::ID {
				return this->char_values.emplace_back(character);
			}

			[[nodiscard]] auto getCharValue(sema::CharValue::ID id) const -> const sema::CharValue& {
				return this->char_values[id];
			}


			///////////////////////////////////
			// null

			[[nodiscard]] auto createNull(Token::ID null_token_id) -> sema::Null::ID {
				return sema::Null::ID(this->misc_tokens.emplace_back(null_token_id));
			}

			[[nodiscard]] auto getNull(sema::Uninit::ID id) const -> Token::ID {
				return this->misc_tokens[id.get()];
			}


			///////////////////////////////////
			// uninit

			[[nodiscard]] auto createUninit(Token::ID uninit_token_id) -> sema::Uninit::ID {
				return sema::Uninit::ID(this->misc_tokens.emplace_back(uninit_token_id));
			}

			[[nodiscard]] auto getUninit(sema::Uninit::ID id) const -> Token::ID {
				return this->misc_tokens[id.get()];
			}


			///////////////////////////////////
			// zeroinit

			[[nodiscard]] auto createZeroinit(Token::ID zeroinit_token_id) -> sema::Zeroinit::ID {
				return sema::Zeroinit::ID(this->misc_tokens.emplace_back(zeroinit_token_id));
			}

			[[nodiscard]] auto getZeroinit(sema::Zeroinit::ID id) const -> Token::ID {
				return this->misc_tokens[id.get()];
			}


	
		private:
			core::SyncLinearStepAlloc<sema::Func, sema::Func::ID> funcs{};
			core::SyncLinearStepAlloc<sema::FuncAlias, sema::FuncAlias::ID> func_aliases{};
			core::SyncLinearStepAlloc<sema::TemplatedFunc, sema::TemplatedFunc::ID> templated_funcs{};
			core::SyncLinearStepAlloc<sema::TemplatedStruct, sema::TemplatedStruct::ID> templated_structs{};
			core::SyncLinearStepAlloc<sema::StructTemplateAlias, sema::StructTemplateAlias::ID>
				struct_template_aliases{};
			core::SyncLinearStepAlloc<sema::Var, sema::Var::ID> vars{};
			core::SyncLinearStepAlloc<sema::GlobalVar, sema::GlobalVar::ID> global_vars{};
			core::SyncLinearStepAlloc<sema::Param, sema::Param::ID> params{};
			core::SyncLinearStepAlloc<sema::VariadicParam, sema::VariadicParam::ID> variadic_params{};
			core::SyncLinearStepAlloc<sema::ReturnParam, sema::ReturnParam::ID> return_params{};
			core::SyncLinearStepAlloc<sema::ErrorReturnParam, sema::ErrorReturnParam::ID> error_return_params{};
			core::SyncLinearStepAlloc<sema::BlockExprOutput, sema::BlockExprOutput::ID> block_expr_outputs{};
			core::SyncLinearStepAlloc<sema::ExceptParam, sema::ExceptParam::ID> except_params{};
			core::SyncLinearStepAlloc<sema::ForParam, sema::ForParam::ID> for_params{};

			core::SyncLinearStepAlloc<sema::FuncCall, sema::FuncCall::ID> func_calls{};
			core::SyncLinearStepAlloc<sema::TryElse, sema::TryElse::ID> try_elses{};
			core::SyncLinearStepAlloc<sema::TryElseInterface, sema::TryElseInterface::ID> try_else_interfaces{};
			core::SyncLinearStepAlloc<sema::Assign, sema::Assign::ID> assigns{};
			core::SyncLinearStepAlloc<sema::MultiAssign, sema::MultiAssign::ID> multi_assigns{};
			core::SyncLinearStepAlloc<sema::Return, sema::Return::ID> returns{};
			core::SyncLinearStepAlloc<sema::Error, sema::Error::ID> errors{};
			core::SyncLinearStepAlloc<sema::Break, sema::Break::ID> breaks{};
			core::SyncLinearStepAlloc<sema::Continue, sema::Continue::ID> continues{};
			core::SyncLinearStepAlloc<sema::Delete, sema::Delete::ID> deletes{};
			core::SyncLinearStepAlloc<sema::BlockScope, sema::BlockScope::ID> block_scopes{};
			core::SyncLinearStepAlloc<sema::Conditional, sema::Conditional::ID> conds{};
			core::SyncLinearStepAlloc<sema::While, sema::While::ID> whiles{};
			core::SyncLinearStepAlloc<sema::For, sema::For::ID> fors{};
			core::SyncLinearStepAlloc<sema::ForUnroll, sema::ForUnroll::ID> for_unrolls{};
			core::SyncLinearStepAlloc<sema::Switch, sema::Switch::ID> switches{};
			core::SyncLinearStepAlloc<sema::Defer, sema::Defer::ID> defers{};
			core::SyncLinearStepAlloc<sema::LifetimeStart, sema::LifetimeStart::ID> lifetime_starts{};
			core::SyncLinearStepAlloc<sema::LifetimeEnd, sema::LifetimeEnd::ID> lifetime_ends{};
			core::SyncLinearStepAlloc<sema::UnusedExpr, sema::UnusedExpr::ID> unused_exprs{};
			core::SyncLinearStepAlloc<sema::Copy, sema::Copy::ID> copies{};
			core::SyncLinearStepAlloc<sema::Move, sema::Move::ID> moves{};
			core::SyncLinearStepAlloc<sema::Forward, sema::Forward::ID> forwards{};

			core::SyncLinearStepAlloc<sema::Expr, uint32_t> misc_exprs{};
			core::SyncLinearStepAlloc<sema::Deref, sema::Deref::ID> derefs{};
			core::SyncLinearStepAlloc<sema::Unwrap, sema::Unwrap::ID> unwraps{};
			core::SyncLinearStepAlloc<sema::ConversionToOptional, sema::ConversionToOptional::ID> 
				conversion_to_optionals{};
			core::SyncLinearStepAlloc<sema::OptionalNullCheck, sema::OptionalNullCheck::ID> optional_null_checks{};
			core::SyncLinearStepAlloc<sema::OptionalExtract, sema::OptionalExtract::ID> optional_extracts{};
			core::SyncLinearStepAlloc<sema::Accessor, sema::Accessor::ID> accessors{};
			core::SyncLinearStepAlloc<sema::UnionAccessor, sema::UnionAccessor::ID> union_accessors{};
			core::SyncLinearStepAlloc<sema::LogicalAnd, sema::LogicalAnd::ID> logical_ands{};
			core::SyncLinearStepAlloc<sema::LogicalOr, sema::LogicalOr::ID> logical_ors{};
			core::SyncLinearStepAlloc<sema::TryElseExpr, sema::TryElseExpr::ID> try_else_exprs{};
			core::SyncLinearStepAlloc<sema::TryElseInterfaceExpr, sema::TryElseInterfaceExpr::ID>
				try_else_interface_exprs{};
			core::SyncLinearStepAlloc<sema::BlockExpr, sema::BlockExpr::ID> block_exprs{};
			core::SyncLinearStepAlloc<sema::FakeTermInfo, sema::FakeTermInfo::ID> fake_term_infos{};
			core::SyncLinearStepAlloc<sema::MakeInterfacePtr, sema::MakeInterfacePtr::ID> make_interface_ptrs{};
			core::SyncLinearStepAlloc<sema::InterfacePtrExtractThis, sema::InterfacePtrExtractThis::ID>
				interface_ptr_extract_thises{};
			core::SyncLinearStepAlloc<sema::InterfaceCall, sema::InterfaceCall::ID> interface_calls{};
			core::SyncLinearStepAlloc<sema::Indexer, sema::Indexer::ID> indexers{};
			core::SyncLinearStepAlloc<sema::DefaultNew, sema::DefaultNew::ID> default_news{};
			core::SyncLinearStepAlloc<sema::InitArrayRef, sema::InitArrayRef::ID> init_array_ref{};
			core::SyncLinearStepAlloc<sema::ArrayRefIndexer, sema::ArrayRefIndexer::ID> array_ref_indexers{};
			core::SyncLinearStepAlloc<sema::ArrayRefSize, sema::ArrayRefSize::ID> array_ref_size{};
			core::SyncLinearStepAlloc<sema::ArrayRefDimensions, sema::ArrayRefDimensions::ID> array_ref_dimensions{};
			core::SyncLinearStepAlloc<sema::ArrayRefData, sema::ArrayRefData::ID> array_ref_data{};
			core::SyncLinearStepAlloc<sema::UnionDesignatedInitNew, sema::UnionDesignatedInitNew::ID>
				union_designated_init_news{};
			core::SyncLinearStepAlloc<sema::UnionTagCmp, sema::UnionTagCmp::ID> union_tag_cmps{};
			core::SyncLinearStepAlloc<sema::SameTypeCmp, sema::SameTypeCmp::ID> same_type_cmps{};

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
