////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../../include/sema/SemaBuffer.hpp"

#include "ScopeManager.hpp"


namespace pcit::panther::sema{


	struct SemaBuffer::Internal{
		core::SyncLinearStepAlloc<Func, Func::ID> funcs{};
		core::SyncLinearStepAlloc<FuncAlias, FuncAlias::ID> func_aliases{};
		core::SyncLinearStepAlloc<TemplatedFunc, TemplatedFunc::ID> templated_funcs{};
		core::SyncLinearStepAlloc<TemplatedStruct, TemplatedStruct::ID> templated_structs{};
		core::SyncLinearStepAlloc<StructTemplateAlias, StructTemplateAlias::ID>
			struct_template_aliases{};
		core::SyncLinearStepAlloc<Var, Var::ID> vars{};
		core::SyncLinearStepAlloc<GlobalVar, GlobalVar::ID> global_vars{};
		core::SyncLinearStepAlloc<Param, Param::ID> _params{};
		core::SyncLinearStepAlloc<VariadicParam, VariadicParam::ID> variadic_params{};
		core::SyncLinearStepAlloc<ReturnParam, ReturnParam::ID> return_params{};
		core::SyncLinearStepAlloc<ErrorReturnParam, ErrorReturnParam::ID> error_return_params{};
		core::SyncLinearStepAlloc<BlockExprOutput, BlockExprOutput::ID> block_expr_outputs{};
		core::SyncLinearStepAlloc<ExceptParam, ExceptParam::ID> except_params{};
		core::SyncLinearStepAlloc<ForParam, ForParam::ID> for_params{};

		core::SyncLinearStepAlloc<FuncCall, FuncCall::ID> func_calls{};
		core::SyncLinearStepAlloc<TryElse, TryElse::ID> try_elses{};
		core::SyncLinearStepAlloc<TryElseInterface, TryElseInterface::ID> try_else_interfaces{};
		core::SyncLinearStepAlloc<Asm, Asm::ID> asms{};
		core::SyncLinearStepAlloc<Assign, Assign::ID> assigns{};
		core::SyncLinearStepAlloc<MultiAssign, MultiAssign::ID> multi_assigns{};
		core::SyncLinearStepAlloc<Return, Return::ID> returns{};
		core::SyncLinearStepAlloc<Error, Error::ID> errors{};
		core::SyncLinearStepAlloc<Unreachable, Unreachable::ID> unreachables{};
		core::SyncLinearStepAlloc<Break, Break::ID> breaks{};
		core::SyncLinearStepAlloc<Continue, Continue::ID> continues{};
		core::SyncLinearStepAlloc<Delete, Delete::ID> deletes{};
		core::SyncLinearStepAlloc<BlockScope, BlockScope::ID> block_scopes{};
		core::SyncLinearStepAlloc<Conditional, Conditional::ID> conds{};
		core::SyncLinearStepAlloc<While, While::ID> whiles{};
		core::SyncLinearStepAlloc<For, For::ID> fors{};
		core::SyncLinearStepAlloc<ForUnroll, ForUnroll::ID> for_unrolls{};
		core::SyncLinearStepAlloc<Switch, Switch::ID> switches{};
		core::SyncLinearStepAlloc<Defer, Defer::ID> defers{};
		core::SyncLinearStepAlloc<LifetimeStart, LifetimeStart::ID> lifetime_starts{};
		core::SyncLinearStepAlloc<LifetimeEnd, LifetimeEnd::ID> lifetime_ends{};
		core::SyncLinearStepAlloc<UnusedExpr, UnusedExpr::ID> unused_exprs{};
		core::SyncLinearStepAlloc<Copy, Copy::ID> copies{};
		core::SyncLinearStepAlloc<Move, Move::ID> moves{};
		core::SyncLinearStepAlloc<Forward, Forward::ID> forwards{};

		core::SyncLinearStepAlloc<Expr, uint32_t> misc_exprs{};
		core::SyncLinearStepAlloc<FuncPtr, FuncPtr::ID> func_ptrs{};
		core::SyncLinearStepAlloc<Deref, Deref::ID> derefs{};
		core::SyncLinearStepAlloc<Unwrap, Unwrap::ID> unwraps{};
		core::SyncLinearStepAlloc<ConversionToOptional, ConversionToOptional::ID> conversion_to_optionals{};
		core::SyncLinearStepAlloc<OptionalNullCheck, OptionalNullCheck::ID> optional_null_checks{};
		core::SyncLinearStepAlloc<OptionalExtract, OptionalExtract::ID> optional_extracts{};
		core::SyncLinearStepAlloc<Accessor, Accessor::ID> accessors{};
		core::SyncLinearStepAlloc<UnionAccessor, UnionAccessor::ID> union_accessors{};
		core::SyncLinearStepAlloc<LogicalAnd, LogicalAnd::ID> logical_ands{};
		core::SyncLinearStepAlloc<LogicalOr, LogicalOr::ID> logical_ors{};
		core::SyncLinearStepAlloc<TryElseExpr, TryElseExpr::ID> try_else_exprs{};
		core::SyncLinearStepAlloc<TryElseInterfaceExpr, TryElseInterfaceExpr::ID> try_else_interface_exprs{};
		core::SyncLinearStepAlloc<BlockExpr, BlockExpr::ID> block_exprs{};
		core::SyncLinearStepAlloc<FakeTermInfo, FakeTermInfo::ID> fake_term_infos{};
		core::SyncLinearStepAlloc<MakeInterfacePtr, MakeInterfacePtr::ID> make_interface_ptrs{};
		core::SyncLinearStepAlloc<InterfacePtrExtractThis, InterfacePtrExtractThis::ID>
			interface_ptr_extract_thises{};
		core::SyncLinearStepAlloc<InterfaceCall, InterfaceCall::ID> interface_calls{};
		core::SyncLinearStepAlloc<Indexer, Indexer::ID> indexers{};
		core::SyncLinearStepAlloc<DefaultNew, DefaultNew::ID> default_news{};
		core::SyncLinearStepAlloc<InitArrayRef, InitArrayRef::ID> init_array_ref{};
		core::SyncLinearStepAlloc<ArrayRefIndexer, ArrayRefIndexer::ID> array_ref_indexers{};
		core::SyncLinearStepAlloc<ArrayRefSize, ArrayRefSize::ID> array_ref_size{};
		core::SyncLinearStepAlloc<ArrayRefDimensions, ArrayRefDimensions::ID> array_ref_dimensions{};
		core::SyncLinearStepAlloc<ArrayRefData, ArrayRefData::ID> array_ref_data{};
		core::SyncLinearStepAlloc<UnionDesignatedInitNew, UnionDesignatedInitNew::ID> union_designated_init_news{};
		core::SyncLinearStepAlloc<UnionTagCmp, UnionTagCmp::ID> union_tag_cmps{};
		core::SyncLinearStepAlloc<SameTypeCmp, SameTypeCmp::ID> same_type_cmps{};

		core::SyncLinearStepAlloc<
			TemplateIntrinsicFuncInstantiation, TemplateIntrinsicFuncInstantiation::ID
		> templated_intrinsic_func_instantiations{};

		core::SyncLinearStepAlloc<IntValue, IntValue::ID> int_values{};
		core::SyncLinearStepAlloc<FloatValue, FloatValue::ID> float_values{};
		core::SyncLinearStepAlloc<BoolValue, BoolValue::ID> bool_values{};
		core::SyncLinearStepAlloc<StringValue, StringValue::ID> string_values{};
		core::SyncLinearStepAlloc<AggregateValue, AggregateValue::ID> aggregate_values{};
		core::SyncLinearStepAlloc<CharValue, CharValue::ID> char_values{};

		core::SyncLinearStepAlloc<Token::ID, uint32_t> misc_tokens{};


		ScopeManager scope_manager{};
	};



	SemaBuffer::SemaBuffer() : internal(new Internal()) {}

	SemaBuffer::~SemaBuffer(){
		delete this->internal;
	}



	auto SemaBuffer::getScopeManager() const -> const ScopeManager& { return this->internal->scope_manager; }
	auto SemaBuffer::getScopeManager()       ->       ScopeManager& { return this->internal->scope_manager; }



	///////////////////////////////////
	// func

	auto SemaBuffer::createFunc(
		evo::Variant<SourceID, CFamilySourceID, BuiltinModuleID> sourceID,
		Func::Name name,
		std::string&& cFamilyMangledName, // empty if not c-family
		std::optional<EncapsulatingSymbolID> parent,
		BaseType::Function::ID typeID,
		evo::SmallVector<Func::Param>&& params,
		evo::SmallVector<Token::ID>&& returnParamIdents, // empty if not named
		evo::SmallVector<Token::ID>&& errorParamIdents,  // empty if not named
		std::optional<SymbolProcID> symbolProcID, // only value if is sema src and not auto-generated
		uint32_t minNumArgs,
		bool hasInParam,
		Func::Attributes attributes,
		std::optional<sema::TemplatedFuncID> templated_func_id,
		uint32_t instanceID
	) -> Func::ID {
		return this->internal->funcs.emplace_back(
			sourceID,
			name,
			std::move(cFamilyMangledName),
			parent,
			typeID,
			std::move(params),
			std::move(returnParamIdents),
			std::move(errorParamIdents),
			symbolProcID,
			minNumArgs,
			hasInParam,
			attributes,
			templated_func_id,
			instanceID
		);
	}


	auto SemaBuffer::getFunc(Func::ID id) const -> const Func& { return this->internal->funcs[id]; }
	auto SemaBuffer::getFunc(Func::ID id)       ->       Func& { return this->internal->funcs[id]; }


	auto SemaBuffer::getFuncs() const -> evo::IterRange<Func::ID::Iterator> {
		return evo::IterRange<Func::ID::Iterator>(
			Func::ID::Iterator(Func::ID(0)),
			Func::ID::Iterator(Func::ID(uint32_t(this->internal->funcs.size())))
		);
	};

	auto SemaBuffer::numFuncs() const -> size_t { return this->internal->funcs.size(); }



	///////////////////////////////////
	// func alias

	auto SemaBuffer::createFuncAlias(
		SourceID sourceID,
		Token::ID ident,
		std::optional<EncapsulatingSymbolID> parent,
		evo::SmallVector<evo::Variant<sema::FuncID, sema::TemplatedFuncID>>&& aliasedOverloads,
		bool isPub,
		bool isPriv
	) -> FuncAlias::ID {
		return this->internal->func_aliases.emplace_back(sourceID, ident, parent, std::move(aliasedOverloads), isPub, isPriv);
	}

	auto SemaBuffer::getFuncAlias(FuncAlias::ID id) const -> const FuncAlias& {
		return this->internal->func_aliases[id];
	}



	///////////////////////////////////
	// templated funcs

	auto SemaBuffer::createTemplatedFunc(
		SymbolProc& symbol_proc,
		size_t min_num_template_args,
		evo::SmallVector<TemplatedFunc::TemplateParam>&& template_params,
		evo::SmallVector<bool>&& param_is_deducer,
		bool is_variadic
	) -> TemplatedFunc::ID {
		return this->internal->templated_funcs.emplace_back(
			symbol_proc, min_num_template_args, std::move(template_params), std::move(param_is_deducer), is_variadic
		);
	}

	auto SemaBuffer::getTemplatedFunc(TemplatedFunc::ID id) const -> const TemplatedFunc& {
		return this->internal->templated_funcs[id];
	}

	auto SemaBuffer::getTemplatedFunc(TemplatedFunc::ID id) -> TemplatedFunc& {
		return this->internal->templated_funcs[id];
	}



	///////////////////////////////////
	// templated strucs

	auto SemaBuffer::createTemplatedStruct(BaseType::StructTemplate::ID templateID, SymbolProc& symbolProc)
	-> TemplatedStruct::ID {
		return this->internal->templated_structs.emplace_back(templateID, symbolProc);
	}


	auto SemaBuffer::getTemplatedStruct(TemplatedStruct::ID id) const -> const TemplatedStruct& {
		return this->internal->templated_structs[id];
	}


	///////////////////////////////////
	// struct template alias

	auto SemaBuffer::createStructTemplateAlias(
		SourceID sourceID,
		Token::ID ident,
		std::optional<EncapsulatingSymbolID> parent,
		evo::Variant<TemplatedStruct::ID, StructTemplateAlias::ID> aliasedID,
		bool requiresPub,
		bool isDistinct,
		bool isPub,
		bool isPriv
	) -> StructTemplateAlias::ID {
		return this->internal->struct_template_aliases.emplace_back(
			sourceID, ident, parent, aliasedID, requiresPub, isDistinct, isPub, isPriv
		);
	}

	auto SemaBuffer::getStructTemplateAlias(StructTemplateAlias::ID id) const -> const StructTemplateAlias& {
		return this->internal->struct_template_aliases[id];
	}



	///////////////////////////////////
	// vars

	auto SemaBuffer::createVar(
		AST::VarDef::Kind kind,
		Token::ID ident,
		Expr expr,
		std::optional<TypeInfo::ID> typeID,
		uint32_t line,
		uint32_t collumn
	) -> Var::ID {
		return this->internal->vars.emplace_back(kind, ident, expr, typeID, line, collumn);
	}


	auto SemaBuffer::getVar(Var::ID id) const -> const Var& {
		return this->internal->vars[id];
	}

	auto SemaBuffer::getVars() const -> evo::IterRange<Var::ID::Iterator> {
		return evo::IterRange<Var::ID::Iterator>(
			Var::ID::Iterator(Var::ID(0)),
			Var::ID::Iterator(Var::ID(uint32_t(this->internal->vars.size())))
		);
	};

	auto SemaBuffer::numVars() const -> size_t { return this->internal->vars.size(); }



	///////////////////////////////////
	// global vars

	auto SemaBuffer::createGlobalVar(
		AST::VarDef::Kind kind,
		evo::Variant<SourceID, CFamilySourceID, BuiltinModuleID> sourceID,
		evo::Variant<Token::ID, CFamilySourceDeclInfoID, BuiltinModuleStringID> ident,
		std::string cFamilyMangledName,
		std::optional<EncapsulatingSymbolID> parent,
		evo::Variant<std::monostate, Expr, GlobalVar::DeletedInfo> value,
		std::optional<TypeInfo::ID> typeID,
		bool isPub,
		bool isPriv,
		std::optional<SymbolProcID> symbolProcID,
		bool defCompleted
	) -> GlobalVar::ID {
		return this->internal->global_vars.emplace_back(
			kind, sourceID, ident, cFamilyMangledName, parent, value, typeID, isPub, isPriv, symbolProcID, defCompleted
		);
	}

	auto SemaBuffer::getGlobalVar(GlobalVar::ID id) const -> const GlobalVar& { return this->internal->global_vars[id]; }
	auto SemaBuffer::getGlobalVar(GlobalVar::ID id)       ->       GlobalVar& { return this->internal->global_vars[id]; }

	auto SemaBuffer::getGlobalVars() const -> evo::IterRange<GlobalVar::ID::Iterator> {
		return evo::IterRange<GlobalVar::ID::Iterator>(
			GlobalVar::ID::Iterator(GlobalVar::ID(0)),
			GlobalVar::ID::Iterator(GlobalVar::ID(uint32_t(this->internal->global_vars.size())))
		);
	};

	auto SemaBuffer::numGlobalVars() const -> size_t { return this->internal->global_vars.size(); }



	///////////////////////////////////
	// params

	auto SemaBuffer::createParam(uint32_t index, uint32_t abiIndex) -> Param::ID {
		return this->internal->_params.emplace_back(index, abiIndex);
	}

	auto SemaBuffer::getParam(Param::ID id) const -> const Param& {
		return this->internal->_params[id];
	}



	///////////////////////////////////
	// variadic param

	auto SemaBuffer::createVariadicParam(uint32_t startIndex, uint32_t startABIIndex, uint32_t numParams)
	-> VariadicParam::ID {
		return this->internal->variadic_params.emplace_back(startIndex, startABIIndex, numParams);
	}

	auto SemaBuffer::getVariadicParam(VariadicParam::ID id) const -> const VariadicParam& {
		return this->internal->variadic_params[id];
	}


	///////////////////////////////////
	// return param

	auto SemaBuffer::createReturnParam(uint32_t index, uint32_t abiIndex) -> ReturnParam::ID {
		return this->internal->return_params.emplace_back(index, abiIndex);
	}

	auto SemaBuffer::getReturnParam(ReturnParam::ID id) const -> const ReturnParam& {
		return this->internal->return_params[id];
	}


	///////////////////////////////////
	// error return params

	auto SemaBuffer::createErrorReturnParam(uint32_t index, uint32_t abiIndex) -> ErrorReturnParam::ID {
		return this->internal->error_return_params.emplace_back(index, abiIndex);
	}

	auto SemaBuffer::getErrorReturnParam(ErrorReturnParam::ID id) const -> const ErrorReturnParam& {
		return this->internal->error_return_params[id];
	}


	///////////////////////////////////
	// block expr outputs

	auto SemaBuffer::createBlockExprOutput(
		uint32_t index, Token::ID label, Token::ID ident, TypeInfo::ID typeID
	) -> BlockExprOutput::ID {
		return this->internal->block_expr_outputs.emplace_back(index, label, ident, typeID);
	}

	auto SemaBuffer::getBlockExprOutput(BlockExprOutput::ID id) const -> const BlockExprOutput& {
		return this->internal->block_expr_outputs[id];
	}


	///////////////////////////////////
	// except param

	auto SemaBuffer::createExceptParam(Token::ID ident, uint32_t index, TypeInfo::ID typeID) -> ExceptParam::ID {
		return this->internal->except_params.emplace_back(ident, index, typeID);
	}

	auto SemaBuffer::getExceptParam(ExceptParam::ID id) const -> const ExceptParam& {
		return this->internal->except_params[id];
	}


	///////////////////////////////////
	// for param

	auto SemaBuffer::createForParam(Token::ID ident, TypeInfo::ID typeID, bool isIndex, bool isMut)
		-> ForParam::ID {
		return this->internal->for_params.emplace_back(ident, typeID, isIndex, isMut);
	}

	auto SemaBuffer::getForParam(ForParam::ID id) const -> const ForParam& {
		return this->internal->for_params[id];
	}


	///////////////////////////////////
	// func calls

	auto SemaBuffer::createFuncCall(
		evo::Variant<
			FuncID, IntrinsicFunc::Kind, TemplateIntrinsicFuncInstantiationID, FuncCall::FuncPtr
		> target,
		evo::SmallVector<Expr>&& args,
		uint32_t line,
		uint32_t collumn
	) -> FuncCall::ID {
		return this->internal->func_calls.emplace_back(target, std::move(args), line, collumn);
	}

	auto SemaBuffer::getFuncCall(FuncCall::ID id) const -> const FuncCall& {
		return this->internal->func_calls[id];
	}


	///////////////////////////////////
	// try else

	auto SemaBuffer::createTryElse(
		evo::Variant<FuncID, TryElse::FuncPtr> target,
		evo::SmallVector<Expr>&& args,
		evo::SmallVector<ExceptParamID>&& exceptParams,
		StmtBlock&& elseBlock,
		uint32_t line,
		uint32_t collumn
	) -> TryElse::ID {
		return this->internal->try_elses.emplace_back(
			target, std::move(args), std::move(exceptParams), std::move(elseBlock), line, collumn
		);
	}

	auto SemaBuffer::getTryElse(TryElse::ID id) const -> const TryElse& {
		return this->internal->try_elses[id];
	}

	auto SemaBuffer::getTryElse(TryElse::ID id) -> TryElse& {
		return this->internal->try_elses[id];
	}


	///////////////////////////////////
	// try else interface

	auto SemaBuffer::createTryElseInterface(
		Expr value,
		BaseType::Function::ID funcTypeID,
		BaseType::Interface::ID interfaceID,
		uint32_t vtableFuncIndex,
		evo::SmallVector<Expr>&& args,
		evo::SmallVector<ExceptParamID>&& exceptParams,
		StmtBlock&& elseBlock,
		uint32_t line,
		uint32_t collumn
	) -> TryElseInterface::ID {
		return this->internal->try_else_interfaces.emplace_back(
			value,
			funcTypeID,
			interfaceID,
			vtableFuncIndex,
			std::move(args),
			std::move(exceptParams),
			std::move(elseBlock),
			line,
			collumn
		);
	}

	auto SemaBuffer::getTryElseInterface(TryElseInterface::ID id) const -> const TryElseInterface& {
		return this->internal->try_else_interfaces[id];
	}

	auto SemaBuffer::getTryElseInterface(TryElseInterface::ID id) -> TryElseInterface& {
		return this->internal->try_else_interfaces[id];
	}


	///////////////////////////////////
	// asm

	auto SemaBuffer::createAsm(
		std::string_view code,
		evo::SmallVector<Asm::Param>&& params,
		evo::SmallVector<std::string_view>&& clobbers,
		evo::SmallVector<Asm::RetParam>&& retParams,
		bool isSideEffect,
		bool isAlignStack,
		uint32_t line,
		uint32_t collumn
	) -> Asm::ID {
		return this->internal->asms.emplace_back(
			code,
			std::move(params),
			std::move(clobbers),
			std::move(retParams),
			isSideEffect,
			isAlignStack,
			line,
			collumn
		);
	}

	auto SemaBuffer::getAsm(Asm::ID id) const -> const Asm& {
		return this->internal->asms[id];
	}


	///////////////////////////////////
	// assignments

	auto SemaBuffer::createAssign(std::optional<Expr> lhs, Expr rhs, uint32_t line, uint32_t collumn) -> Assign::ID {
		return this->internal->assigns.emplace_back(lhs, rhs, line, collumn);
	}

	auto SemaBuffer::getAssign(Assign::ID id) const -> const Assign& {
		return this->internal->assigns[id];
	}


	///////////////////////////////////
	// multi-assign

	auto SemaBuffer::createMultiAssign(
		evo::SmallVector<evo::Variant<Expr, TypeInfo::ID>>&& targets, Expr value, uint32_t line, uint32_t collumn
	) -> MultiAssign::ID {
		return this->internal->multi_assigns.emplace_back(std::move(targets), value, line, collumn);
	}

	auto SemaBuffer::getMultiAssign(MultiAssign::ID id) const -> const MultiAssign& {
		return this->internal->multi_assigns[id];
	}


	///////////////////////////////////
	// returns

	auto SemaBuffer::createReturn(
		std::optional<Expr> value, std::optional<Token::ID> targetLabel, uint32_t line, uint32_t collumn
	) -> Return::ID {
		return this->internal->returns.emplace_back(value, targetLabel, line, collumn);
	}

	auto SemaBuffer::getReturn(Return::ID id) const -> const Return& {
		return this->internal->returns[id];
	}


	///////////////////////////////////
	// errors

	auto SemaBuffer::createError(std::optional<Expr> value, uint32_t line, uint32_t collumn) -> Error::ID {
		return this->internal->errors.emplace_back(value, line, collumn);
	}

	auto SemaBuffer::getError(Error::ID id) const -> const Error& {
		return this->internal->errors[id];
	}


	///////////////////////////////////
	// unreachables

	auto SemaBuffer::createUnreachable(std::optional<Expr> message, uint32_t line, uint32_t collumn)
	-> Unreachable::ID {
		return this->internal->unreachables.emplace_back(message, line, collumn);
	}

	auto SemaBuffer::getUnreachable(Unreachable::ID id) const -> const Unreachable& {
		return this->internal->unreachables[id];
	}


	///////////////////////////////////
	// breaks

	auto SemaBuffer::createBreak(std::optional<Token::ID> label) -> Break::ID {
		return this->internal->breaks.emplace_back(label);
	}

	auto SemaBuffer::getBreak(Break::ID id) const -> const Break& {
		return this->internal->breaks[id];
	}


	///////////////////////////////////
	// continues

	auto SemaBuffer::createContinue(std::optional<Token::ID> label) -> Continue::ID {
		return this->internal->continues.emplace_back(label);
	}

	auto SemaBuffer::getContinue(Continue::ID id) const -> const Continue& {
		return this->internal->continues[id];
	}


	///////////////////////////////////
	// deletes

	auto SemaBuffer::createDelete(Expr expr, TypeInfo::ID exprTypeID, uint32_t line, uint32_t collumn) -> Delete::ID {
		return this->internal->deletes.emplace_back(expr, exprTypeID, line, collumn);
	}

	auto SemaBuffer::getDelete(Delete::ID id) const -> const Delete& {
		return this->internal->deletes[id];
	}

	///////////////////////////////////
	// block scopes

	auto SemaBuffer::createBlockScope(Token::ID openBrace, Token::ID closeBrace, StmtBlock&& block)
	-> BlockScope::ID {
		return this->internal->block_scopes.emplace_back(openBrace, closeBrace, std::move(block));
	}

	auto SemaBuffer::getBlockScope(BlockScope::ID id) const -> const BlockScope& {
		return this->internal->block_scopes[id];
	}

	auto SemaBuffer::getBlockScope(BlockScope::ID id) -> BlockScope& {
		return this->internal->block_scopes[id];
	}


	///////////////////////////////////
	// conditionals

	auto SemaBuffer::createConditional(
		Expr cond,
		Token::ID ifToken,
		std::optional<Token::ID> elseToken,
		Token::ID closeBraceToken,
		StmtBlock&& thenStmts,
		StmtBlock&& elseStmts
	) -> Conditional::ID {
		return this->internal->conds.emplace_back(
			cond, ifToken, elseToken, closeBraceToken, std::move(thenStmts), std::move(elseStmts)
		);
	}

	auto SemaBuffer::getConditional(Conditional::ID id) const -> const Conditional& {
		return this->internal->conds[id];
	}

	auto SemaBuffer::getConditional(Conditional::ID id) -> Conditional& {
		return this->internal->conds[id];
	}


	///////////////////////////////////
	// whiles

	auto SemaBuffer::createWhile(Expr cond, Token::ID whileToken, std::optional<Token::ID> label, StmtBlock&& block)
	-> While::ID {
		return this->internal->whiles.emplace_back(cond, whileToken, label, std::move(block));
	}

	auto SemaBuffer::getWhile(While::ID id) const -> const While& {
		return this->internal->whiles[id];
	}

	auto SemaBuffer::getWhile(While::ID id) -> While& {
		return this->internal->whiles[id];
	}


	///////////////////////////////////
	// fors

	auto SemaBuffer::createFor(
		evo::SmallVector<For::Iterable>&& iterables,
		Token::ID forToken,
		std::optional<Token::ID> label,
		bool hasIndex,
		evo::SmallVector<For::Param>&& params,
		StmtBlock&& block
	) -> For::ID {
		return this->internal->fors.emplace_back(
			std::move(iterables), forToken, label, hasIndex, std::move(params), std::move(block)
		);
	}

	auto SemaBuffer::getFor(For::ID id) const -> const For& {
		return this->internal->fors[id];
	}

	auto SemaBuffer::getFor(For::ID id) -> For& {
		return this->internal->fors[id];
	}


	///////////////////////////////////
	// for unrolls

	auto SemaBuffer::createForUnroll(
		Token::ID forToken, std::optional<Token::ID> label, evo::SmallVector<StmtBlock>&& stmtBlocks
	) -> ForUnroll::ID {
		return this->internal->for_unrolls.emplace_back(forToken, label, std::move(stmtBlocks));
	}

	auto SemaBuffer::getForUnroll(ForUnroll::ID id) const -> const ForUnroll& {
		return this->internal->for_unrolls[id];
	}

	auto SemaBuffer::getForUnroll(ForUnroll::ID id) -> ForUnroll& {
		return this->internal->for_unrolls[id];
	}


	///////////////////////////////////
	// switches

	auto SemaBuffer::createSwitch(
		Token::ID switchToken,
		TypeInfo::ID condTypeID,
		Expr cond,
		evo::SmallVector<Switch::Case>&& cases,
		Switch::Kind kind
	) -> Switch::ID {
		return this->internal->switches.emplace_back(switchToken, condTypeID, cond, std::move(cases), kind);
	}

	auto SemaBuffer::getSwitch(Switch::ID id) const -> const Switch& {
		return this->internal->switches[id];
	}

	auto SemaBuffer::getSwitch(Switch::ID id) -> Switch& {
		return this->internal->switches[id];
	}


	///////////////////////////////////
	// defers

	auto SemaBuffer::createDefer(Token::ID deferToken, bool isErrorDefer, StmtBlock&& block) -> Defer::ID {
		return this->internal->defers.emplace_back(deferToken, isErrorDefer, std::move(block));
	}

	auto SemaBuffer::getDefer(Defer::ID id) const -> const Defer& {
		return this->internal->defers[id];
	}

	auto SemaBuffer::getDefer(Defer::ID id) -> Defer& {
		return this->internal->defers[id];
	}


	///////////////////////////////////
	// lifetime start

	auto SemaBuffer::createLifetimeStart(evo::Variant<Expr, OpDeleteThisAccessor> target, TypeInfo::ID typeID)
	-> LifetimeStart::ID {
		return this->internal->lifetime_starts.emplace_back(target, typeID);
	}

	auto SemaBuffer::getLifetimeStart(LifetimeStart::ID id) const -> const LifetimeStart& {
		return this->internal->lifetime_starts[id];
	}


	///////////////////////////////////
	// lifetime end

	auto SemaBuffer::createLifetimeEnd(evo::Variant<Expr, OpDeleteThisAccessor> target, TypeInfo::ID typeID)
		-> LifetimeEnd::ID {
		return this->internal->lifetime_ends.emplace_back(target, typeID);
	}

	auto SemaBuffer::getLifetimeEnd(LifetimeEnd::ID id) const -> const LifetimeEnd& {
		return this->internal->lifetime_ends[id];
	}


	///////////////////////////////////
	// unused expr

	auto SemaBuffer::createUnusedExpr(Expr expr) -> UnusedExpr::ID {
		return this->internal->unused_exprs.emplace_back(expr);
	}

	auto SemaBuffer::getUnusedExpr(UnusedExpr::ID id) const -> const UnusedExpr& {
		return this->internal->unused_exprs[id];
	}


	///////////////////////////////////
	// copies

	auto SemaBuffer::createCopy(Expr expr, TypeInfo::ID exprTypeID, bool isInitialization) -> Copy::ID {
		return this->internal->copies.emplace_back(expr, exprTypeID, isInitialization);
	}

	auto SemaBuffer::getCopy(Copy::ID id) const -> const Copy& {
		return this->internal->copies[id];
	}

	auto SemaBuffer::getCopy(Copy::ID id) -> Copy& {
		return this->internal->copies[id];
	}


	///////////////////////////////////
	// moves

	auto SemaBuffer::createMove(Expr expr, TypeInfo::ID exprTypeID, bool isInitialization) -> Move::ID {
		return this->internal->moves.emplace_back(expr, exprTypeID, isInitialization);
	}

	auto SemaBuffer::getMove(Move::ID id) const -> const Move& {
		return this->internal->moves[id];
	}

	auto SemaBuffer::getMove(Move::ID id) -> Move& {
		return this->internal->moves[id];
	}


	///////////////////////////////////
	// forwards

	auto SemaBuffer::createForward(Expr expr, TypeInfo::ID exprTypeID, bool isInitialization) -> Forward::ID {
		return this->internal->forwards.emplace_back(expr, exprTypeID, isInitialization);
	}

	auto SemaBuffer::getForward(Forward::ID id) const -> const Forward& {
		return this->internal->forwards[id];
	}

	auto SemaBuffer::getForward(Forward::ID id) -> Forward& {
		return this->internal->forwards[id];
	}


	///////////////////////////////////
	// func ptrs

	auto SemaBuffer::createFuncPtr(FuncID targetFuncID) -> FuncPtr::ID {
		return this->internal->func_ptrs.emplace_back(targetFuncID);
	}

	auto SemaBuffer::getFuncPtr(FuncPtr::ID id) const -> const FuncPtr& {
		return this->internal->func_ptrs[id];
	}



	///////////////////////////////////
	// address ofs

	auto SemaBuffer::createAddrOf(Expr expr) -> AddrOf::ID {
		return AddrOf::ID(this->internal->misc_exprs.emplace_back(expr));
	}

	auto SemaBuffer::getAddrOf(AddrOf::ID id) const -> const Expr& {
		return this->internal->misc_exprs[id.get()];
	}


	///////////////////////////////////
	// optional null check

	auto SemaBuffer::createConversionToOptional(Expr expr, TypeInfo::ID targetTypeID)
		-> ConversionToOptional::ID {
		return this->internal->conversion_to_optionals.emplace_back(expr, targetTypeID);
	}

	auto SemaBuffer::getConversionToOptional(ConversionToOptional::ID id) const -> const ConversionToOptional& {
		return this->internal->conversion_to_optionals[id];
	}


	///////////////////////////////////
	// optional null check

	auto SemaBuffer::createOptionalNullCheck(Expr expr, TypeInfo::ID targetTypeID, bool equal)
		-> OptionalNullCheck::ID {
		return this->internal->optional_null_checks.emplace_back(expr, targetTypeID, equal);
	}

	auto SemaBuffer::getOptionalNullCheck(OptionalNullCheck::ID id) const -> const OptionalNullCheck& {
		return this->internal->optional_null_checks[id];
	}


	///////////////////////////////////
	// optional extract

	auto SemaBuffer::createOptionalExtract(Expr expr, TypeInfo::ID targetTypeID) -> OptionalExtract::ID {
		return this->internal->optional_extracts.emplace_back(expr, targetTypeID);
	}

	auto SemaBuffer::getOptionalExtract(OptionalExtract::ID id) const -> const OptionalExtract& {
		return this->internal->optional_extracts[id];
	}


	///////////////////////////////////
	// dereferences

	auto SemaBuffer::createDeref(Expr expr, TypeInfo::ID targetTypeID) -> Deref::ID {
		return this->internal->derefs.emplace_back(expr, targetTypeID);
	}

	auto SemaBuffer::getDeref(Deref::ID id) const -> const Deref& {
		return this->internal->derefs[id];
	}


	///////////////////////////////////
	// unwraps

	auto SemaBuffer::createUnwrap(Expr expr, TypeInfo::ID targetTypeID, bool isComptime) -> Unwrap::ID {
		return this->internal->unwraps.emplace_back(expr, targetTypeID, isComptime);
	}

	auto SemaBuffer::getUnwrap(Unwrap::ID id) const -> const Unwrap& {
		return this->internal->unwraps[id];
	}


	///////////////////////////////////
	// accessors

	auto SemaBuffer::createAccessor(Expr target, TypeInfo::ID targetTypeID, uint32_t memberABIIndex)
	-> Accessor::ID {
		return this->internal->accessors.emplace_back(target, targetTypeID, memberABIIndex);
	}

	auto SemaBuffer::getAccessor(Accessor::ID id) const -> const Accessor& {
		return this->internal->accessors[id];
	}


	///////////////////////////////////
	// union accessors

	auto SemaBuffer::createUnionAccessor(Expr target, TypeInfo::ID targetTypeID, uint32_t fieldIndex)
	-> UnionAccessor::ID {
		return this->internal->union_accessors.emplace_back(target, targetTypeID, fieldIndex);
	}

	auto SemaBuffer::getUnionAccessor(UnionAccessor::ID id) const -> const UnionAccessor& {
		return this->internal->union_accessors[id];
	}


	///////////////////////////////////
	// logical and

	auto SemaBuffer::createLogicalAnd(Expr lhs, Expr rhs) -> LogicalAnd::ID {
		return this->internal->logical_ands.emplace_back(lhs, rhs);
	}

	auto SemaBuffer::getLogicalAnd(LogicalAnd::ID id) const -> const LogicalAnd& {
		return this->internal->logical_ands[id];
	}


	///////////////////////////////////
	// logical or

	auto SemaBuffer::createLogicalOr(Expr lhs, Expr rhs) -> LogicalOr::ID {
		return this->internal->logical_ors.emplace_back(lhs, rhs);
	}

	auto SemaBuffer::getLogicalOr(LogicalOr::ID id) const -> const LogicalOr& {
		return this->internal->logical_ors[id];
	}


	///////////////////////////////////
	// try/else expr

	auto SemaBuffer::createTryElseExpr(
		Expr attempt,
		Expr except,
		evo::SmallVector<ExceptParamID>&& exceptParams,
		uint32_t line,
		uint32_t collumn
	) -> TryElseExpr::ID {
		return this->internal->try_else_exprs.emplace_back(attempt, except, std::move(exceptParams), line, collumn);
	}

	auto SemaBuffer::getTryElseExpr(TryElseExpr::ID id) const -> const TryElseExpr& {
		return this->internal->try_else_exprs[id];
	}


	///////////////////////////////////
	// try/else interface expr

	auto SemaBuffer::createTryElseInterfaceExpr(
		Expr attempt,
		Expr except,
		evo::SmallVector<ExceptParamID>&& exceptParams,
		uint32_t line,
		uint32_t collumn
	) -> TryElseInterfaceExpr::ID {
		return this->internal->try_else_interface_exprs.emplace_back(attempt, except, std::move(exceptParams), line, collumn);
	}

	auto SemaBuffer::getTryElseInterfaceExpr(TryElseInterfaceExpr::ID id) const
	-> const TryElseInterfaceExpr& {
		return this->internal->try_else_interface_exprs[id];
	}


	///////////////////////////////////
	// block expr

	auto SemaBuffer::createBlockExpr(
		Token::ID label, evo::SmallVector<BlockExpr::Output>&& outputs, StmtBlock&& block
	) -> BlockExpr::ID {
		return this->internal->block_exprs.emplace_back(label, std::move(outputs), std::move(block));
	}

	auto SemaBuffer::getBlockExpr(BlockExpr::ID id) const -> const BlockExpr& {
		return this->internal->block_exprs[id];
	}

	auto SemaBuffer::getBlockExpr(BlockExpr::ID id) -> BlockExpr& {
		return this->internal->block_exprs[id];
	}


	///////////////////////////////////
	// fake term info

	auto SemaBuffer::createFakeTermInfo(
		FakeTermInfo::ValueCategory valueCategory,
		FakeTermInfo::ValueState valueState,
		TypeInfo::ID typeID,
		Expr expr,
		bool isComptime
	) -> FakeTermInfo::ID {
		return this->internal->fake_term_infos.emplace_back(valueCategory, valueState, typeID, expr, isComptime);
	}

	auto SemaBuffer::getFakeTermInfo(FakeTermInfo::ID id) const -> const FakeTermInfo& {
		return this->internal->fake_term_infos[id];
	}


	///////////////////////////////////
	// make interface ptr

	auto SemaBuffer::createMakeInterfacePtr(Expr expr, BaseType::Interface::ID interfaceID, TypeInfo::ID implTypeID)
	-> MakeInterfacePtr::ID {
		return this->internal->make_interface_ptrs.emplace_back(expr, interfaceID, implTypeID);
	}

	auto SemaBuffer::getMakeInterfacePtr(MakeInterfacePtr::ID id) const -> const MakeInterfacePtr& {
		return this->internal->make_interface_ptrs[id];
	}


	///////////////////////////////////
	// interface ptr extract this

	auto SemaBuffer::createInterfacePtrExtractThis(Expr expr) -> InterfacePtrExtractThis::ID {
		return this->internal->interface_ptr_extract_thises.emplace_back(expr);
	}

	auto SemaBuffer::getInterfacePtrExtractThis(InterfacePtrExtractThis::ID id) const
	-> const InterfacePtrExtractThis& {
		return this->internal->interface_ptr_extract_thises[id];
	}


	///////////////////////////////////
	// interface call

	auto SemaBuffer::createInterfaceCall(
		Expr value,
		BaseType::Function::ID funcTypeID,
		BaseType::Interface::ID interfaceID,
		uint32_t vtableFuncIndex,
		evo::SmallVector<Expr>&& args
	) -> InterfaceCall::ID {
		return this->internal->interface_calls.emplace_back(value, funcTypeID, interfaceID, vtableFuncIndex, std::move(args));
	}

	auto SemaBuffer::getInterfaceCall(InterfaceCall::ID id) const -> const InterfaceCall& {
		return this->internal->interface_calls[id];
	}


	///////////////////////////////////
	// indexer

	auto SemaBuffer::createIndexer(
		Expr target, TypeInfo::ID targetTypeID, evo::SmallVector<Expr>&& indices
	) -> Indexer::ID {
		return this->internal->indexers.emplace_back(target, targetTypeID, std::move(indices));
	}

	auto SemaBuffer::getIndexer(Indexer::ID id) const -> const Indexer& {
		return this->internal->indexers[id];
	}


	///////////////////////////////////
	// default init

	auto SemaBuffer::createDefaultNew(TypeInfo::ID targetTypeID, bool isInitialization) -> DefaultNew::ID {
		return this->internal->default_news.emplace_back(targetTypeID, isInitialization);
	}

	auto SemaBuffer::getDefaultNew(DefaultNew::ID id) const -> const DefaultNew& {
		return this->internal->default_news[id];
	}

	auto SemaBuffer::getDefaultNew(DefaultNew::ID id) -> DefaultNew& {
		return this->internal->default_news[id];
	}


	///////////////////////////////////
	// init array ref

	auto SemaBuffer::createInitArrayRef(
		Expr expr,
		BaseType::ArrayRef::ID targetTypeID,
		evo::SmallVector<evo::Variant<uint64_t, Expr>>&& dimensions
	) -> InitArrayRef::ID {
		return this->internal->init_array_ref.emplace_back(expr, targetTypeID, std::move(dimensions));
	}

	auto SemaBuffer::getInitArrayRef(InitArrayRef::ID id) const -> const InitArrayRef& {
		return this->internal->init_array_ref[id];
	}


	///////////////////////////////////
	// array ref indexer

	auto SemaBuffer::createArrayRefIndexer(
		Expr target, BaseType::ArrayRef::ID targetTypeID, evo::SmallVector<Expr>&& indices
	) -> ArrayRefIndexer::ID {
		return this->internal->array_ref_indexers.emplace_back(target, targetTypeID, std::move(indices));
	}

	auto SemaBuffer::getArrayRefIndexer(ArrayRefIndexer::ID id) const -> const ArrayRefIndexer& {
		return this->internal->array_ref_indexers[id];
	}


	///////////////////////////////////
	// array ref size

	auto SemaBuffer::createArrayRefSize(Expr target, BaseType::ArrayRef::ID targetTypeID)
		-> ArrayRefSize::ID {
		return this->internal->array_ref_size.emplace_back(target, targetTypeID);
	}

	auto SemaBuffer::getArrayRefSize(ArrayRefSize::ID id) const -> const ArrayRefSize& {
		return this->internal->array_ref_size[id];
	}


	///////////////////////////////////
	// array ref dimensions

	auto SemaBuffer::createArrayRefDimensions(Expr target, BaseType::ArrayRef::ID targetTypeID)
	-> ArrayRefDimensions::ID {
		return this->internal->array_ref_dimensions.emplace_back(target, targetTypeID);
	}

	auto SemaBuffer::getArrayRefDimensions(ArrayRefDimensions::ID id) const -> const ArrayRefDimensions& {
		return this->internal->array_ref_dimensions[id];
	}


	///////////////////////////////////
	// array ref data

	auto SemaBuffer::createArrayRefData(Expr target, BaseType::ArrayRef::ID targetTypeID) -> ArrayRefData::ID {
		return this->internal->array_ref_data.emplace_back(target, targetTypeID);
	}

	auto SemaBuffer::getArrayRefData(ArrayRefData::ID id) const -> const ArrayRefData& {
		return this->internal->array_ref_data[id];
	}


	///////////////////////////////////
	// union designated init new

	auto SemaBuffer::createUnionDesignatedInitNew(
		Expr value, BaseType::Union::ID unionTypeID, uint32_t fieldIndex
	) -> UnionDesignatedInitNew::ID {
		return this->internal->union_designated_init_news.emplace_back(value, unionTypeID, fieldIndex);
	}

	auto SemaBuffer::getUnionDesignatedInitNew(UnionDesignatedInitNew::ID id) const -> const UnionDesignatedInitNew& {
		return this->internal->union_designated_init_news[id];
	}


	///////////////////////////////////
	// union tag cmp

	auto SemaBuffer::createUnionTagCmp(
		Expr value, BaseType::Union::ID unionTypeID, uint32_t fieldIndex, bool isEqual
	) -> UnionTagCmp::ID {
		return this->internal->union_tag_cmps.emplace_back(value, unionTypeID, fieldIndex, isEqual);
	}

	auto SemaBuffer::getUnionTagCmp(UnionTagCmp::ID id) const -> const UnionTagCmp& {
		return this->internal->union_tag_cmps[id];
	}


	///////////////////////////////////
	// same type cmp

	auto SemaBuffer::createSameTypeCmp(TypeInfo::ID typeID, Expr lhs, Expr rhs, bool isEqual) -> SameTypeCmp::ID {
		return this->internal->same_type_cmps.emplace_back(typeID, lhs, rhs, isEqual);
	}

	auto SemaBuffer::getSameTypeCmp(SameTypeCmp::ID id) const -> const SameTypeCmp& {
		return this->internal->same_type_cmps[id];
	}


	///////////////////////////////////
	// template intrinsic instantiations

	auto SemaBuffer::createTemplateIntrinsicFuncInstantiation(
		TemplateIntrinsicFunc::Kind kind,
		evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>&& templateArgs
	) -> TemplateIntrinsicFuncInstantiation::ID {
		return this->internal->templated_intrinsic_func_instantiations.emplace_back(kind, std::move(templateArgs));
	}

	auto SemaBuffer::getTemplateIntrinsicFuncInstantiation(
		TemplateIntrinsicFuncInstantiation::ID id
	) const -> const TemplateIntrinsicFuncInstantiation& {
		return this->internal->templated_intrinsic_func_instantiations[id];
	}


	///////////////////////////////////
	// ints

	auto SemaBuffer::createIntValue(core::GenericInt&& integer, std::optional<BaseType::ID> type_info_id)
	-> IntValue::ID {
		return this->internal->int_values.emplace_back(std::move(integer), type_info_id);
	}

	auto SemaBuffer::getIntValue(IntValue::ID id) const -> const IntValue& {
		return this->internal->int_values[id];
	}

	auto SemaBuffer::getIntValue(IntValue::ID id) -> IntValue& {
		return this->internal->int_values[id];
	}


	///////////////////////////////////
	// floats

	auto SemaBuffer::createFloatValue(
		core::GenericFloat&& floating_point, std::optional<BaseType::ID> type_info_id
	) -> FloatValue::ID {
		return this->internal->float_values.emplace_back(std::move(floating_point), type_info_id);
	}

	auto SemaBuffer::getFloatValue(FloatValue::ID id) const -> const FloatValue& {
		return this->internal->float_values[id];
	}

	auto SemaBuffer::getFloatValue(FloatValue::ID id) -> FloatValue& {
		return this->internal->float_values[id];
	}


	///////////////////////////////////
	// bools

	auto SemaBuffer::createBoolValue(bool boolean, bool is_bool_32) -> BoolValue::ID {
		return this->internal->bool_values.emplace_back(boolean, is_bool_32);
	}

	auto SemaBuffer::getBoolValue(BoolValue::ID id) const -> const BoolValue& {
		return this->internal->bool_values[id];
	}

	auto SemaBuffer::getBoolValue(BoolValue::ID id) -> BoolValue& {
		return this->internal->bool_values[id];
	}


	///////////////////////////////////
	// strings

	auto SemaBuffer::createStringValue(std::string&& value) -> StringValue::ID {
		return this->internal->string_values.emplace_back(std::move(value));
	}

	auto SemaBuffer::createStringValue(const std::string& value) -> StringValue::ID {
		return this->internal->string_values.emplace_back(value);
	}

	auto SemaBuffer::getStringValue(StringValue::ID id) const -> const StringValue& {
		return this->internal->string_values[id];
	}


	///////////////////////////////////
	// aggregates

	auto SemaBuffer::createAggregateValue(evo::SmallVector<Expr>&& values, BaseType::ID typeID)
	-> AggregateValue::ID {
		return this->internal->aggregate_values.emplace_back(std::move(values), typeID);
	}

	auto SemaBuffer::createAggregateValue(
		const evo::SmallVector<Expr>& values, BaseType::ID typeID
	) -> AggregateValue::ID {
		return this->internal->aggregate_values.emplace_back(values, typeID);
	}

	auto SemaBuffer::getAggregateValue(AggregateValue::ID id) const -> const AggregateValue& {
		return this->internal->aggregate_values[id];
	}


	///////////////////////////////////
	// chars

	auto SemaBuffer::createCharValue(char character) -> CharValue::ID {
		return this->internal->char_values.emplace_back(character);
	}

	auto SemaBuffer::getCharValue(CharValue::ID id) const -> const CharValue& {
		return this->internal->char_values[id];
	}


	///////////////////////////////////
	// null

	auto SemaBuffer::createNull(Token::ID null_token_id) -> Null::ID {
		return Null::ID(this->internal->misc_tokens.emplace_back(null_token_id));
	}

	auto SemaBuffer::getNull(Uninit::ID id) const -> Token::ID {
		return this->internal->misc_tokens[id.get()];
	}


	///////////////////////////////////
	// uninit

	auto SemaBuffer::createUninit(Token::ID uninit_token_id) -> Uninit::ID {
		return Uninit::ID(this->internal->misc_tokens.emplace_back(uninit_token_id));
	}

	auto SemaBuffer::getUninit(Uninit::ID id) const -> Token::ID {
		return this->internal->misc_tokens[id.get()];
	}


	///////////////////////////////////
	// zeroinit

	auto SemaBuffer::createZeroinit(Token::ID zeroinit_token_id) -> Zeroinit::ID {
		return Zeroinit::ID(this->internal->misc_tokens.emplace_back(zeroinit_token_id));
	}

	auto SemaBuffer::getZeroinit(Zeroinit::ID id) const -> Token::ID {
		return this->internal->misc_tokens[id.get()];
	}





}