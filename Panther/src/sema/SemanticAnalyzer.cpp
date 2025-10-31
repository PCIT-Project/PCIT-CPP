////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./SemanticAnalyzer.h"

#include <queue>

#include "../symbol_proc/SymbolProcBuilder.h"
#include "./attributes.h"
#include "./ConstexprIntrinsicEvaluator.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

namespace pcit::panther{

	using Instruction = SymbolProc::Instruction;



	//////////////////////////////////////////////////////////////////////
	// semantic analyzer


	auto SemanticAnalyzer::analyze() -> void {
		if(this->symbol_proc.passedOnByWhenCond()){ return; }

		
		{
			const auto lock = std::scoped_lock(this->symbol_proc.waiting_for_lock);
			this->symbol_proc.setStatusWorking();
		}


		EVO_DEFER([&](){
			evo::debugAssert(
				this->symbol_proc.status != SymbolProc::Status::WORKING, "Symbol Proc being worked on should be false"
			);
		});

		while(this->symbol_proc.isAtEnd() == false){
			#if defined(PCIT_CONFIG_DEBUG)
				{
					// separate var to prevent debugging race conditons
					const SymbolProc::Status current_status = this->symbol_proc.status.load();
					evo::debugAssert(
						current_status == SymbolProc::Status::WORKING,
						"Symbol Proc should only be working if the status is `WORKING`"
					);
				}
			#endif

			switch(this->analyze_instr(this->symbol_proc.getInstruction())){
				case Result::SUCCESS: {
					this->symbol_proc.nextInstruction();
				} break;

				case Result::FINISHED_EARLY: {
					this->symbol_proc.setInstructionIndex(
						SymbolProc::InstructionIndex(uint32_t(this->symbol_proc.instructions.size()))
					);
				} break;


				case Result::ERROR: {
					this->context.symbol_proc_manager.symbol_proc_done();

					if(this->symbol_proc.extra_info.is<SymbolProc::StructInfo>()){
						SymbolProc::StructInfo& struct_info = this->symbol_proc.extra_info.as<SymbolProc::StructInfo>();
						if(struct_info.instantiation != nullptr){ struct_info.instantiation->errored = true; }

					}else if(this->symbol_proc.extra_info.is<SymbolProc::FuncInfo>()){
						SymbolProc::FuncInfo& func_info = this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>();
						if(func_info.instantiation != nullptr){
							func_info.instantiation->errored_reason = 
								sema::TemplatedFunc::Instantiation::ErroredReasonErroredAfterDecl();
						}
					}

					this->symbol_proc.setStatusErrored();
					return;
				} break;

				case Result::RECOVERABLE_ERROR: {
					evo::unimplemented("recoverable errors");
					// this->symbol_proc.status = SymbolProc::Status::ERRORED;
					// if(this->symbol_proc.extra_info.is<SymbolProc::StructInfo>()){
					// 	SymbolProc::StructInfo& struct_info = this->symbol_proc.extra_info.as<SymbolProc::StructInfo>();
					// 	if(struct_info.instantiation != nullptr){ struct_info.instantiation->errored = true; }
					// }
					// this->symbol_proc.nextInstruction();
				} break;

				case Result::NEED_TO_WAIT: {
					const auto lock = std::scoped_lock(this->symbol_proc.waiting_for_lock);

					if(this->symbol_proc.waiting_for.empty()){ continue; } // prevent race condition
					
					this->symbol_proc.setStatusWaiting();
					return;
				} break;

				case Result::NEED_TO_WAIT_BEFORE_NEXT_INSTR: {
					const auto lock = std::scoped_lock(this->symbol_proc.waiting_for_lock);

					if(this->symbol_proc.waiting_for.empty()){ continue; } // prevent race condition

					this->symbol_proc.nextInstruction();
					this->symbol_proc.setStatusWaiting();
					return;
				} break;

				case Result::SUSPEND: {
					const auto lock = std::scoped_lock(this->symbol_proc.waiting_for_lock);

					this->symbol_proc.nextInstruction();
					
					if(this->symbol_proc.setStatusSuspended()){
						if(this->scope.getCurrentObjectScope().is<sema::Func::ID>()){
							sema::Func& sema_func = this->context.sema_buffer.funcs[
								this->scope.getCurrentObjectScope().as<sema::Func::ID>()
							];

							sema_func.status = sema::Func::Status::SUSPENDED;
						}

						this->context.symbol_proc_manager.symbol_proc_suspended();
						return;
					}
				} break;
			}
		}

		this->context.symbol_proc_manager.symbol_proc_done();
		this->symbol_proc.setStatusDone();
	}


	auto SemanticAnalyzer::analyze_instr(const Instruction& instr) -> Result {
		switch(instr.kind()){
			case Instruction::Kind::SUSPEND_SYMBOL_PROC:
				return this->instr_suspend_symbol_proc();

			case Instruction::Kind::NON_LOCAL_VAR_DECL:
				return this->instr_non_local_var_decl(this->context.symbol_proc_manager.getNonLocalVarDecl(instr));

			case Instruction::Kind::NON_LOCAL_VAR_DEF:
				return this->instr_non_local_var_def(this->context.symbol_proc_manager.getNonLocalVarDef(instr));

			case Instruction::Kind::NON_LOCAL_VAR_DECL_DEF:
				return this->instr_non_local_var_decl_def(
					this->context.symbol_proc_manager.getNonLocalVarDeclDef(instr)
				);

			case Instruction::Kind::WHEN_COND:
				return this->instr_when_cond(this->context.symbol_proc_manager.getWhenCond(instr));

			case Instruction::Kind::ALIAS_DECL:
				return this->instr_alias_decl(this->context.symbol_proc_manager.getAliasDecl(instr));

			case Instruction::Kind::ALIAS_DEF:
				return this->instr_alias_def(this->context.symbol_proc_manager.getAliasDef(instr));

			case Instruction::Kind::STRUCT_DECL_INSTANTIATION:
				return this->instr_struct_decl<true>(
					this->context.symbol_proc_manager.getStructDeclInstatiation(instr)
				);

			case Instruction::Kind::STRUCT_DECL:
				return this->instr_struct_decl<false>(this->context.symbol_proc_manager.getStructDecl(instr));

			case Instruction::Kind::STRUCT_DEF:
				return this->instr_struct_def();

			case Instruction::Kind::STRUCT_CREATED_SPECIAL_MEMBERS_PIR_IF_NEEDED:
				return this->instr_struct_created_sepcial_members_pir_if_needed();

			case Instruction::Kind::TEMPLATE_STRUCT:
				return this->instr_template_struct(this->context.symbol_proc_manager.getTemplateStruct(instr));

			case Instruction::Kind::UNION_DECL:
				return this->instr_union_decl(this->context.symbol_proc_manager.getUnionDecl(instr));

			case Instruction::Kind::UNION_ADD_FIELDS:
				return this->instr_union_add_fields(this->context.symbol_proc_manager.getUnionAddFields(instr));

			case Instruction::Kind::UNION_DEF:
				return this->instr_union_def();

			case Instruction::Kind::ENUM_DECL:
				return this->instr_enum_decl(this->context.symbol_proc_manager.getEnumDecl(instr));

			case Instruction::Kind::ENUM_ADD_ENUMERATORS:
				return this->instr_enum_add_enumerators(this->context.symbol_proc_manager.getEnumAddEnumerators(instr));

			case Instruction::Kind::ENUM_DEF:
				return this->instr_enum_def();

			case Instruction::Kind::FUNC_DECL_EXTRACT_DEDUCERS_IF_NEEDED:
				return this->instr_func_decl_extract_deducers_if_needed(
					this->context.symbol_proc_manager.getFuncDeclExtractDeducersIfNeeded(instr)
				);

			case Instruction::Kind::FUNC_DECL_INSTANTIATION:
				return this->instr_func_decl<true>(this->context.symbol_proc_manager.getFuncDeclInstantiation(instr));

			case Instruction::Kind::FUNC_DECL:
				return this->instr_func_decl<false>(this->context.symbol_proc_manager.getFuncDecl(instr));

			case Instruction::Kind::FUNC_PRE_BODY:
				return this->instr_func_pre_body(this->context.symbol_proc_manager.getFuncPreBody(instr));

			case Instruction::Kind::FUNC_DEF:
				return this->instr_func_def(this->context.symbol_proc_manager.getFuncDef(instr));

			case Instruction::Kind::FUNC_PREPARE_CONSTEXPR_PIR_IF_NEEDED:
				return this->instr_func_prepare_constexpr_pir_if_needed(
					this->context.symbol_proc_manager.getFuncPrepareConstexprPIRIfNeeded(instr)
				);

			case Instruction::Kind::FUNC_CONSTEXPR_PIR_READY_IF_NEEDED:
				return this->instr_func_constexpr_pir_ready_if_needed();

			case Instruction::Kind::TEMPLATE_FUNC_BEGIN:
				return this->instr_template_func_begin(this->context.symbol_proc_manager.getTemplateFuncBegin(instr));

			case Instruction::Kind::TEMPLATE_FUNC_CHECK_PARAM_IS_INTERFACE:
				return this->instr_template_func_check_param_is_interface(
					this->context.symbol_proc_manager.getTemplateFuncCheckParamIsInterface(instr)
				);

			case Instruction::Kind::TEMPLATE_FUNC_SET_PARAM_IS_DEDUCER:
				return this->instr_template_set_param_is_deducer(
					this->context.symbol_proc_manager.getTemplateFuncSetParamIsDeducer(instr)
				);

			case Instruction::Kind::TEMPLATE_FUNC_END:
				return this->instr_template_func_end(this->context.symbol_proc_manager.getTemplateFuncEnd(instr));

			case Instruction::Kind::DELETED_SPECIAL_METHOD:
				return this->instr_deleted_special_method(
					this->context.symbol_proc_manager.getDeletedSpecialMethod(instr)
				);

			case Instruction::Kind::INTERFACE_DECL:
				return this->instr_interface_decl(this->context.symbol_proc_manager.getInterfaceDecl(instr));

			case Instruction::Kind::INTERFACE_DEF:
				return this->instr_interface_def();

			case Instruction::Kind::INTERFACE_FUNC_DEF:
				return this->instr_interface_func_def(this->context.symbol_proc_manager.getInterfaceFuncDef(instr));

			case Instruction::Kind::INTERFACE_IMPL_DECL:
				return this->instr_interface_impl_decl(this->context.symbol_proc_manager.getInterfaceImplDecl(instr));

			case Instruction::Kind::INTERFACE_IMPL_METHOD_LOOKUP:
				return this->instr_interface_impl_method_lookup(
					this->context.symbol_proc_manager.getInterfaceImplMethodLookup(instr)
				);

			case Instruction::Kind::INTERFACE_IMPL_DEF:
				return this->instr_interface_impl_def(this->context.symbol_proc_manager.getInterfaceImplDef(instr));

			case Instruction::Kind::INTERFACE_IMPL_CONSTEXPR_PIR:
				return this->instr_interface_impl_constexpr_pir();

			case Instruction::Kind::LOCAL_VAR:
				return this->instr_local_var(this->context.symbol_proc_manager.getLocalVar(instr));

			case Instruction::Kind::LOCAL_ALIAS:
				return this->instr_local_alias(this->context.symbol_proc_manager.getLocalAlias(instr));

			case Instruction::Kind::RETURN:
				return this->instr_return(this->context.symbol_proc_manager.getReturn(instr));

			case Instruction::Kind::LABELED_RETURN:
				return this->instr_labeled_return(this->context.symbol_proc_manager.getLabeledReturn(instr));

			case Instruction::Kind::ERROR:
				return this->instr_error(this->context.symbol_proc_manager.getError(instr));

			case Instruction::Kind::UNREACHABLE:
				return this->instr_unreachable(this->context.symbol_proc_manager.getUnreachable(instr));

			case Instruction::Kind::BREAK:
				return this->instr_break(this->context.symbol_proc_manager.getBreak(instr));

			case Instruction::Kind::CONTINUE:
				return this->instr_continue(this->context.symbol_proc_manager.getContinue(instr));

			case Instruction::Kind::DELETE:
				return this->instr_delete(this->context.symbol_proc_manager.getDelete(instr));

			case Instruction::Kind::BEGIN_COND:
				return this->instr_begin_cond(this->context.symbol_proc_manager.getBeginCond(instr));

			case Instruction::Kind::COND_NO_ELSE:
				return this->instr_cond_no_else();

			case Instruction::Kind::COND_ELSE:
				return this->instr_cond_else();

			case Instruction::Kind::COND_ELSE_IF:
				return this->instr_cond_else_if();

			case Instruction::Kind::END_COND:
				return this->instr_end_cond();

			case Instruction::Kind::END_COND_SET:
				return this->instr_end_cond_set(this->context.symbol_proc_manager.getEndCondSet(instr));

			case Instruction::Kind::BEGIN_LOCAL_WHEN_COND:
				return this->instr_begin_local_when_cond(
					this->context.symbol_proc_manager.getBeginLocalWhenCond(instr)
				);

			case Instruction::Kind::END_LOCAL_WHEN_COND:
				return this->instr_end_local_when_cond(this->context.symbol_proc_manager.getEndLocalWhenCond(instr));

			case Instruction::Kind::BEGIN_WHILE:
				return this->instr_begin_while(this->context.symbol_proc_manager.getBeginWhile(instr));

			case Instruction::Kind::END_WHILE:
				return this->instr_end_while(this->context.symbol_proc_manager.getEndWhile(instr));

			case Instruction::Kind::BEGIN_DEFER:
				return this->instr_begin_defer(this->context.symbol_proc_manager.getBeginDefer(instr));

			case Instruction::Kind::END_DEFER:
				return this->instr_end_defer(this->context.symbol_proc_manager.getEndDefer(instr));

			case Instruction::Kind::BEGIN_STMT_BLOCK:
				return this->instr_begin_stmt_block(this->context.symbol_proc_manager.getBeginStmtBlock(instr));

			case Instruction::Kind::END_STMT_BLOCK:
				return this->instr_end_stmt_block(this->context.symbol_proc_manager.getEndStmtBlock(instr));

			case Instruction::Kind::FUNC_CALL:
				return this->instr_func_call(this->context.symbol_proc_manager.getFuncCall(instr));

			case Instruction::Kind::ASSIGNMENT:
				return this->instr_assignment(this->context.symbol_proc_manager.getAssignment(instr));

			case Instruction::Kind::ASSIGNMENT_NEW:
				return this->instr_assignment_new(this->context.symbol_proc_manager.getAssignmentNew(instr));

			case Instruction::Kind::ASSIGNMENT_COPY:
				return this->instr_assignment_copy(this->context.symbol_proc_manager.getAssignmentCopy(instr));

			case Instruction::Kind::ASSIGNMENT_MOVE:
				return this->instr_assignment_move(this->context.symbol_proc_manager.getAssignmentMove(instr));

			case Instruction::Kind::ASSIGNMENT_FORWARD:
				return this->instr_assignment_forward(this->context.symbol_proc_manager.getAssignmentForward(instr));

			case Instruction::Kind::MULTI_ASSIGN:
				return this->instr_multi_assign(this->context.symbol_proc_manager.getMultiAssign(instr));

			case Instruction::Kind::DISCARDING_ASSIGNMENT:
				return this->instr_discarding_assignment(
					this->context.symbol_proc_manager.getDiscardingAssignment(instr)
				);

			case Instruction::Kind::TRY_ELSE_BEGIN:
				return this->instr_try_else_begin(this->context.symbol_proc_manager.getTryElseBegin(instr));

			case Instruction::Kind::TRY_ELSE_END:
				return this->instr_try_else_end();

			case Instruction::Kind::TYPE_TO_TERM:
				return this->instr_type_to_term(this->context.symbol_proc_manager.getTypeToTerm(instr));

			case Instruction::Kind::REQUIRE_THIS_DEF:
				return this->instr_require_this_def();

			case Instruction::Kind::WAIT_ON_SUB_SYMBOL_PROC_DEF:
				return this->instr_wait_on_sub_symbol_proc_def(
					this->context.symbol_proc_manager.getWaitOnSubSymbolProcDef(instr)
				);

			case Instruction::Kind::FUNC_CALL_EXPR_CONSTEXPR_ERRORS:
				return this->instr_func_call_expr<true, true>(
					this->context.symbol_proc_manager.getFuncCallExprConstexprErrors(instr)
				);

			case Instruction::Kind::FUNC_CALL_EXPR_CONSTEXPR:
				return this->instr_func_call_expr<true, false>(
					this->context.symbol_proc_manager.getFuncCallExprConstexpr(instr)
				);

			case Instruction::Kind::FUNC_CALL_EXPR_ERRORS:
				return this->instr_func_call_expr<false, true>(
					this->context.symbol_proc_manager.getFuncCallExprErrors(instr)
				);

			case Instruction::Kind::FUNC_CALL_EXPR:
				return this->instr_func_call_expr<false, false>(
					this->context.symbol_proc_manager.getFuncCallExpr(instr)
				);

			case Instruction::Kind::CONSTEXPR_FUNC_CALL_RUN:
				return this->instr_constexpr_func_call_run(
					this->context.symbol_proc_manager.getConstexprFuncCallRun(instr)
				);

			case Instruction::Kind::IMPORT_PANTHER:
				return this->instr_import<Instruction::Language::PANTHER>(
					this->context.symbol_proc_manager.getImportPanther(instr)
				);

			case Instruction::Kind::IMPORT_C:
				return this->instr_import<Instruction::Language::C>(
					this->context.symbol_proc_manager.getImportC(instr)
				);

			case Instruction::Kind::IMPORT_CPP:
				return this->instr_import<Instruction::Language::CPP>(
					this->context.symbol_proc_manager.getImportCPP(instr)
				);

			case Instruction::Kind::IS_MACRO_DEFINED:
				return this->instr_is_macro_defined(this->context.symbol_proc_manager.getIsMacroDefined(instr));

			case Instruction::Kind::TEMPLATE_INTRINSIC_FUNC_CALL_CONSTEXPR:
				return this->instr_template_intrinsic_func_call<true>(
					this->context.symbol_proc_manager.getTemplateIntrinsicFuncCallConstexpr(instr)
				);

			case Instruction::Kind::TEMPLATE_INTRINSIC_FUNC_CALL:
				return this->instr_template_intrinsic_func_call<false>(
					this->context.symbol_proc_manager.getTemplateIntrinsicFuncCall(instr)
				);

			case Instruction::Kind::INDEXER_CONSTEXPR:
				return this->instr_indexer<true>(this->context.symbol_proc_manager.getIndexerConstexpr(instr));

			case Instruction::Kind::INDEXER:
				return this->instr_indexer<false>(this->context.symbol_proc_manager.getIndexer(instr));

			case Instruction::Kind::TEMPLATED_TERM:
				return this->instr_templated_term(this->context.symbol_proc_manager.getTemplatedTerm(instr));

			case Instruction::Kind::TEMPLATED_TERM_WAIT_FOR_DEF:
				return this->instr_templated_term_wait<true>(
					this->context.symbol_proc_manager.getTemplatedTermWaitForDef(instr)
				);

			case Instruction::Kind::TEMPLATED_TERM_WAIT_FOR_DECL:
				return this->instr_templated_term_wait<false>(
					this->context.symbol_proc_manager.getTemplatedTermWaitForDecl(instr)
				);

			case Instruction::Kind::PUSH_TEMPLATE_DECL_INSTANTIATION_TYPES_SCOPE:
				return this->instr_push_template_decl_instantiation_types_scope();

			case Instruction::Kind::POP_TEMPLATE_DECL_INSTANTIATION_TYPES_SCOPE:
				return this->instr_pop_template_decl_instantiation_types_scope();

			case Instruction::Kind::ADD_TEMPLATE_DECL_INSTANTIATION_TYPE:
				return this->instr_add_template_decl_instantiation_type(
					this->context.symbol_proc_manager.getAddTemplateDeclInstantiationType(instr)
				);

			case Instruction::Kind::COPY:
				return this->instr_copy(this->context.symbol_proc_manager.getCopy(instr));

			case Instruction::Kind::MOVE:
				return this->instr_move(this->context.symbol_proc_manager.getMove(instr));

			case Instruction::Kind::FORWARD:
				return this->instr_forward(this->context.symbol_proc_manager.getForward(instr));

			case Instruction::Kind::ADDR_OF_CONSTEXPR:
				return this->instr_addr_of(this->context.symbol_proc_manager.getAddrOfReadOnly(instr));

			case Instruction::Kind::ADDR_OF:
				return this->instr_addr_of(this->context.symbol_proc_manager.getAddrOf(instr));

			case Instruction::Kind::PREFIX_NEGATE_CONSTEXPR:
				return this->instr_prefix_negate<true>(
					this->context.symbol_proc_manager.getPrefixNegateConstexpr(instr)
				);

			case Instruction::Kind::PREFIX_NEGATE:
				return this->instr_prefix_negate<false>(this->context.symbol_proc_manager.getPrefixNegate(instr));

			case Instruction::Kind::PREFIX_NOT_CONSTEXPR:
				return this->instr_prefix_not<true>(this->context.symbol_proc_manager.getPrefixNotConstexpr(instr));

			case Instruction::Kind::PREFIX_NOT:
				return this->instr_prefix_not<false>(this->context.symbol_proc_manager.getPrefixNot(instr));

			case Instruction::Kind::PREFIX_BITWISE_NOT_CONSTEXPR:
				return this->instr_prefix_bitwise_not<true>(
					this->context.symbol_proc_manager.getPrefixBitwiseNotConstexpr(instr)
				);

			case Instruction::Kind::PREFIX_BITWISE_NOT:
				return this->instr_prefix_bitwise_not<false>(
					this->context.symbol_proc_manager.getPrefixBitwiseNot(instr)
				);

			case Instruction::Kind::DEREF:
				return this->instr_deref(this->context.symbol_proc_manager.getDeref(instr));

			case Instruction::Kind::UNWRAP:
				return this->instr_unwrap(this->context.symbol_proc_manager.getUnwrap(instr));

			case Instruction::Kind::NEW_CONSTEXPR:
				return this->instr_new<true>(
					this->context.symbol_proc_manager.getNewConstexpr(instr)
				);

			case Instruction::Kind::NEW:
				return this->instr_new<false>(
					this->context.symbol_proc_manager.getNew(instr)
				);

			case Instruction::Kind::ARRAY_INIT_NEW_CONSTEXPR:
				return this->instr_array_init_new<true>(
					this->context.symbol_proc_manager.getArrayInitNewConstexpr(instr)
				);

			case Instruction::Kind::ARRAY_INIT_NEW:
				return this->instr_array_init_new<false>(
					this->context.symbol_proc_manager.getArrayInitNew(instr)
				);

			case Instruction::Kind::DESIGNATED_INIT_NEW_CONSTEXPR:
				return this->instr_designated_init_new<true>(
					this->context.symbol_proc_manager.getDesignatedInitNewConstexpr(instr)
				);

			case Instruction::Kind::DESIGNATED_INIT_NEW:
				return this->instr_designated_init_new<false>(
					this->context.symbol_proc_manager.getDesignatedInitNew(instr)
				);

			case Instruction::Kind::PREPARE_TRY_HANDLER:
				return this->instr_prepare_try_handler(this->context.symbol_proc_manager.getPrepareTryHandler(instr));

			case Instruction::Kind::TRY_ELSE_EXPR:
				return this->instr_try_else_expr(this->context.symbol_proc_manager.getTryElseExpr(instr));

			case Instruction::Kind::BEGIN_EXPR_BLOCK:
				return this->instr_begin_expr_block(this->context.symbol_proc_manager.getBeginExprBlock(instr));

			case Instruction::Kind::END_EXPR_BLOCK:
				return this->instr_end_expr_block(this->context.symbol_proc_manager.getEndExprBlock(instr));

			case Instruction::Kind::AS_CONTEXPR:
				return this->instr_expr_as<true>(this->context.symbol_proc_manager.getAsConstexpr(instr));

			case Instruction::Kind::AS:
				return this->instr_expr_as<false>(this->context.symbol_proc_manager.getAs(instr));

			case Instruction::Kind::OPTIONAL_NULL_CHECK:
				return this->instr_optional_null_check(this->context.symbol_proc_manager.getOptionalNullCheck(instr));

			case Instruction::Kind::MATH_INFIX_CONSTEXPR_COMPARATIVE:
				return this->instr_expr_math_infix<true, Instruction::MathInfixKind::COMPARATIVE>(
					this->context.symbol_proc_manager.getMathInfixConstexprComparative(instr)
				);

			case Instruction::Kind::MATH_INFIX_CONSTEXPR_MATH:
				return this->instr_expr_math_infix<true, Instruction::MathInfixKind::MATH>(
					this->context.symbol_proc_manager.getMathInfixConstexprMath(instr)
				);

			case Instruction::Kind::MATH_INFIX_CONSTEXPR_INTEGRAL_MATH:
				return this->instr_expr_math_infix<true, Instruction::MathInfixKind::INTEGRAL_MATH>(
					this->context.symbol_proc_manager.getMathInfixConstexprIntegralMath(instr)
				);

			case Instruction::Kind::MATH_INFIX_CONSTEXPR_LOGICAL:
				return this->instr_expr_math_infix<true, Instruction::MathInfixKind::LOGICAL>(
					this->context.symbol_proc_manager.getMathInfixConstexprLogical(instr)
				);

			case Instruction::Kind::MATH_INFIX_CONSTEXPR_BITWISE_LOGICAL:
				return this->instr_expr_math_infix<true, Instruction::MathInfixKind::BITWISE_LOGICAL>(
					this->context.symbol_proc_manager.getMathInfixConstexprBitwiseLogical(instr)
				);

			case Instruction::Kind::MATH_INFIX_CONSTEXPR_SHIFT:
				return this->instr_expr_math_infix<true, Instruction::MathInfixKind::SHIFT>(
					this->context.symbol_proc_manager.getMathInfixConstexprShift(instr)
				);

			case Instruction::Kind::MATH_INFIX_COMPARATIVE:
				return this->instr_expr_math_infix<false, Instruction::MathInfixKind::COMPARATIVE>(
					this->context.symbol_proc_manager.getMathInfixComparative(instr)
				);

			case Instruction::Kind::MATH_INFIX_MATH:
				return this->instr_expr_math_infix<false, Instruction::MathInfixKind::MATH>(
					this->context.symbol_proc_manager.getMathInfixMath(instr)
				);

			case Instruction::Kind::MATH_INFIX_INTEGRAL_MATH:
				return this->instr_expr_math_infix<false, Instruction::MathInfixKind::INTEGRAL_MATH>(
					this->context.symbol_proc_manager.getMathInfixIntegralMath(instr)
				);

			case Instruction::Kind::MATH_INFIX_LOGICAL:
				return this->instr_expr_math_infix<false, Instruction::MathInfixKind::LOGICAL>(
					this->context.symbol_proc_manager.getMathInfixLogical(instr)
				);

			case Instruction::Kind::MATH_INFIX_BITWISE_LOGICAL:
				return this->instr_expr_math_infix<false, Instruction::MathInfixKind::BITWISE_LOGICAL>(
					this->context.symbol_proc_manager.getMathInfixBitwiseLogical(instr)
				);

			case Instruction::Kind::MATH_INFIX_SHIFT:
				return this->instr_expr_math_infix<false, Instruction::MathInfixKind::SHIFT>(
					this->context.symbol_proc_manager.getMathInfixShift(instr)
				);

			case Instruction::Kind::ACCESSOR_NEEDS_DEF:
				return this->instr_expr_accessor<true>(this->context.symbol_proc_manager.getAccessorNeedsDef(instr));

			case Instruction::Kind::ACCESSOR:
				return this->instr_expr_accessor<false>(this->context.symbol_proc_manager.getAccessor(instr));

			case Instruction::Kind::PRIMITIVE_TYPE:
				return this->instr_primitive_type<false>(this->context.symbol_proc_manager.getPrimitiveType(instr));

			case Instruction::Kind::PRIMITIVE_TYPE_NEEDS_DEF:
				return this->instr_primitive_type<true>(
					this->context.symbol_proc_manager.getPrimitiveTypeNeedsDef(instr)
				);

			case Instruction::Kind::ARRAY_TYPE:
				return this->instr_array_type(this->context.symbol_proc_manager.getArrayType(instr));

			case Instruction::Kind::ARRAY_REF:
				return this->instr_array_ref(this->context.symbol_proc_manager.getArrayRef(instr));

			case Instruction::Kind::TYPE_ID_CONVERTER:
				return this->instr_type_id_converter(this->context.symbol_proc_manager.getTypeIDConverter(instr));

			case Instruction::Kind::USER_TYPE:
				return this->instr_user_type(this->context.symbol_proc_manager.getUserType(instr));

			case Instruction::Kind::BASE_TYPE_IDENT:
				return this->instr_base_type_ident(this->context.symbol_proc_manager.getBaseTypeIdent(instr));

			case Instruction::Kind::IDENT_NEEDS_DEF:
				return this->instr_ident<true>(this->context.symbol_proc_manager.getIdentNeedsDef(instr));

			case Instruction::Kind::IDENT:
				return this->instr_ident<false>(this->context.symbol_proc_manager.getIdent(instr));

			case Instruction::Kind::INTRINSIC:
				return this->instr_intrinsic(this->context.symbol_proc_manager.getIntrinsic(instr));

			case Instruction::Kind::LITERAL:
				return this->instr_literal(this->context.symbol_proc_manager.getLiteral(instr));

			case Instruction::Kind::UNINIT:
				return this->instr_uninit(this->context.symbol_proc_manager.getUninit(instr));

			case Instruction::Kind::ZEROINIT:
				return this->instr_zeroinit(this->context.symbol_proc_manager.getZeroinit(instr));

			case Instruction::Kind::THIS:
				return this->instr_this(this->context.symbol_proc_manager.getThis(instr));

			case Instruction::Kind::TYPE_DEDUCER:
				return this->instr_type_deducer(this->context.symbol_proc_manager.getTypeDeducer(instr));

			case Instruction::Kind::EXPR_DEDUCER:
				return this->instr_expr_deducer(this->context.symbol_proc_manager.getExprDeducer(instr));
		}

		evo::debugFatalBreak("Unknown SymbolProc::Instruction");
	}



	auto SemanticAnalyzer::instr_suspend_symbol_proc() -> Result {
		return Result::SUSPEND;
	}



	auto SemanticAnalyzer::instr_non_local_var_decl(const Instruction::NonLocalVarDecl& instr) -> Result {
		const std::string_view var_ident = this->source.getTokenBuffer()[instr.var_def.ident].getString();

		const evo::Result<GlobalVarAttrs> var_attrs =
			this->analyze_global_var_attrs(instr.var_def, instr.attribute_params_info);
		if(var_attrs.isError()){ return Result::ERROR; }


		const TypeInfo::VoidableID got_type_info_id = this->get_type(instr.type_id);

		if(got_type_info_id.isVoid()){
			this->emit_error(
				Diagnostic::Code::SEMA_VAR_TYPE_VOID,
				*instr.var_def.type,
				"Variables cannot be type `Void`"
			);
			return Result::ERROR;
		}
		
		bool is_global = true;
		if(instr.var_def.kind == AST::VarDef::Kind::DEF){
			if(var_attrs.value().is_global){
				this->emit_error(
					Diagnostic::Code::SEMA_VAR_DEF_WITH_ATTR_GLOBAL,
					instr.var_def,
					"A [def] variable should not have the attribute `#global`"
				);
				return Result::ERROR;
			}

		}else if(this->scope.isGlobalScope()){
			if(var_attrs.value().is_global){
				this->emit_error(
					Diagnostic::Code::SEMA_VAR_GLOBAL_VAR_WITH_ATTR_GLOBAL,
					instr.var_def,
					"Global variable should not have the attribute `#global`"
				);
				return Result::ERROR;
			}
			
		}else{
			is_global = var_attrs.value().is_global;
		}


		if(is_global){
			const sema::GlobalVar::ID new_sema_var = this->context.sema_buffer.createGlobalVar(
				instr.var_def.kind,
				this->source.getID(),
				instr.var_def.ident,
				std::string(),
				std::optional<sema::Expr>(),
				got_type_info_id.asTypeID(),
				var_attrs.value().is_pub,
				this->symbol_proc_id
			);

			if(this->add_ident_to_scope(var_ident, instr.var_def, new_sema_var).isError()){ return Result::ERROR; }

			this->symbol_proc.extra_info.emplace<SymbolProc::NonLocalVarInfo>(new_sema_var);

			if(instr.var_def.kind == AST::VarDef::Kind::CONST){
				auto sema_to_pir = SemaToPIR(
					this->context, this->context.constexpr_pir_module, this->context.constexpr_sema_to_pir_data
				);

				sema::GlobalVar& sema_var = this->context.sema_buffer.global_vars[new_sema_var];
				sema_var.constexprJITGlobal = *sema_to_pir.lowerGlobalDecl(new_sema_var);
			}
		}else{
			BaseType::Struct& current_struct = this->context.type_manager.getStruct(
				this->scope.getCurrentObjectScope().as<BaseType::Struct::ID>()
			);

			const uint32_t member_index = [&](){
				const auto lock = std::scoped_lock(current_struct.memberVarsLock);
				current_struct.memberVars.emplace_back(
					instr.var_def.kind, instr.var_def.ident, got_type_info_id.asTypeID()
				);
				return uint32_t(current_struct.memberVars.size() - 1);
			}();

			if(this->add_ident_to_scope(
				var_ident, instr.var_def.ident, sema::ScopeLevel::MemberVarFlag{}, instr.var_def.ident
			).isError()){
				return Result::ERROR;
			}

			this->symbol_proc.extra_info.emplace<SymbolProc::NonLocalVarInfo>(member_index);
		}


		this->propagate_finished_decl();
		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_non_local_var_def(const Instruction::NonLocalVarDef& instr) -> Result {
		const bool is_global = 
			this->symbol_proc.extra_info.as<SymbolProc::NonLocalVarInfo>().sema_id.is<sema::GlobalVar::ID>();

		const TypeInfo::ID var_type_id = [&](){
			if(is_global){
				const sema::GlobalVar::ID sema_var_id =
					this->symbol_proc.extra_info.as<SymbolProc::NonLocalVarInfo>().sema_id.as<sema::GlobalVar::ID>();
				sema::GlobalVar& sema_var = this->context.sema_buffer.global_vars[sema_var_id];
				return *sema_var.typeID;

			}else{
				const uint32_t member_index =
					this->symbol_proc.extra_info.as<SymbolProc::NonLocalVarInfo>().sema_id.as<uint32_t>();

				BaseType::Struct& current_struct = this->context.type_manager.getStruct(
					this->scope.getCurrentObjectScope().as<BaseType::Struct::ID>()
				);

				const auto lock = std::scoped_lock(current_struct.memberVarsLock);
				return current_struct.memberVars[member_index].typeID;
			}
		}();

		if(instr.value_id.has_value()){
			TermInfo& value_term_info = this->get_term_info(*instr.value_id);

			if(value_term_info.value_category == TermInfo::ValueCategory::INITIALIZER){
				if(instr.var_def.kind != AST::VarDef::Kind::VAR){
					this->emit_error(
						Diagnostic::Code::SEMA_VAR_INITIALIZER_ON_NON_VAR,
						instr.var_def,
						"Only [var] global variables can be defined with an initializer value"
					);
					return Result::ERROR;
				}

			}else if(value_term_info.value_category == TermInfo::ValueCategory::NULL_VALUE){
				// do nothing...

			}else{
				if(value_term_info.is_ephemeral() == false){
					if(this->check_term_isnt_type(value_term_info, *instr.var_def.value).isError()){
						return Result::ERROR;
					}

					if(value_term_info.is_module()){
						this->error_type_mismatch(
							var_type_id, value_term_info, "Variable definition", *instr.var_def.value
						);
						return Result::ERROR;
					}

					this->emit_error(
						Diagnostic::Code::SEMA_VAR_DEF_NOT_EPHEMERAL,
						*instr.var_def.value,
						"Cannot define a variable with a non-ephemeral value"
					);
					return Result::ERROR;
				}
				
				if(this->type_check<true, true>(
					var_type_id, value_term_info, "Variable definition", *instr.var_def.value
				).ok == false){
					return Result::ERROR;
				}
			}

		}else if(is_global){
			this->emit_error(
				Diagnostic::Code::SEMA_VAR_GLOBAL_LIFETIME_VAR_WITHOUT_VALUE,
				instr.var_def,
				"Varibales with global lifetime must be declared with a value"
			);
			return Result::ERROR;
		}



		if(is_global){
			const sema::GlobalVar::ID sema_var_id =
				this->symbol_proc.extra_info.as<SymbolProc::NonLocalVarInfo>().sema_id.as<sema::GlobalVar::ID>();
			sema::GlobalVar& sema_var = this->context.sema_buffer.global_vars[sema_var_id];

			sema_var.expr = this->get_term_info(*instr.value_id).getExpr();

			if(instr.var_def.kind == AST::VarDef::Kind::CONST){
				auto sema_to_pir = SemaToPIR(
					this->context, this->context.constexpr_pir_module, this->context.constexpr_sema_to_pir_data
				);

				sema_to_pir.lowerGlobalDef(sema_var_id);

				const evo::Expected<void, evo::SmallVector<std::string>> add_module_subset_result = 
					this->context.constexpr_jit_engine.addModuleSubsetWithWeakDependencies(
						this->context.constexpr_pir_module,
						pir::JITEngine::ModuleSubsets{ .globalVars = *sema_var.constexprJITGlobal, }
					);

				if(add_module_subset_result.has_value() == false){
					auto infos = evo::SmallVector<Diagnostic::Info>();
					for(const std::string& error : add_module_subset_result.error()){
						infos.emplace_back(std::format("Message from LLVM: \"{}\"", error));
					}

					this->emit_fatal(
						Diagnostic::Code::MISC_LLVM_ERROR,
						instr.var_def,
						Diagnostic::createFatalMessage("Failed to setup PIR JIT interface for const global variable"),
						std::move(infos)
					);
					return Result::ERROR;
				}
			}

		}else if(instr.value_id.has_value()){ // member var with default value
			const uint32_t member_index =
				this->symbol_proc.extra_info.as<SymbolProc::NonLocalVarInfo>().sema_id.as<uint32_t>();

			BaseType::Struct& current_struct = this->context.type_manager.getStruct(
				this->scope.getCurrentObjectScope().as<BaseType::Struct::ID>()
			);

			const TermInfo& default_term = this->get_term_info(*instr.value_id);

			const auto lock = std::scoped_lock(current_struct.memberVarsLock);
			current_struct.memberVars[member_index].defaultValue = BaseType::Struct::MemberVar::DefaultValue(
				default_term.getExpr(), default_term.value_stage == TermInfo::ValueStage::CONSTEXPR
			);
		}
		

		this->propagate_finished_def();
		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_non_local_var_decl_def(const Instruction::NonLocalVarDeclDef& instr) -> Result {
		const std::string_view var_ident = this->source.getTokenBuffer()[instr.var_def.ident].getString();

		const evo::Result<GlobalVarAttrs> var_attrs =
			this->analyze_global_var_attrs(instr.var_def, instr.attribute_params_info);
		if(var_attrs.isError()){ return Result::ERROR; }


		TermInfo& value_term_info = this->get_term_info(instr.value_id);
		if(value_term_info.value_category == TermInfo::ValueCategory::MODULE){
			if(instr.var_def.kind != AST::VarDef::Kind::DEF){
				this->emit_error(
					Diagnostic::Code::SEMA_MODULE_VAR_MUST_BE_DEF,
					*instr.var_def.value,
					"Variable that has a module value must be declared as [def]"
				);
				return Result::ERROR;
			}

			const evo::Result<> add_ident_result = this->add_ident_to_scope(
				var_ident,
				instr.var_def,
				value_term_info.type_id.as<Source::ID>(),
				instr.var_def.ident,
				var_attrs.value().is_pub
			);

			// TODO(FUTURE): propgate if `add_ident_result` errored?
			this->propagate_finished_decl_def();
			return add_ident_result.isError() ? Result::ERROR : Result::SUCCESS;

		}else if(value_term_info.value_category == TermInfo::ValueCategory::CLANG_MODULE){
			if(instr.var_def.kind != AST::VarDef::Kind::DEF){
				this->emit_error(
					Diagnostic::Code::SEMA_MODULE_VAR_MUST_BE_DEF,
					*instr.var_def.value,
					"Variable that has a module value must be declared as [def]"
				);
				return Result::ERROR;
			}

			const evo::Result<> add_ident_result = this->add_ident_to_scope(
				var_ident,
				instr.var_def,
				value_term_info.type_id.as<ClangSource::ID>(),
				instr.var_def.ident,
				var_attrs.value().is_pub
			);

			// TODO(FUTURE): propgate if `add_ident_result` errored?
			this->propagate_finished_decl_def();
			return add_ident_result.isError() ? Result::ERROR : Result::SUCCESS;
		}


		if(value_term_info.value_category == TermInfo::ValueCategory::INITIALIZER){
			this->emit_error(
				Diagnostic::Code::SEMA_VAR_INITIALIZER_WITHOUT_EXPLICIT_TYPE,
				*instr.var_def.value,
				"Cannot define a variable with an initializer value without an explicit type"
			);
			return Result::ERROR;

		}else if(value_term_info.value_category == TermInfo::ValueCategory::NULL_VALUE){
			this->emit_error(
				Diagnostic::Code::SEMA_VAR_NULL_WITHOUT_EXPLICIT_TYPE,
				*instr.var_def.value,
				"Cannot define a variable with a [null] value without an explicit type"
			);
			return Result::ERROR;
		}


		if(value_term_info.is_ephemeral() == false){
			if(this->check_term_isnt_type(value_term_info, *instr.var_def.value).isError()){ return Result::ERROR; }

			this->emit_error(
				Diagnostic::Code::SEMA_VAR_DEF_NOT_EPHEMERAL,
				*instr.var_def.value,
				"Cannot define a variable with a non-ephemeral value"
			);
			return Result::ERROR;
		}

			
		if(value_term_info.isMultiValue()){
			this->emit_error(
				Diagnostic::Code::SEMA_MULTI_RETURN_INTO_SINGLE_VALUE,
				*instr.var_def.value,
				"Cannot define a variable with multiple values"
			);
			return Result::ERROR;
		}

		if(
			instr.var_def.kind != AST::VarDef::Kind::DEF &&
			value_term_info.value_category == TermInfo::ValueCategory::EPHEMERAL_FLUID
		){
			this->emit_error(
				Diagnostic::Code::SEMA_CANNOT_INFER_TYPE,
				*instr.var_def.value,
				"Cannot infer the type of a fluid literal",
				Diagnostic::Info("Did you mean this variable to be [def]? If not, give the variable an explicit type")
			);
			return Result::ERROR;
		}


		if(instr.type_id.has_value()){
			const TypeInfo::VoidableID got_type_info_id = this->get_type(*instr.type_id);

			if(got_type_info_id.isVoid()){
				this->emit_error(
					Diagnostic::Code::SEMA_VAR_TYPE_VOID, *instr.var_def.type, "Variables cannot be type `Void`"
				);
				return Result::ERROR;
			}


			const TypeCheckInfo type_check_info = this->type_check<true, true>(
				got_type_info_id.asTypeID(), value_term_info, "Variable definition", *instr.var_def.value
			);

			if(type_check_info.ok == false){ return Result::ERROR; }

			if(type_check_info.deduced_terms.empty() == false){
				if(this->scope.isGlobalScope()){
					this->emit_error(
						Diagnostic::Code::SEMA_DEDUCER_IN_GLOBAL_VAR,
						*instr.var_def.type,
						"Global variables cannot have deducers"
					);
					return Result::ERROR;
				}

				if(this->add_deduced_terms_to_scope(type_check_info.deduced_terms).isError()){ return Result::ERROR; }
			}

		}

		const std::optional<TypeInfo::ID> type_id = [&](){
			if(value_term_info.type_id.is<TypeInfo::ID>()){
				return std::optional<TypeInfo::ID>(value_term_info.type_id.as<TypeInfo::ID>());
			}
			return std::optional<TypeInfo::ID>();
		}();



		bool is_global = true;
		if(instr.var_def.kind == AST::VarDef::Kind::DEF){
			if(var_attrs.value().is_global){
				this->emit_error(
					Diagnostic::Code::SEMA_VAR_DEF_WITH_ATTR_GLOBAL,
					instr.var_def,
					"A [def] variable should not have the attribute `#global`"
				);
				return Result::ERROR;
			}

		}else if(this->scope.isGlobalScope()){
			if(var_attrs.value().is_global){
				this->emit_error(
					Diagnostic::Code::SEMA_VAR_GLOBAL_VAR_WITH_ATTR_GLOBAL,
					instr.var_def,
					"Global variable should not have the attribute `#global`"
				);
				return Result::ERROR;
			}
			
		}else{
			is_global = var_attrs.value().is_global;
		}


		if(is_global){
			const sema::GlobalVar::ID new_sema_var = this->context.sema_buffer.createGlobalVar(
				instr.var_def.kind,
				this->source.getID(),
				instr.var_def.ident,
				std::string(),
				std::optional<sema::Expr>(value_term_info.getExpr()),
				type_id,
				var_attrs.value().is_pub,
				this->symbol_proc_id
			);

			if(this->add_ident_to_scope(var_ident, instr.var_def, new_sema_var).isError()){ return Result::ERROR; }


			if(instr.var_def.kind == AST::VarDef::Kind::CONST){
				auto sema_to_pir = SemaToPIR(
					this->context, this->context.constexpr_pir_module, this->context.constexpr_sema_to_pir_data
				);

				sema::GlobalVar& sema_var = this->context.sema_buffer.global_vars[new_sema_var];
				sema_var.constexprJITGlobal = *sema_to_pir.lowerGlobalDecl(new_sema_var);
				sema_to_pir.lowerGlobalDef(new_sema_var);

				const evo::Expected<void, evo::SmallVector<std::string>> add_module_subset_result = 
					this->context.constexpr_jit_engine.addModuleSubsetWithWeakDependencies(
						this->context.constexpr_pir_module,
						pir::JITEngine::ModuleSubsets{ .globalVars = *sema_var.constexprJITGlobal, }
					);

				if(add_module_subset_result.has_value() == false){
					auto infos = evo::SmallVector<Diagnostic::Info>();
					for(const std::string& error : add_module_subset_result.error()){
						infos.emplace_back(std::format("Message from LLVM: \"{}\"", error));
					}

					this->emit_fatal(
						Diagnostic::Code::MISC_LLVM_ERROR,
						instr.var_def,
						Diagnostic::createFatalMessage("Failed to setup PIR JIT interface for const global variable"),
						std::move(infos)
					);
					return Result::ERROR;
				}
			}

		}else{
			BaseType::Struct& current_struct = this->context.type_manager.getStruct(
				this->scope.getCurrentObjectScope().as<BaseType::Struct::ID>()
			);

			{
				const auto lock = std::scoped_lock(current_struct.memberVarsLock);
				current_struct.memberVars.emplace_back(
					instr.var_def.kind,
					instr.var_def.ident,
					*type_id,
					BaseType::Struct::MemberVar::DefaultValue(
						value_term_info.getExpr(), value_term_info.value_stage == TermInfo::ValueStage::CONSTEXPR
					)
				);
			}

			if(this->add_ident_to_scope(
				var_ident, instr.var_def.ident, sema::ScopeLevel::MemberVarFlag{}, instr.var_def.ident
			).isError()){
				return Result::ERROR;
			}
		}

		this->propagate_finished_decl_def();
		return Result::SUCCESS;
	}



	auto SemanticAnalyzer::instr_when_cond(const Instruction::WhenCond& instr) -> Result {
		TermInfo& cond_term_info = this->get_term_info(instr.cond);
		if(this->check_term_isnt_type(cond_term_info, instr.when_cond.cond).isError()){ return Result::ERROR; }

		if(this->type_check<true, true>(
			this->context.getTypeManager().getTypeBool(),
			cond_term_info,
			"Condition in when conditional",
			instr.when_cond.cond
		).ok == false){
			// TODO(FUTURE): propgate error to children?
			return Result::ERROR;
		}

		SymbolProc::WhenCondInfo& when_cond_info = this->symbol_proc.extra_info.as<SymbolProc::WhenCondInfo>();
		auto passed_symbols = std::queue<SymbolProc::ID>();

		const bool cond = this->context.sema_buffer.getBoolValue(cond_term_info.getExpr().boolValueID()).value;

		if(cond){
			for(const SymbolProc::ID& then_id : when_cond_info.then_ids){
				SymbolProc& then_symbol = this->context.symbol_proc_manager.getSymbolProc(then_id);
				then_symbol.sema_scope_id = this->context.sema_buffer.scope_manager.copyScope(
					*this->symbol_proc.sema_scope_id
				);
				this->set_waiting_for_is_done(then_id, this->symbol_proc_id);
			}

			for(const SymbolProc::ID& else_id : when_cond_info.else_ids){
				passed_symbols.push(else_id);
			}

		}else{
			for(const SymbolProc::ID& else_id : when_cond_info.else_ids){
				SymbolProc& else_symbol = this->context.symbol_proc_manager.getSymbolProc(else_id);
				else_symbol.sema_scope_id = this->context.sema_buffer.scope_manager.copyScope(
					*this->symbol_proc.sema_scope_id
				);
				this->set_waiting_for_is_done(else_id, this->symbol_proc_id);
			}

			for(const SymbolProc::ID& then_id : when_cond_info.then_ids){
				passed_symbols.push(then_id);
			}
		}

		while(passed_symbols.empty() == false){
			SymbolProc::ID passed_symbol_id = passed_symbols.front();
			passed_symbols.pop();


			SymbolProc& passed_symbol = this->context.symbol_proc_manager.getSymbolProc(passed_symbol_id);

			{
				const auto lock = std::scoped_lock(passed_symbol.waiting_for_lock);
				passed_symbol.setStatusPassedOnByWhenCond();
			}


			{
				const auto lock = std::scoped_lock(passed_symbol.decl_waited_on_lock, passed_symbol.def_waited_on_lock);

				for(const SymbolProc::ID& decl_waited_on_id : passed_symbol.decl_waited_on_by){
					this->set_waiting_for_is_done(decl_waited_on_id, passed_symbol_id);
				}
				for(const SymbolProc::ID& def_waited_on_id : passed_symbol.def_waited_on_by){
					this->set_waiting_for_is_done(def_waited_on_id, passed_symbol_id);
				}
			}


			if(passed_symbol.extra_info.is<SymbolProc::WhenCondInfo>()){
				const SymbolProc::WhenCondInfo& passed_when_cond_info =
					passed_symbol.extra_info.as<SymbolProc::WhenCondInfo>();

				for(const SymbolProc::ID& then_id : passed_when_cond_info.then_ids){
					passed_symbols.push(then_id);
				}

				for(const SymbolProc::ID& else_id : passed_when_cond_info.else_ids){
					passed_symbols.push(else_id);
				}
			}
		}

		this->propagate_finished_def();
		return Result::SUCCESS;
	}



	auto SemanticAnalyzer::instr_alias_decl(const Instruction::AliasDecl& instr) -> Result {
		auto attr_pub = ConditionalAttribute(*this, "pub");

		const AST::AttributeBlock& attribute_block = 
			this->source.getASTBuffer().getAttributeBlock(instr.alias_def.attributeBlock);

		for(size_t i = 0; const AST::AttributeBlock::Attribute& attribute : attribute_block.attributes){
			EVO_DEFER([&](){ i += 1; });
			
			const std::string_view attribute_str = this->source.getTokenBuffer()[attribute.attribute].getString();

			if(attribute_str == "pub"){
				if(instr.attribute_params_info[i].empty()){
					if(attr_pub.set(attribute.attribute, true).isError()){ return Result::ERROR; } 

				}else if(instr.attribute_params_info[i].size() == 1){
					TermInfo& cond_term_info = this->get_term_info(instr.attribute_params_info[i][0]);
					if(this->check_term_isnt_type(cond_term_info, attribute.args[0]).isError()){ return Result::ERROR; }

					if(this->type_check<true, true>(
						this->context.getTypeManager().getTypeBool(),
						cond_term_info,
						"Condition in #pub",
						attribute.args[0]
					).ok == false){
						return Result::ERROR;
					}

					const bool pub_cond = this->context.sema_buffer
						.getBoolValue(cond_term_info.getExpr().boolValueID()).value;

					if(attr_pub.set(attribute.attribute, pub_cond).isError()){ return Result::ERROR; }

				}else{
					this->emit_error(
						Diagnostic::Code::SEMA_TOO_MANY_ATTRIBUTE_ARGS,
						attribute.args[1],
						"Attribute #pub does not accept more than 1 argument"
					);
					return Result::ERROR;
				}

			}else{
				this->emit_error(
					Diagnostic::Code::SEMA_UNKNOWN_ATTRIBUTE,
					attribute.attribute,
					std::format("Unknown alias attribute #{}", attribute_str)
				);
				return Result::ERROR;
			}
		}


		///////////////////////////////////
		// create

		const BaseType::ID created_alias = this->context.type_manager.getOrCreateAlias(
			BaseType::Alias(
				this->source.getID(), instr.alias_def.ident, std::optional<TypeInfoID>(), attr_pub.is_set()
			)
		);

		this->symbol_proc.extra_info.emplace<SymbolProc::AliasInfo>(created_alias.aliasID());

		const std::string_view ident_str = this->source.getTokenBuffer()[instr.alias_def.ident].getString();
		if(this->add_ident_to_scope(ident_str, instr.alias_def, created_alias.aliasID()).isError()){
			return Result::ERROR;
		}

		this->context.symbol_proc_manager.addTypeSymbolProc(
			this->context.type_manager.getOrCreateTypeInfo(TypeInfo(created_alias)), this->symbol_proc_id
		);

		this->propagate_finished_decl();
		return Result::SUCCESS;
	}



	auto SemanticAnalyzer::instr_alias_def(const Instruction::AliasDef& instr) -> Result {
		BaseType::Alias& alias_info = this->context.type_manager.getAlias(
			this->symbol_proc.extra_info.as<SymbolProc::AliasInfo>().alias_id
		);

		const TypeInfo::VoidableID aliased_type = this->get_type(instr.aliased_type);
		if(aliased_type.isVoid()){
			this->emit_error(
				Diagnostic::Code::SEMA_ALIAS_CANNOT_BE_VOID,
				instr.alias_def.type,
				"Alias cannot be type `Void`"
			);
			return Result::ERROR;
		}


		alias_info.aliasedType = aliased_type.asTypeID();

		this->propagate_finished_def();
		return Result::SUCCESS;
	};


	template<bool IS_INSTANTIATION>
	auto SemanticAnalyzer::instr_struct_decl(const Instruction::StructDecl<IS_INSTANTIATION>& instr) -> Result {
		const evo::Result<StructAttrs> struct_attrs =
			this->analyze_struct_attrs(instr.struct_def, instr.attribute_params_info);
		if(struct_attrs.isError()){ return Result::ERROR; }


		///////////////////////////////////
		// create

		SymbolProc::StructInfo& struct_info = this->symbol_proc.extra_info.as<SymbolProc::StructInfo>();


		const BaseType::ID created_struct = this->context.type_manager.getOrCreateStruct(
			BaseType::Struct{
				.sourceID          = this->source.getID(),
				.name              = instr.struct_def.ident,
				.templateID        = instr.struct_template_id,
				.instantiation     = instr.instantiation_id,
				.memberVars        = evo::SmallVector<BaseType::Struct::MemberVar>(),
				.memberVarsABI     = evo::SmallVector<BaseType::Struct::MemberVar*>(),
				.namespacedMembers = &struct_info.member_symbols,
				.scopeLevel        = nullptr,
				.isPub             = struct_attrs.value().is_pub,
				.isOrdered         = struct_attrs.value().is_ordered,
				.isPacked          = struct_attrs.value().is_packed,
				.shouldLower       = true,
			}
		);

		struct_info.struct_id = created_struct.structID();

		if constexpr(IS_INSTANTIATION == false){
			const std::string_view ident_str = this->source.getTokenBuffer()[instr.struct_def.ident].getString();
			if(this->add_ident_to_scope(ident_str, instr.struct_def, created_struct.structID()).isError()){
				return Result::ERROR;
			}
		}


		///////////////////////////////////
		// setup member statements

		this->push_scope_level(nullptr, created_struct.structID());

		BaseType::Struct& created_struct_ref = this->context.type_manager.getStruct(created_struct.structID());
		created_struct_ref.scopeLevel = &this->get_current_scope_level();


		for(const SymbolProc::ID& member_stmt_id : struct_info.stmts){
			SymbolProc& member_stmt = this->context.symbol_proc_manager.getSymbolProc(member_stmt_id);

			member_stmt.sema_scope_id = this->context.sema_buffer.scope_manager.copyScope(
				*this->symbol_proc.sema_scope_id
			);

			if(member_stmt.ast_node.kind() == AST::Kind::FUNC_DEF){
				const auto lock = std::scoped_lock(this->symbol_proc.waiting_for_lock, member_stmt.decl_waited_on_lock);
				this->symbol_proc.waiting_for.emplace_back(member_stmt_id);
				member_stmt.decl_waited_on_by.emplace_back(this->symbol_proc_id);

			}else{
				const auto lock = std::scoped_lock(this->symbol_proc.waiting_for_lock, member_stmt.def_waited_on_lock);
				this->symbol_proc.waiting_for.emplace_back(member_stmt_id);
				member_stmt.def_waited_on_by.emplace_back(this->symbol_proc_id);
			}
		}


		if constexpr(IS_INSTANTIATION){
			struct_info.instantiation->structID = created_struct.structID();
		}

		this->context.symbol_proc_manager.addTypeSymbolProc(
			this->context.type_manager.getOrCreateTypeInfo(TypeInfo(created_struct)), this->symbol_proc_id
		);

		this->propagate_finished_decl();

		if(struct_info.stmts.empty()){
			return Result::SUCCESS;
		}else{
			return Result::NEED_TO_WAIT_BEFORE_NEXT_INSTR;
		}
	}


	auto SemanticAnalyzer::instr_struct_def() -> Result {
		if(this->pop_scope_level<PopScopeLevelKind::SYMBOL_END>().isError()){ return Result::ERROR; }

 
		const BaseType::Struct::ID created_struct_id =
			this->symbol_proc.extra_info.as<SymbolProc::StructInfo>().struct_id;
		BaseType::Struct& created_struct = this->context.type_manager.getStruct(created_struct_id);


		if(created_struct.newInitOverloads.empty() && created_struct.newAssignOverloads.empty() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_STRUCT_NEW_REASSIGN_WITHOUT_NEW_INIT,
				this->symbol_proc.ast_node,
				"Cannot define a struct with a assignment operator [new] overload "
					"without an initializer operator [new] overload"
			);
			return Result::ERROR;
		}


		///////////////////////////////////
		// sorting members ABI

		const auto sorting_func = [](
			const BaseType::Struct::MemberVar& lhs, const BaseType::Struct::MemberVar& rhs
		) -> bool {
			return lhs.name.as<Token::ID>().get() < rhs.name.as<Token::ID>().get();
		};

		std::sort(created_struct.memberVars.begin(), created_struct.memberVars.end(), sorting_func);

		// TODO(FEATURE): optimal ordering (when not #ordered)

		for(BaseType::Struct::MemberVar& member_var : created_struct.memberVars){
			created_struct.memberVarsABI.emplace_back(&member_var);
		}

		///////////////////////////////////
		// PIR lowering for constexpr

		auto sema_to_pir = SemaToPIR(
			this->context, this->context.constexpr_pir_module, this->context.constexpr_sema_to_pir_data
		);

		sema_to_pir.lowerStruct(created_struct_id);



		///////////////////////////////////
		// default construction

		this->symbol_proc.data_stack.push(SymbolProc::StructSpecialMemberFuncs());

		auto funcs_to_wait_on = std::unordered_set<sema::Func::ID>();


		if(created_struct.newInitOverloads.empty()){
			created_struct.isDefaultInitializable = true;
			created_struct.isTriviallyDefaultInitializable = true;
			created_struct.isConstexprDefaultInitializable = true;
			created_struct.isNoErrorDefaultInitializable = true;

			for(const BaseType::Struct::MemberVar& member_var : created_struct.memberVars){
				if(this->context.getTypeManager().isDefaultInitializable(member_var.typeID) == false){
					if(member_var.defaultValue.has_value()){
						created_struct.isTriviallyDefaultInitializable = false;

						if(member_var.defaultValue->isConstexpr == false){
							created_struct.isConstexprDefaultInitializable = false;
						}
						
					}else{
						created_struct.isDefaultInitializable = false;
						created_struct.isTriviallyDefaultInitializable = false;
						created_struct.isConstexprDefaultInitializable = false;
						created_struct.isNoErrorDefaultInitializable = false;
						break;
					}
				}

				if(this->context.getTypeManager().isNoErrorDefaultInitializable(member_var.typeID) == false){
					created_struct.isDefaultInitializable = false;
					created_struct.isTriviallyDefaultInitializable = false;
					created_struct.isConstexprDefaultInitializable = false;
					created_struct.isNoErrorDefaultInitializable = false;
					break;
				}

				if(created_struct.isConstexprDefaultInitializable){
					if(
						(member_var.defaultValue.has_value() && member_var.defaultValue->isConstexpr == false)
						|| this->context.getTypeManager().isConstexprDefaultInitializable(member_var.typeID) == false
					){
						created_struct.isConstexprDefaultInitializable = false;
					}
				}

				if(created_struct.isTriviallyDefaultInitializable){
					if(	
						member_var.defaultValue.has_value()
						|| this->context.getTypeManager().isTriviallyDefaultInitializable(member_var.typeID) == false
					){
						created_struct.isTriviallyDefaultInitializable = false;
					}
				}
			}

			if(created_struct.isDefaultInitializable && created_struct.isTriviallyDefaultInitializable == false){
				const TypeInfo::ID created_struct_type_id = 
					this->context.type_manager.getOrCreateTypeInfo(TypeInfo(BaseType::ID(created_struct_id)));

				const BaseType::ID default_init_func_type = this->context.type_manager.getOrCreateFunction(
					BaseType::Function(
						evo::SmallVector<BaseType::Function::Param>(),
						evo::SmallVector<BaseType::Function::ReturnParam>{
							BaseType::Function::ReturnParam(created_struct.name.as<Token::ID>(), created_struct_type_id)
						},
						evo::SmallVector<BaseType::Function::ReturnParam>()
					)
				);

				const sema::Func::ID created_default_init_new_id = this->context.sema_buffer.createFunc(
					this->source.getID(),
					sema::Func::CompilerCreatedOpOverload(
						created_struct.name.as<Token::ID>(), Token::Kind::KEYWORD_NEW
					),
					std::string(),
					default_init_func_type.funcID(),
					evo::SmallVector<sema::Func::Param>(),
					std::nullopt,
					0,
					false,
					created_struct.isConstexprDefaultInitializable,
					false,
					false
				);


				sema::Func& created_default_init_new = this->context.sema_buffer.funcs[created_default_init_new_id];

				const sema::Expr return_param = sema::Expr(this->context.sema_buffer.createReturnParam(0, 0));

				for(size_t i = 0; const BaseType::Struct::MemberVar& member_var : created_struct.memberVars){
					EVO_DEFER([&](){ i += 1; });

					const sema::Expr member_var_expr = [&](){
						for(
							uint32_t j = 0;
							const BaseType::Struct::MemberVar* member_var_abi : created_struct.memberVarsABI
						){
							if(member_var_abi == &member_var){
								return sema::Expr(this->context.sema_buffer.createAccessor(
									return_param, created_struct_type_id, j
								));
							}

							j += 1;
						}
						evo::debugFatalBreak("Didn't find ABI member");
					}();

					if(member_var.defaultValue.has_value()){
						created_default_init_new.stmtBlock.emplace_back(
							this->context.sema_buffer.createAssign(member_var_expr, member_var.defaultValue->value)
						);
						continue;
					}


					if(this->context.getTypeManager().isTriviallyDefaultInitializable(member_var.typeID)){ continue; }

					const TypeInfo& member_type = this->context.getTypeManager().getTypeInfo(member_var.typeID);


					if(member_type.isOptional()){
						created_default_init_new.stmtBlock.emplace_back(
							this->context.sema_buffer.createAssign(
								member_var_expr,
								sema::Expr(this->context.sema_buffer.createNull(member_var.typeID))
							)
						);
						continue;
					}

					switch(member_type.baseTypeID().kind()){
						case BaseType::Kind::ARRAY_REF: {
							created_default_init_new.stmtBlock.emplace_back(
								this->context.sema_buffer.createAssign(
									member_var_expr,
									sema::Expr(this->context.sema_buffer.createDefaultInitArrayRef(
										member_type.baseTypeID().arrayRefID()
									))
								)
							);
						} break;

						case BaseType::Kind::STRUCT: {
							const BaseType::Struct& member_struct_type = 
								this->context.getTypeManager().getStruct(member_type.baseTypeID().structID());

							for(sema::Func::ID new_init_overload_id : member_struct_type.newInitOverloads){
								const sema::Func& new_init_overload =
									this->context.getSemaBuffer().getFunc(new_init_overload_id);

								if(new_init_overload.minNumArgs != 0){ continue; }

								funcs_to_wait_on.emplace(new_init_overload_id);

								auto args = evo::SmallVector<sema::Expr>();
								for(size_t j = new_init_overload.minNumArgs; j < new_init_overload.params.size(); j+=1){
									args.emplace_back(*new_init_overload.params[j].defaultValue);
								}

								created_default_init_new.stmtBlock.emplace_back(
									this->context.sema_buffer.createAssign(
										member_var_expr,
										sema::Expr(this->context.sema_buffer.createFuncCall(
											new_init_overload_id, std::move(args)
										))
									)
								);
								break;
							}
						} break;

						default: {
							this->emit_error(
								Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
								created_struct_id,
								"Creating the default constructor of this type is unimplemented as default "
									"constructing one of the member types is unimplemented"
							);
							return Result::ERROR;
						} break;
					}
				}

				created_default_init_new.stmtBlock.emplace_back(this->context.sema_buffer.createReturn());

				created_default_init_new.isTerminated = true;
				created_default_init_new.status = sema::Func::Status::DEF_DONE;

				created_struct.newInitOverloads.emplace_back(created_default_init_new_id);

				if(created_struct.isConstexprDefaultInitializable){
					this->symbol_proc.data_stack.top().as<SymbolProc::StructSpecialMemberFuncs>().init_func = 
						created_default_init_new_id;
				}
			}

		}else{
			for(const sema::Func::ID& new_init_overload_id : created_struct.newInitOverloads){
				const sema::Func& new_init_overload = this->context.getSemaBuffer().getFunc(new_init_overload_id);
				const BaseType::Function& new_init_overload_func_type =
					this->context.getTypeManager().getFunction(new_init_overload.typeID);

				if(new_init_overload.minNumArgs > 0){ continue; }

				created_struct.isDefaultInitializable = true;

				if(new_init_overload.isConstexpr){
					created_struct.isConstexprDefaultInitializable = true;
				}

				if(new_init_overload_func_type.errorParams.empty()){
					created_struct.isNoErrorDefaultInitializable = true;
				}

				break;
			}
		}



		///////////////////////////////////
		// default deleting

		if(created_struct.deleteOverload.load().has_value() == false){
			bool is_trivially_deletable = true;
			bool is_constexpr_deletable = true;


			for(const BaseType::Struct::MemberVar& member_var : created_struct.memberVars){
				if(this->context.getTypeManager().isTriviallyDeletable(member_var.typeID) == false){
					is_trivially_deletable = false;
					is_constexpr_deletable = false;
					break;
				}

				if(is_constexpr_deletable == false){ continue; }
				if(this->context.getTypeManager().isConstexprDeletable(
					member_var.typeID, this->context.getSemaBuffer()
				) == false){
					is_constexpr_deletable = false;
				}
			}


			if(is_trivially_deletable == false){
				const TypeInfo::ID created_struct_type_id = 
					this->context.type_manager.getOrCreateTypeInfo(TypeInfo(BaseType::ID(created_struct_id)));

				const BaseType::ID default_delete_func_type = this->context.type_manager.getOrCreateFunction(
					BaseType::Function(
						evo::SmallVector<BaseType::Function::Param>{
							BaseType::Function::Param(
								created_struct_type_id, BaseType::Function::Param::Kind::MUT, false
							),
						},
						evo::SmallVector<BaseType::Function::ReturnParam>{
							BaseType::Function::ReturnParam(std::nullopt, TypeInfo::VoidableID::Void())
						},
						evo::SmallVector<BaseType::Function::ReturnParam>()
					)
				);

				const sema::Func::ID created_default_delete_id = this->context.sema_buffer.createFunc(
					this->source.getID(),
					sema::Func::CompilerCreatedOpOverload(
						created_struct.name.as<Token::ID>(), Token::Kind::KEYWORD_DELETE
					),
					std::string(),
					default_delete_func_type.funcID(),
					evo::SmallVector<sema::Func::Param>{
						sema::Func::Param(created_struct.name.as<Token::ID>(), std::nullopt)
					},
					std::nullopt,
					1,
					false,
					is_constexpr_deletable,
					false,
					false
				);


				sema::Func& created_default_delete = this->context.sema_buffer.funcs[created_default_delete_id];

				const sema::Expr this_param = sema::Expr(this->context.sema_buffer.createParam(0, 0));

				for(const BaseType::Struct::MemberVar& member_var : created_struct.memberVars){
					if(this->context.getTypeManager().isTriviallyDeletable(member_var.typeID)){ continue; }

					const sema::Expr member_var_expr = [&](){
						for(
							uint32_t j = 0;
							const BaseType::Struct::MemberVar* member_var_abi : created_struct.memberVarsABI
						){
							if(member_var_abi == &member_var){
								return sema::Expr(this->context.sema_buffer.createAccessor(
									this_param, created_struct_type_id, j
								));
							}

							j += 1;
						}
						evo::debugFatalBreak("Didn't find ABI member");
					}();


					this->get_special_member_stmt_dependents<SpecialMemberKind::DELETE>(
						member_var.typeID, funcs_to_wait_on
					);
					created_default_delete.stmtBlock.emplace_back(
						this->context.sema_buffer.createDelete(member_var_expr, member_var.typeID)
					);

				}

				created_default_delete.stmtBlock.emplace_back(this->context.sema_buffer.createReturn());

				created_default_delete.isTerminated = true;
				created_default_delete.status = sema::Func::Status::DEF_DONE;

				created_struct.deleteOverload = created_default_delete_id;

				if(is_constexpr_deletable){
					this->symbol_proc.data_stack.top().as<SymbolProc::StructSpecialMemberFuncs>().delete_func = 
						created_default_delete_id;
				}
			}
		}


		///////////////////////////////////
		// default move

		const BaseType::Struct::DeletableOverload copy_init_overload = created_struct.copyInitOverload.load();
		const BaseType::Struct::DeletableOverload move_init_overload = created_struct.moveInitOverload.load();

		if(move_init_overload.wasDeleted == false && move_init_overload.funcID.has_value() == false){
			if(copy_init_overload.wasDeleted){
				created_struct.moveInitOverload = BaseType::Struct::DeletableOverload(std::nullopt, true);

			}else if(copy_init_overload.funcID.has_value()){
				created_struct.moveInitOverload = BaseType::Struct::DeletableOverload(copy_init_overload.funcID, false);

			}else{
				if(created_struct.moveAssignOverload.load().has_value()){
					this->emit_error(
						Diagnostic::Code::SEMA_INVALID_OPERATOR_MOVE_OVERLOAD,
						*created_struct.moveAssignOverload.load(),
						"Operator [move] assignment overload cannot be defined without an operator [move] "
							"initialization overload"
					);
					return Result::ERROR;
				}


				bool is_movable = true;
				bool is_trivially_movable = true;
				bool is_constexpr_movable = true;

				for(const BaseType::Struct::MemberVar& member_var : created_struct.memberVars){
					if(this->context.getTypeManager().isMovable(member_var.typeID) == false){
						is_movable = false;
						is_trivially_movable = false;
						is_constexpr_movable = false;

						created_struct.moveInitOverload = 
							BaseType::Struct::DeletableOverload(copy_init_overload.funcID, true);

						break;
					}

					if(
						is_trivially_movable
						&& this->context.getTypeManager().isTriviallyMovable(member_var.typeID) == false
					){
						is_trivially_movable = false;
					}


					if(is_constexpr_movable == false){ continue; }
					if(this->context.getTypeManager().isConstexprMovable(
						member_var.typeID, this->context.getSemaBuffer()
					) == false){
						is_constexpr_movable = false;
					}
				}



				if(is_movable && is_trivially_movable == false){
					const TypeInfo::ID created_struct_type_id = 
						this->context.type_manager.getOrCreateTypeInfo(TypeInfo(BaseType::ID(created_struct_id)));

					const BaseType::ID default_move_func_type = this->context.type_manager.getOrCreateFunction(
						BaseType::Function(
							evo::SmallVector<BaseType::Function::Param>{
								BaseType::Function::Param(
									created_struct_type_id, BaseType::Function::Param::Kind::MUT, false
								),
							},
							evo::SmallVector<BaseType::Function::ReturnParam>{
								BaseType::Function::ReturnParam(
									created_struct.name.as<Token::ID>(), created_struct_type_id
								)
							},
							evo::SmallVector<BaseType::Function::ReturnParam>()
						)
					);

					const sema::Func::ID created_default_move_id = this->context.sema_buffer.createFunc(
						this->source.getID(),
						sema::Func::CompilerCreatedOpOverload(
							created_struct.name.as<Token::ID>(), Token::Kind::KEYWORD_MOVE
						),
						std::string(),
						default_move_func_type.funcID(),
						evo::SmallVector<sema::Func::Param>{
							sema::Func::Param(created_struct.name.as<Token::ID>(), std::nullopt)
						},
						std::nullopt,
						1,
						false,
						is_constexpr_movable,
						false,
						false
					);


					sema::Func& created_default_move = this->context.sema_buffer.funcs[created_default_move_id];

					const sema::Expr this_param = sema::Expr(this->context.sema_buffer.createParam(0, 0));
					const sema::Expr output_param = sema::Expr(this->context.sema_buffer.createReturnParam(0, 1));

					for(const BaseType::Struct::MemberVar& member_var : created_struct.memberVars){
						auto this_member = std::optional<sema::Expr>();
						auto output_member = std::optional<sema::Expr>();

						for(
							uint32_t j = 0;
							const BaseType::Struct::MemberVar* member_var_abi : created_struct.memberVarsABI
						){
							if(member_var_abi == &member_var){
								this_member = sema::Expr(this->context.sema_buffer.createAccessor(
									this_param, created_struct_type_id, j
								));

								output_member = sema::Expr(this->context.sema_buffer.createAccessor(
									output_param, created_struct_type_id, j
								));

								break;
							}

							j += 1;
						}


						this->get_special_member_stmt_dependents<SpecialMemberKind::MOVE>(
							member_var.typeID, funcs_to_wait_on
						);

						created_default_move.stmtBlock.emplace_back(
							this->context.sema_buffer.createAssign(
								*output_member,
								sema::Expr(this->context.sema_buffer.createMove(*this_member, member_var.typeID))
							)
						);
					}

					created_default_move.stmtBlock.emplace_back(this->context.sema_buffer.createReturn());

					created_default_move.isTerminated = true;
					created_default_move.status = sema::Func::Status::DEF_DONE;

					created_struct.moveInitOverload = 
						BaseType::Struct::DeletableOverload(created_default_move_id, false);

					if(is_constexpr_movable){
						this->symbol_proc.data_stack.top().as<SymbolProc::StructSpecialMemberFuncs>().move_func = 
							created_default_move_id;
					}
				}
			}
		}


		///////////////////////////////////
		// default copy

		if(copy_init_overload.wasDeleted == false && copy_init_overload.funcID.has_value() == false){
			if(move_init_overload.funcID.has_value()){
				created_struct.copyInitOverload = BaseType::Struct::DeletableOverload(std::nullopt, true);

			}else{
				if(created_struct.copyAssignOverload.load().has_value()){
					this->emit_error(
						Diagnostic::Code::SEMA_INVALID_OPERATOR_COPY_OVERLOAD,
						*created_struct.copyAssignOverload.load(),
						"Operator [copy] assignment overload cannot be defined without an operator [copy] "
							"initialization overload"
					);
					return Result::ERROR;
				}

				bool is_copyable = true;
				bool is_trivially_copyable = true;
				bool is_constexpr_copyable = true;

				for(const BaseType::Struct::MemberVar& member_var : created_struct.memberVars){
					if(this->context.getTypeManager().isCopyable(member_var.typeID) == false){
						is_copyable = false;
						is_trivially_copyable = false;
						is_constexpr_copyable = false;

						created_struct.copyInitOverload = BaseType::Struct::DeletableOverload(std::nullopt, true);
						continue;
					}

					if(
						is_trivially_copyable
						&& this->context.getTypeManager().isTriviallyCopyable(member_var.typeID) == false
					){
						is_trivially_copyable = false;
						break;
					}


					if(is_constexpr_copyable == false){ continue; }
					if(this->context.getTypeManager().isConstexprCopyable(
						member_var.typeID, this->context.getSemaBuffer()
					) == false){
						is_constexpr_copyable = false;
					}
				}



				if(is_copyable && is_trivially_copyable == false){
					const TypeInfo::ID created_struct_type_id = 
						this->context.type_manager.getOrCreateTypeInfo(TypeInfo(BaseType::ID(created_struct_id)));

					const BaseType::ID default_copy_func_type = this->context.type_manager.getOrCreateFunction(
						BaseType::Function(
							evo::SmallVector<BaseType::Function::Param>{
								BaseType::Function::Param(
									created_struct_type_id, BaseType::Function::Param::Kind::MUT, false
								),
							},
							evo::SmallVector<BaseType::Function::ReturnParam>{
								BaseType::Function::ReturnParam(
									created_struct.name.as<Token::ID>(), created_struct_type_id
								)
							},
							evo::SmallVector<BaseType::Function::ReturnParam>()
						)
					);

					const sema::Func::ID created_default_copy_id = this->context.sema_buffer.createFunc(
						this->source.getID(),
						sema::Func::CompilerCreatedOpOverload(
							created_struct.name.as<Token::ID>(), Token::Kind::KEYWORD_COPY
						),
						std::string(),
						default_copy_func_type.funcID(),
						evo::SmallVector<sema::Func::Param>{
							sema::Func::Param(created_struct.name.as<Token::ID>(), std::nullopt)
						},
						std::nullopt,
						1,
						false,
						is_constexpr_copyable,
						false,
						false
					);


					sema::Func& created_default_copy = this->context.sema_buffer.funcs[created_default_copy_id];

					const sema::Expr this_param = sema::Expr(this->context.sema_buffer.createParam(0, 0));
					const sema::Expr output_param = sema::Expr(this->context.sema_buffer.createReturnParam(0, 1));

					for(const BaseType::Struct::MemberVar& member_var : created_struct.memberVars){
						auto this_member = std::optional<sema::Expr>();
						auto output_member = std::optional<sema::Expr>();

						for(
							uint32_t j = 0;
							const BaseType::Struct::MemberVar* member_var_abi : created_struct.memberVarsABI
						){
							if(member_var_abi == &member_var){
								this_member = sema::Expr(this->context.sema_buffer.createAccessor(
									this_param, created_struct_type_id, j
								));

								output_member = sema::Expr(this->context.sema_buffer.createAccessor(
									output_param, created_struct_type_id, j
								));

								break;
							}

							j += 1;
						}



						this->get_special_member_stmt_dependents<SpecialMemberKind::COPY>(
							member_var.typeID, funcs_to_wait_on
						);

						created_default_copy.stmtBlock.emplace_back(
							this->context.sema_buffer.createAssign(
								*output_member,
								sema::Expr(this->context.sema_buffer.createCopy(*this_member, member_var.typeID))
							)
						);
					}

					created_default_copy.stmtBlock.emplace_back(this->context.sema_buffer.createReturn());

					created_default_copy.isTerminated = true;
					created_default_copy.status = sema::Func::Status::DEF_DONE;

					created_struct.copyInitOverload = 
						BaseType::Struct::DeletableOverload(created_default_copy_id, false);

					if(is_constexpr_copyable){
						this->symbol_proc.data_stack.top().as<SymbolProc::StructSpecialMemberFuncs>().copy_func = 
							created_default_copy_id;
					}
				}
			}
		}




		///////////////////////////////////
		// wait on funcs

		bool waiting_on_any = false;
		for(const sema::Func::ID func_to_wait_on_id : funcs_to_wait_on){
			const sema::Func& func_to_wait_on = this->context.getSemaBuffer().getFunc(func_to_wait_on_id);
			if(func_to_wait_on.symbolProcID.has_value() == false){ continue; }

			SymbolProc& wait_on_func = this->context.symbol_proc_manager.getSymbolProc(*func_to_wait_on.symbolProcID);

			switch(wait_on_func.waitOnDeclIfNeeded(this->symbol_proc_id, this->context, *func_to_wait_on.symbolProcID)){
				break; case SymbolProc::WaitOnResult::NOT_NEEDED:                 // do nothing
				break; case SymbolProc::WaitOnResult::WAITING:                    waiting_on_any = true;
				break; case SymbolProc::WaitOnResult::WAS_ERRORED:                return Result::ERROR;
				break; case SymbolProc::WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND: return Result::ERROR;
				break; case SymbolProc::WaitOnResult::CIRCULAR_DEP_DETECTED:      return Result::ERROR;
			}
		}

		if(waiting_on_any){
			return Result::NEED_TO_WAIT_BEFORE_NEXT_INSTR;
		}else{
			return Result::SUCCESS;
		}
	}



	auto SemanticAnalyzer::instr_struct_created_sepcial_members_pir_if_needed() -> Result {
		auto sema_to_pir = SemaToPIR(
			this->context, this->context.constexpr_pir_module, this->context.constexpr_sema_to_pir_data
		);


		const SymbolProc::StructSpecialMemberFuncs& struct_special_member_funcs = 
			this->symbol_proc.data_stack.top().as<SymbolProc::StructSpecialMemberFuncs>();


		const auto lower_func = [&](sema::Func::ID target_func_id) -> evo::Result<> {
			sema::Func& target_func = this->context.sema_buffer.funcs[target_func_id];


			target_func.constexprJITFunc = sema_to_pir.lowerFuncDeclConstexpr(target_func_id);

			sema_to_pir.lowerFuncDef(target_func_id);

			target_func.constexprJITInterfaceFunc = sema_to_pir.createFuncJITInterface(
				target_func_id, *target_func.constexprJITFunc
			);


			auto module_subset_funcs = evo::StaticVector<pir::Function::ID, 2>{
				*target_func.constexprJITFunc, *target_func.constexprJITInterfaceFunc
			};

			const evo::Expected<void, evo::SmallVector<std::string>> add_module_subset_result = 
				this->context.constexpr_jit_engine.addModuleSubsetWithWeakDependencies(
					this->context.constexpr_pir_module,
					pir::JITEngine::ModuleSubsets{ .funcs = module_subset_funcs, }
				);

			if(add_module_subset_result.has_value() == false){
				auto infos = evo::SmallVector<Diagnostic::Info>();
				for(const std::string& error : add_module_subset_result.error()){
					infos.emplace_back(std::format("Message from LLVM: \"{}\"", error));
				}

				this->emit_fatal(
					Diagnostic::Code::MISC_LLVM_ERROR,
					this->symbol_proc.extra_info.as<SymbolProc::StructInfo>().struct_id,
					Diagnostic::createFatalMessage(
						"Failed to setup PIR JIT interface generated default operator [new]"
					),
					std::move(infos)
				);
				return evo::resultError;
			}

			return evo::Result<>();
		};



		if(struct_special_member_funcs.init_func.has_value()){
			if(lower_func(*struct_special_member_funcs.init_func).isError()){ return Result::ERROR; }
		}

		if(struct_special_member_funcs.delete_func.has_value()){
			if(lower_func(*struct_special_member_funcs.delete_func).isError()){ return Result::ERROR; }
		}

		if(struct_special_member_funcs.copy_func.has_value()){
			if(lower_func(*struct_special_member_funcs.copy_func).isError()){ return Result::ERROR; }
		}

		if(struct_special_member_funcs.move_func.has_value()){
			if(lower_func(*struct_special_member_funcs.move_func).isError()){ return Result::ERROR; }
		}



		this->symbol_proc.data_stack.pop();


		///////////////////////////////////
		// done

		this->context.type_manager.getStruct(
			this->symbol_proc.extra_info.as<SymbolProc::StructInfo>().struct_id
		).defCompleted = true;

		this->propagate_finished_def();

		return Result::SUCCESS;
	}



	auto SemanticAnalyzer::instr_template_struct(const Instruction::TemplateStruct& instr) -> Result {
		size_t minimum_num_template_args = 0;
		auto params = evo::SmallVector<BaseType::StructTemplate::Param>();

		const AST::TemplatePack& ast_template_pack = 
			this->source.getASTBuffer().getTemplatePack(*instr.struct_def.templatePack);

		using TemplateParamInfo = SymbolProc::Instruction::TemplateParamInfo;
		for(size_t i = 0; const TemplateParamInfo& template_param_info : instr.template_param_infos){
			EVO_DEFER([&](){ i += 1; });

			auto type_id = std::optional<TypeInfoID>();
			if(template_param_info.type_id.has_value()){
				const TypeInfo::VoidableID type_info_voidable_id = this->get_type(*template_param_info.type_id);
				if(type_info_voidable_id.isVoid()){
					this->emit_error(
						Diagnostic::Code::SEMA_TEMPLATE_PARAM_CANNOT_BE_TYPE_VOID,
						template_param_info.param.type,
						"Template parameter cannot be type `Void`"
					);
					return Result::ERROR;
				}
				type_id = type_info_voidable_id.asTypeID();
			}

			TermInfo* default_value = nullptr;
			if(template_param_info.default_value.has_value()){
				default_value = &this->get_term_info(*template_param_info.default_value);

				if(type_id.has_value()){
					if(default_value->isSingleValue() == false){
						if(default_value->isMultiValue()){
							this->emit_error(
								Diagnostic::Code::SEMA_TEMPLATE_PARAM_EXPR_DEFAULT_MUST_BE_EXPR,
								*template_param_info.param.defaultValue,
								"Default of an expression template parameter must be a single expression"
							);	
						}else{
							this->emit_error(
								Diagnostic::Code::SEMA_TEMPLATE_PARAM_EXPR_DEFAULT_MUST_BE_EXPR,
								*template_param_info.param.defaultValue,
								"Default of an expression template parameter must be an expression"
							);
						}
						return Result::ERROR;
					}

					const TypeCheckInfo type_check_info = this->type_check<true, true>(
						*type_id,
						*default_value,
						"Default value of template parameter",
						*template_param_info.param.defaultValue
					);
					if(type_check_info.ok == false){
						return Result::ERROR;
					}

				}else{
					if(default_value->value_category != TermInfo::ValueCategory::TYPE){
						this->emit_error(
							Diagnostic::Code::SEMA_TEMPLATE_PARAM_TYPE_DEFAULT_MUST_BE_TYPE,
							*template_param_info.param.defaultValue,
							"Default of a [Type] template parameter must be an type"
						);
						return Result::ERROR;
					}
				}
			}else{
				minimum_num_template_args += 1;
			}

			if(default_value == nullptr){
				params.emplace_back(
					this->source.getASTBuffer().getType(ast_template_pack.params[i].type), type_id, std::monostate()
				);

			}else if(default_value->value_category == TermInfo::ValueCategory::TYPE){
				params.emplace_back(
					this->source.getASTBuffer().getType(ast_template_pack.params[i].type),
					type_id,
					default_value->type_id.as<TypeInfo::VoidableID>()
				);

			}else{
				params.emplace_back(
					this->source.getASTBuffer().getType(ast_template_pack.params[i].type),
					type_id,
					default_value->getExpr()
				);
			}
		}


		const BaseType::ID created_struct_type_id = this->context.type_manager.getOrCreateStructTemplate(
			BaseType::StructTemplate(
				this->source.getID(), instr.struct_def.ident, std::move(params), minimum_num_template_args
			)
		);
		
		const sema::TemplatedStruct::ID new_templated_struct = this->context.sema_buffer.createTemplatedStruct(
			created_struct_type_id.structTemplateID(), this->symbol_proc
		);

		const std::string_view ident_str = this->source.getTokenBuffer()[instr.struct_def.ident].getString();
		if(this->add_ident_to_scope(ident_str, instr.struct_def, new_templated_struct).isError()){
			return Result::ERROR;
		}

		this->propagate_finished_decl_def();

		return Result::SUCCESS;
	};


	auto SemanticAnalyzer::instr_union_decl(const Instruction::UnionDecl& instr) -> Result {
		const evo::Result<UnionAttrs> union_attrs = 
			this->analyze_union_attrs(instr.union_def, instr.attribute_params_info);
		if(union_attrs.isError()){ return Result::ERROR; }



		///////////////////////////////////
		// create

		SymbolProc::UnionInfo& union_info = this->symbol_proc.extra_info.as<SymbolProc::UnionInfo>();


		const BaseType::ID created_union = this->context.type_manager.getOrCreateUnion(
			BaseType::Union(
				this->source.getID(),
				instr.union_def.ident,
				evo::SmallVector<BaseType::Union::Field>(),
				&union_info.member_symbols,
				nullptr,
				union_attrs.value().is_pub,
				union_attrs.value().is_untagged
			)
		);

		union_info.union_id = created_union.unionID();


		const std::string_view ident_str = this->source.getTokenBuffer()[instr.union_def.ident].getString();
		if(this->add_ident_to_scope(ident_str, instr.union_def, created_union.unionID()).isError()){
			return Result::ERROR;
		}

		this->context.symbol_proc_manager.addTypeSymbolProc(
			this->context.type_manager.getOrCreateTypeInfo(TypeInfo(created_union)), this->symbol_proc_id
		);



		///////////////////////////////////
		// setup scope

		this->push_scope_level(nullptr, created_union.unionID());

		BaseType::Union& created_union_ref = this->context.type_manager.getUnion(created_union.unionID());
		created_union_ref.scopeLevel = &this->get_current_scope_level();


		for(const SymbolProc::ID& member_stmt_id : union_info.stmts){
			SymbolProc& member_stmt = this->context.symbol_proc_manager.getSymbolProc(member_stmt_id);

			member_stmt.sema_scope_id = this->context.sema_buffer.scope_manager.copyScope(
				*this->symbol_proc.sema_scope_id
			);
		}


		///////////////////////////////////
		// done

		this->propagate_finished_decl();

		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_union_add_fields(const Instruction::UnionAddFields& instr) -> Result {
		SymbolProc::UnionInfo& union_info = this->symbol_proc.extra_info.as<SymbolProc::UnionInfo>();

		BaseType::Union& union_type = this->context.type_manager.getUnion(union_info.union_id);

		union_type.fields.reserve(instr.field_types.size());
		for(size_t i = 0; SymbolProc::TypeID field_symbol_proc_type_id : instr.field_types){
			const TypeInfo::VoidableID field_type_id = this->get_type(field_symbol_proc_type_id);

			const AST::UnionDef::Field& ast_field = instr.union_def.fields[union_type.fields.size()];

			if(union_type.isUntagged){
				if(field_type_id.isVoid()){
					this->emit_error(
						Diagnostic::Code::SEMA_UNION_UNTAGGED_WITH_VOID_FIELD,
						ast_field.type,
						"Fields in untagged unions cannot be type \"Void\""
					);
					return Result::ERROR;
				}

				if(this->context.getTypeManager().isTriviallyDeletable(field_type_id.asTypeID()) == false){
					this->emit_error(
						Diagnostic::Code::SEMA_UNION_UNTAGGED_NON_TRIVIALLY_DELETABLE_FIELD,
						ast_field.type,
						"Fields in untagged unions must be trivially deletable"
					);
					return Result::ERROR;
				}

				if(this->context.getTypeManager().isTriviallyCopyable(field_type_id.asTypeID()) == false){
					this->emit_error(
						Diagnostic::Code::SEMA_UNION_UNTAGGED_NON_TRIVIALLY_COPYABLE_FIELD,
						ast_field.type,
						"Fields in untagged unions must be trivially copyable"
					);
					return Result::ERROR;
				}

				if(this->context.getTypeManager().isTriviallyMovable(field_type_id.asTypeID()) == false){
					this->emit_error(
						Diagnostic::Code::SEMA_UNION_UNTAGGED_NON_TRIVIALLY_MOVABLE_FIELD,
						ast_field.type,
						"Fields in untagged unions must be trivially movable"
					);
					return Result::ERROR;
				}
			}

			

			union_type.fields.emplace_back(ast_field.ident, field_type_id);

			const std::string_view ident_str = this->source.getTokenBuffer()[ast_field.ident].getString();
			if(this->add_ident_to_scope(
				ident_str, ast_field.ident, sema::ScopeLevel::UnionFieldFlag{}, ast_field.ident, uint32_t(i)
			).isError()){
				return Result::ERROR;
			}

			i += 1;
		}


		///////////////////////////////////
		// wait on stmts

		bool waiting_on_any = false;
		for(const SymbolProc::ID& member_stmt_id : union_info.stmts){
			SymbolProc& member_stmt = this->context.symbol_proc_manager.getSymbolProc(member_stmt_id);

			switch(member_stmt.waitOnDeclIfNeeded(this->symbol_proc_id, this->context, member_stmt_id)){
				break; case SymbolProc::WaitOnResult::NOT_NEEDED:                 // do nothing
				break; case SymbolProc::WaitOnResult::WAITING:                    waiting_on_any = true;
				break; case SymbolProc::WaitOnResult::WAS_ERRORED:                return Result::ERROR;
				break; case SymbolProc::WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND: return Result::ERROR;
				break; case SymbolProc::WaitOnResult::CIRCULAR_DEP_DETECTED:      return Result::ERROR;
			}
		}

		if(waiting_on_any){
			return Result::NEED_TO_WAIT_BEFORE_NEXT_INSTR;
		}else{
			return Result::SUCCESS;
		}
	}

	auto SemanticAnalyzer::instr_union_def() -> Result {
		SymbolProc::UnionInfo& union_info = this->symbol_proc.extra_info.as<SymbolProc::UnionInfo>();

		BaseType::Union& union_type = this->context.type_manager.getUnion(union_info.union_id);

		union_type.defCompleted = true;


		auto sema_to_pir = SemaToPIR(
			this->context, this->context.constexpr_pir_module, this->context.constexpr_sema_to_pir_data
		);

		sema_to_pir.lowerUnion(union_info.union_id);

		this->propagate_finished_def();

		return Result::SUCCESS;
	}



	auto SemanticAnalyzer::instr_enum_decl(const Instruction::EnumDecl& instr) -> Result {
		BaseType::Primitive::ID underlying_type_id = BaseType::Primitive::ID::dummy();

		if(instr.underlying_type.has_value()){
			const TypeInfo::VoidableID computed_underlying_type = this->get_type(*instr.underlying_type);

			if(this->context.getTypeManager().isIntegral(computed_underlying_type) == false){
				this->emit_error(
					Diagnostic::Code::SEMA_ENUM_INVALID_UNDERLYING_TYPE,
					*instr.enum_def.underlyingType,
					"Invalid underlying type for enum",
					Diagnostic::Info("Note: underlying type for enums must be integral")
				);
				return Result::ERROR;
			}

			underlying_type_id = this->context.getTypeManager()
				.getTypeInfo(computed_underlying_type.asTypeID())
				.baseTypeID()
				.primitiveID();

		}else{
			underlying_type_id =
				this->context.type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_UI_N, 32).primitiveID();
		}


		const evo::Result<EnumAttrs> enum_attrs = 
			this->analyze_enum_attrs(instr.enum_def, instr.attribute_params_info);
		if(enum_attrs.isError()){ return Result::ERROR; }


		///////////////////////////////////
		// create

		SymbolProc::EnumInfo& enum_info = this->symbol_proc.extra_info.as<SymbolProc::EnumInfo>();


		const BaseType::ID created_enum = this->context.type_manager.getOrCreateEnum(
			BaseType::Enum(
				this->source.getID(),
				instr.enum_def.ident,
				evo::SmallVector<BaseType::Enum::Enumerator>(),
				underlying_type_id,
				&enum_info.member_symbols,
				nullptr,
				enum_attrs.value().is_pub
			)
		);

		enum_info.enum_id = created_enum.enumID();


		const std::string_view ident_str = this->source.getTokenBuffer()[instr.enum_def.ident].getString();
		if(this->add_ident_to_scope(ident_str, instr.enum_def, created_enum.enumID()).isError()){
			return Result::ERROR;
		}

		this->context.symbol_proc_manager.addTypeSymbolProc(
			this->context.type_manager.getOrCreateTypeInfo(TypeInfo(created_enum)), this->symbol_proc_id
		);



		///////////////////////////////////
		// setup scope

		this->push_scope_level(nullptr, created_enum.enumID());

		BaseType::Enum& created_enum_ref = this->context.type_manager.getEnum(created_enum.enumID());
		created_enum_ref.scopeLevel = &this->get_current_scope_level();


		for(const SymbolProc::ID& member_stmt_id : enum_info.stmts){
			SymbolProc& member_stmt = this->context.symbol_proc_manager.getSymbolProc(member_stmt_id);

			member_stmt.sema_scope_id = this->context.sema_buffer.scope_manager.copyScope(
				*this->symbol_proc.sema_scope_id
			);
		}


		///////////////////////////////////
		// done

		this->propagate_finished_decl();

		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_enum_add_enumerators(const Instruction::EnumAddEnumerators& instr) -> Result {
		const SymbolProc::EnumInfo& enum_info = this->symbol_proc.extra_info.as<SymbolProc::EnumInfo>();
			
		BaseType::Enum& target_enum = this->context.type_manager.getEnum(enum_info.enum_id);

		const unsigned underlying_bits =
			unsigned(this->context.getTypeManager().numBits(BaseType::ID(target_enum.underlyingTypeID), false));

		const TypeInfo::ID underlying_type_info_id = this->context.type_manager.getOrCreateTypeInfo(
			TypeInfo(BaseType::ID(target_enum.underlyingTypeID))
		);

		auto enumerator_value_counter = core::GenericInt(underlying_bits, 0);
		for(size_t i = 0; const AST::EnumDef::Enumerator& enumerator : instr.enum_def.enumerators){
			if(enumerator.value.has_value()){
				TermInfo& value_term_info = this->get_term_info(*instr.enumerator_values[i]);

				if(value_term_info.value_stage != TermInfo::ValueStage::CONSTEXPR){
					this->emit_error(
						Diagnostic::Code::SEMA_EXPR_NOT_CONSTEXPR,
						*enumerator.value,
						"Enumerator value is not constexpr"
					);
					return Result::ERROR;
				}

				if(this->type_check<true, true>(
					underlying_type_info_id, value_term_info, "Value for enumerator", *enumerator.value
				).ok == false){
					return Result::ERROR;
				}

				const sema::IntValue& int_value =
					this->context.getSemaBuffer().getIntValue(value_term_info.getExpr().intValueID());

				target_enum.enumerators.emplace_back(enumerator.ident, int_value.value.trunc(underlying_bits));
				enumerator_value_counter = 
					int_value.value.trunc(underlying_bits).uadd(core::GenericInt(underlying_bits, 1)).result;

			}else{
				target_enum.enumerators.emplace_back(enumerator.ident, enumerator_value_counter);
				enumerator_value_counter = enumerator_value_counter.uadd(core::GenericInt(underlying_bits, 1)).result;
			}

			i += 1;
		}

		///////////////////////////////////
		// wait on stmts

		bool waiting_on_any = false;
		for(const SymbolProc::ID& member_stmt_id : enum_info.stmts){
			SymbolProc& member_stmt = this->context.symbol_proc_manager.getSymbolProc(member_stmt_id);

			switch(member_stmt.waitOnDeclIfNeeded(this->symbol_proc_id, this->context, member_stmt_id)){
				break; case SymbolProc::WaitOnResult::NOT_NEEDED:                 // do nothing
				break; case SymbolProc::WaitOnResult::WAITING:                    waiting_on_any = true;
				break; case SymbolProc::WaitOnResult::WAS_ERRORED:                return Result::ERROR;
				break; case SymbolProc::WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND: return Result::ERROR;
				break; case SymbolProc::WaitOnResult::CIRCULAR_DEP_DETECTED:      return Result::ERROR;
			}
		}

		if(waiting_on_any){
			return Result::NEED_TO_WAIT_BEFORE_NEXT_INSTR;
		}else{
			return Result::SUCCESS;
		}
	}


	auto SemanticAnalyzer::instr_enum_def() -> Result {
		SymbolProc::EnumInfo& enum_info = this->symbol_proc.extra_info.as<SymbolProc::EnumInfo>();
		BaseType::Enum& enum_type = this->context.type_manager.getEnum(enum_info.enum_id);
		enum_type.defCompleted = true;
		this->propagate_finished_def();

		return Result::SUCCESS;
	}






	auto SemanticAnalyzer::instr_func_decl_extract_deducers_if_needed(
		const Instruction::FuncDeclExtractDeducersIfNeeded& instr
	) -> Result {
		evo::debugAssert(
			this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>().instantiation != nullptr,
			"Should only use this instruction if is a function instantiation"
		);

		const TypeInfo::VoidableID param_type = this->get_type(instr.param_type);
		const TypeInfo& param_type_info = this->context.getTypeManager().getTypeInfo(param_type.asTypeID());
		if(param_type_info.isInterface()){ return Result::SUCCESS; }

		const evo::Result<evo::SmallVector<DeducedTerm>> deducers = this->extract_deducers(
			param_type.asTypeID(),
			*this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>().instantiation_param_arg_types[instr.param_index]
		);
		
		if(deducers.isError()){
			SymbolProc::FuncInfo& func_info = this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>();

			func_info.instantiation->errored_reason = 
				sema::TemplatedFunc::Instantiation::ErroredReasonParamDeductionFailed(instr.param_index);

			this->propagate_finished_decl();
			return Result::FINISHED_EARLY;
		}

		if(this->add_deduced_terms_to_scope(deducers.value()).isError()){ return Result::ERROR; }

		return Result::SUCCESS;
	}


	template<bool IS_INSTANTIATION>
	auto SemanticAnalyzer::instr_func_decl(const Instruction::FuncDecl<IS_INSTANTIATION>& instr) -> Result {
		const evo::Result<FuncAttrs> func_attrs =
			this->analyze_func_attrs(instr.func_def, instr.attribute_params_info);
		if(func_attrs.isError()){ return Result::ERROR; }

		SymbolProc::FuncInfo& func_info = this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>();

		switch(this->source.getTokenBuffer()[instr.func_def.name].kind()){
			case Token::lookupKind("+"):    case Token::lookupKind("+%"):     case Token::lookupKind("+|"):
			case Token::lookupKind("-"):    case Token::lookupKind("-%"):     case Token::lookupKind("-|"):
			case Token::lookupKind("*"):    case Token::lookupKind("*%"):     case Token::lookupKind("*|"):
			case Token::lookupKind("/"):    case Token::lookupKind("%"):      case Token::lookupKind("=="):
			case Token::lookupKind("!="):   case Token::lookupKind("<"):      case Token::lookupKind("<="):
			case Token::lookupKind(">"):    case Token::lookupKind(">="):     case Token::lookupKind("!"):
			case Token::lookupKind("&&"):   case Token::lookupKind("||"):     case Token::lookupKind("<<"):
			case Token::lookupKind("<<|"):  case Token::lookupKind(">>"):     case Token::lookupKind("&"):
			case Token::lookupKind("|"):    case Token::lookupKind("^"):      case Token::lookupKind("~"): {
				// do nothing...
			} break;

			default: {
				if(func_attrs.value().is_commutative){
					this->emit_error(
						Diagnostic::Code::SEMA_INVALID_ATTRIBUTE_USE,
						instr.func_def,
						"This function cannot have attribute #commutative"
					);
					return Result::ERROR;
				}

				if(func_attrs.value().is_swapped){
					this->emit_error(
						Diagnostic::Code::SEMA_INVALID_ATTRIBUTE_USE,
						instr.func_def,
						"This function cannot have attribute #swapped"
					);
					return Result::ERROR;
				}
			} break;
		}


		///////////////////////////////////
		// create func type

		const ASTBuffer& ast_buffer = this->source.getASTBuffer();


		auto params = evo::SmallVector<BaseType::Function::Param>();
		params.reserve(instr.func_def.params.size());

		auto sema_params = evo::SmallVector<sema::Func::Param>();
		sema_params.reserve(instr.func_def.params.size());

		uint32_t min_num_args = 0;
		bool has_in_param = false;

		// only matters if is instantiation
		bool has_template_params = false;
		auto param_is_template = evo::SmallVector<bool>();

		bool has_this_param = false;

		for(size_t i = 0; const std::optional<SymbolProc::TypeID>& symbol_proc_param_type_id : instr.params()){
			EVO_DEFER([&](){ i += 1; });

			const AST::FuncDef::Param& param = instr.func_def.params[i];
			
			evo::debugAssert(
				symbol_proc_param_type_id.has_value() == (param.name.kind() != AST::Kind::THIS),
				"[this] is the only must not have a type, and everything else must have a type"
			);


			const BaseType::Function::Param::Kind type_param_kind = [&](){
				switch(param.kind){
					case AST::FuncDef::Param::Kind::READ: return BaseType::Function::Param::Kind::READ;
					case AST::FuncDef::Param::Kind::MUT:  return BaseType::Function::Param::Kind::MUT;
					case AST::FuncDef::Param::Kind::IN:   return BaseType::Function::Param::Kind::IN;
				}
				evo::debugFatalBreak("Unknown ast param kind");
			}();


			if(symbol_proc_param_type_id.has_value()){ // regular param
				const TypeInfo::VoidableID param_type_id = this->get_type(*symbol_proc_param_type_id);

				if(param_type_id.isVoid()){
					this->emit_error(
						Diagnostic::Code::SEMA_PARAM_TYPE_VOID, *param.type, "Function parameter cannot be type `Void`"
					);
					return Result::ERROR;
				}


				const TypeInfo& param_type_info = this->context.getTypeManager().getTypeInfo(param_type_id.asTypeID());

				if(param_type_info.isUninitPointer()){
					this->emit_error(
						Diagnostic::Code::SEMA_PARAM_TYPE_UNINIT_PTR,
						*param.type,
						"Function parameter cannot be type uninitialized pointer"
					);
					return Result::ERROR;
				}


				// TODO(PERF): only calculate these as needed
				const bool param_type_is_interface = param_type_info.isInterface();
				const bool param_type_is_deducer =
					this->context.getTypeManager().isTypeDeducer(param_type_id.asTypeID());


				if(func_info.instantiation == nullptr){
					if(param_type_is_interface || param_type_is_deducer){
						has_template_params = true;
						param_is_template.emplace_back(true);
						continue;
					}else{
						param_is_template.emplace_back(false);

						if(has_template_params){ continue; }
					}

				}else{
					if(param_type_is_interface){
						const BaseType::Interface& param_interface = this->context.getTypeManager().getInterface(
							param_type_info.baseTypeID().interfaceID()
						);

						const TypeInfo& arg_type_info = this->context.getTypeManager().getTypeInfo(
							*func_info.instantiation_param_arg_types[i - size_t(has_this_param)]
						);

						if(arg_type_info.qualifiers().empty() == false){
							func_info.instantiation->errored_reason = 
								sema::TemplatedFunc::Instantiation::ErroredReasonArgTypeMismatch{
									i,
									param_type_id.asTypeID(),
									*func_info.instantiation_param_arg_types[i - size_t(has_this_param)]
								};

							this->propagate_finished_decl();
							return Result::FINISHED_EARLY;
						}

						const auto lock = std::scoped_lock(param_interface.implsLock);
						if(param_interface.impls.contains(arg_type_info.baseTypeID()) == false){
							func_info.instantiation->errored_reason = 
								sema::TemplatedFunc::Instantiation::ErroredReasonTypeDoesntImplInterface{
									i,
									param_type_id.asTypeID(),
									*func_info.instantiation_param_arg_types[i - size_t(has_this_param)]
								};

							this->propagate_finished_decl();
							return Result::FINISHED_EARLY;
						}
					}
				}



				// saving types to check if param should be copy
				// 	(need to do later after definitions of types are gotten)
				if(param.kind == AST::FuncDef::Param::Kind::READ){
					if(param_type_is_interface || param_type_is_deducer){
						func_info.param_type_to_check_if_is_copy.emplace_back(
							*func_info.instantiation_param_arg_types[i - size_t(has_this_param)]
						);
					}else{
						func_info.param_type_to_check_if_is_copy.emplace_back(param_type_id.asTypeID());
					}
					
				}else{
					func_info.param_type_to_check_if_is_copy.emplace_back();
				}



				if(param.kind == AST::FuncDef::Param::Kind::IN){
					has_in_param = true;
				}


				if(param_type_is_interface){
					const TypeInfo::ID arg_type = *func_info.instantiation_param_arg_types[i - size_t(has_this_param)];

					const BaseType::ID interface_impl_instantiation_id = 
						this->context.type_manager.getOrCreateInterfaceImplInstantiation(
							BaseType::InterfaceImplInstantiation(
								param_type_info.baseTypeID().interfaceID(), arg_type
							)
						);

					const TypeInfo::ID impl_arg_type = this->context.type_manager.getOrCreateTypeInfo(
						TypeInfo(interface_impl_instantiation_id)
					);

					params.emplace_back(impl_arg_type, type_param_kind, false);

				}else if(param_type_is_deducer){
					params.emplace_back(
						*func_info.instantiation_param_arg_types[i - size_t(has_this_param)], type_param_kind, false
					);

				}else{
					params.emplace_back(param_type_id.asTypeID(), type_param_kind, false);
				}

				if(instr.default_param_values[i - size_t(has_this_param)].has_value()){
					TermInfo default_param_value =
						this->get_term_info(*instr.default_param_values[i - size_t(has_this_param)]);

					if(
						this->type_check<true, true>(
							param_type_id.asTypeID(),
							default_param_value,
							"Default value of function parameter",
							*instr.func_def.params[i].defaultValue
						).ok == false
					){
						return Result::ERROR;
					}

					sema_params.emplace_back(ast_buffer.getIdent(param.name), default_param_value.getExpr());


				}else{
					sema_params.emplace_back(ast_buffer.getIdent(param.name), std::nullopt);
					min_num_args += 1;
				}

			}else{ // [this] param
				has_this_param = true;

				const std::optional<sema::ScopeManager::Scope::ObjectScope> current_type_scope = 
					this->scope.getCurrentTypeScopeIfExists();

				if(current_type_scope.has_value() == false){
					// TODO(FUTURE): better messaging
					this->emit_error(
						Diagnostic::Code::SEMA_INVALID_SCOPE_FOR_THIS_PARAM,
						param.name,
						"[this] parameters are only valid inside type scope"
					);
					return Result::ERROR;
				}

				if(i != 0){
					this->emit_error(
						Diagnostic::Code::SEMA_THIS_PARAM_NOT_FIRST,
						param.name,
						instr.params()[0].has_value()
							? "[this] parameters must be the first parameter"
							: "Cannot have multiple [this] parameters"
					);
					return Result::ERROR;
				}


				current_type_scope->visit([&](const auto& type_scope) -> void {
					using TypeScope = std::decay_t<decltype(type_scope)>;

					if constexpr(
						std::is_same<TypeScope, BaseType::Struct::ID>() 
						|| std::is_same<TypeScope, BaseType::Union::ID>()
						|| std::is_same<TypeScope, BaseType::Enum::ID>()
						|| std::is_same<TypeScope, BaseType::Interface::ID>()
					){
						const TypeInfo::ID this_type = this->context.type_manager.getOrCreateTypeInfo(
							TypeInfo(BaseType::ID(type_scope))
						);
						params.emplace_back(this_type, type_param_kind, false);
					}else{
						evo::debugFatalBreak("Invalid type object scope");
					}
				});

				sema_params.emplace_back(ast_buffer.getThis(param.name), std::nullopt);
				min_num_args += 1;

				func_info.param_type_to_check_if_is_copy.emplace_back();
			}
		}


		if(func_info.instantiation == nullptr && has_template_params){
			for(const AST::FuncDef::Param& param : instr.func_def.params){
				if(param.defaultValue.has_value()){
					this->emit_error(
						Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
						*param.defaultValue,
						"Template functions with default parameter values is unimplemented"
					);
					return Result::ERROR;
				}
			}


			const sema::TemplatedFunc::ID new_templated_func = 
				this->context.sema_buffer.createTemplatedFunc(
					this->symbol_proc,
					0,
					evo::SmallVector<sema::TemplatedFunc::TemplateParam>(),
					std::move(param_is_template)
				);

			const Token& name_token = this->source.getTokenBuffer()[instr.func_def.name];

			if(this->add_ident_to_scope(
				name_token.getString(), instr.func_def, new_templated_func
			).isError()){
				return Result::ERROR;
			}

			this->propagate_finished_decl_def();
			return Result::FINISHED_EARLY;
		}


		auto return_params = evo::SmallVector<BaseType::Function::ReturnParam>();
		for(size_t i = 0; const SymbolProc::TypeID& symbol_proc_return_param_type_id : instr.returns()){
			EVO_DEFER([&](){ i += 1; });

			const TypeInfo::VoidableID type_id = this->get_type(symbol_proc_return_param_type_id);

			const AST::FuncDef::Return& ast_return_param = instr.func_def.returns[i];

			if(i == 0){
				if(type_id.isVoid() && ast_return_param.ident.has_value()){
					this->emit_error(
						Diagnostic::Code::SEMA_NAMED_VOID_RETURN,
						*ast_return_param.ident,
						"A function return parameter that is type `Void` cannot be named"
					);
					return Result::ERROR;
				}
			}else{
				if(type_id.isVoid()){
					this->emit_error(
						Diagnostic::Code::SEMA_NOT_FIRST_RETURN_VOID,
						ast_return_param.type,
						"Only the first function return parameter can be type `Void`"
					);
					return Result::ERROR;
				}
			}

			return_params.emplace_back(ast_return_param.ident, type_id);
		}


		auto error_return_params = evo::SmallVector<BaseType::Function::ReturnParam>();
		for(size_t i = 0; const SymbolProc::TypeID& symbol_proc_error_return_param_type_id : instr.errorReturns()){
			EVO_DEFER([&](){ i += 1; });

			const TypeInfo::VoidableID type_id = this->get_type(symbol_proc_error_return_param_type_id);

			const AST::FuncDef::Return& ast_error_return_param = instr.func_def.errorReturns[i];

			if(i == 0){
				if(type_id.isVoid() && ast_error_return_param.ident.has_value()){
					this->emit_error(
						Diagnostic::Code::SEMA_NAMED_VOID_RETURN,
						*ast_error_return_param.ident,
						"A function error return parameter that is type `Void` cannot be named"
					);
					return Result::ERROR;
				}
			}else{
				if(type_id.isVoid()){
					this->emit_error(
						Diagnostic::Code::SEMA_NOT_FIRST_RETURN_VOID,
						ast_error_return_param.type,
						"Only the first function error return parameter can be type `Void`"
					);
					return Result::ERROR;
				}
			}

			error_return_params.emplace_back(ast_error_return_param.ident, type_id);
		}



		///////////////////////////////////
		// create func

		const BaseType::ID created_func_base_type = this->context.type_manager.getOrCreateFunction(
			BaseType::Function(std::move(params), std::move(return_params), std::move(error_return_params))
		);


		const bool is_constexpr = !func_attrs.value().is_runtime;

		const sema::Func::ID created_func_id = this->context.sema_buffer.createFunc(
			this->source.getID(),
			instr.func_def.name,
			std::string(),
			created_func_base_type.funcID(),
			std::move(sema_params),
			this->symbol_proc_id,
			min_num_args,
			func_attrs.value().is_pub,
			is_constexpr,
			func_attrs.value().is_export,
			has_in_param,
			instr.instantiation_id
		);

		if(func_attrs.value().is_entry){
			this->context.entry = created_func_id;
		}


		if(func_info.instantiation != nullptr){
			func_info.instantiation->funcID = created_func_id;
		}


		sema::Func& created_func = this->context.sema_buffer.funcs[created_func_id];

		if constexpr(IS_INSTANTIATION == false){
			const Token& name_token = this->source.getTokenBuffer()[instr.func_def.name];

			switch(name_token.kind()){
				case Token::Kind::IDENT: {
					const std::string_view ident_str = name_token.getString();
					if(this->add_ident_to_scope(ident_str, instr.func_def, created_func_id, this->context).isError()){
						return Result::ERROR;
					}
				} break;

				case Token::Kind::KEYWORD_AS: {
					if(this->scope.inObjectScope() == false){
						this->emit_error(
							Diagnostic::Code::SEMA_OPERATOR_OVERLOAD_NOT_IN_TYPE,
							instr.func_def,
							"Operator overload cannot be a free function"
						);
						return Result::ERROR;
					}


					if(this->scope.getCurrentObjectScope().is<BaseType::Struct::ID>() == false){
						this->emit_error(
							Diagnostic::Code::SEMA_OPERATOR_OVERLOAD_NOT_IN_TYPE,
							instr.func_def,
							"Operator overload cannot be a free function"
						);
						return Result::ERROR;
					}



					if(created_func.params.size() != 1){
						if(created_func.params.size() > 1){
							if(
								this->source.getTokenBuffer()[created_func.params[0].ident.as<Token::ID>()].kind()
								== Token::Kind::KEYWORD_THIS
							){
								this->emit_error(
									Diagnostic::Code::SEMA_INVALID_OPERATOR_AS_OVERLOAD,
									created_func.params[1].ident.as<Token::ID>(),
									"Operator [as] overload can only have a [this] parameter"
								);
							}else{
								this->emit_error(
									Diagnostic::Code::SEMA_INVALID_OPERATOR_AS_OVERLOAD,
									created_func.params[0].ident.as<Token::ID>(),
									"Operator [as] overload can only have a [this] parameter"
								);
							}
						}else{
							this->emit_error(
								Diagnostic::Code::SEMA_INVALID_OPERATOR_AS_OVERLOAD,
								created_func_id,
								"Operator [as] overload must have a [this] parameter"
							);
						}

						return Result::ERROR;
					}

					if(
						this->source.getTokenBuffer()[created_func.params[0].ident.as<Token::ID>()].kind()
						!= Token::Kind::KEYWORD_THIS
					){
						this->emit_error(
							Diagnostic::Code::SEMA_INVALID_OPERATOR_AS_OVERLOAD,
							created_func.params[1].ident.as<Token::ID>(),
							"Operator [as] overload can only have a [this] parameter"
						);
						return Result::ERROR;
					}

					const BaseType::Function& created_func_type =
						this->context.getTypeManager().getFunction(created_func_base_type.funcID());

					if(created_func_type.returnParams.size() != 1){
						this->emit_error(
							Diagnostic::Code::SEMA_INVALID_OPERATOR_AS_OVERLOAD,
							created_func_type.returnParams[1].ident.value_or(created_func.name.as<Token::ID>()),
							"Operator [as] overload can only have single return"
						);
						return Result::ERROR;
					}

					if(created_func_type.returnParams[0].typeID.isVoid()){
						this->emit_error(
							Diagnostic::Code::SEMA_INVALID_OPERATOR_AS_OVERLOAD,
							created_func_type.returnParams[1].ident.value_or(created_func.name.as<Token::ID>()),
							"Operator [as] overload must return a value"
						);
						return Result::ERROR;
					}


					if(created_func_type.errorParams.empty() == false){
						this->emit_error(
							Diagnostic::Code::SEMA_INVALID_OPERATOR_AS_OVERLOAD,
							created_func_type.errorParams[0].ident.value_or(created_func.name.as<Token::ID>()),
							"Operator [as] overload cannot error"
						);
						return Result::ERROR;
					}


					BaseType::Struct& current_struct = this->context.type_manager.getStruct(
						this->scope.getCurrentObjectScope().as<BaseType::Struct::ID>()
					);

					const TypeInfo::ID conversion_type = created_func_type.returnParams[0].typeID.asTypeID();

					const auto lock = std::scoped_lock(current_struct.operatorAsOverloadsLock);

					const auto find = current_struct.operatorAsOverloads.find(conversion_type);
					if(find != current_struct.operatorAsOverloads.end()){
						this->emit_error(
							Diagnostic::Code::SEMA_INVALID_OPERATOR_AS_OVERLOAD,
							created_func_type.returnParams[0].ident.value_or(created_func.name.as<Token::ID>()),
							"Operator [as] overload for this type already defined",
							Diagnostic::Info("Defined here:", this->get_location(find->second))
						);
						return Result::ERROR;
					}

					current_struct.operatorAsOverloads.emplace(conversion_type, created_func_id);
				} break;


				case Token::Kind::KEYWORD_NEW: {
					if(this->scope.inObjectScope() == false){
						this->emit_error(
							Diagnostic::Code::SEMA_OPERATOR_OVERLOAD_NOT_IN_TYPE,
							instr.func_def,
							"Operator overload cannot be a free function"
						);
						return Result::ERROR;
					}

					if(this->scope.getCurrentObjectScope().is<BaseType::Struct::ID>() == false){
						this->emit_error(
							Diagnostic::Code::SEMA_OPERATOR_OVERLOAD_NOT_IN_TYPE,
							instr.func_def,
							"Operator overload cannot be a free function"
						);
						return Result::ERROR;
					}

					const BaseType::Function& created_func_type =
						this->context.getTypeManager().getFunction(created_func_base_type.funcID());

					BaseType::Struct& current_struct = this->context.type_manager.getStruct(
						this->scope.getCurrentObjectScope().as<BaseType::Struct::ID>()
					);

					if(
						created_func.params.empty() == false
						&& this->source.getTokenBuffer()[created_func.params[0].ident.as<Token::ID>()].kind()
							== Token::Kind::KEYWORD_THIS
					){ // assignment
						if(created_func_type.returnsVoid() == false){
							this->emit_error(
								Diagnostic::Code::SEMA_INVALID_OPERATOR_NEW_OVERLOAD,
								instr.func_def,
								"Assignment operator `new` cannot have any return values"
							);
							return Result::ERROR;
						}

						{
							const auto lock = std::scoped_lock(current_struct.newAssignOverloadsLock);

							for(sema::Func::ID new_reassign_overload_id : current_struct.newAssignOverloads){
								const sema::Func& new_reassign_overload =
									this->context.getSemaBuffer().getFunc(new_reassign_overload_id);

								if(new_reassign_overload.isEquivalentOverload(created_func, this->context)){
									this->emit_error(
										Diagnostic::Code::SEMA_INVALID_OPERATOR_NEW_OVERLOAD,
										instr.func_def,
										"Assignment operator [new] overload has an overload "
											"that collides with this declaration",
										Diagnostic::Info(
											"First defined here:", this->get_location(new_reassign_overload)
										)
									);
									return Result::ERROR;
								}
							}

							current_struct.newAssignOverloads.emplace_back(created_func_id);
						}

					}else{ // initialization
						const TypeInfo::ID expected_return_type = this->context.type_manager.getOrCreateTypeInfo(
							TypeInfo(
								BaseType::ID(this->scope.getCurrentObjectScope().as<BaseType::Struct::ID>())
							)
						);

						if(
							created_func_type.returnParams.size() != 1
							|| created_func_type.returnParams[0].typeID != expected_return_type
						){
							this->emit_error(
								Diagnostic::Code::SEMA_INVALID_OPERATOR_NEW_OVERLOAD,
								instr.func_def,
								"Initialization operator `new` must return the newly created value"
							);
							return Result::ERROR;
						}

						if(created_func_type.hasNamedReturns() == false){
							this->emit_error(
								Diagnostic::Code::SEMA_INVALID_OPERATOR_NEW_OVERLOAD,
								instr.func_def,
								"Initialization operator `new` must have a named return"
							);
							return Result::ERROR;
						}

						{
							const auto lock = std::scoped_lock(current_struct.newInitOverloadsLock);

							for(sema::Func::ID new_init_overload_id : current_struct.newInitOverloads){
								const sema::Func& new_init_overload =
									this->context.getSemaBuffer().getFunc(new_init_overload_id);

								if(new_init_overload.isEquivalentOverload(created_func, this->context)){
									this->emit_error(
										Diagnostic::Code::SEMA_INVALID_OPERATOR_NEW_OVERLOAD,
										instr.func_def,
										"Initialization operator [new] overload has an overload "
											"that collides with this declaration",
										Diagnostic::Info(
											"First defined here:", this->get_location(new_init_overload)
										)
									);
									return Result::ERROR;
								}
							}

							current_struct.newInitOverloads.emplace_back(created_func_id);
						}
					}
				} break;

				case Token::Kind::KEYWORD_DELETE: {
					if(this->scope.inObjectScope() == false){
						this->emit_error(
							Diagnostic::Code::SEMA_OPERATOR_OVERLOAD_NOT_IN_TYPE,
							instr.func_def,
							"Operator overload cannot be a free function"
						);
						return Result::ERROR;
					}

					if(this->scope.getCurrentObjectScope().is<BaseType::Struct::ID>() == false){
						this->emit_error(
							Diagnostic::Code::SEMA_OPERATOR_OVERLOAD_NOT_IN_TYPE,
							instr.func_def,
							"Operator overload cannot be a free function"
						);
						return Result::ERROR;
					}


					if(created_func.params.size() != 1){
						if(created_func.params.empty()){
							this->emit_error(
								Diagnostic::Code::SEMA_INVALID_OPERATOR_DELETE_OVERLOAD,
								instr.func_def,
								"Operator [delete] overload must have a [this] parameter"
							);
						}else{
							this->emit_error(
								Diagnostic::Code::SEMA_INVALID_OPERATOR_DELETE_OVERLOAD,
								instr.func_def.params[1],
								"Operator [delete] overload can only have a [this] parameter"
							);
						}
						return Result::ERROR;
					}

					if(
						this->source.getTokenBuffer()[created_func.params[0].ident.as<Token::ID>()].kind()
						!= Token::Kind::KEYWORD_THIS
					){
						this->emit_error(
							Diagnostic::Code::SEMA_INVALID_OPERATOR_DELETE_OVERLOAD,
							instr.func_def.params[0],
							"Operator [delete] can only have a [this] parameter"
						);
						return Result::ERROR;
					}


					const BaseType::Function& created_func_type =
						this->context.getTypeManager().getFunction(created_func_base_type.funcID());

					BaseType::Struct& current_struct = this->context.type_manager.getStruct(
						this->scope.getCurrentObjectScope().as<BaseType::Struct::ID>()
					);


					if(created_func_type.returnsVoid() == false){
						if(created_func_type.returnParams[0].ident.has_value()){
							this->emit_error(
								Diagnostic::Code::SEMA_INVALID_OPERATOR_DELETE_OVERLOAD,
								*created_func_type.returnParams[0].ident,
								"Operator [delete] overload must return `Void`"
							);
						}else{
							this->emit_error(
								Diagnostic::Code::SEMA_INVALID_OPERATOR_DELETE_OVERLOAD,
								instr.func_def.returns[0].type,
								"Operator [delete] overload must return `Void`"
							);
						}
						return Result::ERROR;
					}

					if(created_func_type.hasErrorReturn()){
						if(created_func_type.errorParams[0].ident.has_value()){
							this->emit_error(
								Diagnostic::Code::SEMA_INVALID_OPERATOR_DELETE_OVERLOAD,
								*created_func_type.errorParams[0].ident,
								"Operator [delete] cannot error"
							);
						}else{
							this->emit_error(
								Diagnostic::Code::SEMA_INVALID_OPERATOR_DELETE_OVERLOAD,
								instr.func_def.errorReturns[0].type,
								"Operator [delete] cannot error"
							);
						}
						return Result::ERROR;
					}


					auto expected = std::optional<sema::Func::ID>();
					if(current_struct.deleteOverload.compare_exchange_strong(expected, created_func_id) == false){
						this->emit_error(
							Diagnostic::Code::SEMA_INVALID_OPERATOR_DELETE_OVERLOAD,
							instr.func_def,
							"Operator [delete] was already defined for this type",
							Diagnostic::Info(
								"First defined here:", this->get_location(*current_struct.deleteOverload.load())
							)
						);
						return Result::ERROR;
					}
				} break;

				case Token::Kind::KEYWORD_COPY: {
					if(this->scope.inObjectScope() == false){
						this->emit_error(
							Diagnostic::Code::SEMA_OPERATOR_OVERLOAD_NOT_IN_TYPE,
							instr.func_def,
							"Operator overload cannot be a free function"
						);
						return Result::ERROR;
					}

					if(this->scope.getCurrentObjectScope().is<BaseType::Struct::ID>() == false){
						this->emit_error(
							Diagnostic::Code::SEMA_OPERATOR_OVERLOAD_NOT_IN_TYPE,
							instr.func_def,
							"Operator overload cannot be a free function"
						);
						return Result::ERROR;
					}

					const BaseType::Function& created_func_type =
						this->context.getTypeManager().getFunction(created_func_base_type.funcID());

					BaseType::Struct& current_struct = this->context.type_manager.getStruct(
						this->scope.getCurrentObjectScope().as<BaseType::Struct::ID>()
					);


					const TypeInfo::ID struct_type_info_id = this->context.type_manager.getOrCreateTypeInfo(
						TypeInfo(BaseType::ID(this->scope.getCurrentObjectScope().as<BaseType::Struct::ID>()))
					);


					switch(created_func.params.size()){
						case 0: {
							this->emit_error(
								Diagnostic::Code::SEMA_INVALID_OPERATOR_COPY_OVERLOAD,
								instr.func_def,
								"Operator [copy] overload must have a [this] parameter"
							);
							return Result::ERROR;
						} break;

						case 1: {
							if(
								this->source.getTokenBuffer()[created_func.params[0].ident.as<Token::ID>()].kind()
								!= Token::Kind::KEYWORD_THIS
							){
								this->emit_error(
									Diagnostic::Code::SEMA_INVALID_OPERATOR_COPY_OVERLOAD,
									instr.func_def.params[0],
									"Operator [copy] overload must have a [this] parameter"
								);
								return Result::ERROR;
							}

							if(created_func_type.returnParams.size() != 1){
								if(created_func_type.returnParams.empty()){
									this->emit_error(
										Diagnostic::Code::SEMA_INVALID_OPERATOR_COPY_OVERLOAD,
										instr.func_def,
										"Operator [copy] initialization overload must return the newly created value"
									);
								}else{
									this->emit_error(
										Diagnostic::Code::SEMA_INVALID_OPERATOR_COPY_OVERLOAD,
										instr.func_def.returns[1],
										"Operator [copy] initialization overload must return the newly created value"
									);
								}
								return Result::ERROR;
							}


							if(created_func_type.returnParams[0].typeID != struct_type_info_id){
								this->emit_error(
									Diagnostic::Code::SEMA_INVALID_OPERATOR_COPY_OVERLOAD,
									instr.func_def.returns[0].type,
									"Operator [copy] initialization overload must return `This` (or equivalent type)"
								);
								return Result::ERROR;
							}


							if(created_func_type.errorParams.empty() == false){
								this->emit_error(
									Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
									instr.func_def.errorReturns[0],
									"Erroring operator [copy] is unimplemented"
								);
								return Result::ERROR;
							}

							auto expected = BaseType::Struct::DeletableOverload();
							if(current_struct.copyInitOverload.compare_exchange_strong(
								expected, BaseType::Struct::DeletableOverload(created_func_id, false)
							) == false){
								if(expected.wasDeleted){
									this->emit_error(
										Diagnostic::Code::SEMA_INVALID_OPERATOR_COPY_OVERLOAD,
										instr.func_def,
										"Operator [copy] can not be defined for this type as it was explicitly deleted"
									);

								}else{
									this->emit_error(
										Diagnostic::Code::SEMA_INVALID_OPERATOR_COPY_OVERLOAD,
										instr.func_def,
										"Operator [copy] initialization was already defined for this type",
										Diagnostic::Info(
											"First defined here:",
											this->get_location(*current_struct.copyInitOverload.load().funcID)
										)
									);
								}

								return Result::ERROR;
							}
						} break;

						case 2: {
							if(
								this->source.getTokenBuffer()[created_func.params[0].ident.as<Token::ID>()].kind()
								!= Token::Kind::KEYWORD_THIS
							){
								this->emit_error(
									Diagnostic::Code::SEMA_INVALID_OPERATOR_COPY_OVERLOAD,
									instr.func_def.params[0],
									"Operator [copy] overload must have a [this] parameter"
								);
								return Result::ERROR;
							}

							if(created_func_type.params[1].typeID != struct_type_info_id){
								this->emit_error(
									Diagnostic::Code::SEMA_INVALID_OPERATOR_COPY_OVERLOAD,
									instr.func_def.params[1],
									"Argument index 1 of operator [copy] assignment overload must be `This` "
										"(or equivalent type)"
								);
								return Result::ERROR;
							}

							if(created_func_type.returnsVoid() == false){
								this->emit_error(
									Diagnostic::Code::SEMA_INVALID_OPERATOR_COPY_OVERLOAD,
									instr.func_def.returns[0],
									"Operator [copy] assignment overload cannot return any values"
								);
								return Result::ERROR;
							}

							if(created_func_type.errorParams.empty() == false){
								this->emit_error(
									Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
									instr.func_def.errorReturns[0],
									"Erroring operator [copy] is unimplemented"
								);
								return Result::ERROR;
							}

							auto expected = std::optional<sema::FuncID>();
							if(current_struct.copyAssignOverload.compare_exchange_strong(
								expected, created_func_id
							) == false){
								this->emit_error(
									Diagnostic::Code::SEMA_INVALID_OPERATOR_COPY_OVERLOAD,
									instr.func_def,
									"Operator [copy] assignment was already defined for this type",
									Diagnostic::Info(
										"First defined here:",
										this->get_location(*current_struct.copyAssignOverload.load())
									)
								);
								return Result::ERROR;
							}

							if(current_struct.copyInitOverload.load().wasDeleted){
								this->emit_error(
									Diagnostic::Code::SEMA_INVALID_OPERATOR_COPY_OVERLOAD,
									instr.func_def,
									"Operator [copy] can not be defined for this type as it was explicitly deleted"
								);
								return Result::ERROR;
							}
						} break;

						default: {
							this->emit_error(
								Diagnostic::Code::SEMA_INVALID_OPERATOR_COPY_OVERLOAD,
								instr.func_def.params[2],
								"Too many arguments for operator [copy] overload"
							);
							return Result::ERROR;
						} break;
					}
				} break;

				case Token::Kind::KEYWORD_MOVE: {
					if(this->scope.inObjectScope() == false){
						this->emit_error(
							Diagnostic::Code::SEMA_OPERATOR_OVERLOAD_NOT_IN_TYPE,
							instr.func_def,
							"Operator overload cannot be a free function"
						);
						return Result::ERROR;
					}

					if(this->scope.getCurrentObjectScope().is<BaseType::Struct::ID>() == false){
						this->emit_error(
							Diagnostic::Code::SEMA_OPERATOR_OVERLOAD_NOT_IN_TYPE,
							instr.func_def,
							"Operator overload cannot be a free function"
						);
						return Result::ERROR;
					}

					const BaseType::Function& created_func_type =
						this->context.getTypeManager().getFunction(created_func_base_type.funcID());

					BaseType::Struct& current_struct = this->context.type_manager.getStruct(
						this->scope.getCurrentObjectScope().as<BaseType::Struct::ID>()
					);


					const TypeInfo::ID struct_type_info_id = this->context.type_manager.getOrCreateTypeInfo(
						TypeInfo(BaseType::ID(this->scope.getCurrentObjectScope().as<BaseType::Struct::ID>()))
					);


					switch(created_func.params.size()){
						case 0: {
							this->emit_error(
								Diagnostic::Code::SEMA_INVALID_OPERATOR_MOVE_OVERLOAD,
								instr.func_def,
								"Operator [move] overload must have a [this] parameter"
							);
							return Result::ERROR;
						} break;

						case 1: {
							if(
								this->source.getTokenBuffer()[created_func.params[0].ident.as<Token::ID>()].kind()
								!= Token::Kind::KEYWORD_THIS
							){
								this->emit_error(
									Diagnostic::Code::SEMA_INVALID_OPERATOR_MOVE_OVERLOAD,
									instr.func_def.params[0],
									"Operator [move] overload must have a [this] parameter"
								);
								return Result::ERROR;
							}

							if(created_func_type.returnParams.size() != 1){
								if(created_func_type.returnParams.empty()){
									this->emit_error(
										Diagnostic::Code::SEMA_INVALID_OPERATOR_MOVE_OVERLOAD,
										instr.func_def,
										"Operator [move] initialization overload must return the newly created value"
									);
								}else{
									this->emit_error(
										Diagnostic::Code::SEMA_INVALID_OPERATOR_MOVE_OVERLOAD,
										instr.func_def.returns[1],
										"Operator [move] initialization overload must return the newly created value"
									);
								}
								return Result::ERROR;
							}


							if(created_func_type.returnParams[0].typeID != struct_type_info_id){
								this->emit_error(
									Diagnostic::Code::SEMA_INVALID_OPERATOR_MOVE_OVERLOAD,
									instr.func_def.returns[0].type,
									"Operator [move] initialization overload must return `This` (or equivalent type)"
								);
								return Result::ERROR;
							}


							if(created_func_type.errorParams.empty() == false){
								this->emit_error(
									Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
									instr.func_def.errorReturns[0],
									"Erroring operator [move] is unimplemented"
								);
								return Result::ERROR;
							}

							auto expected = BaseType::Struct::DeletableOverload();
							if(current_struct.moveInitOverload.compare_exchange_strong(
								expected, BaseType::Struct::DeletableOverload(created_func_id, false)
							) == false){
								if(expected.wasDeleted){
									this->emit_error(
										Diagnostic::Code::SEMA_INVALID_OPERATOR_MOVE_OVERLOAD,
										instr.func_def,
										"Operator [move] can not be defined for this type as it was explicitly deleted"
									);

								}else{
									this->emit_error(
										Diagnostic::Code::SEMA_INVALID_OPERATOR_MOVE_OVERLOAD,
										instr.func_def,
										"Operator [move] initialization was already defined for this type",
										Diagnostic::Info(
											"First defined here:",
											this->get_location(*current_struct.moveInitOverload.load().funcID)
										)
									);
								}

								return Result::ERROR;
							}
						} break;

						case 2: {
							if(
								this->source.getTokenBuffer()[created_func.params[0].ident.as<Token::ID>()].kind()
								!= Token::Kind::KEYWORD_THIS
							){
								this->emit_error(
									Diagnostic::Code::SEMA_INVALID_OPERATOR_MOVE_OVERLOAD,
									instr.func_def.params[0],
									"Operator [move] overload must have a [this] parameter"
								);
								return Result::ERROR;
							}

							if(created_func_type.params[1].typeID != struct_type_info_id){
								this->emit_error(
									Diagnostic::Code::SEMA_INVALID_OPERATOR_MOVE_OVERLOAD,
									instr.func_def.params[1],
									"Argument index 1 of operator [move] assignment overload must be `This` "
										"(or equivalent type)"
								);
								return Result::ERROR;
							}

							if(created_func_type.returnsVoid() == false){
								this->emit_error(
									Diagnostic::Code::SEMA_INVALID_OPERATOR_MOVE_OVERLOAD,
									instr.func_def.returns[0],
									"Operator [move] assignment overload cannot return any values"
								);
								return Result::ERROR;
							}

							if(created_func_type.errorParams.empty() == false){
								this->emit_error(
									Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
									instr.func_def.errorReturns[0],
									"Erroring operator [move] is unimplemented"
								);
								return Result::ERROR;
							}

							auto expected = std::optional<sema::FuncID>();
							if(current_struct.moveAssignOverload.compare_exchange_strong(
								expected, created_func_id
							) == false){
								this->emit_error(
									Diagnostic::Code::SEMA_INVALID_OPERATOR_MOVE_OVERLOAD,
									instr.func_def,
									"Operator [move] assignment was already defined for this type",
									Diagnostic::Info(
										"First defined here:",
										this->get_location(*current_struct.moveAssignOverload.load())
									)
								);
								return Result::ERROR;
							}

							if(current_struct.moveInitOverload.load().wasDeleted){
								this->emit_error(
									Diagnostic::Code::SEMA_INVALID_OPERATOR_MOVE_OVERLOAD,
									instr.func_def,
									"Operator [move] can not be defined for this type as it was explicitly deleted"
								);
								return Result::ERROR;
							}
						} break;

						default: {
							this->emit_error(
								Diagnostic::Code::SEMA_INVALID_OPERATOR_MOVE_OVERLOAD,
								instr.func_def.params[2],
								"Too many arguments for operator [move] overload"
							);
							return Result::ERROR;
						} break;
					}
				} break;

				case Token::lookupKind("+"):    case Token::lookupKind("+%"):  case Token::lookupKind("+|"):
				case Token::lookupKind("-"):    case Token::lookupKind("-%"):  case Token::lookupKind("-|"):
				case Token::lookupKind("*"):    case Token::lookupKind("*%"):  case Token::lookupKind("*|"):
				case Token::lookupKind("/"):    case Token::lookupKind("%"):   case Token::lookupKind("=="):
				case Token::lookupKind("!="):   case Token::lookupKind("<"):   case Token::lookupKind("<="):
				case Token::lookupKind(">"):    case Token::lookupKind(">="):  case Token::lookupKind("&&"):
				case Token::lookupKind("||"):   case Token::lookupKind("<<"):  case Token::lookupKind("<<|"):
				case Token::lookupKind(">>"):   case Token::lookupKind("&"):   case Token::lookupKind("|"):
				case Token::lookupKind("^"):    case Token::lookupKind("~"):

				case Token::lookupKind("+="):   case Token::lookupKind("+%="): case Token::lookupKind("+|="):
				case Token::lookupKind("-="):   case Token::lookupKind("-%="): case Token::lookupKind("-|="):
				case Token::lookupKind("*="):   case Token::lookupKind("*%="): case Token::lookupKind("*|="):
				case Token::lookupKind("/="):   case Token::lookupKind("%="):  case Token::lookupKind("<<="):
				case Token::lookupKind("<<|="): case Token::lookupKind(">>="): case Token::lookupKind("&="):
				case Token::lookupKind("|="):   case Token::lookupKind("^="): {
					if(this->scope.inObjectScope() == false){
						this->emit_error(
							Diagnostic::Code::SEMA_OPERATOR_OVERLOAD_NOT_IN_TYPE,
							instr.func_def,
							"Operator overload cannot be a free function"
						);
						return Result::ERROR;
					}

					if(this->scope.getCurrentObjectScope().is<BaseType::Struct::ID>() == false){
						this->emit_error(
							Diagnostic::Code::SEMA_OPERATOR_OVERLOAD_NOT_IN_TYPE,
							instr.func_def,
							"Operator overload cannot be a free function"
						);
						return Result::ERROR;
					}

					const BaseType::Function& created_func_type =
						this->context.getTypeManager().getFunction(created_func_base_type.funcID());

					BaseType::Struct& current_struct = this->context.type_manager.getStruct(
						this->scope.getCurrentObjectScope().as<BaseType::Struct::ID>()
					);


					const TypeInfo::ID struct_type_info_id = this->context.type_manager.getOrCreateTypeInfo(
						TypeInfo(BaseType::ID(this->scope.getCurrentObjectScope().as<BaseType::Struct::ID>()))
					);


					if(created_func_type.params.size() != 2){
						if(created_func_type.params.size() == 0){
							this->emit_error(
								Diagnostic::Code::SEMA_INVALID_OPERATOR_INFIX_OVERLOAD,
								instr.func_def,
								"Infix operator overload doesn't have enough parameters"
							);
							return Result::ERROR;

						}else if(created_func_type.params.size() == 1){
							this->emit_error(
								Diagnostic::Code::SEMA_INVALID_OPERATOR_INFIX_OVERLOAD,
								instr.func_def.params[0],
								"Infix operator overload doesn't have enough parameters"
							);
							return Result::ERROR;
							
						}else{
							this->emit_error(
								Diagnostic::Code::SEMA_INVALID_OPERATOR_INFIX_OVERLOAD,
								instr.func_def.params[2],
								"Infix operator overload has too many parameters"
							);
							return Result::ERROR;
						}
					}

					if(
						this->source.getTokenBuffer()[created_func.params[0].ident.as<Token::ID>()].kind()
						!= Token::Kind::KEYWORD_THIS
					){
						this->emit_error(
							Diagnostic::Code::SEMA_INVALID_OPERATOR_INFIX_OVERLOAD,
							instr.func_def.params[0],
							"Infix operator overload must have a [this] parameter"
						);
						return Result::ERROR;
					}


					if(created_func_type.returnParams.size() > 1){
						this->emit_error(
							Diagnostic::Code::SEMA_INVALID_OPERATOR_INFIX_OVERLOAD,
							instr.func_def.returns[1],
							"Infix operator overload cannot have multiple return values"
						);
						return Result::ERROR;
					}


					switch(name_token.kind()){
						case Token::lookupKind("+"):    case Token::lookupKind("+%"):   case Token::lookupKind("+|"):
						case Token::lookupKind("-"):    case Token::lookupKind("-%"):   case Token::lookupKind("-|"):
						case Token::lookupKind("*"):    case Token::lookupKind("*%"):   case Token::lookupKind("*|"):
						case Token::lookupKind("/"):    case Token::lookupKind("%"):    case Token::lookupKind("<<"):
						case Token::lookupKind("<<|"):  case Token::lookupKind(">>"):   case Token::lookupKind("&"):
						case Token::lookupKind("|"):    case Token::lookupKind("^"):    case Token::lookupKind("~"): {
							if(created_func_type.returnsVoid()){
								this->emit_error(
									Diagnostic::Code::SEMA_INVALID_OPERATOR_INFIX_OVERLOAD,
									instr.func_def.returns[0].type,
									std::format(
										"Infix [{}] overload must return a value", Token::printKind(name_token.kind())
									)
								);
								return Result::ERROR;
							}
						} break;

						case Token::lookupKind("=="): case Token::lookupKind("!="): case Token::lookupKind("<"):
						case Token::lookupKind("<="): case Token::lookupKind(">"):  case Token::lookupKind(">="):
						case Token::lookupKind("&&"): case Token::lookupKind("||"): {
							if(created_func_type.returnParams[0].typeID != TypeManager::getTypeBool()){
								this->emit_error(
									Diagnostic::Code::SEMA_INVALID_OPERATOR_INFIX_OVERLOAD,
									instr.func_def.returns[0].type,
									std::format(
										"Infix [{}] overload must return a `Bool`", Token::printKind(name_token.kind())
									)
								);
								return Result::ERROR;
							}
						} break;

						case Token::lookupKind("+="):   case Token::lookupKind("+%="): case Token::lookupKind("+|="):
						case Token::lookupKind("-="):   case Token::lookupKind("-%="): case Token::lookupKind("-|="):
						case Token::lookupKind("*="):   case Token::lookupKind("*%="): case Token::lookupKind("*|="):
						case Token::lookupKind("/="):   case Token::lookupKind("%="):  case Token::lookupKind("<<="):
						case Token::lookupKind("<<|="): case Token::lookupKind(">>="): case Token::lookupKind("&="):
						case Token::lookupKind("|="):   case Token::lookupKind("^="): {
							if(created_func_type.returnsVoid() == false){
								this->emit_error(
									Diagnostic::Code::SEMA_INVALID_OPERATOR_INFIX_OVERLOAD,
									instr.func_def.returns[0].type,
									std::format(
										"Infix [{}] overload cannot return a value", Token::printKind(name_token.kind())
									)
								);
								return Result::ERROR;
							}
						}
					}


					if(created_func_type.hasErrorReturn()){
						this->emit_error(
							Diagnostic::Code::SEMA_INVALID_OPERATOR_INFIX_OVERLOAD,
							instr.func_def.errorReturns[0],
							"Infix operator overload that error are unimplemented"
						);
						return Result::ERROR;
					}



					const auto add_overload = [&](BaseType::Struct& target_struct, bool swapped) -> evo::Result<> {
						const auto lock = std::scoped_lock(target_struct.infixOverloadsLock);

						const auto [begin_overloads_range, end_overloads_range] = 
							target_struct.infixOverloads.equal_range(name_token.kind());


						const sema::Func::ID overload_func_id_to_add = [&](){
							if(swapped){
								const BaseType::ID swapped_type_id = this->context.type_manager.getOrCreateFunction(
									BaseType::Function(
										evo::SmallVector<BaseType::Function::Param>{
											created_func_type.params[1], created_func_type.params[0]
										},
										created_func_type.returnParams,
										created_func_type.errorParams
									)
								);

								const sema::Func::ID created_swapped_func_id = this->context.sema_buffer.createFunc(
									this->source.getID(),
									created_func.name,
									std::string(),
									swapped_type_id.funcID(),
									evo::SmallVector<sema::Func::Param>{created_func.params[1], created_func.params[0]},
									this->symbol_proc_id,
									2,
									false,
									created_func.isConstexpr,
									false,
									created_func.hasInParam
								);

								sema::Func& created_swapped_func =
									this->context.sema_buffer.funcs[created_swapped_func_id];

								created_swapped_func.stmtBlock.emplace_back(
									this->context.sema_buffer.createReturn(
										sema::Expr(
											this->context.sema_buffer.createFuncCall(
												created_func_id,
												evo::SmallVector<sema::Expr>{
													sema::Expr(this->context.sema_buffer.createParam(1, 1)),
													sema::Expr(this->context.sema_buffer.createParam(0, 0))
												}
											)
										),
										std::nullopt
									)
								);

								created_swapped_func.isTerminated = true;
								created_swapped_func.status = sema::Func::Status::DEF_DONE;

								func_info.flipped_version = created_swapped_func_id;

								return created_swapped_func_id;
								
							}else{
								return created_func_id;
							}
						}();


						const sema::Func& overload_func_to_add =
							this->context.getSemaBuffer().getFunc(overload_func_id_to_add);


						const auto overloads_range = evo::IterRange(begin_overloads_range, end_overloads_range);
						for(const auto& [_, existing_func_id] : overloads_range){
							const sema::Func& existing_func =
								this->context.getSemaBuffer().getFunc(existing_func_id);
							
							if(overload_func_to_add.isEquivalentOverload(existing_func, this->context)){
								this->emit_error(
									Diagnostic::Code::SEMA_INVALID_OPERATOR_INFIX_OVERLOAD,
									instr.func_def,
									"This operator overload was already defined",
									Diagnostic::Info(
										"Previously defined here:", this->get_location(existing_func_id)
									)
								);
								return evo::resultError;
							}
						}

						target_struct.infixOverloads.emplace(name_token.kind(), overload_func_id_to_add);
						return evo::Result<>();
					};



					if(func_attrs.value().is_commutative){
						if(struct_type_info_id == created_func_type.params[1].typeID){
							this->emit_error(
								Diagnostic::Code::SEMA_INVALID_OPERATOR_INFIX_OVERLOAD,
								instr.func_def,
								"Infix operator overload where the LHS and RHS are the same type "
									"cannot have attribute #commutative"
							);
							return Result::ERROR;
						}


						// normal
						if(add_overload(current_struct, false).isError()){ return Result::ERROR; }

						// swapped
						if(struct_type_info_id != created_func_type.params[1].typeID){
							const TypeInfo& other_type = 
								this->context.getTypeManager().getTypeInfo(created_func_type.params[1].typeID);

							if(other_type.baseTypeID().kind() == BaseType::Kind::STRUCT){
								BaseType::Struct& other_struct =
									this->context.type_manager.getStruct(other_type.baseTypeID().structID());

								if(add_overload(other_struct, true).isError()){ return Result::ERROR; }

							}else{
								if(add_overload(current_struct, true).isError()){ return Result::ERROR; }
							}
						}

					}else if(func_attrs.value().is_swapped){
						if(struct_type_info_id == created_func_type.params[1].typeID){
							this->emit_error(
								Diagnostic::Code::SEMA_INVALID_OPERATOR_INFIX_OVERLOAD,
								instr.func_def,
								"Infix operator overload where the LHS and RHS are the same type "
									"cannot have attribute #swapped"
							);
							return Result::ERROR;
						}

						const TypeInfo& other_type = 
							this->context.getTypeManager().getTypeInfo(created_func_type.params[1].typeID);

						if(other_type.baseTypeID().kind() == BaseType::Kind::STRUCT){
							BaseType::Struct& other_struct =
								this->context.type_manager.getStruct(other_type.baseTypeID().structID());

							if(add_overload(other_struct, true).isError()){ return Result::ERROR; }

						}else{
							if(add_overload(current_struct, true).isError()){ return Result::ERROR; }
						}
						
					}else{
						if(add_overload(current_struct, false).isError()){ return Result::ERROR; }
					}
				} break;

				default: {
					this->emit_error(
						Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
						instr.func_def.name,
						"This operator overload is currently unimplemented"
					);
					return Result::ERROR;
				} break;
			}

		}


		this->push_scope_level(&created_func.stmtBlock, created_func_id);


		///////////////////////////////////
		// done

		this->propagate_finished_decl();

		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_func_pre_body(const Instruction::FuncPreBody& instr) -> Result {
		const sema::Func::ID current_func_id = this->scope.getCurrentObjectScope().as<sema::Func::ID>();
		sema::Func& current_func = this->context.sema_buffer.funcs[current_func_id];

		BaseType::Function& func_type = this->context.type_manager.getFunction(current_func.typeID);
		const SymbolProc::FuncInfo& func_info = this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>();


		//////////////////
		// check valid in parameter

		for(size_t i = 0; const BaseType::Function::Param& param : func_type.params){
			if(
				param.kind == BaseType::Function::Param::Kind::IN
				&& this->context.getTypeManager().isCopyable(param.typeID) == false
				&& this->context.getTypeManager().isMovable(param.typeID) == false
			){
				this->emit_error(
					Diagnostic::Code::SEMA_IN_PARAM_NOT_COPYABLE_OR_MOVABLE,
					*instr.func_def.params[i].type,
					"Function [in] parameter type must be copyable and/or movable"
				);
				return Result::ERROR;
			}

			i += 1;
		}


		//////////////////
		// check param is copy

		for(size_t i = 0; const std::optional<TypeInfo::ID>& type_id : func_info.param_type_to_check_if_is_copy){
			EVO_DEFER([&](){ i += 1; });

			if(type_id.has_value() == false){ continue; }

			if(
				this->context.getTypeManager().isTriviallyCopyable(*type_id)
				&& this->context.getTypeManager().isTriviallySized(*type_id)
			){
				func_type.params[i].shouldCopy = true;
			}
		}


		if(func_info.flipped_version.has_value()){
			const sema::Func& flipped_version = this->context.getSemaBuffer().getFunc(*func_info.flipped_version);
			BaseType::Function& flipped_version_type = this->context.type_manager.getFunction(flipped_version.typeID);

			for(size_t i = 0; i < func_type.params.size(); i+=1){
				flipped_version_type.params[func_type.params.size()-i-1].shouldCopy = func_type.params[i].shouldCopy;
			}
		}



		//////////////////
		// check entry has valid signature

		if(this->context.entry == current_func_id){
			if(func_type.params.empty() == false){
				this->emit_error(
					Diagnostic::Code::SEMA_INVALID_ENTRY,
					current_func.params[0].ident.as<Token::ID>(),
					"Functions with the [#entry] attribute cannot have parameters"
				);
				return Result::ERROR;
			}

			if(instr.func_def.returns[0].ident.has_value()){
				this->emit_error(
					Diagnostic::Code::SEMA_INVALID_ENTRY,
					instr.func_def.returns[0],
					"Functions with the [#entry] attribute cannot have named returns"
				);
				return Result::ERROR;
			}

			if(
				func_type.returnParams[0].typeID.isVoid() ||
				this->get_actual_type<false, false>(func_type.returnParams[0].typeID.asTypeID())
					!= TypeManager::getTypeUI8()
			){
				auto infos = evo::SmallVector<Diagnostic::Info>();
				this->diagnostic_print_type_info(func_type.returnParams[0].typeID.asTypeID(), infos, "Returned type: ");
				this->emit_error(
					Diagnostic::Code::SEMA_INVALID_ENTRY,
					instr.func_def.returns[0].type,
					"Functions with the [#entry] attribute must return [UI8]",
					std::move(infos)
				);
				return Result::ERROR;
			}

			if(func_type.errorParams.empty() == false){
				this->emit_error(
					Diagnostic::Code::SEMA_INVALID_ENTRY,
					instr.func_def.errorReturns[0],
					"Functions with the [#entry] attribute cannot have error returns"
				);
				return Result::ERROR;
			}
		}


		//////////////////
		// prepare pir

		if(current_func.isConstexpr){
			auto sema_to_pir = SemaToPIR(
				this->context, this->context.constexpr_pir_module, this->context.constexpr_sema_to_pir_data
			);

			current_func.constexprJITFunc = sema_to_pir.lowerFuncDeclConstexpr(current_func_id);


			if(func_info.flipped_version.has_value()){
				sema::Func& flipped_version = this->context.sema_buffer.funcs[*func_info.flipped_version];

				flipped_version.constexprJITFunc = sema_to_pir.lowerFuncDeclConstexpr(*func_info.flipped_version);
			}

			this->propagate_finished_pir_decl();
		}


		//////////////////
		// adding params to scope

		uint32_t abi_index = 0;

		for(uint32_t i = 0; const AST::FuncDef::Param& param : instr.func_def.params){
			EVO_DEFER([&](){
				abi_index += 1;
				i += 1;
			});

			if(param.name.kind() == AST::Kind::THIS){
				this->scope.addThisParam(this->context.sema_buffer.createParam(i, abi_index));
				continue;
			}

			const std::string_view param_name = this->source.getTokenBuffer()[
				this->source.getASTBuffer().getIdent(param.name)
			].getString();

			if(this->add_ident_to_scope(
				param_name, param, this->context.sema_buffer.createParam(i, abi_index)
			).isError()){
				return Result::ERROR;
			}
		}


		if(func_type.hasNamedReturns()){
			for(uint32_t i = 0; const AST::FuncDef::Return& return_param : instr.func_def.returns){
				EVO_DEFER([&](){ i += 1; });

				const std::string_view return_param_name =
					this->source.getTokenBuffer()[*return_param.ident].getString();

				const sema::ReturnParam::ID created_return_param_id =
					this->context.sema_buffer.createReturnParam(i, abi_index);

				if(this->add_ident_to_scope(return_param_name, return_param, created_return_param_id).isError()){
					return Result::ERROR;
				}

				switch(this->source.getTokenBuffer()[current_func.name.as<Token::ID>()].kind()){
					case Token::Kind::KEYWORD_NEW: case Token::Kind::KEYWORD_COPY: case Token::Kind::KEYWORD_MOVE: {
						const TypeInfo& return_type = 
							this->context.getTypeManager().getTypeInfo(func_type.returnParams[i].typeID.asTypeID());
						const BaseType::Struct& return_struct_type =
							this->context.getTypeManager().getStruct(return_type.baseTypeID().structID());

						if(return_struct_type.memberVars.empty()) [[unlikely]] {
							// mark the output param already initialized if has no members
							this->get_current_scope_level().addIdentValueState(
								created_return_param_id, sema::ScopeLevel::ValueState::INIT
							);
							
						}else{
							this->get_current_scope_level().addIdentValueState(
								created_return_param_id, sema::ScopeLevel::ValueState::INITIALIZING
							);

							for(uint32_t j = 0; j < return_struct_type.memberVars.size(); j+=1){
								this->get_current_scope_level().addIdentValueState(
									sema::ReturnParamAccessorValueStateID(created_return_param_id, j),
									sema::ScopeLevel::ValueState::UNINIT
								);
							}

							SymbolProc::FuncInfo& func_info_mut =
								this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>();
							func_info_mut.num_members_of_initializing_are_uninit = return_struct_type.memberVars.size();
						}

					} break;

					default: {
						this->get_current_scope_level().addIdentValueState(
							created_return_param_id, sema::ScopeLevel::ValueState::UNINIT
						);
					} break;
				}

				abi_index += 1;
			}
		}else{
			if(this->source.getTokenBuffer()[current_func.name.as<Token::ID>()].kind() == Token::Kind::KEYWORD_DELETE){
				const TypeInfo& this_type = this->context.getTypeManager().getTypeInfo(func_type.params[0].typeID);
				const BaseType::Struct& this_struct_type =
					this->context.getTypeManager().getStruct(this_type.baseTypeID().structID());

				for(uint32_t i = 0; i < this_struct_type.memberVars.size(); i+=1){
					this->get_current_scope_level().addIdentValueState(
						sema::OpDeleteThisAccessorValueStateID(i), sema::ScopeLevel::ValueState::INIT
					);
				}
			}
		}


		if(func_type.hasNamedErrorReturns()){
			// account for the RET param
			if(func_type.returnsVoid() == false && func_type.hasNamedReturns() == false){
				abi_index += 1;
			}

			
			for(uint32_t i = 0; const AST::FuncDef::Return& error_return_param : instr.func_def.errorReturns){
				EVO_DEFER([&](){ i += 1; });

				const sema::ErrorReturnParam::ID created_error_return_param_id =
					this->context.sema_buffer.createErrorReturnParam(i, abi_index);

				if(this->add_ident_to_scope(
					this->source.getTokenBuffer()[*error_return_param.ident].getString(),
					error_return_param,
					created_error_return_param_id
				).isError()){
					return Result::ERROR;
				}

				this->get_current_scope_level().addIdentValueState(
					created_error_return_param_id, sema::ScopeLevel::ValueState::UNINIT
				);
			}
		}

		return Result::SUCCESS;
	}



	auto SemanticAnalyzer::instr_func_def(const Instruction::FuncDef& instr) -> Result {
		sema::Func& current_func = this->get_current_func();
		const BaseType::Function& func_type = this->context.getTypeManager().getFunction(current_func.typeID);


		if(this->get_current_scope_level().isTerminated()){
			current_func.isTerminated = true;

		}else{
			if(func_type.returnsVoid() == false){
				this->emit_error(
					Diagnostic::Code::SEMA_FUNC_ISNT_TERMINATED,
					instr.func_def,
					"Function isn't terminated",
					Diagnostic::Info(
						"A function that doesn't return `Void` is terminated when all control paths end in a "
						"[return], [error], [unreachable], or a function call that has the attribute [#noReturn]"
					)
				);
				return Result::ERROR;
			}

			this->add_auto_delete_calls<AutoDeleteMode::RETURN>();
		}


		{
			const std::optional<sema::ScopeManager::Scope::ObjectScope> current_interface_scope = 
				this->scope.getCurrentInterfaceScopeIfExists();

			if(current_interface_scope.has_value()){
				BaseType::Interface& current_interface = this->context.type_manager.getInterface(
					current_interface_scope->as<BaseType::Interface::ID>()
				);
				current_interface.methods.emplace_back(this->scope.getCurrentObjectScope().as<sema::Func::ID>());
			}
		}


		current_func.status = sema::Func::Status::DEF_DONE;
		this->propagate_finished_def();


		if(current_func.isConstexpr){
			bool any_waiting = false;
			const SymbolProc::FuncInfo& func_info = this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>();
			for(sema::Func::ID dependent_func_id : func_info.dependent_funcs){
				const sema::Func& dependent_func = this->context.getSemaBuffer().getFunc(dependent_func_id);

				if(dependent_func.symbolProcID.has_value() == false){ continue; }

				SymbolProc& dependent_func_symbol_proc = 
					this->context.symbol_proc_manager.getSymbolProc(*dependent_func.symbolProcID);

				const SymbolProc::WaitOnResult wait_on_result = dependent_func_symbol_proc.waitOnPIRDeclIfNeeded(
					this->symbol_proc_id, this->context, *dependent_func.symbolProcID
				);

				switch(wait_on_result){
					case SymbolProc::WaitOnResult::NOT_NEEDED:
						break;

					case SymbolProc::WaitOnResult::WAITING:
						any_waiting = true; break;

					case SymbolProc::WaitOnResult::WAS_ERRORED:
						return Result::ERROR;

					case SymbolProc::WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND:
						evo::debugFatalBreak("Shouldn't be possible");

					case SymbolProc::WaitOnResult::CIRCULAR_DEP_DETECTED:
						evo::debugFatalBreak("Shouldn't be possible");
				}
			}


			if(any_waiting){
				if(this->symbol_proc.shouldContinueRunning()){
					return Result::SUCCESS;
				}else{
					return Result::NEED_TO_WAIT_BEFORE_NEXT_INSTR;
				}

			}else{
				return Result::SUCCESS;
			}
		}

		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_func_prepare_constexpr_pir_if_needed(
		const Instruction::FuncPrepareConstexprPIRIfNeeded& instr
	) -> Result {
		const sema::Func& current_func = this->get_current_func();
		const BaseType::Function& func_type = this->context.getTypeManager().getFunction(current_func.typeID);

		if(current_func.isConstexpr){
			{
				auto sema_to_pir = SemaToPIR(
					this->context, this->context.constexpr_pir_module, this->context.constexpr_sema_to_pir_data
				);

				const sema::Func::ID sema_func_id = this->scope.getCurrentObjectScope().as<sema::Func::ID>();
				sema::Func& sema_func = this->context.sema_buffer.funcs[sema_func_id];

				sema_to_pir.lowerFuncDef(sema_func_id);


				const SymbolProc::FuncInfo& func_info = this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>();

				auto module_subset_funcs = evo::StaticVector<pir::Function::ID, 4>();
				module_subset_funcs.emplace_back(*sema_func.constexprJITFunc);


				// create jit interface if needed
				if(func_type.returnsVoid() == false && func_type.returnParams.size() == 1){
					sema_func.constexprJITInterfaceFunc = sema_to_pir.createFuncJITInterface(
						sema_func_id, *sema_func.constexprJITFunc
					);
					module_subset_funcs.emplace_back(*sema_func.constexprJITInterfaceFunc);


					if(func_info.flipped_version.has_value()){
						sema::Func& flipped_version = this->context.sema_buffer.funcs[*func_info.flipped_version];

						sema_to_pir.lowerFuncDef(*func_info.flipped_version);
						module_subset_funcs.emplace_back(*flipped_version.constexprJITFunc);

						flipped_version.constexprJITInterfaceFunc = sema_to_pir.createFuncJITInterface(
							*func_info.flipped_version, *flipped_version.constexprJITFunc
						);
						module_subset_funcs.emplace_back(*flipped_version.constexprJITInterfaceFunc);
					}
				}


				const evo::Expected<void, evo::SmallVector<std::string>> add_module_subset_result = 
					this->context.constexpr_jit_engine.addModuleSubsetWithWeakDependencies(
						this->context.constexpr_pir_module,
						pir::JITEngine::ModuleSubsets{ .funcs = module_subset_funcs, }
					);

				if(add_module_subset_result.has_value() == false){
					auto infos = evo::SmallVector<Diagnostic::Info>();
					for(const std::string& error : add_module_subset_result.error()){
						infos.emplace_back(std::format("Message from LLVM: \"{}\"", error));
					}

					this->emit_fatal(
						Diagnostic::Code::MISC_LLVM_ERROR,
						instr.func_def,
						Diagnostic::createFatalMessage("Failed to setup PIR JIT interface for constexpr function"),
						std::move(infos)
					);
					return Result::ERROR;
				}
			}



			bool any_waiting = false;
			for(
				sema::Func::ID dependent_func_id
				: this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>().dependent_funcs
			){
				const sema::Func& dependent_func = this->context.getSemaBuffer().getFunc(dependent_func_id);

				if(dependent_func.symbolProcID.has_value() == false){ continue; }

				SymbolProc& dependent_func_symbol_proc = 
					this->context.symbol_proc_manager.getSymbolProc(*dependent_func.symbolProcID);

				const SymbolProc::WaitOnResult wait_on_result = dependent_func_symbol_proc.waitOnPIRDefIfNeeded(
					this->symbol_proc_id, this->context, *dependent_func.symbolProcID
				);

				switch(wait_on_result){
					case SymbolProc::WaitOnResult::NOT_NEEDED:
						break;

					case SymbolProc::WaitOnResult::WAITING:
						any_waiting = true; break;

					case SymbolProc::WaitOnResult::WAS_ERRORED:
						return Result::ERROR;

					case SymbolProc::WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND:
						evo::debugFatalBreak("Shouldn't be possible");

					case SymbolProc::WaitOnResult::CIRCULAR_DEP_DETECTED:
						evo::debugFatalBreak("Shouldn't be possible");
				}
			}
			for(
				sema::GlobalVar::ID dependent_var_id
				: this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>().dependent_vars
			){
				const sema::GlobalVar& dependent_var = this->context.sema_buffer.getGlobalVar(dependent_var_id);
				SymbolProc& dependent_var_symbol_proc =
					this->context.symbol_proc_manager.getSymbolProc(*dependent_var.symbolProcID);

				const SymbolProc::WaitOnResult wait_on_result = dependent_var_symbol_proc.waitOnDefIfNeeded(
					this->symbol_proc_id, this->context, *dependent_var.symbolProcID
				);

				switch(wait_on_result){
					case SymbolProc::WaitOnResult::NOT_NEEDED:
						break;

					case SymbolProc::WaitOnResult::WAITING:
						any_waiting = true; break;

					case SymbolProc::WaitOnResult::WAS_ERRORED:
						return Result::ERROR;

					case SymbolProc::WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND:
						evo::debugFatalBreak("Shouldn't be possible");

					case SymbolProc::WaitOnResult::CIRCULAR_DEP_DETECTED:
						evo::debugFatalBreak("Shouldn't be possible");
				}
			}

			if(any_waiting){
				if(this->symbol_proc.shouldContinueRunning()){
					return Result::SUCCESS;
				}else{
					return Result::NEED_TO_WAIT_BEFORE_NEXT_INSTR;
				}

			}else{
				return Result::SUCCESS;
			}
		}

		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_func_constexpr_pir_ready_if_needed() -> Result {
		const sema::Func& current_func = this->get_current_func();

		if(current_func.isConstexpr){
			this->propagate_finished_pir_def();
		}

		if(this->pop_scope_level<PopScopeLevelKind::SYMBOL_END>().isError()){ return Result::ERROR; }

		return Result::SUCCESS;
	}



	auto SemanticAnalyzer::instr_template_func_begin(const Instruction::TemplateFuncBegin& instr) -> Result {
		{
			const Token::Kind name_token_kind = this->source.getTokenBuffer()[instr.func_def.name].kind();

			if(name_token_kind != Token::Kind::IDENT){
				if(instr.func_def.templatePack.has_value()){
					this->emit_error(
						Diagnostic::Code::SEMA_TEMPLATED_OPERATOR_OVERLOAD,
						instr.func_def.name,
						"Operator overload cannot have a template parameter pack"
					);
					return Result::ERROR;

				}else if(name_token_kind != Token::Kind::KEYWORD_NEW){
					this->emit_error(
						Diagnostic::Code::SEMA_TEMPLATED_OPERATOR_OVERLOAD,
						instr.func_def.name,
						"This operator overload cannot be template"
					);
					return Result::ERROR;
				}
			}
		}
		

		size_t minimum_num_template_args = 0;
		auto template_params = evo::SmallVector<sema::TemplatedFunc::TemplateParam>();

		for(const SymbolProc::Instruction::TemplateParamInfo& template_param_info : instr.template_param_infos){
			auto type_id = std::optional<TypeInfo::ID>();
			if(template_param_info.type_id.has_value()){
				const TypeInfo::VoidableID type_info_voidable_id = this->get_type(*template_param_info.type_id);
				if(type_info_voidable_id.isVoid()){
					this->emit_error(
						Diagnostic::Code::SEMA_TEMPLATE_PARAM_CANNOT_BE_TYPE_VOID,
						template_param_info.param.type,
						"Template parameter cannot be type `Void`"
					);
					return Result::ERROR;
				}
				type_id = type_info_voidable_id.asTypeID();
			}

			TermInfo* default_value = nullptr;
			if(template_param_info.default_value.has_value()){
				default_value = &this->get_term_info(*template_param_info.default_value);

				if(type_id.has_value()){
					if(default_value->isSingleValue() == false){
						if(default_value->isMultiValue()){
							this->emit_error(
								Diagnostic::Code::SEMA_TEMPLATE_PARAM_EXPR_DEFAULT_MUST_BE_EXPR,
								*template_param_info.param.defaultValue,
								"Default of an expression template parameter must be a single expression"
							);	
						}else{
							this->emit_error(
								Diagnostic::Code::SEMA_TEMPLATE_PARAM_EXPR_DEFAULT_MUST_BE_EXPR,
								*template_param_info.param.defaultValue,
								"Default of an expression template parameter must be an expression"
							);
						}
						return Result::ERROR;
					}

					const TypeCheckInfo type_check_info = this->type_check<true, true>(
						*type_id,
						*default_value,
						"Default value of template parameter",
						*template_param_info.param.defaultValue
					);
					if(type_check_info.ok == false){
						return Result::ERROR;
					}

				}else{
					if(default_value->value_category != TermInfo::ValueCategory::TYPE){
						this->emit_error(
							Diagnostic::Code::SEMA_TEMPLATE_PARAM_TYPE_DEFAULT_MUST_BE_TYPE,
							*template_param_info.param.defaultValue,
							"Default of a [Type] template parameter must be an type"
						);
						return Result::ERROR;
					}
				}
			}else{
				minimum_num_template_args += 1;
			}

			if(default_value == nullptr){
				template_params.emplace_back(type_id, std::monostate());

			}else if(default_value->value_category == TermInfo::ValueCategory::TYPE){
				template_params.emplace_back(type_id, default_value->type_id.as<TypeInfo::VoidableID>());

			}else{
				template_params.emplace_back(type_id, default_value->getExpr());
			}
		}


		const sema::TemplatedFunc::ID new_templated_func_id = this->context.sema_buffer.createTemplatedFunc(
			this->symbol_proc,
			minimum_num_template_args,
			std::move(template_params),
			evo::SmallVector<bool>(instr.func_def.params.size(), false)
		);

		this->symbol_proc.extra_info.emplace<SymbolProc::TemplateFuncInfo>(
			new_templated_func_id, this->context.sema_buffer.templated_funcs[new_templated_func_id]
		);

		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_template_func_check_param_is_interface(
		const Instruction::TemplateFuncCheckParamIsInterface& instr
	) -> Result {
		const TypeInfo::VoidableID type_id = this->get_type(instr.param_type);

		if(type_id.isVoid()){ return Result::SUCCESS; }

		const TypeInfo& ident_type = this->context.getTypeManager().getTypeInfo(
			this->get_actual_type<true, true>(type_id.asTypeID())
		);

		if(ident_type.qualifiers().empty() == false){ return Result::SUCCESS; }
		if(ident_type.baseTypeID().kind() != BaseType::Kind::INTERFACE){ return Result::SUCCESS; }

		SymbolProc::TemplateFuncInfo& template_func_info = 
			this->symbol_proc.extra_info.as<SymbolProc::TemplateFuncInfo>();
		template_func_info.templated_func.paramIsTemplate[instr.param_index] = true;

		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_template_set_param_is_deducer(
		const Instruction::TemplateFuncSetParamIsDeducer& instr
	) -> Result {
		SymbolProc::TemplateFuncInfo& template_func_info = 
			this->symbol_proc.extra_info.as<SymbolProc::TemplateFuncInfo>();
		template_func_info.templated_func.paramIsTemplate[instr.param_index] = true;

		return Result::SUCCESS;
	}

	
	auto SemanticAnalyzer::instr_template_func_end(const Instruction::TemplateFuncEnd& instr) -> Result {
		const Token& name_token = this->source.getTokenBuffer()[instr.func_def.name];

		if(name_token.kind() == Token::Kind::IDENT){
			const std::string_view name = name_token.getString();
			const sema::TemplatedFunc::ID templated_func_id =
				this->symbol_proc.extra_info.as<SymbolProc::TemplateFuncInfo>().templated_func_id;

			if(this->add_ident_to_scope(name, instr.func_def, templated_func_id).isError()){
				return Result::ERROR;
			}

			this->propagate_finished_decl_def();

			return Result::SUCCESS;

		}else{
			evo::debugAssert(name_token.kind() == Token::Kind::KEYWORD_NEW);
			
			this->emit_error(
				Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
				instr.func_def.name,
				"This operator overload being a template is unimplemented"
			);
			return Result::ERROR;
		}

	}



	auto SemanticAnalyzer::instr_deleted_special_method(const Instruction::DeletedSpecialMethod& instr) -> Result {
		if(this->scope.inObjectScope() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_OPERATOR_OVERLOAD_NOT_IN_TYPE,
				instr.deleted_special_method,
				"Operator overload cannot be a free function"
			);
			return Result::ERROR;
		}

		if(this->scope.getCurrentObjectScope().is<BaseType::Struct::ID>() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_OPERATOR_OVERLOAD_NOT_IN_TYPE,
				instr.deleted_special_method,
				"Operator overload cannot be a free function"
			);
			return Result::ERROR;
		}

		BaseType::Struct& current_struct = this->context.type_manager.getStruct(
			this->scope.getCurrentObjectScope().as<BaseType::Struct::ID>()
		);

		const Token::Kind deleted_kind = this->source.getTokenBuffer()[instr.deleted_special_method.memberToken].kind();


		switch(deleted_kind){
			case Token::Kind::KEYWORD_COPY: {
				auto init_expected = BaseType::Struct::DeletableOverload();
				if(current_struct.copyInitOverload.compare_exchange_strong(
					init_expected, BaseType::Struct::DeletableOverload(std::nullopt, true)
				) == false){
					if(init_expected.wasDeleted){
						this->emit_error(
							Diagnostic::Code::SEMA_INVALID_OPERATOR_COPY_OVERLOAD,
							instr.deleted_special_method,
							"Operator overload [copy] was already deleted"
						);
					}else{
						this->emit_error(
							Diagnostic::Code::SEMA_INVALID_OPERATOR_COPY_OVERLOAD,
							*init_expected.funcID,
							"Operator overload [copy] was already deleted"
						);
					}
					return Result::ERROR;
				}

				if(current_struct.copyAssignOverload.load().has_value()){
					this->emit_error(
						Diagnostic::Code::SEMA_INVALID_OPERATOR_COPY_OVERLOAD,
						*current_struct.copyAssignOverload.load(),
						"Operator overload [copy] was already deleted"
					);
					return Result::ERROR;
				}
			} break;

			case Token::Kind::KEYWORD_MOVE: {
				auto init_expected = BaseType::Struct::DeletableOverload();
				if(current_struct.moveInitOverload.compare_exchange_strong(
					init_expected, BaseType::Struct::DeletableOverload(std::nullopt, true)
				) == false){
					if(init_expected.wasDeleted){
						this->emit_error(
							Diagnostic::Code::SEMA_INVALID_OPERATOR_MOVE_OVERLOAD,
							instr.deleted_special_method,
							"Operator overload [move] was already deleted"
						);
					}else{
						this->emit_error(
							Diagnostic::Code::SEMA_INVALID_OPERATOR_MOVE_OVERLOAD,
							*init_expected.funcID,
							"Operator overload [move] was already deleted"
						);
					}
					return Result::ERROR;
				}

				if(current_struct.moveAssignOverload.load().has_value()){
					this->emit_error(
						Diagnostic::Code::SEMA_INVALID_OPERATOR_MOVE_OVERLOAD,
						*current_struct.moveAssignOverload.load(),
						"Operator overload [move] was already deleted"
					);
					return Result::ERROR;
				}
			} break;

			default: {
				evo::debugFatalBreak("Unknown or unsupported deletable overload");
			} break;
		}


		this->propagate_finished_decl_def();

		return Result::SUCCESS;
	}




	auto SemanticAnalyzer::instr_interface_decl(const Instruction::InterfaceDecl& instr) -> Result {
		const evo::Result<InterfaceAttrs> interface_attrs = this->analyze_interface_attrs(
			instr.interface_def, instr.attribute_params_info
		);
		if(interface_attrs.isError()){ return Result::ERROR; }

		const BaseType::ID created_interface_type_id = this->context.type_manager.getOrCreateInterface(
			BaseType::Interface(
				instr.interface_def.ident,
				this->source.getID(),
				this->symbol_proc_id,
				interface_attrs.value().is_pub
			)
		);

		const std::string_view ident_str = this->source.getTokenBuffer()[instr.interface_def.ident].getString();
		if(this->add_ident_to_scope(
			ident_str, instr.interface_def, created_interface_type_id.interfaceID()
		).isError()){
			return Result::ERROR;
		}

		this->push_scope_level(nullptr, created_interface_type_id.interfaceID());
		this->get_current_scope_level().setDontDoShadowingChecks();

		this->context.symbol_proc_manager.addTypeSymbolProc(
			this->context.type_manager.getOrCreateTypeInfo(TypeInfo(created_interface_type_id)),
			this->symbol_proc_id
		);

		this->propagate_finished_decl();

		return Result::SUCCESS;
	}



	auto SemanticAnalyzer::instr_interface_def() -> Result {
		BaseType::Interface::ID current_interface_id =
			this->scope.getCurrentObjectScope().as<BaseType::Interface::ID>();

		BaseType::Interface& current_interface = this->context.type_manager.getInterface(current_interface_id);

		current_interface.defCompleted = true;

		if(this->pop_scope_level<PopScopeLevelKind::SYMBOL_END>().isError()){ return Result::ERROR; }

		auto sema_to_pir = SemaToPIR(
			this->context, this->context.constexpr_pir_module, this->context.constexpr_sema_to_pir_data
		);
		sema_to_pir.lowerInterface(current_interface_id);

		this->propagate_finished_def();

		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_interface_func_def(const Instruction::InterfaceFuncDef& instr) -> Result {
		const sema::Func::ID current_method_id = this->scope.getCurrentObjectScope().as<sema::Func::ID>();
		sema::Func& current_method = this->context.sema_buffer.funcs[current_method_id];

		if(instr.func_def.block.has_value()){ // has default 
			if(this->get_current_scope_level().isTerminated()){
				current_method.isTerminated = true;

			}else{
				const BaseType::Function& func_type = this->context.getTypeManager().getFunction(current_method.typeID);

				if(func_type.returnsVoid() == false){
					this->emit_error(
						Diagnostic::Code::SEMA_FUNC_ISNT_TERMINATED,
						instr.func_def,
						"Function isn't terminated",
						Diagnostic::Info(
							"A function is terminated when all control paths end in a [return], [error], [unreachable],"
							" or a function call that has the attribute [#noReturn]"
						)
					);
					return Result::ERROR;
				}
			}

			current_method.status = sema::Func::Status::DEF_DONE;

			this->add_auto_delete_calls<AutoDeleteMode::NORMAL>();

		}else{
			current_method.status = sema::Func::Status::INTERFACE_METHOD_NO_DEFAULT;
		}

		if(this->pop_scope_level<PopScopeLevelKind::SYMBOL_END>().isError()){ return Result::ERROR; }
		this->propagate_finished_def();

		BaseType::Interface& current_interface = this->context.type_manager.getInterface(
			this->scope.getCurrentObjectScope().as<BaseType::Interface::ID>()
		);
		current_interface.methods.emplace_back(current_method_id);

		return Result::SUCCESS;
	}



	auto SemanticAnalyzer::instr_interface_impl_decl(const Instruction::InterfaceImplDecl& instr) -> Result {
		const TypeInfo::VoidableID target_type_id = this->get_type(instr.target);

		if(target_type_id.isVoid()){
			this->emit_error(
				Diagnostic::Code::SEMA_INTERFACE_IMPL_TARGET_NOT_INTERFACE,
				instr.interface_impl.target,
				"Interface impl target is not an interface"
			);
			return Result::ERROR;
		}

		const TypeInfo& target_type = this->context.getTypeManager().getTypeInfo(target_type_id.asTypeID());

		if(target_type.qualifiers().empty() == false || target_type.baseTypeID().kind() != BaseType::Kind::INTERFACE){
			this->emit_error(
				Diagnostic::Code::SEMA_INTERFACE_IMPL_TARGET_NOT_INTERFACE,
				instr.interface_impl.target,
				"Interface impl target is not an interface"
			);
			return Result::ERROR;
		}

		const BaseType::Interface::ID target_interface_id = target_type.baseTypeID().interfaceID();
		BaseType::Interface& target_interface = this->context.type_manager.getInterface(target_interface_id);

		if(!this->scope.inObjectScope() || !this->scope.getCurrentObjectScope().is<BaseType::Struct::ID>()){
			this->emit_error(
				Diagnostic::Code::SEMA_INTERFACE_IMPL_NOT_DEFINED_IN_STRUCT,
				instr.interface_impl,
				"Interface impl must be defined in type scope"
			);
			return Result::ERROR;
		}

		const BaseType::Struct::ID current_struct_id = this->scope.getCurrentObjectScope().as<BaseType::Struct::ID>();
		const BaseType::Struct& current_struct = this->context.getTypeManager().getStruct(current_struct_id);

		BaseType::Interface::Impl& interface_impl = this->context.type_manager.createInterfaceImpl();

		this->symbol_proc.extra_info.emplace<SymbolProc::InterfaceImplInfo>(
			target_interface_id, target_interface, current_struct, interface_impl
		);

		this->propagate_finished_decl();

		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_interface_impl_method_lookup(const Instruction::InterfaceImplMethodLookup& instr)
	-> Result {
		SymbolProc::InterfaceImplInfo& info = this->symbol_proc.extra_info.as<SymbolProc::InterfaceImplInfo>();

		const std::string_view target_ident_str = this->source.getTokenBuffer()[instr.method_name].getString();

		const WaitOnSymbolProcResult wait_on_symbol_proc_result = this->wait_on_symbol_proc<false>(
			info.current_struct.namespacedMembers, target_ident_str
		);

		switch(wait_on_symbol_proc_result){
			case WaitOnSymbolProcResult::NOT_FOUND: case WaitOnSymbolProcResult::ERROR_PASSED_BY_WHEN_COND: {
				this->wait_on_symbol_proc_emit_error(
					wait_on_symbol_proc_result,
					instr.method_name,
					std::format("Interface has no method named \"{}\"", target_ident_str)
				);
				return Result::ERROR;
			} break;

			case WaitOnSymbolProcResult::CIRCULAR_DEP_DETECTED: case WaitOnSymbolProcResult::EXISTS_BUT_ERRORED: {
				return Result::ERROR;
			} break;

			case WaitOnSymbolProcResult::NEED_TO_WAIT: {
				return Result::NEED_TO_WAIT;
			} break;

			case WaitOnSymbolProcResult::SEMAS_READY: {
				// do nothing...
			} break;
		}


		evo::Expected<TermInfo, AnalyzeExprIdentInScopeLevelError> expr_ident = 
			this->analyze_expr_ident_in_scope_level<false, false>(
				instr.method_name,
				target_ident_str,
				*info.current_struct.scopeLevel,
				true,
				true,
				&this->context.getSourceManager()[info.current_struct.sourceID.as<Source::ID>()]
			);


		if(expr_ident.has_value() == false){
			switch(expr_ident.error()){
				case AnalyzeExprIdentInScopeLevelError::DOESNT_EXIST: {
					evo::debugFatalBreak("Decl is done, but can't find sema of symbol");
				} break;

				case AnalyzeExprIdentInScopeLevelError::NEEDS_TO_WAIT_ON_DEF: {
					evo::debugFatalBreak(
						"Sema doesn't have completed info for decl despite SymbolProc saying it should"
					);
				} break;

				case AnalyzeExprIdentInScopeLevelError::ERROR_EMITTED: return Result::ERROR;
			}
		}


		info.targets.emplace_back(std::move(*expr_ident));

		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_interface_impl_def(const Instruction::InterfaceImplDef& instr) -> Result {
		SymbolProc::InterfaceImplInfo& info = this->symbol_proc.extra_info.as<SymbolProc::InterfaceImplInfo>();

		const Source& target_source = this->context.getSourceManager()[info.target_interface.sourceID];

		info.interface_impl.methods.reserve(info.target_interface.methods.size());

		const auto interface_has_member = [&](std::string_view ident) -> bool {
			for(const sema::Func::ID method_id : info.target_interface.methods){
				const sema::Func& method = this->context.getSemaBuffer().getFunc(method_id);
				const std::string_view target_method_name = method.getName(this->context.getSourceManager());
				if(target_method_name == ident){ return true; }
			}

			return false;
		};

		size_t method_init_i = 0;
		for(sema::Func::ID target_method_id : info.target_interface.methods){
			const sema::Func& target_method = this->context.getSemaBuffer().getFunc(target_method_id);
			const std::string_view target_method_name = target_method.getName(this->context.getSourceManager());

			if(method_init_i >= instr.interface_impl.methods.size()){
				if(target_method.status == sema::Func::Status::DEF_DONE){ // has default
					info.interface_impl.methods.emplace_back(target_method_id);
					continue;
				}

				if(instr.interface_impl.methods.empty()){
					this->emit_error(
						Diagnostic::Code::SEMA_INTERFACE_IMPL_METHOD_NOT_SET,
						instr.interface_impl,
						std::format("Method \"{}\" was not set in interface impl", target_method_name)
					);
				}else{
					this->emit_error(
						Diagnostic::Code::SEMA_INTERFACE_IMPL_METHOD_NOT_SET,
						instr.interface_impl,
						std::format("Method \"{}\" was not set in interface impl", target_method_name),
						Diagnostic::Info(
							std::format("Method listing for \"{}\" should go after this one", target_method_name),
							this->get_location(instr.interface_impl.methods[method_init_i - 1].method)
						)
					);
				}

				return Result::ERROR;

				
			}else{
				const AST::InterfaceImpl::Method& method_init = instr.interface_impl.methods[method_init_i];

				const std::string_view method_init_name = this->source.getTokenBuffer()[method_init.method].getString();

				if(target_method_name != method_init_name){
					if(target_method.status == sema::Func::Status::DEF_DONE){ // has default
						info.interface_impl.methods.emplace_back(target_method_id);
						continue;
					}

					if(interface_has_member(method_init_name)){
						this->emit_error(
							Diagnostic::Code::SEMA_INTERFACE_IMPL_METHOD_NOT_SET,
							instr.interface_impl,
							std::format("Method \"{}\" was not set in interface impl", target_method_name),
							Diagnostic::Info(
								std::format("Method listing for \"{}\" should go before this one", target_method_name),
								this->get_location(method_init.method)
							)
						);
					}else{
						this->emit_error(
							Diagnostic::Code::SEMA_INTERFACE_IMPL_METHOD_DOESNT_EXIST,
							method_init.method,
							std::format("This interface has no method \"{}\"", method_init_name),
							Diagnostic::Info(
								"Interface was declared here:",
								Diagnostic::Location::get(info.target_interface.identTokenID, target_source)
							)
						);
						return Result::ERROR;
					}

					return Result::ERROR;
				}

				// find if of the overloads any match
				bool found_overload = false;
				using Overload = evo::Variant<sema::FuncID, sema::TemplatedFuncID>;
				for(const Overload& overload : info.targets[method_init_i].type_id.as<TermInfo::FuncOverloadList>()){
					if(overload.is<sema::TemplatedFunc::ID>()){ continue; }

					const sema::Func& overload_sema =
						this->context.getSemaBuffer().getFunc(overload.as<sema::Func::ID>());

					if(target_method.typeID != overload_sema.typeID){
						if(target_method.isMethod(this->context) == false){ continue; }
						if(overload_sema.isMethod(this->context) == false){ continue; }

						const BaseType::Function& target_method_type = 
							this->context.getTypeManager().getFunction(target_method.typeID);

						const BaseType::Function& overload_sema_type = 
							this->context.getTypeManager().getFunction(overload_sema.typeID);


						if(target_method_type.params.size() != overload_sema_type.params.size()){ continue; }

						for(size_t param_i = 1; param_i < target_method_type.params.size(); param_i+=1){
							if(target_method_type.params[param_i] != overload_sema_type.params[param_i]){ continue; }
						}
						if(target_method_type.returnParams != overload_sema_type.returnParams){ continue; }
						if(target_method_type.errorParams != overload_sema_type.errorParams){ continue; }
					}

					if(target_method.isConstexpr && overload_sema.isConstexpr == false){ continue; }

					info.interface_impl.methods.emplace_back(overload.as<sema::Func::ID>());
					found_overload = true;
					break;
				}

				if(found_overload == false){
					this->emit_error(
						Diagnostic::Code::SEMA_INTERFACE_IMPL_NO_OVERLOAD_MATCHES,
						method_init.method,
						"This type has no method that has the correct signature",
						Diagnostic::Info("Interface method declared here:", this->get_location(target_method_id))
					);
					return Result::ERROR;
				}

				method_init_i += 1;
			}
		}




		{
			const auto lock = std::scoped_lock(info.target_interface.implsLock);
			info.target_interface.impls.emplace(
				BaseType::ID(this->scope.getCurrentObjectScope().as<BaseType::Struct::ID>()), info.interface_impl
			);
		}

		this->propagate_finished_def();


		bool any_waiting = false;
		for(const sema::Func::ID method_id : info.interface_impl.methods){
			const sema::Func& method = this->context.getSemaBuffer().getFunc(method_id);

			if(method.isConstexpr == false){ continue; }

			SymbolProc& method_symbol_proc = this->context.symbol_proc_manager.getSymbolProc(*method.symbolProcID);

			const SymbolProc::WaitOnResult wait_on_result = method_symbol_proc.waitOnPIRDeclIfNeeded(
				this->symbol_proc_id, this->context, *method.symbolProcID
			);

			switch(wait_on_result){
				case SymbolProc::WaitOnResult::NOT_NEEDED:                break;
				case SymbolProc::WaitOnResult::WAITING:                   any_waiting = true; break;
				case SymbolProc::WaitOnResult::WAS_ERRORED:               return Result::ERROR;
				case SymbolProc::WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND:evo::debugFatalBreak("Shouldn't be possible");
				case SymbolProc::WaitOnResult::CIRCULAR_DEP_DETECTED:     evo::debugFatalBreak("Shouldn't be possible");
			}
		}

		if(any_waiting){
			if(this->symbol_proc.shouldContinueRunning()){
				return Result::SUCCESS;
			}else{
				return Result::NEED_TO_WAIT_BEFORE_NEXT_INSTR;
			}

		}else{
			return Result::SUCCESS;
		}
	}


	auto SemanticAnalyzer::instr_interface_impl_constexpr_pir() -> Result {
		SymbolProc::InterfaceImplInfo& info = this->symbol_proc.extra_info.as<SymbolProc::InterfaceImplInfo>();

		const BaseType::ID current_type_base_type_id =
			BaseType::ID(this->scope.getCurrentObjectScope().as<BaseType::Struct::ID>());


		auto sema_to_pir = SemaToPIR(
			this->context, this->context.constexpr_pir_module, this->context.constexpr_sema_to_pir_data
		);

		const BaseType::Interface::Impl& interface_impl = [&](){
			const auto lock = std::scoped_lock(info.target_interface.implsLock);
			return info.target_interface.impls.at(current_type_base_type_id);
		}();

		sema_to_pir.lowerInterfaceVTableConstexpr(
			info.target_interface_id, current_type_base_type_id, interface_impl.methods
		);

		this->propagate_finished_pir_def();

		return Result::SUCCESS;
	}




	auto SemanticAnalyzer::instr_local_var(const Instruction::LocalVar& instr) -> Result {
		if(this->check_scope_isnt_terminated(instr.var_def).isError()){ return Result::ERROR; }

		const std::string_view var_ident = this->source.getTokenBuffer()[instr.var_def.ident].getString();

		const evo::Result<VarAttrs> var_attrs = this->analyze_var_attrs(instr.var_def, instr.attribute_params_info);
		if(var_attrs.isError()){ return Result::ERROR; }

		if(var_attrs.value().is_global){
			this->emit_error(
				Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
				instr.var_def,
				"Static variables are currently unimplemented"
			);
			return Result::ERROR;
		}


		TermInfo& value_term_info = this->get_term_info(instr.value);
		if(value_term_info.value_category == TermInfo::ValueCategory::MODULE){
			if(instr.var_def.kind != AST::VarDef::Kind::DEF){
				this->emit_error(
					Diagnostic::Code::SEMA_MODULE_VAR_MUST_BE_DEF,
					*instr.var_def.value,
					"Variable that has a module value must be declared as [def]"
				);
				return Result::ERROR;
			}

			const evo::Result<> add_ident_result = this->add_ident_to_scope(
				var_ident,
				instr.var_def,
				value_term_info.type_id.as<Source::ID>(),
				instr.var_def.ident,
				false
			);

			return add_ident_result.isError() ? Result::ERROR : Result::SUCCESS;
		}


		if(value_term_info.value_category == TermInfo::ValueCategory::INITIALIZER){
			if(instr.type_id.has_value() == false){
				this->emit_error(
					Diagnostic::Code::SEMA_VAR_INITIALIZER_WITHOUT_EXPLICIT_TYPE,
					*instr.var_def.value,
					"Cannot define a variable with an initializer value without an explicit type"
				);
				return Result::ERROR;
			}

		}else if(value_term_info.value_category == TermInfo::ValueCategory::NULL_VALUE){
			if(instr.type_id.has_value() == false){
				this->emit_error(
					Diagnostic::Code::SEMA_VAR_NULL_WITHOUT_EXPLICIT_TYPE,
					*instr.var_def.value,
					"Cannot define a variable with a value [null] value without an explicit type"
				);
				return Result::ERROR;
			}

		}else if(value_term_info.is_ephemeral() == false){
			if(this->check_term_isnt_type(value_term_info, *instr.var_def.value).isError()){ return Result::ERROR; }

			this->emit_error(
				Diagnostic::Code::SEMA_VAR_DEF_NOT_EPHEMERAL,
				*instr.var_def.value,
				"Cannot define a variable with a value that is not ephemeral, an initializer value, or [null]"
			);
			return Result::ERROR;
		}

			
		if(value_term_info.isMultiValue()){
			this->emit_error(
				Diagnostic::Code::SEMA_MULTI_RETURN_INTO_SINGLE_VALUE,
				*instr.var_def.value,
				"Cannot define a variable with multiple values"
			);
			return Result::ERROR;
		}


		if(instr.type_id.has_value()){
			const TypeInfo::VoidableID got_type_info_id = this->get_type(*instr.type_id);

			if(got_type_info_id.isVoid()){
				this->emit_error(
					Diagnostic::Code::SEMA_VAR_TYPE_VOID, *instr.var_def.type, "Variables cannot be type `Void`"
				);
				return Result::ERROR;
			}


			if(value_term_info.value_category != TermInfo::ValueCategory::INITIALIZER){
				const TypeCheckInfo type_check_info = this->type_check<true, true>(
					got_type_info_id.asTypeID(), value_term_info, "Variable definition", *instr.var_def.value
				);

				if(type_check_info.ok == false){ return Result::ERROR; }

				if(this->add_deduced_terms_to_scope(type_check_info.deduced_terms).isError()){ return Result::ERROR; }
			}

		}else if(
			instr.var_def.kind != AST::VarDef::Kind::DEF &&
			value_term_info.value_category == TermInfo::ValueCategory::EPHEMERAL_FLUID
		){
			this->emit_error(
				Diagnostic::Code::SEMA_CANNOT_INFER_TYPE,
				*instr.var_def.value,
				"Cannot infer the type of a fluid literal",
				Diagnostic::Info("Did you mean this variable to be [def]? If not, give the variable an explicit type")
			);
			return Result::ERROR;
		}

		const std::optional<TypeInfo::ID> type_id = [&]() -> std::optional<TypeInfo::ID> {
			if(value_term_info.type_id.is<TypeInfo::ID>()){
				return std::optional<TypeInfo::ID>(value_term_info.type_id.as<TypeInfo::ID>());
			}

			if(
				value_term_info.value_category == TermInfo::ValueCategory::INITIALIZER
				|| value_term_info.value_category == TermInfo::ValueCategory::NULL_VALUE
			){
				return this->get_type(*instr.type_id).asTypeID();
			}

			return std::optional<TypeInfo::ID>();
		}();

		const sema::Var::ID new_sema_var = this->context.sema_buffer.createVar(
			instr.var_def.kind, instr.var_def.ident, value_term_info.getExpr(), type_id
		);
		this->get_current_scope_level().stmtBlock().emplace_back(new_sema_var);

		if(this->add_ident_to_scope(var_ident, instr.var_def, new_sema_var).isError()){ return Result::ERROR; }


		if(value_term_info.value_category == TermInfo::ValueCategory::INITIALIZER){
			this->get_current_scope_level().addIdentValueState(new_sema_var, sema::ScopeLevel::ValueState::UNINIT);
		}else{
			this->get_current_scope_level().addIdentValueState(new_sema_var, sema::ScopeLevel::ValueState::INIT);
		}

		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_local_alias(const Instruction::LocalAlias& instr) -> Result {
		if(instr.attribute_params_info.empty() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_UNKNOWN_ATTRIBUTE,
				this->source.getASTBuffer().getAttributeBlock(instr.alias_def.attributeBlock).attributes[0].attribute,
				"Unknown local alias attribute"
			);
			return Result::ERROR;
		}

		const TypeInfo::VoidableID aliased_type = this->get_type(instr.aliased_type);
		if(aliased_type.isVoid()){
			this->emit_error(
				Diagnostic::Code::SEMA_ALIAS_CANNOT_BE_VOID,
				instr.alias_def.type,
				"Alias cannot be type `Void`"
			);
			return Result::ERROR;
		}


		const BaseType::ID created_alias_id = this->context.type_manager.getOrCreateAlias(
			BaseType::Alias(
				this->source.getID(),
				instr.alias_def.ident,
				std::atomic<std::optional<TypeInfo::ID>>(aliased_type.asTypeID()),
				false
			)
		);

		const std::string_view alias_ident = this->source.getTokenBuffer()[instr.alias_def.ident].getString();
		if(this->add_ident_to_scope(alias_ident, instr.alias_def.ident, created_alias_id.aliasID()).isError()){
			return Result::ERROR;
		}

		return Result::SUCCESS;
	}



	auto SemanticAnalyzer::instr_return(const Instruction::Return& instr) -> Result {
		evo::debugAssert(instr.return_stmt.label.has_value() == false, "Wrong instruction for a labeled return");

		if(this->check_scope_isnt_terminated(instr.return_stmt).isError()){ return Result::ERROR; }

		for(size_t i = this->scope.size() - 1; const sema::ScopeLevel::ID& target_scope_level_id : this->scope){
			EVO_DEFER([&](){ i -= 1; });

			if(i == this->scope.getCurrentObjectScopeIndex()){ break; }

			const sema::ScopeLevel& target_scope_level = 
				this->context.sema_buffer.scope_manager.getLevel(target_scope_level_id);

			if(target_scope_level.isDeferMainScope()){
				this->emit_error(
					Diagnostic::Code::SEMA_UNLABELED_RETURN_IN_DEFER,
					instr.return_stmt,
					"Unlabeled return statements are not allowed in [defer]/[errorDefer] blocks"
				);
				return Result::ERROR;
			}
		}

		const sema::Func& current_func = this->get_current_func();
		const BaseType::Function& current_func_type = this->context.getTypeManager().getFunction(current_func.typeID);


		auto return_value = std::optional<sema::Expr>();
		if(instr.return_stmt.value.is<std::monostate>()){ // return;
			if(current_func_type.returnsVoid() == false){
				this->emit_error(
					Diagnostic::Code::SEMA_INCORRECT_RETURN_STMT_KIND,
					instr.return_stmt,
					"Functions that have a return type other than `Void` must return a value"
				);
				return Result::ERROR;
			}

			if(current_func_type.hasNamedReturns()){
				this->emit_error(
					Diagnostic::Code::SEMA_INCORRECT_RETURN_STMT_KIND,
					instr.return_stmt,
					"Incorrect return statement kind for a function named return parameters",
					Diagnostic::Info("Initialize/set all return parameters, and use \"return...;\" instead")
				);
				return Result::ERROR;
			}
			
		}else if(instr.return_stmt.value.is<AST::Node>()){ // return {EXPRESSION};
			evo::debugAssert(instr.value.has_value(), "Return value needs to have value analyzed");

			if(current_func_type.returnsVoid()){
				this->emit_error(
					Diagnostic::Code::SEMA_INCORRECT_RETURN_STMT_KIND,
					instr.return_stmt,
					"Functions that have a return type of `Void` cannot return a value"
				);
				return Result::ERROR;
			}

			if(current_func_type.hasNamedReturns()){
				this->emit_error(
					Diagnostic::Code::SEMA_INCORRECT_RETURN_STMT_KIND,
					instr.return_stmt,
					"Incorrect return statement kind for a function with named return parameters",
					Diagnostic::Info("Initialize/set all return parameters, and use \"return...;\" instead")
				);
				return Result::ERROR;
			}


			TermInfo& return_value_term = this->get_term_info(*instr.value);

			if(return_value_term.is_ephemeral() == false){
				this->emit_error(
					Diagnostic::Code::SEMA_RETURN_NOT_EPHEMERAL,
					instr.return_stmt.value.as<AST::Node>(),
					"Value of return statement is not ephemeral"
				);
				return Result::ERROR;
			}

			if(this->type_check<true, true>(
				current_func_type.returnParams.front().typeID.asTypeID(),
				return_value_term,
				"Return",
				instr.return_stmt.value.as<AST::Node>()
			).ok == false){
				return Result::ERROR;
			}

			return_value = return_value_term.getExpr();
			
		}else{ // return...;
			evo::debugAssert(instr.return_stmt.value.is<Token::ID>(), "Unknown return kind");

			if(current_func_type.returnsVoid()){
				this->emit_error(
					Diagnostic::Code::SEMA_INCORRECT_RETURN_STMT_KIND,
					instr.return_stmt,
					"Functions that have a return type of `Void` cannot return a value"
				);
				return Result::ERROR;
			}

			if(current_func_type.hasNamedReturns() == false){
				this->emit_error(
					Diagnostic::Code::SEMA_INCORRECT_RETURN_STMT_KIND,
					instr.return_stmt,
					"Incorrect return statement kind for single unnamed return parameters",
					Diagnostic::Info("Use \"return {EXPRESSION};\" instead")
				);
				return Result::ERROR;
			}

			const SymbolProc::FuncInfo& func_info = this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>();
			if(func_info.num_members_of_initializing_are_uninit != 0){
				if(func_info.num_members_of_initializing_are_uninit == 1){
					this->emit_error(
						Diagnostic::Code::SEMA_NOT_ALL_MEMBERS_OF_OVERLOAD_OUTPUT_ARE_INIT,
						instr.return_stmt,
						"Not all members of the output are initialized",
						Diagnostic::Info("Missing 1 member")
					);

				}else{
					this->emit_error(
						Diagnostic::Code::SEMA_NOT_ALL_MEMBERS_OF_OVERLOAD_OUTPUT_ARE_INIT,
						instr.return_stmt,
						"Not all members of the output are initialized",
						Diagnostic::Info(
							std::format("Missing {} members", func_info.num_members_of_initializing_are_uninit)
						)
					);	
				}
				return Result::ERROR;
			}

			// check that all named returns are initialized
			auto infos = evo::SmallVector<Diagnostic::Info>();
			for(const auto [value_state_id, value_state_info] : this->get_current_scope_level().getValueStateInfos()){
				if(value_state_info.info.is<sema::ScopeLevel::ValueStateInfo::ModifyInfo>()){ continue; }
				if(value_state_info.state == sema::ScopeLevel::ValueState::INIT){ continue; }
				if(value_state_id.is<sema::ReturnParam::ID>() == false){ continue; }

				infos.emplace_back("This one:", this->get_location(value_state_id.as<sema::ReturnParam::ID>()));
			}

			if(infos.empty() == false){
				this->emit_error(
					Diagnostic::Code::SEMA_RETURN_NOT_ALL_RET_PARAMS_ARE_INIT,
					instr.return_stmt,
					"Not all return parameters are initialized",
					std::move(infos)
				);
				return Result::ERROR;
			}
		}

		this->add_auto_delete_calls<AutoDeleteMode::RETURN>();

		const sema::Return::ID sema_return_id = this->context.sema_buffer.createReturn(return_value, std::nullopt);

		this->get_current_scope_level().stmtBlock().emplace_back(sema_return_id);
		this->get_current_scope_level().setTerminated();

		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_labeled_return(const Instruction::LabeledReturn& instr) -> Result {
		evo::debugAssert(instr.return_stmt.label.has_value(), "Not a labeled return");

		if(this->check_scope_isnt_terminated(instr.return_stmt).isError()){ return Result::ERROR; }

		const Token::ID target_label_id = ASTBuffer::getIdent(*instr.return_stmt.label);
		const sema::ScopeLevel* scope_level = nullptr;

		const std::string_view return_label = this->source.getTokenBuffer()[target_label_id].getString();

		///////////////////////////////////
		// find scope level

		for(size_t i = this->scope.size() - 1; const sema::ScopeLevel::ID& target_scope_level_id : this->scope){
			EVO_DEFER([&](){ i -= 1; });

			if(i == this->scope.getCurrentObjectScopeIndex()){
				this->emit_error(
					Diagnostic::Code::SEMA_RETURN_LABEL_NOT_FOUND,
					*instr.return_stmt.label,
					std::format("Label \"{}\" not found", return_label)
				);
				return Result::ERROR;
			}

			scope_level = &this->context.sema_buffer.scope_manager.getLevel(target_scope_level_id);
			if(scope_level->hasLabel() == false){ continue; }
		
			const std::string_view scope_label = this->source.getTokenBuffer()[scope_level->getLabel()].getString();

			if(return_label == scope_label){ break; }
		}


		///////////////////////////////////
		// analyze return value(s)

		if(scope_level->getLabelNode().is<sema::BlockExpr::ID>() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_CANNOT_RETURN_TO_THIS_LABEL,
				*instr.return_stmt.label,
				std::format("Label \"{}\" cannot be returned to", return_label)
			);
			return Result::ERROR;
		}

		const sema::BlockExpr& target_block_expr = this->context.getSemaBuffer().getBlockExpr(
			scope_level->getLabelNode().as<sema::BlockExpr::ID>()
		);

		auto return_value = std::optional<sema::Expr>();
		if(instr.return_stmt.value.is<std::monostate>()){ // return;
			this->emit_error(
				Diagnostic::Code::SEMA_INCORRECT_RETURN_STMT_KIND,
				instr.return_stmt,
				"Expression block must return a value"
			);
			return Result::ERROR;

		}else if(instr.return_stmt.value.is<AST::Node>()){ // return {EXPRESSION};
			evo::debugAssert(instr.value.has_value(), "Return value needs to have value analyzed");

			if(target_block_expr.hasNamedOutputs()){
				this->emit_error(
					Diagnostic::Code::SEMA_INCORRECT_RETURN_STMT_KIND,
					instr.return_stmt,
					"Incorrect return statement kind for an expression block with named outputs",
					Diagnostic::Info(
						std::format(
							"Initialize/set all return parameters, and use \"return->{} ...;\" instead", return_label
						)
					)
				);
				return Result::ERROR;
			}


			TermInfo& return_value_term = this->get_term_info(*instr.value);

			if(return_value_term.is_ephemeral() == false){
				this->emit_error(
					Diagnostic::Code::SEMA_RETURN_NOT_EPHEMERAL,
					instr.return_stmt.value.as<AST::Node>(),
					"Return values must be ephemeral"
				);
				return Result::ERROR;
			}

			if(this->type_check<true, true>(
				target_block_expr.outputs.front().typeID,
				return_value_term,
				"Labeled return",
				instr.return_stmt.value.as<AST::Node>()
			).ok == false){
				return Result::ERROR;
			}

			return_value = return_value_term.getExpr();
			
		}else{ // return...;
			evo::debugAssert(instr.return_stmt.value.is<Token::ID>(), "Unknown return kind");
			evo::debugAssert(instr.value.has_value() == false, "`return...;` should not have return value");

			if(target_block_expr.hasNamedOutputs() == false){
				this->emit_error(
					Diagnostic::Code::SEMA_INCORRECT_RETURN_STMT_KIND,
					instr.return_stmt,
					"Incorrect return statement kind for single unnamed output value",
					Diagnostic::Info("Use \"return {EXPRESSION};\" instead")
				);
				return Result::ERROR;
			}


			// check that all named returns are initialized
			auto infos = evo::SmallVector<Diagnostic::Info>();
			for(const auto [value_state_id, value_state_info] : this->get_current_scope_level().getValueStateInfos()){
				if(value_state_info.info.is<sema::ScopeLevel::ValueStateInfo::ModifyInfo>()){ continue; }
				if(value_state_info.state == sema::ScopeLevel::ValueState::INIT){ continue; }
				if(value_state_id.is<sema::BlockExprOutput::ID>() == false){ continue; }

				infos.emplace_back("This one:", this->get_location(value_state_id.as<sema::BlockExprOutput::ID>()));
			}

			if(infos.empty() == false){
				this->emit_error(
					Diagnostic::Code::SEMA_RETURN_NOT_ALL_RET_PARAMS_ARE_INIT,
					instr.return_stmt,
					"Not all block expression return parameters are initialized",
					std::move(infos)
				);
				return Result::ERROR;
			}
		}


		this->add_auto_delete_calls<AutoDeleteMode::NORMAL>();

		const sema::Return::ID sema_return_id = this->context.sema_buffer.createReturn(return_value, target_label_id);

		this->get_current_scope_level().stmtBlock().emplace_back(sema_return_id);
		this->get_current_scope_level().setLabelTerminated();

		return Result::SUCCESS;
	}




	auto SemanticAnalyzer::instr_error(const Instruction::Error& instr) -> Result {
		if(this->check_scope_isnt_terminated(instr.error_stmt).isError()){ return Result::ERROR; }

		for(size_t i = this->scope.size() - 1; const sema::ScopeLevel::ID& target_scope_level_id : this->scope){
			EVO_DEFER([&](){ i -= 1; });

			if(i == this->scope.getCurrentObjectScopeIndex()){ break; }

			const sema::ScopeLevel& target_scope_level = 
				this->context.sema_buffer.scope_manager.getLevel(target_scope_level_id);

			if(target_scope_level.isDeferMainScope()){
				this->emit_error(
					Diagnostic::Code::SEMA_ERROR_IN_DEFER,
					instr.error_stmt,
					"Error statements are not allowed in [defer]/[errorDefer] blocks"
				);
				return Result::ERROR;
			}
		}

		const sema::Func& current_func = this->get_current_func();
		const BaseType::Function& current_func_type = this->context.getTypeManager().getFunction(current_func.typeID);


		if(current_func_type.hasErrorReturn() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_ERROR_IN_FUNC_WITHOUT_ERRORS,
				instr.error_stmt,
				"Cannot error return in a function that does not have error returns"
			);
			return Result::ERROR;
		}


		auto error_value = std::optional<sema::Expr>();
		if(instr.error_stmt.value.is<std::monostate>()){ // error;
			if(current_func_type.hasErrorReturnParams()){
				this->emit_error(
					Diagnostic::Code::SEMA_INCORRECT_RETURN_STMT_KIND,
					instr.error_stmt,
					"Incorrect error return statement kind for a function named error return parameters",
					Diagnostic::Info("Set all error return values and use \"error...;\" instead")
				);
				return Result::ERROR;
			}
			
		}else if(instr.error_stmt.value.is<AST::Node>()){ // error {EXPRESSION};
			evo::debugAssert(instr.value.has_value(), "error return value needs to have value analyzed");

			if(current_func_type.hasNamedErrorReturns()){
				this->emit_error(
					Diagnostic::Code::SEMA_INCORRECT_RETURN_STMT_KIND,
					instr.error_stmt,
					"Incorrect error return statement kind for a function named error return parameters",
					Diagnostic::Info("Set all error return values and use \"error...;\" instead")
				);
				return Result::ERROR;
			}


			TermInfo& error_value_term = this->get_term_info(*instr.value);

			if(error_value_term.is_ephemeral() == false){
				this->emit_error(
					Diagnostic::Code::SEMA_RETURN_NOT_EPHEMERAL,
					instr.error_stmt.value.as<AST::Node>(),
					"Error return values must be ephemeral"
				);
				return Result::ERROR;
			}

			if(this->type_check<true, true>(
				current_func_type.errorParams.front().typeID.asTypeID(),
				error_value_term,
				"Error return",
				instr.error_stmt.value.as<AST::Node>()
			).ok == false){
				return Result::ERROR;
			}

			error_value = error_value_term.getExpr();
			
		}else{ // error...;
			evo::debugAssert(instr.error_stmt.value.is<Token::ID>(), "Unknown return kind");

			if(current_func_type.hasNamedErrorReturns() == false){
				this->emit_error(
					Diagnostic::Code::SEMA_INCORRECT_RETURN_STMT_KIND,
					instr.error_stmt,
					"Incorrect error return statement kind for single unnamed error return parameters",
					Diagnostic::Info("Use \"error {EXPRESSION};\" instead")
				);
				return Result::ERROR;
			}

			// check that all named returns are initialized
			auto infos = evo::SmallVector<Diagnostic::Info>();
			for(const auto [value_state_id, value_state_info] : this->get_current_scope_level().getValueStateInfos()){
				if(value_state_info.info.is<sema::ScopeLevel::ValueStateInfo::ModifyInfo>()){ continue; }
				if(value_state_info.state == sema::ScopeLevel::ValueState::INIT){ continue; }
				if(value_state_id.is<sema::ErrorReturnParam::ID>() == false){ continue; }

				infos.emplace_back("This one:", this->get_location(value_state_id.as<sema::ErrorReturnParam::ID>()));
			}

			if(infos.empty() == false){
				this->emit_error(
					Diagnostic::Code::SEMA_ERROR_NOT_ALL_RET_PARAMS_ARE_INIT,
					instr.error_stmt,
					"Not all error return parameters are initialized",
					std::move(infos)
				);
				return Result::ERROR;
			}
		}

		this->add_auto_delete_calls<AutoDeleteMode::ERROR>();

		const sema::Error::ID sema_error_id = this->context.sema_buffer.createError(error_value);

		this->get_current_scope_level().stmtBlock().emplace_back(sema_error_id);
		this->get_current_scope_level().setTerminated();

		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_unreachable(const Instruction::Unreachable& instr) -> Result {
		if(this->check_scope_isnt_terminated(instr.keyword).isError()){ return Result::ERROR; }
		
		this->get_current_scope_level().stmtBlock().emplace_back(sema::Stmt::createUnreachable(instr.keyword));
		this->get_current_scope_level().setTerminated();

		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_break(const Instruction::Break& instr) -> Result {
		if(this->check_scope_isnt_terminated(instr.break_stmt).isError()){ return Result::ERROR; }

		if(instr.break_stmt.label.has_value()){
			const sema::ScopeLevel* scope_level = nullptr;

			const std::string_view return_label = this->source.getTokenBuffer()[*instr.break_stmt.label].getString();

			for(size_t i = this->scope.size() - 1; const sema::ScopeLevel::ID& target_scope_level_id : this->scope){
				EVO_DEFER([&](){ i -= 1; });

				if(i == this->scope.getCurrentObjectScopeIndex()){
					this->emit_error(
						Diagnostic::Code::SEMA_BREAK_LABEL_NOT_FOUND,
						*instr.break_stmt.label,
						std::format("Label \"{}\" not found", return_label)
					);
					return Result::ERROR;
				}

				scope_level = &this->context.sema_buffer.scope_manager.getLevel(target_scope_level_id);
				if(scope_level->hasLabel() == false){ continue; }
			
				const std::string_view scope_label = this->source.getTokenBuffer()[scope_level->getLabel()].getString();

				if(return_label == scope_label){ break; }
			}

			if(scope_level->isLoopMainScope() == false){
				this->emit_error(
					Diagnostic::Code::SEMA_CANNOT_BREAK_TO_THIS_LABEL,
					*instr.break_stmt.label,
					std::format("Cannot break to label \"{}\"", return_label)
				);
				return Result::ERROR;
			}

		}else{
			bool found_loop = false;

			for(size_t i = this->scope.size() - 1; const sema::ScopeLevel::ID& target_scope_level_id : this->scope){
				EVO_DEFER([&](){ i -= 1; });

				const sema::ScopeLevel& scope_level =
					this->context.sema_buffer.scope_manager.getLevel(target_scope_level_id);

				if(scope_level.isLoopMainScope()){
					found_loop = true;
					break;
				}
			}

			if(found_loop == false){
				this->emit_error(
					Diagnostic::Code::SEMA_NO_LOOP_TO_BREAK_TO,
					instr.break_stmt,
					"No loop to break to"
				);
				return Result::ERROR;
			}
		}

		this->add_auto_delete_calls<AutoDeleteMode::NORMAL>();
		const sema::Break::ID new_break_id = this->context.sema_buffer.createBreak(instr.break_stmt.label);
		this->get_current_scope_level().stmtBlock().emplace_back(new_break_id);
		this->get_current_scope_level().setTerminated();

		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_continue(const Instruction::Continue& instr) -> Result {
		if(this->check_scope_isnt_terminated(instr.continue_stmt).isError()){ return Result::ERROR; }

		if(instr.continue_stmt.label.has_value()){
			const sema::ScopeLevel* scope_level = nullptr;

			const std::string_view return_label = this->source.getTokenBuffer()[*instr.continue_stmt.label].getString();

			for(size_t i = this->scope.size() - 1; const sema::ScopeLevel::ID& target_scope_level_id : this->scope){
				EVO_DEFER([&](){ i -= 1; });

				if(i == this->scope.getCurrentObjectScopeIndex()){
					this->emit_error(
						Diagnostic::Code::SEMA_CONTINUE_LABEL_NOT_FOUND,
						*instr.continue_stmt.label,
						std::format("Label \"{}\" not found", return_label)
					);
					return Result::ERROR;
				}

				scope_level = &this->context.sema_buffer.scope_manager.getLevel(target_scope_level_id);
				if(scope_level->hasLabel() == false){ continue; }
			
				const std::string_view scope_label = this->source.getTokenBuffer()[scope_level->getLabel()].getString();

				if(return_label == scope_label){ break; }
			}

			if(scope_level->isLoopMainScope() == false){
				this->emit_error(
					Diagnostic::Code::SEMA_CANNOT_CONTINUE_TO_THIS_LABEL,
					*instr.continue_stmt.label,
					std::format("Cannot continue to label \"{}\"", return_label)
				);
				return Result::ERROR;
			}

		}else{
			bool found_loop = false;

			for(size_t i = this->scope.size() - 1; const sema::ScopeLevel::ID& target_scope_level_id : this->scope){
				EVO_DEFER([&](){ i -= 1; });

				const sema::ScopeLevel& scope_level =
					this->context.sema_buffer.scope_manager.getLevel(target_scope_level_id);

				if(scope_level.isLoopMainScope()){
					found_loop = true;
					break;
				}
			}

			if(found_loop == false){
				this->emit_error(
					Diagnostic::Code::SEMA_NO_LOOP_TO_CONTINUE_TO,
					instr.continue_stmt,
					"No loop to continue to"
				);
				return Result::ERROR;
			}
		}

		this->add_auto_delete_calls<AutoDeleteMode::NORMAL>();
		const sema::Continue::ID new_continue_id = this->context.sema_buffer.createContinue(instr.continue_stmt.label);
		this->get_current_scope_level().stmtBlock().emplace_back(new_continue_id);
		this->get_current_scope_level().setTerminated();

		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_delete(const Instruction::Delete& instr) -> Result {
		const TermInfo& target = this->get_term_info(instr.delete_expr);

		switch(target.value_state){
			case TermInfo::ValueState::NOT_APPLICABLE: {
				this->emit_error(
					Diagnostic::Code::SEMA_DELETE_ARG_INVALID,
					instr.delete_stmt.value,
					"Argument of [delete] statement is invalid"
				);
				return Result::ERROR;
			} break;

			case TermInfo::ValueState::INIT: {
				// do nothing, correct
			} break;

			case TermInfo::ValueState::INITIALIZING: {
				this->emit_error(
					Diagnostic::Code::SEMA_DELETE_ARG_INVALID,
					instr.delete_stmt.value,
					"Argument of [delete] statement is invalid as it isn't fully initialized"
				);
				return Result::ERROR;
			} break;

			case TermInfo::ValueState::UNINIT: {
				this->emit_error(
					Diagnostic::Code::SEMA_DELETE_ARG_INVALID,
					instr.delete_stmt.value,
					"Argument of [delete] statement is invalid as it is uninitialized"
				);
				return Result::ERROR;
			} break;

			case TermInfo::ValueState::MOVED_FROM: {
				if(this->get_project_config().warn.deleteMovedFromExpr){
					this->emit_warning(
						Diagnostic::Code::SEMA_WARN_DELETE_MOVED_FROM_EXPR,
						instr.delete_stmt.value,
						"Argument of [delete] statement was moved-from"
					);
				}
			} break;
		}


		if(
			this->get_project_config().warn.deleteTriviallyDeletableType
			&& this->context.getTypeManager().isTriviallyDeletable(target.type_id.as<TypeInfo::ID>())
		){
			this->emit_warning(
				Diagnostic::Code::SEMA_WARN_DELETE_TRIVIALLY_DELETABLE_TYPE,
				instr.delete_stmt.value,
				"Argument of [delete] statement is a trivially deletable type"
			);
		}


		this->get_special_member_stmt_dependents<SpecialMemberKind::DELETE>(
			target.type_id.as<TypeInfo::ID>(), this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>().dependent_funcs
		);
		this->get_current_scope_level().stmtBlock().emplace_back(
			this->context.sema_buffer.createDelete(target.getExpr(), target.type_id.as<TypeInfo::ID>())
		);

		this->set_ident_value_state_if_needed(target.getExpr(), sema::ScopeLevel::ValueState::UNINIT);

		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_begin_cond(const Instruction::BeginCond& instr) -> Result {
		TermInfo& cond = this->get_term_info(instr.cond_expr);

		if(cond.value_state != TermInfo::ValueState::INIT && cond.value_state != TermInfo::ValueState::NOT_APPLICABLE){
			this->emit_error(
				Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
				instr.conditional.cond,
				"Condition in [if] conditional must be initialized"
			);
			return Result::ERROR;
		}

		if(this->type_check<true, true>(
			TypeManager::getTypeBool(), cond, "Condition in [if] condtional", instr.conditional.cond
		).ok == false){
			return Result::ERROR;
		}

		if(this->get_project_config().warn.constexprIfCond && cond.value_stage == TermInfo::ValueStage::CONSTEXPR){
			this->emit_warning(
				Diagnostic::Code::SEMA_WARN_CONSTEXPR_IF_COND,
				instr.conditional.cond,
				"Condition in [if] condition is constexpr",
				Diagnostic::Info("Consider converting it to a [when] condition")
			);
		}


		const sema::Conditional::ID new_sema_conditional_id = 
			this->context.sema_buffer.createConditional(cond.getExpr());

		this->get_current_scope_level().stmtBlock().emplace_back(new_sema_conditional_id);

		this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>().subscopes.emplace(new_sema_conditional_id);

		sema::Conditional& new_sema_conditional = this->context.sema_buffer.conds[new_sema_conditional_id];
		this->push_scope_level(&new_sema_conditional.thenStmts);

		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_cond_no_else() -> Result {
		this->add_auto_delete_calls<AutoDeleteMode::NORMAL>();
		if(this->pop_scope_level().isError()){ return Result::ERROR; }
		this->get_current_scope_level().addSubScope();
		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_cond_else() -> Result {
		this->add_auto_delete_calls<AutoDeleteMode::NORMAL>();
		if(this->pop_scope_level().isError()){ return Result::ERROR; }

		const sema::Stmt current_cond_stmt = this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>().subscopes.top();
		sema::Conditional& current_conditional = this->context.sema_buffer.conds[current_cond_stmt.conditionalID()];

		this->push_scope_level(&current_conditional.elseStmts);

		return Result::SUCCESS;
	}

	auto SemanticAnalyzer::instr_cond_else_if() -> Result {
		this->add_auto_delete_calls<AutoDeleteMode::NORMAL>();
		if(this->pop_scope_level().isError()){ return Result::ERROR; }
		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_end_cond() -> Result {
		this->add_auto_delete_calls<AutoDeleteMode::NORMAL>();
		this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>().subscopes.pop();
		if(this->pop_scope_level().isError()){ return Result::ERROR; }
		return Result::SUCCESS;
	}

	auto SemanticAnalyzer::instr_end_cond_set(const Instruction::EndCondSet& instr) -> Result {
		sema::ScopeLevel& current_scope_level = this->get_current_scope_level();

		if(current_scope_level.isTerminated() && current_scope_level.stmtBlock().isTerminated() == false){
			current_scope_level.stmtBlock().setTerminated();
		}

		if(this->end_sub_scopes(this->get_location(instr.close_brace)).isError()){ return Result::ERROR; }
		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_begin_local_when_cond(const Instruction::BeginLocalWhenCond& instr) -> Result {
		TermInfo& cond = this->get_term_info(instr.cond_expr);

		if(this->type_check<true, true>(
			TypeManager::getTypeBool(), cond, "Condition in [when] condtional", instr.when_cond.cond
		).ok == false){
			return Result::ERROR;
		}

		const bool when_cond_value = this->context.getSemaBuffer().getBoolValue(cond.getExpr().boolValueID()).value;

		if(when_cond_value == false){
			this->symbol_proc.setInstructionIndex(instr.else_index);
		}

		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_end_local_when_cond(const Instruction::EndLocalWhenCond& instr) -> Result {
		this->symbol_proc.setInstructionIndex(instr.end_index);
		return Result::SUCCESS;
	}



	auto SemanticAnalyzer::instr_begin_while(const Instruction::BeginWhile& instr) -> Result {
		if(this->check_scope_isnt_terminated(instr.while_stmt).isError()){ return Result::ERROR; }

		TermInfo& cond_term_info = this->get_term_info(instr.cond_expr);

		if(
			cond_term_info.value_state != TermInfo::ValueState::INIT
			&& cond_term_info.value_state != TermInfo::ValueState::NOT_APPLICABLE
		){
			this->emit_error(
				Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
				instr.while_stmt.cond,
				"Condition in [while] loop must be initialized"
			);
			return Result::ERROR;
		}

		if(this->type_check<true, true>(
			TypeManager::getTypeBool(), cond_term_info, "Condition in [while] loop", instr.while_stmt.cond
		).ok == false){
			return Result::ERROR;
		}

		const AST::Block& while_block = this->source.getASTBuffer().getBlock(instr.while_stmt.block);

		const sema::While::ID sema_while_id = this->context.sema_buffer.createWhile(
			cond_term_info.getExpr(), while_block.label
		);
		this->get_current_scope_level().stmtBlock().emplace_back(sema_while_id);

		sema::While& sema_while = this->context.sema_buffer.whiles[sema_while_id];
		if(while_block.label.has_value()){
			this->push_scope_level(sema_while.block, *while_block.label, sema_while_id);
		}else{
			this->push_scope_level(&sema_while.block);
		}

		this->get_current_scope_level().setIsLoopMainScope();

		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_end_while(const Instruction::EndWhile& instr) -> Result {
		this->add_auto_delete_calls<AutoDeleteMode::NORMAL>();
		if(this->pop_scope_level().isError()){ return Result::ERROR; }
		if(this->end_sub_scopes(this->get_location(instr.close_brace)).isError()){ return Result::ERROR; }
		return Result::SUCCESS;
	}




	auto SemanticAnalyzer::instr_begin_defer(const Instruction::BeginDefer& instr) -> Result {
		if(this->check_scope_isnt_terminated(instr.defer_stmt).isError()){ return Result::ERROR; }

		const bool is_error_defer = 
			this->source.getTokenBuffer()[instr.defer_stmt.keyword].kind() == Token::Kind::KEYWORD_ERROR_DEFER;

		if(
			is_error_defer &&
			this->context.getTypeManager().getFunction(this->get_current_func().typeID).hasErrorReturn() == false
		){
			this->emit_error(
				Diagnostic::Code::SEMA_ERROR_DEFER_IN_NON_ERRORING_FUNC,
				instr.defer_stmt,
				"Functions that do not error cannot have [errorDefer] statements"
			);
			return Result::ERROR;
		}


		const sema::Defer::ID sema_defer_id = this->context.sema_buffer.createDefer(is_error_defer);
		this->get_current_scope_level().stmtBlock().emplace_back(sema_defer_id);

		sema::Defer& sema_defer = this->context.sema_buffer.defers[sema_defer_id];
		this->push_scope_level(&sema_defer.block);

		this->get_current_scope_level().setIsDeferMainScope();

		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_end_defer(const Instruction::EndDefer& instr) -> Result {
		this->add_auto_delete_calls<AutoDeleteMode::NORMAL>();
		if(this->pop_scope_level().isError()){ return Result::ERROR; }
		if(this->end_sub_scopes(this->get_location(instr.close_brace)).isError()){ return Result::ERROR; }
		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_begin_stmt_block(const Instruction::BeginStmtBlock& instr) -> Result {
		if(this->check_scope_isnt_terminated(instr.stmt_block).isError()){ return Result::ERROR; }

		const sema::BlockScope::ID block_scope_id = this->context.sema_buffer.createBlockScope();
		this->get_current_scope_level().stmtBlock().emplace_back(block_scope_id);

		sema::BlockScope& block_scope = this->context.sema_buffer.block_scopes[block_scope_id];

		this->push_scope_level(&block_scope.block);
		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_end_stmt_block(const Instruction::EndStmtBlock& instr) -> Result {
		this->add_auto_delete_calls<AutoDeleteMode::NORMAL>();
		if(this->pop_scope_level().isError()){ return Result::ERROR; }
		if(this->end_sub_scopes(this->get_location(instr.close_brace)).isError()){ return Result::ERROR; }
		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_func_call(const Instruction::FuncCall& instr) -> Result {
		if(this->check_scope_isnt_terminated(instr.func_call).isError()){ return Result::ERROR; }

		const TermInfo& target_term_info = this->get_term_info(instr.target);

		const evo::Expected<FuncCallImplData, bool> func_call_impl_res = this->func_call_impl<false, false>(
			instr.func_call, target_term_info, instr.args, instr.template_args
		);
		if(func_call_impl_res.has_value() == false){
			if(func_call_impl_res.error()){
				return Result::ERROR;
			}else{
				return Result::NEED_TO_WAIT;
			}
		}

		//////////////////////////////////////////////////////////////////////
		// 

		if(func_call_impl_res.value().selected_func_type.returnsVoid() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_DISCARDING_RETURNS,
				instr.func_call.target,
				"Discarding return value of function call"
			);
			return Result::ERROR;
		}

		if(this->get_current_func().isConstexpr && func_call_impl_res.value().is_src_func()){
			if(func_call_impl_res.value().selected_func->isConstexpr == false){
				this->emit_error(
					Diagnostic::Code::SEMA_FUNC_ISNT_CONSTEXPR,
					instr.func_call.target,
					"Cannot call a non-constexpr function within a constexpr function",
					Diagnostic::Info(
						"Called function was defined here:",
						this->get_location(*func_call_impl_res.value().selected_func_id)
					)
				);
				return Result::ERROR;
			}

			this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>().dependent_funcs.emplace(
				*func_call_impl_res.value().selected_func_id
			);
		}


		auto sema_args = evo::SmallVector<sema::Expr>();
		if(target_term_info.value_category == TermInfo::ValueCategory::METHOD_CALL){
			const sema::FakeTermInfo& fake_term_info = this->context.getSemaBuffer().getFakeTermInfo(
				target_term_info.getExpr().fakeTermInfoID()
			);

			if(func_call_impl_res.value().selected_func->isMethod(this->context)){
				sema_args.emplace_back(fake_term_info.expr);

			}else if(
				this->get_project_config().warn.methodCallOnNonMethod
				&& fake_term_info.expr.kind() != sema::Expr::Kind::PARAM
			){
				this->emit_warning(
					Diagnostic::Code::SEMA_WARN_METHOD_CALL_ON_NON_METHOD,
					instr.func_call,
					"Making a method call to a function that is not a method",
					evo::SmallVector<Diagnostic::Info>{
						Diagnostic::Info("Call the function through the type instead"), // TODO(FUTURE): better message
						Diagnostic::Info(
							"Function declared here:", this->get_location(*func_call_impl_res.value().selected_func_id)
						),
					}
				);
			}
		}

		for(const SymbolProc::TermInfoID& arg : instr.args){
			sema_args.emplace_back(this->get_term_info(arg).getExpr());
		}



		if(func_call_impl_res.value().is_src_func() == false) [[unlikely]] {
			const IntrinsicFunc::Kind intrinsic_kind = target_term_info.getExpr().intrinsicFuncID();

			const Context::IntrinsicFuncInfo& intrinsic_func_info = this->context.getIntrinsicFuncInfo(intrinsic_kind);


			if(this->get_current_func().isConstexpr){
				if(intrinsic_func_info.allowedInComptime == false){
					this->emit_error(
						Diagnostic::Code::SEMA_FUNC_ISNT_CONSTEXPR,
						instr.func_call.target,
						"Cannot call a non-constexpr function within a constexpr function"
					);
					return Result::ERROR;
				}

			}else{
				if(intrinsic_func_info.allowedInRuntime == false){
					this->emit_error(
						Diagnostic::Code::SEMA_FUNC_ISNT_RUNTIME,
						instr.func_call.target,
						"Cannot call a non-runtime function within a runtime function"
					);
					return Result::ERROR;
				}
			}

			switch(this->context.getConfig().mode){
				case Context::Config::Mode::COMPILE: {
					if(intrinsic_func_info.allowedInCompile == false){
						this->emit_error(
							Diagnostic::Code::SEMA_INVALID_MODE_FOR_INTRINSIC,
							instr.func_call.target,
							"Calling this intrinsic is not allowed in compile mode"
						);
						return Result::ERROR;
					}
				} break;

				case Context::Config::Mode::SCRIPTING: {
					if(intrinsic_func_info.allowedInScript == false){
						this->emit_error(
							Diagnostic::Code::SEMA_INVALID_MODE_FOR_INTRINSIC,
							instr.func_call.target,
							"Calling this intrinsic is not allowed in scripting mode"
						);
						return Result::ERROR;
					}
				} break;

				case Context::Config::Mode::BUILD_SYSTEM: {
					if(intrinsic_func_info.allowedInBuildSystem == false){
						this->emit_error(
							Diagnostic::Code::SEMA_INVALID_MODE_FOR_INTRINSIC,
							instr.func_call.target,
							"Calling this intrinsic is not allowed in build system mode"
						);
						return Result::ERROR;
					}
				} break;
			}


			const sema::FuncCall::ID sema_func_call_id = this->context.sema_buffer.createFuncCall(
				intrinsic_kind, std::move(sema_args)
			);

			this->get_current_scope_level().stmtBlock().emplace_back(sema_func_call_id);




		}else{
			for(size_t i = sema_args.size(); i < func_call_impl_res.value().selected_func->params.size(); i+=1){
				sema_args.emplace_back(*func_call_impl_res.value().selected_func->params[i].defaultValue);
			}

			if(target_term_info.value_category == TermInfo::ValueCategory::INTERFACE_CALL){
				const sema::FakeTermInfo& fake_term_info = this->context.getSemaBuffer().getFakeTermInfo(
					target_term_info.getExpr().fakeTermInfoID()
				);

				const TypeInfo& expr_type_info = this->context.getTypeManager().getTypeInfo(fake_term_info.typeID);
				const BaseType::Interface& target_interface =
					this->context.getTypeManager().getInterface(expr_type_info.baseTypeID().interfaceID());

				for(size_t i = 0; const sema::Func::ID method : target_interface.methods){
					if(method == *func_call_impl_res.value().selected_func_id){
						const sema::InterfaceCall::ID interface_call_id = this->context.sema_buffer.createInterfaceCall(
							fake_term_info.expr,
							func_call_impl_res.value().selected_func->typeID,
							expr_type_info.baseTypeID().interfaceID(),
							uint32_t(i),
							std::move(sema_args)
						);

						this->get_current_scope_level().stmtBlock().emplace_back(interface_call_id);
						break;
					}

					i += 1;
				}

			}else{
				const sema::FuncCall::ID sema_func_call_id = this->context.sema_buffer.createFuncCall(
					*func_call_impl_res.value().selected_func_id, std::move(sema_args)
				);

				this->get_current_scope_level().stmtBlock().emplace_back(sema_func_call_id);
			}

		}

		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_assignment(const Instruction::Assignment& instr) -> Result {
		if(this->check_scope_isnt_terminated(instr.infix).isError()){ return Result::ERROR; }

		TermInfo& lhs = this->get_term_info(instr.lhs);
		TermInfo& rhs = this->get_term_info(instr.rhs);

		if(lhs.is_concrete() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_ASSIGN_LHS_NOT_CONCRETE,
				instr.infix.lhs,
				"LHS of assignment must be concrete"
			);
			return Result::ERROR;
		}

		if(lhs.is_const() && lhs.value_state != TermInfo::ValueState::UNINIT){
			this->emit_error(
				Diagnostic::Code::SEMA_ASSIGN_LHS_NOT_MUTABLE,
				instr.infix.lhs,
				"LHS of assignment must be mutable"
			);
			return Result::ERROR;
		}

		if(lhs.value_state == TermInfo::ValueState::MOVED_FROM){
			this->emit_error(
				Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
				instr.infix.lhs,
				"LHS of assignment cannot have a value state of moved from"
			);
			return Result::ERROR;
		}



		const Token::Kind op_kind = this->source.getTokenBuffer()[instr.infix.opTokenID].kind();

		if(op_kind == Token::lookupKind("=")){
			if(rhs.is_ephemeral() == false && rhs.value_category != TermInfo::ValueCategory::NULL_VALUE){
				this->emit_error(
					Diagnostic::Code::SEMA_ASSIGN_RHS_NOT_EPHEMERAL,
					instr.infix.rhs,
					"RHS of assignment must be ephemeral or (if applicable) value [null]"
				);
				return Result::ERROR;
			}

			if(this->type_check<true, true>(
				lhs.type_id.as<TypeInfo::ID>(), rhs, std::format("RHS of [{}]", op_kind), instr.infix.rhs
			).ok == false){
				return Result::ERROR;
			}

			if(lhs.value_state == TermInfo::ValueState::UNINIT){
				this->set_ident_value_state_if_needed(lhs.getExpr(), sema::ScopeLevel::ValueState::INIT);

			}else{
				if(this->context.getTypeManager().isTriviallyDeletable(lhs.type_id.as<TypeInfo::ID>()) == false){
					this->get_special_member_stmt_dependents<SpecialMemberKind::DELETE>(
						lhs.type_id.as<TypeInfo::ID>(),
						this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>().dependent_funcs
					);
					this->get_current_scope_level().stmtBlock().emplace_back(
						this->context.sema_buffer.createDelete(lhs.getExpr(), lhs.type_id.as<TypeInfo::ID>())
					);
				}
			}

			this->get_current_scope_level().stmtBlock().emplace_back(
				this->context.sema_buffer.createAssign(lhs.getExpr(), rhs.getExpr())
			);


			return Result::SUCCESS;

		}else{
			const TypeInfo::ID lhs_actual_type_id = this->get_actual_type<false, true>(lhs.type_id.as<TypeInfo::ID>());
			const TypeInfo& lhs_actual_type = this->context.getTypeManager().getTypeInfo(lhs_actual_type_id);

			if(lhs_actual_type.baseTypeID().kind() == BaseType::Kind::STRUCT){
				const BaseType::Struct& lhs_struct =
					this->context.getTypeManager().getStruct(lhs_actual_type.baseTypeID().structID());

				const evo::Expected<sema::FuncCall::ID, Result> infix_overload_result = 
					this->infix_overload_impl(lhs_struct.infixOverloads, lhs, rhs, instr.infix);

				if(infix_overload_result.has_value() == false){
					return infix_overload_result.error();
				}

				this->get_current_scope_level().stmtBlock().emplace_back(infix_overload_result.value());
				return Result::SUCCESS;

			}else{
				using MathInfixKind = Instruction::MathInfixKind;

				switch(op_kind){
					case Token::lookupKind("+="): case Token::lookupKind("-="): case Token::lookupKind("*="):
					case Token::lookupKind("/="): case Token::lookupKind("%="): {
						const auto math_infix = Instruction::MathInfix<false, MathInfixKind::MATH>(
							instr.infix, instr.lhs, instr.rhs, instr.builtin_composite_expr_term_info_id
						);

						const Result result = this->instr_expr_math_infix(math_infix);
						if(result != Result::SUCCESS){ return result; }
					} break;


					case Token::lookupKind("+%="): case Token::lookupKind("+|="): case Token::lookupKind("-%="):
					case Token::lookupKind("-|="): case Token::lookupKind("*%="): case Token::lookupKind("*|="): {
						const auto math_infix = Instruction::MathInfix<false, MathInfixKind::INTEGRAL_MATH>(
							instr.infix, instr.lhs, instr.rhs, instr.builtin_composite_expr_term_info_id
						);

						const Result result = this->instr_expr_math_infix(math_infix);
						if(result != Result::SUCCESS){ return result; }
					} break;

					case Token::lookupKind("<<="): case Token::lookupKind("<<|="): case Token::lookupKind(">>="): {
						const auto math_infix = Instruction::MathInfix<false, MathInfixKind::SHIFT>(
							instr.infix, instr.lhs, instr.rhs, instr.builtin_composite_expr_term_info_id
						);

						const Result result = this->instr_expr_math_infix(math_infix);
						if(result != Result::SUCCESS){ return result; }
					} break;
					
					case Token::lookupKind("&="): case Token::lookupKind("|="): case Token::lookupKind("^="): {
						const auto math_infix = Instruction::MathInfix<false, MathInfixKind::BITWISE_LOGICAL>(
							instr.infix, instr.lhs, instr.rhs, instr.builtin_composite_expr_term_info_id
						);

						const Result result = this->instr_expr_math_infix(math_infix);
						if(result != Result::SUCCESS){ return result; }
					} break;
					
					default: {
						evo::debugFatalBreak("Unknown or unsupported composite assignment");
					} break;
				}

				this->get_current_scope_level().stmtBlock().emplace_back(
					this->context.sema_buffer.createAssign(
						lhs.getExpr(), this->get_term_info(instr.builtin_composite_expr_term_info_id).getExpr()
					)
				);

				return Result::SUCCESS;
			}
		}
	}


	auto SemanticAnalyzer::instr_assignment_new(const Instruction::AssignmentNew& instr) -> Result {
		if(this->check_scope_isnt_terminated(instr.infix).isError()){ return Result::ERROR; }

		const TermInfo& lhs = this->get_term_info(instr.lhs);

		const AST::New& ast_new = this->source.getASTBuffer().getNew(instr.infix.rhs);

		if(lhs.is_concrete() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_ASSIGN_LHS_NOT_CONCRETE,
				instr.infix.lhs,
				"LHS of assignment must be concrete"
			);
			return Result::ERROR;
		}

		if(lhs.is_const() && lhs.value_state != TermInfo::ValueState::UNINIT){
			this->emit_error(
				Diagnostic::Code::SEMA_ASSIGN_LHS_NOT_MUTABLE,
				instr.infix.lhs,
				"LHS of assignment must be mutable"
			);
			return Result::ERROR;
		}

		if(lhs.value_state == TermInfo::ValueState::MOVED_FROM){
			this->emit_error(
				Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
				instr.infix.lhs,
				"LHS of assignment cannot have a value state of moved from"
			);
			return Result::ERROR;
		}



		const TypeInfo::VoidableID target_type_id = this->get_type(instr.type_id);
		if(target_type_id.isVoid()){
			this->emit_error(
				Diagnostic::Code::SEMA_NEW_TYPE_VOID,
				this->source.getASTBuffer().getNew(instr.infix.rhs).type,
				"Operator [new] cannot accept type `Void`"
			);
			return Result::ERROR;
		}

		const TypeInfo& actual_target_type_info = this->context.getTypeManager().getTypeInfo(
			this->get_actual_type<true, true>(target_type_id.asTypeID())
		);


		if(actual_target_type_info.qualifiers().empty() == false){
			if(actual_target_type_info.isOptional()){
				if(
					lhs.value_state == TermInfo::ValueState::UNINIT
					&& this->context.getTypeManager().isTriviallyDeletable(target_type_id.asTypeID()) == false
				){
					this->emit_error(
						Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
						instr.infix.rhs,
						"assignment operator [new] for optional that is not trivially deletable is unimplemented"
					);
					return Result::ERROR;
				}


				if(instr.args.empty()){
					this->get_current_scope_level().stmtBlock().emplace_back(
						this->context.sema_buffer.createAssign(
							lhs.getExpr(), sema::Expr(this->context.sema_buffer.createNull(target_type_id.asTypeID()))
						)
					);

					if(lhs.value_state == TermInfo::ValueState::UNINIT){
						this->set_ident_value_state_if_needed(lhs.getExpr(), sema::ScopeLevel::ValueState::INIT);
					}

					return Result::SUCCESS;

				}else if(instr.args.size() == 1){
					TermInfo& arg = this->get_term_info(instr.args[0]);


					if(ast_new.args[0].label.has_value()){
						this->emit_error(
							Diagnostic::Code::SEMA_NEW_OPTIONAL_NO_MATCHING_OVERLOAD,
							ast_new.type,
							"No matching operator [new] overload for this type",
							Diagnostic::Info("No operator [new] of optional accepts arguments with labels")
						);
						return Result::ERROR;
					}

					if(arg.value_category == TermInfo::ValueCategory::NULL_VALUE){
						this->get_current_scope_level().stmtBlock().emplace_back(
							this->context.sema_buffer.createAssign(
								lhs.getExpr(),
								sema::Expr(this->context.sema_buffer.createNull(target_type_id.asTypeID()))
							)
						);

					}else{
						if(arg.is_ephemeral() == false){
							this->emit_error(
								Diagnostic::Code::SEMA_NEW_OPTIONAL_ARG_NOT_EPHEMERAL,
								ast_new.args[0].value,
								"Argument in operator [new] for optional must be ephemeral or [null]"
							);
							return Result::ERROR;
						}

						const TypeInfo::ID optional_held_type_id = this->context.type_manager.getOrCreateTypeInfo(
							TypeInfo(
								actual_target_type_info.baseTypeID(),
								evo::SmallVector<AST::Type::Qualifier>(
									actual_target_type_info.qualifiers().begin(),
									std::prev(actual_target_type_info.qualifiers().end())
								)
							)
						);

						if(this->type_check<true, true>(
							optional_held_type_id, arg, "Argument in operator [new] for optional", ast_new.args[0].value
						).ok == false){
							return Result::ERROR;
						}

						this->get_current_scope_level().stmtBlock().emplace_back(
							this->context.sema_buffer.createAssign(
								lhs.getExpr(),
								sema::Expr(
									this->context.sema_buffer.createConversionToOptional(
										arg.getExpr(), target_type_id.asTypeID()
									)
								)
							)
						);
					}


					if(lhs.value_state == TermInfo::ValueState::UNINIT){
						this->set_ident_value_state_if_needed(lhs.getExpr(), sema::ScopeLevel::ValueState::INIT);
					}

					return Result::SUCCESS;

				}else{
					this->emit_error(
						Diagnostic::Code::SEMA_NEW_OPTIONAL_NO_MATCHING_OVERLOAD,
						ast_new.type,
						"No matching operator [new] overload for this type",
						Diagnostic::Info("Too may arguments")
					);
					return Result::ERROR;
				}
			}


			this->emit_error(
				Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
				ast_new.type,
				"Operator [new] of this type is unimplemented"
			);
			return Result::ERROR;
		}


		switch(actual_target_type_info.baseTypeID().kind()){
			case BaseType::Kind::PRIMITIVE: {
				if(instr.args.empty()){
					const BaseType::Primitive& primitive =
						this->context.getTypeManager().getPrimitive(actual_target_type_info.baseTypeID().primitiveID());

					if(primitive.kind() == Token::Kind::TYPE_RAWPTR || primitive.kind() == Token::Kind::TYPE_TYPEID){
						this->emit_error(
							Diagnostic::Code::SEMA_NEW_PRIMITIVE_NO_MATCHING_OVERLOAD,
							ast_new.type,
							"No matching operator [new] overload for this type"
						);
						return Result::ERROR;
					}


					this->get_current_scope_level().stmtBlock().emplace_back(
						this->context.sema_buffer.createAssign(
							lhs.getExpr(),
							sema::Expr(this->context.sema_buffer.createDefaultInitPrimitive(
								actual_target_type_info.baseTypeID()
							))
						)
					);

					if(lhs.value_state == TermInfo::ValueState::UNINIT){
						this->set_ident_value_state_if_needed(lhs.getExpr(), sema::ScopeLevel::ValueState::INIT);
					}
					return Result::SUCCESS;
					
				}else if(instr.args.size() == 1){
					TermInfo& arg = this->get_term_info(instr.args[0]);

					if(arg.is_ephemeral() == false){
						this->emit_error(
							Diagnostic::Code::SEMA_NEW_PRIMITIVE_ARG_NOT_EPHEMERAL,
							ast_new.args[0].value,
							"Argument in operator [new] for primitive must be ephemeral"
						);
						return Result::ERROR;
					}

					if(this->type_check<true, true>(
						target_type_id.asTypeID(),
						arg,
						"Argument of operator [new] for primitive",
						ast_new.args[0].value
					).ok == false){
						return Result::ERROR;
					}

					this->get_current_scope_level().stmtBlock().emplace_back(
						this->context.sema_buffer.createAssign(lhs.getExpr(), arg.getExpr())
					);

					if(lhs.value_state == TermInfo::ValueState::UNINIT){
						this->set_ident_value_state_if_needed(lhs.getExpr(), sema::ScopeLevel::ValueState::INIT);
					}

					return Result::SUCCESS;
					
				}else{
					this->emit_error(
						Diagnostic::Code::SEMA_NEW_PRIMITIVE_NO_MATCHING_OVERLOAD,
						ast_new.type,
						"No matching operator [new] overload for this type",
						Diagnostic::Info("Too may arguments")
					);
					return Result::ERROR;
				}
			} break;

			case BaseType::Kind::ARRAY_REF: {
				if(instr.args.empty()){
					this->get_current_scope_level().stmtBlock().emplace_back(
						this->context.sema_buffer.createAssign(
							lhs.getExpr(),
							sema::Expr(this->context.sema_buffer.createDefaultInitArrayRef(
								actual_target_type_info.baseTypeID().arrayRefID()
							))
						)
					);

					if(lhs.value_state == TermInfo::ValueState::UNINIT){
						this->set_ident_value_state_if_needed(lhs.getExpr(), sema::ScopeLevel::ValueState::INIT);
					}

					return Result::SUCCESS;
				}

				const BaseType::ArrayRef& array_ref =
					this->context.getTypeManager().getArrayRef(actual_target_type_info.baseTypeID().arrayRefID());

				const size_t num_ref_ptrs = array_ref.getNumRefPtrs();

				if(instr.args.size() != num_ref_ptrs + 1){
					this->emit_error(
						Diagnostic::Code::SEMA_NEW_ARRAY_REF_NO_MATCHING_OVERLOAD,
						ast_new.type,
						"No matching operator [new] overload for this type",
						Diagnostic::Info(
							std::format("Expected {} arguments, got {}", num_ref_ptrs + 1, instr.args.size())
						)
					);
					return Result::ERROR;
				}


				if(this->get_term_info(instr.args[0]).is_ephemeral() == false){
					this->emit_error(
						Diagnostic::Code::SEMA_NEW_ARRAY_REF_ARG_NOT_EPHEMERAL,
						ast_new.args[0].value,
						"Argument in operator [new] for optional must be ephemeral"
					);
					return Result::ERROR;
				}

				const TypeInfo::ID array_ptr_type = this->context.type_manager.getOrCreateTypeInfo(
					this->context.getTypeManager().getTypeInfo(array_ref.elementTypeID)
						.copyWithPushedQualifier(AST::Type::Qualifier(true, array_ref.isReadOnly, false, false))
				);

				if(this->type_check<true, true>(
					array_ptr_type,
					this->get_term_info(instr.args[0]),
					"Pointer argument of operator [new] for array reference",
					ast_new.args[0].value
				).ok == false){
					return Result::ERROR;
				}

				if(ast_new.args[0].label.has_value()){
					this->emit_error(
						Diagnostic::Code::SEMA_NEW_ARRAY_REF_NO_MATCHING_OVERLOAD,
						ast_new.type,
						"No matching operator [new] overload for this type",
						Diagnostic::Info("No operator [new] of array reference accepts arguments with labels")
					);
				}


				for(size_t i = 1; i < num_ref_ptrs + 1; i+=1){
					TermInfo& arg = this->get_term_info(instr.args[i]);

					if(arg.is_ephemeral() == false){
						this->emit_error(
							Diagnostic::Code::SEMA_NEW_ARRAY_REF_ARG_NOT_EPHEMERAL,
							ast_new.args[i].value,
							"Argument in operator [new] for array reference must be ephemera"
						);
						return Result::ERROR;
					}

					if(this->type_check<true, true>(
						TypeManager::getTypeUSize(),
						arg,
						"Dimension argument of operator [new] for array reference",
						ast_new.args[i].value
					).ok == false){
						return Result::ERROR;
					}

					if(ast_new.args[i].label.has_value()){
						this->emit_error(
							Diagnostic::Code::SEMA_NEW_ARRAY_REF_NO_MATCHING_OVERLOAD,
							ast_new.type,
							"No matching operator [new] overload for this type",
							Diagnostic::Info("No operator [new] of array reference accepts arguments with labels")
						);
					}
				}


				auto dimensions = evo::SmallVector<evo::Variant<uint64_t, sema::Expr>>();
				dimensions.reserve(array_ref.dimensions.size());
				for(const BaseType::ArrayRef::Dimension& dimension : array_ref.dimensions){
					if(dimension.isPtr()){
						dimensions.emplace_back(this->get_term_info(instr.args[dimensions.size() + 1]).getExpr());
					}else{
						dimensions.emplace_back(dimension.length());
					}
				}

				this->get_current_scope_level().stmtBlock().emplace_back(
					this->context.sema_buffer.createAssign(
						lhs.getExpr(),
						sema::Expr(this->context.sema_buffer.createInitArrayRef(
							this->get_term_info(instr.args[0]).getExpr(), std::move(dimensions)
						))
					)
				);

				if(lhs.value_state == TermInfo::ValueState::UNINIT){
					this->set_ident_value_state_if_needed(lhs.getExpr(), sema::ScopeLevel::ValueState::INIT);
				}

				return Result::SUCCESS;
			} break;

			case BaseType::Kind::STRUCT: {
				// just break as will be done after switch
			} break;

			default: {
				this->emit_error(
					Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
					ast_new.type,
					"Operator [new] of this type is unimplemented"
				);
				return Result::ERROR;
			} break;
		}



		const BaseType::Struct& target_struct =
			this->context.getTypeManager().getStruct(actual_target_type_info.baseTypeID().structID());


		auto overloads = evo::SmallVector<SelectFuncOverloadFuncInfo>();
		auto args = evo::SmallVector<SelectFuncOverloadArgInfo>();

		const bool is_semantically_initialization = lhs.value_state == TermInfo::ValueState::UNINIT;
		const bool should_run_initialization = 
			is_semantically_initialization || target_struct.newAssignOverloads.empty();



		if(should_run_initialization){
			if(target_struct.newInitOverloads.empty()){
				if(target_struct.isTriviallyDefaultInitializable){
					if(lhs.value_state == TermInfo::ValueState::UNINIT){
						this->set_ident_value_state_if_needed(lhs.getExpr(), sema::ScopeLevel::ValueState::INIT);
					}
					return Result::SUCCESS;

				}else{
					this->emit_error(
						Diagnostic::Code::SEMA_NEW_STRUCT_NO_MATCHING_OVERLOAD,
						ast_new.type,
						"No matching operator [new] overload for this type"
					);
					return Result::ERROR;
				}
			}


			overloads.reserve(target_struct.newInitOverloads.size());
			for(const sema::Func::ID overload_id : target_struct.newInitOverloads){
				const sema::Func& overload = this->context.getSemaBuffer().getFunc(overload_id);

				overloads.emplace_back(overload_id, this->context.getTypeManager().getFunction(overload.typeID));
			}

			args.reserve(instr.args.size());
		}else{
			overloads.reserve(target_struct.newAssignOverloads.size());
			for(const sema::Func::ID overload_id : target_struct.newAssignOverloads){
				const sema::Func& overload = this->context.getSemaBuffer().getFunc(overload_id);

				overloads.emplace_back(overload_id, this->context.getTypeManager().getFunction(overload.typeID));
			}

			args.reserve(instr.args.size() + 1);
			args.emplace_back(this->get_term_info(instr.lhs), instr.infix.lhs, std::nullopt);
		}



		for(size_t i = 0; const SymbolProc::TermInfoID& arg_id : instr.args){
			args.emplace_back(this->get_term_info(arg_id), ast_new.args[i].value, ast_new.args[i].label);

			i += 1;
		}


		const evo::Result<size_t> selected_overload = this->select_func_overload(
			overloads, args, ast_new, !should_run_initialization, evo::SmallVector<Diagnostic::Info>()
		);
		if(selected_overload.isError()){ return Result::ERROR; }

		const sema::Func::ID selected_func_id = overloads[selected_overload.value()].func_id.as<sema::Func::ID>();
		const sema::Func& selected_func = this->context.getSemaBuffer().getFunc(selected_func_id);


		auto output_args = evo::SmallVector<sema::Expr>();
		if(should_run_initialization){
			output_args.reserve(selected_func.params.size());
		}else{
			output_args.reserve(selected_func.params.size() + 1);
			output_args.emplace_back(lhs.getExpr());
		}
		for(const SymbolProc::TermInfoID& arg_id : instr.args){
			output_args.emplace_back(this->get_term_info(arg_id).getExpr());
		}

		// default values
		for(size_t i = output_args.size(); i < selected_func.params.size(); i+=1){
			output_args.emplace_back(*selected_func.params[i].defaultValue);
		}


		const sema::FuncCall::ID created_func_call_id = this->context.sema_buffer.createFuncCall(
			selected_func_id, std::move(output_args)
		);

		this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>().dependent_funcs.emplace(selected_func_id);

		if(should_run_initialization){
			if(
				is_semantically_initialization == false
				&& this->context.getTypeManager().isTriviallyDeletable(actual_target_type_info.baseTypeID()) == false
			){
				this->emit_error(
					Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
					instr.infix.rhs,
					"assignment operator [new] that runs an initializer predicated by a destroy is unimplemented"
				);
				return Result::ERROR;
			}

			this->get_current_scope_level().stmtBlock().emplace_back(
				this->context.sema_buffer.createAssign(lhs.getExpr(), sema::Expr(created_func_call_id))
			);

		}else{
			if(this->context.getTypeManager().isTriviallyDeletable(lhs.type_id.as<TypeInfo::ID>()) == false){
				this->get_special_member_stmt_dependents<SpecialMemberKind::DELETE>(
					lhs.type_id.as<TypeInfo::ID>(),
					this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>().dependent_funcs
				);
				this->get_current_scope_level().stmtBlock().emplace_back(
					this->context.sema_buffer.createDelete(lhs.getExpr(), lhs.type_id.as<TypeInfo::ID>())
				);
			}
			this->get_current_scope_level().stmtBlock().emplace_back(sema::Stmt(created_func_call_id));
		}

		if(lhs.value_state == TermInfo::ValueState::UNINIT){
			this->set_ident_value_state_if_needed(lhs.getExpr(), sema::ScopeLevel::ValueState::INIT);
		}

		return Result::SUCCESS;
	}



	auto SemanticAnalyzer::instr_assignment_copy(const Instruction::AssignmentCopy& instr) -> Result {
		if(this->check_scope_isnt_terminated(instr.infix).isError()){ return Result::ERROR; }

		const TermInfo& lhs = this->get_term_info(instr.lhs);
		TermInfo& target = this->get_term_info(instr.target);

		if(lhs.is_concrete() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_ASSIGN_LHS_NOT_CONCRETE,
				instr.infix.lhs,
				"LHS of assignment must be concrete"
			);
			return Result::ERROR;
		}

		if(lhs.is_const() && lhs.value_state != TermInfo::ValueState::UNINIT){
			this->emit_error(
				Diagnostic::Code::SEMA_ASSIGN_LHS_NOT_MUTABLE,
				instr.infix.lhs,
				"LHS of assignment must be mutable"
			);
			return Result::ERROR;
		}

		if(lhs.value_state == TermInfo::ValueState::MOVED_FROM){
			this->emit_error(
				Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
				instr.infix.lhs,
				"LHS of assignment cannot have a value state of moved from"
			);
			return Result::ERROR;
		}





		if(target.is_concrete() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_COPY_ARG_NOT_CONCRETE,
				this->source.getASTBuffer().getPrefix(instr.infix.rhs).rhs,
				"Argument of operator [copy] must be concrete"
			);
			return Result::ERROR;
		}

		if(this->context.getTypeManager().isCopyable(target.type_id.as<TypeInfo::ID>()) == false){
			this->emit_error(
				Diagnostic::Code::SEMA_COPY_ARG_TYPE_NOT_COPYABLE,
				this->source.getASTBuffer().getPrefix(instr.infix.rhs).rhs,
				"Type of argument of operator [copy] is not copyable"
			);
			return Result::ERROR;
		}

		if(
			target.value_state != TermInfo::ValueState::INIT
			&& target.value_state != TermInfo::ValueState::NOT_APPLICABLE
		){
			this->emit_error(
				Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
				this->source.getASTBuffer().getPrefix(instr.infix.rhs).rhs,
				"Argument of operator [copy] must be initialized"
			);
			return Result::ERROR;
		}


		if(this->type_check<false, true>(
			lhs.type_id.as<TypeInfo::ID>(), target, "RHS of assignment", instr.infix.rhs
		).ok == false){
			return Result::ERROR;
		}


		const auto default_copy = [&]() -> void {
			if(this->context.getTypeManager().isTriviallyDeletable(target.type_id.as<TypeInfo::ID>()) == false){
				this->get_special_member_stmt_dependents<SpecialMemberKind::DELETE>(
					lhs.type_id.as<TypeInfo::ID>(),
					this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>().dependent_funcs
				);
				this->get_current_scope_level().stmtBlock().emplace_back(
					this->context.sema_buffer.createDelete(lhs.getExpr(), lhs.type_id.as<TypeInfo::ID>())
				);
			}

			this->get_current_scope_level().stmtBlock().emplace_back(
				this->context.sema_buffer.createAssign(
					lhs.getExpr(),
					sema::Expr(
						this->context.sema_buffer.createCopy(target.getExpr(), target.type_id.as<TypeInfo::ID>())
					)
				)
			);

			if(lhs.value_state == TermInfo::ValueState::UNINIT){
				this->set_ident_value_state_if_needed(lhs.getExpr(), sema::ScopeLevel::ValueState::INIT);
			}
		};

		const TypeInfo& target_type_info =
			this->context.getTypeManager().getTypeInfo(target.type_id.as<TypeInfo::ID>());

		if(target_type_info.qualifiers().empty() == false){
			default_copy();
			return Result::SUCCESS;
		}

		if(target_type_info.baseTypeID().kind() != BaseType::Kind::STRUCT){
			default_copy();
			return Result::SUCCESS;
		}

		const BaseType::Struct& struct_type =
			this->context.getTypeManager().getStruct(target_type_info.baseTypeID().structID());



		if(lhs.value_state == TermInfo::ValueState::UNINIT){
			const BaseType::Struct::DeletableOverload copy_init_overload = struct_type.copyInitOverload.load();

			if(copy_init_overload.wasDeleted){
				this->emit_error(
					Diagnostic::Code::SEMA_COPY_ARG_TYPE_NOT_COPYABLE,
					this->source.getASTBuffer().getInfix(instr.infix.rhs).rhs,
					"This type is not copyable as its operator [copy] was deleted"
				);
				return Result::ERROR;
			}

			if(copy_init_overload.funcID.has_value() == false){
				default_copy();
				return Result::SUCCESS;
			}

			this->get_current_scope_level().stmtBlock().emplace_back(
				this->context.sema_buffer.createAssign(
					lhs.getExpr(),
					sema::Expr(
						this->context.sema_buffer.createFuncCall(
							*copy_init_overload.funcID, evo::SmallVector<sema::Expr>{target.getExpr()}
						)
					)
				)
			);

			this->set_ident_value_state_if_needed(lhs.getExpr(), sema::ScopeLevel::ValueState::INIT);

			return Result::SUCCESS;

		}else{
			const std::optional<sema::FuncID> copy_assign_overload = struct_type.copyAssignOverload.load();

			if(copy_assign_overload.has_value()){
				this->get_current_scope_level().stmtBlock().emplace_back(
					this->context.sema_buffer.createFuncCall(
						*copy_assign_overload,
						evo::SmallVector<sema::Expr>{target.getExpr(), lhs.getExpr()}
					)
				);

				return Result::SUCCESS;
			}


			const BaseType::Struct::DeletableOverload copy_init_overload = struct_type.copyInitOverload.load();

			if(copy_init_overload.wasDeleted){
				this->emit_error(
					Diagnostic::Code::SEMA_COPY_ARG_TYPE_NOT_COPYABLE,
					this->source.getASTBuffer().getInfix(instr.infix.rhs).rhs,
					"This type is not copyable as its operator [copy] was deleted"
				);
				return Result::ERROR;			}


			if(copy_init_overload.funcID.has_value() == false){
				default_copy();
				return Result::SUCCESS;
			}

			if(this->context.getTypeManager().isTriviallyDeletable(target.type_id.as<TypeInfo::ID>()) == false){
				this->get_special_member_stmt_dependents<SpecialMemberKind::DELETE>(
					lhs.type_id.as<TypeInfo::ID>(),
					this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>().dependent_funcs
				);
				this->get_current_scope_level().stmtBlock().emplace_back(
					this->context.sema_buffer.createDelete(lhs.getExpr(), lhs.type_id.as<TypeInfo::ID>())
				);
			}

			this->get_current_scope_level().stmtBlock().emplace_back(
				this->context.sema_buffer.createAssign(
					lhs.getExpr(),
					sema::Expr(
						this->context.sema_buffer.createFuncCall(
							*copy_init_overload.funcID, evo::SmallVector<sema::Expr>{target.getExpr()}
						)
					)
				)
			);

			return Result::SUCCESS;
		}
	}




	auto SemanticAnalyzer::instr_assignment_move(const Instruction::AssignmentMove& instr) -> Result {
		if(this->check_scope_isnt_terminated(instr.infix).isError()){ return Result::ERROR; }

		const TermInfo& lhs = this->get_term_info(instr.lhs);
		TermInfo& target = this->get_term_info(instr.target);

		if(lhs.is_concrete() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_ASSIGN_LHS_NOT_CONCRETE,
				instr.infix.lhs,
				"LHS of assignment must be concrete"
			);
			return Result::ERROR;
		}

		if(lhs.is_const() && lhs.value_state != TermInfo::ValueState::UNINIT){
			this->emit_error(
				Diagnostic::Code::SEMA_ASSIGN_LHS_NOT_MUTABLE,
				instr.infix.lhs,
				"LHS of assignment must be mutable"
			);
			return Result::ERROR;
		}

		if(lhs.value_state == TermInfo::ValueState::MOVED_FROM){
			this->emit_error(
				Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
				instr.infix.lhs,
				"LHS of assignment cannot have a value state of moved from"
			);
			return Result::ERROR;
		}




		if(target.value_category != TermInfo::ValueCategory::CONCRETE_MUT){
			if(target.value_category == TermInfo::ValueCategory::FORWARDABLE){
				this->emit_error(
					Diagnostic::Code::SEMA_MOVE_ARG_IS_IN_PARAM,
					this->source.getASTBuffer().getPrefix(instr.infix.rhs).rhs,
					"Argument of operator [move] cannot be an in-parameter",
					Diagnostic::Info("Use operator [forward] instead")
				);
			}else if(target.is_concrete() == false){
				this->emit_error(
					Diagnostic::Code::SEMA_MOVE_ARG_NOT_CONCRETE,
					this->source.getASTBuffer().getPrefix(instr.infix.rhs).rhs,
					"Argument of operator [move] must be concrete"
				);
			}else{
				this->emit_error(
					Diagnostic::Code::SEMA_MOVE_ARG_NOT_MUTABLE,
					this->source.getASTBuffer().getPrefix(instr.infix.rhs).rhs,
					"Argument of operator [move] must be mutable"
				);
			}

			return Result::ERROR;
		}


		if(this->context.getTypeManager().isMovable(target.type_id.as<TypeInfo::ID>()) == false){
			this->emit_error(
				Diagnostic::Code::SEMA_MOVE_ARG_TYPE_NOT_MOVABLE,
				this->source.getASTBuffer().getPrefix(instr.infix.rhs).rhs,
				"Type of argument of operator [move] is not movable"
			);
			return Result::ERROR;
		}


		switch(target.value_state){
			case TermInfo::ValueState::NOT_APPLICABLE: {
				// do nothing...
			} break;

			case TermInfo::ValueState::INIT: {
				this->set_ident_value_state_if_needed(target.getExpr(), sema::ScopeLevel::ValueState::MOVED_FROM);
			} break;

			case TermInfo::ValueState::INITIALIZING: case TermInfo::ValueState::UNINIT: {
				this->emit_error(
					Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
					this->source.getASTBuffer().getPrefix(instr.infix.rhs).rhs,
					"Argument of operator [move] must be initialized"
				);
				return Result::ERROR;
			} break;

			case TermInfo::ValueState::MOVED_FROM: {
				this->emit_error(
					Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
					this->source.getASTBuffer().getPrefix(instr.infix.rhs).rhs,
					"Argument of operator [move] must be initialized",
					Diagnostic::Info("This argument was already moved from")
				);
				return Result::ERROR;
			} break;
		}


		if(this->type_check<false, true>(
			lhs.type_id.as<TypeInfo::ID>(), target, "RHS of assignment", instr.infix.rhs
		).ok == false){
			return Result::ERROR;
		}


		const auto default_move = [&]() -> void {
			if(this->context.getTypeManager().isTriviallyDeletable(target.type_id.as<TypeInfo::ID>()) == false){
				this->get_special_member_stmt_dependents<SpecialMemberKind::DELETE>(
					lhs.type_id.as<TypeInfo::ID>(),
					this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>().dependent_funcs
				);
				this->get_current_scope_level().stmtBlock().emplace_back(
					this->context.sema_buffer.createDelete(lhs.getExpr(), lhs.type_id.as<TypeInfo::ID>())
				);
			}

			this->get_current_scope_level().stmtBlock().emplace_back(
				this->context.sema_buffer.createAssign(
					lhs.getExpr(),
					sema::Expr(
						this->context.sema_buffer.createMove(target.getExpr(), target.type_id.as<TypeInfo::ID>())
					)
				)
			);

			if(lhs.value_state == TermInfo::ValueState::UNINIT){
				this->set_ident_value_state_if_needed(lhs.getExpr(), sema::ScopeLevel::ValueState::INIT);
			}
		};

		const TypeInfo& target_type_info =
			this->context.getTypeManager().getTypeInfo(target.type_id.as<TypeInfo::ID>());

		if(target_type_info.qualifiers().empty() == false){
			default_move();
			return Result::SUCCESS;
		}

		if(target_type_info.baseTypeID().kind() != BaseType::Kind::STRUCT){
			default_move();
			return Result::SUCCESS;
		}

		const BaseType::Struct& struct_type =
			this->context.getTypeManager().getStruct(target_type_info.baseTypeID().structID());



		if(lhs.value_state == TermInfo::ValueState::UNINIT){
			const BaseType::Struct::DeletableOverload move_init_overload = struct_type.moveInitOverload.load();

			if(move_init_overload.wasDeleted){
				this->emit_error(
					Diagnostic::Code::SEMA_COPY_ARG_TYPE_NOT_COPYABLE,
					this->source.getASTBuffer().getInfix(instr.infix.rhs).rhs,
					"This type is not movable as its operator [move] was deleted"
				);
				return Result::ERROR;
			}

			if(move_init_overload.funcID.has_value() == false){
				default_move();
				return Result::SUCCESS;
			}

			this->get_current_scope_level().stmtBlock().emplace_back(
				this->context.sema_buffer.createAssign(
					lhs.getExpr(),
					sema::Expr(
						this->context.sema_buffer.createFuncCall(
							*move_init_overload.funcID, evo::SmallVector<sema::Expr>{target.getExpr()}
						)
					)
				)
			);

			this->set_ident_value_state_if_needed(lhs.getExpr(), sema::ScopeLevel::ValueState::INIT);

			return Result::SUCCESS;

		}else{
			const std::optional<sema::FuncID> move_assign_overload = struct_type.moveAssignOverload.load();

			if(move_assign_overload.has_value()){
				this->get_current_scope_level().stmtBlock().emplace_back(
					this->context.sema_buffer.createFuncCall(
						*move_assign_overload,
						evo::SmallVector<sema::Expr>{target.getExpr(), lhs.getExpr()}
					)
				);

				return Result::SUCCESS;
			}


			const BaseType::Struct::DeletableOverload move_init_overload = struct_type.moveInitOverload.load();

			if(move_init_overload.wasDeleted){
				this->emit_error(
					Diagnostic::Code::SEMA_COPY_ARG_TYPE_NOT_COPYABLE,
					this->source.getASTBuffer().getInfix(instr.infix.rhs).rhs,
					"This type is not movable as its operator [move] was deleted"
				);
				return Result::ERROR;
			}


			if(move_init_overload.funcID.has_value() == false){
				default_move();
				return Result::SUCCESS;
			}

			if(this->context.getTypeManager().isTriviallyDeletable(target.type_id.as<TypeInfo::ID>()) == false){
				this->get_special_member_stmt_dependents<SpecialMemberKind::DELETE>(
					lhs.type_id.as<TypeInfo::ID>(),
					this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>().dependent_funcs
				);
				this->get_current_scope_level().stmtBlock().emplace_back(
					this->context.sema_buffer.createDelete(lhs.getExpr(), lhs.type_id.as<TypeInfo::ID>())
				);
			}

			this->get_current_scope_level().stmtBlock().emplace_back(
				this->context.sema_buffer.createAssign(
					lhs.getExpr(),
					sema::Expr(
						this->context.sema_buffer.createFuncCall(
							*move_init_overload.funcID, evo::SmallVector<sema::Expr>{target.getExpr()}
						)
					)
				)
			);

			return Result::SUCCESS;
		}
	}



	auto SemanticAnalyzer::instr_assignment_forward(const Instruction::AssignmentForward& instr) -> Result {
		const TermInfo& lhs = this->get_term_info(instr.lhs);
		const TermInfo& target = this->get_term_info(instr.target);

		if(lhs.is_concrete() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_ASSIGN_LHS_NOT_CONCRETE,
				instr.infix.lhs,
				"LHS of assignment must be concrete"
			);
			return Result::ERROR;
		}

		if(lhs.is_const() && lhs.value_state != TermInfo::ValueState::UNINIT){
			this->emit_error(
				Diagnostic::Code::SEMA_ASSIGN_LHS_NOT_MUTABLE,
				instr.infix.lhs,
				"LHS of assignment must be mutable"
			);
			return Result::ERROR;
		}

		if(lhs.value_state == TermInfo::ValueState::MOVED_FROM){
			this->emit_error(
				Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
				instr.infix.lhs,
				"LHS of assignment cannot have a value state of moved from"
			);
			return Result::ERROR;
		}

		if(target.value_category != TermInfo::ValueCategory::FORWARDABLE){
			this->emit_error(
				Diagnostic::Code::SEMA_MOVE_ARG_NOT_MUTABLE,
				instr.infix,
				"Argument of operator [forward] must be forwardable"
			);

			return Result::ERROR;
		}


		if(this->get_current_func().isConstexpr){
			if(
				this->context.getTypeManager().isCopyable(target.type_id.as<TypeInfo::ID>())
				&& this->context.getTypeManager().isConstexprCopyable(
					target.type_id.as<TypeInfo::ID>(), this->context.getSemaBuffer()
				) == false
			){
				this->emit_error(
					Diagnostic::Code::SEMA_COMPTIME_COPY_ARG_TYPE_NOT_CONSTEXPR_COPYABLE,
					instr.infix,
					"Type of argument of operator [forward] is not constexpr copyable"
				);
				return Result::ERROR;
			}

			if(
				this->context.getTypeManager().isMovable(target.type_id.as<TypeInfo::ID>())
				&& this->context.getTypeManager().isConstexprMovable(
					target.type_id.as<TypeInfo::ID>(), this->context.getSemaBuffer()
				) == false
			){
				this->emit_error(
					Diagnostic::Code::SEMA_COMPTIME_MOVE_ARG_TYPE_NOT_CONSTEXPR_MOVABLE,
					instr.infix,
					"Type of argument of operator [forward] is not constexpr movable"
				);
				return Result::ERROR;
			}
		}


		switch(target.value_state){
			case TermInfo::ValueState::NOT_APPLICABLE: {
				// do nothing...
			} break;

			case TermInfo::ValueState::INIT: {
				this->set_ident_value_state_if_needed(target.getExpr(), sema::ScopeLevel::ValueState::MOVED_FROM);
			} break;

			case TermInfo::ValueState::INITIALIZING: case TermInfo::ValueState::UNINIT: {
				this->emit_error(
					Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
					this->source.getASTBuffer().getInfix(instr.infix.rhs).rhs,
					"Argument of operator [forward] must be initialized"
				);
				return Result::ERROR;
			} break;

			case TermInfo::ValueState::MOVED_FROM: {
				this->emit_error(
					Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
					this->source.getASTBuffer().getInfix(instr.infix.rhs).rhs,
					"Argument of operator [forward] must be initialized",
					Diagnostic::Info("This argument was already forwarded")
				);
				return Result::ERROR;
			} break;
		}



		bool is_initialization = false;

		if(lhs.value_state == TermInfo::ValueState::UNINIT){
			this->set_ident_value_state_if_needed(lhs.getExpr(), sema::ScopeLevel::ValueState::INIT);
			is_initialization = true;
		}

		this->get_current_scope_level().stmtBlock().emplace_back(
			this->context.sema_buffer.createAssign(
				lhs.getExpr(),
				sema::Expr(
					this->context.sema_buffer.createForward(
						target.getExpr(), target.type_id.as<TypeInfo::ID>(), is_initialization
					)
				)
			)
		);

		return Result::SUCCESS;
	}



	auto SemanticAnalyzer::instr_multi_assign(const Instruction::MultiAssign& instr) -> Result {
		if(this->check_scope_isnt_terminated(instr.multi_assign).isError()){ return Result::ERROR; }

		TermInfo& value = this->get_term_info(instr.value);

		if(value.is_ephemeral() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_ASSIGN_RHS_NOT_EPHEMERAL,
				instr.multi_assign.value,
				"RHS of assignment must be ephemeral"
			);
			return Result::ERROR;
		}

		if(value.isMultiValue() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_MULTI_ASSIGN_RHS_NOT_MULTI,
				instr.multi_assign.value,
				"RHS of multi-assignment must multi-value"
			);
			return Result::ERROR;
		}


		if(value.type_id.as<evo::SmallVector<TypeInfo::ID>>().size() != instr.targets.size()){
			this->emit_error(
				Diagnostic::Code::SEMA_MULTI_ASSIGN_RHS_WRONG_NUM,
				instr.multi_assign.value,
				"RHS of multi-assignment has wrong number of assignment targets",
				Diagnostic::Info(
					std::format(
						"Expression requires {}, got {}",
						value.type_id.as<evo::SmallVector<TypeInfo::ID>>().size(),
						instr.targets.size()
					)
				)
			);
			return Result::ERROR;
		}


		auto targets = evo::SmallVector<evo::Variant<sema::Expr, TypeInfo::ID>>();
		targets.reserve(instr.targets.size());
		for(size_t i = 0; const std::optional<SymbolProc::TermInfoID> target_id : instr.targets){
			EVO_DEFER([&](){ i += 1; });

			if(target_id.has_value() == false){
				targets.emplace_back(value.type_id.as<evo::SmallVector<TypeInfo::ID>>()[i]);
				continue;
			}

			const TermInfo& target = this->get_term_info(*target_id);

			if(target.is_concrete() == false){
				this->emit_error(
					Diagnostic::Code::SEMA_ASSIGN_LHS_NOT_CONCRETE,
					instr.multi_assign.assigns[i],
					"LHS of assignment must be concrete"
				);
				return Result::ERROR;
			}

			if(target.is_const() && target.value_state != TermInfo::ValueState::UNINIT){
				this->emit_error(
					Diagnostic::Code::SEMA_ASSIGN_LHS_NOT_MUTABLE,
					instr.multi_assign.assigns[i],
					"LHS of assignment must be mutable"
				);
				return Result::ERROR;
			}

			if(this->type_check<true, true>(
				target.type_id.as<TypeInfo::ID>(), value, "RHS of assignment", instr.multi_assign, unsigned(i)
			).ok == false){
				return Result::ERROR;
			}

			targets.emplace_back(target.getExpr());

			if(target.value_state == TermInfo::ValueState::UNINIT){
				this->set_ident_value_state_if_needed(target.getExpr(), sema::ScopeLevel::ValueState::INIT);

			}else{
				if(this->context.getTypeManager().isTriviallyDeletable(target.type_id.as<TypeInfo::ID>()) == false){
					this->get_special_member_stmt_dependents<SpecialMemberKind::DELETE>(
						target.type_id.as<TypeInfo::ID>(),
						this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>().dependent_funcs
					);
					this->get_current_scope_level().stmtBlock().emplace_back(
						this->context.sema_buffer.createDelete(target.getExpr(), target.type_id.as<TypeInfo::ID>())
					);
				}
			}
		}

		this->get_current_scope_level().stmtBlock().emplace_back(
			this->context.sema_buffer.createMultiAssign(std::move(targets), value.getExpr())
		);

		return Result::SUCCESS;
	}



	auto SemanticAnalyzer::instr_discarding_assignment(const Instruction::DiscardingAssignment& instr) -> Result {
		if(this->check_scope_isnt_terminated(instr.infix).isError()){ return Result::ERROR; }

		const TermInfo& rhs = this->get_term_info(instr.rhs);

		if(rhs.isMultiValue()){
			auto targets = evo::SmallVector<evo::Variant<sema::Expr, TypeInfo::ID>>();
			targets.reserve(rhs.type_id.as<evo::SmallVector<TypeInfo::ID>>().size());

			for(TypeInfo::ID discard_type_id : rhs.type_id.as<evo::SmallVector<TypeInfo::ID>>()){
				targets.emplace_back(discard_type_id);
			}

			this->get_current_scope_level().stmtBlock().emplace_back(
				this->context.sema_buffer.createMultiAssign(std::move(targets), rhs.getExpr())
			);

		}else{
			this->get_current_scope_level().stmtBlock().emplace_back(
				this->context.sema_buffer.createAssign(std::nullopt, rhs.getExpr())
			);
		}

		return Result::SUCCESS;
	}



	auto SemanticAnalyzer::instr_try_else_begin(const Instruction::TryElseBegin& instr) -> Result {
		const AST::FuncCall& ast_func_call = this->source.getASTBuffer().getFuncCall(instr.try_else.attemptExpr);

		const TermInfo& target_term_info = this->get_term_info(instr.func_call_target);

		const evo::Expected<FuncCallImplData, bool> func_call_impl_res = this->func_call_impl<false, true>(
			ast_func_call, target_term_info, instr.func_call_args, instr.func_call_template_args
		);
		if(func_call_impl_res.has_value() == false){
			if(func_call_impl_res.error()){
				return Result::ERROR;
			}else{
				return Result::NEED_TO_WAIT;
			}
		}


		auto sema_args = evo::SmallVector<sema::Expr>();
		if(target_term_info.value_category == TermInfo::ValueCategory::METHOD_CALL){
			const sema::FakeTermInfo& fake_term_info = this->context.getSemaBuffer().getFakeTermInfo(
				target_term_info.getExpr().fakeTermInfoID()
			);

			if(func_call_impl_res.value().selected_func->isMethod(this->context)){
				sema_args.emplace_back(fake_term_info.expr);

			}else if( // TODO(FUTURE): make this warn on non-template params
				this->get_project_config().warn.methodCallOnNonMethod
				&& fake_term_info.expr.kind() != sema::Expr::Kind::PARAM
			){
				this->emit_warning(
					Diagnostic::Code::SEMA_WARN_METHOD_CALL_ON_NON_METHOD,
					ast_func_call,
					"Making a method call to a function that is not a method",
					evo::SmallVector<Diagnostic::Info>{
						Diagnostic::Info("Call the function through the type instead"), // TODO(FUTURE): better message
						Diagnostic::Info(
							"Function declared here:", this->get_location(*func_call_impl_res.value().selected_func_id)
						),
					}
				);
			}

		}else if(target_term_info.value_category == TermInfo::ValueCategory::INTERFACE_CALL){
			const sema::FakeTermInfo& fake_term_info = this->context.getSemaBuffer().getFakeTermInfo(
				target_term_info.getExpr().fakeTermInfoID()
			);

			if(func_call_impl_res.value().selected_func->isMethod(this->context)){
				const sema::Expr extract_this = sema::Expr(
					this->context.sema_buffer.createInterfacePtrExtractThis(fake_term_info.expr)
				);

				sema_args.emplace_back(
					sema::Expr(this->context.sema_buffer.createDeref(extract_this, TypeManager::getTypeRawPtr()))
				);
			}
		}


		for(const SymbolProc::TermInfoID& arg : instr.func_call_args){
			const TermInfo& arg_info = this->get_term_info(arg);
			sema_args.emplace_back(arg_info.getExpr());
		}


		// default values
		for(size_t i = sema_args.size(); i < func_call_impl_res.value().selected_func->params.size(); i+=1){
			sema_args.emplace_back(*func_call_impl_res.value().selected_func->params[i].defaultValue);
		}


		const BaseType::Function& selected_func_type = func_call_impl_res.value().selected_func_type;

		if(
			instr.try_else.exceptParams.size() != selected_func_type.errorParams.size()
			&& selected_func_type.errorParams[0].typeID.isVoid() == false
		){
			this->emit_error(
				Diagnostic::Code::SEMA_TRY_EXCEPT_PARAMS_WRONG_NUM,
				instr.try_else.elseTokenID,
				"Number of except parameters does not match attempt function call",
				Diagnostic::Info(
					std::format(
						"Expected {}, got {}", selected_func_type.errorParams.size(), instr.try_else.exceptParams.size()
					)
				)
			);
			return Result::ERROR;
		}


		auto except_params = evo::SmallVector<sema::ExceptParam::ID>();
		except_params.reserve(instr.try_else.exceptParams.size());
		for(size_t i = 0; const Token::ID except_param_token_id : instr.try_else.exceptParams){
			EVO_DEFER([&](){ i += 1; });

			const Token& except_param_token = this->source.getTokenBuffer()[except_param_token_id];
			
			if(except_param_token.kind() == Token::lookupKind("_")){ continue; }

			const std::string_view except_param_ident_str = except_param_token.getString();

			const sema::ExceptParam::ID except_param_id = this->context.sema_buffer.createExceptParam(
				instr.try_else.exceptParams[i], uint32_t(i), selected_func_type.errorParams[i].typeID.asTypeID()
			);
			except_params.emplace_back(except_param_id);

			if(this->add_ident_to_scope(
				except_param_ident_str, instr.try_else.exceptParams[i], except_param_id
			).isError()){
				return Result::ERROR;
			}

			this->get_current_scope_level().addIdentValueState(except_param_id, sema::ScopeLevel::ValueState::INIT);
		}



		if(target_term_info.value_category == TermInfo::ValueCategory::INTERFACE_CALL){
			const sema::FakeTermInfo& fake_term_info = this->context.getSemaBuffer().getFakeTermInfo(
				target_term_info.getExpr().fakeTermInfoID()
			);

			const TypeInfo& expr_type_info = this->context.getTypeManager().getTypeInfo(fake_term_info.typeID);
			const BaseType::Interface& target_interface =
				this->context.getTypeManager().getInterface(expr_type_info.baseTypeID().interfaceID());

			const sema::Func& selected_func =
				this->context.getSemaBuffer().getFunc(*func_call_impl_res.value().selected_func_id);

			for(size_t i = 0; const sema::Func::ID method : target_interface.methods){
				if(method == *func_call_impl_res.value().selected_func_id){
					const sema::TryElseInterface::ID sema_try_else_interface_id = 
						this->context.sema_buffer.createTryElseInterface(
							fake_term_info.expr,
							selected_func.typeID,
							expr_type_info.baseTypeID().interfaceID(),
							uint32_t(i),
							std::move(sema_args),
							std::move(except_params)
						);

					this->get_current_scope_level().stmtBlock().emplace_back(sema_try_else_interface_id);

					sema::TryElseInterface& sema_try_else_interface =
						this->context.sema_buffer.try_else_interfaces[sema_try_else_interface_id];
					this->push_scope_level(&sema_try_else_interface.elseBlock);
				}

				i += 1;
			}

		}else{
			this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>().dependent_funcs.emplace(
				*func_call_impl_res.value().selected_func_id
			);

			const sema::TryElse::ID sema_try_else_id = this->context.sema_buffer.createTryElse(
				*func_call_impl_res.value().selected_func_id, std::move(sema_args), std::move(except_params)
			);

			this->get_current_scope_level().stmtBlock().emplace_back(sema_try_else_id);

			sema::TryElse& sema_try_else = this->context.sema_buffer.try_elses[sema_try_else_id];
			this->push_scope_level(&sema_try_else.elseBlock);
		}


		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_try_else_end() -> Result {
		if(this->pop_scope_level().isError()){ return Result::ERROR; }
		return Result::SUCCESS;
	}



	auto SemanticAnalyzer::instr_type_to_term(const Instruction::TypeToTerm& instr) -> Result {
		this->return_term_info(instr.to,
			TermInfo::ValueCategory::TYPE, this->get_type(instr.from)
		);
		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_require_this_def() -> Result {
		const std::optional<sema::ScopeManager::Scope::ObjectScope> current_type_scope = 
			this->scope.getCurrentTypeScopeIfExists();

		if(current_type_scope->is<BaseType::Struct::ID>()){
			const BaseType::Struct::ID current_struct_type_id = current_type_scope->as<BaseType::Struct::ID>();
			const TypeInfo::ID current_type_id = this->context.type_manager.getOrCreateTypeInfo(
				TypeInfo(BaseType::ID(current_struct_type_id))
			);

			const std::optional<SymbolProc::ID> current_struct_symbol_proc =
				this->context.symbol_proc_manager.getTypeSymbolProc(current_type_id);

			const SymbolProc::WaitOnResult wait_on_result = this->context.symbol_proc_manager
				.getSymbolProc(*current_struct_symbol_proc)
				.waitOnDefIfNeeded(this->symbol_proc_id, this->context, *current_struct_symbol_proc);

			switch(wait_on_result){
				case SymbolProc::WaitOnResult::NOT_NEEDED:                return Result::SUCCESS;
				case SymbolProc::WaitOnResult::WAITING:                   return Result::NEED_TO_WAIT_BEFORE_NEXT_INSTR;
				case SymbolProc::WaitOnResult::WAS_ERRORED:               return Result::ERROR;
				case SymbolProc::WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND:evo::debugFatalBreak("Not possible");
				case SymbolProc::WaitOnResult::CIRCULAR_DEP_DETECTED:     return Result::ERROR;
			}

			evo::unreachable();

		}else if(current_type_scope->is<BaseType::Union::ID>()){
			const BaseType::Union::ID current_union_type_id = current_type_scope->as<BaseType::Union::ID>();
			const TypeInfo::ID current_type_id = this->context.type_manager.getOrCreateTypeInfo(
				TypeInfo(BaseType::ID(current_union_type_id))
			);

			const std::optional<SymbolProc::ID> current_union_symbol_proc =
				this->context.symbol_proc_manager.getTypeSymbolProc(current_type_id);

			const SymbolProc::WaitOnResult wait_on_result = this->context.symbol_proc_manager
				.getSymbolProc(*current_union_symbol_proc)
				.waitOnDefIfNeeded(this->symbol_proc_id, this->context, *current_union_symbol_proc);

			switch(wait_on_result){
				case SymbolProc::WaitOnResult::NOT_NEEDED:                return Result::SUCCESS;
				case SymbolProc::WaitOnResult::WAITING:                   return Result::NEED_TO_WAIT_BEFORE_NEXT_INSTR;
				case SymbolProc::WaitOnResult::WAS_ERRORED:               return Result::ERROR;
				case SymbolProc::WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND:evo::debugFatalBreak("Not possible");
				case SymbolProc::WaitOnResult::CIRCULAR_DEP_DETECTED:     return Result::ERROR;
			}

			evo::unreachable();

		}else if(current_type_scope->is<BaseType::Enum::ID>()){
			const BaseType::Enum::ID current_enum_type_id = current_type_scope->as<BaseType::Enum::ID>();
			const TypeInfo::ID current_type_id = this->context.type_manager.getOrCreateTypeInfo(
				TypeInfo(BaseType::ID(current_enum_type_id))
			);

			const std::optional<SymbolProc::ID> current_enum_symbol_proc =
				this->context.symbol_proc_manager.getTypeSymbolProc(current_type_id);

			const SymbolProc::WaitOnResult wait_on_result = this->context.symbol_proc_manager
				.getSymbolProc(*current_enum_symbol_proc)
				.waitOnDefIfNeeded(this->symbol_proc_id, this->context, *current_enum_symbol_proc);

			switch(wait_on_result){
				case SymbolProc::WaitOnResult::NOT_NEEDED:                return Result::SUCCESS;
				case SymbolProc::WaitOnResult::WAITING:                   return Result::NEED_TO_WAIT_BEFORE_NEXT_INSTR;
				case SymbolProc::WaitOnResult::WAS_ERRORED:               return Result::ERROR;
				case SymbolProc::WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND:evo::debugFatalBreak("Not possible");
				case SymbolProc::WaitOnResult::CIRCULAR_DEP_DETECTED:     return Result::ERROR;
			}

			evo::unreachable();

		}else{
			return Result::SUCCESS;
		}
	}


	auto SemanticAnalyzer::instr_wait_on_sub_symbol_proc_def(const Instruction::WaitOnSubSymbolProcDef& instr)
	-> Result {
		SymbolProc& sub_symbol_proc = this->context.symbol_proc_manager.getSymbolProc(instr.symbol_proc_id);

		sub_symbol_proc.sema_scope_id = 
			this->context.sema_buffer.scope_manager.copyScope(*this->symbol_proc.sema_scope_id);


		{
			const auto lock = std::scoped_lock(sub_symbol_proc.waiting_for_lock);
			sub_symbol_proc.setStatusInQueue();
			this->context.add_task_to_work_manager(instr.symbol_proc_id);
		}


		const SymbolProc::WaitOnResult wait_on_result = 
			sub_symbol_proc.waitOnDefIfNeeded(this->symbol_proc_id, this->context, instr.symbol_proc_id);

		switch(wait_on_result){
			case SymbolProc::WaitOnResult::NOT_NEEDED:                 return Result::SUCCESS;
			case SymbolProc::WaitOnResult::WAITING:                    return Result::NEED_TO_WAIT_BEFORE_NEXT_INSTR;
			case SymbolProc::WaitOnResult::WAS_ERRORED:                return Result::ERROR;
			case SymbolProc::WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND: evo::debugFatalBreak("Not possible");
			case SymbolProc::WaitOnResult::CIRCULAR_DEP_DETECTED:      return Result::ERROR;
		}

		evo::unreachable();
	}


	template<bool IS_CONSTEXPR, bool ERRORS>
	auto SemanticAnalyzer::instr_func_call_expr(const Instruction::FuncCallExpr<IS_CONSTEXPR, ERRORS>& instr)
	-> Result {
		const TermInfo& target_term_info = this->get_term_info(instr.target);

		const evo::Expected<FuncCallImplData, bool> func_call_impl_res = this->func_call_impl<IS_CONSTEXPR, ERRORS>(
			instr.func_call, target_term_info, instr.args, instr.template_args
		);
		if(func_call_impl_res.has_value() == false){
			if(func_call_impl_res.error()){
				return Result::ERROR;
			}else{
				return Result::NEED_TO_WAIT;
			}
		}

		auto sema_args = evo::SmallVector<sema::Expr>();
		if(target_term_info.value_category == TermInfo::ValueCategory::METHOD_CALL){
			const sema::FakeTermInfo& fake_term_info = this->context.getSemaBuffer().getFakeTermInfo(
				target_term_info.getExpr().fakeTermInfoID()
			);

			if(func_call_impl_res.value().selected_func->isMethod(this->context)){
				sema_args.emplace_back(fake_term_info.expr);

			}else if( // TODO(FUTURE): make this warn on non-template params
				this->get_project_config().warn.methodCallOnNonMethod
				&& fake_term_info.expr.kind() != sema::Expr::Kind::PARAM
			){
				this->emit_warning(
					Diagnostic::Code::SEMA_WARN_METHOD_CALL_ON_NON_METHOD,
					instr.func_call,
					"Making a method call to a function that is not a method",
					evo::SmallVector<Diagnostic::Info>{
						Diagnostic::Info("Call the function through the type instead"), // TODO(FUTURE): better message
						Diagnostic::Info(
							"Function declared here:", this->get_location(*func_call_impl_res.value().selected_func_id)
						),
					}
				);
			}

		}else if(target_term_info.value_category == TermInfo::ValueCategory::INTERFACE_CALL){
			const sema::FakeTermInfo& fake_term_info = this->context.getSemaBuffer().getFakeTermInfo(
				target_term_info.getExpr().fakeTermInfoID()
			);

			if(func_call_impl_res.value().selected_func->isMethod(this->context)){
				const sema::Expr extract_this = sema::Expr(
					this->context.sema_buffer.createInterfacePtrExtractThis(fake_term_info.expr)
				);

				sema_args.emplace_back(
					sema::Expr(this->context.sema_buffer.createDeref(extract_this, TypeManager::getTypeRawPtr()))
				);
			}
		}


		bool all_args_are_constexpr = true;
		for(const SymbolProc::TermInfoID& arg : instr.args){
			const TermInfo& arg_info = this->get_term_info(arg);
			sema_args.emplace_back(arg_info.getExpr());
			if(arg_info.value_stage != TermInfo::ValueStage::CONSTEXPR){ all_args_are_constexpr = false; }
		}


		if(target_term_info.value_category == TermInfo::ValueCategory::BUILTIN_TYPE_METHOD){
			return this->builtin_type_method_call(target_term_info, std::move(sema_args), instr.output);
		}

		if(func_call_impl_res.value().is_src_func() == false) [[unlikely]] {
			const IntrinsicFunc::Kind intrinsic_kind = target_term_info.getExpr().intrinsicFuncID();

			const Context::IntrinsicFuncInfo& intrinsic_func_info = this->context.getIntrinsicFuncInfo(intrinsic_kind);

			if(this->get_current_func().isConstexpr){
				if(intrinsic_func_info.allowedInComptime == false){
					this->emit_error(
						Diagnostic::Code::SEMA_FUNC_ISNT_CONSTEXPR,
						instr.func_call.target,
						"Cannot call a non-constexpr function within a constexpr function"
					);
					return Result::ERROR;
				}

			}else{
				if(intrinsic_func_info.allowedInRuntime == false){
					this->emit_error(
						Diagnostic::Code::SEMA_FUNC_ISNT_RUNTIME,
						instr.func_call.target,
						"Cannot call a non-runtime function within a runtime function"
					);
					return Result::ERROR;
				}
			}

			switch(this->context.getConfig().mode){
				case Context::Config::Mode::COMPILE: {
					if(intrinsic_func_info.allowedInCompile == false){
						this->emit_error(
							Diagnostic::Code::SEMA_INVALID_MODE_FOR_INTRINSIC,
							instr.func_call.target,
							"Calling this intrinsic is not allowed in compile mode"
						);
						return Result::ERROR;
					}
				} break;

				case Context::Config::Mode::SCRIPTING: {
					if(intrinsic_func_info.allowedInScript == false){
						this->emit_error(
							Diagnostic::Code::SEMA_INVALID_MODE_FOR_INTRINSIC,
							instr.func_call.target,
							"Calling this intrinsic is not allowed in scripting mode"
						);
						return Result::ERROR;
					}
				} break;

				case Context::Config::Mode::BUILD_SYSTEM: {
					if(intrinsic_func_info.allowedInBuildSystem == false){
						this->emit_error(
							Diagnostic::Code::SEMA_INVALID_MODE_FOR_INTRINSIC,
							instr.func_call.target,
							"Calling this intrinsic is not allowed in build system mode"
						);
						return Result::ERROR;
					}
				} break;
			}

			const sema::FuncCall::ID sema_func_call_id = this->context.sema_buffer.createFuncCall(
				intrinsic_kind, std::move(sema_args)
			);


			const TermInfo::ValueStage value_stage = [&](){
				if constexpr(IS_CONSTEXPR){
					return TermInfo::ValueStage::CONSTEXPR;
				}else{
					if(this->get_current_func().isConstexpr){
						return TermInfo::ValueStage::COMPTIME;
					}else{
						return TermInfo::ValueStage::RUNTIME;
					}
				}
			}();

			const evo::SmallVector<BaseType::Function::ReturnParam>& selected_func_type_return_params = 
				func_call_impl_res.value().selected_func_type.returnParams;

			if(selected_func_type_return_params.size() == 1){ // single return
				this->return_term_info(instr.output,
					TermInfo::ValueCategory::EPHEMERAL,
					value_stage,
					TermInfo::ValueState::NOT_APPLICABLE,
					selected_func_type_return_params[0].typeID.asTypeID(),
					sema::Expr(sema_func_call_id)
				);
				
			}else{ // multi-return
				auto return_types = evo::SmallVector<TypeInfo::ID>();
				return_types.reserve(selected_func_type_return_params.size());
				for(const BaseType::Function::ReturnParam& return_param : selected_func_type_return_params){
					return_types.emplace_back(return_param.typeID.asTypeID());
				}

				this->return_term_info(instr.output,
					TermInfo::ValueCategory::EPHEMERAL,
					value_stage,
					TermInfo::ValueState::NOT_APPLICABLE,
					std::move(return_types),
					sema::Expr(sema_func_call_id)
				);
			}


			if constexpr(IS_CONSTEXPR){
				evo::debugFatalBreak("No constexpr non-templated intrinsics exist");

			}else{
				if(this->get_current_func().isConstexpr){
					if(func_call_impl_res.value().selected_func->isConstexpr == false){
						this->emit_error(
							Diagnostic::Code::SEMA_FUNC_ISNT_CONSTEXPR,
							instr.func_call.target,
							"Cannot call a non-constexpr function within a constexpr function",
							Diagnostic::Info(
								"Called function was defined here:",
								this->get_location(*func_call_impl_res.value().selected_func_id)
							)
						);
						return Result::ERROR;
					}
				}

				return Result::SUCCESS;
			}
		}


		// default values
		for(size_t i = sema_args.size(); i < func_call_impl_res.value().selected_func->params.size(); i+=1){
			sema_args.emplace_back(*func_call_impl_res.value().selected_func->params[i].defaultValue);
		}


		if(target_term_info.value_category == TermInfo::ValueCategory::INTERFACE_CALL){
			return this->interface_func_call<IS_CONSTEXPR>(
				target_term_info, std::move(sema_args), *func_call_impl_res.value().selected_func_id, instr.output
			);
		}


		const sema::FuncCall::ID sema_func_call_id = this->context.sema_buffer.createFuncCall(
			*func_call_impl_res.value().selected_func_id, std::move(sema_args)
		);

		const TermInfo::ValueStage value_stage = [&](){
			if constexpr(IS_CONSTEXPR){
				return TermInfo::ValueStage::CONSTEXPR;
			}else{
				if(all_args_are_constexpr && func_call_impl_res.value().selected_func->isConstexpr){
					return TermInfo::ValueStage::CONSTEXPR;
				}

				if(this->get_current_func().isConstexpr){
					return TermInfo::ValueStage::COMPTIME;
				}else{
					return TermInfo::ValueStage::RUNTIME;
				}
			}
		}();

		const evo::SmallVector<BaseType::Function::ReturnParam>& selected_func_type_return_params = 
			func_call_impl_res.value().selected_func_type.returnParams;

		if(selected_func_type_return_params.size() == 1){ // single return
			this->return_term_info(instr.output,
				TermInfo::ValueCategory::EPHEMERAL,
				value_stage,
				TermInfo::ValueState::NOT_APPLICABLE,
				selected_func_type_return_params[0].typeID.asTypeID(),
				sema::Expr(sema_func_call_id)
			);
			
		}else{ // multi-return
			auto return_types = evo::SmallVector<TypeInfo::ID>();
			return_types.reserve(selected_func_type_return_params.size());
			for(const BaseType::Function::ReturnParam& return_param : selected_func_type_return_params){
				return_types.emplace_back(return_param.typeID.asTypeID());
			}

			this->return_term_info(instr.output,
				TermInfo::ValueCategory::EPHEMERAL,
				value_stage,
				TermInfo::ValueState::NOT_APPLICABLE,
				std::move(return_types),
				sema::Expr(sema_func_call_id)
			);
		}

		if constexpr(IS_CONSTEXPR){
			if(func_call_impl_res.value().selected_func->isConstexpr == false){
				this->emit_error(
					Diagnostic::Code::SEMA_FUNC_ISNT_CONSTEXPR,
					instr.func_call.target,
					"Constexpr value cannot be a call to a function that is not constexpr",
					Diagnostic::Info(
						"Called function was defined here:",
						this->get_location(*func_call_impl_res.value().selected_func_id)
					)
				);
				return Result::ERROR;
			}


			SymbolProc& selected_func_symbol_proc = this->context.symbol_proc_manager.getSymbolProc(
				*func_call_impl_res.value().selected_func->symbolProcID
			);

			const SymbolProc::WaitOnResult wait_on_result = selected_func_symbol_proc.waitOnPIRDefIfNeeded(
				this->symbol_proc_id, this->context, *func_call_impl_res.value().selected_func->symbolProcID
			);

			switch(wait_on_result){
				case SymbolProc::WaitOnResult::NOT_NEEDED:                break;
				case SymbolProc::WaitOnResult::WAITING:                   return Result::NEED_TO_WAIT_BEFORE_NEXT_INSTR;
				case SymbolProc::WaitOnResult::WAS_ERRORED:               return Result::ERROR;
				case SymbolProc::WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND:evo::debugFatalBreak("Shouldn't be possible");
				case SymbolProc::WaitOnResult::CIRCULAR_DEP_DETECTED:     evo::debugFatalBreak("Shouldn't be possible");
			}

			return Result::SUCCESS;

		}else{
			if(this->get_current_func().isConstexpr){
				if(func_call_impl_res.value().selected_func->isConstexpr == false){
					this->emit_error(
						Diagnostic::Code::SEMA_FUNC_ISNT_CONSTEXPR,
						instr.func_call.target,
						"Cannot call a non-constexpr function within a constexpr function",
						Diagnostic::Info(
							"Called function was defined here:",
							this->get_location(*func_call_impl_res.value().selected_func_id)
						)
					);
					return Result::ERROR;
				}

				this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>().dependent_funcs.emplace(
					*func_call_impl_res.value().selected_func_id
				);
			}

			return Result::SUCCESS;
		}
	}


	auto SemanticAnalyzer::builtin_type_method_call(
		const TermInfo& target_term_info, evo::SmallVector<sema::Expr>&& args, SymbolProc::TermInfoID output
	) -> Result {
		evo::debugAssert(args.empty(), "None of these methods take any args");
		std::ignore = args;


		const TermInfo::BuiltinTypeMethod& builtin_type_method =
			target_term_info.type_id.as<TermInfo::BuiltinTypeMethod>();

		const sema::FakeTermInfo& fake_term_info = 
			this->context.getSemaBuffer().getFakeTermInfo(target_term_info.getExpr().fakeTermInfoID());

		switch(builtin_type_method.kind){
			case TermInfo::BuiltinTypeMethod::Kind::OPT_EXTRACT: {
				const TypeInfo::ID held_type_id = this->context.type_manager.getOrCreateTypeInfo(
					this->context.getTypeManager().getTypeInfo(fake_term_info.typeID).copyWithPoppedQualifier()
				);

				this->return_term_info(output,
					TermInfo::ValueCategory::EPHEMERAL,
					TermInfo::convertValueStage(fake_term_info.valueStage),
					TermInfo::ValueState::NOT_APPLICABLE,
					held_type_id,
					sema::Expr(
						this->context.sema_buffer.createOptionalExtract(fake_term_info.expr, fake_term_info.typeID)
					)
				);
				return Result::SUCCESS;
			} break;

			case TermInfo::BuiltinTypeMethod::Kind::ARRAY_SIZE: {
				const BaseType::Array& array_type = this->context.getTypeManager().getArray(
					this->context.getTypeManager().getTypeInfo(fake_term_info.typeID).baseTypeID().arrayID()
				);

				uint64_t size = 1;
				for(uint64_t dimension : array_type.dimensions){
					size *= dimension;
				}

				const sema::IntValue::ID created_int_value = this->context.sema_buffer.createIntValue(
					core::GenericInt::create<uint64_t>(size),
					this->context.getTypeManager().getTypeInfo(TypeManager::getTypeUSize()).baseTypeID()
				);

				this->return_term_info(output,
					TermInfo::ValueCategory::EPHEMERAL,
					TermInfo::convertValueStage(fake_term_info.valueStage),
					TermInfo::ValueState::NOT_APPLICABLE,
					TypeManager::getTypeUSize(),
					sema::Expr(created_int_value)
				);
				return Result::SUCCESS;
			} break;

			case TermInfo::BuiltinTypeMethod::Kind::ARRAY_DIMENSIONS: {
				const BaseType::Array& array_type = this->context.getTypeManager().getArray(
					this->context.getTypeManager().getTypeInfo(fake_term_info.typeID).baseTypeID().arrayID()
				);

				const BaseType::Function& call_type = this->context.getTypeManager().getFunction(
					this->context.getTypeManager().getTypeInfo(builtin_type_method.typeID).baseTypeID().funcID()
				);

				auto values = evo::SmallVector<sema::Expr>();
				values.reserve(array_type.dimensions.size());
				for(uint64_t dimension : array_type.dimensions){
					values.emplace_back(
						this->context.sema_buffer.createIntValue(
							core::GenericInt::create<uint64_t>(dimension),
							this->context.getTypeManager().getTypeInfo(TypeManager::getTypeUSize()).baseTypeID()
						)
					);
				}

				const TypeInfo::ID return_type = call_type.returnParams[0].typeID.asTypeID();


				const sema::AggregateValue::ID created_aggregate_value = this->context.sema_buffer.createAggregateValue(
					std::move(values), this->context.getTypeManager().getTypeInfo(return_type).baseTypeID()
				);
				
				this->return_term_info(output,
					TermInfo::ValueCategory::EPHEMERAL,
					TermInfo::convertValueStage(fake_term_info.valueStage),
					TermInfo::ValueState::NOT_APPLICABLE,
					return_type,
					sema::Expr(created_aggregate_value)
				);
				return Result::SUCCESS;
			} break;

			case TermInfo::BuiltinTypeMethod::Kind::ARRAY_REF_SIZE: {
				const BaseType::ArrayRef::ID array_ref_type_id = 
					this->context.getTypeManager().getTypeInfo(fake_term_info.typeID).baseTypeID().arrayRefID();

				const sema::ArrayRefSize::ID created_array_ref_size =
					this->context.sema_buffer.createArrayRefSize(fake_term_info.expr, array_ref_type_id);

				this->return_term_info(output,
					TermInfo::ValueCategory::EPHEMERAL,
					TermInfo::convertValueStage(fake_term_info.valueStage),
					TermInfo::ValueState::NOT_APPLICABLE,
					TypeManager::getTypeUSize(),
					sema::Expr(created_array_ref_size)
				);
				return Result::SUCCESS;
			} break;
			
			case TermInfo::BuiltinTypeMethod::Kind::ARRAY_REF_DIMENSIONS: {
				const BaseType::ArrayRef::ID array_ref_type_id = 
					this->context.getTypeManager().getTypeInfo(fake_term_info.typeID).baseTypeID().arrayRefID();

				const sema::ArrayRefDimensions::ID created_array_ref_dimensions =
					this->context.sema_buffer.createArrayRefDimensions(fake_term_info.expr, array_ref_type_id);

				const BaseType::Function& call_type = this->context.getTypeManager().getFunction(
					this->context.getTypeManager().getTypeInfo(builtin_type_method.typeID).baseTypeID().funcID()
				);

				const TypeInfo::ID return_type = call_type.returnParams[0].typeID.asTypeID();

				this->return_term_info(output,
					TermInfo::ValueCategory::EPHEMERAL,
					TermInfo::convertValueStage(fake_term_info.valueStage),
					TermInfo::ValueState::NOT_APPLICABLE,
					return_type,
					sema::Expr(created_array_ref_dimensions)
				);
				return Result::SUCCESS;
			} break;
		}
		evo::debugFatalBreak("Unknown builtin-type method");
	}


	template<bool IS_CONSTEXPR>
	auto SemanticAnalyzer::interface_func_call(
		const TermInfo& target_term_info,
		evo::SmallVector<sema::Expr>&& args,
		sema::Func::ID selected_func_call_id,
		SymbolProc::TermInfoID output
	) -> Result {
		const sema::FakeTermInfo& fake_term_info = this->context.getSemaBuffer().getFakeTermInfo(
			target_term_info.getExpr().fakeTermInfoID()
		);

		const TypeInfo& expr_type_info = this->context.getTypeManager().getTypeInfo(fake_term_info.typeID);
		const BaseType::Interface& target_interface =
			this->context.getTypeManager().getInterface(expr_type_info.baseTypeID().interfaceID());

		const sema::Func& selected_func = this->context.getSemaBuffer().getFunc(selected_func_call_id);
		const BaseType::Function& selected_func_type = this->context.getTypeManager().getFunction(selected_func.typeID);

		for(size_t i = 0; const sema::Func::ID method : target_interface.methods){
			if(method == selected_func_call_id){
				const sema::InterfaceCall::ID interface_call_id = this->context.sema_buffer.createInterfaceCall(
					fake_term_info.expr,
					selected_func.typeID,
					expr_type_info.baseTypeID().interfaceID(),
					uint32_t(i),
					std::move(args)
				);

				if(selected_func_type.returnParams.size() == 1){ // single return
					this->return_term_info(output,
						TermInfo::ValueCategory::EPHEMERAL,
						target_term_info.value_stage,
						TermInfo::ValueState::NOT_APPLICABLE,
						selected_func_type.returnParams[0].typeID.asTypeID(),
						sema::Expr(interface_call_id)
					);
					
				}else{ // multi-return
					auto return_types = evo::SmallVector<TypeInfo::ID>();
					return_types.reserve(selected_func_type.returnParams.size());
					for(const BaseType::Function::ReturnParam& return_param : selected_func_type.returnParams){
						return_types.emplace_back(return_param.typeID.asTypeID());
					}

					this->return_term_info(output,
						TermInfo::ValueCategory::EPHEMERAL,
						target_term_info.value_stage,
						TermInfo::ValueState::NOT_APPLICABLE,
						std::move(return_types),
						sema::Expr(interface_call_id)
					);
				}

				return Result::SUCCESS;
			}

			i += 1;
		}

		evo::debugFatalBreak("Didn't find selected func id");
	}




	auto SemanticAnalyzer::instr_constexpr_func_call_run(const Instruction::ConstexprFuncCallRun& instr) -> Result {
		const TermInfo& func_call_term = this->get_term_info(instr.target);

		const sema::FuncCall& sema_func_call =
			this->context.getSemaBuffer().getFuncCall(func_call_term.getExpr().funcCallID());

		const sema::Func& target_func = 
			this->context.getSemaBuffer().getFunc(sema_func_call.target.as<sema::Func::ID>()); 

		const BaseType::Function& target_func_type = this->context.getTypeManager().getFunction(target_func.typeID);

		evo::debugAssert(target_func_type.returnsVoid() == false, "Constexpr function call expr cannot return void");
		evo::debugAssert(target_func.status == sema::Func::Status::DEF_DONE, "def of func not completed");

		auto jit_args = evo::SmallVector<core::GenericValue>();
		jit_args.reserve(instr.args.size() && size_t(target_func_type.hasNamedReturns()));
		for(size_t i = 0; const SymbolProc::TermInfoID& arg_id : instr.args){
			const TermInfo& arg = this->get_term_info(arg_id);

			jit_args.emplace_back(this->sema_expr_to_generic_value(arg.getExpr()));

			i += 1;
		}

		const bool uses_rvo = target_func_type.hasNamedReturns() 
			|| target_func_type.isImplicitRVO(this->context.getTypeManager());

		if(uses_rvo){
			jit_args.emplace_back(
				core::GenericValue::createUninit(
					this->context.getTypeManager().numBytes(target_func_type.returnParams[0].typeID.asTypeID())
				)
			);
		}

		// Uncomment this to print out the state of the constexpr pir module (for debugging purposes)
		// {
		// 	auto printer = core::Printer::createConsole();
		// 	pir::printModule(this->context.constexpr_pir_module, printer);
		// }

		core::GenericValue run_result = this->context.constexpr_jit_engine.runFunc(
			this->context.constexpr_pir_module,
			*target_func.constexprJITInterfaceFunc,
			jit_args,
			this->context.constexpr_pir_module.getFunction(*target_func.constexprJITFunc).getReturnType()
		);

		if(target_func_type.hasErrorReturn()){
			// 	// TODO(FUTURE): better messaging
			// 	this->emit_error(
			// 		Diagnostic::Code::SEMA_ERROR_RETURNED_FROM_CONSTEXPR_FUNC_RUN,
			// 		instr.func_call,
			// 		"Constexpr function returned error"
			// 	);
			// 	return Result::ERROR;

			this->emit_error(
				Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
				instr.func_call,
				"Running a constexpr function that has error returns is unimplemented"
			);
			return Result::ERROR;

		}else{
			const TypeInfo& target_func_return_type = this->context.getTypeManager().getTypeInfo(
				target_func_type.returnParams[0].typeID.asTypeID()
			);


			if(uses_rvo){
				run_result = std::move(jit_args.back());
			}

			if(target_func_return_type.qualifiers().empty() == false){
				this->emit_error(
					Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
					instr.func_call,
					"Running a constexpr function as a constexpr expression that returns "
						"a qualified type is unimplemented"
				);
				return Result::ERROR;
			}


			const sema::Expr return_sema_expr = this->generic_value_to_sema_expr(run_result, target_func_return_type);

			this->return_term_info(instr.output,
				TermInfo(
					TermInfo::ValueCategory::EPHEMERAL,
					TermInfo::ValueStage::CONSTEXPR,
					TermInfo::ValueState::NOT_APPLICABLE,
					func_call_term.type_id,
					return_sema_expr
				)
			);
			return Result::SUCCESS;
		}

	}


	template<Instruction::Language LANGUAGE>
	auto SemanticAnalyzer::instr_import(const Instruction::Import<LANGUAGE>& instr) -> Result {
		const TermInfo& location = this->get_term_info(instr.location);

		// TODO(FUTURE): type checking of location

		const std::string_view lookup_path = this->context.getSemaBuffer().getStringValue(
			location.getExpr().stringValueID()
		).value;


		auto lookup_error = std::optional<Context::LookupSourceIDError>();

		if constexpr(LANGUAGE == Instruction::Language::PANTHER){
			const evo::Expected<Source::ID, Context::LookupSourceIDError> import_lookup = 
				this->context.lookupSourceID(lookup_path, this->source);

			if(import_lookup.has_value()){
				this->return_term_info(instr.output, TermInfo(TermInfo::ValueCategory::MODULE, import_lookup.value()));
				return Result::SUCCESS;
			}
			
			lookup_error = import_lookup.error();

		}else if constexpr(LANGUAGE == Instruction::Language::C || LANGUAGE == Instruction::Language::CPP){
			const evo::Expected<ClangSource::ID, Context::LookupSourceIDError> import_lookup = 
				this->context.lookupClangSourceID(lookup_path, this->source, LANGUAGE == Instruction::Language::CPP);

			if(import_lookup.has_value()){
				this->return_term_info(instr.output, 
					TermInfo(TermInfo::ValueCategory::CLANG_MODULE, import_lookup.value())
				);
				return Result::SUCCESS;
			}
			
			lookup_error = import_lookup.error();

		}else{
			static_assert(false, "Unknown language");
		}


		switch(*lookup_error){
			case Context::LookupSourceIDError::EMPTY_PATH: {
				this->emit_error(
					Diagnostic::Code::SEMA_FAILED_TO_IMPORT_MODULE,
					instr.func_call.args[0].value,
					"Empty path is an invalid import location"
				);
				return Result::ERROR;
			} break;

			case Context::LookupSourceIDError::SAME_AS_CALLER: {
				// TODO(FUTURE): better messaging
				this->emit_error(
					Diagnostic::Code::SEMA_FAILED_TO_IMPORT_MODULE,
					instr.func_call.args[0].value,
					"Cannot import self"
				);
				return Result::ERROR;
			} break;

			case Context::LookupSourceIDError::NOT_ONE_OF_SOURCES: {
				this->emit_error(
					Diagnostic::Code::SEMA_FAILED_TO_IMPORT_MODULE,
					instr.func_call.args[0].value,
					std::format("File \"{}\" is not one of the files being compiled", lookup_path)
				);
				return Result::ERROR;
			} break;

			case Context::LookupSourceIDError::DOESNT_EXIST: {
				this->emit_error(
					Diagnostic::Code::SEMA_FAILED_TO_IMPORT_MODULE,
					instr.func_call.args[0].value,
					std::format("Couldn't find file \"{}\"", lookup_path)
				);
				return Result::ERROR;
			} break;

			case Context::LookupSourceIDError::FAILED_DURING_ANALYSIS_OF_NEWLY_LOADED: {
				return Result::ERROR;
			} break;

			case Context::LookupSourceIDError::WRONG_LANGUAGE: {
				this->emit_error(
					Diagnostic::Code::SEMA_FAILED_TO_IMPORT_MODULE,
					instr.func_call.args[0].value,
					std::format(
						"Couldn't import file \"{}\" as this language as it was included as a different one",
						lookup_path
					)
				);
				return Result::ERROR;
			} break;
		}

		evo::unreachable();
	}



	auto SemanticAnalyzer::instr_is_macro_defined(const Instruction::IsMacroDefined& instr) -> Result {
		const TermInfo& clang_module_term_info = this->get_term_info(instr.clang_module);
		const TermInfo& macro_name_term_info = this->get_term_info(instr.macro_name);


		if(clang_module_term_info.value_category != TermInfo::ValueCategory::CLANG_MODULE){
			evo::debugAssert("MUST BE CLANG MODULE");
		}

		const ClangSource& clang_module = 
			this->context.source_manager[clang_module_term_info.type_id.as<ClangSourceID>()];

		const sema::StringValue macro_name = 
			this->context.getSemaBuffer().getStringValue(macro_name_term_info.getExpr().stringValueID());

		const bool is_macro_defined = clang_module.getDefine(macro_name.value).has_value();
			
		this->return_term_info(instr.output,
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::CONSTEXPR,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(this->context.sema_buffer.createBoolValue(is_macro_defined))
		);
		return Result::SUCCESS;
	}



	template<bool IS_CONSTEXPR>
	auto SemanticAnalyzer::instr_template_intrinsic_func_call(
		const Instruction::TemplateIntrinsicFuncCall<IS_CONSTEXPR>& instr
	) -> Result {
		const TermInfo& target_term_info = this->get_term_info(instr.target);

		const evo::Expected<FuncCallImplData, bool> selected_func = this->func_call_impl<IS_CONSTEXPR, false>(
			instr.func_call, target_term_info, instr.args, instr.template_args
		);
		if(selected_func.has_value() == false){
			if(selected_func.error()){
				return Result::ERROR;
			}else{
				return Result::NEED_TO_WAIT;
			}
		}


		const Context::TemplateIntrinsicFuncInfo& template_intrinsic_func_info = 
			this->context.getTemplateIntrinsicFuncInfo(target_term_info.type_id.as<TemplateIntrinsicFunc::Kind>());

		if constexpr(IS_CONSTEXPR){
			if(template_intrinsic_func_info.allowedInConstexpr == false){
				this->emit_error(
					Diagnostic::Code::SEMA_FUNC_ISNT_CONSTEXPR,
					instr.func_call.target,
					"Cannot call a non-constexpr function as a constexpr value"
				);
				return Result::ERROR;
			}

		}else{
			if(this->get_current_func().isConstexpr){
				if(template_intrinsic_func_info.allowedInComptime == false){
					this->emit_error(
						Diagnostic::Code::SEMA_FUNC_ISNT_COMPTIME,
						instr.func_call.target,
						"Cannot call a non-comptime function within a comptime function"
					);
					return Result::ERROR;
				}

			}else{
				if(template_intrinsic_func_info.allowedInRuntime == false){
					this->emit_error(
						Diagnostic::Code::SEMA_FUNC_ISNT_RUNTIME,
						instr.func_call.target,
						"Cannot call a non-runtime function within a runtime function"
					);
					return Result::ERROR;
				}
			}
		}


		switch(this->context.getConfig().mode){
			case Context::Config::Mode::COMPILE: {
				if(template_intrinsic_func_info.allowedInCompile == false){
					this->emit_error(
						Diagnostic::Code::SEMA_INVALID_MODE_FOR_INTRINSIC,
						instr.func_call.target,
						"Calling this intrinsic is not allowed in compile mode"
					);
					return Result::ERROR;
				}
			} break;

			case Context::Config::Mode::SCRIPTING: {
				if(template_intrinsic_func_info.allowedInScript == false){
					this->emit_error(
						Diagnostic::Code::SEMA_INVALID_MODE_FOR_INTRINSIC,
						instr.func_call.target,
						"Calling this intrinsic is not allowed in scripting mode"
					);
					return Result::ERROR;
				}
			} break;

			case Context::Config::Mode::BUILD_SYSTEM: {
				if(template_intrinsic_func_info.allowedInBuildSystem == false){
					this->emit_error(
						Diagnostic::Code::SEMA_INVALID_MODE_FOR_INTRINSIC,
						instr.func_call.target,
						"Calling this intrinsic is not allowed in build system mode"
					);
					return Result::ERROR;
				}
			} break;
		}



		auto template_args = evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>();
		for(const SymbolProcTermInfoID& template_arg_id : instr.template_args){
			const TermInfo& template_arg = this->get_term_info(template_arg_id);

			if(template_arg.value_category == TermInfo::ValueCategory::TYPE){
				template_args.emplace_back(template_arg.type_id.as<TypeInfo::VoidableID>());
			}else{
				const sema::Expr& value_expr = template_arg.getExpr();

				switch(value_expr.kind()){
					case sema::Expr::Kind::INT_VALUE: {
						template_args.emplace_back(
							core::GenericValue(
								evo::copy(this->context.sema_buffer.getIntValue(value_expr.intValueID()).value)
							)
						);
					} break;
					case sema::Expr::Kind::FLOAT_VALUE: {
						template_args.emplace_back(
							core::GenericValue(
								evo::copy(this->context.sema_buffer.getFloatValue(value_expr.floatValueID()).value)
							)
						);
					} break;
					case sema::Expr::Kind::BOOL_VALUE: {
						template_args.emplace_back(
							core::GenericValue(
								this->context.sema_buffer.getBoolValue(value_expr.boolValueID()).value
							)
						);
					} break;
					case sema::Expr::Kind::STRING_VALUE: {
						template_args.emplace_back(
							core::GenericValue(
								evo::copy(this->context.sema_buffer.getStringValue(value_expr.stringValueID()).value)
							)
						);
					} break;
					case sema::Expr::Kind::AGGREGATE_VALUE: {
						this->emit_error(
							Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
							instr.func_call.target,
							"Func calls with aggregate template parameters are unimplemented"
						);
						return Result::ERROR;

						// template_args.emplace_back(
						// 	core::GenericValue(
						// 		this->context.sema_buffer.getStringValue(value_expr.stringValueID()).value
						// 	)
						// );
					} break;
					case sema::Expr::Kind::CHAR_VALUE: {
						template_args.emplace_back(
							core::GenericValue(this->context.sema_buffer.getCharValue(value_expr.charValueID()).value)
						);
					} break;
				}
			}
		}

		auto args = evo::SmallVector<sema::Expr>();
		for(const SymbolProc::TermInfoID& arg_term_info_id : instr.args){
			args.emplace_back(this->get_term_info(arg_term_info_id).getExpr());
		}

		const auto create_runtime_call = [&](evo::ArrayProxy<BaseType::Function::ReturnParam> return_params) -> void {
			auto return_types = evo::SmallVector<TypeInfo::ID>();
			for(const BaseType::Function::ReturnParam& return_param : return_params){
				return_types.emplace_back(return_param.typeID.asTypeID());
			}

			const sema::TemplateIntrinsicFuncInstantiation::ID intrinsic_target = 
				this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
					target_term_info.type_id.as<TemplateIntrinsicFunc::Kind>(), std::move(template_args)
				);

			if(return_types.size() == 1){
				this->return_term_info(instr.output,
					TermInfo::ValueCategory::EPHEMERAL,
					TermInfo::ValueStage::CONSTEXPR,
					TermInfo::ValueState::NOT_APPLICABLE,
					return_types[0],
					sema::Expr(this->context.sema_buffer.createFuncCall(intrinsic_target, std::move(args)))
				);

			}else{
				this->return_term_info(instr.output,
					TermInfo::ValueCategory::EPHEMERAL,
					TermInfo::ValueStage::CONSTEXPR,
					TermInfo::ValueState::NOT_APPLICABLE,
					std::move(return_types),
					sema::Expr(this->context.sema_buffer.createFuncCall(intrinsic_target, std::move(args)))
				);
			}
		};


		auto constexpr_intrinsic_evaluator = ConstexprIntrinsicEvaluator(
			this->context.type_manager, this->context.sema_buffer
		);

		switch(target_term_info.type_id.as<TemplateIntrinsicFunc::Kind>()){
			case TemplateIntrinsicFunc::Kind::GET_TYPE_ID: {
				this->return_term_info(
					instr.output,
					constexpr_intrinsic_evaluator.getTypeID(template_args[0].as<TypeInfo::VoidableID>().asTypeID())
				);
			} break;

			case TemplateIntrinsicFunc::Kind::NUM_BYTES: {
				this->return_term_info(
					instr.output,
					constexpr_intrinsic_evaluator.numBytes(template_args[0].as<TypeInfo::VoidableID>().asTypeID())
				);
			} break;

			case TemplateIntrinsicFunc::Kind::NUM_BITS: {
				this->return_term_info(
					instr.output,
					constexpr_intrinsic_evaluator.numBits(template_args[0].as<TypeInfo::VoidableID>().asTypeID())
				);
			} break;

			case TemplateIntrinsicFunc::Kind::BIT_CAST: {
				create_runtime_call(selected_func.value().selected_func_type.returnParams);
			} break;

			case TemplateIntrinsicFunc::Kind::TRUNC: {
				if constexpr(IS_CONSTEXPR){
					this->return_term_info(instr.output, constexpr_intrinsic_evaluator.trunc(
						template_args[1].as<TypeInfo::VoidableID>().asTypeID(),
						this->context.sema_buffer.getIntValue(args[0].intValueID()).value
					));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::FTRUNC: {
				if constexpr(IS_CONSTEXPR){
					this->return_term_info(instr.output, constexpr_intrinsic_evaluator.ftrunc(
						template_args[1].as<TypeInfo::VoidableID>().asTypeID(),
						this->context.sema_buffer.getFloatValue(args[0].floatValueID()).value
					));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::SEXT: {
				if constexpr(IS_CONSTEXPR){
					this->return_term_info(instr.output, constexpr_intrinsic_evaluator.sext(
						template_args[1].as<TypeInfo::VoidableID>().asTypeID(),
						this->context.sema_buffer.getIntValue(args[0].intValueID()).value
					));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::ZEXT: {
				if constexpr(IS_CONSTEXPR){
					this->return_term_info(instr.output, constexpr_intrinsic_evaluator.zext(
						template_args[1].as<TypeInfo::VoidableID>().asTypeID(),
						this->context.sema_buffer.getIntValue(args[0].intValueID()).value
					));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::FEXT: {
				if constexpr(IS_CONSTEXPR){
					this->return_term_info(instr.output, constexpr_intrinsic_evaluator.fext(
						template_args[1].as<TypeInfo::VoidableID>().asTypeID(),
						this->context.sema_buffer.getFloatValue(args[0].floatValueID()).value
					));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::I_TO_F: {
				if constexpr(IS_CONSTEXPR){
					this->return_term_info(instr.output, constexpr_intrinsic_evaluator.iToF(
						template_args[1].as<TypeInfo::VoidableID>().asTypeID(),
						this->context.sema_buffer.getIntValue(args[0].intValueID()).value
					));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::F_TO_I: {
				if constexpr(IS_CONSTEXPR){
					this->return_term_info(instr.output, constexpr_intrinsic_evaluator.fToI(
						template_args[1].as<TypeInfo::VoidableID>().asTypeID(),
						this->context.sema_buffer.getFloatValue(args[0].floatValueID()).value
					));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::ADD: {
				if constexpr(IS_CONSTEXPR){
					evo::Result<TermInfo> result = constexpr_intrinsic_evaluator.add(
						template_args[0].as<TypeInfo::VoidableID>().asTypeID(),
						template_args[1].as<core::GenericValue>().getBool(),
						this->context.sema_buffer.getIntValue(args[0].intValueID()).value,
						this->context.sema_buffer.getIntValue(args[1].intValueID()).value
					);

					if(result.isError()){
						// TODO(FUTURE): better messaging
						this->emit_error(
							Diagnostic::Code::SEMA_CONSTEXPR_INTRIN_MATH_ERROR,
							instr.func_call,
							"Constexpr intrinsic @add wrapped"
						);
						return Result::ERROR;
					}

					this->return_term_info(instr.output, std::move(result.value()));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::ADD_WRAP: {
				create_runtime_call(selected_func.value().selected_func_type.returnParams);
			} break;

			case TemplateIntrinsicFunc::Kind::ADD_SAT: {
				if constexpr(IS_CONSTEXPR){
					this->return_term_info(instr.output, constexpr_intrinsic_evaluator.addSat(
						template_args[0].as<TypeInfo::VoidableID>().asTypeID(),
						this->context.sema_buffer.getIntValue(args[0].intValueID()).value,
						this->context.sema_buffer.getIntValue(args[1].intValueID()).value
					));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::FADD: {
				if constexpr(IS_CONSTEXPR){
					this->return_term_info(instr.output, constexpr_intrinsic_evaluator.fadd(
						template_args[0].as<TypeInfo::VoidableID>().asTypeID(),
						this->context.sema_buffer.getFloatValue(args[0].floatValueID()).value,
						this->context.sema_buffer.getFloatValue(args[1].floatValueID()).value
					));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::SUB: {
				if constexpr(IS_CONSTEXPR){
					evo::Result<TermInfo> result = constexpr_intrinsic_evaluator.sub(
						template_args[0].as<TypeInfo::VoidableID>().asTypeID(),
						template_args[1].as<core::GenericValue>().getBool(),
						this->context.sema_buffer.getIntValue(args[0].intValueID()).value,
						this->context.sema_buffer.getIntValue(args[1].intValueID()).value
					);

					if(result.isError()){
						// TODO(FUTURE): better messaging
						this->emit_error(
							Diagnostic::Code::SEMA_CONSTEXPR_INTRIN_MATH_ERROR,
							instr.func_call,
							"Constexpr intrinsic @sub wrapped"
						);
						return Result::ERROR;
					}

					this->return_term_info(instr.output, std::move(result.value()));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::SUB_WRAP: {
				create_runtime_call(selected_func.value().selected_func_type.returnParams);
			} break;

			case TemplateIntrinsicFunc::Kind::SUB_SAT: {
				if constexpr(IS_CONSTEXPR){
					this->return_term_info(instr.output, constexpr_intrinsic_evaluator.subSat(
						template_args[0].as<TypeInfo::VoidableID>().asTypeID(),
						this->context.sema_buffer.getIntValue(args[0].intValueID()).value,
						this->context.sema_buffer.getIntValue(args[1].intValueID()).value
					));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::FSUB: {
				if constexpr(IS_CONSTEXPR){
					this->return_term_info(instr.output, constexpr_intrinsic_evaluator.fsub(
						template_args[0].as<TypeInfo::VoidableID>().asTypeID(),
						this->context.sema_buffer.getFloatValue(args[0].floatValueID()).value,
						this->context.sema_buffer.getFloatValue(args[1].floatValueID()).value
					));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::MUL: {
				if constexpr(IS_CONSTEXPR){
					evo::Result<TermInfo> result = constexpr_intrinsic_evaluator.mul(
						template_args[0].as<TypeInfo::VoidableID>().asTypeID(),
						template_args[1].as<core::GenericValue>().getBool(),
						this->context.sema_buffer.getIntValue(args[0].intValueID()).value,
						this->context.sema_buffer.getIntValue(args[1].intValueID()).value
					);

					if(result.isError()){
						// TODO(FUTURE): better messaging
						this->emit_error(
							Diagnostic::Code::SEMA_CONSTEXPR_INTRIN_MATH_ERROR,
							instr.func_call,
							"Constexpr intrinsic @mul wrapped"
						);
						return Result::ERROR;
					}

					this->return_term_info(instr.output, std::move(result.value()));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::MUL_WRAP: {
				create_runtime_call(selected_func.value().selected_func_type.returnParams);
			} break;

			case TemplateIntrinsicFunc::Kind::MUL_SAT: {
				if constexpr(IS_CONSTEXPR){
					this->return_term_info(instr.output, constexpr_intrinsic_evaluator.mulSat(
						template_args[0].as<TypeInfo::VoidableID>().asTypeID(),
						this->context.sema_buffer.getIntValue(args[0].intValueID()).value,
						this->context.sema_buffer.getIntValue(args[1].intValueID()).value
					));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::FMUL: {
				if constexpr(IS_CONSTEXPR){
					this->return_term_info(instr.output, constexpr_intrinsic_evaluator.fmul(
						template_args[0].as<TypeInfo::VoidableID>().asTypeID(),
						this->context.sema_buffer.getFloatValue(args[0].floatValueID()).value,
						this->context.sema_buffer.getFloatValue(args[1].floatValueID()).value
					));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::DIV: {
				if constexpr(IS_CONSTEXPR){
					evo::Result<TermInfo> result = constexpr_intrinsic_evaluator.div(
						template_args[0].as<TypeInfo::VoidableID>().asTypeID(),
						template_args[1].as<core::GenericValue>().getBool(),
						this->context.sema_buffer.getIntValue(args[0].intValueID()).value,
						this->context.sema_buffer.getIntValue(args[1].intValueID()).value
					);

					if(result.isError()){
						// TODO(FUTURE): better messaging
						this->emit_error(
							Diagnostic::Code::SEMA_CONSTEXPR_INTRIN_MATH_ERROR,
							instr.func_call,
							"Constexpr intrinsic @div was not exact"
						);
						return Result::ERROR;
					}

					this->return_term_info(instr.output, std::move(result.value()));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::FDIV: {
				if constexpr(IS_CONSTEXPR){
					this->return_term_info(instr.output, constexpr_intrinsic_evaluator.fdiv(
						template_args[0].as<TypeInfo::VoidableID>().asTypeID(),
						this->context.sema_buffer.getFloatValue(args[0].floatValueID()).value,
						this->context.sema_buffer.getFloatValue(args[1].floatValueID()).value
					));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::REM: {
				if constexpr(IS_CONSTEXPR){
					const TypeInfo::ID arg_type = template_args[0].as<TypeInfo::VoidableID>().asTypeID();

					if(this->context.getTypeManager().isFloatingPoint(arg_type)){
						this->return_term_info(instr.output, constexpr_intrinsic_evaluator.rem(
							arg_type,
							this->context.sema_buffer.getFloatValue(args[0].floatValueID()).value,
							this->context.sema_buffer.getFloatValue(args[1].floatValueID()).value
						));
					}else{
						this->return_term_info(instr.output, constexpr_intrinsic_evaluator.rem(
							arg_type,
							this->context.sema_buffer.getIntValue(args[0].intValueID()).value,
							this->context.sema_buffer.getIntValue(args[1].intValueID()).value
						));
					}

				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::FNEG: {
				if constexpr(IS_CONSTEXPR){
					this->return_term_info(instr.output, constexpr_intrinsic_evaluator.fneg(
						template_args[0].as<TypeInfo::VoidableID>().asTypeID(),
						this->context.sema_buffer.getFloatValue(args[0].floatValueID()).value
					));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::EQ: {
				if constexpr(IS_CONSTEXPR){
					const TypeInfo::ID arg_type = template_args[0].as<TypeInfo::VoidableID>().asTypeID();

					if(this->context.getTypeManager().isFloatingPoint(arg_type)){
						this->return_term_info(instr.output, constexpr_intrinsic_evaluator.eq(
							arg_type,
							this->context.sema_buffer.getFloatValue(args[0].floatValueID()).value,
							this->context.sema_buffer.getFloatValue(args[1].floatValueID()).value
						));
					}else{
						this->return_term_info(instr.output, constexpr_intrinsic_evaluator.eq(
							arg_type,
							this->context.sema_buffer.getIntValue(args[0].intValueID()).value,
							this->context.sema_buffer.getIntValue(args[1].intValueID()).value
						));
					}
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::NEQ: {
				if constexpr(IS_CONSTEXPR){
					const TypeInfo::ID arg_type = template_args[0].as<TypeInfo::VoidableID>().asTypeID();

					if(this->context.getTypeManager().isFloatingPoint(arg_type)){
						this->return_term_info(instr.output, constexpr_intrinsic_evaluator.neq(
							arg_type,
							this->context.sema_buffer.getFloatValue(args[0].floatValueID()).value,
							this->context.sema_buffer.getFloatValue(args[1].floatValueID()).value
						));
					}else{
						this->return_term_info(instr.output, constexpr_intrinsic_evaluator.neq(
							arg_type,
							this->context.sema_buffer.getIntValue(args[0].intValueID()).value,
							this->context.sema_buffer.getIntValue(args[1].intValueID()).value
						));
					}

				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::LT: {
				if constexpr(IS_CONSTEXPR){
					const TypeInfo::ID arg_type = template_args[0].as<TypeInfo::VoidableID>().asTypeID();

					if(this->context.getTypeManager().isFloatingPoint(arg_type)){
						this->return_term_info(instr.output,  constexpr_intrinsic_evaluator.lt(
							arg_type,
							this->context.sema_buffer.getFloatValue(args[0].floatValueID()).value,
							this->context.sema_buffer.getFloatValue(args[1].floatValueID()).value
						));
					}else{
						this->return_term_info(instr.output,  constexpr_intrinsic_evaluator.lt(
							arg_type,
							this->context.sema_buffer.getIntValue(args[0].intValueID()).value,
							this->context.sema_buffer.getIntValue(args[1].intValueID()).value
						));
					}
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::LTE: {
				if constexpr(IS_CONSTEXPR){
					const TypeInfo::ID arg_type = template_args[0].as<TypeInfo::VoidableID>().asTypeID();

					if(this->context.getTypeManager().isFloatingPoint(arg_type)){
						this->return_term_info(instr.output, constexpr_intrinsic_evaluator.lte(
							arg_type,
							this->context.sema_buffer.getFloatValue(args[0].floatValueID()).value,
							this->context.sema_buffer.getFloatValue(args[1].floatValueID()).value
						));
					}else{
						this->return_term_info(instr.output, constexpr_intrinsic_evaluator.lte(
							arg_type,
							this->context.sema_buffer.getIntValue(args[0].intValueID()).value,
							this->context.sema_buffer.getIntValue(args[1].intValueID()).value
						));
					}
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::GT: {
				if constexpr(IS_CONSTEXPR){
					const TypeInfo::ID arg_type = template_args[0].as<TypeInfo::VoidableID>().asTypeID();

					if(this->context.getTypeManager().isFloatingPoint(arg_type)){
						this->return_term_info(instr.output, constexpr_intrinsic_evaluator.gt(
							arg_type,
							this->context.sema_buffer.getFloatValue(args[0].floatValueID()).value,
							this->context.sema_buffer.getFloatValue(args[1].floatValueID()).value
						));
					}else{
						this->return_term_info(instr.output, constexpr_intrinsic_evaluator.gt(
							arg_type,
							this->context.sema_buffer.getIntValue(args[0].intValueID()).value,
							this->context.sema_buffer.getIntValue(args[1].intValueID()).value
						));
					}
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::GTE: {
				if constexpr(IS_CONSTEXPR){
					const TypeInfo::ID arg_type = template_args[0].as<TypeInfo::VoidableID>().asTypeID();

					if(this->context.getTypeManager().isFloatingPoint(arg_type)){
						this->return_term_info(instr.output, constexpr_intrinsic_evaluator.gte(
							arg_type,
							this->context.sema_buffer.getFloatValue(args[0].floatValueID()).value,
							this->context.sema_buffer.getFloatValue(args[1].floatValueID()).value
						));
					}else{
						this->return_term_info(instr.output, constexpr_intrinsic_evaluator.gte(
							arg_type,
							this->context.sema_buffer.getIntValue(args[0].intValueID()).value,
							this->context.sema_buffer.getIntValue(args[1].intValueID()).value
						));
					}
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::AND: {
				if constexpr(IS_CONSTEXPR){
					this->return_term_info(instr.output, constexpr_intrinsic_evaluator.bitwiseAnd(
						template_args[0].as<TypeInfo::VoidableID>().asTypeID(),
						this->context.sema_buffer.getIntValue(args[0].intValueID()).value,
						this->context.sema_buffer.getIntValue(args[1].intValueID()).value
					));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::OR: {
				if constexpr(IS_CONSTEXPR){
					this->return_term_info(instr.output, constexpr_intrinsic_evaluator.bitwiseOr(
						template_args[0].as<TypeInfo::VoidableID>().asTypeID(),
						this->context.sema_buffer.getIntValue(args[0].intValueID()).value,
						this->context.sema_buffer.getIntValue(args[1].intValueID()).value
					));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::XOR: {
				if constexpr(IS_CONSTEXPR){
					this->return_term_info(instr.output, constexpr_intrinsic_evaluator.bitwiseXor(
						template_args[0].as<TypeInfo::VoidableID>().asTypeID(),
						this->context.sema_buffer.getIntValue(args[0].intValueID()).value,
						this->context.sema_buffer.getIntValue(args[1].intValueID()).value
					));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::SHL: {
				if constexpr(IS_CONSTEXPR){
					evo::Result<TermInfo> result = constexpr_intrinsic_evaluator.shl(
						template_args[0].as<TypeInfo::VoidableID>().asTypeID(),
						template_args[2].as<core::GenericValue>().getBool(),
						this->context.sema_buffer.getIntValue(args[0].intValueID()).value,
						this->context.sema_buffer.getIntValue(args[1].intValueID()).value
					);

					if(result.isError()){
						// TODO(FUTURE): better messaging
						this->emit_error(
							Diagnostic::Code::SEMA_CONSTEXPR_INTRIN_MATH_ERROR,
							instr.func_call,
							"Constexpr intrinsic @shl wrapped"
						);
						return Result::ERROR;
					}

					this->return_term_info(instr.output, std::move(result.value()));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::SHL_SAT: {
				if constexpr(IS_CONSTEXPR){
					this->return_term_info(instr.output, constexpr_intrinsic_evaluator.shlSat(
						template_args[0].as<TypeInfo::VoidableID>().asTypeID(),
						this->context.sema_buffer.getIntValue(args[0].intValueID()).value,
						this->context.sema_buffer.getIntValue(args[1].intValueID()).value
					));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::SHR: {
				if constexpr(IS_CONSTEXPR){
					evo::Result<TermInfo> result = constexpr_intrinsic_evaluator.shr(
						template_args[0].as<TypeInfo::VoidableID>().asTypeID(),
						template_args[2].as<core::GenericValue>().getBool(),
						this->context.sema_buffer.getIntValue(args[0].intValueID()).value,
						this->context.sema_buffer.getIntValue(args[1].intValueID()).value
					);

					if(result.isError()){
						// TODO(FUTURE): better messaging
						this->emit_error(
							Diagnostic::Code::SEMA_CONSTEXPR_INTRIN_MATH_ERROR,
							instr.func_call,
							"Constexpr intrinsic @shr wrapped"
						);
						return Result::ERROR;
					}

					this->return_term_info(instr.output, std::move(result.value()));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::BIT_REVERSE: {
				if constexpr(IS_CONSTEXPR){
					this->return_term_info(instr.output, constexpr_intrinsic_evaluator.bitReverse(
						template_args[0].as<TypeInfo::VoidableID>().asTypeID(),
						this->context.sema_buffer.getIntValue(args[0].intValueID()).value
					));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::BSWAP: {
				if constexpr(IS_CONSTEXPR){
					this->return_term_info(instr.output, constexpr_intrinsic_evaluator.bSwap(
						template_args[0].as<TypeInfo::VoidableID>().asTypeID(),
						this->context.sema_buffer.getIntValue(args[0].intValueID()).value
					));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::CTPOP: {
				if constexpr(IS_CONSTEXPR){
					this->return_term_info(instr.output, constexpr_intrinsic_evaluator.ctPop(
						template_args[0].as<TypeInfo::VoidableID>().asTypeID(),
						this->context.sema_buffer.getIntValue(args[0].intValueID()).value
					));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::CTLZ: {
				if constexpr(IS_CONSTEXPR){
					this->return_term_info(instr.output, constexpr_intrinsic_evaluator.ctlz(
						template_args[0].as<TypeInfo::VoidableID>().asTypeID(),
						this->context.sema_buffer.getIntValue(args[0].intValueID()).value
					));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::CTTZ: {
				if constexpr(IS_CONSTEXPR){
					this->return_term_info(instr.output, constexpr_intrinsic_evaluator.cttz(
						template_args[0].as<TypeInfo::VoidableID>().asTypeID(),
						this->context.sema_buffer.getIntValue(args[0].intValueID()).value
					));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}
			} break;

			case TemplateIntrinsicFunc::Kind::_MAX_: {
				evo::debugFatalBreak("Invalid template intrinsic func");
			} break;
		}


		return Result::SUCCESS;
	}




	auto SemanticAnalyzer::instr_copy(const Instruction::Copy& instr) -> Result {
		const TermInfo& target = this->get_term_info(instr.target);

		if(target.is_concrete() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_COPY_ARG_NOT_CONCRETE,
				instr.prefix,
				"Argument of operator [copy] must be concrete"
			);
			return Result::ERROR;
		}

		if(this->context.getTypeManager().isCopyable(target.type_id.as<TypeInfo::ID>()) == false){
			this->emit_error(
				Diagnostic::Code::SEMA_COPY_ARG_TYPE_NOT_COPYABLE,
				instr.prefix,
				"Type of argument of operator [copy] is not copyable"
			);
			return Result::ERROR;
		}


		if(this->currently_in_func() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_EXPR_INVALID_OBJECT_SCOPE,
				instr.prefix,
				"Operator [copy] must be in function scope"
			);
			return Result::ERROR;
		}


		if(
			this->get_current_func().isConstexpr
			&& this->context.getTypeManager().isConstexprCopyable(
				target.type_id.as<TypeInfo::ID>(), this->context.getSemaBuffer()
			) == false
		){
			this->emit_error(
				Diagnostic::Code::SEMA_COMPTIME_COPY_ARG_TYPE_NOT_CONSTEXPR_COPYABLE,
				instr.prefix,
				"Type of argument of operator [copy] is not constexpr copyable"
			);
			return Result::ERROR;
		}

		if(
			target.value_state != TermInfo::ValueState::INIT
			&& target.value_state != TermInfo::ValueState::NOT_APPLICABLE
		){
			this->emit_error(
				Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
				instr.prefix,
				"Argument of operator [copy] must be initialized"
			);
			return Result::ERROR;
		}

		this->get_special_member_stmt_dependents<SpecialMemberKind::COPY>(
			target.type_id.as<TypeInfo::ID>(), this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>().dependent_funcs
		);

		this->return_term_info(instr.output,
			TermInfo::ValueCategory::EPHEMERAL,
			target.value_stage,
			TermInfo::ValueState::NOT_APPLICABLE,
			target.type_id,
			sema::Expr(this->context.sema_buffer.createCopy(target.getExpr(), target.type_id.as<TypeInfo::ID>()))
		);

		return Result::SUCCESS;
	}

	auto SemanticAnalyzer::instr_move(const Instruction::Move& instr) -> Result {
		const TermInfo& target = this->get_term_info(instr.target);

		if(target.value_category != TermInfo::ValueCategory::CONCRETE_MUT){
			if(target.value_category == TermInfo::ValueCategory::FORWARDABLE){
				this->emit_error(
					Diagnostic::Code::SEMA_MOVE_ARG_IS_IN_PARAM,
					instr.prefix.rhs,
					"Argument of operator [move] cannot be an in-parameter",
					Diagnostic::Info("Use operator [forward] instead")
				);
			}else if(target.is_concrete() == false){
				this->emit_error(
					Diagnostic::Code::SEMA_MOVE_ARG_NOT_CONCRETE,
					instr.prefix.rhs,
					"Argument of operator [move] must be concrete"
				);
			}else{
				this->emit_error(
					Diagnostic::Code::SEMA_MOVE_ARG_NOT_MUTABLE,
					instr.prefix.rhs,
					"Argument of operator [move] must be mutable"
				);
			}

			return Result::ERROR;
		}


		if(this->context.getTypeManager().isMovable(target.type_id.as<TypeInfo::ID>()) == false){
			this->emit_error(
				Diagnostic::Code::SEMA_MOVE_ARG_TYPE_NOT_MOVABLE,
				instr.prefix.rhs,
				"Type of argument of operator [move] is not movable"
			);
			return Result::ERROR;
		}


		if(this->currently_in_func() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_EXPR_INVALID_OBJECT_SCOPE,
				instr.prefix,
				"Operator [move] must be in function scope"
			);
			return Result::ERROR;
		}


		if(
			this->get_current_func().isConstexpr
			&& this->context.getTypeManager().isConstexprMovable(
				target.type_id.as<TypeInfo::ID>(), this->context.getSemaBuffer()
			) == false
		){
			this->emit_error(
				Diagnostic::Code::SEMA_COMPTIME_MOVE_ARG_TYPE_NOT_CONSTEXPR_MOVABLE,
				instr.prefix,
				"Type of argument of operator [move] is not constexpr movable"
			);
			return Result::ERROR;
		}


		switch(target.value_state){
			case TermInfo::ValueState::NOT_APPLICABLE: {
				// do nothing...
			} break;

			case TermInfo::ValueState::INIT: {
				this->set_ident_value_state_if_needed(target.getExpr(), sema::ScopeLevel::ValueState::MOVED_FROM);
			} break;

			case TermInfo::ValueState::INITIALIZING: case TermInfo::ValueState::UNINIT: {
				this->emit_error(
					Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
					instr.prefix.rhs,
					"Argument of operator [move] must be initialized"
				);
				return Result::ERROR;
			} break;

			case TermInfo::ValueState::MOVED_FROM: {
				this->emit_error(
					Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
					instr.prefix.rhs,
					"Argument of operator [move] must be initialized",
					Diagnostic::Info("This argument was already moved from")
				);
				return Result::ERROR;
			} break;
		}

		this->get_special_member_stmt_dependents<SpecialMemberKind::MOVE>(
			target.type_id.as<TypeInfo::ID>(), this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>().dependent_funcs
		);

		this->return_term_info(instr.output,
			TermInfo::ValueCategory::EPHEMERAL,
			target.value_stage,
			TermInfo::ValueState::NOT_APPLICABLE,
			target.type_id,
			sema::Expr(this->context.sema_buffer.createMove(target.getExpr(), target.type_id.as<TypeInfo::ID>()))
		);

		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_forward(const Instruction::Forward& instr) -> Result {
		const TermInfo& target = this->get_term_info(instr.target);

		if(target.value_category != TermInfo::ValueCategory::FORWARDABLE){
			this->emit_error(
				Diagnostic::Code::SEMA_MOVE_ARG_NOT_MUTABLE,
				instr.prefix,
				"Argument of operator [forward] must be forwardable"
			);

			return Result::ERROR;
		}

		if(this->currently_in_func() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_EXPR_INVALID_OBJECT_SCOPE,
				instr.prefix,
				"Operator [forward] must be in function scope"
			);
			return Result::ERROR;
		}

		if(this->get_current_func().isConstexpr){
			if(
				this->context.getTypeManager().isCopyable(target.type_id.as<TypeInfo::ID>())
				&& this->context.getTypeManager().isConstexprCopyable(
					target.type_id.as<TypeInfo::ID>(), this->context.getSemaBuffer()
				) == false
			){
				this->emit_error(
					Diagnostic::Code::SEMA_COMPTIME_COPY_ARG_TYPE_NOT_CONSTEXPR_COPYABLE,
					instr.prefix,
					"Type of argument of operator [forward] is not constexpr copyable"
				);
				return Result::ERROR;
			}

			if(
				this->context.getTypeManager().isMovable(target.type_id.as<TypeInfo::ID>())
				&& this->context.getTypeManager().isConstexprMovable(
					target.type_id.as<TypeInfo::ID>(), this->context.getSemaBuffer()
				) == false
			){
				this->emit_error(
					Diagnostic::Code::SEMA_COMPTIME_MOVE_ARG_TYPE_NOT_CONSTEXPR_MOVABLE,
					instr.prefix,
					"Type of argument of operator [forward] is not constexpr movable"
				);
				return Result::ERROR;
			}
		}


		switch(target.value_state){
			case TermInfo::ValueState::NOT_APPLICABLE: {
				// do nothing...
			} break;

			case TermInfo::ValueState::INIT: {
				this->set_ident_value_state_if_needed(target.getExpr(), sema::ScopeLevel::ValueState::MOVED_FROM);
			} break;

			case TermInfo::ValueState::INITIALIZING: case TermInfo::ValueState::UNINIT: {
				this->emit_error(
					Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
					instr.prefix.rhs,
					"Argument of operator [forward] must be initialized"
				);
				return Result::ERROR;
			} break;

			case TermInfo::ValueState::MOVED_FROM: {
				this->emit_error(
					Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
					instr.prefix.rhs,
					"Argument of operator [forward] must be initialized",
					Diagnostic::Info("This argument was already forwarded")
				);
				return Result::ERROR;
			} break;
		}


		this->return_term_info(instr.output,
			TermInfo::ValueCategory::EPHEMERAL,
			target.value_stage,
			TermInfo::ValueState::NOT_APPLICABLE,
			target.type_id,
			sema::Expr(
				this->context.sema_buffer.createForward(target.getExpr(), target.type_id.as<TypeInfo::ID>(), true)
			)
		);

		return Result::SUCCESS;
	}


	template<bool IS_READ_ONLY>
	auto SemanticAnalyzer::instr_addr_of(const Instruction::AddrOf<IS_READ_ONLY>& instr) -> Result {
		const TermInfo& target = this->get_term_info(instr.target);

		if(target.is_concrete() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_ADDR_OF_ARG_NOT_CONCRETE,
				instr.prefix,
				"Argument of operator prefix [&] must be concrete"
			);
			return Result::ERROR;
		}

		if(target.value_state == TermInfo::ValueState::MOVED_FROM){
			this->emit_error(
				Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
				instr.prefix,
				"Argument of operator prefix [&] cannot be moved-from"
			);
			return Result::ERROR;
		}

		const bool is_read_only = [&](){
			if constexpr(IS_READ_ONLY){
				return true;
			}else{
				return target.value_category == TermInfo::ValueCategory::CONCRETE_CONST;
			}
		}();


		const TypeInfo& target_type = this->context.type_manager.getTypeInfo(target.type_id.as<TypeInfo::ID>());

		auto resultant_qualifiers = evo::SmallVector<AST::Type::Qualifier>();
		resultant_qualifiers.reserve(target_type.qualifiers().size() + 1);
		for(const AST::Type::Qualifier& qualifier : target_type.qualifiers()){
			resultant_qualifiers.emplace_back(qualifier);
		}
		resultant_qualifiers.emplace_back(
			true, is_read_only, target.value_state == TermInfo::ValueState::UNINIT, false
		);

		const TypeInfo::ID resultant_type_id = this->context.type_manager.getOrCreateTypeInfo(
			TypeInfo(target_type.baseTypeID(), std::move(resultant_qualifiers))
		);


		this->return_term_info(instr.output,
			TermInfo::ValueCategory::EPHEMERAL,
			target.value_stage,
			TermInfo::ValueState::NOT_APPLICABLE,
			resultant_type_id,
			sema::Expr(this->context.sema_buffer.createAddrOf(target.getExpr()))
		);

		return Result::SUCCESS;
	}


	template<bool IS_CONSTEXPR>
	auto SemanticAnalyzer::instr_prefix_negate(const Instruction::PrefixNegate<IS_CONSTEXPR>& instr) -> Result {
		TermInfo& expr = this->get_term_info(instr.expr);

		if(expr.isSingleValue() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_MULTI_RETURN_INTO_SINGLE_VALUE,
				instr.prefix.rhs,
				"Operator prefix [-] cannot accept multiple values"
			);
			return Result::ERROR;
		}

		if constexpr(IS_CONSTEXPR){
			if(expr.getExpr().kind() == sema::Expr::Kind::INT_VALUE){
				sema::IntValue& int_value = this->context.sema_buffer.int_values[expr.getExpr().intValueID()];
				int_value.value = core::GenericInt(int_value.value.getBitWidth(), 0).ssub(int_value.value).result;

				this->return_term_info(instr.output, expr);
				return Result::SUCCESS;
				
			}else{
				sema::FloatValue& float_value =
					this->context.sema_buffer.float_values[expr.getExpr().floatValueID()];
				float_value.value = float_value.value.neg();

				this->return_term_info(instr.output, expr);
				return Result::SUCCESS;
			}

		}else{
			if(
				expr.value_state != TermInfo::ValueState::INIT
				&& expr.value_state != TermInfo::ValueState::NOT_APPLICABLE
			){
				this->emit_error(
					Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
					instr.prefix.rhs,
					"Argument of operator prefix [-] must be initialized"
				);
				return Result::ERROR;
			}

			if(expr.value_category == TermInfo::ValueCategory::EPHEMERAL_FLUID){
				if(expr.getExpr().kind() == sema::Expr::Kind::INT_VALUE){
					sema::IntValue& int_value = this->context.sema_buffer.int_values[expr.getExpr().intValueID()];
					int_value.value = core::GenericInt(int_value.value.getBitWidth(), 0).ssub(int_value.value).result;

					this->return_term_info(instr.output, expr);
					return Result::SUCCESS;
					
				}else{
					sema::FloatValue& float_value =
						this->context.sema_buffer.float_values[expr.getExpr().floatValueID()];
					float_value.value = float_value.value.neg();

					this->return_term_info(instr.output, expr);
					return Result::SUCCESS;
				}

			}else{
				if(this->context.getTypeManager().isIntegral(expr.type_id.as<TypeInfo::ID>())){
					using InstantiationID = sema::TemplateIntrinsicFuncInstantiation::ID;
					const InstantiationID instantiation_id =
						this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
							TemplateIntrinsicFunc::Kind::SUB,
							evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{
								expr.type_id.as<TypeInfo::ID>(), core::GenericValue(false)
							}
						);

					const sema::IntValue::ID zero = this->context.sema_buffer.createIntValue(
						core::GenericInt::create<int64_t>(0), // TODO(FUTURE): set to same width as expr?
						this->context.getTypeManager().getTypeInfo(expr.type_id.as<TypeInfo::ID>()).baseTypeID()
					);

					const sema::FuncCall::ID created_func_call_id = this->context.sema_buffer.createFuncCall(
						instantiation_id, evo::SmallVector<sema::Expr>{sema::Expr(zero), expr.getExpr()}
					);

					this->return_term_info(instr.output,
						TermInfo::ValueCategory::EPHEMERAL,
						expr.value_stage,
						TermInfo::ValueState::NOT_APPLICABLE,
						expr.type_id.as<TypeInfo::ID>(),
						sema::Expr(created_func_call_id)
					);
					return Result::SUCCESS;

					
				}else if(this->context.getTypeManager().isFloatingPoint(expr.type_id.as<TypeInfo::ID>())){
					using InstantiationID = sema::TemplateIntrinsicFuncInstantiation::ID;
					const InstantiationID instantiation_id =
						this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
							TemplateIntrinsicFunc::Kind::FNEG,
							evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{
								expr.type_id.as<TypeInfo::ID>()
							}
						);

					const sema::FuncCall::ID created_func_call_id = this->context.sema_buffer.createFuncCall(
						instantiation_id, evo::SmallVector<sema::Expr>{expr.getExpr()}
					);

					this->return_term_info(instr.output,
						TermInfo::ValueCategory::EPHEMERAL,
						expr.value_stage,
						TermInfo::ValueState::NOT_APPLICABLE,
						expr.type_id.as<TypeInfo::ID>(),
						sema::Expr(created_func_call_id)
					);
					return Result::SUCCESS;
					
				}else{
					this->emit_error(
						Diagnostic::Code::SEMA_NEGATE_ARG_INVALID_TYPE,
						instr.prefix.rhs,
						"Operator prefix [-] can only accept integrals and floats"
					);
					return Result::ERROR;
				}
			}
		}
	}


	template<bool IS_CONSTEXPR>
	auto SemanticAnalyzer::instr_prefix_not(const Instruction::PrefixNot<IS_CONSTEXPR>& instr) -> Result {
		TermInfo& expr = this->get_term_info(instr.expr);

		if(this->type_check<true, true>(
			TypeManager::getTypeBool(), expr, "RHS of operator [!]", instr.prefix.rhs
		).ok == false){
			return Result::ERROR;
		}

		if(
			expr.value_state != TermInfo::ValueState::INIT
			&& expr.value_state != TermInfo::ValueState::NOT_APPLICABLE
		){
			this->emit_error(
				Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
				instr.prefix.rhs,
				"Argument of operator [!] must be initialized"
			);
			return Result::ERROR;
		}


		if constexpr(IS_CONSTEXPR){
			sema::BoolValue& bool_value = this->context.sema_buffer.bool_values[expr.getExpr().boolValueID()];
			bool_value.value = !bool_value.value;

			this->return_term_info(instr.output, expr);
			return Result::SUCCESS;

		}else{
			using InstantiationID = sema::TemplateIntrinsicFuncInstantiation::ID;
			const InstantiationID instantiation_id =
				this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
					TemplateIntrinsicFunc::Kind::XOR,
					evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{
						expr.type_id.as<TypeInfo::ID>()
					}
				);

			const sema::BoolValue::ID true_value = this->context.sema_buffer.createBoolValue(true);

			const sema::FuncCall::ID created_func_call_id = this->context.sema_buffer.createFuncCall(
				instantiation_id, evo::SmallVector<sema::Expr>{expr.getExpr(), sema::Expr(true_value)}
			);

			this->return_term_info(instr.output,
				TermInfo::ValueCategory::EPHEMERAL,
				expr.value_stage,
				TermInfo::ValueState::NOT_APPLICABLE,
				TypeManager::getTypeBool(),
				sema::Expr(created_func_call_id)
			);
			return Result::SUCCESS;
		}
	}



	template<bool IS_CONSTEXPR>
	auto SemanticAnalyzer::instr_prefix_bitwise_not(const Instruction::PrefixBitwiseNot<IS_CONSTEXPR>& instr)
	-> Result {
		TermInfo& expr = this->get_term_info(instr.expr);

		if(expr.isSingleValue() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_MULTI_RETURN_INTO_SINGLE_VALUE,
				instr.prefix.rhs,
				"Operator [~] cannot accept multiple values"
			);
			return Result::ERROR;
		}

		if(expr.value_category == TermInfo::ValueCategory::EPHEMERAL_FLUID){
			this->emit_error(
				Diagnostic::Code::SEMA_BITWISE_NOT_ARG_FLUID,
				instr.prefix.rhs,
				"Operator [~] cannot accept fluid values"
			);
			return Result::ERROR;
		}


		if(
			expr.value_state != TermInfo::ValueState::INIT
			&& expr.value_state != TermInfo::ValueState::NOT_APPLICABLE
		){
			this->emit_error(
				Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
				instr.prefix.rhs,
				"Argument of operator [~] must be initialized"
			);
			return Result::ERROR;
		}

			
		if(this->context.getTypeManager().isIntegral(expr.type_id.as<TypeInfo::ID>()) == false){
			this->emit_error(
				Diagnostic::Code::SEMA_BITWISE_NOT_ARG_NOT_INTEGRAL,
				instr.prefix.rhs,
				"Operator [~] can only accept integrals"
			);
			return Result::ERROR;
		}


		if constexpr(IS_CONSTEXPR){
			sema::IntValue& int_value = this->context.sema_buffer.int_values[expr.getExpr().intValueID()];
			int_value.value =
				int_value.value.bitwiseXor(core::GenericInt(int_value.value.getBitWidth(), 0).bitwiseNot());

			this->return_term_info(instr.output, expr);
			return Result::SUCCESS;

		}else{
			if(expr.value_category == TermInfo::ValueCategory::EPHEMERAL_FLUID){
				sema::IntValue& int_value = this->context.sema_buffer.int_values[expr.getExpr().intValueID()];
				int_value.value =
					int_value.value.bitwiseXor(core::GenericInt(int_value.value.getBitWidth(), 0).bitwiseNot());

				this->return_term_info(instr.output, expr);
				return Result::SUCCESS;
				
			}else{
				using InstantiationID = sema::TemplateIntrinsicFuncInstantiation::ID;
				const InstantiationID instantiation_id =
					this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
						TemplateIntrinsicFunc::Kind::XOR,
						evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{
							expr.type_id.as<TypeInfo::ID>()
						}
					);

				const sema::IntValue::ID all_ones = this->context.sema_buffer.createIntValue(
					core::GenericInt::create<int64_t>(0).bitwiseNot(), // TODO(FUTURE): set to same width as expr?
					this->context.getTypeManager().getTypeInfo(expr.type_id.as<TypeInfo::ID>()).baseTypeID()
				);

				const sema::FuncCall::ID created_func_call_id = this->context.sema_buffer.createFuncCall(
					instantiation_id, evo::SmallVector<sema::Expr>{expr.getExpr(), sema::Expr(all_ones)}
				);

				this->return_term_info(instr.output,
					TermInfo::ValueCategory::EPHEMERAL,
					expr.value_stage,
					TermInfo::ValueState::NOT_APPLICABLE,
					expr.type_id.as<TypeInfo::ID>(),
					sema::Expr(created_func_call_id)
				);
				return Result::SUCCESS;
			}
		}
	}




	auto SemanticAnalyzer::instr_deref(const Instruction::Deref& instr) -> Result {
		const TermInfo& target = this->get_term_info(instr.target);

		if(
			target.value_state != TermInfo::ValueState::INIT
			&& target.value_state != TermInfo::ValueState::NOT_APPLICABLE
		){
			this->emit_error(
				Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
				instr.postfix.lhs,
				"Argument of operator [.*] must be initialized"
			);
			return Result::ERROR;
		}


		if(target.type_id.is<TypeInfo::ID>() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_DEREF_ARG_NOT_PTR,
				instr.postfix.lhs,
				"Argument of operator [.*] must be a pointer"
			);
			return Result::ERROR;
		}

		const TypeInfo& target_type = this->context.getTypeManager().getTypeInfo(target.type_id.as<TypeInfo::ID>());

		if(target_type.isPointer() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_DEREF_ARG_NOT_PTR,
				instr.postfix.lhs,
				"Argument of operator [.*] must be a pointer"
			);
			return Result::ERROR;
		}

		if(target_type.isOptional()){
			this->emit_error(
				Diagnostic::Code::SEMA_DEREF_ARG_NOT_PTR,
				instr.postfix.lhs,
				"Argument of operator [.*] must be a non-optional pointer",
				Diagnostic::Info("Did you mean \".?.*\" instead?")
			);
			return Result::ERROR;
		}

		if(target_type.isInterfacePointer()){
			this->emit_error(
				Diagnostic::Code::SEMA_DEREF_ARG_NOT_PTR,
				instr.postfix.lhs,
				"Argument of operator [.*] must be a pointer",
				Diagnostic::Info("Cannot be an interface pointer")
			);
			return Result::ERROR;
		}

		auto resultant_qualifiers = evo::SmallVector<AST::Type::Qualifier>();
		if(resultant_qualifiers.empty() == false){
			resultant_qualifiers.reserve(target_type.qualifiers().size() - 1);
			for(size_t i = 0; i < target_type.qualifiers().size() - 1; i+=1){
				resultant_qualifiers.emplace_back(target_type.qualifiers()[i]);
			}
		}

		const TypeInfo::ID resultant_type_id = this->context.type_manager.getOrCreateTypeInfo(
			TypeInfo(target_type.baseTypeID(), std::move(resultant_qualifiers))
		);


		const std::optional<SymbolProc::ID> resultant_type_symbol_proc_id =
			this->context.symbol_proc_manager.getTypeSymbolProc(resultant_type_id);

		if(resultant_type_symbol_proc_id.has_value()){
			const SymbolProc::WaitOnResult wait_on_result = this->context.symbol_proc_manager
				.getSymbolProc(*resultant_type_symbol_proc_id)
				.waitOnDefIfNeeded(this->symbol_proc_id, this->context, *resultant_type_symbol_proc_id);

			switch(wait_on_result){
				case SymbolProc::WaitOnResult::NOT_NEEDED:                 break;
				case SymbolProc::WaitOnResult::WAITING:                    return Result::NEED_TO_WAIT;
				case SymbolProc::WaitOnResult::WAS_ERRORED:                return Result::ERROR;
				case SymbolProc::WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND: evo::debugFatalBreak("Not possible");
				case SymbolProc::WaitOnResult::CIRCULAR_DEP_DETECTED:      return Result::ERROR;
			}
		}


		using ValueCategory = TermInfo::ValueCategory;

		this->return_term_info(instr.output,
			target_type.qualifiers().back().isReadOnly ? ValueCategory::CONCRETE_CONST : ValueCategory::CONCRETE_MUT,
			target.value_stage,
			target_type.qualifiers().back().isUninit
				? TermInfo::ValueState::UNINIT
				: TermInfo::ValueState::NOT_APPLICABLE,
			resultant_type_id,
			sema::Expr(this->context.sema_buffer.createDeref(target.getExpr(), resultant_type_id))
		);

		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_unwrap(const Instruction::Unwrap& instr) -> Result {
		const TermInfo& target = this->get_term_info(instr.target);

		if(
			target.value_state != TermInfo::ValueState::INIT
			&& target.value_state != TermInfo::ValueState::NOT_APPLICABLE
		){
			this->emit_error(
				Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
				instr.postfix.lhs,
				"Argument of operator [.?] must be initialized"
			);
			return Result::ERROR;
		}

		if(target.type_id.is<TypeInfo::ID>() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_UNWRAP_ARG_NOT_OPTIONAL,
				instr.postfix.lhs,
				"Argument of operator [.?] must be an optional"
			);
			return Result::ERROR;
		}

		const TypeInfo& target_type = this->context.getTypeManager().getTypeInfo(target.type_id.as<TypeInfo::ID>());

		if(target_type.isOptional() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_UNWRAP_ARG_NOT_OPTIONAL,
				instr.postfix.lhs,
				"Argument of operator [.?] must be an opional"
			);
			return Result::ERROR;
		}


		auto resultant_qualifiers = evo::SmallVector<AST::Type::Qualifier>(
			target_type.qualifiers().begin(), target_type.qualifiers().end()
		);
		resultant_qualifiers.back().isOptional = false;
		if(target_type.isPointer() == false){
			resultant_qualifiers.back().isPtr = true;
			resultant_qualifiers.back().isReadOnly = target.is_const();
		}
		const TypeInfo::ID resultant_type_id = this->context.type_manager.getOrCreateTypeInfo(
			TypeInfo(target_type.baseTypeID(), std::move(resultant_qualifiers))
		);

		this->return_term_info(instr.output,
			TermInfo::ValueCategory::EPHEMERAL,
			target.value_stage,
			TermInfo::ValueState::NOT_APPLICABLE,
			resultant_type_id,
			sema::Expr(this->context.sema_buffer.createUnwrap(target.getExpr(), target.type_id.as<TypeInfo::ID>()))
		);
		return Result::SUCCESS;
	}



	template<bool IS_CONSTEXPR>
	auto SemanticAnalyzer::instr_new(const Instruction::New<IS_CONSTEXPR>& instr) -> Result {
		const TypeInfo::VoidableID target_type_id = this->get_type(instr.type_id);
		if(target_type_id.isVoid()){
			this->emit_error(
				Diagnostic::Code::SEMA_NEW_TYPE_VOID,
				instr.ast_new.type,
				"Operator [new] cannot accept type `Void`"
			);
			return Result::ERROR;
		}

		const TypeInfo& actual_target_type_info = this->context.getTypeManager().getTypeInfo(
			this->get_actual_type<true, true>(target_type_id.asTypeID())
		);


		if(actual_target_type_info.qualifiers().empty() == false){
			if(actual_target_type_info.isOptional()){
				if(instr.args.empty()){
					this->return_term_info(instr.output,
						TermInfo::ValueCategory::EPHEMERAL,
						TermInfo::ValueStage::CONSTEXPR,
						TermInfo::ValueState::NOT_APPLICABLE,
						target_type_id.asTypeID(),
						sema::Expr(this->context.sema_buffer.createNull(target_type_id.asTypeID()))
					);
					return Result::SUCCESS;

				}else if(instr.args.size() == 1){
					TermInfo& arg = this->get_term_info(instr.args[0]);

					if(instr.ast_new.args[0].label.has_value()){
						this->emit_error(
							Diagnostic::Code::SEMA_NEW_OPTIONAL_NO_MATCHING_OVERLOAD,
							instr.ast_new.type,
							"No matching operator [new] overload for this type",
							Diagnostic::Info("No operator [new] of optional accepts arguments with labels")
						);
						return Result::ERROR;
					}

					if(arg.value_category == TermInfo::ValueCategory::NULL_VALUE){
						this->return_term_info(instr.output,
							TermInfo::ValueCategory::EPHEMERAL,
							TermInfo::ValueStage::CONSTEXPR,
							TermInfo::ValueState::NOT_APPLICABLE,
							target_type_id.asTypeID(),
							sema::Expr(this->context.sema_buffer.createNull(target_type_id.asTypeID()))
						);

					}else{
						if(arg.is_ephemeral() == false){
							this->emit_error(
								Diagnostic::Code::SEMA_NEW_OPTIONAL_ARG_NOT_EPHEMERAL,
								instr.ast_new.args[0].value,
								"Argument in operator [new] for optional must be ephemeral or [null]"
							);
							return Result::ERROR;
						}

						const TypeInfo::ID optional_held_type_id = this->context.type_manager.getOrCreateTypeInfo(
							TypeInfo(
								actual_target_type_info.baseTypeID(),
								evo::SmallVector<AST::Type::Qualifier>(
									actual_target_type_info.qualifiers().begin(),
									std::prev(actual_target_type_info.qualifiers().end())
								)
							)
						);

						if(this->type_check<true, true>(
							optional_held_type_id,
							arg,
							"Argument in operator [new] for optional",
							instr.ast_new.args[0].value
						).ok == false){
							return Result::ERROR;
						}

						this->return_term_info(instr.output,
							TermInfo::ValueCategory::EPHEMERAL,
							TermInfo::ValueStage::CONSTEXPR,
							TermInfo::ValueState::NOT_APPLICABLE,
							target_type_id.asTypeID(),
							sema::Expr(
								this->context.sema_buffer.createConversionToOptional(
									arg.getExpr(), target_type_id.asTypeID()
								)
							)
						);
					}

					return Result::SUCCESS;

				}else{
					this->emit_error(
						Diagnostic::Code::SEMA_NEW_OPTIONAL_NO_MATCHING_OVERLOAD,
						instr.ast_new.type,
						"No matching operator [new] overload for this type",
						Diagnostic::Info("Too may arguments")
					);
					return Result::ERROR;
				}
			}

			this->emit_error(
				Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
				instr.ast_new.type,
				"Operator [new] of this type is unimplemented"
			);
			return Result::ERROR;
		}


		switch(actual_target_type_info.baseTypeID().kind()){
			case BaseType::Kind::PRIMITIVE: {
				if(instr.args.empty()){
					const BaseType::Primitive& primitive =
						this->context.getTypeManager().getPrimitive(actual_target_type_info.baseTypeID().primitiveID());

					if(primitive.kind() == Token::Kind::TYPE_RAWPTR || primitive.kind() == Token::Kind::TYPE_TYPEID){
						this->emit_error(
							Diagnostic::Code::SEMA_NEW_PRIMITIVE_NO_MATCHING_OVERLOAD,
							instr.ast_new.type,
							"No matching operator [new] overload for this type"
						);
						return Result::ERROR;
					}

					this->return_term_info(instr.output,
						TermInfo::ValueCategory::EPHEMERAL,
						TermInfo::ValueStage::CONSTEXPR,
						TermInfo::ValueState::NOT_APPLICABLE,
						target_type_id.asTypeID(),
						sema::Expr(this->context.sema_buffer.createDefaultInitPrimitive(
							actual_target_type_info.baseTypeID()
						))
					);
					return Result::SUCCESS;
					
				}else if(instr.args.size() == 1){
					TermInfo& arg = this->get_term_info(instr.args[0]);

					if(arg.is_ephemeral() == false){
						this->emit_error(
							Diagnostic::Code::SEMA_NEW_PRIMITIVE_ARG_NOT_EPHEMERAL,
							instr.ast_new.args[0].value,
							"Argument in operator [new] for primitive must be ephemeral"
						);
						return Result::ERROR;
					}

					if(this->type_check<true, true>(
						target_type_id.asTypeID(),
						arg,
						"Argument of operator [new] for primitive",
						instr.ast_new.args[0].value
					).ok == false){
						return Result::ERROR;
					}

					this->return_term_info(instr.output, arg);
					return Result::SUCCESS;
					
				}else{
					this->emit_error(
						Diagnostic::Code::SEMA_NEW_PRIMITIVE_NO_MATCHING_OVERLOAD,
						instr.ast_new.type,
						"No matching operator [new] overload for this type",
						Diagnostic::Info("Too may arguments")
					);
					return Result::ERROR;
				}
			} break;

			case BaseType::Kind::ARRAY_REF: {
				if(instr.args.empty()){
					this->return_term_info(instr.output,
						TermInfo::ValueCategory::EPHEMERAL,
						TermInfo::ValueStage::CONSTEXPR,
						TermInfo::ValueState::NOT_APPLICABLE,
						target_type_id.asTypeID(),
						sema::Expr(this->context.sema_buffer.createDefaultInitArrayRef(
							actual_target_type_info.baseTypeID().arrayRefID()
						))
					);
					return Result::SUCCESS;
				}

				const BaseType::ArrayRef& array_ref =
					this->context.getTypeManager().getArrayRef(actual_target_type_info.baseTypeID().arrayRefID());

				const size_t num_ref_ptrs = array_ref.getNumRefPtrs();

				if(instr.args.size() != num_ref_ptrs + 1){
					this->emit_error(
						Diagnostic::Code::SEMA_NEW_ARRAY_REF_NO_MATCHING_OVERLOAD,
						instr.ast_new.type,
						"No matching operator [new] overload for this type",
						Diagnostic::Info(
							std::format("Expected {} arguments, got {}", num_ref_ptrs + 1, instr.args.size())
						)
					);
					return Result::ERROR;
				}

				if(this->get_term_info(instr.args[0]).is_ephemeral() == false){
					this->emit_error(
						Diagnostic::Code::SEMA_NEW_ARRAY_REF_ARG_NOT_EPHEMERAL,
						instr.ast_new.args[0].value,
						"Argument in operator [new] for array reference must be ephemeral"
					);
					return Result::ERROR;
				}

				const TypeInfo::ID array_ptr_type = this->context.type_manager.getOrCreateTypeInfo(
					this->context.getTypeManager().getTypeInfo(array_ref.elementTypeID)
						.copyWithPushedQualifier(AST::Type::Qualifier(true, array_ref.isReadOnly, false, false))
				);

				if(this->type_check<true, true>(
					array_ptr_type,
					this->get_term_info(instr.args[0]),
					"Pointer argument of operator [new] for array reference",
					instr.ast_new.args[0].value
				).ok == false){
					return Result::ERROR;
				}

				if(instr.ast_new.args[0].label.has_value()){
					this->emit_error(
						Diagnostic::Code::SEMA_NEW_ARRAY_REF_NO_MATCHING_OVERLOAD,
						instr.ast_new.type,
						"No matching operator [new] overload for this type",
						Diagnostic::Info("No operator [new] of array reference accepts arguments with labels")
					);
				}


				for(size_t i = 1; i < num_ref_ptrs + 1; i+=1){
					TermInfo& arg_term_info = this->get_term_info(instr.args[i]);

					if(arg_term_info.is_ephemeral() == false){
						this->emit_error(
							Diagnostic::Code::SEMA_NEW_ARRAY_REF_ARG_NOT_EPHEMERAL,
							instr.ast_new.args[i].value,
							"Argument in operator [new] for array reference must be ephemeral"
						);
						return Result::ERROR;
					}

					if(this->type_check<true, true>(
						TypeManager::getTypeUSize(),
						arg_term_info,
						"Dimension argument of operator [new] for array reference",
						instr.ast_new.args[i].value
					).ok == false){
						return Result::ERROR;
					}

					if(instr.ast_new.args[i].label.has_value()){
						this->emit_error(
							Diagnostic::Code::SEMA_NEW_ARRAY_REF_NO_MATCHING_OVERLOAD,
							instr.ast_new.type,
							"No matching operator [new] overload for this type",
							Diagnostic::Info("No operator [new] of array reference accepts arguments with labels")
						);
					}
				}


				auto dimensions = evo::SmallVector<evo::Variant<uint64_t, sema::Expr>>();
				dimensions.reserve(array_ref.dimensions.size());
				for(const BaseType::ArrayRef::Dimension& dimension : array_ref.dimensions){
					if(dimension.isPtr()){
						dimensions.emplace_back(this->get_term_info(instr.args[dimensions.size() + 1]).getExpr());
					}else{
						dimensions.emplace_back(dimension.length());
					}
				}

				this->return_term_info(instr.output,
					TermInfo::ValueCategory::EPHEMERAL,
					TermInfo::ValueStage::CONSTEXPR,
					TermInfo::ValueState::NOT_APPLICABLE,
					target_type_id.asTypeID(),
					sema::Expr(this->context.sema_buffer.createInitArrayRef(
						this->get_term_info(instr.args[0]).getExpr(), std::move(dimensions)
					))
				);
				return Result::SUCCESS;
			} break;

			case BaseType::Kind::STRUCT: {
				// just break as will be done after switch
			} break;

			default: {
				this->emit_error(
					Diagnostic::Code::SEMA_NEW_STRUCT_NO_MATCHING_OVERLOAD,
					instr.ast_new.type,
					"No matching operator [new] overload for this type"
				);
				return Result::ERROR;
			} break;
		}


		const BaseType::Struct& target_struct =
			this->context.getTypeManager().getStruct(actual_target_type_info.baseTypeID().structID());

		if(instr.args.empty() && target_struct.isTriviallyDefaultInitializable){
			this->return_term_info(instr.output,
				TermInfo::ValueCategory::EPHEMERAL,
				this->get_current_func().isConstexpr ? TermInfo::ValueStage::COMPTIME : TermInfo::ValueStage::RUNTIME,
				TermInfo::ValueState::NOT_APPLICABLE,
				target_type_id.asTypeID(),
				sema::Expr(
					this->context.sema_buffer.createDefaultTriviallyInitStruct(actual_target_type_info.baseTypeID())
				)
			);
			return Result::SUCCESS;
		}

		if(target_struct.newInitOverloads.empty()){
			this->emit_error(
				Diagnostic::Code::SEMA_NEW_STRUCT_NO_MATCHING_OVERLOAD,
				instr.ast_new.type,
				"No matching operator [new] overload for this type",
				Diagnostic::Info("Compiler didn't generate a default operator [new]")
			);
			return Result::ERROR;
		}


		auto overloads = evo::SmallVector<SelectFuncOverloadFuncInfo>();
		overloads.reserve(target_struct.newInitOverloads.size());
		for(const sema::Func::ID overload_id : target_struct.newInitOverloads){
			const sema::Func& overload = this->context.getSemaBuffer().getFunc(overload_id);

			overloads.emplace_back(overload_id, this->context.getTypeManager().getFunction(overload.typeID));
		}


		auto args = evo::SmallVector<SelectFuncOverloadArgInfo>();
		args.reserve(instr.args.size());
		for(size_t i = 0; const SymbolProc::TermInfoID& arg_id : instr.args){
			args.emplace_back(this->get_term_info(arg_id), instr.ast_new.args[i].value, instr.ast_new.args[i].label);

			i += 1;
		}

		const evo::Result<size_t> selected_overload =
			this->select_func_overload(overloads, args, instr.ast_new, false, evo::SmallVector<Diagnostic::Info>());
		if(selected_overload.isError()){ return Result::ERROR; }

		const sema::Func::ID selected_func_id = overloads[selected_overload.value()].func_id.as<sema::Func::ID>();
		const sema::Func& selected_func = this->context.getSemaBuffer().getFunc(selected_func_id);


		auto output_args = evo::SmallVector<sema::Expr>();
		output_args.reserve(selected_func.params.size());
		for(const SymbolProc::TermInfoID& arg_id : instr.args){
			output_args.emplace_back(this->get_term_info(arg_id).getExpr());
		}

		// default values
		for(size_t i = output_args.size(); i < selected_func.params.size(); i+=1){
			output_args.emplace_back(*selected_func.params[i].defaultValue);
		}



		if constexpr(IS_CONSTEXPR){
			this->emit_error(
				Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
				instr.ast_new,
				"Constexpr operator [new] is unimplemented"
			);
			return Result::ERROR;

		}else{
			const sema::FuncCall::ID created_func_call_id = this->context.sema_buffer.createFuncCall(
				selected_func_id, std::move(output_args)
			);

			this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>().dependent_funcs.emplace(selected_func_id);

			this->return_term_info(instr.output,
				TermInfo::ValueCategory::EPHEMERAL,
				this->get_current_func().isConstexpr ? TermInfo::ValueStage::COMPTIME : TermInfo::ValueStage::RUNTIME,
				TermInfo::ValueState::NOT_APPLICABLE,
				target_type_id.asTypeID(),
				sema::Expr(created_func_call_id)
			);
			return Result::SUCCESS;
		}

	}



	template<bool IS_CONSTEXPR>
	auto SemanticAnalyzer::instr_array_init_new(const Instruction::ArrayInitNew<IS_CONSTEXPR>& instr) -> Result {
		const TypeInfo::VoidableID target_type_id = this->get_type(instr.type_id);
		if(target_type_id.isVoid()){
			this->emit_error(
				Diagnostic::Code::SEMA_NEW_TYPE_VOID,
				instr.array_init_new.type,
				"Operator [new] cannot accept type `Void`"
			);
			return Result::ERROR;
		}

		const TypeInfo& actual_target_type_info = this->context.getTypeManager().getTypeInfo(
			this->get_actual_type<true, true>(target_type_id.asTypeID())
		);

		if(
			actual_target_type_info.qualifiers().empty() == false
			|| actual_target_type_info.baseTypeID().kind() != BaseType::Kind::ARRAY
		){
			this->emit_error(
				Diagnostic::Code::SEMA_NEW_ARRAY_INIT_NOT_ARRAY,
				instr.array_init_new.type,
				"Array initializer operator [new] cannot accept a type that's not an array"
			);
			return Result::ERROR;
		}

		const BaseType::Array& target_type = this->context.getTypeManager().getArray(
			actual_target_type_info.baseTypeID().arrayID()
		);

		if(target_type.dimensions.size() > 1){
			this->emit_error(
				Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
				instr.array_init_new.type,
				"Array initializer operator [new] for multi-dimensional array types is currently unimplemented"
			);
			return Result::ERROR;
		}

		if(instr.values.size() != target_type.dimensions[0]){
			this->emit_error(
				Diagnostic::Code::SEMA_NEW_ARRAY_INIT_INCORRECT_SIZE,
				instr.array_init_new.keyword,
				"Array initializer operator [new] got incorrect number of values",
				Diagnostic::Info(std::format("Expected {}, got {}", target_type.dimensions[0], instr.values.size()))
			);
			return Result::ERROR;
		}


		auto values = evo::SmallVector<sema::Expr>();
		values.reserve(instr.values.size() + size_t(target_type.terminator.has_value()));

		for(size_t i = 0; const SymbolProc::TermInfoID value_id : instr.values){
			TermInfo& value = this->get_term_info(value_id);

			if(value.is_ephemeral() == false){
				this->emit_error(
					Diagnostic::Code::SEMA_NEW_ARRAY_INIT_VAL_NOT_EPHERMERAL,
					instr.array_init_new.values[i],
					"Array initializer operator [new] value initializer must be ephemeral"
				);
				return Result::ERROR;
			}

			if(this->type_check<true, true>(
				target_type.elementTypeID, value, "Value initializer", instr.array_init_new.values[i]
			).ok == false){
				return Result::ERROR;
			}

			values.emplace_back(value.getExpr());

			i += 1;
		}

		if(target_type.terminator.has_value()){
			values.emplace_back(
				this->generic_value_to_sema_expr(
					*target_type.terminator, this->context.getTypeManager().getTypeInfo(target_type.elementTypeID)
				)
			);
		}

		const sema::AggregateValue::ID created_aggregate_value = this->context.sema_buffer.createAggregateValue(
			std::move(values), actual_target_type_info.baseTypeID()
		);

		const TermInfo::ValueStage value_stage = [&](){
			if constexpr(IS_CONSTEXPR){
				return TermInfo::ValueStage::CONSTEXPR;
			}else{
				if(this->currently_in_func() == false){
					return TermInfo::ValueStage::CONSTEXPR;

				}else if(this->get_current_func().isConstexpr){
					return TermInfo::ValueStage::COMPTIME;

				}else{
					return TermInfo::ValueStage::RUNTIME;
				}
			}
		}();


		this->return_term_info(instr.output,
			TermInfo::ValueCategory::EPHEMERAL,
			value_stage,
			TermInfo::ValueState::NOT_APPLICABLE,
			target_type_id.asTypeID(),
			sema::Expr(created_aggregate_value)
		);
		return Result::SUCCESS;
	}


	template<bool IS_CONSTEXPR>
	auto SemanticAnalyzer::instr_designated_init_new(const Instruction::DesignatedInitNew<IS_CONSTEXPR>& instr)
	-> Result {
		const TypeInfo::VoidableID target_type_id = this->get_type(instr.type_id);
		if(target_type_id.isVoid()){
			this->emit_error(
				Diagnostic::Code::SEMA_NEW_TYPE_VOID,
				instr.designated_init_new.type,
				"Operator [new] cannot accept type `Void`"
			);
			return Result::ERROR;
		}

		const TypeInfo& target_type_info = this->context.getTypeManager().getTypeInfo(
			this->get_actual_type<true, true>(target_type_id.asTypeID())
		);
		if(target_type_info.qualifiers().empty() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_NEW_STRUCT_INIT_NOT_STRUCT,
				instr.designated_init_new.type,
				"Designated initializer operator [new] cannot accept a type that's not a struct or a union"
			);
			return Result::ERROR;
		}


		if(target_type_info.baseTypeID().kind() == BaseType::Kind::UNION){
			return this->union_designated_init_new(instr, target_type_id.asTypeID());

		}else if(target_type_info.baseTypeID().kind() != BaseType::Kind::STRUCT){
			this->emit_error(
				Diagnostic::Code::SEMA_NEW_STRUCT_INIT_NOT_STRUCT,
				instr.designated_init_new.type,
				"Designated initializer operator [new] cannot accept a type that's not a struct or a union"
			);
			return Result::ERROR;
		}



		const BaseType::Struct& target_type = this->context.getTypeManager().getStruct(
			target_type_info.baseTypeID().structID()
		);


		if(target_type.newInitOverloads.empty() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_NEW_STRUCT_DESIGNATED_ON_TYPE_WITH_OVEROAD_NEW,
				instr.designated_init_new,
				"Designatied initializer operator [new] on type that has overloaded operator [new]"
			);
			return Result::ERROR;
		}


		if(target_type.memberVars.empty()){
			if(instr.designated_init_new.memberInits.empty() == false){
				const AST::DesignatedInitNew::MemberInit& member_init = instr.designated_init_new.memberInits[0];

				const std::string_view member_init_ident = this->source.getTokenBuffer()[member_init.ident].getString();

				this->emit_error(
					Diagnostic::Code::SEMA_NEW_STRUCT_MEMBER_DOESNT_EXIST,
					member_init.ident,
					std::format("This struct has no member \"{}\"", member_init_ident),
					evo::SmallVector<Diagnostic::Info>{
						Diagnostic::Info("Struct is empty"),
						Diagnostic::Info(
							"Struct was declared here:",
							this->get_location(target_type_info.baseTypeID().structID())
						)
					}
				);
				return Result::ERROR;
			}


			const sema::AggregateValue::ID created_aggregate_value = this->context.sema_buffer.createAggregateValue(
				evo::SmallVector<sema::Expr>(), target_type_info.baseTypeID()
			);

			const TermInfo::ValueStage value_stage = [&](){
				if constexpr(IS_CONSTEXPR){
					return TermInfo::ValueStage::CONSTEXPR;
				}else{
					if(this->currently_in_func() == false){
						return TermInfo::ValueStage::CONSTEXPR;

					}else if(this->get_current_func().isConstexpr){
						return TermInfo::ValueStage::COMPTIME;

					}else{
						return TermInfo::ValueStage::RUNTIME;
					}
				}
			}();


			this->return_term_info(instr.output,
				TermInfo::ValueCategory::EPHEMERAL,
				value_stage,
				TermInfo::ValueState::NOT_APPLICABLE,
				target_type_id.asTypeID(),
				sema::Expr(created_aggregate_value)
			);
			return Result::SUCCESS;
		}



		const auto struct_has_member = [&](std::string_view ident) -> bool {
			for(const BaseType::Struct::MemberVar& member_var : target_type.memberVars){
				const std::string_view member_var_ident = 
					target_type.getMemberName(member_var, this->context.getSourceManager());
			
				if(member_var_ident == ident){ return true; }
			}

			return false;
		};

		auto values = evo::SmallVector<sema::Expr>();
		values.reserve(target_type.memberVars.size());

		size_t member_init_i = 0;
		for(const BaseType::Struct::MemberVar* member_var : target_type.memberVarsABI){
			const std::string_view member_var_ident =
				target_type.getMemberName(*member_var, this->context.getSourceManager());

			if(member_init_i >= instr.designated_init_new.memberInits.size()){
				if(member_var->defaultValue.has_value()){
					values.emplace_back(member_var->defaultValue->value);
					continue;
				}

				if(instr.designated_init_new.memberInits.empty()){
					this->emit_error(
						Diagnostic::Code::SEMA_NEW_STRUCT_MEMBER_NOT_SET,
						instr.designated_init_new,
						std::format("Member \"{}\" was not set in struct initializer operator [new]", member_var_ident)
					);
				}else{
					this->emit_error(
						Diagnostic::Code::SEMA_NEW_STRUCT_MEMBER_NOT_SET,
						instr.designated_init_new,
						std::format("Member \"{}\" was not set in struct initializer operator [new]", member_var_ident),
						Diagnostic::Info(
							std::format("Member initializer for \"{}\" should go after this one", member_var_ident),
							this->get_location(instr.designated_init_new.memberInits[member_init_i - 1].ident)
						)
					);
				}

				return Result::ERROR;

			}else{
				const AST::DesignatedInitNew::MemberInit& member_init =
					instr.designated_init_new.memberInits[member_init_i];


				const std::string_view member_init_ident = this->source.getTokenBuffer()[member_init.ident].getString();

				if(member_var_ident != member_init_ident){
					if(member_var->defaultValue.has_value()){
						values.emplace_back(member_var->defaultValue->value);
						continue;
					}

					if(struct_has_member(member_init_ident)){
						this->emit_error(
							Diagnostic::Code::SEMA_NEW_STRUCT_MEMBER_NOT_SET,
							instr.designated_init_new,
							std::format(
								"Member \"{}\" was not set in struct initializer operator [new]", member_var_ident
							),
							Diagnostic::Info(
								std::format(
									"Member initializer for \"{}\" should go before this one", member_var_ident
								),
								this->get_location(member_init.ident)
							)
						);
					}else{
						this->emit_error(
							Diagnostic::Code::SEMA_NEW_STRUCT_MEMBER_DOESNT_EXIST,
							member_init.ident,
							std::format("This struct has no member \"{}\"", member_init_ident),
							Diagnostic::Info(
								"Struct was declared here:",
								this->get_location(target_type_info.baseTypeID().structID())
							)
						);
						return Result::ERROR;
					}

					return Result::ERROR;
				}

				TermInfo& member_init_expr = this->get_term_info(instr.member_init_exprs[member_init_i]);

				if(member_init_expr.is_ephemeral() == false){
					this->emit_error(
						Diagnostic::Code::SEMA_NEW_STRUCT_MEMBER_VAL_NOT_EPHEMERAL,
						member_init.expr,
						"Member initializer value is not ephemeral"
					);
					return Result::ERROR;
				}

				if(this->type_check<true, true>(
					member_var->typeID, member_init_expr, "Member initializer", member_init.expr
				).ok == false){
					return Result::ERROR;
				}

				values.emplace_back(member_init_expr.getExpr());

				member_init_i += 1;
			}
		}


		if(member_init_i < instr.designated_init_new.memberInits.size()){
			const AST::DesignatedInitNew::MemberInit& member_init =
				instr.designated_init_new.memberInits[member_init_i];

			const std::string_view member_init_ident = this->source.getTokenBuffer()[member_init.ident].getString();

			this->emit_error(
				Diagnostic::Code::SEMA_NEW_STRUCT_MEMBER_DOESNT_EXIST,
				member_init.ident,
				std::format("This struct has no member \"{}\"", member_init_ident),
				Diagnostic::Info(
					"Struct was declared here:",  this->get_location(target_type_info.baseTypeID().structID())
				)
			);
			return Result::ERROR;
		}


		const sema::AggregateValue::ID created_aggregate_value = this->context.sema_buffer.createAggregateValue(
			std::move(values), target_type_info.baseTypeID()
		);

		const TermInfo::ValueStage value_stage = [&](){
			if constexpr(IS_CONSTEXPR){
				return TermInfo::ValueStage::CONSTEXPR;
			}else{
				if(this->currently_in_func() == false){
					return TermInfo::ValueStage::CONSTEXPR;

				}else if(this->get_current_func().isConstexpr){
					return TermInfo::ValueStage::COMPTIME;

				}else{
					return TermInfo::ValueStage::RUNTIME;
				}
			}
		}();


		this->return_term_info(instr.output,
			TermInfo::ValueCategory::EPHEMERAL,
			value_stage,
			TermInfo::ValueState::NOT_APPLICABLE,
			target_type_id.asTypeID(),
			sema::Expr(created_aggregate_value)
		);
		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_prepare_try_handler(const Instruction::PrepareTryHandler& instr) -> Result {
		this->push_scope_level();

		const SemaBuffer& sema_buffer = this->context.getSemaBuffer();

		const TermInfo& attempt_expr = this->get_term_info(instr.attempt_expr);

		
		const BaseType::Function& attempt_func_type = [&]() -> const BaseType::Function& {
			if(attempt_expr.getExpr().kind() == sema::Expr::Kind::INTERFACE_CALL){
				const sema::InterfaceCall& interface_call = 
					this->context.getSemaBuffer().getInterfaceCall(attempt_expr.getExpr().interfaceCallID());

				return this->context.getTypeManager().getFunction(interface_call.funcTypeID);

			}else{
				const sema::FuncCall& attempt_func_call = sema_buffer.getFuncCall(attempt_expr.getExpr().funcCallID());

				return *attempt_func_call.target.visit([&](const auto& target) -> const BaseType::Function* {
					using Target = std::decay_t<decltype(target)>;

					if constexpr(std::is_same<Target, sema::Func::ID>()){
						return &this->context.getTypeManager().getFunction(sema_buffer.getFunc(target).typeID);
						
					}else if constexpr(std::is_same<Target, IntrinsicFunc::Kind>()){
						const TypeInfo::ID type_info_id = this->context.getIntrinsicFuncInfo(target).typeID;
						const TypeInfo& type_info = this->context.getTypeManager().getTypeInfo(type_info_id);
						return &this->context.getTypeManager().getFunction(type_info.baseTypeID().funcID());
						
					}else if constexpr(std::is_same<Target, sema::TemplateIntrinsicFuncInstantiation::ID>()){
						const sema::TemplateIntrinsicFuncInstantiation& instantiation =
							this->context.getSemaBuffer().getTemplateIntrinsicFuncInstantiation(target);

						const Context::TemplateIntrinsicFuncInfo& template_intrinsic_func_info = 
							this->context.getTemplateIntrinsicFuncInfo(instantiation.kind);

						auto instantiation_args = evo::SmallVector<std::optional<TypeInfo::VoidableID>>();
						instantiation_args.reserve(instantiation.templateArgs.size());
						using TemplateArg = evo::Variant<TypeInfo::VoidableID, core::GenericValue>;
						for(const TemplateArg& template_arg : instantiation.templateArgs){
							if(template_arg.is<TypeInfo::VoidableID>()){
								instantiation_args.emplace_back(template_arg.as<TypeInfo::VoidableID>());
							}else{
								instantiation_args.emplace_back();
							}
						}

						return &this->context.getTypeManager().getFunction(
							this->context.type_manager.getOrCreateFunction(
								template_intrinsic_func_info.getTypeInstantiation(instantiation_args)
							).funcID()
						);
						
					}else{
						static_assert(false, "Unsupported func call target");
					}
				});
			}
		}();

		
		if(
			attempt_func_type.errorParams.size() != instr.except_params.size()
			&& attempt_func_type.errorParams[0].typeID.isVoid() == false
		){
			this->emit_error(
				Diagnostic::Code::SEMA_TRY_EXCEPT_PARAMS_WRONG_NUM,
				instr.handler_kind_token_id,
				"Number of except parameters does not match attempt function call",
				Diagnostic::Info(
					std::format("Expected {}, got {}", attempt_func_type.errorParams.size(), instr.except_params.size())
				)
			);
			return Result::ERROR;
		}

		auto except_params = evo::SmallVector<sema::Expr>();
		except_params.reserve(instr.except_params.size());
		for(size_t i = 0; i < instr.except_params.size(); i+=1){
			const Token& except_param_token = this->source.getTokenBuffer()[instr.except_params[i]];

			if(except_param_token.kind() == Token::lookupKind("_")){
				except_params.emplace_back(sema::Expr::createNone());
				continue;
			}

			const std::string_view except_param_ident_str = except_param_token.getString();

			const sema::ExceptParam::ID except_param_id = this->context.sema_buffer.createExceptParam(
				instr.except_params[i], uint32_t(i), attempt_func_type.errorParams[i].typeID.asTypeID()
			);
			except_params.emplace_back(sema::Expr(except_param_id));

			if(this->add_ident_to_scope(except_param_ident_str, instr.except_params[i], except_param_id).isError()){
				return Result::ERROR;
			}

			this->get_current_scope_level().addIdentValueState(except_param_id, sema::ScopeLevel::ValueState::INIT);
		}

		this->return_term_info(instr.output_except_params,
			TermInfo::ValueCategory::EXCEPT_PARAM_PACK,
			this->get_current_func().isConstexpr ? TermInfo::ValueStage::COMPTIME : TermInfo::ValueStage::RUNTIME,
			TermInfo::ExceptParamPack{},
			std::move(except_params)
		);
		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_try_else_expr(const Instruction::TryElseExpr& instr) -> Result {
		const TermInfo& attempt_expr = this->get_term_info(instr.attempt_expr);
		TermInfo& except_expr = this->get_term_info(instr.except_expr);

		if(attempt_expr.value_category != TermInfo::ValueCategory::EPHEMERAL){
			this->emit_error(
				Diagnostic::Code::SEMA_TRY_ELSE_ATTEMPT_NOT_FUNC_CALL,
				instr.try_else.attemptExpr,
				"Attempt in try/else expression is not function call"
			);
			return Result::ERROR;
		}

		if(
			attempt_expr.getExpr().kind() != sema::Expr::Kind::FUNC_CALL
			&& attempt_expr.getExpr().kind() != sema::Expr::Kind::INTERFACE_CALL
		){
			this->emit_error(
				Diagnostic::Code::SEMA_TRY_ELSE_ATTEMPT_NOT_FUNC_CALL,
				instr.try_else.attemptExpr,
				"Attempt in try/else expression is not function call"
			);
			return Result::ERROR;
		}

		if(attempt_expr.isMultiValue()){
			this->emit_error(
				Diagnostic::Code::SEMA_TRY_ELSE_ATTEMPT_NOT_SINGLE,
				instr.try_else.attemptExpr,
				"Attempt function call in try/else expression returns multiple values"
			);
			return Result::ERROR;
		}

		if(except_expr.is_ephemeral() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_TRY_ELSE_EXCEPT_NOT_EPHEMERAL,
				instr.try_else.exceptExpr,
				"Except in try/else expression is not function call"
			);
			return Result::ERROR;
		}

		if(this->type_check<true, true>(
			attempt_expr.type_id.as<TypeInfo::ID>(),
			except_expr,
			"Except in try/else expression",
			instr.try_else.exceptExpr
		).ok == false){
			return Result::ERROR;
		}

		const TermInfo& except_params_term_info = this->get_term_info(instr.except_params);
		auto except_params = evo::SmallVector<sema::ExceptParam::ID>();
		except_params.reserve(except_params_term_info.getExceptParamPack().size());
		for(const sema::Expr& except_param : except_params_term_info.getExceptParamPack()){
			if(except_param.kind() == sema::Expr::Kind::EXCEPT_PARAM){
				except_params.emplace_back(except_param.exceptParamID());
			}
		}


		using ValueStage = TermInfo::ValueStage;
		const ValueStage value_stage = [&](){
			if(attempt_expr.value_stage == ValueStage::CONSTEXPR && except_expr.value_stage == ValueStage::CONSTEXPR){
				return TermInfo::ValueStage::CONSTEXPR;
			}

			if(attempt_expr.value_stage >= ValueStage::COMPTIME && except_expr.value_stage >= ValueStage::COMPTIME){
				return TermInfo::ValueStage::COMPTIME;
			}

			return TermInfo::ValueStage::RUNTIME;
		}();


		this->add_auto_delete_calls<AutoDeleteMode::NORMAL>();
		if(this->pop_scope_level().isError()){ return Result::ERROR; }


		const sema::Expr try_else_expr = [&](){
			if(attempt_expr.getExpr().kind() == sema::Expr::Kind::FUNC_CALL){
				return sema::Expr(
					this->context.sema_buffer.createTryElseExpr(
						attempt_expr.getExpr(), except_expr.getExpr(), std::move(except_params)
					)
				);
			}else{
				return sema::Expr(
					this->context.sema_buffer.createTryElseInterfaceExpr(
						attempt_expr.getExpr(), except_expr.getExpr(), std::move(except_params)
					)
				);
			}
		}();

		this->return_term_info(instr.output,
			TermInfo::ValueCategory::EPHEMERAL,
			value_stage,
			TermInfo::ValueState::NOT_APPLICABLE,
			attempt_expr.type_id,
			try_else_expr
		);
		return Result::SUCCESS;
	}




	auto SemanticAnalyzer::instr_begin_expr_block(const Instruction::BeginExprBlock& instr) -> Result {
		const sema::BlockExpr::ID sema_block_expr_id = this->context.sema_buffer.createBlockExpr(instr.label);
		sema::BlockExpr& sema_block_expr = this->context.sema_buffer.block_exprs[sema_block_expr_id];

		///////////////////////////////////
		// check for label reuse

		const std::string_view label_str = this->source.getTokenBuffer()[instr.label].getString();
		for(size_t i = this->scope.size() - 1; const sema::ScopeLevel::ID& target_scope_level_id : this->scope){
			EVO_DEFER([&](){ i -= 1; });

			if(i == this->scope.getCurrentObjectScopeIndex()){ break; }

			const sema::ScopeLevel& target_scope_level = 
				this->context.sema_buffer.scope_manager.getLevel(target_scope_level_id);

			if(target_scope_level.hasLabel() == false){ continue; }

			if(label_str == this->source.getTokenBuffer()[target_scope_level.getLabel()].getString()){
				this->emit_error(
					Diagnostic::Code::SEMA_IDENT_ALREADY_IN_SCOPE,
					instr.label,
					std::format("Label \"{}\" was already defined in this scope", label_str),
					Diagnostic::Info("First defined here:", this->get_location(target_scope_level.getLabel()))
				);
				return Result::ERROR;
			}
		}


		///////////////////////////////////
		// build outputs

		this->push_scope_level(sema_block_expr.block, instr.label, sema_block_expr_id);

		sema_block_expr.outputs.reserve(instr.output_types.size());
		for(size_t i = 0; const SymbolProc::TypeID& output_type_id : instr.output_types){
			const TypeInfo::VoidableID output_type = this->get_type(output_type_id);

			if(output_type.isVoid()){
				this->emit_error(
					Diagnostic::Code::SEMA_BLOCK_EXPR_OUTPUT_PARAM_VOID,
					instr.block.outputs[i].typeID,
					"Block expression output cannot be type `Void`"
				);
				return Result::ERROR;
			}

			sema_block_expr.outputs.emplace_back(output_type.asTypeID(), instr.block.outputs[i].ident);

			if(instr.block.outputs[i].ident.has_value()){
				const std::string_view ident_str = 
					this->source.getTokenBuffer()[*instr.block.outputs[i].ident].getString();

				const sema::BlockExprOutput::ID block_expr_output = 
					this->context.sema_buffer.createBlockExprOutput(uint32_t(i), instr.label, output_type.asTypeID());

				if(this->add_ident_to_scope(ident_str, *instr.block.outputs[i].ident, block_expr_output).isError()){
					return Result::ERROR;
				}

				this->get_current_scope_level().addIdentValueState(
					block_expr_output, sema::ScopeLevel::ValueState::UNINIT
				);
			}

			i += 1;
		}

		return Result::SUCCESS;
	}

	auto SemanticAnalyzer::instr_end_expr_block(const Instruction::EndExprBlock& instr) -> Result {
		if(this->get_current_scope_level().isTerminated() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_BLOCK_EXPR_NOT_TERMINATED, instr.block, "Block expression not terminated"
			);
			return Result::ERROR;
		}

		const sema::BlockExpr::ID sema_block_expr_id =
			this->get_current_scope_level().getLabelNode().as<sema::BlockExpr::ID>();
		const sema::BlockExpr& sema_block_expr = this->context.sema_buffer.getBlockExpr(sema_block_expr_id);

		if(sema_block_expr.outputs.size() == 1){
			this->return_term_info(instr.output,
				TermInfo::ValueCategory::EPHEMERAL,
				this->get_current_func().isConstexpr ? TermInfo::ValueStage::COMPTIME : TermInfo::ValueStage::RUNTIME,
				TermInfo::ValueState::NOT_APPLICABLE,
				sema_block_expr.outputs[0].typeID,
				sema::Expr(sema_block_expr_id)
			);
		}else{
			auto types = evo::SmallVector<TypeInfo::ID>();
			types.reserve(sema_block_expr.outputs.size());
			for(const sema::BlockExpr::Output& output : sema_block_expr.outputs){
				types.emplace_back(output.typeID);
			}

			this->return_term_info(instr.output,
				TermInfo::ValueCategory::EPHEMERAL,
				this->get_current_func().isConstexpr ? TermInfo::ValueStage::COMPTIME : TermInfo::ValueStage::RUNTIME,
				TermInfo::ValueState::NOT_APPLICABLE,
				std::move(types),
				sema::Expr(sema_block_expr_id)
			);
		}

		this->add_auto_delete_calls<AutoDeleteMode::NORMAL>();

		if(this->pop_scope_level<PopScopeLevelKind::LABEL_TERMINATE>().isError()){ return Result::ERROR; }
		if(this->end_sub_scopes(this->get_location(instr.block.closeBrace)).isError()){ return Result::ERROR; }
		return Result::SUCCESS;
	}



	template<bool IS_CONSTEXPR>
	auto SemanticAnalyzer::instr_indexer(const Instruction::Indexer<IS_CONSTEXPR>& instr) -> Result {
		const TermInfo& target = this->get_term_info(instr.target);

		if(target.type_id.is<TypeInfo::ID>() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_INDEXER_INVALID_TARGET,
				instr.indexer.target,
				"Invalid target for indexer"
			);
			return Result::ERROR;
		}

		if(
			target.value_state != TermInfo::ValueState::INIT
			&& target.value_state != TermInfo::ValueState::NOT_APPLICABLE
		){
			this->emit_error(
				Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
				instr.indexer.target,
				"Indexer target must be initialized"
			);
			return Result::ERROR;
		}

		const TypeInfo& actual_target_type = this->context.getTypeManager().getTypeInfo(
			this->get_actual_type<true, true>(target.type_id.as<TypeInfo::ID>())
		);



		bool is_arr_ref = false;
		bool is_read_only_arr_ref = false;
		bool is_ptr = false;

		auto elem_type = std::optional<TypeInfo::ID>();

		const TypeInfo* element_type = nullptr;

		if(actual_target_type.baseTypeID().kind() == BaseType::Kind::ARRAY_REF){
			is_arr_ref = true;

			if(actual_target_type.qualifiers().empty() == false){
				if(actual_target_type.isOptional()){
					this->emit_error(
						Diagnostic::Code::SEMA_INDEXER_INVALID_TARGET,
						instr.indexer,
						"Invalid target for indexer",
						Diagnostic::Info("Optional values need to be unwrapped")
					);
				}else{
					this->emit_error(
						Diagnostic::Code::SEMA_INDEXER_INVALID_TARGET,
						instr.indexer,
						"Invalid target for indexer"
					);
				}
				return Result::ERROR;
			}

			const BaseType::ArrayRef& target_array_ref_type =
				this->context.getTypeManager().getArrayRef(actual_target_type.baseTypeID().arrayRefID());

			is_read_only_arr_ref = target_array_ref_type.isReadOnly;

			if(target_array_ref_type.dimensions.size() != instr.indices.size()){
				this->emit_error(
					Diagnostic::Code::SEMA_INDEXER_INCORRECT_NUM_INDICES,
					instr.indexer.indices[std::min(instr.indices.size() - 1, target_array_ref_type.dimensions.size())],
					"Incorrect number of indices in indexer for the target array reference type",
					evo::SmallVector<Diagnostic::Info>{
						Diagnostic::Info(std::format("Expected: {}", target_array_ref_type.dimensions.size())),
						Diagnostic::Info(std::format("Got:      {}", instr.indices.size())),
					}
				);
				return Result::ERROR;
			}

			element_type = &this->context.type_manager.getTypeInfo(target_array_ref_type.elementTypeID);


		}else if(actual_target_type.baseTypeID().kind() == BaseType::Kind::ARRAY){
			if(actual_target_type.qualifiers().empty() == false){
				if(actual_target_type.qualifiers().size() == 1 && actual_target_type.isNormalPointer()){
					is_ptr = true;
				}else{
					if(actual_target_type.isOptional()){
						this->emit_error(
							Diagnostic::Code::SEMA_INDEXER_INVALID_TARGET,
							instr.indexer,
							"Invalid target for indexer",
							Diagnostic::Info("Optional values need to be unwrapped")
						);
					}else{
						this->emit_error(
							Diagnostic::Code::SEMA_INDEXER_INVALID_TARGET,
							instr.indexer,
							"Invalid target for indexer"
						);
					}
					return Result::ERROR;
				}
			}

			const BaseType::Array& target_array_type =
				this->context.getTypeManager().getArray(actual_target_type.baseTypeID().arrayID());

			if(target_array_type.dimensions.size() != instr.indices.size()){
				this->emit_error(
					Diagnostic::Code::SEMA_INDEXER_INCORRECT_NUM_INDICES,
					instr.indexer.indices[std::min(instr.indices.size() - 1, target_array_type.dimensions.size())],
					"Incorrect number of indices in indexer for the target array type",
					evo::SmallVector<Diagnostic::Info>{
						Diagnostic::Info(std::format("Expected: {}", target_array_type.dimensions.size())),
						Diagnostic::Info(std::format("Got:      {}", instr.indices.size())),
					}
				);
				return Result::ERROR;
			}

			element_type = &this->context.type_manager.getTypeInfo(target_array_type.elementTypeID);

		}else{
			this->emit_error(
				Diagnostic::Code::SEMA_INDEXER_INVALID_TARGET,
				instr.indexer,
				"Invalid target for indexer"
			);
			return Result::ERROR;
		}


		auto indices = evo::SmallVector<sema::Expr>();
		indices.reserve(instr.indices.size());
		for(size_t i = 0; const SymbolProc::TermInfoID index_id : instr.indices){
			TermInfo& index = this->get_term_info(index_id);

			if(this->type_check<true, true>(
				TypeManager::getTypeUSize(), index, "Index in indexer", instr.indexer.indices[i]
			).ok == false){
				return Result::ERROR;
			}

			indices.emplace_back(index.getExpr());

			i += 1;
		}


		auto resultant_qualifiers = evo::SmallVector<AST::Type::Qualifier>();
		resultant_qualifiers.reserve(element_type->qualifiers().size() + 1);
		for(const AST::Type::Qualifier& qualifier : element_type->qualifiers()){
			resultant_qualifiers.emplace_back(qualifier);
		}
		resultant_qualifiers.emplace_back(
			true,
			target.is_const() || (is_ptr && actual_target_type.qualifiers().back().isReadOnly) || is_read_only_arr_ref,
			false,
			false
		);

		const TypeInfo::ID resultant_type_id = this->context.type_manager.getOrCreateTypeInfo(
			TypeInfo(element_type->baseTypeID(), std::move(resultant_qualifiers))
		);

		const sema::Expr sema_indexer_expr = [&](){
			if(is_arr_ref){
				return sema::Expr(this->context.sema_buffer.createArrayRefIndexer(
					target.getExpr(),
					actual_target_type.baseTypeID().arrayRefID(),
					std::move(indices)
				));

			}else if(is_ptr){
				auto derefed_qualifiers = evo::SmallVector<AST::Type::Qualifier>();
				derefed_qualifiers.reserve(actual_target_type.qualifiers().size() - 1);
				for(size_t i = 0; i < actual_target_type.qualifiers().size() - 1; i+=1){
					derefed_qualifiers.emplace_back(actual_target_type.qualifiers()[i]);
				}

				const TypeInfo::ID derefed_type_id = this->context.type_manager.getOrCreateTypeInfo(
					TypeInfo(actual_target_type.baseTypeID(), std::move(derefed_qualifiers))
				);

				const sema::Deref::ID deref = this->context.sema_buffer.createDeref(target.getExpr(), derefed_type_id);

				return sema::Expr(this->context.sema_buffer.createIndexer(
					sema::Expr(deref), derefed_type_id, std::move(indices)
				));

			}else{
				return sema::Expr(this->context.sema_buffer.createIndexer(
					target.getExpr(), target.type_id.as<TypeInfo::ID>(), std::move(indices)
				));
			}
		}();


		this->return_term_info(instr.output,
			TermInfo::ValueCategory::EPHEMERAL,
			target.value_stage,
			TermInfo::ValueState::NOT_APPLICABLE,
			resultant_type_id,
			sema_indexer_expr
		);
		return Result::SUCCESS;
	}




	auto SemanticAnalyzer::instr_templated_term(const Instruction::TemplatedTerm& instr) -> Result {
		const TermInfo& templated_type_term_info = this->get_term_info(instr.base);

		if(templated_type_term_info.value_category != TermInfo::ValueCategory::TEMPLATE_TYPE){
			this->emit_error(
				Diagnostic::Code::SEMA_NOT_TEMPLATED_TYPE_WITH_TEMPLATE_ARGS,
				instr.templated_expr.base,
				"Base of templated type is not a template"
			);
			return Result::ERROR;
		}

		const sema::TemplatedStruct& sema_templated_struct = this->context.sema_buffer.templated_structs[
			templated_type_term_info.type_id.as<sema::TemplatedStruct::ID>()
		];

		BaseType::StructTemplate& struct_template = 
			this->context.type_manager.getStructTemplate(sema_templated_struct.templateID);


		///////////////////////////////////
		// check args

		if(instr.arguments.size() < struct_template.minNumTemplateArgs){
			auto infos = evo::SmallVector<Diagnostic::Info>();

			if(struct_template.hasAnyDefaultParams()){
				infos.emplace_back(
					std::format(
						"This type requires at least {}, got {}",
						struct_template.minNumTemplateArgs, instr.arguments.size()
					)
				);
			}else{
				infos.emplace_back(
					std::format(
						"This type requires {}, got {}", struct_template.minNumTemplateArgs, instr.arguments.size()
					)
				);
			}

			this->emit_error(
				Diagnostic::Code::SEMA_TEMPLATE_TOO_FEW_ARGS,
				instr.templated_expr,
				"Too few template arguments for this type",
				std::move(infos)
			);
			return Result::ERROR;
		}


		if(instr.arguments.size() > struct_template.params.size()){
			auto infos = evo::SmallVector<Diagnostic::Info>();

			if(struct_template.hasAnyDefaultParams()){
				infos.emplace_back(
					std::format(
						"This type requires at most {}, got {}",
						struct_template.params.size(), instr.arguments.size()
					)
				);
			}else{
				infos.emplace_back(
					std::format(
						"This type requires {}, got {}", struct_template.params.size(), instr.arguments.size()
					)
				);
			}

			this->emit_error(
				Diagnostic::Code::SEMA_TEMPLATE_TOO_MANY_ARGS,
				instr.templated_expr,
				"Too many template arguments for this type",
				std::move(infos)
			);
			return Result::ERROR;
		}


		///////////////////////////////////
		// get instantiation args

		bool is_deducer = false;


		this->scope.pushTemplateDeclInstantiationTypesScope();
		EVO_DEFER([&](){ this->scope.popTemplateDeclInstantiationTypesScope(); });

		const SemaBuffer& sema_buffer = this->context.getSemaBuffer();

		auto instantiation_lookup_args = evo::SmallVector<BaseType::StructTemplate::Arg>();
		instantiation_lookup_args.reserve(instr.arguments.size());

		auto instantiation_args = evo::SmallVector<evo::Variant<TypeInfo::VoidableID, sema::Expr>>();
		instantiation_args.reserve(instr.arguments.size());
		for(size_t i = 0; const evo::Variant<SymbolProc::TermInfoID, SymbolProc::TypeID>& arg : instr.arguments){
			EVO_DEFER([&](){ i += 1; });

			if(arg.is<SymbolProc::TermInfoID>()){
				TermInfo& arg_term_info = this->get_term_info(arg.as<SymbolProc::TermInfoID>());

				if(arg_term_info.isMultiValue()){
					this->emit_error(
						Diagnostic::Code::SEMA_MULTI_RETURN_INTO_SINGLE_VALUE,
						instr.templated_expr.args[i],
						"Template argument cannot be multiple values"
					);
					return Result::ERROR;
				}

				if(arg_term_info.value_category == TermInfo::ValueCategory::TYPE){
					const TypeInfo::VoidableID arg_type_voidable_id = arg_term_info.type_id.as<TypeInfo::VoidableID>();

					if(this->context.getTypeManager().isTypeDeducer(arg_type_voidable_id)){
						instantiation_lookup_args.emplace_back(arg_type_voidable_id);
						is_deducer = true;
						continue;
					}

					if(struct_template.params[i].isExpr()){
						const ASTBuffer& ast_buffer = this->source.getASTBuffer();
						const AST::StructDef& ast_struct =
							ast_buffer.getStructDef(sema_templated_struct.symbolProc.ast_node);
						const AST::TemplatePack& ast_template_pack =
							ast_buffer.getTemplatePack(*ast_struct.templatePack);

						this->emit_error(
							Diagnostic::Code::SEMA_TEMPLATE_INVALID_ARG,
							instr.templated_expr.args[i],
							"Expected an expression template argument, got a type",
							Diagnostic::Info(
								"Parameter declared here:", this->get_location(ast_template_pack.params[i].ident)
							)
						);
						return Result::ERROR;
					}


					instantiation_lookup_args.emplace_back(arg_type_voidable_id);
					instantiation_args.emplace_back(arg_type_voidable_id);
					continue;
				}

				if(struct_template.params[i].isType()){
					const ASTBuffer& ast_buffer = this->source.getASTBuffer();
					const AST::StructDef& ast_struct =
						ast_buffer.getStructDef(sema_templated_struct.symbolProc.ast_node);
					const AST::TemplatePack& ast_template_pack = ast_buffer.getTemplatePack(*ast_struct.templatePack);

					if(arg_term_info.value_category == TermInfo::ValueCategory::TEMPLATE_TYPE){
						this->emit_error(
							Diagnostic::Code::SEMA_TEMPLATE_TYPE_NOT_INSTANTIATED,
							instr.templated_expr.args[i],
							"Templated type needs to be instantiated",
							Diagnostic::Info(
								"Type declared here:",
								this->get_location(arg_term_info.type_id.as<sema::TemplatedStruct::ID>())
							)
						);
					}else{
						this->emit_error(
							Diagnostic::Code::SEMA_TEMPLATE_INVALID_ARG,
							instr.templated_expr.args[i],
							"Expected a type template argument, got an expression",
							Diagnostic::Info(
								"Parameter declared here:", this->get_location(ast_template_pack.params[i].ident)
							)
						);
					}

					return Result::ERROR;
				}



				const evo::Result<TypeInfo::ID> expr_type_id = [&]() -> evo::Result<TypeInfo::ID> {
					if(struct_template.params[i].typeID->isTemplateDeclInstantiation()){
						const AST::StructDef& ast_struct =
							this->source.getASTBuffer().getStructDef(sema_templated_struct.symbolProc.ast_node);
						const AST::TemplatePack& ast_template_pack = 
							this->source.getASTBuffer().getTemplatePack(*ast_struct.templatePack);

						const evo::Result<TypeInfo::VoidableID> resolved_type = this->resolve_type(
							this->source.getASTBuffer().getType(ast_template_pack.params[i].type)
						);
						if(resolved_type.isError()){ return evo::resultError; }

						if(resolved_type.value().isVoid()){
							this->emit_error(
								Diagnostic::Code::SEMA_TEMPLATE_PARAM_CANNOT_BE_TYPE_VOID,
								ast_template_pack.params[i].type,
								"Template expression parameter cannot be type `Void`"
							);
							return evo::resultError;
						}

						return resolved_type.value().asTypeID();
					}else{
						return *struct_template.params[i].typeID;
					}
				}();
				if(expr_type_id.isError()){ return Result::ERROR; }
				
			
				if(this->type_check<true, true>(
					expr_type_id.value(), arg_term_info, "Template argument", instr.templated_expr.args[i]
				).ok == false){
					return Result::ERROR;
				}

				const sema::Expr& arg_expr = arg_term_info.getExpr();
				instantiation_args.emplace_back(arg_expr);
				switch(arg_expr.kind()){
					case sema::Expr::Kind::INT_VALUE: {
						instantiation_lookup_args.emplace_back(
							core::GenericValue(evo::copy(sema_buffer.getIntValue(arg_expr.intValueID()).value))
						);
					} break;

					case sema::Expr::Kind::FLOAT_VALUE: {
						instantiation_lookup_args.emplace_back(
							core::GenericValue(evo::copy(sema_buffer.getFloatValue(arg_expr.floatValueID()).value))
						);
					} break;

					case sema::Expr::Kind::BOOL_VALUE: {
						instantiation_lookup_args.emplace_back(
							core::GenericValue(evo::copy(sema_buffer.getBoolValue(arg_expr.boolValueID()).value))
						);
					} break;

					case sema::Expr::Kind::STRING_VALUE: {
						instantiation_lookup_args.emplace_back(
							core::GenericValue(evo::copy(sema_buffer.getStringValue(arg_expr.stringValueID()).value))
						);
					} break;

					case sema::Expr::Kind::AGGREGATE_VALUE: {
						evo::debugFatalBreak(
							"Aggregate value template args are not supported yet (getting here should be impossible)"
						);
					} break;

					case sema::Expr::Kind::CHAR_VALUE: {
						instantiation_lookup_args.emplace_back(
							core::GenericValue(sema_buffer.getCharValue(arg_expr.charValueID()).value)
						);
					} break;

					default: evo::debugFatalBreak("Invalid template argument value");
				}
				
			}else{
				const ASTBuffer& ast_buffer = this->source.getASTBuffer();
				const AST::StructDef& ast_struct = ast_buffer.getStructDef(sema_templated_struct.symbolProc.ast_node);
				const AST::TemplatePack& ast_template_pack = ast_buffer.getTemplatePack(*ast_struct.templatePack);

				const TypeInfo::VoidableID type_id = this->get_type(arg.as<SymbolProc::TypeID>());

				if(this->context.getTypeManager().isTypeDeducer(type_id)){
					instantiation_lookup_args.emplace_back(type_id);
					is_deducer = true;
					continue;
				}

				if(struct_template.params[i].isExpr()){
					this->emit_error(
						Diagnostic::Code::SEMA_TEMPLATE_INVALID_ARG,
						instr.templated_expr.args[i],
						"Expected an expression template argument, got a type",
						Diagnostic::Info(
							"Parameter declared here:", this->get_location(ast_template_pack.params[i].ident)
						)
					);
					return Result::ERROR;
				}
				instantiation_lookup_args.emplace_back(type_id);
				instantiation_args.emplace_back(type_id);

				this->scope.addTemplateDeclInstantiationType(
					this->source.getTokenBuffer()[ast_template_pack.params[i].ident].getString(), type_id
				);
			}
		}


		// default values
		for(size_t i = instr.arguments.size(); i < struct_template.params.size(); i+=1){
			struct_template.params[i].defaultValue.visit([&](const auto& default_value) -> void {
				using DefaultValue = std::decay_t<decltype(default_value)>;

				if constexpr(std::is_same<DefaultValue, std::monostate>()){
					evo::debugFatalBreak("Expected template default value, found none");

				}else if constexpr(std::is_same<DefaultValue, sema::Expr>()){
					switch(default_value.kind()){
						case sema::Expr::Kind::INT_VALUE: {
							instantiation_lookup_args.emplace_back(
								core::GenericValue(
									evo::copy(sema_buffer.getIntValue(default_value.intValueID()).value)
								)
							);
						} break;

						case sema::Expr::Kind::FLOAT_VALUE: {
							instantiation_lookup_args.emplace_back(
								core::GenericValue(
									evo::copy(sema_buffer.getFloatValue(default_value.floatValueID()).value)
								)
							);
						} break;

						case sema::Expr::Kind::BOOL_VALUE: {
							instantiation_lookup_args.emplace_back(
								core::GenericValue(
									evo::copy(sema_buffer.getBoolValue(default_value.boolValueID()).value)
								)
							);
						} break;

						case sema::Expr::Kind::STRING_VALUE: {
							instantiation_lookup_args.emplace_back(
								core::GenericValue(
									evo::copy(sema_buffer.getStringValue(default_value.stringValueID()).value)
								)
							);
						} break;

						case sema::Expr::Kind::AGGREGATE_VALUE: {
							evo::debugFatalBreak(
								"String value template args are not supported yet (getting here should be impossible)"
							);
						} break;

						case sema::Expr::Kind::CHAR_VALUE: {
							instantiation_lookup_args.emplace_back(
								core::GenericValue(sema_buffer.getCharValue(default_value.charValueID()).value)
							);
						} break;

						default: evo::debugFatalBreak("Invalid template argument value");
					}
					instantiation_args.emplace_back(default_value);

				}else if constexpr(std::is_same<DefaultValue, TypeInfo::VoidableID>()){
					instantiation_lookup_args.emplace_back(default_value);
					instantiation_args.emplace_back(default_value);

				}else{
					static_assert(false, "Unsupported template default value type");
				}
			});
		}



		///////////////////////////////////
		// lookup / create instantiation

		if(is_deducer){
			const BaseType::ID created_struct_template_deducer = this->context.type_manager.createStructTemplateDeducer(
				BaseType::StructTemplateDeducer(
					this->source.getID(), sema_templated_struct.templateID, std::move(instantiation_lookup_args)
				)
			);

			this->return_struct_instantiation(
				instr.instantiation, created_struct_template_deducer.structTemplateDeducerID()
			);
			return Result::SUCCESS;
		}


		const BaseType::StructTemplate::InstantiationInfo instantiation_info =
			struct_template.createOrLookupInstantiation(std::move(instantiation_lookup_args));

		if(instantiation_info.needsToBeCompiled()){
			auto symbol_proc_builder = SymbolProcBuilder(
				this->context, this->context.source_manager[sema_templated_struct.symbolProc.source_id]
			);

			sema::ScopeManager& scope_manager = this->context.sema_buffer.scope_manager;

			const sema::ScopeManager::Scope::ID instantiation_sema_scope_id = 
				scope_manager.copyScope(*sema_templated_struct.symbolProc.sema_scope_id);


			///////////////////////////////////
			// build instantiation

			const evo::Result<SymbolProc::ID> instantiation_symbol_proc_id = symbol_proc_builder.buildTemplateInstance(
				sema_templated_struct.symbolProc,
				instantiation_info.instantiation,
				instantiation_sema_scope_id,
				sema_templated_struct.templateID,
				*instantiation_info.instantiationID
			);
			if(instantiation_symbol_proc_id.isError()){ return Result::ERROR; }

			instantiation_info.instantiation.symbolProcID = instantiation_symbol_proc_id.value();


			///////////////////////////////////
			// add instantiation args to scope

			sema::ScopeManager::Scope& instantiation_sema_scope = scope_manager.getScope(instantiation_sema_scope_id);

			instantiation_sema_scope.pushLevel(scope_manager.createLevel());

			const AST::StructDef& struct_template_decl = 
				this->source.getASTBuffer().getStructDef(sema_templated_struct.symbolProc.ast_node);

			const AST::TemplatePack& ast_template_pack = this->source.getASTBuffer().getTemplatePack(
				*struct_template_decl.templatePack
			);

			for(size_t i = 0; const evo::Variant<TypeInfo::VoidableID, sema::Expr>& arg : instantiation_args){
				EVO_DEFER([&](){ i += 1; });

				const evo::Result<> add_ident_result = [&](){
					if(arg.is<TypeInfo::VoidableID>()){
						return this->add_ident_to_scope(
							instantiation_sema_scope,
							this->source.getTokenBuffer()[ast_template_pack.params[i].ident].getString(),
							ast_template_pack.params[i].ident,
							sema::ScopeLevel::TemplateTypeParamFlag{},
							arg.as<TypeInfo::VoidableID>(),
							ast_template_pack.params[i].ident
						);

					}else{
						const TypeInfo::ID expr_type_id = [&]() -> TypeInfo::ID {
							if(struct_template.params[i].typeID->isTemplateDeclInstantiation()){
								const AST::StructDef& ast_struct = this->source.getASTBuffer().getStructDef(
									sema_templated_struct.symbolProc.ast_node
								);
								const AST::TemplatePack& ast_template_pack = 
									this->source.getASTBuffer().getTemplatePack(*ast_struct.templatePack);

								const evo::Result<TypeInfo::VoidableID> resolved_type = this->resolve_type(
									this->source.getASTBuffer().getType(ast_template_pack.params[i].type)
								);

								evo::debugAssert(
									resolved_type.isError() == false, "Should have already checked not an error"
								);

								evo::debugAssert(
									resolved_type.value().isVoid() == false, "Should have already checked not Void"
								);

								return resolved_type.value().asTypeID();
							}else{
								return *struct_template.params[i].typeID;
							}
						}();

						return this->add_ident_to_scope(
							instantiation_sema_scope,
							this->source.getTokenBuffer()[ast_template_pack.params[i].ident].getString(),
							ast_template_pack.params[i].ident,
							sema::ScopeLevel::TemplateExprParamFlag{},
							expr_type_id,
							arg.as<sema::Expr>(),
							ast_template_pack.params[i].ident	
						);
					}
				}();

				if(add_ident_result.isError()){ return Result::ERROR; }
			}


			///////////////////////////////////
			// wait on instantiation
			
			SymbolProc& instantiation_symbol_proc = this->context.symbol_proc_manager.getSymbolProc(
				instantiation_symbol_proc_id.value()
			);
			SymbolProc::WaitOnResult wait_on_result = instantiation_symbol_proc.waitOnDeclIfNeeded(
				this->symbol_proc_id, this->context, instantiation_symbol_proc_id.value()
			);
			switch(wait_on_result){
				case SymbolProc::WaitOnResult::NOT_NEEDED:
					evo::debugFatalBreak("Should never be possible");

				case SymbolProc::WaitOnResult::WAITING:
					break;

				case SymbolProc::WaitOnResult::WAS_ERRORED:
					evo::debugFatalBreak("Should never be possible");

				case SymbolProc::WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND:
					evo::debugFatalBreak("Should never be possible");

				case SymbolProc::WaitOnResult::CIRCULAR_DEP_DETECTED:
					return Result::ERROR; // not sure this is possible just in case
			}

			this->return_struct_instantiation(instr.instantiation, instantiation_info.instantiation);


			instantiation_symbol_proc.setStatusInQueue();
			this->context.add_task_to_work_manager(instantiation_symbol_proc_id.value());

			return Result::NEED_TO_WAIT_BEFORE_NEXT_INSTR;

		}else{
			this->return_struct_instantiation(instr.instantiation, instantiation_info.instantiation);

			// TODO(FUTURE): better way of doing this?
			while(instantiation_info.instantiation.symbolProcID.load().has_value() == false){
				std::this_thread::yield();
			}

			SymbolProc& instantiation_symbol_proc = this->context.symbol_proc_manager.getSymbolProc(
				*instantiation_info.instantiation.symbolProcID.load()
			);

			SymbolProc::WaitOnResult wait_on_result = instantiation_symbol_proc.waitOnDeclIfNeeded(
				this->symbol_proc_id, this->context, *instantiation_info.instantiation.symbolProcID.load()
			);
			switch(wait_on_result){
				case SymbolProc::WaitOnResult::NOT_NEEDED:
					return Result::SUCCESS;
				
				case SymbolProc::WaitOnResult::WAITING:
					return Result::NEED_TO_WAIT_BEFORE_NEXT_INSTR;
				
				case SymbolProc::WaitOnResult::WAS_ERRORED:
					return Result::ERROR;
				
				case SymbolProc::WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND:
					evo::debugFatalBreak("Should never be possible");
				
				case SymbolProc::WaitOnResult::CIRCULAR_DEP_DETECTED:
					return Result::ERROR; // not sure this is possible but just in case
			}

			evo::unreachable();
		}
		
	}


	template<bool WAIT_FOR_DEF>
	auto SemanticAnalyzer::instr_templated_term_wait(const Instruction::TemplatedTermWait<WAIT_FOR_DEF>& instr)
	-> Result {
		using Instantiation =
			evo::Variant<const BaseType::StructTemplate::Instantiation*, BaseType::StructTemplateDeducer::ID>;

		const Instantiation instantiation = this->get_struct_instantiation(instr.instantiation);


		if(instantiation.is<const BaseType::StructTemplate::Instantiation*>()){
			const BaseType::StructTemplate::Instantiation& actual_instantiation = 
				*instantiation.as<const BaseType::StructTemplate::Instantiation*>();

			if(actual_instantiation.errored.load()){ return Result::ERROR; }
			evo::debugAssert(actual_instantiation.structID.has_value(), "Should already be completed");

			const TypeInfo::ID target_type_id = this->context.type_manager.getOrCreateTypeInfo(
				TypeInfo(BaseType::ID(*actual_instantiation.structID))
			);

			if constexpr(WAIT_FOR_DEF){
				const SymbolProc::ID target_symbol_proc_id =
					*this->context.symbol_proc_manager.getTypeSymbolProc(target_type_id);

				const SymbolProc::WaitOnResult wait_on_result = this->context.symbol_proc_manager
					.getSymbolProc(target_symbol_proc_id)
					.waitOnDefIfNeeded(this->symbol_proc_id, this->context, target_symbol_proc_id);

				switch(wait_on_result){
					case SymbolProc::WaitOnResult::NOT_NEEDED:                 break;
					case SymbolProc::WaitOnResult::WAITING:                    return Result::NEED_TO_WAIT;
					case SymbolProc::WaitOnResult::WAS_ERRORED:                return Result::ERROR;
					case SymbolProc::WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND: evo::debugFatalBreak("Not possible");
					case SymbolProc::WaitOnResult::CIRCULAR_DEP_DETECTED:      return Result::ERROR;
				}
			}

			this->return_term_info(instr.output,
				TermInfo::ValueCategory::TYPE, TypeInfo::VoidableID(target_type_id)
			);

			return Result::SUCCESS;

		}else{
			evo::debugAssert(
				instantiation.is<BaseType::StructTemplateDeducer::ID>(), "Unknown struct instantiation kind"
			);

			this->return_term_info(instr.output,
				TermInfo::ValueCategory::TYPE,
				TypeInfo::VoidableID(
					this->context.type_manager.getOrCreateTypeInfo(
						TypeInfo(BaseType::ID(instantiation.as<BaseType::StructTemplateDeducer::ID>()))
					)
				)
			);

			return Result::SUCCESS;
		}
	}


	auto SemanticAnalyzer::instr_push_template_decl_instantiation_types_scope() -> Result {
		this->scope.pushTemplateDeclInstantiationTypesScope();
		return Result::SUCCESS;
	}

	auto SemanticAnalyzer::instr_pop_template_decl_instantiation_types_scope() -> Result {
		this->scope.popTemplateDeclInstantiationTypesScope();
		return Result::SUCCESS;
	}

	auto SemanticAnalyzer::instr_add_template_decl_instantiation_type(
		const Instruction::AddTemplateDeclInstantiationType& instr
	) -> Result {
		this->scope.addTemplateDeclInstantiationType(instr.ident, std::nullopt);
		return Result::SUCCESS;
	}



	template<bool IS_CONSTEXPR>
	auto SemanticAnalyzer::instr_expr_as(const Instruction::As<IS_CONSTEXPR>& instr) -> Result {
		TermInfo& expr = this->get_term_info(instr.expr);
		const TypeInfo::VoidableID target_type = this->get_type(instr.target_type);

		if(target_type.isVoid()){
			this->emit_error(
				Diagnostic::Code::SEMA_AS_TO_VOID,
				instr.infix.rhs,
				"Operator [as] cannot convert to type `Void`"
			);
			return Result::ERROR;
		}

		if(expr.value_category == TermInfo::ValueCategory::EPHEMERAL_FLUID){
			if(expr.getExpr().kind() == sema::Expr::Kind::INT_VALUE){
				if(this->context.getTypeManager().isIntegral(target_type.asTypeID())){ // int to int
					if(this->type_check<true, true>(
						target_type.asTypeID(), expr, "Operator [as]", instr.infix
					).ok == false){
						return Result::ERROR;
					}

					this->return_term_info(instr.output,
						TermInfo::ValueCategory::EPHEMERAL,
						TermInfo::ValueStage::CONSTEXPR,
						TermInfo::ValueState::NOT_APPLICABLE,
						target_type.asTypeID(),
						expr.getExpr()
					);
					return Result::SUCCESS;

				}else{ // int to float
					const sema::IntValue& initial_val = 
						this->context.sema_buffer.getIntValue(expr.getExpr().intValueID());

					const sema::FloatValue::ID new_float_value = this->context.sema_buffer.createFloatValue(
						core::GenericFloat::createF128FromInt(initial_val.value, true),
						this->context.getTypeManager().getTypeInfo(target_type.asTypeID()).baseTypeID()
					);

					this->return_term_info(instr.output,
						TermInfo::ValueCategory::EPHEMERAL,
						TermInfo::ValueStage::CONSTEXPR,
						TermInfo::ValueState::NOT_APPLICABLE,
						target_type.asTypeID(),
						sema::Expr(new_float_value)
					);
					return Result::SUCCESS;

				}

			}else{
				if(this->context.getTypeManager().isIntegral(target_type.asTypeID())){ // float to int
					const unsigned width = unsigned(this->context.getTypeManager().numBits(target_type.asTypeID()));
					const bool is_signed = this->context.getTypeManager().isSignedIntegral(target_type.asTypeID());

					const sema::FloatValue& initial_val = 
						this->context.sema_buffer.getFloatValue(expr.getExpr().floatValueID());

					const sema::IntValue::ID new_int_value = this->context.sema_buffer.createIntValue(
						initial_val.value.toGenericInt(width, is_signed),
						this->context.getTypeManager().getTypeInfo(target_type.asTypeID()).baseTypeID()
					);

					this->return_term_info(instr.output,
						TermInfo::ValueCategory::EPHEMERAL,
						TermInfo::ValueStage::CONSTEXPR,
						TermInfo::ValueState::NOT_APPLICABLE,
						target_type.asTypeID(),
						sema::Expr(new_int_value)
					);

					return Result::SUCCESS;

				}else{ // float to float
					if(this->type_check<true, true>(
						target_type.asTypeID(), expr, "Operator [as]", instr.infix
					).ok == false){
						return Result::ERROR;
					}

					this->return_term_info(instr.output,
						TermInfo::ValueCategory::EPHEMERAL,
						TermInfo::ValueStage::CONSTEXPR,
						TermInfo::ValueState::NOT_APPLICABLE,
						target_type.asTypeID(),
						expr.getExpr()
					);
					return Result::SUCCESS;
				}
			}
		}


		if(expr.value_state != TermInfo::ValueState::INIT && expr.value_state != TermInfo::ValueState::NOT_APPLICABLE){
			this->emit_error(
				Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
				instr.infix.lhs,
				"Argument for operator [as] must be initialized"
			);
			return Result::ERROR;
		}


		TypeManager& type_manager = this->context.type_manager;


		const TypeInfo::ID from_underlying_type_id = type_manager.getUnderlyingType(expr.type_id.as<TypeInfo::ID>());
		const TypeInfo& from_underlying_type = type_manager.getTypeInfo(from_underlying_type_id);

		if(from_underlying_type.qualifiers().empty() == false){
			auto infos = evo::SmallVector<Diagnostic::Info>();
			this->diagnostic_print_type_info(expr.type_id.as<TypeInfo::ID>(), infos, "Expression type: ");
			this->diagnostic_print_type_info(target_type.asTypeID(), infos,          "Target type:     ");
			this->emit_error(
				Diagnostic::Code::SEMA_AS_INVALID_FROM,
				instr.infix.lhs,
				"No valid operator [as] for this type",
				std::move(infos)
			);
			return Result::ERROR;
		}

		if(from_underlying_type.baseTypeID().kind() == BaseType::Kind::STRUCT){
			const BaseType::Struct& from_struct = this->context.getTypeManager().getStruct(
				from_underlying_type.baseTypeID().structID()
			);

			const auto find = from_struct.operatorAsOverloads.find(target_type.asTypeID());

			if(find == from_struct.operatorAsOverloads.end()){
				auto infos = evo::SmallVector<Diagnostic::Info>();
				this->diagnostic_print_type_info(expr.type_id.as<TypeInfo::ID>(), infos, "Expression type: ");
				this->diagnostic_print_type_info(target_type.asTypeID(), infos,          "Target type:     ");
				this->emit_error(
					Diagnostic::Code::SEMA_AS_INVALID_TO,
					instr.infix.rhs,
					"No valid operator [as] to this type",
					std::move(infos)
				);
				return Result::ERROR;
			}

			const sema::FuncCall::ID conversion_call = this->context.sema_buffer.createFuncCall(
				find->second, evo::SmallVector<sema::Expr>{expr.getExpr()}
			);

			this->return_term_info(instr.output,
				TermInfo::ValueCategory::EPHEMERAL,
				expr.value_stage,
				TermInfo::ValueState::NOT_APPLICABLE,
				target_type.asTypeID(),
				sema::Expr(conversion_call)
			);
			return Result::SUCCESS;
		}

		const TypeInfo::ID to_underlying_type_id = type_manager.getUnderlyingType(target_type.asTypeID());
		const TypeInfo& to_underlying_type = type_manager.getTypeInfo(to_underlying_type_id);


		const TypeInfo& from_type = type_manager.getTypeInfo(expr.type_id.as<TypeInfo::ID>());
		const TypeInfo& to_type = type_manager.getTypeInfo(target_type.asTypeID());
		if(from_type.isNormalPointer() && from_type.qualifiers().size() == 1 && to_type.isInterfacePointer()){
			return this->operator_as_interface_ptr(instr, expr, from_type, to_type);
		}


		if(to_underlying_type.qualifiers().empty() == false){
			auto infos = evo::SmallVector<Diagnostic::Info>();
			this->diagnostic_print_type_info(expr.type_id.as<TypeInfo::ID>(), infos, "Expression type: ");
			this->diagnostic_print_type_info(target_type.asTypeID(), infos,          "Target type:     ");
			this->emit_error(
				Diagnostic::Code::SEMA_AS_INVALID_TO,
				instr.infix.rhs,
				"No valid operator [as] to this type",
				std::move(infos)
			);
			return Result::ERROR;
		}

		if(to_underlying_type.baseTypeID().kind() == BaseType::Kind::ARRAY_REF){
			if(from_underlying_type.qualifiers().empty() == false || to_underlying_type.qualifiers().empty() == false){
				auto infos = evo::SmallVector<Diagnostic::Info>();
				this->diagnostic_print_type_info(expr.type_id.as<TypeInfo::ID>(), infos, "Expression type: ");
				this->diagnostic_print_type_info(target_type.asTypeID(), infos,          "Target type:     ");
				this->emit_error(
					Diagnostic::Code::SEMA_AS_INVALID_TO,
					instr.infix.rhs,
					"No valid operator [as] to this type",
					std::move(infos)
				);
				return Result::ERROR;
			}

			if(from_underlying_type.baseTypeID().kind() != BaseType::Kind::ARRAY){
				auto infos = evo::SmallVector<Diagnostic::Info>();
				this->diagnostic_print_type_info(expr.type_id.as<TypeInfo::ID>(), infos, "Expression type: ");
				this->diagnostic_print_type_info(target_type.asTypeID(), infos,          "Target type:     ");
				this->emit_error(
					Diagnostic::Code::SEMA_AS_INVALID_TO,
					instr.infix.rhs,
					"No valid operator [as] to this type",
					std::move(infos)
				);
				return Result::ERROR;
			}

			const BaseType::Array& from_array = 
				this->context.getTypeManager().getArray(from_underlying_type.baseTypeID().arrayID());

			const BaseType::ArrayRef& to_array_ref = 
				this->context.getTypeManager().getArrayRef(to_underlying_type.baseTypeID().arrayRefID());

			if(from_array.elementTypeID != to_array_ref.elementTypeID){
				auto infos = evo::SmallVector<Diagnostic::Info>();
				this->diagnostic_print_type_info(expr.type_id.as<TypeInfo::ID>(), infos, "Expression type: ");
				this->diagnostic_print_type_info(target_type.asTypeID(), infos,          "Target type:     ");
				this->emit_error(
					Diagnostic::Code::SEMA_AS_INVALID_TO,
					instr.infix.rhs,
					"No valid operator [as] to this type",
					std::move(infos)
				);
				return Result::ERROR;
			}


			if(from_array.dimensions.size() != to_array_ref.dimensions.size()){
				auto infos = evo::SmallVector<Diagnostic::Info>();
				this->diagnostic_print_type_info(expr.type_id.as<TypeInfo::ID>(), infos, "Expression type: ");
				this->diagnostic_print_type_info(target_type.asTypeID(), infos,          "Target type:     ");
				this->emit_error(
					Diagnostic::Code::SEMA_AS_INVALID_TO,
					instr.infix.rhs,
					"No valid operator [as] to this type",
					std::move(infos)
				);
				return Result::ERROR;
			}

			if(to_array_ref.isReadOnly == false && expr.is_const()){
				auto infos = evo::SmallVector<Diagnostic::Info>();
				this->diagnostic_print_type_info(expr.type_id.as<TypeInfo::ID>(), infos, "Expression type: ");
				this->diagnostic_print_type_info(target_type.asTypeID(), infos,          "Target type:     ");
				infos.emplace_back("Did you mean to make the target array reference read only?");
				this->emit_error(
					Diagnostic::Code::SEMA_AS_INVALID_TO,
					instr.infix.rhs,
					"No valid operator [as] to this type",
					std::move(infos)
				);
				return Result::ERROR;
			}


			auto dimensions = evo::SmallVector<evo::Variant<uint64_t, sema::Expr>>();
			dimensions.reserve(from_array.dimensions.size());
			for(uint64_t dimension : from_array.dimensions){
				dimensions.emplace_back(dimension);
			}

			const sema::Expr created_array_to_array_ref = sema::Expr(
				this->context.sema_buffer.createInitArrayRef(
					sema::Expr(this->context.sema_buffer.createAddrOf(expr.getExpr())), std::move(dimensions)
				)
			);

			this->return_term_info(instr.output,
				TermInfo::ValueCategory::EPHEMERAL,
				expr.value_stage,
				TermInfo::ValueState::NOT_APPLICABLE,
				target_type.asTypeID(),
				created_array_to_array_ref
			);
			return Result::SUCCESS;

		}else if(
			from_underlying_type.baseTypeID().kind() != BaseType::Kind::PRIMITIVE
			|| to_underlying_type.baseTypeID().kind() != BaseType::Kind::PRIMITIVE
		){
			auto infos = evo::SmallVector<Diagnostic::Info>();
			this->diagnostic_print_type_info(expr.type_id.as<TypeInfo::ID>(), infos, "Expression type: ");
			this->diagnostic_print_type_info(target_type.asTypeID(), infos,          "Target type:     ");
			this->emit_error(
				Diagnostic::Code::SEMA_AS_INVALID_TO,
				instr.infix.rhs,
				"No valid operator [as] to this type",
				std::move(infos)
			);
			return Result::ERROR;

		}



		const BaseType::Primitive& from_primitive =
			type_manager.getPrimitive(from_underlying_type.baseTypeID().primitiveID());

		const BaseType::Primitive& to_primitive =
			type_manager.getPrimitive(to_underlying_type.baseTypeID().primitiveID());

		if(from_primitive.kind() == Token::Kind::TYPE_RAWPTR){
			if(to_primitive.kind() != Token::Kind::TYPE_RAWPTR){
				auto infos = evo::SmallVector<Diagnostic::Info>();
				this->diagnostic_print_type_info(expr.type_id.as<TypeInfo::ID>(), infos, "Expression type: ");
				this->diagnostic_print_type_info(target_type.asTypeID(), infos,          "Target type:     ");
				this->emit_error(
					Diagnostic::Code::SEMA_AS_INVALID_TO,
					instr.infix,
					"Operator [as] cannot convert a pointer to this type",
					std::move(infos)
				);
				return Result::ERROR;
			}

			if(from_type.isPointer() && to_type.isPointer()){
				this->emit_error(
					Diagnostic::Code::SEMA_AS_INVALID_TO,
					instr.infix,
					"Operator [as] cannot convert from a pointer to a pointer"
				);
				return Result::ERROR;
			}


			this->return_term_info(instr.output,
				TermInfo::ValueCategory::EPHEMERAL,
				expr.value_stage,
				TermInfo::ValueState::NOT_APPLICABLE,
				target_type.asTypeID(),
				expr.getExpr()
			);
			return Result::SUCCESS;
			
		}else if(to_primitive.kind() == Token::Kind::TYPE_RAWPTR){
			auto infos = evo::SmallVector<Diagnostic::Info>();
			this->diagnostic_print_type_info(expr.type_id.as<TypeInfo::ID>(), infos, "Expression type: ");
			this->diagnostic_print_type_info(target_type.asTypeID(), infos,          "Target type:     ");
			this->emit_error(
				Diagnostic::Code::SEMA_AS_INVALID_TO,
				instr.infix.rhs,
				"No valid operator [as] to this type",
				std::move(infos)
			);
			return Result::ERROR;
		}


		// if converting to same type, no conversion needed
		if(from_underlying_type_id == to_underlying_type_id){
			this->return_term_info(instr.output,
				TermInfo::ValueCategory::EPHEMERAL,
				expr.value_stage,
				TermInfo::ValueState::NOT_APPLICABLE,
				target_type.asTypeID(),
				expr.getExpr()
			);
			return Result::SUCCESS;
		}


		if(from_underlying_type_id == TypeManager::getTypeBool()){
			if constexpr(IS_CONSTEXPR){
				switch(to_primitive.kind()){
					case Token::Kind::TYPE_I_N: case Token::Kind::TYPE_UI_N: {
						this->return_term_info(instr.output,
							TermInfo::ValueCategory::EPHEMERAL,
							TermInfo::ValueStage::CONSTEXPR,
							TermInfo::ValueState::NOT_APPLICABLE,
							target_type.asTypeID(),
							sema::Expr(this->context.sema_buffer.createIntValue(
								core::GenericInt(
									unsigned(to_primitive.bitWidth()),
									uint64_t(
										this->context.getSemaBuffer().getBoolValue(expr.getExpr().boolValueID()).value
									)
								),
								to_type.baseTypeID()
							))
						);
					} break;

					case Token::Kind::TYPE_F16: {
						this->return_term_info(instr.output,
							TermInfo::ValueCategory::EPHEMERAL,
							TermInfo::ValueStage::CONSTEXPR,
							TermInfo::ValueState::NOT_APPLICABLE,
							target_type.asTypeID(),
							sema::Expr(this->context.sema_buffer.createFloatValue(
								core::GenericFloat::createF32(
									float(
										this->context.getSemaBuffer().getBoolValue(expr.getExpr().boolValueID()).value
									)
								).asF16(),
								to_type.baseTypeID()
							))
						);
					} break;

					case Token::Kind::TYPE_BF16: {
						this->return_term_info(instr.output,
							TermInfo::ValueCategory::EPHEMERAL,
							TermInfo::ValueStage::CONSTEXPR,
							TermInfo::ValueState::NOT_APPLICABLE,
							target_type.asTypeID(),
							sema::Expr(this->context.sema_buffer.createFloatValue(
								core::GenericFloat::createF32(
									float(
										this->context.getSemaBuffer().getBoolValue(expr.getExpr().boolValueID()).value
									)
								).asBF16(),
								to_type.baseTypeID()
							))
						);
					} break;

					case Token::Kind::TYPE_F32: {
						this->return_term_info(instr.output,
							TermInfo::ValueCategory::EPHEMERAL,
							TermInfo::ValueStage::CONSTEXPR,
							TermInfo::ValueState::NOT_APPLICABLE,
							target_type.asTypeID(),
							sema::Expr(this->context.sema_buffer.createFloatValue(
								core::GenericFloat::createF32(
									float(
										this->context.getSemaBuffer().getBoolValue(expr.getExpr().boolValueID()).value
									)
								),
								to_type.baseTypeID()
							))
						);
					} break;

					case Token::Kind::TYPE_F64: {
						this->return_term_info(instr.output,
							TermInfo::ValueCategory::EPHEMERAL,
							TermInfo::ValueStage::CONSTEXPR,
							TermInfo::ValueState::NOT_APPLICABLE,
							target_type.asTypeID(),
							sema::Expr(this->context.sema_buffer.createFloatValue(
								core::GenericFloat::createF64(
									double(
										this->context.getSemaBuffer().getBoolValue(expr.getExpr().boolValueID()).value
									)
								),
								to_type.baseTypeID()
							))
						);
					} break;

					case Token::Kind::TYPE_F80: {
						this->return_term_info(instr.output,
							TermInfo::ValueCategory::EPHEMERAL,
							TermInfo::ValueStage::CONSTEXPR,
							TermInfo::ValueState::NOT_APPLICABLE,
							target_type.asTypeID(),
							sema::Expr(this->context.sema_buffer.createFloatValue(
								core::GenericFloat::createF64(
									double(
										this->context.getSemaBuffer().getBoolValue(expr.getExpr().boolValueID()).value
									)
								).asF80(),
								to_type.baseTypeID()
							))
						);
					} break;

					case Token::Kind::TYPE_F128: {
						this->return_term_info(instr.output,
							TermInfo::ValueCategory::EPHEMERAL,
							TermInfo::ValueStage::CONSTEXPR,
							TermInfo::ValueState::NOT_APPLICABLE,
							target_type.asTypeID(),
							sema::Expr(this->context.sema_buffer.createFloatValue(
								core::GenericFloat::createF64(
									double(
										this->context.getSemaBuffer().getBoolValue(expr.getExpr().boolValueID()).value
									)
								).asF128(),
								to_type.baseTypeID()
							))
						);
					} break;

					default: evo::debugFatalBreak("Invalid primitive type");
				}

				return Result::SUCCESS;

			}else{
				switch(to_primitive.kind()){
					case Token::Kind::TYPE_I_N: case Token::Kind::TYPE_UI_N: {
						using InstantiationID = sema::TemplateIntrinsicFuncInstantiation::ID;
						const InstantiationID instantiation_id =
							this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
								TemplateIntrinsicFunc::Kind::ZEXT,
								evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{
									from_underlying_type_id, to_underlying_type_id
								}
							);

						const sema::FuncCall::ID created_func_call_id = this->context.sema_buffer.createFuncCall(
							instantiation_id, evo::SmallVector<sema::Expr>{expr.getExpr()}
						);

						this->return_term_info(instr.output,
							TermInfo::ValueCategory::EPHEMERAL,
							expr.value_stage,
							TermInfo::ValueState::NOT_APPLICABLE,
							target_type.asTypeID(),
							sema::Expr(created_func_call_id)
						);
						return Result::SUCCESS;
					} break;

					case Token::Kind::TYPE_F16: case Token::Kind::TYPE_BF16: case Token::Kind::TYPE_F32:
					case Token::Kind::TYPE_F64: case Token::Kind::TYPE_F80:  case Token::Kind::TYPE_F128: {
						using InstantiationID = sema::TemplateIntrinsicFuncInstantiation::ID;

						const TypeInfo::ID type_id_UI1 = this->context.type_manager.getOrCreateTypeInfo(
							TypeInfo(this->context.type_manager.getOrCreatePrimitiveBaseType(Token::Kind::TYPE_UI_N, 1))
						);


						const InstantiationID bitcast_instantiation_id =
							this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
								TemplateIntrinsicFunc::Kind::BIT_CAST,
								evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{
									from_underlying_type_id, TypeInfo::VoidableID(type_id_UI1)
								}
							);

						const sema::FuncCall::ID bitcast_call = this->context.sema_buffer.createFuncCall(
							bitcast_instantiation_id, evo::SmallVector<sema::Expr>{expr.getExpr()}
						);


						const InstantiationID conv_instantiation_id =
							this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
								TemplateIntrinsicFunc::Kind::I_TO_F,
								evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{
									TypeInfo::VoidableID(type_id_UI1), to_underlying_type_id
								}
							);

						const sema::FuncCall::ID conversion_call = this->context.sema_buffer.createFuncCall(
							conv_instantiation_id, evo::SmallVector<sema::Expr>{sema::Expr(bitcast_call)}
						);

						this->return_term_info(instr.output,
							TermInfo::ValueCategory::EPHEMERAL,
							expr.value_stage,
							TermInfo::ValueState::NOT_APPLICABLE,
							target_type.asTypeID(),
							sema::Expr(conversion_call)
						);
						return Result::SUCCESS;
					} break;

					default: evo::debugFatalBreak("Invalid primitive type");
				}
			}

		}else if(to_underlying_type_id == TypeManager::getTypeBool()){
			if constexpr(IS_CONSTEXPR){
				auto constexpr_intrinsic_evaluator = ConstexprIntrinsicEvaluator(
					this->context.type_manager, this->context.sema_buffer
				);

				switch(from_primitive.kind()){
					case Token::Kind::TYPE_I_N: case Token::Kind::TYPE_UI_N: {
						this->return_term_info(
							instr.output,
							constexpr_intrinsic_evaluator.neq(
								from_underlying_type_id,
								this->context.getSemaBuffer().getIntValue(expr.getExpr().intValueID()).value,
								core::GenericInt(from_primitive.bitWidth(), 0)
							)
						);
					} break;

					case Token::Kind::TYPE_F16: {
						this->return_term_info(
							instr.output,
							constexpr_intrinsic_evaluator.neq(
								from_underlying_type_id,
								this->context.getSemaBuffer().getFloatValue(expr.getExpr().floatValueID()).value,
								core::GenericFloat::createF16(0)
							)
						);
					} break;

					case Token::Kind::TYPE_BF16: {
						this->return_term_info(
							instr.output,
							constexpr_intrinsic_evaluator.neq(
								from_underlying_type_id,
								this->context.getSemaBuffer().getFloatValue(expr.getExpr().floatValueID()).value,
								core::GenericFloat::createBF16(0)
							)
						);
					} break;

					case Token::Kind::TYPE_F32: {
						this->return_term_info(
							instr.output,
							constexpr_intrinsic_evaluator.neq(
								from_underlying_type_id,
								this->context.getSemaBuffer().getFloatValue(expr.getExpr().floatValueID()).value,
								core::GenericFloat::createF32(0)
							)
						);
					} break;

					case Token::Kind::TYPE_F64: {
						this->return_term_info(
							instr.output,
							constexpr_intrinsic_evaluator.neq(
								from_underlying_type_id,
								this->context.getSemaBuffer().getFloatValue(expr.getExpr().floatValueID()).value,
								core::GenericFloat::createF64(0)
							)
						);
					} break;

					case Token::Kind::TYPE_F80: {
						this->return_term_info(
							instr.output,
							constexpr_intrinsic_evaluator.neq(
								from_underlying_type_id,
								this->context.getSemaBuffer().getFloatValue(expr.getExpr().floatValueID()).value,
								core::GenericFloat::createF80(0)
							)
						);
					} break;

					case Token::Kind::TYPE_F128: {
						this->return_term_info(
							instr.output,
							constexpr_intrinsic_evaluator.neq(
								from_underlying_type_id,
								this->context.getSemaBuffer().getFloatValue(expr.getExpr().floatValueID()).value,
								core::GenericFloat::createF128(0)
							)
						);
					} break;

					default: evo::debugFatalBreak("Unknown or unsupported underlying type");
				}

			}else{
				const sema::Expr zero = [&](){
					switch(from_primitive.kind()){
						case Token::Kind::TYPE_I_N: case Token::Kind::TYPE_UI_N: {
							return sema::Expr(this->context.sema_buffer.createIntValue(
								core::GenericInt(from_primitive.bitWidth(), 0), from_underlying_type.baseTypeID()
							));
						} break;

						case Token::Kind::TYPE_F16: {
							return sema::Expr(this->context.sema_buffer.createFloatValue(
								core::GenericFloat::createF16(0), from_underlying_type.baseTypeID()
							));
						} break;

						case Token::Kind::TYPE_BF16: {
							return sema::Expr(this->context.sema_buffer.createFloatValue(
								core::GenericFloat::createBF16(0), from_underlying_type.baseTypeID()
							));
						} break;

						case Token::Kind::TYPE_F32: {
							return sema::Expr(this->context.sema_buffer.createFloatValue(
								core::GenericFloat::createF32(0), from_underlying_type.baseTypeID()
							));
						} break;

						case Token::Kind::TYPE_F64: {
							return sema::Expr(this->context.sema_buffer.createFloatValue(
								core::GenericFloat::createF64(0), from_underlying_type.baseTypeID()
							));
						} break;

						case Token::Kind::TYPE_F80: {
							return sema::Expr(this->context.sema_buffer.createFloatValue(
								core::GenericFloat::createF80(0), from_underlying_type.baseTypeID()
							));
						} break;

						case Token::Kind::TYPE_F128: {
							return sema::Expr(this->context.sema_buffer.createFloatValue(
								core::GenericFloat::createF128(0), from_underlying_type.baseTypeID()
							));
						} break;

						default: evo::debugFatalBreak("Unknown or unsupported underlying type");
					}
				}();


				const sema::TemplateIntrinsicFuncInstantiation::ID instantiation_id = 
					this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
						TemplateIntrinsicFunc::Kind::NEQ,
						evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{
							from_underlying_type_id, to_underlying_type_id
						}
					);

				const sema::FuncCall::ID created_func_call_id = this->context.sema_buffer.createFuncCall(
					instantiation_id, evo::SmallVector<sema::Expr>{expr.getExpr(), zero}
				);

				this->return_term_info(instr.output,
					TermInfo::ValueCategory::EPHEMERAL,
					expr.value_stage,
					TermInfo::ValueState::NOT_APPLICABLE,
					target_type.asTypeID(),
					sema::Expr(created_func_call_id)
				);
			}

			return Result::SUCCESS;
		}


		struct TypeConversionData{
			enum class Kind{
				INTEGER,
				UNSIGNED_INTEGER,
				FLOAT,
			} kind;
			unsigned width;
		};

		auto get_type_conversion_data = [this](const BaseType::Primitive& primitive_type) -> TypeConversionData {
			switch(primitive_type.kind()){
				case Token::Kind::TYPE_I_N:
					return TypeConversionData(TypeConversionData::Kind::INTEGER, primitive_type.bitWidth());
				case Token::Kind::TYPE_UI_N:
					return TypeConversionData(TypeConversionData::Kind::UNSIGNED_INTEGER, primitive_type.bitWidth());
				case Token::Kind::TYPE_F16:  return TypeConversionData(TypeConversionData::Kind::FLOAT, 16);
				case Token::Kind::TYPE_BF16: return TypeConversionData(TypeConversionData::Kind::FLOAT, 16);
				case Token::Kind::TYPE_F32:  return TypeConversionData(TypeConversionData::Kind::FLOAT, 32);
				case Token::Kind::TYPE_F64:  return TypeConversionData(TypeConversionData::Kind::FLOAT, 64);
				case Token::Kind::TYPE_F80:  return TypeConversionData(TypeConversionData::Kind::FLOAT, 80);
				case Token::Kind::TYPE_F128: return TypeConversionData(TypeConversionData::Kind::FLOAT, 128);

				default: evo::debugFatalBreak("Unknown or unsupported underlying type");
			}
		};

		const TypeConversionData from_data = get_type_conversion_data(from_primitive);
		const TypeConversionData to_data = get_type_conversion_data(to_primitive);


		if constexpr(IS_CONSTEXPR){
			auto constexpr_intrinsic_evaluator = ConstexprIntrinsicEvaluator(
				this->context.type_manager, this->context.sema_buffer
			);

			switch(from_data.kind){
				case TypeConversionData::Kind::INTEGER: {
					switch(to_data.kind){
						case TypeConversionData::Kind::INTEGER: case TypeConversionData::Kind::UNSIGNED_INTEGER: {
							if(from_data.width < to_data.width){
								this->return_term_info(instr.output,
									constexpr_intrinsic_evaluator.sext(
										target_type.asTypeID(),
										this->context.sema_buffer.getIntValue(expr.getExpr().intValueID()).value
									)
								);
							}else{
								this->return_term_info(instr.output,
									constexpr_intrinsic_evaluator.trunc(
										target_type.asTypeID(),
										this->context.sema_buffer.getIntValue(expr.getExpr().intValueID()).value
									)
								);
							}
						} break;

						case TypeConversionData::Kind::FLOAT: {
							this->return_term_info(instr.output,
								constexpr_intrinsic_evaluator.iToF(
									target_type.asTypeID(),
									this->context.sema_buffer.getIntValue(expr.getExpr().intValueID()).value
								)
							);
						} break;
					}
				} break;

				case TypeConversionData::Kind::UNSIGNED_INTEGER: {
					switch(to_data.kind){
						case TypeConversionData::Kind::INTEGER: case TypeConversionData::Kind::UNSIGNED_INTEGER: {
							if(from_data.width < to_data.width){
								this->return_term_info(instr.output,
									constexpr_intrinsic_evaluator.iToF(
										target_type.asTypeID(),
										this->context.sema_buffer.getIntValue(expr.getExpr().intValueID()).value
									)
								);
							}else{
								this->return_term_info(instr.output,
									constexpr_intrinsic_evaluator.trunc(
										target_type.asTypeID(),
										this->context.sema_buffer.getIntValue(expr.getExpr().intValueID()).value
									)
								);
							}
						} break;

						case TypeConversionData::Kind::FLOAT: {
							this->return_term_info(instr.output,
								constexpr_intrinsic_evaluator.iToF(
									target_type.asTypeID(),
									this->context.sema_buffer.getIntValue(expr.getExpr().intValueID()).value
								)
							);
						} break;
					}
				} break;

				case TypeConversionData::Kind::FLOAT: {
					switch(to_data.kind){
						case TypeConversionData::Kind::INTEGER: case TypeConversionData::Kind::UNSIGNED_INTEGER: {
							this->return_term_info(instr.output,
								constexpr_intrinsic_evaluator.fToI(
									target_type.asTypeID(),
									this->context.sema_buffer.getFloatValue(expr.getExpr().floatValueID()).value
								)
							);
						} break;

						case TypeConversionData::Kind::FLOAT: {
							if(from_data.width < to_data.width){
								this->return_term_info(instr.output,
									constexpr_intrinsic_evaluator.fext(
										target_type.asTypeID(),
										this->context.sema_buffer.getFloatValue(expr.getExpr().floatValueID()).value
									)
								);
							}else{
								this->return_term_info(instr.output,
									constexpr_intrinsic_evaluator.ftrunc(
										target_type.asTypeID(),
										this->context.sema_buffer.getFloatValue(expr.getExpr().floatValueID()).value
									)
								);
							}
						} break;
					}
				} break;
			}

			return Result::SUCCESS;

		}else{
			const TemplateIntrinsicFunc::Kind intrinsic_kind = [&](){
				switch(from_data.kind){
					case TypeConversionData::Kind::INTEGER: {
						switch(to_data.kind){
							case TypeConversionData::Kind::INTEGER: case TypeConversionData::Kind::UNSIGNED_INTEGER: {
								if(from_data.width < to_data.width){
									return TemplateIntrinsicFunc::Kind::SEXT;
								}else{
									return TemplateIntrinsicFunc::Kind::TRUNC;
								}
							} break;

							case TypeConversionData::Kind::FLOAT: {
								return TemplateIntrinsicFunc::Kind::I_TO_F;
							} break;
						}
					} break;

					case TypeConversionData::Kind::UNSIGNED_INTEGER: {
						switch(to_data.kind){
							case TypeConversionData::Kind::INTEGER: case TypeConversionData::Kind::UNSIGNED_INTEGER: {
								if(from_data.width < to_data.width){
									return TemplateIntrinsicFunc::Kind::ZEXT;
								}else{
									return TemplateIntrinsicFunc::Kind::TRUNC;
								}
							} break;

							case TypeConversionData::Kind::FLOAT: {
								return TemplateIntrinsicFunc::Kind::I_TO_F;
							} break;
						}
					} break;

					case TypeConversionData::Kind::FLOAT: {
						switch(to_data.kind){
							case TypeConversionData::Kind::INTEGER: case TypeConversionData::Kind::UNSIGNED_INTEGER: {
								return TemplateIntrinsicFunc::Kind::F_TO_I;
							} break;

							case TypeConversionData::Kind::FLOAT: {
								if(from_data.width < to_data.width){
									return TemplateIntrinsicFunc::Kind::FEXT;
								}else{
									return TemplateIntrinsicFunc::Kind::FTRUNC;
								}
							} break;
						}
					} break;
				}

				evo::debugFatalBreak("Unknown or unsupported TypeConversionData::Kind");
			}();

			using InstantiationID = sema::TemplateIntrinsicFuncInstantiation::ID;
			const InstantiationID instantiation_id = this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
				intrinsic_kind,
				evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{
					from_underlying_type_id, to_underlying_type_id
				}
			);

			const sema::FuncCall::ID created_func_call_id = this->context.sema_buffer.createFuncCall(
				instantiation_id, evo::SmallVector<sema::Expr>{expr.getExpr()}
			);

			this->return_term_info(instr.output,
				TermInfo::ValueCategory::EPHEMERAL,
				expr.value_stage,
				TermInfo::ValueState::NOT_APPLICABLE,
				target_type.asTypeID(),
				sema::Expr(created_func_call_id)
			);
			return Result::SUCCESS;
		}
	}

	template<bool IS_CONSTEXPR>
	auto SemanticAnalyzer::operator_as_interface_ptr(
		const Instruction::As<IS_CONSTEXPR>& instr,
		const TermInfo& from_expr,
		const TypeInfo& from_type_info,
		const TypeInfo& to_type_info
	) -> Result {
		const BaseType::Interface& target_interface =
			this->context.getTypeManager().getInterface(to_type_info.baseTypeID().interfaceID());


		const auto impl_exists = [&]() -> bool {
			const auto lock = std::scoped_lock(target_interface.implsLock);
			return target_interface.impls.contains(from_type_info.baseTypeID());
		};


		if(impl_exists()){
			const sema::MakeInterfacePtr::ID make_interface_ptr_id = this->context.sema_buffer.createMakeInterfacePtr(
				from_expr.getExpr(), to_type_info.baseTypeID().interfaceID(), from_type_info.baseTypeID()
			);

			this->return_term_info(instr.output,
				TermInfo::ValueCategory::EPHEMERAL,
				from_expr.value_stage,
				TermInfo::ValueState::NOT_APPLICABLE,
				this->get_type(instr.target_type).asTypeID(),
				sema::Expr(make_interface_ptr_id)
			);
			return Result::SUCCESS;
		}


		if(from_type_info.qualifiers().empty() || from_type_info.baseTypeID().kind() != BaseType::Kind::STRUCT){
			this->emit_error(
				Diagnostic::Code::SEMA_INTERFACE_NO_IMPL_FOR_TYPE,
				instr.infix,
				"The type of this expr has no impl of this interface"
			);
			return Result::ERROR;
		}


		const BaseType::Struct& from_struct =
			this->context.getTypeManager().getStruct(from_type_info.baseTypeID().structID());


		const WaitOnSymbolProcResult wait_on_symbol_proc_result = this->wait_on_symbol_proc<false>(
			from_struct.namespacedMembers, "impl"
		);

		switch(wait_on_symbol_proc_result){
			case WaitOnSymbolProcResult::NOT_FOUND: case WaitOnSymbolProcResult::ERROR_PASSED_BY_WHEN_COND: {
				this->emit_error(
					Diagnostic::Code::SEMA_INTERFACE_NO_IMPL_FOR_TYPE,
					instr.infix,
					"The type of this expr has no impl of this interface"
				);
				return Result::ERROR;
			} break;

			case WaitOnSymbolProcResult::CIRCULAR_DEP_DETECTED: case WaitOnSymbolProcResult::EXISTS_BUT_ERRORED: {
				return Result::ERROR;
			} break;

			case WaitOnSymbolProcResult::NEED_TO_WAIT: {
				return Result::NEED_TO_WAIT;
			} break;

			case WaitOnSymbolProcResult::SEMAS_READY: {
				// do nothing...
			} break;
		}


		if(impl_exists()){
			const sema::MakeInterfacePtr::ID make_interface_ptr_id = this->context.sema_buffer.createMakeInterfacePtr(
				from_expr.getExpr(), to_type_info.baseTypeID().interfaceID(), from_type_info.baseTypeID()
			);

			this->return_term_info(instr.output,
				TermInfo::ValueCategory::EPHEMERAL,
				from_expr.value_stage,
				TermInfo::ValueState::NOT_APPLICABLE,
				this->get_type(instr.target_type).asTypeID(),
				sema::Expr(make_interface_ptr_id)
			);
			return Result::SUCCESS;
		}

		
		this->emit_error(
			Diagnostic::Code::SEMA_INTERFACE_NO_IMPL_FOR_TYPE,
			instr.infix,
			"The type of this expr has no impl of this interface"
		);
		return Result::ERROR;
	}



	auto SemanticAnalyzer::instr_optional_null_check(const Instruction::OptionalNullCheck& instr) -> Result {
		const TermInfo& lhs = this->get_term_info(instr.lhs);

		if(lhs.type_id.is<TypeInfo::ID>() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_OPTIONAL_NULL_CHECK_INVALID_LHS,
				instr.infix.lhs,
				"LHS cannot be compared to [null]",
				Diagnostic::Info("Only optional values can be compared to [null]")
			);
			return Result::ERROR;
		}

		if(
			lhs.value_state != TermInfo::ValueState::INIT
			&& lhs.value_state != TermInfo::ValueState::NOT_APPLICABLE
		){
			this->emit_error(
				Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
				instr.infix.lhs,
				"LHS of [null] comparison must be initialized"
			);
			return Result::ERROR;
		}

		const TypeInfo::ID lhs_actual_type_id = this->get_actual_type<true, true>(lhs.type_id.as<TypeInfo::ID>());
		const TypeInfo& lhs_actual_type = this->context.getTypeManager().getTypeInfo(lhs_actual_type_id);

		if(lhs_actual_type.isOptional() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_OPTIONAL_NULL_CHECK_INVALID_LHS,
				instr.infix.lhs,
				"LHS cannot be compared to [null]",
				Diagnostic::Info("Only optional values can be compared to [null]")
			);
			return Result::ERROR;
		}

		const bool is_equal = this->source.getTokenBuffer()[instr.infix.opTokenID].kind() == Token::lookupKind("==");
			
		this->return_term_info(instr.output,
			TermInfo::ValueCategory::EPHEMERAL,
			lhs.value_stage,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(this->context.sema_buffer.createOptionalNullCheck(lhs.getExpr(), lhs_actual_type_id, is_equal))
		);
		return Result::SUCCESS;
	}



	template<bool IS_CONSTEXPR, Instruction::MathInfixKind MATH_INFIX_KIND>
	auto SemanticAnalyzer::instr_expr_math_infix(const Instruction::MathInfix<IS_CONSTEXPR, MATH_INFIX_KIND>& instr)
	-> Result {
		TermInfo& lhs = this->get_term_info(instr.lhs);
		TermInfo& rhs = this->get_term_info(instr.rhs);

		if(lhs.isSingleNormalValue() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_MATH_INFIX_INVALID_LHS, instr.infix.lhs, "Invalid LHS of math infix operator"
			);
			return Result::ERROR;
		}

		if(rhs.isSingleNormalValue() == false){
			if constexpr(MATH_INFIX_KIND == Instruction::MathInfixKind::COMPARATIVE){
				const Token::Kind op_kind = this->source.getTokenBuffer()[instr.infix.opTokenID].kind();

				if(op_kind == Token::lookupKind("==") || op_kind == Token::lookupKind("!=")){
					if(rhs.value_category != TermInfo::ValueCategory::TAGGED_UNION_FIELD_ACCESSOR){
						this->emit_error(
							Diagnostic::Code::SEMA_MATH_INFIX_INVALID_RHS,
							instr.infix.rhs,
							"Invalid RHS of math infix operator"
						);
						return Result::ERROR;
					}

				}else{
					this->emit_error(
						Diagnostic::Code::SEMA_MATH_INFIX_INVALID_RHS,
						instr.infix.rhs,
						"Invalid RHS of math infix operator"
					);
					return Result::ERROR;
				}
			}else{
				this->emit_error(
					Diagnostic::Code::SEMA_MATH_INFIX_INVALID_RHS, instr.infix.rhs, "Invalid RHS of math infix operator"
				);
				return Result::ERROR;
			}
		}


		if(
			lhs.value_state != TermInfo::ValueState::INIT
			&& lhs.value_state != TermInfo::ValueState::NOT_APPLICABLE
		){
			this->emit_error(
				Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
				instr.infix.lhs,
				"LHS of math infix must be initialized"
			);
			return Result::ERROR;
		}

		if(
			rhs.value_state != TermInfo::ValueState::INIT
			&& rhs.value_state != TermInfo::ValueState::NOT_APPLICABLE
		){
			this->emit_error(
				Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
				instr.infix.rhs,
				"RHS of math infix must be initialized"
			);
			return Result::ERROR;
		}


		if(lhs.type_id.is<TypeInfo::ID>()){
			if(rhs.type_id.is<TypeInfo::ID>()){ // neither lhs nor rhs fluid
				const TypeInfo::ID lhs_actual_type_id =
					this->get_actual_type<false, false>(lhs.type_id.as<TypeInfo::ID>());

				const TypeInfo::ID rhs_actual_type_id =
					this->get_actual_type<false, false>(rhs.type_id.as<TypeInfo::ID>());


				const TypeInfo& lhs_actual_type = this->context.getTypeManager().getTypeInfo(lhs_actual_type_id);
				const TypeInfo& rhs_actual_type = this->context.getTypeManager().getTypeInfo(rhs_actual_type_id);


				if(
					lhs_actual_type.qualifiers().empty()
					&& lhs_actual_type.baseTypeID().kind() == BaseType::Kind::STRUCT
				){
					const BaseType::Struct& lhs_struct =
						this->context.getTypeManager().getStruct(lhs_actual_type.baseTypeID().structID());

					const evo::Expected<sema::FuncCall::ID, Result> infix_overload_result = 
						this->infix_overload_impl(lhs_struct.infixOverloads, lhs, rhs, instr.infix);

					if(infix_overload_result.has_value() == false){
						return infix_overload_result.error();
					}


					const sema::FuncCall& created_func_call =
						this->context.getSemaBuffer().getFuncCall(infix_overload_result.value());

					const sema::Func& target_func =
						this->context.getSemaBuffer().getFunc(created_func_call.target.as<sema::Func::ID>());

					const BaseType::Function& target_func_type =
						this->context.getTypeManager().getFunction(target_func.typeID);


					this->return_term_info(instr.output,
						TermInfo::ValueCategory::EPHEMERAL,
						lhs.value_stage,
						TermInfo::ValueState::NOT_APPLICABLE,
						target_func_type.returnParams[0].typeID.asTypeID(),
						sema::Expr(infix_overload_result.value())
					);
					return Result::SUCCESS;

				}else if(
					rhs_actual_type.qualifiers().empty()
					&& rhs_actual_type.baseTypeID().kind() == BaseType::Kind::STRUCT
				){
					const BaseType::Struct& rhs_struct =
						this->context.getTypeManager().getStruct(rhs_actual_type.baseTypeID().structID());

					const evo::Expected<sema::FuncCall::ID, Result> infix_overload_result = 
						this->infix_overload_impl(rhs_struct.infixOverloads, lhs, rhs, instr.infix);

					if(infix_overload_result.has_value() == false){
						return infix_overload_result.error();
					}


					const sema::FuncCall& created_func_call =
						this->context.getSemaBuffer().getFuncCall(infix_overload_result.value());

					const sema::Func& target_func =
						this->context.getSemaBuffer().getFunc(created_func_call.target.as<sema::Func::ID>());

					const BaseType::Function& target_func_type =
						this->context.getTypeManager().getFunction(target_func.typeID);


					this->return_term_info(instr.output,
						TermInfo::ValueCategory::EPHEMERAL,
						rhs.value_stage,
						TermInfo::ValueState::NOT_APPLICABLE,
						target_func_type.returnParams[0].typeID.asTypeID(),
						sema::Expr(infix_overload_result.value())
					);
					return Result::SUCCESS;
				}



				if constexpr(MATH_INFIX_KIND == Instruction::MathInfixKind::COMPARATIVE){
					const Token::Kind op_kind = this->source.getTokenBuffer()[instr.infix.opTokenID].kind();

					if(this->type_check<true, true>(
						lhs.type_id.as<TypeInfo::ID>(),
						rhs,
						std::format(
							"RHS of infix [{}] operator",
							this->source.getTokenBuffer()[instr.infix.opTokenID].kind()
						),
						instr.infix
					).ok == false){
						return Result::ERROR;
					}

					if(op_kind == Token::lookupKind("==") || op_kind == Token::lookupKind("!=")){
						if(lhs_actual_type.qualifiers().empty() == false){
							if(lhs_actual_type.qualifiers().back().isPtr){
								if(lhs_actual_type.baseTypeID().kind() == BaseType::Kind::INTERFACE){
									auto infos = evo::SmallVector<Diagnostic::Info>();
									this->diagnostic_print_type_info(
										lhs.type_id.as<TypeInfo::ID>(), infos, "LHS type: "
									);
									this->emit_error(
										Diagnostic::Code::SEMA_MATH_INFIX_NO_MATCHING_OP,
										instr.infix.lhs,
										std::format(
											"Infix [{}] of interface pointers is invalid",
											this->source.getTokenBuffer()[instr.infix.opTokenID].kind()
										),
										std::move(infos)
									);
									return Result::ERROR;
								}

							}else{
								evo::debugAssert(
									lhs_actual_type.qualifiers().back().isOptional, "Unknown type qualifiers"
								);

								if(this->type_is_comparable(lhs_actual_type) == false){
									auto infos = evo::SmallVector<Diagnostic::Info>();
									this->diagnostic_print_type_info(
										lhs.type_id.as<TypeInfo::ID>(), infos, "Argument type: "
									);
									this->emit_error(
										Diagnostic::Code::SEMA_MATH_INFIX_NO_MATCHING_OP,
										instr.infix.lhs,
										std::format(
											"Infix [{}] of this type is invalid",
											this->source.getTokenBuffer()[instr.infix.opTokenID].kind()
										),
										std::move(infos)
									);
									return Result::ERROR;
								}

								const sema::SameTypeCmp::ID same_type_cmp = this->context.sema_buffer.createSameTypeCmp(
									lhs_actual_type_id, lhs.getExpr(), rhs.getExpr(), op_kind == Token::lookupKind("==")
								);

								this->return_term_info(instr.output,
									TermInfo::ValueCategory::EPHEMERAL,
									lhs.value_stage,
									TermInfo::ValueState::NOT_APPLICABLE,
									TypeManager::getTypeBool(),
									sema::Expr(same_type_cmp)
								);
								return Result::SUCCESS;
							}

						}else{
							switch(lhs_actual_type.baseTypeID().kind()){
								case BaseType::Kind::PRIMITIVE: {
									// do nothing...
								} break;

								case BaseType::Kind::FUNCTION: {
									this->emit_error(
										Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
										instr.infix,
										std::format(
											"Infix [{}] operator of functions is unimplemented",
											this->source.getTokenBuffer()[instr.infix.opTokenID].kind()
										)
									);
									return Result::ERROR;
								} break;

								case BaseType::Kind::ARRAY: case BaseType::Kind::ARRAY_REF: case BaseType::Kind::UNION:{
									if(this->type_is_comparable(lhs_actual_type) == false){
										auto infos = evo::SmallVector<Diagnostic::Info>();
										this->diagnostic_print_type_info(
											lhs.type_id.as<TypeInfo::ID>(), infos, "Argument type: "
										);
										this->emit_error(
											Diagnostic::Code::SEMA_MATH_INFIX_NO_MATCHING_OP,
											instr.infix.lhs,
											std::format(
												"Infix [{}] of this type is invalid",
												this->source.getTokenBuffer()[instr.infix.opTokenID].kind()
											),
											std::move(infos)
										);
										return Result::ERROR;
									}

									const sema::SameTypeCmp::ID same_type_cmp = 
										this->context.sema_buffer.createSameTypeCmp(
											lhs_actual_type_id,
											lhs.getExpr(),
											rhs.getExpr(),
											op_kind == Token::lookupKind("==")
										);

									this->return_term_info(instr.output,
										TermInfo::ValueCategory::EPHEMERAL,
										lhs.value_stage,
										TermInfo::ValueState::NOT_APPLICABLE,
										TypeManager::getTypeBool(),
										sema::Expr(same_type_cmp)
									);
									return Result::SUCCESS;
								} break;
								
								case BaseType::Kind::ENUM: {
									// do nothing...
								} break;

								case BaseType::Kind::DUMMY: evo::debugFatalBreak("Invalid type");

								case BaseType::Kind::ARRAY_DEDUCER:
								case BaseType::Kind::STRUCT_TEMPLATE:
								case BaseType::Kind::STRUCT_TEMPLATE_DEDUCER:
								case BaseType::Kind::TYPE_DEDUCER:
								case BaseType::Kind::INTERFACE:
								case BaseType::Kind::INTERFACE_IMPL_INSTANTIATION: {
									evo::debugFatalBreak("Invalid type to be compared");
								} break;


								case BaseType::Kind::ALIAS: case BaseType::Kind::DISTINCT_ALIAS: {
									evo::debugFatalBreak("Should have been skipped by getting actual type");
								} break;

								case BaseType::Kind::STRUCT: {
									evo::debugFatalBreak("Should have already been checked");
								} break;
							}
						}

					}else{ // <, <=, >, >=
						if(
							lhs_actual_type.qualifiers().empty() == false
							|| lhs_actual_type.baseTypeID().kind() != BaseType::Kind::PRIMITIVE
							|| this->context.getTypeManager().getPrimitive(
									lhs_actual_type.baseTypeID().primitiveID()
								).kind() == Token::Kind::TYPE_TYPEID
						){
							auto infos = evo::SmallVector<Diagnostic::Info>();
							this->diagnostic_print_type_info(
								lhs.type_id.as<TypeInfo::ID>(), infos, "Argument type: "
							);
							this->emit_error(
								Diagnostic::Code::SEMA_MATH_INFIX_NO_MATCHING_OP,
								instr.infix.lhs,
								std::format(
									"Infix [{}] of this type is invalid :(",
									this->source.getTokenBuffer()[instr.infix.opTokenID].kind()
								),
								std::move(infos)
							);
							return Result::ERROR;
						}
					}

				}else if constexpr(MATH_INFIX_KIND == Instruction::MathInfixKind::SHIFT){
					if(this->context.getTypeManager().isIntegral(lhs_actual_type_id) == false){
						auto infos = evo::SmallVector<Diagnostic::Info>();
						this->diagnostic_print_type_info(lhs.type_id.as<TypeInfo::ID>(), infos, "LHS type: ");
						this->emit_error(
							Diagnostic::Code::SEMA_MATH_INFIX_INVALID_LHS,
							instr.infix.lhs,
							std::format(
								"LHS of [{}] operator must be integral",
								this->source.getTokenBuffer()[instr.infix.opTokenID].kind()
							),
							std::move(infos)
						);
						return Result::ERROR;
					}

					if(this->context.getTypeManager().isUnsignedIntegral(rhs_actual_type_id) == false){
						auto infos = evo::SmallVector<Diagnostic::Info>();
						this->diagnostic_print_type_info(rhs.type_id.as<TypeInfo::ID>(), infos, "RHS type: ");
						this->emit_error(
							Diagnostic::Code::SEMA_MATH_INFIX_INVALID_RHS,
							instr.infix.lhs,
							std::format(
								"RHS of [{}] operator must be unsigned integral",
								this->source.getTokenBuffer()[instr.infix.opTokenID].kind()
							),
							std::move(infos)
						);
						return Result::ERROR;
					}

					const uint64_t num_bits_lhs_type =
						this->context.getTypeManager().numBits(lhs_actual_type_id);

					const uint64_t num_bits_rhs_type =
						this->context.getTypeManager().numBits(rhs_actual_type_id);

					const uint64_t expected_num_bits_rhs_type =
						uint64_t(std::ceil(std::log2(double(num_bits_lhs_type))));

					if(num_bits_rhs_type != expected_num_bits_rhs_type){
						auto infos = evo::SmallVector<Diagnostic::Info>();
						infos.emplace_back(std::format("Correct type: UI{}", expected_num_bits_rhs_type));
						this->diagnostic_print_type_info(rhs.type_id.as<TypeInfo::ID>(), infos, "LHS type:     ");
						this->emit_error(
							Diagnostic::Code::SEMA_MATH_INFIX_INVALID_RHS,
							instr.infix.rhs,
							std::format(
								"RHS of [{}] operator is incorrect bit-width for this LHS",
								this->source.getTokenBuffer()[instr.infix.opTokenID].kind()
							),
							std::move(infos)
						);
						return Result::ERROR;
					}

				}else{
					if(this->type_check<true, true>(
						lhs.type_id.as<TypeInfo::ID>(),
						rhs,
						std::format(
							"RHS of infix [{}] operator", this->source.getTokenBuffer()[instr.infix.opTokenID].kind()
						),
						instr.infix
					).ok == false){
						return Result::ERROR;
					}

					if constexpr(MATH_INFIX_KIND == Instruction::MathInfixKind::MATH){
						if(
							this->context.getTypeManager().isIntegral(lhs_actual_type_id) == false
							&& this->context.getTypeManager().isFloatingPoint(lhs_actual_type_id) == false
						){
							auto infos = evo::SmallVector<Diagnostic::Info>();
							this->diagnostic_print_type_info(lhs.type_id.as<TypeInfo::ID>(), infos, "Argument type: ");
							this->emit_error(
								Diagnostic::Code::SEMA_MATH_INFIX_NO_MATCHING_OP,
								instr.infix,
								"No matching operation for this type",
								std::move(infos)
							);
							return Result::ERROR;
						}
					
					}else if constexpr(MATH_INFIX_KIND == Instruction::MathInfixKind::INTEGRAL_MATH){
						if(this->context.getTypeManager().isIntegral(lhs_actual_type_id) == false){
							auto infos = evo::SmallVector<Diagnostic::Info>();
							this->diagnostic_print_type_info(lhs.type_id.as<TypeInfo::ID>(), infos, "Argument type: ");
							this->emit_error(
								Diagnostic::Code::SEMA_MATH_INFIX_NO_MATCHING_OP,
								instr.infix,
								"No matching operation for this type",
								std::move(infos)
							);
							return Result::ERROR;
						}

					}else if constexpr(
						MATH_INFIX_KIND == Instruction::MathInfixKind::LOGICAL
						|| MATH_INFIX_KIND == Instruction::MathInfixKind::BITWISE_LOGICAL
					){
						if(
							lhs_actual_type.qualifiers().empty() == false
							|| lhs_actual_type.baseTypeID().kind() != BaseType::Kind::PRIMITIVE
						){
							auto infos = evo::SmallVector<Diagnostic::Info>();
							this->diagnostic_print_type_info(lhs.type_id.as<TypeInfo::ID>(), infos, "Argument type: ");
							this->emit_error(
								Diagnostic::Code::SEMA_MATH_INFIX_NO_MATCHING_OP,
								instr.infix,
								"No matching operation for this type",
								std::move(infos)
							);
							return Result::ERROR;
						}

						const BaseType::Primitive& lhs_actual_primitive =
							this->context.getTypeManager().getPrimitive(lhs_actual_type.baseTypeID().primitiveID());

						if(
							this->context.getTypeManager().isIntegral(lhs_actual_type_id) == false
							&& lhs_actual_primitive.kind() != Token::Kind::TYPE_BOOL
						){
							auto infos = evo::SmallVector<Diagnostic::Info>();
							this->diagnostic_print_type_info(lhs.type_id.as<TypeInfo::ID>(), infos, "Argument type: ");
							this->emit_error(
								Diagnostic::Code::SEMA_MATH_INFIX_NO_MATCHING_OP,
								instr.infix,
								"No matching operation for this type",
								std::move(infos)
							);
							return Result::ERROR;
						}
						
					}
				}


			}else{ // rhs fluid
				const TypeInfo::ID lhs_actual_type_id =
					this->get_actual_type<false, false>(lhs.type_id.as<TypeInfo::ID>());

				const TypeInfo& lhs_actual_type = this->context.getTypeManager().getTypeInfo(lhs_actual_type_id);

				if(lhs_actual_type.baseTypeID().kind() == BaseType::Kind::STRUCT){
					const BaseType::Struct& lhs_struct =
						this->context.getTypeManager().getStruct(lhs_actual_type.baseTypeID().structID());

					const evo::Expected<sema::FuncCall::ID, Result> infix_overload_result = 
						this->infix_overload_impl(lhs_struct.infixOverloads, lhs, rhs, instr.infix);

					if(infix_overload_result.has_value() == false){
						return infix_overload_result.error();
					}


					const sema::FuncCall& created_func_call =
						this->context.getSemaBuffer().getFuncCall(infix_overload_result.value());

					const sema::Func& target_func =
						this->context.getSemaBuffer().getFunc(created_func_call.target.as<sema::Func::ID>());

					const BaseType::Function& target_func_type =
						this->context.getTypeManager().getFunction(target_func.typeID);


					this->return_term_info(instr.output,
						TermInfo::ValueCategory::EPHEMERAL,
						lhs.value_stage,
						TermInfo::ValueState::NOT_APPLICABLE,
						target_func_type.returnParams[0].typeID.asTypeID(),
						sema::Expr(infix_overload_result.value())
					);
					return Result::SUCCESS;
				}


				if constexpr(MATH_INFIX_KIND == Instruction::MathInfixKind::SHIFT){
					if(this->context.getTypeManager().isIntegral(lhs_actual_type_id) == false){
						auto infos = evo::SmallVector<Diagnostic::Info>();
						this->diagnostic_print_type_info(lhs.type_id.as<TypeInfo::ID>(), infos, "LHS type: ");
						this->emit_error(
							Diagnostic::Code::SEMA_MATH_INFIX_INVALID_LHS,
							instr.infix.lhs,
							std::format(
								"LHS of [{}] operator must be integral",
								this->source.getTokenBuffer()[instr.infix.opTokenID].kind()
							),
							std::move(infos)
						);
						return Result::ERROR;
					}

					const uint64_t num_bits_lhs_type =
						this->context.getTypeManager().numBits(lhs_actual_type_id);

					const uint32_t expected_num_bits_rhs_type =
						uint32_t(std::ceil(std::log2(double(num_bits_lhs_type))));

					const TypeInfo::ID expected_rhs_type = this->context.type_manager.getOrCreateTypeInfo(
						TypeInfo(
							this->context.type_manager.getOrCreatePrimitiveBaseType(
								Token::Kind::TYPE_UI_N, expected_num_bits_rhs_type
							)
						)
					);

					if(this->type_check<true, true>(
						expected_rhs_type,
						rhs,
						std::format(
							"RHS of [{}] operator", this->source.getTokenBuffer()[instr.infix.opTokenID].kind()
						),
						instr.infix.rhs
					).ok == false){
						return Result::ERROR;
					}


				}else{
					if constexpr(MATH_INFIX_KIND == Instruction::MathInfixKind::COMPARATIVE){
						const Token::Kind op_kind = this->source.getTokenBuffer()[instr.infix.opTokenID].kind();

						if(op_kind == Token::lookupKind("==") || op_kind == Token::lookupKind("!=")){
							if(rhs.value_category == TermInfo::ValueCategory::TAGGED_UNION_FIELD_ACCESSOR){
								const TermInfo::TaggedUnionFieldAccessor& tagged_union_field_accessor = 
									rhs.type_id.as<TermInfo::TaggedUnionFieldAccessor>();

								if(
									lhs_actual_type.qualifiers().empty() == false
									|| lhs_actual_type.baseTypeID().kind() != BaseType::Kind::UNION
									|| lhs_actual_type.baseTypeID().unionID() != tagged_union_field_accessor.union_id
								){
									this->error_type_mismatch(
										this->context.type_manager.getOrCreateTypeInfo(
											TypeInfo(BaseType::ID(tagged_union_field_accessor.union_id))
										),
										lhs,
										"RHS of union tag comparison",
										instr.infix.lhs
									);
									return Result::ERROR;
								}
								
								if constexpr(IS_CONSTEXPR){
									this->emit_error(
										Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
										instr.infix,
										"Constexpr comparison of union tag is unimplemented"
									);
									return Result::ERROR;
								}else{
									this->return_term_info(instr.output,
										TermInfo::ValueCategory::EPHEMERAL,
										lhs.value_stage,
										TermInfo::ValueState::NOT_APPLICABLE,
										TypeManager::getTypeBool(),
										sema::Expr(
											this->context.sema_buffer.createUnionTagCmp(
												lhs.getExpr(),
												lhs_actual_type.baseTypeID().unionID(),
												tagged_union_field_accessor.field_index,
												op_kind == Token::lookupKind("==")
											)
										)
									);
									return Result::SUCCESS;
								}
							}
						}
					}



					if(this->type_check<true, true>(
						lhs_actual_type_id,
						rhs,
						std::format(
							"RHS of [{}] operator", this->source.getTokenBuffer()[instr.infix.opTokenID].kind()
						),
						instr.infix.rhs
					).ok == false){
						return Result::ERROR;
					}

					if constexpr(MATH_INFIX_KIND == Instruction::MathInfixKind::MATH){
						if(
							this->context.getTypeManager().isIntegral(lhs_actual_type_id) == false
							&& this->context.getTypeManager().isFloatingPoint(lhs_actual_type_id) == false
						){
							auto infos = evo::SmallVector<Diagnostic::Info>();
							this->diagnostic_print_type_info(lhs_actual_type_id, infos, "Argument type: ");
							this->emit_error(
								Diagnostic::Code::SEMA_MATH_INFIX_NO_MATCHING_OP,
								instr.infix,
								"No matching operation for this type",
								std::move(infos)
							);
							return Result::ERROR;
						}
					
					}else if constexpr(MATH_INFIX_KIND == Instruction::MathInfixKind::INTEGRAL_MATH){
						if(this->context.getTypeManager().isIntegral(lhs_actual_type_id) == false){
							auto infos = evo::SmallVector<Diagnostic::Info>();
							this->diagnostic_print_type_info(lhs.type_id.as<TypeInfo::ID>(), infos, "Argument type: ");
							this->emit_error(
								Diagnostic::Code::SEMA_MATH_INFIX_NO_MATCHING_OP,
								instr.infix,
								"No matching operation for this type",
								std::move(infos)
							);
							return Result::ERROR;
						}
					}
				}
			}

		}else if(rhs.type_id.is<TypeInfo::ID>()){ // lhs fluid
			const TypeInfo::ID rhs_actual_type_id = this->get_actual_type<false, false>(rhs.type_id.as<TypeInfo::ID>());

			const TypeInfo& rhs_actual_type = this->context.getTypeManager().getTypeInfo(rhs_actual_type_id);

			if(rhs_actual_type.baseTypeID().kind() == BaseType::Kind::STRUCT){
				const BaseType::Struct& rhs_struct =
					this->context.getTypeManager().getStruct(rhs_actual_type.baseTypeID().structID());

				const evo::Expected<sema::FuncCall::ID, Result> infix_overload_result = 
					this->infix_overload_impl(rhs_struct.infixOverloads, lhs, rhs, instr.infix);

				if(infix_overload_result.has_value() == false){
					return infix_overload_result.error();
				}


				const sema::FuncCall& created_func_call =
					this->context.getSemaBuffer().getFuncCall(infix_overload_result.value());

				const sema::Func& target_func =
					this->context.getSemaBuffer().getFunc(created_func_call.target.as<sema::Func::ID>());

				const BaseType::Function& target_func_type =
					this->context.getTypeManager().getFunction(target_func.typeID);


				this->return_term_info(instr.output,
					TermInfo::ValueCategory::EPHEMERAL,
					rhs.value_stage,
					TermInfo::ValueState::NOT_APPLICABLE,
					target_func_type.returnParams[0].typeID.asTypeID(),
					sema::Expr(infix_overload_result.value())
				);
				return Result::SUCCESS;
			}

			if constexpr(MATH_INFIX_KIND == Instruction::MathInfixKind::SHIFT){
				if(this->context.getTypeManager().isUnsignedIntegral(rhs_actual_type_id) == false){
					auto infos = evo::SmallVector<Diagnostic::Info>();
					this->diagnostic_print_type_info(rhs.type_id.as<TypeInfo::ID>(), infos, "RHS type: ");
					this->emit_error(
						Diagnostic::Code::SEMA_MATH_INFIX_INVALID_RHS,
						instr.infix.rhs,
						std::format(
							"RHS of [{}] operator must be unsigned integral",
							this->source.getTokenBuffer()[instr.infix.opTokenID].kind()
						),
						std::move(infos)
					);
					return Result::ERROR;
				}


				const uint64_t num_bits_rhs_type =
					this->context.getTypeManager().numBits(rhs_actual_type_id);

				const uint32_t expected_num_bits_lhs_type = uint32_t(1 << num_bits_rhs_type);

				const TypeInfo::ID expected_lhs_type = this->context.type_manager.getOrCreateTypeInfo(
					TypeInfo(
						this->context.type_manager.getOrCreatePrimitiveBaseType(
							Token::Kind::TYPE_UI_N, expected_num_bits_lhs_type
						)
					)
				);

				if(this->type_check<true, true>(
					expected_lhs_type,
					lhs,
					std::format(
						"LHS of [{}] operator",
						this->source.getTokenBuffer()[instr.infix.opTokenID].kind()
					),
					instr.infix
				).ok == false){
					return Result::ERROR;
				}


			}else{
				if(this->type_check<true, true>(
					rhs_actual_type_id,
					lhs,
					std::format(
						"LHS of [{}] operator",
						this->source.getTokenBuffer()[instr.infix.opTokenID].kind()
					),
					instr.infix
				).ok == false){
					return Result::ERROR;
				}

				if constexpr(MATH_INFIX_KIND == Instruction::MathInfixKind::MATH){
					if(
						this->context.getTypeManager().isIntegral(rhs_actual_type_id) == false
						&& this->context.getTypeManager().isFloatingPoint(rhs_actual_type_id) == false
					){
						auto infos = evo::SmallVector<Diagnostic::Info>();
						this->diagnostic_print_type_info(rhs.type_id.as<TypeInfo::ID>(), infos, "Argument type: ");
						this->emit_error(
							Diagnostic::Code::SEMA_MATH_INFIX_NO_MATCHING_OP,
							instr.infix,
							"No matching operation for this type",
							std::move(infos)
						);
						return Result::ERROR;
					}
				
				}else if constexpr(MATH_INFIX_KIND == Instruction::MathInfixKind::INTEGRAL_MATH){
					if(this->context.getTypeManager().isIntegral(rhs_actual_type_id) == false){
						auto infos = evo::SmallVector<Diagnostic::Info>();
						this->diagnostic_print_type_info(rhs.type_id.as<TypeInfo::ID>(), infos, "Argument type: ");
						this->emit_error(
							Diagnostic::Code::SEMA_MATH_INFIX_NO_MATCHING_OP,
							instr.infix,
							"No matching operation for this type",
							std::move(infos)
						);
						return Result::ERROR;
					}
				}
			}

		}else{ // both lhs and rhs fluid
			if(lhs.getExpr().kind() != rhs.getExpr().kind()){
				this->emit_error(
					Diagnostic::Code::SEMA_MATH_INFIX_NO_MATCHING_OP,
					instr.infix,
					std::format(
						"LHS and RHS of [{}] operator must match fluid kind",
						this->source.getTokenBuffer()[instr.infix.opTokenID].kind()
					)
				);
				return Result::ERROR;
			}


			this->return_term_info(instr.output,
				this->constexpr_infix_math(
					this->source.getTokenBuffer()[instr.infix.opTokenID].kind(),
					lhs.getExpr(),
					rhs.getExpr(),
					std::nullopt
				)
			);
			return Result::SUCCESS;
		}


		if constexpr(IS_CONSTEXPR){
			if constexpr(MATH_INFIX_KIND == Instruction::MathInfixKind::LOGICAL){
				const bool lhs_bool_value = this->context.sema_buffer.getBoolValue(lhs.getExpr().boolValueID()).value;
				const bool rhs_bool_value = this->context.sema_buffer.getBoolValue(rhs.getExpr().boolValueID()).value;

				const bool bool_value = [&](){
					if(this->source.getTokenBuffer()[instr.infix.opTokenID].kind() == Token::lookupKind("&&")){
						return lhs_bool_value & rhs_bool_value;

					}else{
						evo::debugAssert(
							this->source.getTokenBuffer()[instr.infix.opTokenID].kind() == Token::lookupKind("||"),
							"Unknown logical infix operator"
						);

						return lhs_bool_value | rhs_bool_value;
					}
				}();

				this->return_term_info(instr.output,
					TermInfo::ValueCategory::EPHEMERAL,
					TermInfo::ValueStage::CONSTEXPR,
					TermInfo::ValueState::NOT_APPLICABLE,
					TypeManager::getTypeBool(),
					sema::Expr(this->context.sema_buffer.createBoolValue(bool_value))
				);

			}else{
				this->return_term_info(instr.output,
					this->constexpr_infix_math(
						this->source.getTokenBuffer()[instr.infix.opTokenID].kind(),
						lhs.getExpr(),
						rhs.getExpr(),
						this->get_actual_type<true, true>(lhs.type_id.as<TypeInfo::ID>())
					)
				);
			}
			return Result::SUCCESS;

		}else{
			auto resultant_type = std::optional<TypeInfo::ID>();

			const TypeInfo::ID lhs_actual_type_id = this->get_actual_type<false, false>(lhs.type_id.as<TypeInfo::ID>());

			const Token::Kind op_kind = this->source.getTokenBuffer()[instr.infix.opTokenID].kind();

			if(op_kind == Token::lookupKind("&&")){
				this->return_term_info(instr.output,
					TermInfo::ValueCategory::EPHEMERAL,
					lhs.value_stage,
					TermInfo::ValueState::NOT_APPLICABLE,
					TypeManager::getTypeBool(),
					sema::Expr(this->context.sema_buffer.createLogicalAnd(lhs.getExpr(), rhs.getExpr()))
				);
				return Result::SUCCESS;

			}else if(op_kind == Token::lookupKind("||")){
				this->return_term_info(instr.output,
					TermInfo::ValueCategory::EPHEMERAL,
					lhs.value_stage,
					TermInfo::ValueState::NOT_APPLICABLE,
					TypeManager::getTypeBool(),
					sema::Expr(this->context.sema_buffer.createLogicalOr(lhs.getExpr(), rhs.getExpr()))
				);
				return Result::SUCCESS;
			}


			const sema::TemplateIntrinsicFuncInstantiation::ID instantiation_id = [&](){
				switch(this->source.getTokenBuffer()[instr.infix.opTokenID].kind()){
					case Token::lookupKind("=="): {
						resultant_type = TypeManager::getTypeBool();

						return this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
							TemplateIntrinsicFunc::Kind::EQ,
							evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{lhs_actual_type_id}
						);
					} break;

					case Token::lookupKind("!="): {
						resultant_type = TypeManager::getTypeBool();

						return this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
							TemplateIntrinsicFunc::Kind::NEQ,
							evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{lhs_actual_type_id}
						);
					} break;

					case Token::lookupKind("<"): {
						resultant_type = TypeManager::getTypeBool();

						return this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
							TemplateIntrinsicFunc::Kind::LT,
							evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{lhs_actual_type_id}
						);
					} break;

					case Token::lookupKind("<="): {
						resultant_type = TypeManager::getTypeBool();

						return this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
							TemplateIntrinsicFunc::Kind::LTE,
							evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{lhs_actual_type_id}
						);
					} break;

					case Token::lookupKind(">"): {
						resultant_type = TypeManager::getTypeBool();

						return this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
							TemplateIntrinsicFunc::Kind::GT,
							evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{lhs_actual_type_id}
						);
					} break;

					case Token::lookupKind(">="): {
						resultant_type = TypeManager::getTypeBool();

						return this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
							TemplateIntrinsicFunc::Kind::GTE,
							evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{lhs_actual_type_id}
						);
					} break;

					case Token::lookupKind("&"): case Token::lookupKind("&="): {
						resultant_type = lhs.type_id.as<TypeInfo::ID>();

						return this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
							TemplateIntrinsicFunc::Kind::AND,
							evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{lhs_actual_type_id}
						);
					} break;

					case Token::lookupKind("|"): case Token::lookupKind("|="): {
						resultant_type = lhs.type_id.as<TypeInfo::ID>();

						return this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
							TemplateIntrinsicFunc::Kind::OR,
							evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{lhs_actual_type_id}
						);
					} break;

					case Token::lookupKind("^"): case Token::lookupKind("^="): {
						resultant_type = lhs.type_id.as<TypeInfo::ID>();

						return this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
							TemplateIntrinsicFunc::Kind::XOR,
							evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{lhs_actual_type_id}
						);
					} break;

					case Token::lookupKind("<<"): case Token::lookupKind("<<="): {
						const TypeInfo::ID rhs_actual_type_id = 
							this->get_actual_type<false, false>(rhs.type_id.as<TypeInfo::ID>());

						resultant_type = lhs.type_id.as<TypeInfo::ID>();

						return this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
							TemplateIntrinsicFunc::Kind::SHL,
							evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{
								lhs_actual_type_id, rhs_actual_type_id, core::GenericValue(true)
							}
						);
					} break;

					case Token::lookupKind("<<|"): case Token::lookupKind("<<|="): {
						const TypeInfo::ID rhs_actual_type_id = 
							this->get_actual_type<false, false>(rhs.type_id.as<TypeInfo::ID>());

						resultant_type = lhs.type_id.as<TypeInfo::ID>();

						return this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
							TemplateIntrinsicFunc::Kind::SHL_SAT,
							evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{
								lhs_actual_type_id, rhs_actual_type_id
							}
						);
					} break;

					case Token::lookupKind(">>"): case Token::lookupKind(">>="): {
						const TypeInfo::ID rhs_actual_type_id = 
							this->get_actual_type<false, false>(rhs.type_id.as<TypeInfo::ID>());

						resultant_type = lhs.type_id.as<TypeInfo::ID>();

						return this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
							TemplateIntrinsicFunc::Kind::SHR,
							evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{
								lhs_actual_type_id, rhs_actual_type_id, core::GenericValue(true)
							}
						);
					} break;

					case Token::lookupKind("+"): case Token::lookupKind("+="): {
						resultant_type = lhs.type_id.as<TypeInfo::ID>();

						if(this->context.getTypeManager().isIntegral(lhs_actual_type_id)){
							return this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
								TemplateIntrinsicFunc::Kind::ADD,
								evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{
									lhs_actual_type_id, core::GenericValue(false)
								}
							);
						}else{
							return this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
								TemplateIntrinsicFunc::Kind::FADD,
								evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{
									lhs_actual_type_id
								}
							);
						}
					} break;

					case Token::lookupKind("+%"): case Token::lookupKind("+%="): {
						resultant_type = lhs.type_id.as<TypeInfo::ID>();

						return this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
							TemplateIntrinsicFunc::Kind::ADD,
							evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{
								lhs_actual_type_id, core::GenericValue(true)
							}
						);
					} break;

					case Token::lookupKind("+|"): case Token::lookupKind("+|="): {
						resultant_type = lhs.type_id.as<TypeInfo::ID>();

						return this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
							TemplateIntrinsicFunc::Kind::ADD_SAT,
							evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{lhs_actual_type_id}
						);
					} break;

					case Token::lookupKind("-"): case Token::lookupKind("-="): {
						resultant_type = lhs.type_id.as<TypeInfo::ID>();

						if(this->context.getTypeManager().isIntegral(lhs_actual_type_id)){
							return this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
								TemplateIntrinsicFunc::Kind::SUB,
								evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{
									lhs_actual_type_id, core::GenericValue(false)
								}
							);
						}else{
							return this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
								TemplateIntrinsicFunc::Kind::FSUB,
								evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{
									lhs_actual_type_id
								}
							);
						}
					} break;

					case Token::lookupKind("-%"): case Token::lookupKind("-%="): {
						resultant_type = lhs.type_id.as<TypeInfo::ID>();

						return this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
							TemplateIntrinsicFunc::Kind::SUB,
							evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{
								lhs_actual_type_id, core::GenericValue(true)
							}
						);
					} break;

					case Token::lookupKind("-|"): case Token::lookupKind("-|="): {
						resultant_type = lhs.type_id.as<TypeInfo::ID>();

						return this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
							TemplateIntrinsicFunc::Kind::SUB_SAT,
							evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{lhs_actual_type_id}
						);
					} break;

					case Token::lookupKind("*"): case Token::lookupKind("*="): {
						resultant_type = lhs.type_id.as<TypeInfo::ID>();

						if(this->context.getTypeManager().isIntegral(lhs_actual_type_id)){
							return this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
								TemplateIntrinsicFunc::Kind::MUL,
								evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{
									lhs_actual_type_id, core::GenericValue(false)
								}
							);
						}else{
							return this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
								TemplateIntrinsicFunc::Kind::FMUL,
								evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{
									lhs_actual_type_id
								}
							);
						}
					} break;

					case Token::lookupKind("*%"): case Token::lookupKind("*%="): {
						resultant_type = lhs.type_id.as<TypeInfo::ID>();

						return this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
							TemplateIntrinsicFunc::Kind::MUL,
							evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{
								lhs_actual_type_id, core::GenericValue(true)
							}
						);
					} break;

					case Token::lookupKind("*|"): case Token::lookupKind("*|="): {
						resultant_type = lhs.type_id.as<TypeInfo::ID>();

						return this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
							TemplateIntrinsicFunc::Kind::MUL_SAT,
							evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{lhs_actual_type_id}
						);
					} break;

					case Token::lookupKind("/"): case Token::lookupKind("/="): {
						resultant_type = lhs.type_id.as<TypeInfo::ID>();

						if(this->context.getTypeManager().isIntegral(lhs_actual_type_id)){
							return this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
								TemplateIntrinsicFunc::Kind::DIV,
								evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{
									lhs_actual_type_id, core::GenericValue(false)
								}
							);
						}else{
							return this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
								TemplateIntrinsicFunc::Kind::FDIV,
								evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{
									lhs_actual_type_id
								}
							);
						}
					} break;

					case Token::lookupKind("%"): case Token::lookupKind("%="): {
						resultant_type = lhs.type_id.as<TypeInfo::ID>();

						return this->context.sema_buffer.createTemplateIntrinsicFuncInstantiation(
							TemplateIntrinsicFunc::Kind::REM,
							evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>{lhs_actual_type_id}
						);
					} break;

					default: {
						evo::debugFatalBreak("Invalid infix math operator");
					} break;
				}
			}();

			const sema::FuncCall::ID created_func_call_id = this->context.sema_buffer.createFuncCall(
				instantiation_id, evo::SmallVector<sema::Expr>{lhs.getExpr(), rhs.getExpr()}
			);

			this->return_term_info(instr.output,
				TermInfo::ValueCategory::EPHEMERAL,
				lhs.value_stage,
				TermInfo::ValueState::NOT_APPLICABLE,
				*resultant_type,
				sema::Expr(created_func_call_id)
			);
			return Result::SUCCESS;
		}
	}


	template<bool NEEDS_DEF>
	auto SemanticAnalyzer::instr_expr_accessor(const Instruction::Accessor<NEEDS_DEF>& instr) -> Result {
		const std::string_view rhs_ident_str = this->source.getTokenBuffer()[instr.rhs_ident].getString();
		const TermInfo& lhs = this->get_term_info(instr.lhs);

		if(lhs.type_id.is<Source::ID>()){
			return this->module_accessor<NEEDS_DEF>(instr, rhs_ident_str, lhs);

		}else if(lhs.type_id.is<ClangSource::ID>()){
			return this->clang_module_accessor<NEEDS_DEF>(instr, rhs_ident_str, lhs);

		}else if(lhs.type_id.is<TypeInfo::VoidableID>()){
			return this->type_accessor<NEEDS_DEF>(instr, rhs_ident_str, lhs);

		}else if(lhs.type_id.is<BuiltinModule::ID>()){
			return this->builtin_module_accessor<NEEDS_DEF>(instr, rhs_ident_str, lhs);
		}

		if(lhs.type_id.is<TypeInfo::ID>() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_INVALID_ACCESSOR_LHS,
				instr.infix.lhs,
				"Accessor operator of this LHS is invalid"
			);
			return Result::ERROR;
		}

		if(
			lhs.value_state != TermInfo::ValueState::INIT
			&& lhs.value_state != TermInfo::ValueState::INITIALIZING
			&& lhs.value_state != TermInfo::ValueState::NOT_APPLICABLE
		){
			this->emit_error(
				Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
				instr.infix.lhs,
				"LHS of accessor operator must be initialized or initializing"
			);
			return Result::ERROR;
		}


		const TypeInfo::ID actual_lhs_type_id = this->get_actual_type<true, false>(lhs.type_id.as<TypeInfo::ID>());
		const TypeInfo& actual_lhs_type = this->context.getTypeManager().getTypeInfo(actual_lhs_type_id);

		bool is_pointer = false;

		if(actual_lhs_type.qualifiers().empty() == false){
			if(lhs.value_stage == TermInfo::ValueStage::CONSTEXPR){
				this->emit_error(
					Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
					instr.infix.lhs,
					"Accessor operator of this LHS is unimplemented"
				);
				return Result::ERROR;
			}else{
				if(actual_lhs_type.qualifiers().size() > 1){
					if(
						actual_lhs_type.qualifiers().back().isOptional == false
						&& actual_lhs_type.qualifiers()[actual_lhs_type.qualifiers().size() - 2].isOptional
					){
						return this->optional_accessor<NEEDS_DEF>(
							instr, rhs_ident_str, lhs, actual_lhs_type_id, actual_lhs_type, true
						);
					}

					this->emit_error(
						Diagnostic::Code::SEMA_INVALID_ACCESSOR_RHS,
						instr.infix.lhs,
						"Accessor operator of this LHS is invalid"
					);
					return Result::ERROR;
				}

				if(actual_lhs_type.qualifiers().back().isOptional){
					return this->optional_accessor<NEEDS_DEF>(
						instr, rhs_ident_str, lhs, actual_lhs_type_id, actual_lhs_type, false
					);
				}

				is_pointer = true;
			}
		}


		if(actual_lhs_type.isInterfacePointer()){
			return this->interface_accessor<NEEDS_DEF>(
				instr, rhs_ident_str, lhs, actual_lhs_type_id, actual_lhs_type, true
			);
		}

		switch(actual_lhs_type.baseTypeID().kind()){
			case BaseType::Kind::STRUCT: {
				return this->struct_accessor<NEEDS_DEF>(
					instr, rhs_ident_str, lhs, actual_lhs_type_id, actual_lhs_type, is_pointer
				);
			} break;

			case BaseType::Kind::UNION: {
				return this->union_accessor<NEEDS_DEF>(
					instr, rhs_ident_str, lhs, actual_lhs_type_id, actual_lhs_type, is_pointer
				);
			} break;

			case BaseType::Kind::ENUM: {
				return this->enum_accessor<NEEDS_DEF>(
					instr, rhs_ident_str, lhs, actual_lhs_type_id, actual_lhs_type, is_pointer
				);
			} break;

			case BaseType::Kind::ARRAY: {
				return this->array_accessor<NEEDS_DEF>(
					instr, rhs_ident_str, lhs, actual_lhs_type_id, actual_lhs_type, is_pointer
				);
			} break;

			case BaseType::Kind::ARRAY_REF: {
				return this->array_ref_accessor<NEEDS_DEF>(
					instr, rhs_ident_str, lhs, actual_lhs_type_id, actual_lhs_type, is_pointer
				);
			} break;

			case BaseType::Kind::INTERFACE_IMPL_INSTANTIATION: {
				return this->interface_accessor<NEEDS_DEF>(
					instr, rhs_ident_str, lhs, actual_lhs_type_id, actual_lhs_type, false
				);
			} break;

			default: {
				this->emit_error(
					Diagnostic::Code::SEMA_INVALID_ACCESSOR_LHS,
					instr.infix.lhs,
					"Accessor operator of this LHS is invalid"
				);
				return Result::ERROR;
			} break;
		}

	}



	template<bool NEEDS_DEF>
	auto SemanticAnalyzer::instr_primitive_type(const Instruction::PrimitiveType& instr) -> Result {
		auto base_type = std::optional<BaseType::ID>();

		const Token::ID primitive_type_token_id = ASTBuffer::getPrimitiveType(instr.ast_type.base);
		const Token& primitive_type_token = this->source.getTokenBuffer()[primitive_type_token_id];

		switch(primitive_type_token.kind()){
			case Token::Kind::TYPE_VOID: {
				if(instr.ast_type.qualifiers.empty() == false){
					this->emit_error(
						Diagnostic::Code::SEMA_VOID_WITH_QUALIFIERS,
						instr.ast_type.base,
						"Type \"Void\" cannot have qualifiers"
					);
					return Result::ERROR;
				}
				this->return_type(instr.output, TypeInfo::VoidableID::Void());
				return Result::SUCCESS;
			} break;

			case Token::Kind::TYPE_THIS: {
				const std::optional<sema::ScopeManager::Scope::ObjectScope> current_type_scope = 
					this->scope.getCurrentTypeScopeIfExists();

				if(current_type_scope.has_value() == false){
					this->emit_error(
						Diagnostic::Code::SEMA_TYPE_THIS_NOT_IN_VALID_TYPE_SCOPE,
						instr.ast_type.base,
						"Type \"This\" not in valid type scope"
					);
					return Result::ERROR;
				}

				if(current_type_scope->is<BaseType::Interface::ID>()){
					this->emit_error(
						Diagnostic::Code::SEMA_TYPE_THIS_NOT_IN_VALID_TYPE_SCOPE,
						instr.ast_type.base,
						"Type \"This\" not in valid type scope",
						Diagnostic::Info("\"This\" cannot be used within an interface")
					);
					return Result::ERROR;
				}

				const TypeInfo::ID current_type_id = this->context.type_manager.getOrCreateTypeInfo(
					TypeInfo(BaseType::ID(current_type_scope->as<BaseType::Struct::ID>()))
				);


				if constexpr(NEEDS_DEF){
					const BaseType::Struct& struct_type = 
						this->context.getTypeManager().getStruct(current_type_scope->as<BaseType::Struct::ID>());

					if(struct_type.defCompleted.load() == false){
						SymbolProc::ID struct_type_symbol_proc_id =
							*this->context.symbol_proc_manager.getTypeSymbolProc(current_type_id);

						SymbolProc& struct_type_symbol_proc =
							this->context.symbol_proc_manager.getSymbolProc(struct_type_symbol_proc_id);

						const SymbolProc::WaitOnResult wait_on_result = struct_type_symbol_proc.waitOnDefIfNeeded(
							this->symbol_proc_id, this->context, struct_type_symbol_proc_id
						);
							
						switch(wait_on_result){
							case SymbolProc::WaitOnResult::NOT_NEEDED:  break;
							case SymbolProc::WaitOnResult::WAITING:     return Result::NEED_TO_WAIT;
							case SymbolProc::WaitOnResult::WAS_ERRORED: return Result::ERROR;

							case SymbolProc::WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND:
								evo::debugFatalBreak("Should be impossible");

							case SymbolProc::WaitOnResult::CIRCULAR_DEP_DETECTED: return Result::ERROR;
						}
					}
				}

				this->return_type(instr.output, TypeInfo::VoidableID(current_type_id));
				return Result::SUCCESS;
			} break;

			case Token::Kind::TYPE_INT:     case Token::Kind::TYPE_ISIZE:       case Token::Kind::TYPE_UINT:
			case Token::Kind::TYPE_USIZE:   case Token::Kind::TYPE_F16:         case Token::Kind::TYPE_BF16:
			case Token::Kind::TYPE_F32:     case Token::Kind::TYPE_F64:         case Token::Kind::TYPE_F80:
			case Token::Kind::TYPE_F128:    case Token::Kind::TYPE_BYTE:        case Token::Kind::TYPE_BOOL:
			case Token::Kind::TYPE_CHAR:    case Token::Kind::TYPE_RAWPTR:      case Token::Kind::TYPE_TYPEID:
			case Token::Kind::TYPE_C_WCHAR: case Token::Kind::TYPE_C_SHORT:     case Token::Kind::TYPE_C_USHORT:
			case Token::Kind::TYPE_C_INT:   case Token::Kind::TYPE_C_UINT:      case Token::Kind::TYPE_C_LONG:
			case Token::Kind::TYPE_C_ULONG: case Token::Kind::TYPE_C_LONG_LONG: case Token::Kind::TYPE_C_ULONG_LONG:
			case Token::Kind::TYPE_C_LONG_DOUBLE: {
				base_type = this->context.type_manager.getOrCreatePrimitiveBaseType(primitive_type_token.kind());
			} break;

			case Token::Kind::TYPE_I_N: case Token::Kind::TYPE_UI_N: {
				base_type = this->context.type_manager.getOrCreatePrimitiveBaseType(
					primitive_type_token.kind(), primitive_type_token.getBitWidth()
				);
			} break;


			case Token::Kind::TYPE_TYPE: {
				this->emit_error(
					Diagnostic::Code::SEMA_GENERIC_TYPE_NOT_IN_TEMPLATE_PACK_DECL,
					instr.ast_type,
					"Type \"Type\" may only be used in a template pack declaration"
				);
				return Result::ERROR;
			} break;

			default: {
				evo::debugFatalBreak("Unknown or unsupported PrimitiveType: {}", primitive_type_token.kind());
			} break;
		}

		evo::debugAssert(base_type.has_value(), "Base type was not set");

		if(this->check_type_qualifiers(instr.ast_type.qualifiers, instr.ast_type).isError()){ return Result::ERROR; }

		this->return_type(
			instr.output,
			TypeInfo::VoidableID(
				this->context.type_manager.getOrCreateTypeInfo(
					TypeInfo(*base_type, evo::copy(instr.ast_type.qualifiers))
				)
			)
		);
		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_array_type(const Instruction::ArrayType& instr) -> Result {
		const TypeInfo::VoidableID elem_type = this->get_type(instr.elem_type);

		if(elem_type.isVoid()){
			this->emit_error(
				Diagnostic::Code::SEMA_ARRAY_ELEM_TYPE_VOID,
				instr.array_type.elemType,
				"Element type of an array type cannot be type `Void`"
			);
			return Result::ERROR;
		}

		const bool is_deducer = [&](){
			for(const SymbolProc::TermInfoID& length_term_info_id : instr.dimensions){
				if(this->get_term_info(length_term_info_id).value_category == TermInfo::ValueCategory::EXPR_DEDUCER){
					return true;
				}
			}

			if(instr.terminator.has_value() == false){ return false; }
			return this->get_term_info(*instr.terminator).value_category == TermInfo::ValueCategory::EXPR_DEDUCER;
		}();


		if(is_deducer){
			auto dimensions = evo::SmallVector<BaseType::ArrayDeducer::Dimension>();
			dimensions.reserve(instr.dimensions.size());

			for(size_t i = 0; const SymbolProc::TermInfoID& length_term_info_id : instr.dimensions){
				TermInfo& length_term_info = this->get_term_info(length_term_info_id);

				if(length_term_info.value_category == TermInfo::ValueCategory::EXPR_DEDUCER){
					dimensions.emplace_back(length_term_info.type_id.as<TermInfo::ExprDeducerType>().deducer_token_id);

				}else{
					if(this->type_check<true, true>(
						TypeManager::getTypeUSize(),
						length_term_info,
						"Array dimension",
						*instr.array_type.dimensions[i]
					).ok == false){
						return Result::ERROR;
					}

					dimensions.emplace_back(
						static_cast<uint64_t>(
							this->context.getSemaBuffer().getIntValue(length_term_info.getExpr().intValueID()).value
						)
					);

				}

				i += 1;
			}


			auto terminator = evo::Variant<std::monostate, core::GenericValue, Token::ID>();
			if(instr.terminator.has_value()){
				TermInfo& terminator_term_info = this->get_term_info(*instr.terminator);

				if(terminator_term_info.value_category == TermInfo::ValueCategory::EXPR_DEDUCER){
					terminator = terminator_term_info.type_id.as<TermInfo::ExprDeducerType>().deducer_token_id;
					
				}else{
					if(this->type_check<true, true>(
						elem_type.asTypeID(), terminator_term_info, "Array terminator", *instr.array_type.terminator
					).ok == false){
						return Result::ERROR;
					}

					switch(terminator_term_info.getExpr().kind()){
						case sema::Expr::Kind::INT_VALUE: {
							terminator.emplace<core::GenericValue>(
								evo::copy(
									this->context.getSemaBuffer().getIntValue(
										terminator_term_info.getExpr().intValueID()
									).value
								)
							);
						} break;

						case sema::Expr::Kind::FLOAT_VALUE: {
							terminator.emplace<core::GenericValue>(
								evo::copy(
									this->context.getSemaBuffer().getFloatValue(
										terminator_term_info.getExpr().floatValueID()
									).value
								)
							);
						} break;

						case sema::Expr::Kind::BOOL_VALUE: {
							terminator.emplace<core::GenericValue>(
								this->context.getSemaBuffer().getBoolValue(
									terminator_term_info.getExpr().boolValueID()
								).value
							);
						} break;

						case sema::Expr::Kind::STRING_VALUE: {
							terminator.emplace<core::GenericValue>(
								this->context.getSemaBuffer().getStringValue(
									terminator_term_info.getExpr().stringValueID()
								).value
							);
						} break;

						case sema::Expr::Kind::AGGREGATE_VALUE: {
							this->emit_error(
								Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
								*instr.array_type.terminator,
								"Array terminators of aggregate types are unimplemented"
							);
							return Result::ERROR;
						} break;

						case sema::Expr::Kind::CHAR_VALUE: {
							terminator.emplace<core::GenericValue>(
								this->context.getSemaBuffer().getCharValue(
									terminator_term_info.getExpr().charValueID()
								).value
							);
						} break;

						default: evo::debugAssert("Invalid terminator kind");
					}
				}
			}


			const BaseType::ID array_deducer_type = this->context.type_manager.getOrCreateArrayDeducer(
				BaseType::ArrayDeducer(
					this->source.getID(), elem_type.asTypeID(), std::move(dimensions), std::move(terminator)
				)
			);


			this->return_term_info(instr.output,
				TermInfo::ValueCategory::TYPE,
				TypeInfo::VoidableID(this->context.type_manager.getOrCreateTypeInfo(TypeInfo(array_deducer_type)))
			);
			return Result::SUCCESS;
			
		}else{
			auto dimensions = evo::SmallVector<uint64_t>();
			dimensions.reserve(instr.dimensions.size());
			for(size_t i = 0; const SymbolProc::TermInfoID& length_term_info_id : instr.dimensions){
				TermInfo& length_term_info = this->get_term_info(length_term_info_id);

				if(this->type_check<true, true>(
					TypeManager::getTypeUSize(), length_term_info, "Array dimension", *instr.array_type.dimensions[i]
				).ok == false){
					return Result::ERROR;
				}

				dimensions.emplace_back(
					static_cast<uint64_t>(
						this->context.getSemaBuffer().getIntValue(length_term_info.getExpr().intValueID()).value
					)
				);

				i += 1;
			}


			auto terminator = std::optional<core::GenericValue>();
			if(instr.terminator.has_value()){
				TermInfo& terminator_term_info = this->get_term_info(*instr.terminator);

				if(this->type_check<true, true>(
					elem_type.asTypeID(), terminator_term_info, "Array terminator", *instr.array_type.terminator
				).ok == false){
					return Result::ERROR;
				}

				switch(terminator_term_info.getExpr().kind()){
					case sema::Expr::Kind::INT_VALUE: {
						terminator.emplace(
							evo::copy(
								this->context.getSemaBuffer().getIntValue(
									terminator_term_info.getExpr().intValueID()
								).value
							)
						);
					} break;

					case sema::Expr::Kind::FLOAT_VALUE: {
						terminator.emplace(
							evo::copy(
								this->context.getSemaBuffer().getFloatValue(
									terminator_term_info.getExpr().floatValueID()
								).value
							)
						);
					} break;

					case sema::Expr::Kind::BOOL_VALUE: {
						terminator.emplace(
							this->context.getSemaBuffer().getBoolValue(
								terminator_term_info.getExpr().boolValueID()
							).value
						);
					} break;

					case sema::Expr::Kind::STRING_VALUE: {
						terminator.emplace(
							this->context.getSemaBuffer().getStringValue(
								terminator_term_info.getExpr().stringValueID()
							).value
						);
					} break;

					case sema::Expr::Kind::AGGREGATE_VALUE: {
						this->emit_error(
							Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
							*instr.array_type.terminator,
							"Array terminators of aggregate types are unimplemented"
						);
						return Result::ERROR;
					} break;

					case sema::Expr::Kind::CHAR_VALUE: {
						terminator.emplace(
							this->context.getSemaBuffer().getCharValue(
								terminator_term_info.getExpr().charValueID()
							).value
						);
					} break;

					default: evo::debugAssert("Invalid terminator kind");
				}
			}


			const BaseType::ID array_type = this->context.type_manager.getOrCreateArray(
				BaseType::Array(elem_type.asTypeID(), std::move(dimensions), std::move(terminator))
			);


			this->return_term_info(instr.output,
				TermInfo::ValueCategory::TYPE,
				TypeInfo::VoidableID(this->context.type_manager.getOrCreateTypeInfo(TypeInfo(array_type)))
			);
			return Result::SUCCESS;
		}
	}


	auto SemanticAnalyzer::instr_array_ref(const Instruction::ArrayRef& instr) -> Result {
		const TypeInfo::VoidableID elem_type = this->get_type(instr.elem_type);

		if(elem_type.isVoid()){
			this->emit_error(
				Diagnostic::Code::SEMA_ARRAY_ELEM_TYPE_VOID,
				instr.array_type.elemType,
				"Element type of an array type cannot be type `Void`"
			);
			return Result::ERROR;
		}

		auto dimensions = evo::SmallVector<BaseType::ArrayRef::Dimension>();
		dimensions.reserve(instr.dimensions.size());
		for(size_t i = 0; const std::optional<SymbolProc::TermInfoID>& length_term_info_id : instr.dimensions){
			if(length_term_info_id.has_value()){
				TermInfo& length_term_info = this->get_term_info(*length_term_info_id);

				if(this->type_check<true, true>(
					TypeManager::getTypeUSize(), length_term_info, "Array dimension", *instr.array_type.dimensions[i]
				).ok == false){
					return Result::ERROR;
				}

				dimensions.emplace_back(
					static_cast<uint64_t>(
						this->context.getSemaBuffer().getIntValue(length_term_info.getExpr().intValueID()).value
					)
				);

			}else{
				dimensions.emplace_back(BaseType::ArrayRef::Dimension::ptr());
			}

			i += 1;
		}


		auto terminator = std::optional<core::GenericValue>();
		if(instr.terminator.has_value()){
			TermInfo& terminator_term_info = this->get_term_info(*instr.terminator);	

			if(this->type_check<true, true>(
				elem_type.asTypeID(), terminator_term_info, "Array reference terminator", *instr.array_type.terminator
			).ok == false){
				return Result::ERROR;
			}

			switch(terminator_term_info.getExpr().kind()){
				case sema::Expr::Kind::INT_VALUE: {
					terminator.emplace(
						evo::copy(
							this->context.getSemaBuffer().getIntValue(terminator_term_info.getExpr().intValueID()).value
						)
					);
				} break;

				case sema::Expr::Kind::FLOAT_VALUE: {
					terminator.emplace(
						evo::copy(
							this->context.getSemaBuffer().getFloatValue(
								terminator_term_info.getExpr().floatValueID()
							).value
						)
					);
				} break;

				case sema::Expr::Kind::BOOL_VALUE: {
					terminator.emplace(
						this->context.getSemaBuffer().getBoolValue(terminator_term_info.getExpr().boolValueID()).value
					);
				} break;

				case sema::Expr::Kind::STRING_VALUE: {
					terminator.emplace(
						this->context.getSemaBuffer().getStringValue(
							terminator_term_info.getExpr().stringValueID()
						).value
					);
				} break;

				case sema::Expr::Kind::AGGREGATE_VALUE: {
					this->emit_error(
						Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
						*instr.array_type.terminator,
						"Array terminators of aggregate types are unimplemented"
					);
					return Result::ERROR;
				} break;

				case sema::Expr::Kind::CHAR_VALUE: {
					terminator.emplace(
						this->context.getSemaBuffer().getCharValue(terminator_term_info.getExpr().charValueID()).value
					);
				} break;

				default: evo::debugAssert("Invalid terminator kind");
			}
		}


		const BaseType::ID array_ref_type = this->context.type_manager.getOrCreateArrayRef(
			BaseType::ArrayRef(
				elem_type.asTypeID(), std::move(dimensions), std::move(terminator), *instr.array_type.refIsReadOnly
			)
		);


		this->return_term_info(instr.output,
			TermInfo::ValueCategory::TYPE,
			TypeInfo::VoidableID(this->context.type_manager.getOrCreateTypeInfo(TypeInfo(array_ref_type)))
		);
		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_type_id_converter(const Instruction::TypeIDConverter& instr) -> Result {
		TermInfo& type_id_expr = this->get_term_info(instr.expr);

		if(this->type_check<true, true>(
			TypeManager::getTypeTypeID(), type_id_expr, "Type ID converter", instr.type_id_converter.expr
		).ok == false){
			return Result::ERROR;
		}

		const sema::IntValue& int_value =
			this->context.getSemaBuffer().getIntValue(type_id_expr.getExpr().intValueID());

		const TypeInfo::ID target_type_id = TypeInfo::ID(uint32_t(int_value.value));

		this->return_term_info(instr.output,
			TermInfo::ValueCategory::TYPE,
			TypeInfo::VoidableID(target_type_id)
		);
		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_user_type(const Instruction::UserType& instr) -> Result {
		auto base_type_id = std::optional<TypeInfo::ID>();
		const TermInfo& term_info = this->get_term_info(instr.base_type);
		switch(term_info.value_category){
			case TermInfo::ValueCategory::TYPE: {
				evo::debugAssert(
					this->get_term_info(instr.base_type).type_id.as<TypeInfo::VoidableID>().isVoid() == false,
					"`Void` is not a user-type"
				);
				base_type_id = term_info.type_id.as<TypeInfo::VoidableID>().asTypeID();
			} break;

			case TermInfo::ValueCategory::TEMPLATE_TYPE: {
				this->emit_error(
					Diagnostic::Code::SEMA_TEMPLATE_TYPE_NOT_INSTANTIATED,
					instr.ast_type.base,
					"Templated type needs to be instantiated",
					Diagnostic::Info(
						"Type declared here:", this->get_location(term_info.type_id.as<sema::TemplatedStruct::ID>())
					)
				);
				return Result::ERROR;
			} break;

			case TermInfo::ValueCategory::TEMPLATE_DECL_INSTANTIATION_TYPE: {
				this->return_type(instr.output, TypeInfo::VoidableID(TypeInfo::ID::createTemplateDeclInstantiation()));
				return Result::SUCCESS;
			} break;

			default: evo::debugFatalBreak("Invalid user type base");
		}

		const TypeInfo& base_type = this->context.getTypeManager().getTypeInfo(*base_type_id);

		if(this->check_type_qualifiers(instr.ast_type.qualifiers, instr.ast_type).isError()){ return Result::ERROR; }

		this->return_type(
			instr.output,
			TypeInfo::VoidableID(
				this->context.type_manager.getOrCreateTypeInfo(
					TypeInfo(base_type.baseTypeID(), evo::copy(instr.ast_type.qualifiers))
				)
			)
		);
		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_base_type_ident(const Instruction::BaseTypeIdent& instr) -> Result {
		const evo::Expected<TermInfo, Result> lookup_ident_result = this->lookup_ident_impl<true>(instr.ident);
		if(lookup_ident_result.has_value() == false){ return lookup_ident_result.error(); }

		this->return_term_info(instr.output, lookup_ident_result.value());
		return Result::SUCCESS;
	}






	template<bool NEEDS_DEF>
	auto SemanticAnalyzer::instr_ident(const Instruction::Ident<NEEDS_DEF>& instr) -> Result {
		const evo::Expected<TermInfo, Result> lookup_ident_result = this->lookup_ident_impl<NEEDS_DEF>(instr.ident);
		if(lookup_ident_result.has_value() == false){ return lookup_ident_result.error(); }

		if(
			this->scope.inObjectScope()
			&& this->scope.getCurrentObjectScope().is<sema::Func::ID>()
			&& this->expr_in_func_is_valid_value_stage(lookup_ident_result.value(), instr.ident) == false
		){
			return Result::ERROR;
		}

		this->return_term_info(instr.output, std::move(lookup_ident_result.value()));
		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_intrinsic(const Instruction::Intrinsic& instr) -> Result {
		const std::string_view intrinsic_name = this->source.getTokenBuffer()[instr.intrinsic].getString();


		const std::optional<IntrinsicFunc::Kind> intrinsic_kind = IntrinsicFunc::lookupKind(intrinsic_name);
		if(intrinsic_kind.has_value()){
			const TypeInfo::ID intrinsic_type = this->context.getIntrinsicFuncInfo(*intrinsic_kind).typeID;

			this->return_term_info(instr.output,
				TermInfo::ValueCategory::INTRINSIC_FUNC,
				TermInfo::ValueStage::CONSTEXPR,
				TermInfo::ValueState::NOT_APPLICABLE,
				intrinsic_type,
				sema::Expr(*intrinsic_kind)
			);
			return Result::SUCCESS;
		}

		const std::optional<TemplateIntrinsicFunc::Kind> template_intrinsic_kind = 
			TemplateIntrinsicFunc::lookupKind(intrinsic_name);
		if(template_intrinsic_kind.has_value()){
			this->return_term_info(instr.output,
				TermInfo::ValueCategory::TEMPLATE_INTRINSIC_FUNC, *template_intrinsic_kind
			);
			return Result::SUCCESS;
		}


		if(intrinsic_name == "pthr"){
			this->return_term_info(instr.output,
				TermInfo::ValueCategory::BUILTIN_MODULE, BuiltinModule::ID::PTHR
			);
			return Result::SUCCESS;
			
		}else if(intrinsic_name == "build"){
			this->emit_error(
				Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
				instr.intrinsic,
				"Intrinsic `@build` is currently unimplemented"
			);
		}


		this->emit_error(
			Diagnostic::Code::SEMA_INTRINSIC_DOESNT_EXIST,
			instr.intrinsic,
			std::format("Intrinsic \"@{}\" doesn't exist", intrinsic_name)
		);
		return Result::ERROR;
	}


	auto SemanticAnalyzer::instr_literal(const Instruction::Literal& instr) -> Result {
		const Token& literal_token = this->source.getTokenBuffer()[instr.literal];
		switch(literal_token.kind()){
			case Token::Kind::LITERAL_INT: {
				this->return_term_info(instr.output,
					TermInfo::ValueCategory::EPHEMERAL_FLUID,
					TermInfo::ValueStage::CONSTEXPR,
					TermInfo::ValueState::NOT_APPLICABLE,
					TermInfo::FluidType{},
					sema::Expr(this->context.sema_buffer.createIntValue(
						core::GenericInt(256, literal_token.getInt()), std::nullopt
					))
				);
				return Result::SUCCESS;
			} break;

			case Token::Kind::LITERAL_FLOAT: {
				this->return_term_info(instr.output,
					TermInfo::ValueCategory::EPHEMERAL_FLUID,
					TermInfo::ValueStage::CONSTEXPR,
					TermInfo::ValueState::NOT_APPLICABLE,
					TermInfo::FluidType{},
					sema::Expr(this->context.sema_buffer.createFloatValue(
						core::GenericFloat::createF128(literal_token.getFloat()), std::nullopt
					))
				);
				return Result::SUCCESS;
			} break;

			case Token::Kind::LITERAL_BOOL: {
				this->return_term_info(instr.output,
					TermInfo::ValueCategory::EPHEMERAL,
					TermInfo::ValueStage::CONSTEXPR,
					TermInfo::ValueState::NOT_APPLICABLE,
					this->context.getTypeManager().getTypeBool(),
					sema::Expr(this->context.sema_buffer.createBoolValue(literal_token.getBool()))
				);
				return Result::SUCCESS;
			} break;

			case Token::Kind::LITERAL_STRING: {
				this->return_term_info(instr.output,
					TermInfo::ValueCategory::EPHEMERAL,
					TermInfo::ValueStage::CONSTEXPR,
					TermInfo::ValueState::NOT_APPLICABLE,
					this->context.type_manager.getOrCreateTypeInfo(
						TypeInfo(
							this->context.type_manager.getOrCreateArray(
								BaseType::Array(
									this->context.getTypeManager().getTypeChar(),
									evo::SmallVector<uint64_t>{literal_token.getString().size()},
									core::GenericValue('\0')
								)
							),
							evo::SmallVector<AST::Type::Qualifier>{AST::Type::Qualifier(true, true, false, false)}
						)
					),
					sema::Expr(this->context.sema_buffer.createStringValue(std::string(literal_token.getString())))
				);
				return Result::SUCCESS;
			} break;

			case Token::Kind::LITERAL_CHAR: {
				this->return_term_info(instr.output,
					TermInfo::ValueCategory::EPHEMERAL,
					TermInfo::ValueStage::CONSTEXPR,
					TermInfo::ValueState::NOT_APPLICABLE,
					this->context.getTypeManager().getTypeChar(),
					sema::Expr(this->context.sema_buffer.createCharValue(literal_token.getChar()))
				);
				return Result::SUCCESS;
			} break;

			case Token::Kind::KEYWORD_NULL: {
				this->return_term_info(instr.output,
					TermInfo::ValueCategory::NULL_VALUE,
					TermInfo::ValueStage::CONSTEXPR,
					TermInfo::ValueState::NOT_APPLICABLE,
					TermInfo::NullType(),
					sema::Expr(this->context.sema_buffer.createNull(std::nullopt))
				);
				return Result::SUCCESS;
			} break;

			default: evo::debugFatalBreak("Not a valid literal");
		}
	}


	auto SemanticAnalyzer::instr_uninit(const Instruction::Uninit& instr) -> Result {
		this->return_term_info(instr.output,
			TermInfo::ValueCategory::INITIALIZER,
			TermInfo::ValueStage::CONSTEXPR,
			TermInfo::ValueState::NOT_APPLICABLE,
			TermInfo::InitializerType(),
			sema::Expr(this->context.sema_buffer.createUninit(instr.uninit_token))
		);
		return Result::SUCCESS;
	}

	auto SemanticAnalyzer::instr_zeroinit(const Instruction::Zeroinit& instr) -> Result {
		this->return_term_info(instr.output,
			TermInfo::ValueCategory::INITIALIZER,
			TermInfo::ValueStage::CONSTEXPR,
			TermInfo::ValueState::NOT_APPLICABLE,
			TermInfo::InitializerType(),
			sema::Expr(this->context.sema_buffer.createZeroinit(instr.zeroinit_token))
		);
		return Result::SUCCESS;
	}

	auto SemanticAnalyzer::instr_this(const Instruction::This& instr) -> Result {
		if(this->currently_in_func() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_EXPR_INVALID_OBJECT_SCOPE,
				instr.this_token,
				"[this] parameter must be in function scope"
			);
			return Result::ERROR;
		}

		const sema::Func& current_func = this->get_current_func();

		const std::optional<sema::Param::ID> this_param_id = this->scope.getThisParam();

		if(this_param_id.has_value() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_FUNC_HAS_NO_THIS_PARAM,
				instr.this_token,
				"This function doesn't have a [this] parameter",
				Diagnostic::Info("Function declared here:", this->get_location(current_func))
			);
			return Result::ERROR;
		}

		const BaseType::Function& current_func_type = 
			this->context.getTypeManager().getFunction(current_func.typeID);
		const BaseType::Function::Param& param = current_func_type.params[
			this->context.getSemaBuffer().getParam(*this_param_id).index
		];

		const TermInfo::ValueCategory value_category = [&](){
			switch(param.kind){
				case BaseType::Function::Param::Kind::READ:  return TermInfo::ValueCategory::CONCRETE_CONST;
				case BaseType::Function::Param::Kind::MUT:   return TermInfo::ValueCategory::CONCRETE_MUT;
				case BaseType::Function::Param::Kind::IN:    evo::debugFatalBreak("Cannot have an in [this] parameter");
				case BaseType::Function::Param::Kind::C:     evo::debugFatalBreak("Cannot have a c [this] parameter");
			}

			evo::unreachable();
		}();

		this->return_term_info(instr.output,
			value_category,
			current_func.isConstexpr ? TermInfo::ValueStage::COMPTIME : TermInfo::ValueStage::RUNTIME,
			TermInfo::ValueState::INIT,
			param.typeID,
			sema::Expr(*this_param_id)
		);

		return Result::SUCCESS;
	}

	auto SemanticAnalyzer::instr_type_deducer(const Instruction::TypeDeducer& instr) -> Result {
		const BaseType::ID new_type_deducer = this->context.type_manager.getOrCreateTypeDeducer(
			BaseType::TypeDeducer(instr.type_deducer_token, this->source.getID())
		);

		this->return_term_info(instr.output,
			TermInfo::ValueCategory::TYPE,
			TypeInfo::VoidableID(this->context.type_manager.getOrCreateTypeInfo(TypeInfo(new_type_deducer)))
		);
		return Result::SUCCESS;
	}

	auto SemanticAnalyzer::instr_expr_deducer(const Instruction::ExprDeducer& instr) -> Result {
		this->return_term_info(instr.output,
			TermInfo::ValueCategory::EXPR_DEDUCER,
			TermInfo::ExprDeducerType{instr.expr_deducer_token}
		);
		return Result::SUCCESS;
	}



	//////////////////////////////////////////////////////////////////////
	// accessor


	template<bool NEEDS_DEF>
	auto SemanticAnalyzer::module_accessor(
		const Instruction::Accessor<NEEDS_DEF>& instr, std::string_view rhs_ident_str, const TermInfo& lhs
	) -> Result {
		const Source& source_module = this->context.getSourceManager()[lhs.type_id.as<Source::ID>()];

		const sema::ScopeManager::Scope& source_module_sema_scope = 
			this->context.sema_buffer.scope_manager.getScope(*source_module.sema_scope_id);


		const sema::ScopeLevel& scope_level = this->context.sema_buffer.scope_manager.getLevel(
			source_module_sema_scope.getGlobalLevel()
		);

		const WaitOnSymbolProcResult wait_on_symbol_proc_result = this->wait_on_symbol_proc<NEEDS_DEF>(
			&source_module.global_symbol_procs, rhs_ident_str
		);


		switch(wait_on_symbol_proc_result){
			case WaitOnSymbolProcResult::NOT_FOUND: case WaitOnSymbolProcResult::ERROR_PASSED_BY_WHEN_COND: {
				this->wait_on_symbol_proc_emit_error(
					wait_on_symbol_proc_result,
					instr.infix.rhs,
					std::format("Module has no symbol named \"{}\"", rhs_ident_str)
				);
				return Result::ERROR;
			} break;

			case WaitOnSymbolProcResult::CIRCULAR_DEP_DETECTED: case WaitOnSymbolProcResult::EXISTS_BUT_ERRORED: {
				return Result::ERROR;
			} break;

			case WaitOnSymbolProcResult::NEED_TO_WAIT: {
				return Result::NEED_TO_WAIT;
			} break;

			case WaitOnSymbolProcResult::SEMAS_READY: {
				// do nothing...
			} break;
		}

		const evo::Expected<TermInfo, AnalyzeExprIdentInScopeLevelError> expr_ident = 
			this->analyze_expr_ident_in_scope_level<NEEDS_DEF, true>(
				instr.rhs_ident, rhs_ident_str, scope_level, true, true, &source_module
			);


		if(expr_ident.has_value()){
			this->return_term_info(instr.output, std::move(expr_ident.value()));
			return Result::SUCCESS;
		}

		switch(expr_ident.error()){
			case AnalyzeExprIdentInScopeLevelError::DOESNT_EXIST:
				evo::debugFatalBreak("Def is done, but can't find sema of symbol");

			case AnalyzeExprIdentInScopeLevelError::NEEDS_TO_WAIT_ON_DEF:
				evo::debugFatalBreak(
					"Sema doesn't have completed info for def despite SymbolProc saying it should"
				);

			case AnalyzeExprIdentInScopeLevelError::ERROR_EMITTED: return Result::ERROR;
		}

		evo::unreachable();
	}


	template<bool NEEDS_DEF>
	auto SemanticAnalyzer::clang_module_accessor(
		const Instruction::Accessor<NEEDS_DEF>& instr, std::string_view rhs_ident_str, const TermInfo& lhs
	) -> Result {
		const ClangSource& clang_source = this->context.getSourceManager()[lhs.type_id.as<ClangSource::ID>()];

		std::optional<ClangSource::SymbolInfo> clang_symbol = clang_source.getImportedSymbol(rhs_ident_str);

		if(clang_symbol.has_value() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_NO_SYMBOL_IN_SCOPE_WITH_THAT_IDENT,
				instr.infix.rhs,
				std::format("Module has no symbol named \"{}\"", rhs_ident_str)
			);
			return Result::ERROR;
		}

		clang_symbol->symbol.visit([&](const auto& symbol) -> void {
			using SymbolType = std::decay_t<decltype(symbol)>;

			if constexpr(std::is_same<SymbolType, BaseType::ID>()){
				this->return_term_info(instr.output,
					TermInfo::ValueCategory::TYPE,
					TypeInfo::VoidableID(this->context.type_manager.getOrCreateTypeInfo(TypeInfo(symbol)))
				);

			}else if constexpr(std::is_same<SymbolType, sema::Func::ID>()){
				this->return_term_info(instr.output,
					TermInfo::ValueCategory::FUNCTION, TermInfo::FuncOverloadList{symbol}
				);

			}else if constexpr(std::is_same<SymbolType, sema::GlobalVar::ID>()){
				const sema::GlobalVar& global_var = this->context.getSemaBuffer().getGlobalVar(symbol);

				if(global_var.kind == AST::VarDef::Kind::DEF){
					this->return_term_info(instr.output,
						TermInfo::ValueCategory::EPHEMERAL,
						TermInfo::ValueStage::COMPTIME,
						TermInfo::ValueState::NOT_APPLICABLE,
						*global_var.typeID,
						*global_var.expr.load()
					);

				}else{
					this->return_term_info(instr.output,
						global_var.kind == AST::VarDef::Kind::CONST 
							? TermInfo::ValueCategory::CONCRETE_CONST
							: TermInfo::ValueCategory::CONCRETE_MUT,
						TermInfo::ValueStage::RUNTIME,
						TermInfo::ValueState::NOT_APPLICABLE,
						*global_var.typeID,
						sema::Expr(symbol)
					);
				}
				
			}else{
				static_assert(false, "Unknown symbol kind");
			}
		});

		return Result::SUCCESS;
	}


	template<bool NEEDS_DEF>
	auto SemanticAnalyzer::type_accessor(
		const Instruction::Accessor<NEEDS_DEF>& instr, std::string_view rhs_ident_str, const TermInfo& lhs
	) -> Result {
		if(lhs.type_id.as<TypeInfo::VoidableID>().isVoid()){
			this->emit_error(
				Diagnostic::Code::SEMA_INVALID_ACCESSOR_RHS,
				instr.infix.lhs,
				"Accessor operator of type `Void` is invalid"
			);
			return Result::ERROR;
		}

		const TypeInfo::ID actual_lhs_type_id = this->get_actual_type<true, true>(
			lhs.type_id.as<TypeInfo::VoidableID>().asTypeID()
		);
		const TypeInfo& actual_lhs_type = this->context.getTypeManager().getTypeInfo(actual_lhs_type_id);

		if(actual_lhs_type.qualifiers().empty() == false){
			// TODO(FUTURE): better message
			this->emit_error(
				Diagnostic::Code::SEMA_INVALID_ACCESSOR_RHS,
				instr.infix.lhs,
				"Accessor operator of this LHS is unsupported"
			);
			return Result::ERROR;
		}





		SymbolProcNamespace const * namespaced_members = nullptr;
		sema::ScopeLevel const * scope_level = nullptr;
		Source const * type_source = nullptr;

		switch(actual_lhs_type.baseTypeID().kind()){
			case BaseType::Kind::STRUCT: {
				const BaseType::Struct& lhs_struct = this->context.getTypeManager().getStruct(
					actual_lhs_type.baseTypeID().structID()
				);

				namespaced_members = lhs_struct.namespacedMembers;
				scope_level = lhs_struct.scopeLevel;

				if(lhs_struct.isClangType() == false){
					type_source = &this->context.getSourceManager()[lhs_struct.sourceID.as<Source::ID>()];
				}
			} break;

			case BaseType::Kind::UNION: {
				const BaseType::Union& lhs_union = this->context.getTypeManager().getUnion(
					actual_lhs_type.baseTypeID().unionID()
				);

				for(uint32_t i = 0; const BaseType::Union::Field& field : lhs_union.fields){
					if(lhs_union.getFieldName(field, this->context.getSourceManager()) == rhs_ident_str){
						if(lhs_union.isUntagged){
							this->emit_error(
								Diagnostic::Code::SEMA_UNION_UNTAGGED_TYPE_FIELD_ACCESS,
								instr.infix.lhs,
								"Cannot type access a field of an untagged union"
							);
							return Result::SUCCESS;
						}

						this->return_term_info(instr.output,
							TermInfo::ValueCategory::TAGGED_UNION_FIELD_ACCESSOR,
							TermInfo::TaggedUnionFieldAccessor(actual_lhs_type.baseTypeID().unionID(), i)
						);
						return Result::SUCCESS;
					}

					i += 1;
				}

				namespaced_members = lhs_union.namespacedMembers;
				scope_level = lhs_union.scopeLevel;

				if(lhs_union.isClangType() == false){
					type_source = &this->context.getSourceManager()[lhs_union.sourceID.as<Source::ID>()];
				}
			} break;

			case BaseType::Kind::ENUM: {
				const BaseType::Enum& lhs_enum = this->context.getTypeManager().getEnum(
					actual_lhs_type.baseTypeID().enumID()
				);

				for(const BaseType::Enum::Enumerator& enumerator : lhs_enum.enumerators){
					if(lhs_enum.getEnumeratorName(enumerator, this->context.getSourceManager()) == rhs_ident_str){
						this->return_term_info(instr.output,
							TermInfo::ValueCategory::EPHEMERAL,
							TermInfo::ValueStage::CONSTEXPR,
							TermInfo::ValueState::NOT_APPLICABLE,
							actual_lhs_type_id,
							sema::Expr(
								this->context.sema_buffer.createIntValue(enumerator.value, actual_lhs_type.baseTypeID())
							)
						);
						return Result::SUCCESS;
					}
				}

				namespaced_members = lhs_enum.namespacedMembers;
				scope_level = lhs_enum.scopeLevel;

				if(lhs_enum.isClangType() == false){
					type_source = &this->context.getSourceManager()[lhs_enum.sourceID.as<Source::ID>()];
				}
			} break;

			default: {
				this->emit_error(
					Diagnostic::Code::SEMA_INVALID_ACCESSOR_RHS,
					instr.infix.lhs,
					"Accessor operator of the type of this LHS is unsupported"
				);
				return Result::ERROR;
			} break;
		}


		const WaitOnSymbolProcResult wait_on_symbol_proc_result = this->wait_on_symbol_proc<NEEDS_DEF>(
			namespaced_members, rhs_ident_str
		);


		switch(wait_on_symbol_proc_result){
			case WaitOnSymbolProcResult::NOT_FOUND: case WaitOnSymbolProcResult::ERROR_PASSED_BY_WHEN_COND: {
				this->wait_on_symbol_proc_emit_error(
					wait_on_symbol_proc_result,
					instr.infix.rhs,
					std::format("Type has no member named \"{}\"", rhs_ident_str)
				);
				return Result::ERROR;
			} break;

			case WaitOnSymbolProcResult::CIRCULAR_DEP_DETECTED: case WaitOnSymbolProcResult::EXISTS_BUT_ERRORED: {
				return Result::ERROR;
			} break;

			case WaitOnSymbolProcResult::NEED_TO_WAIT: {
				return Result::NEED_TO_WAIT;
			} break;

			case WaitOnSymbolProcResult::SEMAS_READY: {
				// do nothing...
			} break;
		}


		const evo::Expected<TermInfo, AnalyzeExprIdentInScopeLevelError> expr_ident = 
			this->analyze_expr_ident_in_scope_level<NEEDS_DEF, false>(
				instr.rhs_ident, rhs_ident_str, *scope_level, true, true, type_source
			);

		if(expr_ident.has_value() == false){
			switch(expr_ident.error()){
				case AnalyzeExprIdentInScopeLevelError::DOESNT_EXIST: {
					evo::debugFatalBreak("Def is done, but can't find sema of symbol");
				} break;

				case AnalyzeExprIdentInScopeLevelError::NEEDS_TO_WAIT_ON_DEF: {
					evo::debugFatalBreak(
						"Sema doesn't have completed info for def despite SymbolProc saying it should"
					);
				} break;

				case AnalyzeExprIdentInScopeLevelError::ERROR_EMITTED: return Result::ERROR;
			}

		}

		this->return_term_info(instr.output, std::move(expr_ident.value()));
		return Result::SUCCESS;
	}


	template<bool NEEDS_DEF>
	auto SemanticAnalyzer::builtin_module_accessor(
		const Instruction::Accessor<NEEDS_DEF>& instr, std::string_view rhs_ident_str, const TermInfo& lhs
	) -> Result {
		const BuiltinModule& builtin_module = this->context.getSourceManager()[lhs.type_id.as<BuiltinModule::ID>()];

		const std::optional<BuiltinModule::Symbol> symbol_find = builtin_module.getSymbol(rhs_ident_str);

		if(symbol_find.has_value() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_NO_SYMBOL_IN_SCOPE_WITH_THAT_IDENT,
				instr.infix.rhs,
				std::format("Builtin-module has no symbol named \"{}\"", rhs_ident_str)
			);
			return Result::ERROR;
		}

		symbol_find->visit([&](const auto& symbol) -> void {
			using SymbolType = std::decay_t<decltype(symbol)>;

			if constexpr(std::is_same<SymbolType, BaseType::ID>()){
				if(symbol.kind() == BaseType::Kind::STRUCT){
					BaseType::Struct& struct_type = this->context.type_manager.getStruct(symbol.structID());
					struct_type.shouldLower = true;
				}

				this->return_term_info(instr.output,
					TermInfo::ValueCategory::TYPE,
					TypeInfo::VoidableID(this->context.type_manager.getOrCreateTypeInfo(TypeInfo(symbol)))
				);

			}else if constexpr(std::is_same<SymbolType, sema::Func::ID>()){
				this->return_term_info(instr.output,
					TermInfo::ValueCategory::FUNCTION, TermInfo::FuncOverloadList{symbol}
				);

			}else if constexpr(std::is_same<SymbolType, sema::GlobalVar::ID>()){
				const sema::GlobalVar& global_var = this->context.getSemaBuffer().getGlobalVar(symbol);

				this->return_term_info(instr.output,
					global_var.kind == AST::VarDef::Kind::CONST 
						? TermInfo::ValueCategory::CONCRETE_CONST
						: TermInfo::ValueCategory::CONCRETE_MUT,
					TermInfo::ValueStage::RUNTIME,
					TermInfo::ValueState::NOT_APPLICABLE,
					*global_var.typeID,
					sema::Expr(symbol)
				);
				
			}else{
				static_assert(false, "Unknown symbol kind");
			}
		});

		return Result::SUCCESS;
	}




	template<bool NEEDS_DEF>
	auto SemanticAnalyzer::interface_accessor(
		const Instruction::Accessor<NEEDS_DEF>& instr,
		std::string_view rhs_ident_str,
		const TermInfo& lhs,
		TypeInfo::ID actual_lhs_type_id,
		const TypeInfo& actual_lhs_type,
		bool is_pointer
	) -> Result {
		auto impl_instantiation_type_id = std::optional<TypeInfo::ID>();

		const BaseType::Interface& target_interface = [&]() -> const BaseType::Interface& {
			if(is_pointer){
				return this->context.getTypeManager().getInterface(actual_lhs_type.baseTypeID().interfaceID());

			}else{
				const BaseType::InterfaceImplInstantiation& interface_impl_instantiation = 
					this->context.getTypeManager().getInterfaceImplInstantiation(
						actual_lhs_type.baseTypeID().interfaceImplInstantiationID()
					);

				impl_instantiation_type_id = interface_impl_instantiation.implInstantiationTypeID;

				return this->context.getTypeManager().getInterface(interface_impl_instantiation.interfacedID);
			}
		}();


		// make sure def of target interface completed
		if(target_interface.defCompleted.load() == false){
			SymbolProc& target_interface_symbol_proc =
				this->context.symbol_proc_manager.getSymbolProc(target_interface.symbolProcID);

			const SymbolProc::WaitOnResult wait_on_result = target_interface_symbol_proc.waitOnDefIfNeeded(
				this->symbol_proc_id, this->context, target_interface.symbolProcID
			);
				
			switch(wait_on_result){
				case SymbolProc::WaitOnResult::NOT_NEEDED:                 break;
				case SymbolProc::WaitOnResult::WAITING:                    return Result::NEED_TO_WAIT;
				case SymbolProc::WaitOnResult::WAS_ERRORED:                return Result::ERROR;
				case SymbolProc::WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND: evo::debugFatalBreak("Should be impossible");
				case SymbolProc::WaitOnResult::CIRCULAR_DEP_DETECTED:      return Result::ERROR;
			}
		}


		if(is_pointer){
			auto methods = TermInfo::FuncOverloadList();
			for(const sema::Func::ID method_id : target_interface.methods){
				const sema::Func& method = this->context.getSemaBuffer().getFunc(method_id);
				const std::string_view method_name = method.getName(this->context.getSourceManager());

				if(method_name == rhs_ident_str){
					methods.emplace_back(method_id);
				}
			}

			if(methods.empty()){
				this->emit_error(
					Diagnostic::Code::SEMA_INTERFACE_NO_METHOD_WITH_THAT_NAME,
					instr.infix.rhs,
					"This interface has no method with that name"
				);
				return Result::ERROR;
			}


			const sema::FakeTermInfo::ID created_fake_term_info = this->context.sema_buffer.createFakeTermInfo(
				TermInfo::convertValueCategory(lhs.value_category),
				TermInfo::convertValueStage(lhs.value_stage),
				TermInfo::convertValueState(lhs.value_state),
				lhs.type_id.as<TypeInfo::ID>(),
				lhs.getExpr()
			);


			this->return_term_info(instr.output,
				TermInfo::ValueCategory::INTERFACE_CALL,
				lhs.value_stage,
				TermInfo::ValueState::NOT_APPLICABLE,
				std::move(methods),
				sema::Expr(created_fake_term_info)
			);
			return Result::SUCCESS;

		}else{
			const BaseType::Interface::Impl& interface_impl = target_interface.impls.at(
				this->context.getTypeManager().getTypeInfo(*impl_instantiation_type_id).baseTypeID()
			);

			auto func_overload_list = TermInfo::FuncOverloadList();
			for(size_t i = 0; const sema::Func::ID method_id : target_interface.methods){
				const sema::Func& method = this->context.getSemaBuffer().getFunc(method_id);
				const std::string_view method_name = method.getName(this->context.getSourceManager());

				if(method_name == rhs_ident_str){
					func_overload_list.emplace_back(interface_impl.methods[i]);
				}

				i += 1;
			}

			if(func_overload_list.empty()){
				this->emit_error(
					Diagnostic::Code::SEMA_INTERFACE_NO_METHOD_WITH_THAT_NAME,
					instr.infix.rhs,
					"This interface has no method with that name"
				);
				return Result::ERROR;
			}

			const sema::FakeTermInfo::ID method_this = this->context.sema_buffer.createFakeTermInfo(
				TermInfo::convertValueCategory(lhs.value_category),
				TermInfo::convertValueStage(lhs.value_stage),
				TermInfo::convertValueState(lhs.value_state),
				actual_lhs_type_id,
				lhs.getExpr()
			);

			this->return_term_info(instr.output,
				TermInfo::ValueCategory::METHOD_CALL,
				TermInfo::ValueStage::CONSTEXPR,
				TermInfo::ValueState::NOT_APPLICABLE,
				std::move(func_overload_list),
				sema::Expr(method_this)
			);
			return Result::SUCCESS;
		}
	}



	template<bool NEEDS_DEF>
	auto SemanticAnalyzer::optional_accessor(
		const Instruction::Accessor<NEEDS_DEF>& instr,
		std::string_view rhs_ident_str,
		const TermInfo& lhs,
		TypeInfo::ID actual_lhs_type_id,
		const TypeInfo& actual_lhs_type,
		bool is_pointer
	) -> Result {
		const TypeInfo::ID optional_type_id = [&](){
			if(is_pointer){
				return this->context.type_manager.getOrCreateTypeInfo(actual_lhs_type.copyWithPoppedQualifier());
			}else{
				return actual_lhs_type_id;
			}
		}();

		const TypeInfo::ID optional_held_type_id = this->context.type_manager.getOrCreateTypeInfo(
			actual_lhs_type.copyWithPoppedQualifier(1 + size_t(is_pointer))
		);


		const sema::Expr method_this = [&](){
			sema::Expr lhs_expr = lhs.getExpr();
			if(is_pointer){
				lhs_expr = sema::Expr(this->context.sema_buffer.createDeref(lhs_expr, actual_lhs_type_id));
			}

			return sema::Expr(
				this->context.sema_buffer.createFakeTermInfo(
					TermInfo::convertValueCategory(lhs.value_category),
					TermInfo::convertValueStage(lhs.value_stage),
					TermInfo::convertValueState(lhs.value_state),
					optional_type_id,
					lhs_expr
				)
			);
		}();

		if(rhs_ident_str == "extract"){
			const TypeInfo::ID method_type = this->context.type_manager.getOrCreateTypeInfo(
				TypeInfo(
					this->context.type_manager.getOrCreateFunction(
						BaseType::Function(
							evo::SmallVector<BaseType::Function::Param>(),
							evo::SmallVector<BaseType::Function::ReturnParam>{
								BaseType::Function::ReturnParam(std::nullopt, optional_held_type_id)
							},
							evo::SmallVector<BaseType::Function::ReturnParam>()
						)
					)
				)
			);

			this->return_term_info(instr.output,
				TermInfo::ValueCategory::BUILTIN_TYPE_METHOD,
				TermInfo::ValueStage::CONSTEXPR,
				TermInfo::ValueState::NOT_APPLICABLE,
				TermInfo::BuiltinTypeMethod(
					method_type, TermInfo::BuiltinTypeMethod::Kind::OPT_EXTRACT
				),
				method_this
			);
			return Result::SUCCESS;
		}


		this->emit_error(
			Diagnostic::Code::SEMA_INVALID_ACCESSOR_RHS,
			instr.infix.lhs,
			"Accessor operator of this LHS is invalid",
			Diagnostic::Info("Did you mean to unwrap the optional?")
		);
		return Result::ERROR;
	}




	template<bool NEEDS_DEF>
	auto SemanticAnalyzer::struct_accessor(
		const Instruction::Accessor<NEEDS_DEF>& instr,
		std::string_view rhs_ident_str,
		const TermInfo& lhs,
		TypeInfo::ID actual_lhs_type_id,
		const TypeInfo& actual_lhs_type,
		bool is_pointer
	) -> Result {
		const BaseType::Struct& lhs_type_struct = this->context.getTypeManager().getStruct(
			actual_lhs_type.baseTypeID().structID()
		);

		const Source* struct_source = [&]() -> const Source* {
			if(lhs_type_struct.isPTHRSourceType()){
				return &this->context.getSourceManager()[lhs_type_struct.sourceID.as<Source::ID>()];
			}else{
				return nullptr;
			}
		}();

		{
			const auto lock = std::scoped_lock(lhs_type_struct.memberVarsLock);
			for(size_t i = 0; const BaseType::Struct::MemberVar* member_var : lhs_type_struct.memberVarsABI){
				const std::string_view member_ident_str = 
					lhs_type_struct.getMemberName(*member_var, this->context.getSourceManager());

				if(member_ident_str == rhs_ident_str){
					const TermInfo::ValueCategory value_category = [&](){
						if(lhs.is_ephemeral() && is_pointer == false){ return lhs.value_category; }

						if(lhs.value_category == TermInfo::ValueCategory::CONCRETE_CONST){
							return TermInfo::ValueCategory::CONCRETE_CONST;
						}

						if(member_var->kind == AST::VarDef::Kind::CONST){
							return TermInfo::ValueCategory::CONCRETE_CONST;
						}else{
							return TermInfo::ValueCategory::CONCRETE_MUT;
						}
					}();

					using ValueStage = TermInfo::ValueStage;


					if(lhs.value_stage == ValueStage::CONSTEXPR){
						const sema::AggregateValue& lhs_aggregate_value =
							this->context.getSemaBuffer().getAggregateValue(lhs.getExpr().aggregateValueID());

						this->return_term_info(instr.output,
							TermInfo::ValueCategory::EPHEMERAL,
							ValueStage::CONSTEXPR,
							TermInfo::ValueState::NOT_APPLICABLE,
							member_var->typeID,
							lhs_aggregate_value.values[i]
						);
						
					}else{
						const sema::Expr sema_expr = [&](){
							if(is_pointer){
								const TypeInfo::ID resultant_type_id = this->context.type_manager.getOrCreateTypeInfo(
									TypeInfo(actual_lhs_type.baseTypeID())
								);

								const sema::Deref::ID deref =
									this->context.sema_buffer.createDeref(lhs.getExpr(), resultant_type_id);

								return sema::Expr(
									this->context.sema_buffer.createAccessor(
										sema::Expr(deref), resultant_type_id, uint32_t(i)
									)
								);
							}else{
								return sema::Expr(
									this->context.sema_buffer.createAccessor(
										lhs.getExpr(), actual_lhs_type_id, uint32_t(i)
									)
								);
							}
						}();


						const TermInfo::ValueState value_state = [&](){
							if(lhs.value_state == TermInfo::ValueState::INITIALIZING){
								return this->get_ident_value_state(
									sema::ReturnParamAccessorValueStateID(lhs.getExpr().returnParamID(), uint32_t(i))
								);

							}else if(
								this->source.getTokenBuffer()[this->get_current_func().name.as<Token::ID>()].kind()
									== Token::Kind::KEYWORD_DELETE
								&& lhs.getExpr().kind() == sema::Expr::Kind::PARAM
							){
								return this->get_ident_value_state(sema::OpDeleteThisAccessorValueStateID(uint32_t(i)));
							}else{
								return TermInfo::ValueState::NOT_APPLICABLE;
							}
						}();

						this->return_term_info(instr.output,
							value_category,
							this->get_current_func().isConstexpr ? ValueStage::COMPTIME : ValueStage::RUNTIME,
							value_state,
							member_var->typeID,
							sema_expr
						);
					}

					return Result::SUCCESS;
				}

				i += 1;
			}
		}


		///////////////////////////////////
		// method

		const WaitOnSymbolProcResult wait_on_symbol_proc_result = this->wait_on_symbol_proc<NEEDS_DEF>(
			lhs_type_struct.namespacedMembers, rhs_ident_str
		);

		switch(wait_on_symbol_proc_result){
			case WaitOnSymbolProcResult::NOT_FOUND: case WaitOnSymbolProcResult::ERROR_PASSED_BY_WHEN_COND: {
				this->wait_on_symbol_proc_emit_error(
					wait_on_symbol_proc_result,
					instr.infix.rhs,
					std::format("Struct has no member named \"{}\"", rhs_ident_str)
				);
				return Result::ERROR;
			} break;

			case WaitOnSymbolProcResult::CIRCULAR_DEP_DETECTED: case WaitOnSymbolProcResult::EXISTS_BUT_ERRORED: {
				return Result::ERROR;
			} break;

			case WaitOnSymbolProcResult::NEED_TO_WAIT: {
				return Result::NEED_TO_WAIT;
			} break;

			case WaitOnSymbolProcResult::SEMAS_READY: {
				// do nothing...
			} break;
		}


		evo::Expected<TermInfo, AnalyzeExprIdentInScopeLevelError> expr_ident = 
			this->analyze_expr_ident_in_scope_level<NEEDS_DEF, false>(
				instr.rhs_ident, rhs_ident_str, *lhs_type_struct.scopeLevel, true, true, struct_source
			);


		if(expr_ident.has_value() == false){
			switch(expr_ident.error()){
				case AnalyzeExprIdentInScopeLevelError::DOESNT_EXIST: {
					evo::debugFatalBreak("Def is done, but can't find sema of symbol");
				} break;

				case AnalyzeExprIdentInScopeLevelError::NEEDS_TO_WAIT_ON_DEF: {
					evo::debugFatalBreak(
						"Sema doesn't have completed info for def despite SymbolProc saying it should"
					);
				} break;

				case AnalyzeExprIdentInScopeLevelError::ERROR_EMITTED: return Result::ERROR;
			}
		}



		const sema::FakeTermInfo::ID method_this = [&](){
			if(is_pointer){
				const TypeInfo::ID resultant_type_id = this->context.type_manager.getOrCreateTypeInfo(
					TypeInfo(actual_lhs_type.baseTypeID())
				);

				return this->context.sema_buffer.createFakeTermInfo(
					TermInfo::convertValueCategory(lhs.value_category),
					TermInfo::convertValueStage(lhs.value_stage),
					TermInfo::convertValueState(lhs.value_state),
					resultant_type_id,
					sema::Expr(this->context.sema_buffer.createDeref(lhs.getExpr(), resultant_type_id))
				);
				
			}else{
				return this->context.sema_buffer.createFakeTermInfo(
					TermInfo::convertValueCategory(lhs.value_category),
					TermInfo::convertValueStage(lhs.value_stage),
					TermInfo::convertValueState(lhs.value_state),
					actual_lhs_type_id,
					lhs.getExpr()
				);
			}
		}();

		this->return_term_info(instr.output,
			TermInfo::ValueCategory::METHOD_CALL,
			expr_ident.value().value_stage,
			TermInfo::ValueState::NOT_APPLICABLE,
			std::move(expr_ident.value().type_id),
			sema::Expr(method_this)
		);
		return Result::SUCCESS;
	}




	template<bool NEEDS_DEF>
	auto SemanticAnalyzer::union_accessor(
		const Instruction::Accessor<NEEDS_DEF>& instr,
		std::string_view rhs_ident_str,
		const TermInfo& lhs,
		TypeInfo::ID actual_lhs_type_id,
		const TypeInfo& actual_lhs_type,
		bool is_pointer
	) -> Result {
		const BaseType::Union& lhs_type_union = this->context.getTypeManager().getUnion(
			actual_lhs_type.baseTypeID().unionID()
		);

		const Source* union_source = [&]() -> const Source* {
			if(lhs_type_union.isClangType()){
				return nullptr;
			}else{
				return &this->context.getSourceManager()[lhs_type_union.sourceID.as<Source::ID>()];
			}
		}();

		const sema::ScopeLevel::IdentID* lookup_ident = lhs_type_union.scopeLevel->lookupIdent(rhs_ident_str);

		if(lookup_ident == nullptr){
			this->emit_error(
				Diagnostic::Code::SEMA_NO_SYMBOL_IN_SCOPE_WITH_THAT_IDENT,
				instr.infix.rhs,
				std::format("Identifier \"{}\" was not defined in this scope", rhs_ident_str)
			);
			return Result::ERROR;
		}


		if(lookup_ident->is<sema::ScopeLevel::UnionField>()){
			const BaseType::Union::Field& union_field = 
				lhs_type_union.fields[lookup_ident->as<sema::ScopeLevel::UnionField>().field_index];

			if(union_field.typeID.isVoid()){
				this->emit_error(
					Diagnostic::Code::SEMA_UNION_ACCESSOR_IS_VOID,
					instr.infix.rhs,
					std::format("Cannot access union fields that are type `Void`")
				);
				return Result::ERROR;
			}



			const TermInfo::ValueCategory value_category = [&](){
				if(lhs.is_ephemeral() && is_pointer == false){ return lhs.value_category; }

				if(lhs.value_category == TermInfo::ValueCategory::CONCRETE_CONST){
					return TermInfo::ValueCategory::CONCRETE_CONST;
				}

				return TermInfo::ValueCategory::CONCRETE_MUT;
			}();


			using ValueStage = TermInfo::ValueStage;

			if(lhs.value_stage == ValueStage::CONSTEXPR){
				this->emit_error(
					Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
					instr.infix,
					"Constexpr union accessor is currenlty unsupported"
				);
				return Result::ERROR;
				
			}else{
				const sema::Expr sema_expr = [&](){
					if(is_pointer){
						const TypeInfo::ID target_type_id = this->context.type_manager.getOrCreateTypeInfo(
							TypeInfo(actual_lhs_type.baseTypeID())
						);

						const sema::Deref::ID deref =
							this->context.sema_buffer.createDeref(lhs.getExpr(), target_type_id);

						return sema::Expr(
							this->context.sema_buffer.createUnionAccessor(
								sema::Expr(deref),
								target_type_id,
								lookup_ident->as<sema::ScopeLevel::UnionField>().field_index
							)
						);
					}else{
						return sema::Expr(
							this->context.sema_buffer.createUnionAccessor(
								lhs.getExpr(),
								actual_lhs_type_id,
								lookup_ident->as<sema::ScopeLevel::UnionField>().field_index
							)
						);
					}
				}();

				this->return_term_info(instr.output,
					value_category,
					this->get_current_func().isConstexpr ? ValueStage::COMPTIME : ValueStage::RUNTIME,
					TermInfo::ValueState::NOT_APPLICABLE,
					lhs_type_union.fields[
						lookup_ident->as<sema::ScopeLevel::UnionField>().field_index
					].typeID.asTypeID(),
					sema_expr
				);
			}

			return Result::SUCCESS;
		}

		
		///////////////////////////////////
		// method

		const WaitOnSymbolProcResult wait_on_symbol_proc_result = this->wait_on_symbol_proc<NEEDS_DEF>(
			lhs_type_union.namespacedMembers, rhs_ident_str
		);


		switch(wait_on_symbol_proc_result){
			case WaitOnSymbolProcResult::NOT_FOUND: case WaitOnSymbolProcResult::ERROR_PASSED_BY_WHEN_COND: {
				this->wait_on_symbol_proc_emit_error(
					wait_on_symbol_proc_result,
					instr.infix.rhs,
					std::format("Union has no member named \"{}\"", rhs_ident_str)
				);
				return Result::ERROR;
			} break;

			case WaitOnSymbolProcResult::CIRCULAR_DEP_DETECTED: case WaitOnSymbolProcResult::EXISTS_BUT_ERRORED: {
				return Result::ERROR;
			} break;

			case WaitOnSymbolProcResult::NEED_TO_WAIT: {
				return Result::NEED_TO_WAIT;
			} break;

			case WaitOnSymbolProcResult::SEMAS_READY: {
				// do nothing...
			} break;
		}


		evo::Expected<TermInfo, AnalyzeExprIdentInScopeLevelError> expr_ident = 
			this->analyze_expr_ident_in_scope_level<NEEDS_DEF, false>(
				instr.rhs_ident, rhs_ident_str, *lhs_type_union.scopeLevel, true, true, union_source
			);


		if(expr_ident.has_value() == false){
			switch(expr_ident.error()){
				case AnalyzeExprIdentInScopeLevelError::DOESNT_EXIST: {
					evo::debugFatalBreak("Def is done, but can't find sema of symbol");
				} break;

				case AnalyzeExprIdentInScopeLevelError::NEEDS_TO_WAIT_ON_DEF: {
					evo::debugFatalBreak(
						"Sema doesn't have completed info for def despite SymbolProc saying it should"
					);
				} break;

				case AnalyzeExprIdentInScopeLevelError::ERROR_EMITTED: return Result::ERROR;
			}
		}


		const sema::FakeTermInfo::ID method_this = [&](){
			if(is_pointer){
				const TypeInfo::ID resultant_type_id = this->context.type_manager.getOrCreateTypeInfo(
					TypeInfo(actual_lhs_type.baseTypeID())
				);

				return this->context.sema_buffer.createFakeTermInfo(
					TermInfo::convertValueCategory(lhs.value_category),
					TermInfo::convertValueStage(lhs.value_stage),
					TermInfo::convertValueState(lhs.value_state),
					resultant_type_id,
					sema::Expr(this->context.sema_buffer.createDeref(lhs.getExpr(), resultant_type_id))
				);
				
			}else{
				return this->context.sema_buffer.createFakeTermInfo(
					TermInfo::convertValueCategory(lhs.value_category),
					TermInfo::convertValueStage(lhs.value_stage),
					TermInfo::convertValueState(lhs.value_state),
					actual_lhs_type_id,
					lhs.getExpr()
				);
			}
		}();

		this->return_term_info(instr.output,
			TermInfo::ValueCategory::METHOD_CALL,
			expr_ident.value().value_stage,
			TermInfo::ValueState::NOT_APPLICABLE,
			std::move(expr_ident.value().type_id),
			sema::Expr(method_this)
		);
		return Result::SUCCESS;
	}



	template<bool NEEDS_DEF>
	auto SemanticAnalyzer::enum_accessor(
		const Instruction::Accessor<NEEDS_DEF>& instr,
		std::string_view rhs_ident_str,
		const TermInfo& lhs,
		TypeInfo::ID actual_lhs_type_id,
		const TypeInfo& actual_lhs_type,
		bool is_pointer
	) -> Result {
		const BaseType::Enum& lhs_type_enum = this->context.getTypeManager().getEnum(
			actual_lhs_type.baseTypeID().enumID()
		);

		const Source* enum_source = [&]() -> const Source* {
			if(lhs_type_enum.isClangType()){
				return nullptr;
			}else{
				return &this->context.getSourceManager()[lhs_type_enum.sourceID.as<Source::ID>()];
			}
		}();

		const sema::ScopeLevel::IdentID* lookup_ident = lhs_type_enum.scopeLevel->lookupIdent(rhs_ident_str);

		if(lookup_ident == nullptr){
			auto infos = evo::SmallVector<Diagnostic::Info>();

			for(const BaseType::Enum::Enumerator& enumerator : lhs_type_enum.enumerators){
				if(lhs_type_enum.getEnumeratorName(enumerator, this->context.getSourceManager()) == rhs_ident_str){
					infos.emplace_back("Note: Enum enumerators should be accessed through the type");
					infos.emplace_back(
						std::format(
							"Did you mean: `{}.{}`?",
							lhs_type_enum.getName(this->context.getSourceManager()),
							rhs_ident_str
						)
					);
					break;
				}
			}

			this->emit_error(
				Diagnostic::Code::SEMA_NO_SYMBOL_IN_SCOPE_WITH_THAT_IDENT,
				instr.infix.rhs,
				std::format("Identifier \"{}\" was not defined in this scope", rhs_ident_str),
				std::move(infos)
			);
			return Result::ERROR;
		}

	
		///////////////////////////////////
		// method

		const WaitOnSymbolProcResult wait_on_symbol_proc_result = this->wait_on_symbol_proc<NEEDS_DEF>(
			lhs_type_enum.namespacedMembers, rhs_ident_str
		);


		switch(wait_on_symbol_proc_result){
			case WaitOnSymbolProcResult::NOT_FOUND: case WaitOnSymbolProcResult::ERROR_PASSED_BY_WHEN_COND: {
				this->wait_on_symbol_proc_emit_error(
					wait_on_symbol_proc_result,
					instr.infix.rhs,
					std::format("Enum has no member named \"{}\"", rhs_ident_str)
				);
				return Result::ERROR;
			} break;

			case WaitOnSymbolProcResult::CIRCULAR_DEP_DETECTED: case WaitOnSymbolProcResult::EXISTS_BUT_ERRORED: {
				return Result::ERROR;
			} break;

			case WaitOnSymbolProcResult::NEED_TO_WAIT: {
				return Result::NEED_TO_WAIT;
			} break;

			case WaitOnSymbolProcResult::SEMAS_READY: {
				// do nothing...
			} break;
		}


		evo::Expected<TermInfo, AnalyzeExprIdentInScopeLevelError> expr_ident = 
			this->analyze_expr_ident_in_scope_level<NEEDS_DEF, false>(
				instr.rhs_ident, rhs_ident_str, *lhs_type_enum.scopeLevel, true, true, enum_source
			);


		if(expr_ident.has_value() == false){
			switch(expr_ident.error()){
				case AnalyzeExprIdentInScopeLevelError::DOESNT_EXIST: {
					evo::debugFatalBreak("Def is done, but can't find sema of symbol");
				} break;

				case AnalyzeExprIdentInScopeLevelError::NEEDS_TO_WAIT_ON_DEF: {
					evo::debugFatalBreak(
						"Sema doesn't have completed info for def despite SymbolProc saying it should"
					);
				} break;

				case AnalyzeExprIdentInScopeLevelError::ERROR_EMITTED: return Result::ERROR;
			}
		}


		const sema::FakeTermInfo::ID method_this = [&](){
			if(is_pointer){
				const TypeInfo::ID resultant_type_id = this->context.type_manager.getOrCreateTypeInfo(
					TypeInfo(actual_lhs_type.baseTypeID())
				);

				return this->context.sema_buffer.createFakeTermInfo(
					TermInfo::convertValueCategory(lhs.value_category),
					TermInfo::convertValueStage(lhs.value_stage),
					TermInfo::convertValueState(lhs.value_state),
					resultant_type_id,
					sema::Expr(this->context.sema_buffer.createDeref(lhs.getExpr(), resultant_type_id))
				);
				
			}else{
				return this->context.sema_buffer.createFakeTermInfo(
					TermInfo::convertValueCategory(lhs.value_category),
					TermInfo::convertValueStage(lhs.value_stage),
					TermInfo::convertValueState(lhs.value_state),
					actual_lhs_type_id,
					lhs.getExpr()
				);
			}
		}();

		this->return_term_info(instr.output,
			TermInfo::ValueCategory::METHOD_CALL,
			expr_ident.value().value_stage,
			TermInfo::ValueState::NOT_APPLICABLE,
			std::move(expr_ident.value().type_id),
			sema::Expr(method_this)
		);
		return Result::SUCCESS;
	}





	template<bool NEEDS_DEF>
	auto SemanticAnalyzer::array_accessor(
		const Instruction::Accessor<NEEDS_DEF>& instr,
		std::string_view rhs_ident_str,
		const TermInfo& lhs,
		TypeInfo::ID actual_lhs_type_id,
		const TypeInfo& actual_lhs_type,
		bool is_pointer
	) -> Result {
		std::ignore = is_pointer;

		const sema::FakeTermInfo::ID method_this = this->context.sema_buffer.createFakeTermInfo(
			TermInfo::convertValueCategory(lhs.value_category),
			TermInfo::convertValueStage(lhs.value_stage),
			TermInfo::convertValueState(lhs.value_state),
			actual_lhs_type_id,
			lhs.getExpr()
		);

		if(rhs_ident_str == "size"){
			const TypeInfo::ID method_type = this->context.type_manager.getOrCreateTypeInfo(
				TypeInfo(
					this->context.type_manager.getOrCreateFunction(
						BaseType::Function(
							evo::SmallVector<BaseType::Function::Param>(),
							evo::SmallVector<BaseType::Function::ReturnParam>{
								BaseType::Function::ReturnParam(std::nullopt, TypeManager::getTypeUSize())
							},
							evo::SmallVector<BaseType::Function::ReturnParam>()
						)
					)
				)
			);

			this->return_term_info(instr.output,
				TermInfo::ValueCategory::BUILTIN_TYPE_METHOD,
				TermInfo::ValueStage::CONSTEXPR,
				TermInfo::ValueState::NOT_APPLICABLE,
				TermInfo::BuiltinTypeMethod(method_type, TermInfo::BuiltinTypeMethod::Kind::ARRAY_SIZE),
				sema::Expr(method_this)
			);
			return Result::SUCCESS;

		}else if(rhs_ident_str == "dimensions"){
			const BaseType::Array& array_type =
				this->context.getTypeManager().getArray(actual_lhs_type.baseTypeID().arrayID());

			const BaseType::ID returned_array_base_type = this->context.type_manager.getOrCreateArray(
				BaseType::Array(
					TypeManager::getTypeUSize(),
					evo::SmallVector<uint64_t>{uint64_t(array_type.dimensions.size())},
					std::nullopt
				)
			);

			const TypeInfo::ID returned_array_type =
				this->context.type_manager.getOrCreateTypeInfo(TypeInfo(returned_array_base_type));

			const TypeInfo::ID method_type = this->context.type_manager.getOrCreateTypeInfo(
				TypeInfo(
					this->context.type_manager.getOrCreateFunction(
						BaseType::Function(
							evo::SmallVector<BaseType::Function::Param>(),
							evo::SmallVector<BaseType::Function::ReturnParam>{
								BaseType::Function::ReturnParam(std::nullopt, returned_array_type)
							},
							evo::SmallVector<BaseType::Function::ReturnParam>()
						)
					)
				)
			);

			this->return_term_info(instr.output,
				TermInfo::ValueCategory::BUILTIN_TYPE_METHOD,
				TermInfo::ValueStage::CONSTEXPR,
				TermInfo::ValueState::NOT_APPLICABLE,
				TermInfo::BuiltinTypeMethod(method_type, TermInfo::BuiltinTypeMethod::Kind::ARRAY_DIMENSIONS),
				sema::Expr(method_this)
			);
			return Result::SUCCESS;
		}



		this->emit_error(
			Diagnostic::Code::SEMA_ARRAY_DOESNT_HAVE_MEMBER,
			instr.infix.rhs,
			std::format("Array has no member named \"{}\"", rhs_ident_str)
		);
		return Result::ERROR;
	}



	template<bool NEEDS_DEF>
	auto SemanticAnalyzer::array_ref_accessor(
		const Instruction::Accessor<NEEDS_DEF>& instr,
		std::string_view rhs_ident_str,
		const TermInfo& lhs,
		TypeInfo::ID actual_lhs_type_id,
		const TypeInfo& actual_lhs_type,
		bool is_pointer
	) -> Result {
		const TypeInfo::ID array_ref_target_type = [&](){
			if(is_pointer){
				return this->context.type_manager.getOrCreateTypeInfo(actual_lhs_type.copyWithPoppedQualifier());
			}else{
				return actual_lhs_type_id;
			}
		}();

		const sema::Expr method_this = [&](){
			sema::Expr lhs_expr = lhs.getExpr();
			if(is_pointer){
				lhs_expr = sema::Expr(this->context.sema_buffer.createDeref(lhs_expr, actual_lhs_type_id));
			}

			return sema::Expr(
				this->context.sema_buffer.createFakeTermInfo(
					TermInfo::convertValueCategory(lhs.value_category),
					TermInfo::convertValueStage(lhs.value_stage),
					TermInfo::convertValueState(lhs.value_state),
					array_ref_target_type,
					lhs_expr
				)
			);
		}();

		if(rhs_ident_str == "size"){
			const TypeInfo::ID method_type = this->context.type_manager.getOrCreateTypeInfo(
				TypeInfo(
					this->context.type_manager.getOrCreateFunction(
						BaseType::Function(
							evo::SmallVector<BaseType::Function::Param>(),
							evo::SmallVector<BaseType::Function::ReturnParam>{
								BaseType::Function::ReturnParam(std::nullopt, TypeManager::getTypeUSize())
							},
							evo::SmallVector<BaseType::Function::ReturnParam>()
						)
					)
				)
			);

			this->return_term_info(instr.output,
				TermInfo::ValueCategory::BUILTIN_TYPE_METHOD,
				TermInfo::ValueStage::CONSTEXPR,
				TermInfo::ValueState::NOT_APPLICABLE,
				TermInfo::BuiltinTypeMethod(method_type, TermInfo::BuiltinTypeMethod::Kind::ARRAY_REF_SIZE),
				sema::Expr(method_this)
			);
			return Result::SUCCESS;

		}else if(rhs_ident_str == "dimensions"){
			const BaseType::ArrayRef& array_ref_type =
				this->context.getTypeManager().getArrayRef(actual_lhs_type.baseTypeID().arrayRefID());

			const BaseType::ID returned_array_base_type = this->context.type_manager.getOrCreateArray(
				BaseType::Array(
					TypeManager::getTypeUSize(),
					evo::SmallVector<uint64_t>{uint64_t(array_ref_type.dimensions.size())},
					std::nullopt
				)
			);

			const TypeInfo::ID returned_array_type =
				this->context.type_manager.getOrCreateTypeInfo(TypeInfo(returned_array_base_type));

			const TypeInfo::ID method_type = this->context.type_manager.getOrCreateTypeInfo(
				TypeInfo(
					this->context.type_manager.getOrCreateFunction(
						BaseType::Function(
							evo::SmallVector<BaseType::Function::Param>(),
							evo::SmallVector<BaseType::Function::ReturnParam>{
								BaseType::Function::ReturnParam(std::nullopt, returned_array_type)
							},
							evo::SmallVector<BaseType::Function::ReturnParam>()
						)
					)
				)
			);

			this->return_term_info(instr.output,
				TermInfo::ValueCategory::BUILTIN_TYPE_METHOD,
				TermInfo::ValueStage::CONSTEXPR,
				TermInfo::ValueState::NOT_APPLICABLE,
				TermInfo::BuiltinTypeMethod(method_type, TermInfo::BuiltinTypeMethod::Kind::ARRAY_REF_DIMENSIONS),
				sema::Expr(method_this)
			);
			return Result::SUCCESS;
		}



		this->emit_error(
			Diagnostic::Code::SEMA_ARRAY_DOESNT_HAVE_MEMBER,
			instr.infix.rhs,
			std::format("Array reference has no member named \"{}\"", rhs_ident_str)
		);
		return Result::ERROR;
	}





	//////////////////////////////////////////////////////////////////////
	// scope

	auto SemanticAnalyzer::get_current_scope_level() const -> sema::ScopeLevel& {
		return this->context.sema_buffer.scope_manager.getLevel(this->scope.getCurrentLevel());
	}


	auto SemanticAnalyzer::push_scope_level(sema::StmtBlock* stmt_block) -> void {
		if(this->scope.inObjectScope()){
			this->get_current_scope_level().addSubScope();
		}
		this->scope.pushLevel(this->context.sema_buffer.scope_manager.createLevel(stmt_block));
	}

	auto SemanticAnalyzer::push_scope_level(
		sema::StmtBlock& stmt_block, Token::ID label, sema::ScopeLevel::LabelNode label_node
	) -> void {
		if(this->scope.inObjectScope()){
			this->get_current_scope_level().addSubScope();
		}
		this->scope.pushLevel(this->context.sema_buffer.scope_manager.createLevel(stmt_block, label, label_node));
	}

	auto SemanticAnalyzer::push_scope_level(sema::StmtBlock* stmt_block, const auto& object_scope_id) -> void {
		this->get_current_scope_level().addSubScope();
		this->scope.pushLevel(this->context.sema_buffer.scope_manager.createLevel(stmt_block), object_scope_id);
	}


	template<SemanticAnalyzer::PopScopeLevelKind POP_SCOPE_LEVEL_KIND>
	EVO_NODISCARD auto SemanticAnalyzer::pop_scope_level() -> evo::Result<> {
		if constexpr(POP_SCOPE_LEVEL_KIND == PopScopeLevelKind::SYMBOL_END){
			this->scope.popLevel();

		}else{
			sema::ScopeLevel& current_scope_level = this->get_current_scope_level();
			const bool current_scope_is_terminated = current_scope_level.isTerminated();

			if(
				current_scope_level.hasStmtBlock()
				&& current_scope_level.stmtBlock().isTerminated() == false
				&& current_scope_is_terminated
			){
				current_scope_level.stmtBlock().setTerminated();

			}else if(current_scope_is_terminated == false && this->scope.inObjectScope()){
				sema::ScopeLevel& parent_scope_level = 
					this->context.sema_buffer.scope_manager.getLevel(*std::next(this->scope.begin()));

				for(const auto& [value_state_id, value_state_info] : current_scope_level.getValueStateInfos()){
					if(value_state_info.info.is<sema::ScopeLevel::ValueStateInfo::DeclInfo>()){ continue; }

					const evo::Expected<void, sema::ScopeLevel::ValueStateID> set_value_state_res =
						parent_scope_level.setIdentValueStateFromSubScope(value_state_id, value_state_info.state);


					if(set_value_state_res.has_value() == false){
						this->emit_error(
							Diagnostic::Code::SEMA_CANT_DETERMINE_VALUE_STATE,
							set_value_state_res.error(),
							"Can't determine value state in sub-scope"
						); // TODO(FUTURE): emit where the error comes from
						return evo::resultError;
					}
				}	
			}


			if constexpr(POP_SCOPE_LEVEL_KIND == PopScopeLevelKind::LABEL_TERMINATE){
				const bool current_scope_is_label_terminated = current_scope_level.isLabelTerminated();

				this->scope.popLevel(); // `current_scope_level` is now invalid

				if(
					current_scope_is_terminated
					&& this->scope.inObjectScope()
					&& !this->scope.inObjectMainScope()
					&& current_scope_is_label_terminated == false

				){
					this->get_current_scope_level().setSubScopeTerminated();
				}

			}else if constexpr(POP_SCOPE_LEVEL_KIND == PopScopeLevelKind::NORMAL){
				this->scope.popLevel(); // `current_scope_level` is now invalid

				if(
					current_scope_is_terminated
					&& this->scope.inObjectScope()
					&& !this->scope.inObjectMainScope()
				){
					this->get_current_scope_level().setSubScopeTerminated();
				}
			}
		}

		return evo::Result<>();
	}


	template<SemanticAnalyzer::AutoDeleteMode AUTO_DELETE_MODE>
	auto SemanticAnalyzer::add_auto_delete_calls() -> void {
		sema::ScopeLevel& current_scope_level = this->get_current_scope_level();

		struct ValueStateData{
			size_t creation_index;
			TypeInfo::ID type_info_id;
			sema::Expr expr;
		};

		auto value_state_datas = evo::SmallVector<ValueStateData, 8>();

		for(const auto [value_state_id, value_state_info] : current_scope_level.getValueStateInfos()){
			if(value_state_info.info.is<sema::ScopeLevel::ValueStateInfo::ModifyInfo>()){ continue; }
			if(value_state_info.state != sema::ScopeLevel::ValueState::INIT){ continue; }

			auto expr = std::optional<sema::Expr>();
			auto type_info_id = std::optional<TypeInfo::ID>();

			if(value_state_id.is<sema::Var::ID>()){
				const sema::Var& sema_var =
					this->context.getSemaBuffer().getVar(value_state_id.as<sema::Var::ID>());
				if(sema_var.kind == AST::VarDef::Kind::DEF){ continue; }

				value_state_datas.emplace_back(
					value_state_info.creation_index, *sema_var.typeID, sema::Expr(value_state_id.as<sema::Var::ID>())
				);

			}else if(value_state_id.is<sema::ReturnParam::ID>()){
				if constexpr(AUTO_DELETE_MODE == AutoDeleteMode::NORMAL){
					const sema::ReturnParam& return_param = 
						this->context.getSemaBuffer().getReturnParam(value_state_id.as<sema::ReturnParam::ID>());

					const sema::Func& current_func = this->get_current_func();
					const BaseType::Function& current_func_type = 
						this->context.getTypeManager().getFunction(current_func.typeID);

					value_state_datas.emplace_back(
						value_state_info.creation_index,
						current_func_type.returnParams[return_param.index].typeID.asTypeID(),
						sema::Expr(value_state_id.as<sema::ErrorReturnParam::ID>())
					);

				}else{
					continue;
				}

			}else if(value_state_id.is<sema::ErrorReturnParam::ID>()){
				if constexpr(AUTO_DELETE_MODE == AutoDeleteMode::RETURN){
					const sema::ErrorReturnParam& error_return_param = this->context
						.getSemaBuffer()
						.getErrorReturnParam(value_state_id.as<sema::ErrorReturnParam::ID>());

					const sema::Func& current_func = this->get_current_func();
					const BaseType::Function& current_func_type = 
						this->context.getTypeManager().getFunction(current_func.typeID);

					value_state_datas.emplace_back(
						value_state_info.creation_index,
						current_func_type.returnParams[error_return_param.index].typeID.asTypeID(),
						sema::Expr(value_state_id.as<sema::ErrorReturnParam::ID>())
					);
					
				}else{
					continue;
				}

			}else if(value_state_id.is<sema::BlockExprOutput::ID>()){
				if constexpr(AUTO_DELETE_MODE == AutoDeleteMode::RETURN || AUTO_DELETE_MODE == AutoDeleteMode::ERROR){
					const sema::BlockExprOutput& block_expr_output = this->context
						.getSemaBuffer()
						.getBlockExprOutput(value_state_id.as<sema::BlockExprOutput::ID>());

					value_state_datas.emplace_back(
						value_state_info.creation_index,
						block_expr_output.typeID,
						sema::Expr(value_state_id.as<sema::BlockExprOutput::ID>())
					);

				}else{
					continue;
				}

			}else if(value_state_id.is<sema::OpDeleteThisAccessorValueStateID>()){
				const BaseType::Struct::ID delete_struct_type_id = 
					this->scope.getCurrentTypeScopeIfExists()->as<BaseType::Struct::ID>();

				const BaseType::Struct& delete_struct_type = this->context.getTypeManager().getStruct(
					delete_struct_type_id
				);

				for(const BaseType::Struct::MemberVar& member_var : delete_struct_type.memberVars){
					const uint32_t member_var_abi_index = 
						value_state_id.as<sema::OpDeleteThisAccessorValueStateID>().index;

					if(&member_var == delete_struct_type.memberVarsABI[size_t(member_var_abi_index)]){
						value_state_datas.emplace_back(
							value_state_info.creation_index,
							member_var.typeID,
							sema::Expr(
								this->context.sema_buffer.createAccessor(
									sema::Expr(this->context.sema_buffer.createParam(0, 0)),
									this->context.type_manager.getOrCreateTypeInfo(
										TypeInfo(BaseType::ID(delete_struct_type_id))
									),
									member_var_abi_index
								)
							)
						);

						break;
					}
				}

			}else{
				continue;
			}
		}

		// make sure they happen in the correct order
		std::ranges::sort(value_state_datas, [&](const ValueStateData& lhs, const ValueStateData& rhs) -> bool {
			return lhs.creation_index < rhs.creation_index;
		});


		for(const ValueStateData& value_state_data : value_state_datas){
			if(this->context.getTypeManager().isTriviallyDeletable(value_state_data.type_info_id)){ continue; }


			const sema::Defer::ID defer_id = this->context.sema_buffer.createDefer(false);
			current_scope_level.stmtBlock().emplace_back(sema::Stmt(defer_id));

			sema::Defer& defer_stmt = this->context.sema_buffer.defers[defer_id];

			this->get_special_member_stmt_dependents<SpecialMemberKind::DELETE>(
				value_state_data.type_info_id,
				this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>().dependent_funcs
			);
			defer_stmt.block.emplace_back(
				this->context.sema_buffer.createDelete(value_state_data.expr, value_state_data.type_info_id)
			);
		}
	}


	template<SemanticAnalyzer::SpecialMemberKind SPECIAL_MEMBER_KIND>
	auto SemanticAnalyzer::get_special_member_stmt_dependents(
		TypeInfo::ID type_info_id, std::unordered_set<sema::Func::ID>& dependent_funcs
	) -> void {
		const TypeInfo& type_info = this->context.getTypeManager().getTypeInfo(type_info_id);

		if(type_info.qualifiers().size() > 0){
			if(type_info.qualifiers().back().isPtr == false && type_info.qualifiers().back().isOptional){
				const TypeInfo::ID optional_held_type_id = this->context.type_manager.getOrCreateTypeInfo(
					type_info.copyWithPoppedQualifier()
				);

				this->get_special_member_stmt_dependents<SPECIAL_MEMBER_KIND>(optional_held_type_id, dependent_funcs);
			}

		}else{
			switch(type_info.baseTypeID().kind()){
				case BaseType::Kind::DUMMY: {
					evo::debugFatalBreak("Not a valid type");
				} break;

				case BaseType::Kind::PRIMITIVE: {
					// trivially deletable
				} break;

				case BaseType::Kind::FUNCTION: {
					// trivially deletable
				} break;

				case BaseType::Kind::ARRAY: {
					const BaseType::Array& array_type = 
						this->context.getTypeManager().getArray(type_info.baseTypeID().arrayID());

					this->get_special_member_stmt_dependents<SPECIAL_MEMBER_KIND>(
						array_type.elementTypeID, dependent_funcs
					);
				} break;

				case BaseType::Kind::ARRAY_REF: {
					// trivially deletable
				} break;

				case BaseType::Kind::ALIAS: {
					const BaseType::Alias& alias_type = 
						this->context.getTypeManager().getAlias(type_info.baseTypeID().aliasID());

					this->get_special_member_stmt_dependents<SPECIAL_MEMBER_KIND>(
						*alias_type.aliasedType.load(), dependent_funcs
					);
				} break;

				case BaseType::Kind::DISTINCT_ALIAS: {
					const BaseType::DistinctAlias& distinct_alias_type = 
						this->context.getTypeManager().getDistinctAlias(type_info.baseTypeID().distinctAliasID());

					this->get_special_member_stmt_dependents<SPECIAL_MEMBER_KIND>(
						*distinct_alias_type.underlyingType.load(), dependent_funcs
					);
				} break;

				case BaseType::Kind::STRUCT: {
					const BaseType::Struct& struct_type = 
						this->context.getTypeManager().getStruct(type_info.baseTypeID().structID());

					if constexpr(SPECIAL_MEMBER_KIND == SpecialMemberKind::DELETE){
						const std::optional<sema::FuncID> delete_overload = struct_type.deleteOverload.load();

						if(delete_overload.has_value()){
							dependent_funcs.emplace(*delete_overload);
						}

					}else if constexpr(SPECIAL_MEMBER_KIND == SpecialMemberKind::COPY){
						const BaseType::Struct::DeletableOverload copy_overload = struct_type.copyInitOverload.load();

						if(copy_overload.funcID.has_value()){
							dependent_funcs.emplace(*copy_overload.funcID);
						}

					}else if constexpr(SPECIAL_MEMBER_KIND == SpecialMemberKind::MOVE){
						const BaseType::Struct::DeletableOverload move_overload = struct_type.moveInitOverload.load();

						if(move_overload.funcID.has_value()){
							dependent_funcs.emplace(*move_overload.funcID);
						}

					}else{
						static_assert(false, "Unknown special member kind");
					}
				} break;

				case BaseType::Kind::STRUCT_TEMPLATE: {
					evo::debugFatalBreak("Not a valid type to for value");
				} break;

				case BaseType::Kind::UNION: {
					const BaseType::Union& union_type = 
						this->context.getTypeManager().getUnion(type_info.baseTypeID().unionID());

					if(union_type.isUntagged == false){
						for(const BaseType::Union::Field& field : union_type.fields){
							if(field.typeID.isVoid()){ continue; }

							this->get_special_member_stmt_dependents<SPECIAL_MEMBER_KIND>(
								field.typeID.asTypeID(), dependent_funcs
							);
						}
					}
				} break;

				case BaseType::Kind::TYPE_DEDUCER: {
					evo::debugFatalBreak("Not a valid type to for value");
				} break;

				case BaseType::Kind::INTERFACE: {
					evo::debugFatalBreak("Not a valid type to for value");
				} break;
			}
		}
	}





	auto SemanticAnalyzer::currently_in_func() const -> bool {
		return this->scope.inObjectScope() && this->scope.getCurrentObjectScope().is<sema::Func::ID>();
	}

	auto SemanticAnalyzer::get_current_func() -> sema::Func& {
		return this->context.sema_buffer.funcs[this->scope.getCurrentObjectScope().as<sema::Func::ID>()];
	}

	auto SemanticAnalyzer::get_current_func() const -> const sema::Func& {
		return this->context.getSemaBuffer().getFunc(this->scope.getCurrentObjectScope().as<sema::Func::ID>());
	}




	//////////////////////////////////////////////////////////////////////
	// misc

	template<bool NEEDS_DEF>
	auto SemanticAnalyzer::lookup_ident_impl(Token::ID ident) -> evo::Expected<TermInfo, Result> {
		const std::string_view ident_str = this->source.getTokenBuffer()[ident].getString();

		///////////////////////////////////
		// find symbol procs

		auto symbol_proc_namespaces = evo::SmallVector<const SymbolProc::Namespace*>();

		SymbolProc* parent_symbol = this->symbol_proc.parent;
		while(parent_symbol != nullptr){
			if(parent_symbol->extra_info.is<SymbolProc::StructInfo>()){
				symbol_proc_namespaces.emplace_back(
					&parent_symbol->extra_info.as<SymbolProc::StructInfo>().member_symbols
				);
			}
			
			parent_symbol = parent_symbol->parent;
		}
		symbol_proc_namespaces.emplace_back(&this->source.global_symbol_procs);


		const WaitOnSymbolProcResult wait_on_symbol_proc_result = this->wait_on_symbol_proc<NEEDS_DEF>(
			symbol_proc_namespaces, ident_str
		);

		switch(wait_on_symbol_proc_result){
			case WaitOnSymbolProcResult::NOT_FOUND: {
				// Do nothing as it may be an ident might not have a symbol proc (such as template param)
			} break;

			case WaitOnSymbolProcResult::ERROR_PASSED_BY_WHEN_COND: {
				this->wait_on_symbol_proc_emit_error(
					wait_on_symbol_proc_result,
					ident,
					std::format("Identifier \"{}\" was not defined in this scope", ident_str)
				);
				return evo::Unexpected(Result::ERROR);
			} break;

			case WaitOnSymbolProcResult::CIRCULAR_DEP_DETECTED: case WaitOnSymbolProcResult::EXISTS_BUT_ERRORED: {
				return evo::Unexpected(Result::ERROR);
			} break;

			case WaitOnSymbolProcResult::NEED_TO_WAIT: {
				return evo::Unexpected(Result::NEED_TO_WAIT);
			} break;

			case WaitOnSymbolProcResult::SEMAS_READY: {
				// do nothing...
			} break;
		}



		///////////////////////////////////
		// find sema

		for(size_t i = this->scope.size() - 1; sema::ScopeLevel::ID scope_level_id : this->scope){
			const evo::Expected<TermInfo, AnalyzeExprIdentInScopeLevelError> scope_level_lookup = 
				this->analyze_expr_ident_in_scope_level<NEEDS_DEF, false>(
					ident,
					ident_str,
					this->context.sema_buffer.scope_manager.getLevel(scope_level_id),
					i >= this->scope.getCurrentObjectScopeIndex() || i == 0,
					i == 0,
					nullptr
				);

			if(scope_level_lookup.has_value()){ return scope_level_lookup.value(); }

			switch(scope_level_lookup.error()){
				case AnalyzeExprIdentInScopeLevelError::DOESNT_EXIST: {
					// continue...
				} break;

				case AnalyzeExprIdentInScopeLevelError::NEEDS_TO_WAIT_ON_DEF: {
					evo::debugFatalBreak("SymbolProc said done, sema disagreed");
				} break;

				case AnalyzeExprIdentInScopeLevelError::ERROR_EMITTED: {
					return evo::Unexpected(Result::ERROR);
				} break;
			}

			i -= 1;
		}


		///////////////////////////////////
		// look in template decl instantiation types

		const evo::Result<std::optional<TypeInfo::VoidableID>> template_decl_instantiation = 
			this->scope.lookupTemplateDeclInstantiationType(ident_str);
		if(template_decl_instantiation.isSuccess()){
			if(template_decl_instantiation.value().has_value()){
				return TermInfo(TermInfo::ValueCategory::TYPE, template_decl_instantiation.value().value());
			}else{
				return TermInfo(
					TermInfo::ValueCategory::TEMPLATE_DECL_INSTANTIATION_TYPE, TermInfo::TemplateDeclInstantiationType()
				);
			}
		}


		///////////////////////////////////
		// didn't find identifier

		this->wait_on_symbol_proc_emit_error(
			wait_on_symbol_proc_result, ident, std::format("Identifier \"{}\" was not defined in this scope", ident_str)
		);
		return evo::Unexpected(Result::ERROR);
	}




	template<bool NEEDS_DEF, bool PUB_REQUIRED>
	auto SemanticAnalyzer::analyze_expr_ident_in_scope_level(
		const Token::ID& ident,
		std::string_view ident_str,
		const sema::ScopeLevel& scope_level,
		bool variables_in_scope, // TODO(FUTURE): make this template argument?
		bool is_global_scope, // TODO(FUTURE): make this template argumnet?
		const Source* source_module
	) -> evo::Expected<TermInfo, AnalyzeExprIdentInScopeLevelError> {
		if constexpr(PUB_REQUIRED){
			evo::debugAssert(variables_in_scope, "IF `PUB_REQUIRED`, `variables_in_scope` should be true");
			evo::debugAssert(is_global_scope, "IF `PUB_REQUIRED`, `is_global_scope` should be true");
		}

		const sema::ScopeLevel::IdentID* ident_id_lookup = scope_level.lookupIdent(ident_str);
		if(ident_id_lookup == nullptr){
			return evo::Unexpected(AnalyzeExprIdentInScopeLevelError::DOESNT_EXIST);
		}


		using ReturnType = evo::Expected<TermInfo, AnalyzeExprIdentInScopeLevelError>;

		return ident_id_lookup->visit([&](const auto& ident_id) -> ReturnType {
			using IdentIDType = std::decay_t<decltype(ident_id)>;


			if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::FuncOverloadList>()){
				return ReturnType(TermInfo(TermInfo::ValueCategory::FUNCTION, ident_id));

			}else if constexpr(std::is_same<IdentIDType, sema::Var::ID>()){
				if(!variables_in_scope){
					// TODO(FUTURE): better messaging
					this->emit_error(
						Diagnostic::Code::SEMA_IDENT_NOT_IN_SCOPE,
						ident,
						std::format("Variable \"{}\" is not accessable in this scope", ident_str),
						Diagnostic::Info(
							"Local variables, parameters, and members cannot be accessed inside a sub-object scope. "
								"Defined here:",
							this->get_location(ident_id)
						)
					);
					return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::ERROR_EMITTED));
				}

				const sema::Var& sema_var = this->context.getSemaBuffer().getVar(ident_id);


				using ValueCategory = TermInfo::ValueCategory;
				using ValueStage = TermInfo::ValueStage;
				using ValueState = TermInfo::ValueState;

				switch(sema_var.kind){
					case AST::VarDef::Kind::VAR: {
						return ReturnType(TermInfo(
							ValueCategory::CONCRETE_MUT,
							this->get_current_func().isConstexpr ? ValueStage::COMPTIME : ValueStage::RUNTIME,
							this->get_ident_value_state(ident_id),
							*sema_var.typeID,
							sema::Expr(ident_id)
						));
					} break;

					case AST::VarDef::Kind::CONST: {
						return ReturnType(TermInfo(
							ValueCategory::CONCRETE_CONST,
							this->get_current_func().isConstexpr ? ValueStage::COMPTIME : ValueStage::RUNTIME,
							this->get_ident_value_state(ident_id),
							*sema_var.typeID,
							sema::Expr(ident_id)
						));
					} break;

					case AST::VarDef::Kind::DEF: {
						if(sema_var.typeID.has_value()){
							return ReturnType(TermInfo(
								ValueCategory::EPHEMERAL,
								ValueStage::CONSTEXPR,
								ValueState::NOT_APPLICABLE,
								*sema_var.typeID,
								sema_var.expr
							));
						}else{
							return ReturnType(TermInfo(
								ValueCategory::EPHEMERAL_FLUID,
								ValueStage::CONSTEXPR,
								ValueState::NOT_APPLICABLE,
								TermInfo::FluidType{},
								sema_var.expr
							));
						}
					} break;
				}

				evo::debugFatalBreak("Unknown or unsupported AST::VarDef::Kind");

			}else if constexpr(std::is_same<IdentIDType, sema::GlobalVar::ID>()){
				const sema::GlobalVar& sema_var = this->context.getSemaBuffer().getGlobalVar(ident_id);

				if constexpr(PUB_REQUIRED){
					if(sema_var.isPub == false){
						this->emit_error(
							Diagnostic::Code::SEMA_SYMBOL_NOT_PUB,
							ident,
							std::format("Variable \"{}\" does not have the #pub attribute", ident_str),
							Diagnostic::Info("Variable defined here:", this->get_location(ident_id))
						);
						return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::ERROR_EMITTED));
					}

				}

				using ValueCategory = TermInfo::ValueCategory;
				using ValueStage = TermInfo::ValueStage;
				using ValueState = TermInfo::ValueState;

				switch(sema_var.kind){
					case AST::VarDef::Kind::VAR: {
						if constexpr(NEEDS_DEF){
							if(sema_var.expr.load().has_value() == false){
								return ReturnType(
									evo::Unexpected(AnalyzeExprIdentInScopeLevelError::NEEDS_TO_WAIT_ON_DEF)
								);
							}
						}

						const ValueStage value_stage = [&](){
							if(is_global_scope){ return ValueStage::RUNTIME; }

							if(this->currently_in_func() == false){
								return ValueStage::RUNTIME;
							}
							
							if(this->get_current_func().isConstexpr){
								return ValueStage::COMPTIME;
							}else{
								return ValueStage::RUNTIME;
							}
						}();
						
						return ReturnType(TermInfo(
							ValueCategory::CONCRETE_MUT,
							value_stage,
							ValueState::NOT_APPLICABLE, 
							*sema_var.typeID,
							sema::Expr(ident_id)
						));
					} break;

					case AST::VarDef::Kind::CONST: {
						if constexpr(NEEDS_DEF){
							if(sema_var.expr.load().has_value() == false){
								return ReturnType(
									evo::Unexpected(AnalyzeExprIdentInScopeLevelError::NEEDS_TO_WAIT_ON_DEF)
								);
							}
						}

						const ValueStage value_stage = [&](){
							if(is_global_scope){ return ValueStage::COMPTIME; }

							if(this->currently_in_func() == false){
								return ValueStage::COMPTIME;
							}

							if(this->get_current_func().isConstexpr){
								return ValueStage::COMPTIME;
							}else{
								return ValueStage::RUNTIME;
							}
						}();



						if(this->currently_in_func() && this->get_current_func().isConstexpr){
							this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>().dependent_vars.emplace(ident_id);
						}

						return ReturnType(TermInfo(
							ValueCategory::CONCRETE_CONST,
							value_stage,
							ValueState::NOT_APPLICABLE,
							*sema_var.typeID,
							sema::Expr(ident_id)
						));
					} break;

					case AST::VarDef::Kind::DEF: {
						if(sema_var.typeID.has_value()){
							return ReturnType(TermInfo(
								ValueCategory::EPHEMERAL,
								ValueStage::CONSTEXPR,
								ValueState::NOT_APPLICABLE,
								*sema_var.typeID,
								*sema_var.expr.load()
							));
						}else{
							return ReturnType(TermInfo(
								ValueCategory::EPHEMERAL_FLUID,
								ValueStage::CONSTEXPR,
								ValueState::NOT_APPLICABLE,
								TermInfo::FluidType{},
								*sema_var.expr.load()
							));
						}
					};
				}

				evo::debugFatalBreak("Unknown or unsupported AST::VarDef::Kind");

			}else if constexpr(std::is_same<IdentIDType, sema::Param::ID>()){
				const sema::Func& current_func = this->get_current_func();
				const BaseType::Function& current_func_type = 
					this->context.getTypeManager().getFunction(current_func.typeID);
				const size_t param_index = size_t(this->context.getSemaBuffer().getParam(ident_id).index);
				const BaseType::Function::Param& param = current_func_type.params[param_index];

				const TermInfo::ValueCategory value_category = [&](){
					switch(param.kind){
						case BaseType::Function::Param::Kind::READ: return TermInfo::ValueCategory::CONCRETE_CONST;
						case BaseType::Function::Param::Kind::MUT:  return TermInfo::ValueCategory::CONCRETE_MUT;
						case BaseType::Function::Param::Kind::IN:   return TermInfo::ValueCategory::FORWARDABLE;
						case BaseType::Function::Param::Kind::C:    evo::debugFatalBreak("invalid here");
					}

					evo::unreachable();
				}();


				const TypeInfo::ID param_type = [&](){
					const SymbolProc::FuncInfo& func_info = this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>();

					if(func_info.instantiation == nullptr){
						return param.typeID;

					}else{
						if(
							this->context.getTypeManager().getTypeInfo(param.typeID).isInterface()
							|| this->context.getTypeManager().isTypeDeducer(param.typeID)
						){
							return *func_info.instantiation_param_arg_types[param_index];
						}else{
							return param.typeID;	
						}
					}
				}();


				return ReturnType(
					TermInfo(
						value_category,
						current_func.isConstexpr ? TermInfo::ValueStage::COMPTIME : TermInfo::ValueStage::RUNTIME,
						this->get_ident_value_state(ident_id),
						param.typeID,
						sema::Expr(ident_id)
					)
				);

			}else if constexpr(std::is_same<IdentIDType, sema::ReturnParam::ID>()){
				const sema::Func& current_func = this->get_current_func();
				const BaseType::Function& current_func_type = 
					this->context.getTypeManager().getFunction(current_func.typeID);
				const BaseType::Function::ReturnParam& return_param = current_func_type.returnParams[
					this->context.getSemaBuffer().getReturnParam(ident_id).index
				];


				return ReturnType(
					TermInfo(
						TermInfo::ValueCategory::CONCRETE_MUT,
						current_func.isConstexpr ? TermInfo::ValueStage::COMPTIME : TermInfo::ValueStage::RUNTIME,
						this->get_ident_value_state(ident_id),
						return_param.typeID.asTypeID(),
						sema::Expr(ident_id)
					)
				);

			}else if constexpr(std::is_same<IdentIDType, sema::ErrorReturnParam::ID>()){
				const sema::Func& current_func = this->get_current_func();
				const BaseType::Function& current_func_type = 
					this->context.getTypeManager().getFunction(current_func.typeID);
				const BaseType::Function::ReturnParam& error_param = current_func_type.errorParams[
					this->context.getSemaBuffer().getErrorReturnParam(ident_id).index
				];

				return ReturnType(
					TermInfo(
						TermInfo::ValueCategory::CONCRETE_MUT,
						current_func.isConstexpr ? TermInfo::ValueStage::COMPTIME : TermInfo::ValueStage::RUNTIME,
						this->get_ident_value_state(ident_id),
						error_param.typeID.asTypeID(),
						sema::Expr(ident_id)
					)
				);

			}else if constexpr(std::is_same<IdentIDType, sema::BlockExprOutput::ID>()){
				const sema::Func& current_func = this->get_current_func();

				const sema::BlockExprOutput& sema_block_expr_output =
					this->context.getSemaBuffer().getBlockExprOutput(ident_id);

				const sema::BlockExpr::ID block_expr_id = scope_level.getLabelNode().as<sema::BlockExpr::ID>();
				const sema::BlockExpr& block_expr = this->context.getSemaBuffer().getBlockExpr(block_expr_id);

				return ReturnType(
					TermInfo(
						TermInfo::ValueCategory::CONCRETE_MUT,
						current_func.isConstexpr ? TermInfo::ValueStage::COMPTIME : TermInfo::ValueStage::RUNTIME,
						this->get_ident_value_state(ident_id),
						block_expr.outputs[sema_block_expr_output.index].typeID,
						sema::Expr(ident_id)
					)
				);

			}else if constexpr(std::is_same<IdentIDType, sema::ExceptParam::ID>()){
				const sema::Func& current_func = this->get_current_func();

				const sema::ExceptParam& except_param = this->context.getSemaBuffer().getExceptParam(ident_id);
				return ReturnType(
					TermInfo(
						TermInfo::ValueCategory::CONCRETE_MUT,
						current_func.isConstexpr ? TermInfo::ValueStage::COMPTIME : TermInfo::ValueStage::RUNTIME,
						this->get_ident_value_state(ident_id),
						except_param.typeID,
						sema::Expr(ident_id)
					)
				);

			}else if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::ModuleInfo>()){
				if constexpr(PUB_REQUIRED){
					if(ident_id.isPub == false){
						this->emit_error(
							Diagnostic::Code::SEMA_SYMBOL_NOT_PUB,
							ident_id,
							std::format("Identifier \"{}\" does not have the #pub attribute", ident_str),
							Diagnostic::Info(
								"Defined here:",
								Diagnostic::Location::get(ident_id.tokenID, *source_module)
							)
						);
						return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::ERROR_EMITTED));
					}
				}

				return ReturnType(
					TermInfo(
						TermInfo::ValueCategory::MODULE,
						TermInfo::ValueStage::CONSTEXPR,
						TermInfo::ValueState::NOT_APPLICABLE,
						ident_id.sourceID,
						sema::Expr::createModuleIdent(ident_id.tokenID)
					)
				);

			}else if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::ClangModuleInfo>()){
				if constexpr(PUB_REQUIRED){
					if(ident_id.isPub == false){
						this->emit_error(
							Diagnostic::Code::SEMA_SYMBOL_NOT_PUB,
							ident_id,
							std::format("Identifier \"{}\" does not have the #pub attribute", ident_str),
							Diagnostic::Info(
								"Defined here:",
								Diagnostic::Location::get(ident_id.tokenID, *source_module)
							)
						);
						return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::ERROR_EMITTED));
					}
				}

				return ReturnType(
					TermInfo(
						TermInfo::ValueCategory::CLANG_MODULE,
						TermInfo::ValueStage::CONSTEXPR,
						TermInfo::ValueState::NOT_APPLICABLE,
						ident_id.clangSourceID,
						sema::Expr::createModuleIdent(ident_id.tokenID)
					)
				);

			}else if constexpr(std::is_same<IdentIDType, BaseType::Alias::ID>()){
				const BaseType::Alias& alias = this->context.getTypeManager().getAlias(ident_id);

				if constexpr(NEEDS_DEF){
					if(alias.defCompleted() == false){
						return ReturnType(
							evo::Unexpected(AnalyzeExprIdentInScopeLevelError::NEEDS_TO_WAIT_ON_DEF)
						);
					}
				}

				if constexpr(PUB_REQUIRED){
					if(alias.isPub == false){
						this->emit_error(
							Diagnostic::Code::SEMA_SYMBOL_NOT_PUB,
							ident,
							std::format("Alias \"{}\" does not have the #pub attribute", ident_str),
							Diagnostic::Info(
								"Alias declared here:",
								Diagnostic::Location::get(ident_id, this->context)
							)
						);
						return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::ERROR_EMITTED));
					}
				}

				return ReturnType(
					TermInfo(
						TermInfo::ValueCategory::TYPE,
						TypeInfo::VoidableID(
							this->context.type_manager.getOrCreateTypeInfo(TypeInfo(BaseType::ID(ident_id)))
						)
					)
				);

			}else if constexpr(std::is_same<IdentIDType, BaseType::DistinctAlias::ID>()){
				this->emit_error(
					Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
					ident,
					"Using distinct aliases is currently unimplemented"
				);
				return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::ERROR_EMITTED));

			}else if constexpr(std::is_same<IdentIDType, BaseType::Struct::ID>()){
				const BaseType::Struct& struct_info = this->context.getTypeManager().getStruct(ident_id);

				if constexpr(NEEDS_DEF){
					if(struct_info.defCompleted == false){
						return ReturnType(
							evo::Unexpected(AnalyzeExprIdentInScopeLevelError::NEEDS_TO_WAIT_ON_DEF)
						);
					}
				}

				if constexpr(PUB_REQUIRED){
					if(struct_info.isPub == false){
						this->emit_error(
							Diagnostic::Code::SEMA_SYMBOL_NOT_PUB,
							ident,
							std::format("Struct \"{}\" does not have the #pub attribute", ident_str),
							Diagnostic::Info(
								"Struct declared here:",
								Diagnostic::Location::get(ident_id, this->context)
							)
						);
						return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::ERROR_EMITTED));
					}
				}

				return ReturnType(
					TermInfo(
						TermInfo::ValueCategory::TYPE,
						TypeInfo::VoidableID(
							this->context.type_manager.getOrCreateTypeInfo(TypeInfo(BaseType::ID(ident_id)))
						)
					)
				);

			}else if constexpr(std::is_same<IdentIDType, BaseType::Union::ID>()){
				const BaseType::Union& union_info = this->context.getTypeManager().getUnion(ident_id);

				if constexpr(NEEDS_DEF){
					if(union_info.defCompleted == false){
						return ReturnType(
							evo::Unexpected(AnalyzeExprIdentInScopeLevelError::NEEDS_TO_WAIT_ON_DEF)
						);
					}
				}

				if constexpr(PUB_REQUIRED){
					if(union_info.isPub == false){
						this->emit_error(
							Diagnostic::Code::SEMA_SYMBOL_NOT_PUB,
							ident,
							std::format("Union \"{}\" does not have the #pub attribute", ident_str),
							Diagnostic::Info(
								"Union declared here:",
								Diagnostic::Location::get(ident_id, this->context)
							)
						);
						return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::ERROR_EMITTED));
					}
				}

				return ReturnType(
					TermInfo(
						TermInfo::ValueCategory::TYPE,
						TypeInfo::VoidableID(
							this->context.type_manager.getOrCreateTypeInfo(TypeInfo(BaseType::ID(ident_id)))
						)
					)
				);

			}else if constexpr(std::is_same<IdentIDType, BaseType::Enum::ID>()){
				const BaseType::Enum& enum_info = this->context.getTypeManager().getEnum(ident_id);

				if constexpr(NEEDS_DEF){
					if(enum_info.defCompleted == false){
						return ReturnType(
							evo::Unexpected(AnalyzeExprIdentInScopeLevelError::NEEDS_TO_WAIT_ON_DEF)
						);
					}
				}

				if constexpr(PUB_REQUIRED){
					if(enum_info.isPub == false){
						this->emit_error(
							Diagnostic::Code::SEMA_SYMBOL_NOT_PUB,
							ident,
							std::format("Enum \"{}\" does not have the #pub attribute", ident_str),
							Diagnostic::Info(
								"Enum declared here:",
								Diagnostic::Location::get(ident_id, this->context)
							)
						);
						return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::ERROR_EMITTED));
					}
				}

				return ReturnType(
					TermInfo(
						TermInfo::ValueCategory::TYPE,
						TypeInfo::VoidableID(
							this->context.type_manager.getOrCreateTypeInfo(TypeInfo(BaseType::ID(ident_id)))
						)
					)
				);

			}else if constexpr(std::is_same<IdentIDType, BaseType::Interface::ID>()){
				const BaseType::Interface& interface_info = this->context.getTypeManager().getInterface(ident_id);

				if constexpr(NEEDS_DEF){
					if(interface_info.defCompleted == false){
						return ReturnType(
							evo::Unexpected(AnalyzeExprIdentInScopeLevelError::NEEDS_TO_WAIT_ON_DEF)
						);
					}
				}

				if constexpr(PUB_REQUIRED){
					if(interface_info.isPub == false){
						this->emit_error(
							Diagnostic::Code::SEMA_SYMBOL_NOT_PUB,
							ident,
							std::format("Interface \"{}\" does not have the #pub attribute", ident_str),
							Diagnostic::Info(
								"Interface declared here:",
								Diagnostic::Location::get(ident_id, this->context)
							)
						);
						return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::ERROR_EMITTED));
					}
				}

				return ReturnType(
					TermInfo(
						TermInfo::ValueCategory::TYPE,
						TypeInfo::VoidableID(
							this->context.type_manager.getOrCreateTypeInfo(TypeInfo(BaseType::ID(ident_id)))
						)
					)
				);

			}else if constexpr(std::is_same<IdentIDType, sema::TemplatedStruct::ID>()){
				return ReturnType(TermInfo(TermInfo::ValueCategory::TEMPLATE_TYPE, ident_id));

			}else if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::TemplateTypeParam>()){
				return ReturnType(TermInfo(TermInfo::ValueCategory::TYPE, ident_id.typeID));

			}else if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::TemplateExprParam>()){
				return ReturnType(
					TermInfo(
						TermInfo::ValueCategory::EPHEMERAL,
						TermInfo::ValueStage::CONSTEXPR,
						TermInfo::ValueState::NOT_APPLICABLE,
						ident_id.typeID,
						ident_id.value
					)
				);

			}else if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::DeducedType>()){
				return ReturnType(TermInfo(TermInfo::ValueCategory::TYPE, ident_id.typeID));

			}else if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::DeducedExpr>()){
				return ReturnType(
					TermInfo(
						TermInfo::ValueCategory::EPHEMERAL,
						TermInfo::ValueStage::CONSTEXPR,
						TermInfo::ValueState::NOT_APPLICABLE,
						ident_id.typeID,
						ident_id.value
					)
				);

			}else if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::MemberVar>()){
				auto infos = evo::SmallVector<Diagnostic::Info>();

				const sema::Func& current_func = this->get_current_func();
				const Token::Kind func_name_kind = 
					this->source.getTokenBuffer()[current_func.name.as<Token::ID>()].kind();

				if(
					func_name_kind == Token::Kind::KEYWORD_NEW || func_name_kind == Token::Kind::KEYWORD_DELETE
					|| func_name_kind == Token::Kind::KEYWORD_COPY || func_name_kind == Token::Kind::KEYWORD_MOVE
				){
					const BaseType::Function& func_type =
						this->context.getTypeManager().getFunction(current_func.typeID);

					infos.emplace_back(
						std::format(
							"Did you mean `{}.{}`?",
							this->source.getTokenBuffer()[*func_type.returnParams[0].ident].getString(),
							ident_str
						)
					);
				}else{
					infos.emplace_back(std::format("Did you mean `this.{}`?", ident_str));
				}

				this->emit_error(
					Diagnostic::Code::SEMA_IDENT_NOT_IN_SCOPE,
					ident,
					std::format("Member variables are not accessable except through an accessor", ident_str),
					std::move(infos)
				);
				return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::ERROR_EMITTED));

			}else if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::UnionField>()){
				this->emit_error(
					Diagnostic::Code::SEMA_IDENT_NOT_IN_SCOPE,
					ident,
					std::format("Union fields are not accessable except through an accessor", ident_str),
					Diagnostic::Info(std::format("Did you mean `this.{}`?", ident_str))
				);
				return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::ERROR_EMITTED));

			}else{
				static_assert(false, "Unsupported IdentID");
			}
		});
	}



	auto SemanticAnalyzer::end_sub_scopes(Diagnostic::Location&& location) -> evo::Result<> {
		sema::ScopeLevel& current_scope_level = this->get_current_scope_level();

		for(auto& [value_state_id, value_state_info] : current_scope_level.getValueStateInfos()){
			if(value_state_info.info.is<sema::ScopeLevel::ValueStateInfo::DeclInfo>()){
				sema::ScopeLevel::ValueStateInfo::DeclInfo& decl_info = 
					value_state_info.info.as<sema::ScopeLevel::ValueStateInfo::DeclInfo>();

				if(decl_info.potential_state_change.has_value() == false){ continue; }

				if(decl_info.num_sub_scopes != current_scope_level.numUnterminatedSubScopes()){
					this->emit_error(
						Diagnostic::Code::SEMA_CANT_DETERMINE_VALUE_STATE,
						value_state_id,
						"Cannot determine value state from sub-scopes",
						Diagnostic::Info("Sub-scopes end here:", std::move(location))
					);
					return evo::resultError;
				}

				value_state_info.state = *decl_info.potential_state_change;
				decl_info.potential_state_change.reset();
				decl_info.num_sub_scopes = 0;

			}else{
				evo::debugAssert(
					value_state_info.info.is<sema::ScopeLevel::ValueStateInfo::ModifyInfo>(),
					"Unknown value state info kind"
				);

				sema::ScopeLevel::ValueStateInfo::ModifyInfo& modify_info = 
					value_state_info.info.as<sema::ScopeLevel::ValueStateInfo::ModifyInfo>();

				if(modify_info.num_sub_scopes != current_scope_level.numUnterminatedSubScopes()){
					this->emit_error(
						Diagnostic::Code::SEMA_CANT_DETERMINE_VALUE_STATE,
						value_state_id,
						"Cannot determine value state from sub-scopes",
						Diagnostic::Info("Sub-scopes end here:", std::move(location))
					);
					return evo::resultError;
				}

				modify_info.num_sub_scopes = 0;
			}
		}


		current_scope_level.resetSubScopes();
		return evo::Result<>();
	}



	auto SemanticAnalyzer::get_ident_value_state(sema::ScopeLevel::ValueStateID value_state_id) -> TermInfo::ValueState{
		for(const sema::ScopeLevel::ID scope_id : this->scope){
			const sema::ScopeLevel& scope_level = this->context.sema_buffer.scope_manager.getLevel(scope_id);

			const std::optional<sema::ScopeLevel::ValueState> value_state =
				scope_level.getIdentValueState(value_state_id);

			if(value_state.has_value()){
				switch(*value_state){
					case sema::ScopeLevel::ValueState::INIT:         return TermInfo::ValueState::INIT;
					case sema::ScopeLevel::ValueState::INITIALIZING: return TermInfo::ValueState::INITIALIZING;
					case sema::ScopeLevel::ValueState::UNINIT:       return TermInfo::ValueState::UNINIT;
					case sema::ScopeLevel::ValueState::MOVED_FROM:   return TermInfo::ValueState::MOVED_FROM;
				}
				evo::debugFatalBreak("unknown value state");
			}
		}

		return TermInfo::ValueState::NOT_APPLICABLE;
	}



	auto SemanticAnalyzer::set_ident_value_state(
		sema::ScopeLevel::ValueStateID value_state_id, sema::ScopeLevel::ValueState value_state
	) -> void {
		this->get_current_scope_level().setIdentValueState(value_state_id, value_state);
	}


	auto SemanticAnalyzer::set_ident_value_state_if_needed(sema::Expr target, sema::ScopeLevel::ValueState value_state)
	-> void {
		switch(target.kind()){
			case sema::Expr::Kind::PARAM:
				return this->set_ident_value_state(target.paramID(), value_state);

			case sema::Expr::Kind::RETURN_PARAM:
				return this->set_ident_value_state(target.returnParamID(), value_state);

			case sema::Expr::Kind::ERROR_RETURN_PARAM:
				return this->set_ident_value_state(target.errorReturnParamID(), value_state);

			case sema::Expr::Kind::BLOCK_EXPR_OUTPUT:
				return this->set_ident_value_state(target.blockExprOutputID(), value_state);

			case sema::Expr::Kind::EXCEPT_PARAM:
				return this->set_ident_value_state(target.exceptParamID(), value_state);

			case sema::Expr::Kind::VAR:
				return this->set_ident_value_state(target.varID(), value_state);


			case sema::Expr::Kind::ADDR_OF: {
				const sema::Expr& addr_of_target = this->context.getSemaBuffer().getAddrOf(target.addrOfID());
				return this->set_ident_value_state_if_needed(addr_of_target, value_state);
			} break;

			case sema::Expr::Kind::DEREF: {
				const sema::Deref& deref = this->context.getSemaBuffer().getDeref(target.derefID());
				return this->set_ident_value_state_if_needed(deref.expr, value_state);
			} break;


			case sema::Expr::Kind::ACCESSOR: {
				const sema::Accessor& accessor = this->context.getSemaBuffer().getAccessor(target.accessorID());

				switch(accessor.target.kind()){
					case sema::Expr::Kind::PARAM: {
						const Token::ID current_func_name_token_id = this->get_current_func().name.as<Token::ID>();
						const Token& current_func_name_token =
							this->source.getTokenBuffer()[current_func_name_token_id];

						if(current_func_name_token.kind() != Token::Kind::KEYWORD_DELETE){ return; }

						this->set_ident_value_state(
							sema::OpDeleteThisAccessorValueStateID(accessor.memberABIIndex), value_state
						);
					} break;

					case sema::Expr::Kind::RETURN_PARAM: {
						if(accessor.target.kind() != sema::Expr::Kind::RETURN_PARAM){ return; }

						this->set_ident_value_state(
							sema::ReturnParamAccessorValueStateID(
								accessor.target.returnParamID(), accessor.memberABIIndex
							),
							value_state
						);

						SymbolProc::FuncInfo& func_info = this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>();
						func_info.num_members_of_initializing_are_uninit -= 1;

						if(func_info.num_members_of_initializing_are_uninit == 0){
							this->set_ident_value_state(
								accessor.target.returnParamID(), sema::ScopeLevel::ValueState::INIT
							);
						}
					} break;

					default: return;
				}

			} break;

			case sema::Expr::Kind::NONE: evo::debugFatalBreak("Invalid expr");

			case sema::Expr::Kind::MODULE_IDENT:                  case sema::Expr::Kind::NULL_VALUE:
			case sema::Expr::Kind::UNINIT:                        case sema::Expr::Kind::ZEROINIT:
			case sema::Expr::Kind::INT_VALUE:                     case sema::Expr::Kind::FLOAT_VALUE:
			case sema::Expr::Kind::BOOL_VALUE:                    case sema::Expr::Kind::STRING_VALUE:
			case sema::Expr::Kind::AGGREGATE_VALUE:               case sema::Expr::Kind::CHAR_VALUE:
			case sema::Expr::Kind::INTRINSIC_FUNC: case sema::Expr::Kind::TEMPLATED_INTRINSIC_FUNC_INSTANTIATION:
			case sema::Expr::Kind::COPY:                          case sema::Expr::Kind::MOVE:
			case sema::Expr::Kind::FORWARD:                       case sema::Expr::Kind::FUNC_CALL:
			case sema::Expr::Kind::CONVERSION_TO_OPTIONAL:        case sema::Expr::Kind::OPTIONAL_NULL_CHECK:
			case sema::Expr::Kind::OPTIONAL_EXTRACT:              case sema::Expr::Kind::UNWRAP:
			case sema::Expr::Kind::UNION_ACCESSOR:                case sema::Expr::Kind::LOGICAL_AND:
			case sema::Expr::Kind::LOGICAL_OR:                    case sema::Expr::Kind::TRY_ELSE_EXPR:
			case sema::Expr::Kind::TRY_ELSE_INTERFACE_EXPR:       case sema::Expr::Kind::BLOCK_EXPR:
			case sema::Expr::Kind::FAKE_TERM_INFO:                case sema::Expr::Kind::MAKE_INTERFACE_PTR:
			case sema::Expr::Kind::INTERFACE_PTR_EXTRACT_THIS:    case sema::Expr::Kind::INTERFACE_CALL:
			case sema::Expr::Kind::INDEXER:                       case sema::Expr::Kind::DEFAULT_INIT_PRIMITIVE:
			case sema::Expr::Kind::DEFAULT_TRIVIALLY_INIT_STRUCT: case sema::Expr::Kind::DEFAULT_INIT_ARRAY_REF:
			case sema::Expr::Kind::INIT_ARRAY_REF:                case sema::Expr::Kind::ARRAY_REF_INDEXER:
			case sema::Expr::Kind::ARRAY_REF_SIZE:                case sema::Expr::Kind::ARRAY_REF_DIMENSIONS:
			case sema::Expr::Kind::UNION_DESIGNATED_INIT_NEW:     case sema::Expr::Kind::UNION_TAG_CMP:
			case sema::Expr::Kind::SAME_TYPE_CMP:                 case sema::Expr::Kind::GLOBAL_VAR:
			case sema::Expr::Kind::FUNC: {
				return;
			} break;
		}
	}




	template<bool NEEDS_DEF>
	auto SemanticAnalyzer::wait_on_symbol_proc(
		evo::ArrayProxy<const SymbolProc::Namespace*> symbol_proc_namespaces, std::string_view ident_str
	) -> WaitOnSymbolProcResult {
		auto found_range = std::optional<evo::IterRange<SymbolProc::Namespace::const_iterator>>();
		for(const SymbolProc::Namespace* symbol_proc_namespace : symbol_proc_namespaces){
			const auto find = symbol_proc_namespace->equal_range(ident_str);

			if(find.first != symbol_proc_namespace->end()){
				found_range.emplace(find.first, find.second);
				break;
			}			
		}
		if(found_range.has_value() == false){
			return WaitOnSymbolProcResult::NOT_FOUND;
		}


		bool any_waiting = false;
		bool any_ready = false;
		for(auto& pair : *found_range){
			const SymbolProc::ID& found_symbol_proc_id = pair.second;
			SymbolProc& found_symbol_proc = this->context.symbol_proc_manager.getSymbolProc(found_symbol_proc_id);

			const SymbolProc::WaitOnResult wait_on_result = [&](){
				if constexpr(NEEDS_DEF){
					return found_symbol_proc.waitOnDefIfNeeded(
						this->symbol_proc_id, this->context, found_symbol_proc_id
					);
				}else{
					return found_symbol_proc.waitOnDeclIfNeeded(
						this->symbol_proc_id, this->context, found_symbol_proc_id
					);
				}
			}();

			switch(wait_on_result){
				case SymbolProc::WaitOnResult::NOT_NEEDED: {
					any_ready = true;
				} break;

				case SymbolProc::WaitOnResult::WAITING: {
					any_waiting = true;
				} break;

				case SymbolProc::WaitOnResult::WAS_ERRORED: {
					return WaitOnSymbolProcResult::EXISTS_BUT_ERRORED;
				} break;

				case SymbolProc::WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND: {
					// do nothing...
				} break;

				case SymbolProc::WaitOnResult::CIRCULAR_DEP_DETECTED: {
					return WaitOnSymbolProcResult::CIRCULAR_DEP_DETECTED;
				} break;
			}
		}

		if(any_waiting){
			if(this->symbol_proc.shouldContinueRunning()){
				return WaitOnSymbolProcResult::SEMAS_READY;
			}else{
				return WaitOnSymbolProcResult::NEED_TO_WAIT;
			}
		}

		if(any_ready){ return WaitOnSymbolProcResult::SEMAS_READY; }

		return WaitOnSymbolProcResult::ERROR_PASSED_BY_WHEN_COND;
	}



	auto SemanticAnalyzer::wait_on_symbol_proc_emit_error(
		WaitOnSymbolProcResult result, const auto& ident, std::string&& msg
	) -> void {
		switch(result){
			case WaitOnSymbolProcResult::NOT_FOUND: {
				this->emit_error(Diagnostic::Code::SEMA_NO_SYMBOL_IN_SCOPE_WITH_THAT_IDENT, ident, std::move(msg));
			} break;

			case WaitOnSymbolProcResult::CIRCULAR_DEP_DETECTED: case WaitOnSymbolProcResult::EXISTS_BUT_ERRORED: {
				// do nothing...
			} break;

			case WaitOnSymbolProcResult::ERROR_PASSED_BY_WHEN_COND: {
				this->emit_error(
					Diagnostic::Code::SEMA_NO_SYMBOL_IN_SCOPE_WITH_THAT_IDENT,
					ident,
					std::move(msg),
					Diagnostic::Info("The identifier was declared in a when conditional block that wasn't taken")
				);
			} break;

			case WaitOnSymbolProcResult::NEED_TO_WAIT: {
				evo::debugFatalBreak("WaitOnSymbolProcResult::NEED_TO_WAIT is not an error");
			} break;

			case WaitOnSymbolProcResult::SEMAS_READY: {
				evo::debugFatalBreak("WaitOnSymbolProcResult::SEMAS_READY is not an error");
			} break;
		}
	}







	auto SemanticAnalyzer::set_waiting_for_is_done(SymbolProc::ID target_id, SymbolProc::ID done_id) -> void {
		SymbolProc& target = this->context.symbol_proc_manager.getSymbolProc(target_id);

		const auto lock = std::scoped_lock(target.waiting_for_lock);


		evo::debugAssert(target.waiting_for.empty() == false, "Should never have empty list");

		for(size_t i = 0; i < target.waiting_for.size() - 1; i+=1){
			if(target.waiting_for[i] == done_id){
				target.waiting_for[i] = target.waiting_for.back();
				break;
			}
		}

		target.waiting_for.pop_back();

		if(
			target.waiting_for.empty()
			&& target.isTemplateSubSymbol() == false
			&& target.status == SymbolProc::Status::WAITING
		){
			target.setStatusInQueue();
			this->context.add_task_to_work_manager(target_id);
		}
	}


	template<bool LOOK_THROUGH_DISTINCT_ALIAS, bool LOOK_THROUGH_INTERFACE_IMPL_INSTANTIATION>
	auto SemanticAnalyzer::get_actual_type(TypeInfo::ID type_id) const -> TypeInfo::ID {
		const TypeManager& type_manager = this->context.getTypeManager();

		while(true){
			const TypeInfo& type_info = type_manager.getTypeInfo(type_id);
			if(type_info.qualifiers().empty() == false){ return type_id; }


			if(type_info.baseTypeID().kind() == BaseType::Kind::ALIAS){
				const BaseType::Alias& alias = type_manager.getAlias(type_info.baseTypeID().aliasID());

				evo::debugAssert(alias.aliasedType.load().has_value(), "Definition of alias was not completed");
				type_id = *alias.aliasedType.load();

			}else if(type_info.baseTypeID().kind() == BaseType::Kind::DISTINCT_ALIAS){
				if constexpr(LOOK_THROUGH_DISTINCT_ALIAS){
					const BaseType::DistinctAlias& distinct_alias = 
						type_manager.getDistinctAlias(type_info.baseTypeID().distinctAliasID());

					evo::debugAssert(
						distinct_alias.underlyingType.load().has_value(),
						"Definition of distinct alias was not completed"
					);
					type_id = *distinct_alias.underlyingType.load();

				}else{
					return type_id;	
				}

			}else if(type_info.baseTypeID().kind() == BaseType::Kind::INTERFACE_IMPL_INSTANTIATION){
				if constexpr(LOOK_THROUGH_INTERFACE_IMPL_INSTANTIATION){
					const BaseType::InterfaceImplInstantiation& interface_impl_instantiation = 
						type_manager.getInterfaceImplInstantiation(
							type_info.baseTypeID().interfaceImplInstantiationID()
						);

					type_id = interface_impl_instantiation.implInstantiationTypeID;

				}else{
					return type_id;
				}

			}else{
				return type_id;
			}
		}
	}



	auto SemanticAnalyzer::select_func_overload(
		evo::ArrayProxy<SelectFuncOverloadFuncInfo> func_infos,
		evo::SmallVector<SelectFuncOverloadArgInfo>& arg_infos,
		const auto& call_node,
		bool is_member_call,
		evo::SmallVector<Diagnostic::Info>&& instantiation_error_infos
	) -> evo::Result<size_t> {
		evo::debugAssert(func_infos.empty() == false, "need at least 1 function");

		struct OverloadScore{
			using Success = std::monostate;
			struct TooFewArgs{ size_t min_num; size_t got_num; bool accepts_different_nums; };
			struct TooManyArgs{ size_t max_num; size_t got_num; bool accepts_different_nums; };
			struct IntrinsicWrongNumArgs{ size_t required_num; size_t got_num; };
			struct TypeMismatch{ size_t arg_index; };
			struct ValueKindMismatch{ size_t arg_index; };
			struct InArgNotMovable{ size_t arg_index; };
			struct IncorrectLabel{ size_t arg_index; };
			struct IntrinsicArgWithLabel{ size_t arg_index; };

			using Reason = evo::Variant<
				Success,
				TooFewArgs,
				TooManyArgs,
				IntrinsicWrongNumArgs,
				TypeMismatch,
				ValueKindMismatch,
				InArgNotMovable,
				IncorrectLabel,
				IntrinsicArgWithLabel
			>;
			
			unsigned score;
			Reason reason;

			OverloadScore(unsigned _score) : score(_score), reason(std::monostate()) {};
			OverloadScore(Reason _reason) : score(0), reason(_reason) {};
		};
		auto scores = evo::SmallVector<OverloadScore>();
		scores.reserve(func_infos.size());

		unsigned best_score = 0;
		size_t best_score_index = 0;
		bool found_matching_best_score = false;

		
		for(size_t func_i = 0; const SelectFuncOverloadFuncInfo& func_info : func_infos){
			EVO_DEFER([&](){ func_i += 1; });

			unsigned current_score = 0;

			bool need_to_skip_this_arg = false; 
			

			if(
				func_info.func_id.is<SelectFuncOverloadFuncInfo::IntrinsicFlag>()
				|| func_info.func_id.is<SelectFuncOverloadFuncInfo::BuiltinTypeMethodFlag>()
			){
				if(arg_infos.size() != func_info.func_type.params.size()){
					scores.emplace_back(
						OverloadScore::IntrinsicWrongNumArgs(func_info.func_type.params.size(), arg_infos.size())
					);
					continue;
				}

			}else{
				const sema::Func& sema_func = [&]() -> const sema::Func& {
					if(func_info.func_id.is<sema::Func::ID>()){
						return this->context.getSemaBuffer().getFunc(func_info.func_id.as<sema::Func::ID>());
					}else{
						return this->context.getSemaBuffer().getFunc(
							*func_info.func_id.as<sema::TemplatedFunc::InstantiationInfo>().instantiation.funcID
						);
					}
				}();

				need_to_skip_this_arg = is_member_call && sema_func.isMethod(this->context) == false;

				const size_t num_args = arg_infos.size() - size_t(need_to_skip_this_arg);

				if(num_args < sema_func.minNumArgs){
					scores.emplace_back(OverloadScore::TooFewArgs(
						sema_func.minNumArgs, num_args, sema_func.minNumArgs != func_info.func_type.params.size()
					));
					continue;
				}

				if(num_args > func_info.func_type.params.size()){
					scores.emplace_back(OverloadScore::TooManyArgs(
						func_info.func_type.params.size(),
						num_args,
						sema_func.minNumArgs != func_info.func_type.params.size()
					));
					continue;
				}
			}


			bool arg_checking_failed = false;
			for(size_t arg_i = 0; SelectFuncOverloadArgInfo& arg_info : arg_infos){
				EVO_DEFER([&](){ arg_i += 1; });

				if(need_to_skip_this_arg){
					arg_i -= 1;
					continue;
				}


				///////////////////////////////////
				// check type mismatch

				const TypeCheckInfo& type_check_info = this->type_check<false, false>(
					this->get_actual_type<false, true>(func_info.func_type.params[arg_i].typeID),
					arg_info.term_info,
					"",
					arg_info.ast_node
				);

				if(type_check_info.ok == false){
					scores.emplace_back(OverloadScore::TypeMismatch(arg_i));
					arg_checking_failed = true;
					break;
				}

				if(type_check_info.requires_implicit_conversion == false){ current_score += 1; }


				///////////////////////////////////
				// value kind

				switch(func_info.func_type.params[arg_i].kind){
					case BaseType::Function::Param::Kind::READ: {
						// accepts any value kind
					} break;

					case BaseType::Function::Param::Kind::MUT: {
						if(arg_info.term_info.is_const()){
							scores.emplace_back(OverloadScore::ValueKindMismatch(arg_i));
							arg_checking_failed = true;
							break;
						}


						if(arg_info.term_info.is_concrete() == false){
							const bool param_is_not_this = [&](){
								if(arg_i > 0){ return true; }

								if(
									func_info.func_id.is<SelectFuncOverloadFuncInfo::IntrinsicFlag>()
									|| func_info.func_id.is<SelectFuncOverloadFuncInfo::BuiltinTypeMethodFlag>()
								){
									return true;

								}else{
									const sema::Func& sema_func = [&]() -> const sema::Func& {
										if(func_info.func_id.is<sema::Func::ID>()){
											return this->context.getSemaBuffer().getFunc(
												func_info.func_id.as<sema::Func::ID>()
											);
										}else{
											return this->context.getSemaBuffer().getFunc(
												*func_info.func_id.as<sema::TemplatedFunc::InstantiationInfo>()
													.instantiation.funcID
											);
										}
									}();

									return sema_func.isMethod(this->context) == false;
								}
							}();

							if(param_is_not_this){
								scores.emplace_back(OverloadScore::ValueKindMismatch(arg_i));
								arg_checking_failed = true;
								break;
							}
						}

						current_score += 1; // add 1 to prefer mut over read
					} break;

					case BaseType::Function::Param::Kind::IN: {
						if(arg_info.term_info.is_ephemeral() == false){
							scores.emplace_back(OverloadScore::ValueKindMismatch(arg_i));
							arg_checking_failed = true;
							break;
						}

						if(arg_info.term_info.getExpr().kind() != sema::Expr::Kind::COPY){
							const TypeInfo::ID arg_type_info_id = arg_info.term_info.type_id.as<TypeInfo::ID>();
							if(this->context.getTypeManager().isMovable(arg_type_info_id) == false){
								scores.emplace_back(OverloadScore::InArgNotMovable(arg_i));
								arg_checking_failed = true;
								break;
							}
						}
					} break;

					case BaseType::Function::Param::Kind::C: {
						if(arg_info.term_info.is_ephemeral() == false){
							scores.emplace_back(OverloadScore::ValueKindMismatch(arg_i));
							arg_checking_failed = true;
							break;
						}
					} break;
				}


				///////////////////////////////////
				// check label

				if(arg_info.label.has_value()){
					if(
						func_info.func_id.is<SelectFuncOverloadFuncInfo::IntrinsicFlag>()
						|| func_info.func_id.is<SelectFuncOverloadFuncInfo::BuiltinTypeMethodFlag>()
					){
						scores.emplace_back(OverloadScore::IntrinsicArgWithLabel(arg_i));
						arg_checking_failed = true;
						break;

					}else{
						const sema::Func& sema_func = [&]() -> const sema::Func& {
							if(func_info.func_id.is<sema::Func::ID>()){
								return this->context.getSemaBuffer().getFunc(func_info.func_id.as<sema::Func::ID>());
							}else{
								return this->context.getSemaBuffer().getFunc(
									*func_info.func_id.as<sema::TemplatedFunc::InstantiationInfo>().instantiation.funcID
								);
							}
						}();

						const std::string_view arg_label = 
							this->source.getTokenBuffer()[*arg_info.label].getString();

						const std::string_view param_name = 
							sema_func.getParamName(sema_func.params[arg_i], this->context.getSourceManager());

						if(arg_label != param_name){
							scores.emplace_back(OverloadScore::IncorrectLabel(arg_i));
							arg_checking_failed = true;
							break;
						}
					}
				}


				///////////////////////////////////
				// done checking arg

				current_score += 1;
			}
			if(arg_checking_failed){ continue; }

			current_score += 1;
			scores.emplace_back(current_score);
			if(best_score < current_score){
				best_score = current_score;
				best_score_index = func_i;
				found_matching_best_score = false;
			}else if(best_score == current_score){
				found_matching_best_score = true;
			}
		}

		if(best_score == 0){ // found no matches
			auto infos = evo::SmallVector<Diagnostic::Info>();

			for(size_t i = 0; const OverloadScore& score : scores){
				EVO_DEFER([&](){ i += 1; });

				const auto get_func_location = [&]() -> Diagnostic::Location {
					if(func_infos[i].func_id.is<sema::Func::ID>()){
						return this->get_location(func_infos[i].func_id.as<sema::Func::ID>());

					}else if(func_infos[i].func_id.is<sema::TemplatedFunc::InstantiationInfo>()){
						return this->get_location(
							*func_infos[i].func_id.as<sema::TemplatedFunc::InstantiationInfo>().instantiation.funcID
						);

					}else{
						return Diagnostic::Location::NONE;
					}
				};

			
				score.reason.visit([&](const auto& reason) -> void {
					using ReasonT = std::decay_t<decltype(reason)>;
					
					if constexpr(std::is_same<ReasonT, OverloadScore::Success>()){
						evo::fatalBreak("Success should not have a score of 0");

					}else if constexpr(std::is_same<ReasonT, OverloadScore::TooFewArgs>()){
						if(reason.accepts_different_nums){
							infos.emplace_back(
								std::format(
									"Failed to match: too few arguments (requires at least {}, got {})",
									reason.min_num,
									reason.got_num
								),
								get_func_location()
							);
							
						}else{
							infos.emplace_back(
								std::format(
									"Failed to match: too few arguments (requires {}, got {})",
									reason.min_num,
									reason.got_num
								),
								get_func_location()
							);
						}

					}else if constexpr(std::is_same<ReasonT, OverloadScore::TooManyArgs>()){
						if(reason.accepts_different_nums){
							infos.emplace_back(
								std::format(
									"Failed to match: too many arguments (requires at most {}, got {})",
									reason.max_num,
									reason.got_num
								),
								get_func_location()
							);
							
						}else{
							infos.emplace_back(
								std::format(
									"Failed to match: too many arguments (requires {}, got {})",
									reason.max_num,
									reason.got_num
								),
								get_func_location()
							);
						}

					}else if constexpr(std::is_same<ReasonT, OverloadScore::IntrinsicWrongNumArgs>()){
						infos.emplace_back(
							std::format(
								"Failed to match: wrong number of arguments (requires {}, got {})",
								reason.required_num,
								reason.got_num
							),
							get_func_location()
						);

					}else if constexpr(std::is_same<ReasonT, OverloadScore::TypeMismatch>()){
						const TypeInfo::ID expected_type_id = func_infos[i].func_type.params[reason.arg_index].typeID;
						const TermInfo& got_arg = arg_infos[reason.arg_index].term_info;

						const bool func_is_method = [&](){
							if(is_member_call == false){ return false; }

							if(func_infos[i].func_id.is<sema::Func::ID>()){
								return this->context.getSemaBuffer()
									.getFunc(func_infos[i].func_id.as<sema::Func::ID>())
									.isMethod(this->context);
							}else if(func_infos[i].func_id.is<sema::TemplatedFunc::InstantiationInfo>()){
								return this->context.getSemaBuffer()
									.getFunc(
										*func_infos[i].func_id.as<sema::TemplatedFunc::InstantiationInfo>()
											.instantiation.funcID
									).isMethod(this->context);
							}else{
								return false;
							}
						}();

						infos.emplace_back(
							std::format(
								"Failed to match: argument (index: {}) type mismatch",
								reason.arg_index - size_t(func_is_method)
							),
							get_func_location(),
							evo::SmallVector<Diagnostic::Info>{
								Diagnostic::Info(
									"This argument:", this->get_location(arg_infos[reason.arg_index].ast_node)
								),
								Diagnostic::Info(std::format("Argument type:  {}", this->print_term_type(got_arg))),
								Diagnostic::Info(
									std::format(
										"Parameter type: {}",
										this->context.getTypeManager().printType(
											expected_type_id, this->context.getSourceManager()
										)
									)
								),
							}
						);

					}else if constexpr(std::is_same<ReasonT, OverloadScore::ValueKindMismatch>()){
						// This is done this way beause the IILE version causes an internal compiler error in MSVC
						// 	I didn't report because I didn't want to spend the time figuring out more info
						// TODO(FUTURE): Is this still a problem?
						bool is_method_this_param = false;
						do{
							if(reason.arg_index != 0){ break; }

							if(func_infos[i].func_id.is<sema::Func::ID>()){
								is_method_this_param = this->context.getSemaBuffer()
									.getFunc(func_infos[i].func_id.as<sema::Func::ID>())
									.isMethod(this->context);
							}else if(func_infos[i].func_id.is<sema::TemplatedFunc::InstantiationInfo>()){
								is_method_this_param = this->context.getSemaBuffer()
									.getFunc(
										*func_infos[i].func_id.as<sema::TemplatedFunc::InstantiationInfo>()
											.instantiation.funcID
									).isMethod(this->context);
							}
						}while(false);

						if(is_method_this_param){
							infos.emplace_back(
								std::format(
									"Failed to match: method target value kind mismatch", reason.arg_index
								),
								get_func_location(),
								evo::SmallVector<Diagnostic::Info>{
									Diagnostic::Info(
										"[this] parameters that are [mut] can only accept values that are mutable",
										this->get_location(arg_infos[reason.arg_index].ast_node)
									),
								}
							);

						}else{
							auto sub_infos = evo::SmallVector<Diagnostic::Info>();
							sub_infos.emplace_back(
								"This argument:", this->get_location(arg_infos[reason.arg_index].ast_node)
							);

							switch(func_infos[i].func_type.params[reason.arg_index].kind){
								case BaseType::Function::Param::Kind::READ: {
									evo::debugFatalBreak("Read parameters should never fail to accept value kind");
								} break;

								case BaseType::Function::Param::Kind::MUT: {
									sub_infos.emplace_back(
										"[mut] parameters can only accept values that are concrete and mutable"
									);
								} break;

								case BaseType::Function::Param::Kind::IN: {
									sub_infos.emplace_back("[in] parameters can only accept ephemeral values");
								} break;

								case BaseType::Function::Param::Kind::C: {
									sub_infos.emplace_back("[c] parameters can only accept ephemeral values");
								} break;
							}

							infos.emplace_back(
								std::format(
									"Failed to match: argument (index: {}) value kind mismatch", reason.arg_index
								),
								get_func_location(),
								std::move(sub_infos)
							);
						}


					}else if constexpr(std::is_same<ReasonT, OverloadScore::InArgNotMovable>()){
						infos.emplace_back(
							std::format("Failed to match: argument (index: {}) is not movable", reason.arg_index),
							get_func_location(),
							evo::SmallVector<Diagnostic::Info>{
								Diagnostic::Info(
									"This argument:", this->get_location(arg_infos[reason.arg_index].ast_node)
								),
							}
						);


					}else if constexpr(std::is_same<ReasonT, OverloadScore::IncorrectLabel>()){
						const sema::Func& sema_func =
							this->context.getSemaBuffer().getFunc(func_infos[i].func_id.as<sema::Func::ID>());

						infos.emplace_back(
							std::format("Failed to match: argument (index: {}) has incorrect label", reason.arg_index),
							get_func_location(),
							evo::SmallVector<Diagnostic::Info>{
								Diagnostic::Info("This label:", this->get_location(*arg_infos[reason.arg_index].label)),
								Diagnostic::Info(
									std::format(
										"Expected label: \"{}\"", 
										sema_func.getParamName(
											sema_func.params[reason.arg_index], this->context.getSourceManager()
										)
									)
								),
							}
						);


					}else if constexpr(std::is_same<ReasonT, OverloadScore::IntrinsicArgWithLabel>()){
						infos.emplace_back(
							std::format("Failed to match: argument (index: {}) has a label", reason.arg_index),
							this->get_location(*arg_infos[reason.arg_index].label),
							evo::SmallVector<Diagnostic::Info>{
								Diagnostic::Info("Arguments to intrinsic functions cannot have labels"),
							}
						);

					}else{
						static_assert(false, "Unsupported overload score reason");
					}
				});
			}

			for(Diagnostic::Info& instantiation_error_info : instantiation_error_infos){
				infos.emplace_back(std::move(instantiation_error_info));
			}

			this->emit_error(
				Diagnostic::Code::SEMA_NO_MATCHING_FUNCTION,
				call_node,
				"No matching function overload found",
				std::move(infos)
			);
			return evo::resultError;


		}else if(found_matching_best_score){ // found multiple matches
			auto infos = evo::SmallVector<Diagnostic::Info>();
			for(size_t i = 0; const OverloadScore& score : scores){
				EVO_DEFER([&](){ i += 1; });

				if(score.score == best_score){
					if(func_infos[i].func_id.is<sema::Func::ID>()){
						infos.emplace_back(
							"Could be this one:", this->get_location(func_infos[i].func_id.as<sema::Func::ID>())
						);

					}else if(func_infos[i].func_id.is<sema::TemplatedFunc::InstantiationInfo>()){
						infos.emplace_back(
							"Could be this one:",
							this->get_location(
								*func_infos[i].func_id.as<sema::TemplatedFunc::InstantiationInfo>().instantiation.funcID
							)
						);

					}else{
						// infos.emplace_back("Could be this one:", Diagnostic::Location::NONE);
					}
				}
			}

			this->emit_error(
				Diagnostic::Code::SEMA_MULTIPLE_MATCHING_FUNCTION_OVERLOADS,
				call_node,
				"Multiple matching function overloads found",
				std::move(infos)
			);
			return evo::resultError;
		}


		const SelectFuncOverloadFuncInfo& selected_func = func_infos[best_score_index];

		bool is_first = true;
		for(size_t i = 0; SelectFuncOverloadArgInfo& arg_info : arg_infos){
			if(is_first && is_member_call){
				is_first = false;

				const sema::Func& selected_sema_func = [&]() -> const sema::Func& {
					if(selected_func.func_id.is<sema::Func::ID>()){
						return this->context.getSemaBuffer().getFunc(selected_func.func_id.as<sema::Func::ID>());
					}else{
						return this->context.getSemaBuffer().getFunc(
							*selected_func.func_id.as<sema::TemplatedFunc::InstantiationInfo>().instantiation.funcID
						);
					}
				}();

				if(selected_sema_func.isMethod(this->context) == false){ continue; }
			}

			is_first = false;

			// implicitly convert all the required args
			if(!this->context.getTypeManager().getTypeInfo(selected_func.func_type.params[i].typeID).isInterface()){
				if(this->type_check<true, true>(
					this->get_actual_type<false, true>(selected_func.func_type.params[i].typeID),
					arg_info.term_info,
					"Function call argument",
					arg_info.ast_node
				).ok == false){
					evo::debugFatalBreak("This should not be able to fail");
				}
			}

			i += 1;
		}

		return best_score_index;
	}


	template<bool IS_CONSTEXPR, bool ERRORS>
	auto SemanticAnalyzer::func_call_impl(
		const AST::FuncCall& func_call,
		const TermInfo& target_term_info,
		evo::ArrayProxy<SymbolProc::TermInfoID> args,
		evo::ArrayProxy<SymbolProc::TermInfoID> template_args
	) -> evo::Expected<FuncCallImplData, bool> {
		TypeManager& type_manager = this->context.type_manager;

		auto func_infos = evo::SmallVector<SelectFuncOverloadFuncInfo>();
		auto instantiation_infos = evo::SmallVector<sema::TemplatedFunc::InstantiationInfo>();

		auto method_this_term_info = std::optional<TermInfo>();

		auto template_overload_match_infos = evo::SmallVector<std::optional<TemplateOverloadMatchFail>>();

		switch(target_term_info.value_category){
			case TermInfo::ValueCategory::FUNCTION: {
				using FuncOverload = evo::Variant<sema::Func::ID, sema::TemplatedFunc::ID>;
				for(const FuncOverload& func_overload : target_term_info.type_id.as<TermInfo::FuncOverloadList>()){
					if(func_overload.is<sema::Func::ID>()){
						const sema::Func& sema_func =
							this->context.getSemaBuffer().getFunc(func_overload.as<sema::Func::ID>());
						const BaseType::Function& func_type = type_manager.getFunction(sema_func.typeID);
						func_infos.emplace_back(func_overload.as<sema::Func::ID>(), func_type);

					}else{
						evo::debugAssert(func_overload.is<sema::TemplatedFunc::ID>(), "Unknown overload id");
						
						evo::Expected<sema::TemplatedFunc::InstantiationInfo, TemplateOverloadMatchFail> template_res =
							this->get_select_func_overload_func_info_for_template(
								func_overload.as<sema::TemplatedFunc::ID>(), args, template_args, false
							);

						if(template_res.has_value() == false){
							template_overload_match_infos.emplace_back(template_res.error());
							continue;
						}
						template_overload_match_infos.emplace_back(std::nullopt);
						instantiation_infos.emplace_back(std::move(template_res.value()));
					}
				}
			} break;

			case TermInfo::ValueCategory::METHOD_CALL: {
				using FuncOverload = evo::Variant<sema::Func::ID, sema::TemplatedFunc::ID>;

				for(const FuncOverload& func_overload : target_term_info.type_id.as<TermInfo::FuncOverloadList>()){
					if(func_overload.is<sema::Func::ID>()){
						const sema::Func& sema_func =
							this->context.getSemaBuffer().getFunc(func_overload.as<sema::Func::ID>());
						const BaseType::Function& func_type = type_manager.getFunction(sema_func.typeID);
						func_infos.emplace_back(func_overload.as<sema::Func::ID>(), func_type);

					}else{
						evo::debugAssert(func_overload.is<sema::TemplatedFunc::ID>(), "Unknown overload id");
						
						evo::Expected<sema::TemplatedFunc::InstantiationInfo, TemplateOverloadMatchFail> template_res =
							this->get_select_func_overload_func_info_for_template(
								func_overload.as<sema::TemplatedFunc::ID>(), args, template_args, true
							);

						if(template_res.has_value() == false){
							template_overload_match_infos.emplace_back(template_res.error());
							continue;
						}
						template_overload_match_infos.emplace_back(std::nullopt);
						instantiation_infos.emplace_back(std::move(template_res.value()));
					}
				}

				const sema::FakeTermInfo& fake_term_info =
					this->context.getSemaBuffer().getFakeTermInfo(target_term_info.getExpr().fakeTermInfoID());

				method_this_term_info.emplace(TermInfo::fromFakeTermInfo(fake_term_info));
			} break;

			case TermInfo::ValueCategory::INTERFACE_CALL: {
				using FuncOverload = evo::Variant<sema::Func::ID, sema::TemplatedFunc::ID>;
				for(const FuncOverload& func_overload : target_term_info.type_id.as<TermInfo::FuncOverloadList>()){
					const sema::Func& sema_func =
						this->context.getSemaBuffer().getFunc(func_overload.as<sema::Func::ID>());
					const BaseType::Function& func_type = type_manager.getFunction(sema_func.typeID);
					func_infos.emplace_back(func_overload.as<sema::Func::ID>(), func_type);
				}

				const sema::FakeTermInfo& fake_term_info =
					this->context.getSemaBuffer().getFakeTermInfo(target_term_info.getExpr().fakeTermInfoID());

				method_this_term_info.emplace(TermInfo::fromFakeTermInfo(fake_term_info));

				method_this_term_info->type_id = this->context.type_manager.getOrCreateTypeInfo(
					this->context.getTypeManager().getTypeInfo(method_this_term_info->type_id.as<TypeInfo::ID>())
						.copyWithPoppedQualifier()
				);
			} break;

			case TermInfo::ValueCategory::INTRINSIC_FUNC: {
				const TypeInfo::ID type_info_id = target_term_info.type_id.as<TypeInfo::ID>();
				const TypeInfo& type_info = type_manager.getTypeInfo(type_info_id);
				const BaseType::Function& func_type = type_manager.getFunction(type_info.baseTypeID().funcID());
				func_infos.emplace_back(SelectFuncOverloadFuncInfo::IntrinsicFlag{}, func_type);
			} break;

			case TermInfo::ValueCategory::TEMPLATE_INTRINSIC_FUNC: {
				auto instantiation_args = evo::SmallVector<std::optional<TypeInfo::VoidableID>>();
				for(const SymbolProc::TermInfoID& arg : template_args){
					const TermInfo& arg_term_info = this->get_term_info(arg);

					if(arg_term_info.value_category != TermInfo::ValueCategory::TYPE){
						instantiation_args.emplace_back();
					}else{
						instantiation_args.emplace_back(arg_term_info.type_id.as<TypeInfo::VoidableID>());
					}
				}
				const Context::TemplateIntrinsicFuncInfo& func_info = this->context.getTemplateIntrinsicFuncInfo(
					target_term_info.type_id.as<TemplateIntrinsicFunc::Kind>()
				);

				const BaseType::ID instantiated_type = this->context.type_manager.getOrCreateFunction(
					func_info.getTypeInstantiation(instantiation_args)
				);

				func_infos.emplace_back(
					SelectFuncOverloadFuncInfo::IntrinsicFlag{},
					this->context.getTypeManager().getFunction(instantiated_type.funcID())
				);
			} break;

			case TermInfo::ValueCategory::BUILTIN_TYPE_METHOD: {
				const TypeInfo::ID type_info_id = target_term_info.type_id.as<TermInfo::BuiltinTypeMethod>().typeID;
				const TypeInfo& type_info = type_manager.getTypeInfo(type_info_id);
				const BaseType::Function& func_type = type_manager.getFunction(type_info.baseTypeID().funcID());
				func_infos.emplace_back(SelectFuncOverloadFuncInfo::BuiltinTypeMethodFlag{}, func_type);
			} break;

			default: {
				this->emit_error(
					Diagnostic::Code::SEMA_CANNOT_CALL_LIKE_FUNCTION,
					func_call.target,
					"Cannot call expression like a function"
				);
				return evo::Unexpected(true);
			} break;
		}


		auto instantiation_error_infos = evo::SmallVector<Diagnostic::Info>();

		if(instantiation_infos.empty()){
			if(template_overload_match_infos.empty() == false){
				const TermInfo::FuncOverloadList& func_overload_list =
					target_term_info.type_id.as<TermInfo::FuncOverloadList>();

				for(size_t i = 0; const std::optional<TemplateOverloadMatchFail>& info : template_overload_match_infos){
					EVO_DEFER([&](){ i += 1; });

					const auto get_func_location = [&]() -> Diagnostic::Location {
						if(func_overload_list[i].is<sema::Func::ID>()){
							return this->get_location(func_overload_list[i].as<sema::Func::ID>());
						}else{
							return this->get_location(func_overload_list[i].as<sema::TemplatedFunc::ID>());
						}
					};

					if(info.has_value() == false){ continue; }
					
					info->reason.visit([&](const auto& reason) -> void {
						using ReasonT = std::decay_t<decltype(reason)>;

						if constexpr(std::is_same<ReasonT, TemplateOverloadMatchFail::Handled>()){
							return;

						}else if constexpr(std::is_same<ReasonT, TemplateOverloadMatchFail::TooFewTemplateArgs>()){
							if(reason.accepts_different_nums){
								instantiation_error_infos.emplace_back(
									std::format(
										"Failed to match: too few template arguments (requires at least {}, got {})",
										reason.min_num,
										reason.got_num
									),
									get_func_location()
								);
								
							}else{
								instantiation_error_infos.emplace_back(
									std::format(
										"Failed to match: too few template arguments (requires {}, got {})",
										reason.min_num,
										reason.got_num
									),
									get_func_location()
								);
							}

						}else if constexpr(std::is_same<ReasonT, TemplateOverloadMatchFail::TooManyTemplateArgs>()){
							if(reason.accepts_different_nums){
								instantiation_error_infos.emplace_back(
									std::format(
										"Failed to match: too many template arguments (requires at most {}, got {})",
										reason.max_num,
										reason.got_num
									),
									get_func_location()
								);
								
							}else{
								instantiation_error_infos.emplace_back(
									std::format(
										"Failed to match: too many template arguments (requires {}, got {})",
										reason.max_num,
										reason.got_num
									),
									get_func_location()
								);
							}

						}else if constexpr(std::is_same<ReasonT, TemplateOverloadMatchFail::TemplateArgWrongKind>()){
							const AST::TemplatedExpr& templated_expr =
								this->source.getASTBuffer().getTemplatedExpr(func_call.target);

							if(reason.supposed_to_be_expr){
								instantiation_error_infos.emplace_back(
									std::format(
										"Failed to match: template parameter (index: {}) expects an expression, "
											"got a type",
										reason.arg_index
									),
									get_func_location(),
									evo::SmallVector<Diagnostic::Info>{
										Diagnostic::Info(
											"This argument:", this->get_location(templated_expr.args[reason.arg_index])
										),
									}
								);
							}else{
								instantiation_error_infos.emplace_back(
									std::format(
										"Failed to match: template parameter (index: {}) expects a type, "
											"got an expression",
										reason.arg_index
									),
									get_func_location(),
									evo::SmallVector<Diagnostic::Info>{
										Diagnostic::Info(
											"This argument:", this->get_location(templated_expr.args[reason.arg_index])
										),
									}
								);
							}

						}else if constexpr(std::is_same<ReasonT, TemplateOverloadMatchFail::WrongNumArgs>()){
							instantiation_error_infos.emplace_back(
								std::format(
									"Failed to match: wrong number of arguments (requires {}, got {})",
									reason.expected_num,
									reason.got_num
								),
								get_func_location()
							);

						}else if constexpr(std::is_same<ReasonT, TemplateOverloadMatchFail::CantDeduceArgType>()){
							instantiation_error_infos.emplace_back(
								std::format(
									"Failed to match: can't deduce type from argument (index: {})",
									reason.arg_index
								),
								get_func_location(),
								evo::SmallVector<Diagnostic::Info>{
									Diagnostic::Info(
										"This argument:", this->get_location(func_call.args[reason.arg_index].value)
									),
									Diagnostic::Info(
										std::format(
											"Argument type: {}",
											this->print_term_type(this->get_term_info(args[reason.arg_index]))
										)
									)
								}
							);
							
						}else{
							static_assert(false, "Unsupported TemplateOverloadMatchFail");
						}
					});
				}

				if(func_infos.empty()){
					this->emit_error(
						Diagnostic::Code::SEMA_NO_MATCHING_FUNCTION,
						func_call.target,
						"No matching function overload found",
						std::move(instantiation_error_infos)
					);
					return evo::Unexpected(true);
				}
			}

		}else{
			bool any_waiting_or_ready = false;
			
			for(const sema::TemplatedFunc::InstantiationInfo& instantiation_info : instantiation_infos){
				const SymbolProc::ID instantiation_symbol_proc_id = 
					*instantiation_info.instantiation.symbolProcID.load();
				SymbolProc& instantiation_symbol_proc =
					this->context.symbol_proc_manager.getSymbolProc(instantiation_symbol_proc_id);

				SymbolProc::WaitOnResult wait_on_result = instantiation_symbol_proc.waitOnDeclIfNeeded(
					this->symbol_proc_id, this->context, instantiation_symbol_proc_id
				);


				switch(wait_on_result){
					case SymbolProc::WaitOnResult::NOT_NEEDED:                 any_waiting_or_ready = true; break;
					case SymbolProc::WaitOnResult::WAITING:                    any_waiting_or_ready = true; break;
					case SymbolProc::WaitOnResult::WAS_ERRORED:                return evo::Unexpected(true);
					case SymbolProc::WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND: break;
					case SymbolProc::WaitOnResult::CIRCULAR_DEP_DETECTED:      return evo::Unexpected(true);
				}
			}

			if(any_waiting_or_ready == false){
				this->emit_error(
					Diagnostic::Code::SEMA_NO_MATCHING_FUNCTION,
					func_call.target,
					"No function overload found",
					Diagnostic::Info("All were passed by when conditionals")
				);
				return evo::Unexpected(true);
			}


			if(this->symbol_proc.shouldContinueRunning() == false){
				return evo::Unexpected(false);
			}

			using ErroredReason = sema::TemplatedFunc::Instantiation::ErroredReason;

			auto instantiation_errors = evo::SmallVector<ErroredReason>();

			for(const sema::TemplatedFunc::InstantiationInfo& instantiation_info : instantiation_infos){
				if(instantiation_info.instantiation.errored()){
					const SymbolProc& instantiation_symbol_proc = this->context.symbol_proc_manager.getSymbolProc(
						*instantiation_info.instantiation.symbolProcID.load()
					);

					const SymbolProc::FuncInfo& func_info =
						instantiation_symbol_proc.extra_info.as<SymbolProc::FuncInfo>();

					if(
						func_info.instantiation->errored() 
						&& func_info.instantiation->errored_reason
							.is<sema::TemplatedFunc::Instantiation::ErroredReasonErroredAfterDecl>() == false
					){
						instantiation_errors.emplace_back(func_info.instantiation->errored_reason);
					}else{
						return evo::Unexpected(true);
					}
					continue;
				}

				const sema::Func& instantiated_func = this->context.getSemaBuffer().getFunc(
					*instantiation_info.instantiation.funcID
				);

				func_infos.emplace_back(
					instantiation_info, this->context.getTypeManager().getFunction(instantiated_func.typeID)
				);
			}

			for(size_t i = 0; const ErroredReason& instantiation_error : instantiation_errors){
				const auto get_func_location = [&]() -> Diagnostic::Location {
					const SymbolProc& symbol_proc = this->context.symbol_proc_manager.getSymbolProc(
						*instantiation_infos[i].instantiation.symbolProcID.load()
					);

					return Diagnostic::Location::get(
						symbol_proc.ast_node, this->context.getSourceManager()[symbol_proc.source_id]
					);
				};


				instantiation_error.visit([&](const auto& reason) -> void {
					using ReasonT = std::decay_t<decltype(reason)>;

					using Instantiation = sema::TemplatedFunc::Instantiation;

					if constexpr(std::is_same<ReasonT, std::monostate>()){
						evo::debugAssert("Template instantiation didn't error");

					}else if constexpr(
						std::is_same<ReasonT, Instantiation::ErroredReasonParamDeductionFailed>()
					){
						instantiation_error_infos.emplace_back(
							std::format(
								"Failed to match: failed to deduce type of parameter (index: {}) ", reason.arg_index
							),
							get_func_location(),
							evo::SmallVector<Diagnostic::Info>{
								Diagnostic::Info(
									"This argument:", this->get_location(func_call.args[reason.arg_index].value)
								),
								Diagnostic::Info(
									std::format(
										"Argument type: {}",
										this->print_term_type(this->get_term_info(args[reason.arg_index]))
									)
								)
							}
						);


					}else if constexpr(
						std::is_same<ReasonT, Instantiation::ErroredReasonArgTypeMismatch>()
					){
						instantiation_error_infos.emplace_back(
							std::format("Failed to match: argument (index: {}) type mismatch", reason.arg_index),
							get_func_location(),
							evo::SmallVector<Diagnostic::Info>{
								Diagnostic::Info(
									"This argument:", this->get_location(func_call.args[reason.arg_index].value)
								),
								Diagnostic::Info(
									std::format(
										"Argument type:  {}",
										this->context.getTypeManager().printType(
											reason.got_type_id, this->context.getSourceManager()
										)
									)
								),
								Diagnostic::Info(
									std::format(
										"Parameter type: {}",
										this->context.getTypeManager().printType(
											reason.expected_type_id, this->context.getSourceManager()
										)
									)
								),
							}
						);

					}else if constexpr(
						std::is_same<ReasonT, Instantiation::ErroredReasonTypeDoesntImplInterface>()
					){
						instantiation_error_infos.emplace_back(
							std::format(
								"Failed to match: type of argument (index: {}) doesn't implement the interface",
								reason.arg_index
							),
							get_func_location(),
							evo::SmallVector<Diagnostic::Info>{
								Diagnostic::Info(
									"This argument:", this->get_location(func_call.args[reason.arg_index].value)
								),
								Diagnostic::Info(
									std::format(
										"Argument type:  {}",
										this->context.getTypeManager().printType(
											reason.got_type_id, this->context.getSourceManager()
										)
									)
								),
								Diagnostic::Info(
									std::format(
										"Interface type: {}",
										this->context.getTypeManager().printType(
											reason.interface_type_id, this->context.getSourceManager()
										)
									)
								),
							}
						);
					}else if constexpr(
						std::is_same<ReasonT, Instantiation::ErroredReasonErroredAfterDecl>()
					){
						evo::debugAssert("Errored after decl, shouldn't get here");

					}else{
						static_assert(false, "Unknown errored reason");
					}
				});

				i += 1;
			}

			if(func_infos.empty()){ // if all instantiations errored
				this->emit_error(
					Diagnostic::Code::SEMA_NO_MATCHING_FUNCTION,
					func_call.target,
					"No function overload found",
					std::move(instantiation_error_infos)
				);
				return evo::Unexpected(true);
			}
		}



		auto arg_infos = evo::SmallVector<SelectFuncOverloadArgInfo>();
		arg_infos.reserve(arg_infos.size() + size_t(method_this_term_info.has_value()));
		if(method_this_term_info.has_value()){
			if(func_call.target.kind() == AST::Kind::INFIX){
				const AST::Infix& target_infix = this->source.getASTBuffer().getInfix(func_call.target);
				arg_infos.emplace_back(*method_this_term_info, target_infix.lhs, std::nullopt);
			}else{
				const AST::TemplatedExpr& template_expr =
					this->source.getASTBuffer().getTemplatedExpr(func_call.target);
				const AST::Infix& target_infix = this->source.getASTBuffer().getInfix(template_expr.base);
				arg_infos.emplace_back(*method_this_term_info, target_infix.lhs, std::nullopt);
			}
		}
		for(size_t i = 0; const SymbolProc::TermInfoID& arg : args){
			TermInfo& arg_term_info = this->get_term_info(arg);

			if constexpr(IS_CONSTEXPR){
				if(arg_term_info.value_stage != TermInfo::ValueStage::CONSTEXPR){
					this->emit_error(
						Diagnostic::Code::SEMA_EXPR_NOT_CONSTEXPR,
						func_call.args[i].value,
						"Arguments in a constexpr function call must have a value stage of constexpr",
						Diagnostic::Info(
							std::format(
								"Value stage of the argument is {}",
								arg_term_info.value_stage == TermInfo::ValueStage::COMPTIME ? "comptime" : "runtime"
							)
						)
					);
					return evo::Unexpected(true);
				}
			}else{
				if(this->expr_in_func_is_valid_value_stage(arg_term_info, func_call.args[i].value) == false){
					return evo::Unexpected(true);
				}

				if(
					arg_term_info.value_state != TermInfo::ValueState::INIT
					&& arg_term_info.value_state != TermInfo::ValueState::NOT_APPLICABLE
				){
					this->emit_error(
						Diagnostic::Code::SEMA_EXPR_WRONG_STATE,
						func_call.args[i].value,
						"Arguments to functions must be initialized"
					);
					return evo::Unexpected(true);
				}
			}

			arg_infos.emplace_back(arg_term_info, func_call.args[i].value, func_call.args[i].label);
			i += 1;
		}


		const evo::Result<size_t> selected_func_overload_index = this->select_func_overload(
			func_infos,
			arg_infos,
			func_call.target,
			method_this_term_info.has_value(),
			std::move(instantiation_error_infos)
		);
		if(selected_func_overload_index.isError()){ return evo::Unexpected(true); }


		if constexpr(ERRORS){
			if(func_infos[selected_func_overload_index.value()].func_type.hasErrorReturn() == false){
				this->emit_error(
					Diagnostic::Code::SEMA_FUNC_DOESNT_ERROR,
					func_call,
					"Function doesn't error"
				);
				return evo::Unexpected(true);
			}
		}else{
			if(func_infos[selected_func_overload_index.value()].func_type.hasErrorReturn()){
				this->emit_error(
					Diagnostic::Code::SEMA_FUNC_DOESNT_ERROR,
					func_call,
					"Function error not handled"
				);
				return evo::Unexpected(true);
			}
		}


		switch(target_term_info.value_category){
			case TermInfo::ValueCategory::FUNCTION: case TermInfo::ValueCategory::METHOD_CALL: {
				const SelectFuncOverloadFuncInfo::FuncID& selected_func_id = 
					func_infos[selected_func_overload_index.value()].func_id;

				if(selected_func_id.is<sema::Func::ID>()){
					return FuncCallImplData(
						selected_func_id.as<sema::Func::ID>(),
						&this->context.sema_buffer.getFunc(selected_func_id.as<sema::Func::ID>()),
						func_infos[selected_func_overload_index.value()].func_type
					);

				}else{
					evo::debugAssert(
						selected_func_id.is<sema::TemplatedFunc::InstantiationInfo>(),
						"Unsupported func id type for this value category"
					);

					const sema::TemplatedFunc::InstantiationInfo& instantiation_info =
						selected_func_id.as<sema::TemplatedFunc::InstantiationInfo>();

					const SymbolProc::ID instantiation_symbol_proc_id = 
						*instantiation_info.instantiation.symbolProcID.load();
					SymbolProc& instantiation_symbol_proc =
						this->context.symbol_proc_manager.getSymbolProc(instantiation_symbol_proc_id);

					if(instantiation_symbol_proc.unsuspendIfNeeded()){
						this->context.symbol_proc_manager.symbol_proc_unsuspended();

						sema::Func& sema_func = this->context.sema_buffer.funcs[
							*instantiation_info.instantiation.funcID
						];
						sema_func.status = sema::Func::Status::NOT_DONE;

						this->context.add_task_to_work_manager(instantiation_symbol_proc_id);
					}

					return FuncCallImplData(
						*instantiation_info.instantiation.funcID,
						&this->context.sema_buffer.getFunc(*instantiation_info.instantiation.funcID),
						func_infos[selected_func_overload_index.value()].func_type
					);
				}
			} break;


			case TermInfo::ValueCategory::INTERFACE_CALL: {
				const sema::Func::ID selected_func_id = 
					func_infos[selected_func_overload_index.value()].func_id.as<sema::Func::ID>();

				return FuncCallImplData(
					selected_func_id,
					&this->context.sema_buffer.getFunc(selected_func_id),
					func_infos[selected_func_overload_index.value()].func_type
				);
			} break;

			case TermInfo::ValueCategory::INTRINSIC_FUNC: case TermInfo::ValueCategory::TEMPLATE_INTRINSIC_FUNC: {
				const BaseType::Function& selected_func_type = 
					func_infos[selected_func_overload_index.value()].func_type;

				return FuncCallImplData(std::nullopt, nullptr, selected_func_type);
			} break;

			case TermInfo::ValueCategory::BUILTIN_TYPE_METHOD: {
				const BaseType::Function& selected_func_type = 
					func_infos[selected_func_overload_index.value()].func_type;

				return FuncCallImplData(std::nullopt, nullptr, selected_func_type);
			} break;

			default: evo::debugFatalBreak("Should have already been caught that value category is not callable func");
		}
	}



	auto SemanticAnalyzer::get_select_func_overload_func_info_for_template(
		sema::TemplatedFunc::ID func_id,
		evo::ArrayProxy<SymbolProc::TermInfoID> args,
		evo::ArrayProxy<SymbolProc::TermInfoID> template_args,
		bool is_member_call
	) -> evo::Expected<sema::TemplatedFunc::InstantiationInfo, TemplateOverloadMatchFail> {
		sema::TemplatedFunc& templated_func = this->context.sema_buffer.templated_funcs[func_id];

		const Source& template_source = this->context.getSourceManager()[templated_func.symbolProc.source_id];
		const AST::FuncDef& ast_func = template_source.getASTBuffer().getFuncDef(templated_func.symbolProc.ast_node);


		auto instantiation_lookup_args = evo::SmallVector<sema::TemplatedFunc::Arg>();
		auto instantiation_args = evo::SmallVector<evo::Variant<TypeInfo::VoidableID, sema::Expr>>();

		if(template_args.size() < templated_func.minNumTemplateArgs){
			return evo::Unexpected<TemplateOverloadMatchFail>(
				TemplateOverloadMatchFail(TemplateOverloadMatchFail::TooFewTemplateArgs(
					templated_func.minNumTemplateArgs,
					template_args.size(),
					templated_func.minNumTemplateArgs != templated_func.templateParams.size()
				))
			);
		}

		if(template_args.size() > templated_func.templateParams.size()){
			return evo::Unexpected<TemplateOverloadMatchFail>(
				TemplateOverloadMatchFail(TemplateOverloadMatchFail::TooManyTemplateArgs(
					templated_func.templateParams.size(),
					template_args.size(),
					templated_func.minNumTemplateArgs != templated_func.templateParams.size()
				))
			);
		}

		this->scope.pushTemplateDeclInstantiationTypesScope();
		EVO_DEFER([&](){ this->scope.popTemplateDeclInstantiationTypesScope(); });


		for(size_t i = 0; const SymbolProc::TermInfoID template_arg_id : template_args){
			EVO_DEFER([&](){ i += 1; });

			const AST::TemplatePack& ast_template_pack =
				template_source.getASTBuffer().getTemplatePack(*ast_func.templatePack);

			TermInfo& template_arg = this->get_term_info(template_arg_id);

			if(template_arg.value_category == TermInfo::ValueCategory::TYPE){ // arg is type
				if(templated_func.templateParams[i].typeID.has_value()){
					return evo::Unexpected<TemplateOverloadMatchFail>(
						TemplateOverloadMatchFail(TemplateOverloadMatchFail::TemplateArgWrongKind(i, true))
					);
				}

				const TypeInfo::VoidableID arg_type_id = template_arg.type_id.as<TypeInfo::VoidableID>();

				instantiation_lookup_args.emplace_back(arg_type_id);
				instantiation_args.emplace_back(arg_type_id);


				this->scope.addTemplateDeclInstantiationType(
					template_source.getTokenBuffer()[ast_template_pack.params[i].ident].getString(), arg_type_id
				);

			}else{ // arg is expr
				if(templated_func.templateParams[i].typeID.has_value() == false){
					return evo::Unexpected<TemplateOverloadMatchFail>(
						TemplateOverloadMatchFail(TemplateOverloadMatchFail::TemplateArgWrongKind(i, false))
					);
				}

				const evo::Result<TypeInfo::ID> expr_type_id = [&]() -> evo::Result<TypeInfo::ID> {
					if(templated_func.templateParams[i].typeID->isTemplateDeclInstantiation()){
						const evo::Result<TypeInfo::VoidableID> resolved_type = this->resolve_type(
							template_source.getASTBuffer().getType(ast_template_pack.params[i].type)
						);
						if(resolved_type.isError()){
							return evo::resultError;
						}

						if(resolved_type.value().isVoid()){
							this->emit_error(
								Diagnostic::Code::SEMA_TEMPLATE_PARAM_CANNOT_BE_TYPE_VOID,
								ast_template_pack.params[i].type,
								"Template expression parameter cannot be type `Void`"
							);
							return evo::resultError;
						}

						return resolved_type.value().asTypeID();
					}else{
						return *templated_func.templateParams[i].typeID;
					}
				}();
				if(expr_type_id.isError()){
					return evo::Unexpected<TemplateOverloadMatchFail>(
						TemplateOverloadMatchFail(TemplateOverloadMatchFail::Handled())
					);
				}


				if(this->type_check<true, false>(
					expr_type_id.value(), template_arg, "", Diagnostic::Location::NONE
				).ok == false){
					return evo::Unexpected<TemplateOverloadMatchFail>(
						TemplateOverloadMatchFail(TemplateOverloadMatchFail::Handled())
					);
				}
				

				const sema::Expr& arg_expr = template_arg.getExpr();
				instantiation_args.emplace_back(arg_expr);
				switch(arg_expr.kind()){
					case sema::Expr::Kind::INT_VALUE: {
						instantiation_lookup_args.emplace_back(
							core::GenericValue(
								evo::copy(this->context.getSemaBuffer().getIntValue(arg_expr.intValueID()).value)
							)
						);
					} break;

					case sema::Expr::Kind::FLOAT_VALUE: {
						instantiation_lookup_args.emplace_back(
							core::GenericValue(
								evo::copy(this->context.getSemaBuffer().getFloatValue(arg_expr.floatValueID()).value)
							)
						);
					} break;

					case sema::Expr::Kind::BOOL_VALUE: {
						instantiation_lookup_args.emplace_back(
							core::GenericValue(
								evo::copy(this->context.getSemaBuffer().getBoolValue(arg_expr.boolValueID()).value)
							)
						);
					} break;

					case sema::Expr::Kind::STRING_VALUE: {
						instantiation_lookup_args.emplace_back(
							core::GenericValue(
								evo::copy(this->context.getSemaBuffer().getStringValue(arg_expr.stringValueID()).value)
							)
						);
					} break;

					case sema::Expr::Kind::AGGREGATE_VALUE: {
						evo::debugFatalBreak(
							"Aggregate value template args are not supported yet (getting here should be impossible)"
						);
					} break;

					case sema::Expr::Kind::CHAR_VALUE: {
						instantiation_lookup_args.emplace_back(
							core::GenericValue(this->context.getSemaBuffer().getCharValue(arg_expr.charValueID()).value)
						);
					} break;

					default: evo::debugFatalBreak("Invalid template argument value");
				}
			}
		}


		// default template args
		for(size_t i = template_args.size(); i < templated_func.templateParams.size(); i+=1){
			templated_func.templateParams[i].defaultValue.visit([&](const auto& default_value) -> void {
				using DefaultValue = std::decay_t<decltype(default_value)>;

				if constexpr(std::is_same<DefaultValue, std::monostate>()){
					evo::debugFatalBreak("Expected template default value, found none");

				}else if constexpr(std::is_same<DefaultValue, sema::Expr>()){
					switch(default_value.kind()){
						case sema::Expr::Kind::INT_VALUE: {
							instantiation_lookup_args.emplace_back(
								core::GenericValue(
									evo::copy(
										this->context.getSemaBuffer().getIntValue(default_value.intValueID()).value
									)
								)
							);
						} break;

						case sema::Expr::Kind::FLOAT_VALUE: {
							instantiation_lookup_args.emplace_back(
								core::GenericValue(
									evo::copy(
										this->context.getSemaBuffer().getFloatValue(default_value.floatValueID()).value
									)
								)
							);
						} break;

						case sema::Expr::Kind::BOOL_VALUE: {
							instantiation_lookup_args.emplace_back(
								core::GenericValue(
									evo::copy(
										this->context.getSemaBuffer().getBoolValue(default_value.boolValueID()).value
									)
								)
							);
						} break;

						case sema::Expr::Kind::STRING_VALUE: {
							instantiation_lookup_args.emplace_back(
								core::GenericValue(
									evo::copy(
										this->context.getSemaBuffer().getStringValue(
											default_value.stringValueID()
										).value
									)
								)
							);
						} break;

						case sema::Expr::Kind::AGGREGATE_VALUE: {
							evo::debugFatalBreak(
								"String value template args are not supported yet (getting here should be impossible)"
							);
						} break;

						case sema::Expr::Kind::CHAR_VALUE: {
							instantiation_lookup_args.emplace_back(
								core::GenericValue(
									this->context.getSemaBuffer().getCharValue(default_value.charValueID()).value
								)
							);
						} break;

						default: evo::debugFatalBreak("Invalid template argument value");
					}
					instantiation_args.emplace_back(default_value);

				}else if constexpr(std::is_same<DefaultValue, TypeInfo::VoidableID>()){
					instantiation_lookup_args.emplace_back(default_value);
					instantiation_args.emplace_back(default_value);

				}else{
					static_assert(false, "Unsupported template default value type");
				}
			});
		}


		const bool is_method = templated_func.isMethod(this->context);

		{
			const size_t expected_num_args = ast_func.params.size() - size_t(is_member_call && is_method);
			const size_t got_num_args = args.size();

			if(expected_num_args != got_num_args){
				return evo::Unexpected<TemplateOverloadMatchFail>(
					TemplateOverloadMatchFail(TemplateOverloadMatchFail::WrongNumArgs(expected_num_args, got_num_args))
				);
			}
		}

		auto arg_types = evo::SmallVector<std::optional<TypeInfo::ID>>();
		arg_types.reserve(args.size());
		for(size_t i = size_t(is_method); const SymbolProc::TermInfoID arg_id : args){
			const TermInfo& arg = this->get_term_info(arg_id);
			if(arg.type_id.is<TypeInfo::ID>()){
				arg_types.emplace_back(arg.type_id.as<TypeInfo::ID>());

				if(templated_func.paramIsTemplate[i]){
					instantiation_lookup_args.emplace_back(arg.type_id.as<TypeInfo::ID>());
				}

			}else{
				if(templated_func.paramIsTemplate[i]){
					return evo::Unexpected<TemplateOverloadMatchFail>(
						TemplateOverloadMatchFail(TemplateOverloadMatchFail::CantDeduceArgType(i - size_t(is_method)))
					);
				}
				arg_types.emplace_back(std::nullopt);
			}

			i += 1;
		}


		const sema::TemplatedFunc::InstantiationInfo instantiation_info = 
			templated_func.createOrLookupInstantiation(std::move(instantiation_lookup_args));

		if(instantiation_info.needsToBeCompiled()){
			auto symbol_proc_builder = SymbolProcBuilder(
				this->context, this->context.source_manager[templated_func.symbolProc.source_id]
			);

			sema::ScopeManager& scope_manager = this->context.sema_buffer.scope_manager;

			const sema::ScopeManager::Scope::ID instantiation_sema_scope_id = 
				scope_manager.copyScope(*templated_func.symbolProc.sema_scope_id);

			
			///////////////////////////////////
			// build instantiation

			const evo::Result<SymbolProc::ID> instantiation_symbol_proc_id = symbol_proc_builder.buildTemplateInstance(
				templated_func.symbolProc,
				instantiation_info.instantiation,
				instantiation_sema_scope_id,
				*instantiation_info.instantiationID,
				std::move(arg_types)
			);
			if(instantiation_symbol_proc_id.isError()){
				return evo::Unexpected<TemplateOverloadMatchFail>(
					TemplateOverloadMatchFail(TemplateOverloadMatchFail::Handled())
				);
			}

			instantiation_info.instantiation.symbolProcID = instantiation_symbol_proc_id.value();


			///////////////////////////////////
			// add instantiation args to scope

			sema::ScopeManager::Scope& instantiation_sema_scope = scope_manager.getScope(instantiation_sema_scope_id);
			instantiation_sema_scope.pushLevel(scope_manager.createLevel());

			auto template_param_names = evo::SmallVector<Token::ID>();

			if(ast_func.templatePack.has_value()){
				const AST::TemplatePack& ast_template_pack =
					template_source.getASTBuffer().getTemplatePack(*ast_func.templatePack);

				for(const AST::TemplatePack::Param& template_param : ast_template_pack.params){
					template_param_names.emplace_back(template_param.ident);
				}
			}

			for(
				size_t i = 0;
				const evo::Variant<TypeInfo::VoidableID, sema::Expr>& instantiation_arg : instantiation_args
			){
				EVO_DEFER([&](){ i += 1; });

				const evo::Result<> add_ident_result = [&](){
					if(instantiation_arg.is<TypeInfo::VoidableID>()){
						return this->add_ident_to_scope(
							instantiation_sema_scope,
							template_source.getTokenBuffer()[template_param_names[i]].getString(),
							template_param_names[i],
							sema::ScopeLevel::TemplateTypeParamFlag{},
							instantiation_arg.as<TypeInfo::VoidableID>(),
							template_param_names[i]
						);
					}else{
						const AST::TemplatePack& ast_template_pack =
							template_source.getASTBuffer().getTemplatePack(*ast_func.templatePack);

						const TypeInfo::ID expr_type_id = [&]() -> TypeInfo::ID {
							if(templated_func.templateParams[i].typeID->isTemplateDeclInstantiation()){
								const evo::Result<TypeInfo::VoidableID> resolved_type = this->resolve_type(
									this->source.getASTBuffer().getType(ast_template_pack.params[i].type)
								);

								evo::debugAssert(resolved_type.isSuccess(), "Should have already checked not an error");

								evo::debugAssert(
									resolved_type.value().isVoid() == false, "Should have already checked not Void"
								);

								return resolved_type.value().asTypeID();
							}else{
								return *templated_func.templateParams[i].typeID;
							}
						}();

						return this->add_ident_to_scope(
							instantiation_sema_scope,
							this->source.getTokenBuffer()[ast_template_pack.params[i].ident].getString(),
							ast_template_pack.params[i].ident,
							sema::ScopeLevel::TemplateExprParamFlag{},
							expr_type_id,
							instantiation_arg.as<sema::Expr>(),
							ast_template_pack.params[i].ident	
						);
					}
				}();

				if(add_ident_result.isError()){
					return evo::Unexpected<TemplateOverloadMatchFail>(
						TemplateOverloadMatchFail(TemplateOverloadMatchFail::Handled())
					);
				}
			}


			///////////////////////////////////
			// done

			SymbolProc& created_symbol_proc =
				this->context.symbol_proc_manager.getSymbolProc(instantiation_symbol_proc_id.value());
			created_symbol_proc.setStatusInQueue();
			this->context.add_task_to_work_manager(instantiation_symbol_proc_id.value());

			return instantiation_info;

		}else{
			return instantiation_info;
		}
	}




	auto SemanticAnalyzer::expr_in_func_is_valid_value_stage(const TermInfo& term_info, const auto& node_location)
	-> bool {
		if(this->get_current_func().isConstexpr == false){ return true; }

		if(term_info.value_stage != TermInfo::ValueStage::RUNTIME){ return true; }

		this->emit_error(
			Diagnostic::Code::SEMA_EXPR_NOT_COMPTIME,
			node_location,
			"Expressions in a constexpr function cannot have a value stage of runtime"
		);

		return false;
	}





	auto SemanticAnalyzer::resolve_type(const AST::Type& type) -> evo::Result<TypeInfo::VoidableID> {
		auto base_type_id = std::optional<BaseType::ID>();
		switch(type.base.kind()){
			case AST::Kind::PRIMITIVE_TYPE: {
				evo::unimplemented("Resolve Type (PRIMITIVE_TYPE)");
			} break;

			case AST::Kind::IDENT: {
				const evo::Expected<TermInfo, Result> lookup_ident_result = this->lookup_ident_impl<true>(
					this->source.getASTBuffer().getIdent(type.base)
				);

				const TypeInfo::VoidableID looked_up_ident = 
					lookup_ident_result.value().type_id.as<TypeInfo::VoidableID>();

				if(looked_up_ident.isVoid()){
					if(type.qualifiers.empty() == false){
						this->emit_error(
							Diagnostic::Code::SEMA_VOID_WITH_QUALIFIERS,
							type.base,
							"Type \"Void\" cannot have qualifiers"
						);
						return evo::resultError;
					}

					return TypeInfo::VoidableID::Void();
				}

				base_type_id = this->context.getTypeManager().getTypeInfo(looked_up_ident.asTypeID()).baseTypeID();
			} break;

			case AST::Kind::DEDUCER: {
				evo::unimplemented("Resolve Type (DEDUCER)");
			} break;

			case AST::Kind::TEMPLATED_EXPR: {
				evo::unimplemented("Resolve Type (TEMPLATED_EXPR)");
			} break;

			case AST::Kind::INFIX: {
				evo::unimplemented("Resolve Type (INFIX)");
			} break;

			case AST::Kind::TYPEID_CONVERTER: {
				evo::unimplemented("Resolve Type (TYPEID_CONVERTER)");
			} break;

			default: evo::debugFatalBreak("Should not ever be invalid type");
		}


		return TypeInfo::VoidableID(this->context.type_manager.getOrCreateTypeInfo(TypeInfo(*base_type_id)));
	}




	auto SemanticAnalyzer::generic_value_to_sema_expr(const core::GenericValue& value, const TypeInfo& target_type)
	-> sema::Expr {
		switch(target_type.baseTypeID().kind()){
			case BaseType::Kind::DUMMY: evo::debugFatalBreak("Invalid type");

			case BaseType::Kind::PRIMITIVE: {
				const BaseType::Primitive& primitive_type = this->context.getTypeManager().getPrimitive(
					target_type.baseTypeID().primitiveID()
				);

				switch(primitive_type.kind()){
					case Token::Kind::TYPE_INT:      case Token::Kind::TYPE_ISIZE:
					case Token::Kind::TYPE_I_N:      case Token::Kind::TYPE_UINT:
					case Token::Kind::TYPE_USIZE:    case Token::Kind::TYPE_UI_N:
					case Token::Kind::TYPE_BYTE:     case Token::Kind::TYPE_TYPEID:
					case Token::Kind::TYPE_C_WCHAR:  case Token::Kind::TYPE_C_SHORT:
					case Token::Kind::TYPE_C_USHORT: case Token::Kind::TYPE_C_INT:
					case Token::Kind::TYPE_C_UINT:   case Token::Kind::TYPE_C_LONG:
					case Token::Kind::TYPE_C_ULONG:  case Token::Kind::TYPE_C_LONG_LONG:
					case Token::Kind::TYPE_C_ULONG_LONG: {
						return sema::Expr(
							this->context.sema_buffer.createIntValue(
								value.getInt(
									unsigned(this->context.getTypeManager().numBits(target_type.baseTypeID()))
								),
								target_type.baseTypeID()
							)
						);
					} break;

					case Token::Kind::TYPE_F16: {
						return sema::Expr(
							this->context.sema_buffer.createFloatValue(value.getF16(), target_type.baseTypeID())
						);
					} break;

					case Token::Kind::TYPE_BF16: {
						return sema::Expr(
							this->context.sema_buffer.createFloatValue(value.getBF16(), target_type.baseTypeID())
						);
					} break;

					case Token::Kind::TYPE_F32: {
						return sema::Expr(
							this->context.sema_buffer.createFloatValue(value.getF32(), target_type.baseTypeID())
						);
					} break;

					case Token::Kind::TYPE_F64: {
						return sema::Expr(
							this->context.sema_buffer.createFloatValue(value.getF64(), target_type.baseTypeID())
						);
					} break;

					case Token::Kind::TYPE_F80: {
						return sema::Expr(
							this->context.sema_buffer.createFloatValue(value.getF80(), target_type.baseTypeID())
						);
					} break;

					case Token::Kind::TYPE_F128: {
					 	return sema::Expr(
					 		this->context.sema_buffer.createFloatValue(value.getF128(), target_type.baseTypeID())
					 	);
				 	} break;

					case Token::Kind::TYPE_C_LONG_DOUBLE: {
						if(this->context.getTypeManager().numBits(target_type.baseTypeID()) == 64){
							return sema::Expr(
								this->context.sema_buffer.createFloatValue(value.getF64(), target_type.baseTypeID())
							);
						}else{
							return sema::Expr(
								this->context.sema_buffer.createFloatValue(value.getF80(), target_type.baseTypeID())
							);
						}
					} break;

					case Token::Kind::TYPE_BOOL: {
						return sema::Expr(this->context.sema_buffer.createBoolValue(value.getBool()));
					} break;

					case Token::Kind::TYPE_CHAR: {
						return sema::Expr(this->context.sema_buffer.createCharValue(value.getChar()));
					} break;

					case Token::Kind::TYPE_RAWPTR: evo::unimplemented("Token::Kind::TYPE_RAWPTR");

					default: evo::debugFatalBreak("Invalid type");
				}
			} break;

			case BaseType::Kind::FUNCTION: {
				evo::unimplemented("generic_value_to_sema_expr - BaseType::Kind::FUNCTION");
			} break;

			case BaseType::Kind::ARRAY: {
				const BaseType::Array& array_type = this->context.getTypeManager().getArray(
					target_type.baseTypeID().arrayID()
				);

				const uint64_t num_elems = [&](){
					uint64_t total_num_elems = 1;
					for(uint64_t dimension : array_type.dimensions){
						total_num_elems *= dimension;
					}
					if(array_type.terminator.has_value()){
						total_num_elems += 1;
					}
					return total_num_elems;
				}();

				const uint64_t elem_size = this->context.getTypeManager().numBytes(array_type.elementTypeID);
				const TypeInfo& elem_type_info = this->context.getTypeManager().getTypeInfo(array_type.elementTypeID);

				auto member_vals = evo::SmallVector<sema::Expr>();
				member_vals.reserve(size_t(num_elems));


				for(size_t i = 0; i < num_elems; i+=1){
					const auto elem_range = evo::ArrayProxy<std::byte>(&value.dataRange()[i * elem_size], elem_size);

					member_vals.emplace_back(
						this->generic_value_to_sema_expr(core::GenericValue::fromData(elem_range), elem_type_info)
					);
				}

				return sema::Expr(
					this->context.sema_buffer.createAggregateValue(std::move(member_vals), target_type.baseTypeID())
				);
			} break;

			case BaseType::Kind::ARRAY_DEDUCER: {
				evo::debugFatalBreak("Function cannot return an array deducer");
			} break;

			case BaseType::Kind::ARRAY_REF: {
				evo::unimplemented("BaseType::Kind::ARRAY_REF"); // TODO(FUTURE): handling underlying data???
			} break;

			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias_type = this->context.getTypeManager().getAlias(
					target_type.baseTypeID().aliasID()
				);

				return this->generic_value_to_sema_expr(
					value, this->context.getTypeManager().getTypeInfo(*alias_type.aliasedType.load())
				);
			} break;

			case BaseType::Kind::DISTINCT_ALIAS: {
				const BaseType::DistinctAlias& distinct_alias_type = this->context.getTypeManager().getDistinctAlias(
					target_type.baseTypeID().distinctAliasID()
				);

				return this->generic_value_to_sema_expr(
					value, this->context.getTypeManager().getTypeInfo(*distinct_alias_type.underlyingType.load())
				);
			} break;

			case BaseType::Kind::STRUCT: {
				const BaseType::Struct& struct_type = this->context.getTypeManager().getStruct(
					target_type.baseTypeID().structID()
				);

				size_t offset = 0;

				auto member_vals = evo::SmallVector<sema::Expr>();
				member_vals.reserve(struct_type.memberVarsABI.size());

				for(const BaseType::Struct::MemberVar* member_var : struct_type.memberVarsABI){
					const size_t member_size = this->context.getTypeManager().numBytes(member_var->typeID);

					const auto member_range = evo::ArrayProxy<std::byte>(&value.dataRange()[offset], member_size);
					member_vals.emplace_back(
						this->generic_value_to_sema_expr(
							core::GenericValue::fromData(member_range),
							this->context.getTypeManager().getTypeInfo(member_var->typeID)
						)
					);

					offset += member_size;
				}

				return sema::Expr(
					this->context.sema_buffer.createAggregateValue(std::move(member_vals), target_type.baseTypeID())
				);
			} break;

			case BaseType::Kind::STRUCT_TEMPLATE: {
				evo::debugFatalBreak("Function cannot return a struct template");
			} break;

			case BaseType::Kind::STRUCT_TEMPLATE_DEDUCER: {
				evo::debugFatalBreak("Function cannot return a struct template deducer");
			} break;

			case BaseType::Kind::UNION: {
				// const BaseType::Union& union_type = this->context.getTypeManager().getUnion(
				// 	target_type.baseTypeID().unionID()
				// );

				const uint64_t num_elems = this->context.getTypeManager().numBytes(target_type.baseTypeID());

				auto member_vals = evo::SmallVector<sema::Expr>();
				member_vals.reserve(size_t(num_elems));


				for(size_t i = 0; i < num_elems; i+=1){
					const auto elem_range = evo::ArrayProxy<std::byte>(&value.dataRange()[i], 1);

					member_vals.emplace_back(
						this->generic_value_to_sema_expr(
							core::GenericValue::fromData(elem_range),
							this->context.getTypeManager().getTypeInfo(TypeManager::getTypeByte())
						)
					);
				}

				return sema::Expr(
					this->context.sema_buffer.createAggregateValue(std::move(member_vals), target_type.baseTypeID())
				);
			} break;

			case BaseType::Kind::ENUM: {
				const BaseType::Enum& enum_type =
					this->context.getTypeManager().getEnum(target_type.baseTypeID().enumID());

				return this->generic_value_to_sema_expr(value, TypeInfo(BaseType::ID(enum_type.underlyingTypeID)));
			} break;

			case BaseType::Kind::TYPE_DEDUCER: {
				evo::debugFatalBreak("Function cannot return a type deducer");
			} break;

			case BaseType::Kind::INTERFACE: {
				evo::debugFatalBreak("Function cannot return an interface");
			} break;

			case BaseType::Kind::INTERFACE_IMPL_INSTANTIATION: {
				const BaseType::InterfaceImplInstantiation& interface_impl_instantiation_info =
					this->context.getTypeManager().getInterfaceImplInstantiation(
						target_type.baseTypeID().interfaceImplInstantiationID()
					);

				return this->generic_value_to_sema_expr(
					value,
					this->context.getTypeManager().getTypeInfo(
						interface_impl_instantiation_info.implInstantiationTypeID
					)
				);
			} break;
		}

		evo::unreachable();
	}




	auto SemanticAnalyzer::sema_expr_to_generic_value(const sema::Expr& expr) -> core::GenericValue {
		switch(expr.kind()){
			case sema::Expr::Kind::INT_VALUE: {
				return core::GenericValue(this->context.getSemaBuffer().getIntValue(expr.intValueID()).value);
			} break;

			case sema::Expr::Kind::FLOAT_VALUE: {
				return core::GenericValue(this->context.getSemaBuffer().getFloatValue(expr.floatValueID()).value);
			} break;

			case sema::Expr::Kind::BOOL_VALUE: {
				return core::GenericValue(this->context.getSemaBuffer().getBoolValue(expr.boolValueID()).value);
			} break;

			case sema::Expr::Kind::STRING_VALUE: {
				return core::GenericValue(
					std::string_view(this->context.getSemaBuffer().getStringValue(expr.stringValueID()).value)
				);
			} break;

			case sema::Expr::Kind::AGGREGATE_VALUE: {
				const sema::AggregateValue& aggregate_value =
					this->context.getSemaBuffer().getAggregateValue(expr.aggregateValueID());

				const size_t output_size = this->context.getTypeManager().numBytes(aggregate_value.typeID);
				core::GenericValue output = core::GenericValue::createUninit(output_size);

				if(aggregate_value.typeID.kind() == BaseType::Kind::STRUCT){
					const BaseType::Struct& struct_type = 
						this->context.getTypeManager().getStruct(aggregate_value.typeID.structID());

					size_t offset = 0;

					for(size_t i = 0; const BaseType::Struct::MemberVar* member : struct_type.memberVarsABI){
						const core::GenericValue member_val =
							this->sema_expr_to_generic_value(aggregate_value.values[i]);

						const size_t member_size = this->context.getTypeManager().numBytes(member->typeID);

						std::memcpy(&output.writableDataRange()[offset], member_val.dataRange().data(), member_size);

						offset += member_size;

						i += 1;
					}

				}else if(aggregate_value.typeID.kind() == BaseType::Kind::ARRAY){
					const BaseType::Array& array_type = 
						this->context.getTypeManager().getArray(aggregate_value.typeID.arrayID());

					const size_t elem_size = this->context.getTypeManager().numBytes(array_type.elementTypeID);

					const size_t num_elems = output_size / elem_size;
					for(size_t i = 0; i < num_elems; i+=1){
						const core::GenericValue elem_val = this->sema_expr_to_generic_value(aggregate_value.values[i]);

						std::memcpy(&output.writableDataRange()[i * elem_size], elem_val.dataRange().data(), elem_size);
					}

				}else{
					evo::debugAssert(aggregate_value.typeID.kind() == BaseType::Kind::UNION, "Unknown aggregate type");

					const size_t num_elems = this->context.getTypeManager().numBytes(aggregate_value.typeID);
					for(size_t i = 0; i < num_elems; i+=1){
						const core::GenericValue elem_val = this->sema_expr_to_generic_value(aggregate_value.values[i]);

						output.writableDataRange()[i] = *elem_val.dataRange().data();
					}
				}

				return output;
			} break;

			case sema::Expr::Kind::CHAR_VALUE: {
				return core::GenericValue(this->context.getSemaBuffer().getCharValue(expr.charValueID()).value);
			} break;

			default: evo::debugFatalBreak("Invalid constexpr value");
		}
	}





	auto SemanticAnalyzer::get_project_config() const -> const Source::ProjectConfig& {
		return this->context.getSourceManager().getSourceProjectConfig(this->source.getProjectConfigID());
	}



	auto SemanticAnalyzer::extract_deducers(TypeInfo::VoidableID deducer_id, TypeInfo::VoidableID got_type_id)
	-> evo::Result<evo::SmallVector<DeducedTerm>> {
		if(deducer_id.isVoid()){
			if(got_type_id.isVoid()){
				return evo::SmallVector<DeducedTerm>();
			}else{
				return evo::resultError;
			}

		}else{
			if(got_type_id.isVoid()){
				return evo::resultError;
			}else{
				return this->extract_deducers(deducer_id.asTypeID(), got_type_id.asTypeID());
			}
		}
	}


	auto SemanticAnalyzer::extract_deducers(TypeInfo::ID deducer_id, TypeInfo::ID got_type_id)
	-> evo::Result<evo::SmallVector<DeducedTerm>> {
		const TypeManager& type_manager = this->context.getTypeManager();

		auto output = evo::SmallVector<DeducedTerm>();

		const TypeInfo& deducer  = type_manager.getTypeInfo(deducer_id);
		const TypeInfo& got_type = type_manager.getTypeInfo(got_type_id);

		if(deducer.qualifiers().size() < got_type.qualifiers().size()){
			for(size_t i = 0; i < deducer.qualifiers().size(); i+=1){
				if(deducer.qualifiers()[i] != got_type.qualifiers()[i - got_type.qualifiers().size()]){
					return evo::resultError;
				}
			}

		}else if(deducer.qualifiers().size() == got_type.qualifiers().size()){
			if(deducer.qualifiers() != got_type.qualifiers()){ return evo::resultError; }
			
		}else{ // deducer.qualifiers().size() > got_type.qualifiers().size()
			return evo::resultError;
		}


		switch(deducer.baseTypeID().kind()){
			case BaseType::Kind::TYPE_DEDUCER: {
				const BaseType::TypeDeducer& type_deducer = 
					type_manager.getTypeDeducer(deducer.baseTypeID().typeDeducerID());

				const Token& type_deducer_token = this->source.getTokenBuffer()[type_deducer.identTokenID];

				if(type_deducer_token.kind() == Token::Kind::ANONYMOUS_DEDUCER){
					return output;
				}

				if(deducer.qualifiers().empty()){
					output.emplace_back(got_type_id, type_deducer.identTokenID);
				}else{
					auto qualifiers = evo::SmallVector<AST::Type::Qualifier>();

					for(size_t i = 0; i < got_type.qualifiers().size() - deducer.qualifiers().size(); i+=1){
						qualifiers.emplace_back(got_type.qualifiers()[i]);
					}

					output.emplace_back(
						this->context.type_manager.getOrCreateTypeInfo(
							TypeInfo(got_type.baseTypeID(), std::move(qualifiers))
						),
						type_deducer.identTokenID
					);
				}
			} break;

			case BaseType::Kind::ARRAY: {
				if(got_type.baseTypeID().kind() != BaseType::Kind::ARRAY){ return evo::resultError; }

				const BaseType::Array& deducer_array_type = 
					this->context.getTypeManager().getArray(deducer.baseTypeID().arrayID());

				const BaseType::Array& got_array_type = 
					this->context.getTypeManager().getArray(got_type.baseTypeID().arrayID());

				if(deducer_array_type.dimensions != got_array_type.dimensions){ return evo::resultError; }
				if(deducer_array_type.terminator != got_array_type.terminator){ return evo::resultError; }

				const evo::Result<evo::SmallVector<DeducedTerm>> arr_deduced = this->extract_deducers(
					deducer_array_type.elementTypeID, got_array_type.elementTypeID
				);
				if(arr_deduced.isError()){ return evo::resultError; }

				output.reserve(output.size() + arr_deduced.value().size());
				for(const DeducedTerm& arr_deduced_term : arr_deduced.value()){
					output.emplace_back(arr_deduced_term);
				}
			} break;

			case BaseType::Kind::ARRAY_REF: {
				if(got_type.baseTypeID().kind() != BaseType::Kind::ARRAY_REF){ return evo::resultError; }

				const BaseType::ArrayRef& deducer_array_ref_type = 
					this->context.getTypeManager().getArrayRef(deducer.baseTypeID().arrayRefID());

				const BaseType::ArrayRef& got_array_ref_type = 
					this->context.getTypeManager().getArrayRef(got_type.baseTypeID().arrayRefID());

				if(deducer_array_ref_type.isReadOnly != got_array_ref_type.isReadOnly){ return evo::resultError; }
				if(deducer_array_ref_type.terminator != got_array_ref_type.terminator){ return evo::resultError; }

				const evo::Result<evo::SmallVector<DeducedTerm>> arr_deduced = this->extract_deducers(
					deducer_array_ref_type.elementTypeID, got_array_ref_type.elementTypeID
				);
				if(arr_deduced.isError()){ return evo::resultError; }

				output.reserve(output.size() + arr_deduced.value().size());
				for(const DeducedTerm& arr_deduced_term : arr_deduced.value()){
					output.emplace_back(arr_deduced_term);
				}
			} break;

			case BaseType::Kind::ARRAY_DEDUCER: {
				if(got_type.baseTypeID().kind() != BaseType::Kind::ARRAY){ return evo::resultError; }

				const BaseType::ArrayDeducer& deducer_array_deducer_type = 
					this->context.getTypeManager().getArrayDeducer(deducer.baseTypeID().arrayDeducerID());

				const BaseType::Array& got_array_type = 
					this->context.getTypeManager().getArray(got_type.baseTypeID().arrayID());

				const evo::Result<evo::SmallVector<DeducedTerm>> arr_deduced = this->extract_deducers(
					deducer_array_deducer_type.elementTypeID, got_array_type.elementTypeID
				);
				if(arr_deduced.isError()){ return evo::resultError; }

				output.reserve(std::bit_ceil(output.size() + arr_deduced.value().size()));
				for(const DeducedTerm& arr_deduced_term : arr_deduced.value()){
					output.emplace_back(arr_deduced_term);
				}


				if(deducer_array_deducer_type.dimensions.size() != got_array_type.dimensions.size()){
					return evo::resultError;
				}

				for(size_t i = 0; i < deducer_array_deducer_type.dimensions.size(); i+=1){
					if(deducer_array_deducer_type.dimensions[i].is<uint64_t>()){
						if(deducer_array_deducer_type.dimensions[i].as<uint64_t>() != got_array_type.dimensions[i]){
							return evo::resultError;
						}

					}else{
						const Token& deducer_token = 
							this->source.getTokenBuffer()[deducer_array_deducer_type.dimensions[i].as<Token::ID>()];

						if(deducer_token.kind() == Token::Kind::DEDUCER){
							const sema::Expr created_int_value = sema::Expr(
								this->context.sema_buffer.createIntValue(
									core::GenericInt(
										unsigned(this->context.getTypeManager().numBitsOfPtr()),
										got_array_type.dimensions[i]
									),
									this->context.getTypeManager().getTypeInfo(TypeManager::getTypeUSize()).baseTypeID()
								)
							);

							output.emplace_back(
								DeducedTerm::Expr(TypeManager::getTypeUSize(), created_int_value),
								deducer_array_deducer_type.dimensions[i].as<Token::ID>()
							);

						}else{
							evo::debugAssert(
								deducer_token.kind() == Token::Kind::ANONYMOUS_DEDUCER, "Unknown deducer kind"
							);
						}
					}
				}

				if(deducer_array_deducer_type.terminator.is<std::monostate>()){
					if(got_array_type.terminator.has_value()){ return evo::resultError; }

				}else if(deducer_array_deducer_type.terminator.is<core::GenericValue>()){
					if(deducer_array_deducer_type.terminator.as<core::GenericValue>() != *got_array_type.terminator){
						return evo::resultError;
					}

				}else{
					const Token& deducer_token = 
						this->source.getTokenBuffer()[deducer_array_deducer_type.terminator.as<Token::ID>()];

					if(deducer_token.kind() == Token::Kind::DEDUCER){
						output.emplace_back(
							DeducedTerm::Expr(
								got_array_type.elementTypeID,
								this->generic_value_to_sema_expr(
									*got_array_type.terminator,
									this->context.getTypeManager().getTypeInfo(got_array_type.elementTypeID)
								)
							),
							deducer_array_deducer_type.terminator.as<Token::ID>()
						);

					}else{
						evo::debugAssert(
							deducer_token.kind() == Token::Kind::ANONYMOUS_DEDUCER, "Unknown deducer kind"
						);
					}
				}
			} break;

			case BaseType::Kind::STRUCT_TEMPLATE_DEDUCER: {
				if(got_type.baseTypeID().kind() != BaseType::Kind::STRUCT){ return evo::resultError; }

				const BaseType::StructTemplateDeducer& struct_template_deducer =
					this->context.getTypeManager().getStructTemplateDeducer(
						deducer.baseTypeID().structTemplateDeducerID()
					);

				const BaseType::Struct& got_struct_type = 
					this->context.getTypeManager().getStruct(got_type.baseTypeID().structID());

				if(got_struct_type.templateID.has_value() == false){ return evo::resultError; }
				if(struct_template_deducer.structTemplateID != *got_struct_type.templateID){ return evo::resultError; }


				const BaseType::StructTemplate& got_struct_template =
					this->context.getTypeManager().getStructTemplate(*got_struct_type.templateID);


				const evo::SmallVector<BaseType::StructTemplate::Arg> got_template_args =
					got_struct_template.getInstantiationArgs(got_struct_type.instantiation);


				for(size_t i = 0; i < struct_template_deducer.args.size(); i+=1){
					const BaseType::StructTemplate::Arg& deducer_arg = struct_template_deducer.args[i];
					const BaseType::StructTemplate::Arg& got_arg = got_template_args[i];

					if(got_arg.is<TypeInfo::VoidableID>()){
						const evo::Result<evo::SmallVector<DeducedTerm>> struct_deduced = this->extract_deducers(
							deducer_arg.as<TypeInfo::VoidableID>(), got_arg.as<TypeInfo::VoidableID>()
						);
						if(struct_deduced.isError()){ return evo::resultError; }

						output.reserve(std::bit_ceil(output.size() + struct_deduced.value().size()));
						for(const DeducedTerm& struct_deduced_term : struct_deduced.value()){
							output.emplace_back(struct_deduced_term);
						}

					}else if(deducer_arg.is<TypeInfo::VoidableID>()){ // arg is deducer
						const TypeInfo& deducer_arg_deducer_type_info = this->context.getTypeManager().getTypeInfo(
							deducer_arg.as<TypeInfo::VoidableID>().asTypeID()
						);

						const BaseType::TypeDeducer& deducer_arg_deducer = 
							this->context.getTypeManager().getTypeDeducer(
								deducer_arg_deducer_type_info.baseTypeID().typeDeducerID()
							);


						const Token& deducer_token = this->source.getTokenBuffer()[deducer_arg_deducer.identTokenID];

						if(deducer_token.kind() == Token::Kind::DEDUCER){
							const TypeInfo::ID arg_type_id = *got_struct_template.params[i].typeID;

							output.emplace_back(
								DeducedTerm::Expr(
									arg_type_id,
									this->generic_value_to_sema_expr(
										got_arg.as<core::GenericValue>(),
										this->context.getTypeManager().getTypeInfo(arg_type_id)
									)
								),
								deducer_arg_deducer.identTokenID
							);
						}

					}else{
						if(deducer_arg.as<core::GenericValue>() != got_arg.as<core::GenericValue>()){
							return evo::resultError;
						}
					}
				}
			} break;

			default: {
				if(deducer.baseTypeID() != got_type.baseTypeID()){ return evo::resultError; }
			} break;
		}

		return output;
	}


	auto SemanticAnalyzer::add_deduced_terms_to_scope(evo::ArrayProxy<DeducedTerm> deduced_terms) -> evo::Result<> {
		for(const DeducedTerm& deduced_term : deduced_terms){
			if(deduced_term.value.is<TypeInfo::VoidableID>()){
				if(
					this->add_ident_to_scope(
						this->source.getTokenBuffer()[deduced_term.tokenID].getString(),
						deduced_term.tokenID,
						sema::ScopeLevel::DeducedTypeFlag{},
						deduced_term.value.as<TypeInfo::VoidableID>(),
						deduced_term.tokenID
					).isError()
				){
					return evo::resultError;
				}

			}else{
				if(
					this->add_ident_to_scope(
						this->source.getTokenBuffer()[deduced_term.tokenID].getString(),
						deduced_term.tokenID,
						sema::ScopeLevel::DeducedExprFlag{},
						deduced_term.value.as<DeducedTerm::Expr>().type_id,
						deduced_term.value.as<DeducedTerm::Expr>().expr,
						deduced_term.tokenID
					).isError()
				){
					return evo::resultError;
				}
			}
		}

		return evo::Result<>();
	}




	auto SemanticAnalyzer::infix_overload_impl(
		const std::unordered_multimap<Token::Kind, sema::Func::ID>& infix_overloads,
		TermInfo& lhs,
		TermInfo& rhs,
		const AST::Infix& ast_infix
	) -> evo::Expected<sema::FuncCall::ID, Result> {
		const Token::Kind op_kind = this->source.getTokenBuffer()[ast_infix.opTokenID].kind();

		const auto [begin_overloads_range, end_overloads_range] = infix_overloads.equal_range(op_kind);
		const auto overloads_range = evo::IterRange(begin_overloads_range, end_overloads_range);

		if(overloads_range.empty()){
			auto infos = evo::SmallVector<Diagnostic::Info>();
			this->diagnostic_print_type_info(lhs.type_id.as<TypeInfo::ID>(), infos, "Infix LHS type: ");
			this->diagnostic_print_type_info(rhs.type_id.as<TypeInfo::ID>(), infos, "Infix RHS type: ");
			this->emit_error(
				Diagnostic::Code::SEMA_MATH_INFIX_NO_MATCHING_OP,
				ast_infix,
				"No matching operation for these arguments",
				std::move(infos)
			);
			return evo::Unexpected(Result::ERROR);
		}

		auto overloads_list = evo::SmallVector<SelectFuncOverloadFuncInfo, 4>();
		overloads_list.reserve(overloads_range.size());
		for(const auto& [_, overload_id] : overloads_range){
			const sema::Func& overload = this->context.getSemaBuffer().getFunc(overload_id);
			overloads_list.emplace_back(
				overload_id, this->context.getTypeManager().getFunction(overload.typeID)
			);
		}

		auto arg_infos = evo::SmallVector<SelectFuncOverloadArgInfo>{
			SelectFuncOverloadArgInfo(lhs, ast_infix.lhs, std::nullopt),
			SelectFuncOverloadArgInfo(rhs, ast_infix.rhs, std::nullopt)
		};

		const evo::Result<size_t> selected_overload_index = this->select_func_overload(
			overloads_list, arg_infos, ast_infix, false, evo::SmallVector<Diagnostic::Info>()
		);

		if(selected_overload_index.isError()){ return evo::Unexpected(Result::ERROR); }

		const sema::Func::ID selected_overload_id =
			overloads_list[selected_overload_index.value()].func_id.as<sema::Func::ID>();

		if(this->get_current_func().isConstexpr){
			const sema::Func& infix_op_sema_func = this->context.getSemaBuffer().getFunc(selected_overload_id);
			if(infix_op_sema_func.isConstexpr == false){
				this->emit_error(
					Diagnostic::Code::SEMA_FUNC_ISNT_CONSTEXPR,
					ast_infix.opTokenID,
					"Cannot call a non-constexpr operator overload within a constexpr function",
					Diagnostic::Info(
						"Called operator overload was defined here:", this->get_location(selected_overload_id)
					)
				);
				return evo::Unexpected(Result::ERROR);
			}

			this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>()
				.dependent_funcs.emplace(selected_overload_id);
		}

		this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>().dependent_funcs.emplace(selected_overload_id);

		return this->context.sema_buffer.createFuncCall(
			selected_overload_id, evo::SmallVector<sema::Expr>{lhs.getExpr(), rhs.getExpr()}
		);
	}



	auto SemanticAnalyzer::constexpr_infix_math(
		Token::Kind op, sema::Expr lhs, sema::Expr rhs, std::optional<TypeInfo::ID> lhs_type
	) -> TermInfo {
		auto constexpr_intrinsic_evaluator = ConstexprIntrinsicEvaluator(
			this->context.type_manager, this->context.sema_buffer
		);

		TermInfo output = [&](){
			switch(op){
				case Token::lookupKind("=="): {
					if(lhs.kind() == sema::Expr::Kind::INT_VALUE){
						return constexpr_intrinsic_evaluator.eq(
							lhs_type.value_or(TypeManager::getTypeI256()),
							this->context.sema_buffer.getIntValue(lhs.intValueID()).value,
							this->context.sema_buffer.getIntValue(rhs.intValueID()).value
						);

					}else if(lhs.kind() == sema::Expr::Kind::BOOL_VALUE){
						return constexpr_intrinsic_evaluator.eq(
							this->context.sema_buffer.getBoolValue(lhs.boolValueID()).value,
							this->context.sema_buffer.getBoolValue(rhs.boolValueID()).value
						);

					}else{
						return constexpr_intrinsic_evaluator.eq(
							lhs_type.value_or(TypeManager::getTypeF128()),
							this->context.sema_buffer.getFloatValue(lhs.floatValueID()).value,
							this->context.sema_buffer.getFloatValue(rhs.floatValueID()).value
						);
					}
				} break;

				case Token::lookupKind("!="): {
					if(lhs.kind() == sema::Expr::Kind::INT_VALUE){
						return constexpr_intrinsic_evaluator.neq(
							lhs_type.value_or(TypeManager::getTypeI256()),
							this->context.sema_buffer.getIntValue(lhs.intValueID()).value,
							this->context.sema_buffer.getIntValue(rhs.intValueID()).value
						);

					}else if(lhs.kind() == sema::Expr::Kind::BOOL_VALUE){
						return constexpr_intrinsic_evaluator.neq(
							this->context.sema_buffer.getBoolValue(lhs.boolValueID()).value,
							this->context.sema_buffer.getBoolValue(rhs.boolValueID()).value
						);

					}else{
						return constexpr_intrinsic_evaluator.neq(
							lhs_type.value_or(TypeManager::getTypeF128()),
							this->context.sema_buffer.getFloatValue(lhs.floatValueID()).value,
							this->context.sema_buffer.getFloatValue(rhs.floatValueID()).value
						);
					}
				} break;

				case Token::lookupKind("<"): {
					if(lhs.kind() == sema::Expr::Kind::INT_VALUE){
						return constexpr_intrinsic_evaluator.lt(
							lhs_type.value_or(TypeManager::getTypeI256()),
							this->context.sema_buffer.getIntValue(lhs.intValueID()).value,
							this->context.sema_buffer.getIntValue(rhs.intValueID()).value
						);

					}else if(lhs.kind() == sema::Expr::Kind::BOOL_VALUE){
						return constexpr_intrinsic_evaluator.lt(
							this->context.sema_buffer.getBoolValue(lhs.boolValueID()).value,
							this->context.sema_buffer.getBoolValue(rhs.boolValueID()).value
						);

					}else{
						return constexpr_intrinsic_evaluator.lt(
							lhs_type.value_or(TypeManager::getTypeF128()),
							this->context.sema_buffer.getFloatValue(lhs.floatValueID()).value,
							this->context.sema_buffer.getFloatValue(rhs.floatValueID()).value
						);
					}
				} break;

				case Token::lookupKind("<="): {
					if(lhs.kind() == sema::Expr::Kind::INT_VALUE){
						return constexpr_intrinsic_evaluator.lte(
							lhs_type.value_or(TypeManager::getTypeI256()),
							this->context.sema_buffer.getIntValue(lhs.intValueID()).value,
							this->context.sema_buffer.getIntValue(rhs.intValueID()).value
						);

					}else if(lhs.kind() == sema::Expr::Kind::BOOL_VALUE){
						return constexpr_intrinsic_evaluator.lte(
							this->context.sema_buffer.getBoolValue(lhs.boolValueID()).value,
							this->context.sema_buffer.getBoolValue(rhs.boolValueID()).value
						);

					}else{
						return constexpr_intrinsic_evaluator.lte(
							lhs_type.value_or(TypeManager::getTypeF128()),
							this->context.sema_buffer.getFloatValue(lhs.floatValueID()).value,
							this->context.sema_buffer.getFloatValue(rhs.floatValueID()).value
						);
					}
				} break;

				case Token::lookupKind(">"): {
					if(lhs.kind() == sema::Expr::Kind::INT_VALUE){
						return constexpr_intrinsic_evaluator.gt(
							lhs_type.value_or(TypeManager::getTypeI256()),
							this->context.sema_buffer.getIntValue(lhs.intValueID()).value,
							this->context.sema_buffer.getIntValue(rhs.intValueID()).value
						);

					}else if(lhs.kind() == sema::Expr::Kind::BOOL_VALUE){
						return constexpr_intrinsic_evaluator.gt(
							this->context.sema_buffer.getBoolValue(lhs.boolValueID()).value,
							this->context.sema_buffer.getBoolValue(rhs.boolValueID()).value
						);

					}else{
						return constexpr_intrinsic_evaluator.gt(
							lhs_type.value_or(TypeManager::getTypeF128()),
							this->context.sema_buffer.getFloatValue(lhs.floatValueID()).value,
							this->context.sema_buffer.getFloatValue(rhs.floatValueID()).value
						);
					}
				} break;

				case Token::lookupKind(">="): {
					if(lhs.kind() == sema::Expr::Kind::INT_VALUE){
						return constexpr_intrinsic_evaluator.gte(
							lhs_type.value_or(TypeManager::getTypeI256()),
							this->context.sema_buffer.getIntValue(lhs.intValueID()).value,
							this->context.sema_buffer.getIntValue(rhs.intValueID()).value
						);

					}else if(lhs.kind() == sema::Expr::Kind::BOOL_VALUE){
						return constexpr_intrinsic_evaluator.gte(
							this->context.sema_buffer.getBoolValue(lhs.boolValueID()).value,
							this->context.sema_buffer.getBoolValue(rhs.boolValueID()).value
						);

					}else{
						return constexpr_intrinsic_evaluator.gte(
							lhs_type.value_or(TypeManager::getTypeF128()),
							this->context.sema_buffer.getFloatValue(lhs.floatValueID()).value,
							this->context.sema_buffer.getFloatValue(rhs.floatValueID()).value
						);
					}
				} break;

				case Token::lookupKind("&"): {
					if(lhs.kind() == sema::Expr::Kind::BOOL_VALUE){
						return constexpr_intrinsic_evaluator.bitwiseAnd(
							this->context.sema_buffer.getBoolValue(lhs.boolValueID()).value,
							this->context.sema_buffer.getBoolValue(rhs.boolValueID()).value
						);

					}else{
						return constexpr_intrinsic_evaluator.bitwiseAnd(
							lhs_type.value_or(TypeManager::getTypeI256()),
							this->context.sema_buffer.getIntValue(lhs.intValueID()).value,
							this->context.sema_buffer.getIntValue(rhs.intValueID()).value
						);
					}
				} break;

				case Token::lookupKind("|"): {
					if(lhs.kind() == sema::Expr::Kind::BOOL_VALUE){
						return constexpr_intrinsic_evaluator.bitwiseOr(
							this->context.sema_buffer.getBoolValue(lhs.boolValueID()).value,
							this->context.sema_buffer.getBoolValue(rhs.boolValueID()).value
						);

					}else{
						return constexpr_intrinsic_evaluator.bitwiseOr(
							lhs_type.value_or(TypeManager::getTypeI256()),
							this->context.sema_buffer.getIntValue(lhs.intValueID()).value,
							this->context.sema_buffer.getIntValue(rhs.intValueID()).value
						);
					}
				} break;

				case Token::lookupKind("^"): {
					if(lhs.kind() == sema::Expr::Kind::BOOL_VALUE){
						return constexpr_intrinsic_evaluator.bitwiseXor(
							this->context.sema_buffer.getBoolValue(lhs.boolValueID()).value,
							this->context.sema_buffer.getBoolValue(rhs.boolValueID()).value
						);

					}else{
						return constexpr_intrinsic_evaluator.bitwiseXor(
							lhs_type.value_or(TypeManager::getTypeI256()),
							this->context.sema_buffer.getIntValue(lhs.intValueID()).value,
							this->context.sema_buffer.getIntValue(rhs.intValueID()).value
						);
					}
				} break;

				case Token::lookupKind("<<"): {
					return constexpr_intrinsic_evaluator.shl(
						lhs_type.value_or(TypeManager::getTypeI256()),
						true,
						this->context.sema_buffer.getIntValue(lhs.intValueID()).value,
						this->context.sema_buffer.getIntValue(rhs.intValueID()).value
					).value();
				} break;

				case Token::lookupKind("<<|"): {
					return constexpr_intrinsic_evaluator.shlSat(
						lhs_type.value_or(TypeManager::getTypeI256()),
						this->context.sema_buffer.getIntValue(lhs.intValueID()).value,
						this->context.sema_buffer.getIntValue(rhs.intValueID()).value
					);
				} break;

				case Token::lookupKind(">>"): {
					return constexpr_intrinsic_evaluator.shr(
						lhs_type.value_or(TypeManager::getTypeI256()),
						true,
						this->context.sema_buffer.getIntValue(lhs.intValueID()).value,
						this->context.sema_buffer.getIntValue(rhs.intValueID()).value
					).value();
				} break;

				case Token::lookupKind("+"): {
					if(lhs.kind() == sema::Expr::Kind::INT_VALUE){
						return constexpr_intrinsic_evaluator.add(
							lhs_type.value_or(TypeManager::getTypeI256()),
							true,
							this->context.sema_buffer.getIntValue(lhs.intValueID()).value,
							this->context.sema_buffer.getIntValue(rhs.intValueID()).value
						).value();

					}else{
						return constexpr_intrinsic_evaluator.fadd(
							lhs_type.value_or(TypeManager::getTypeF128()),
							this->context.sema_buffer.getFloatValue(lhs.floatValueID()).value,
							this->context.sema_buffer.getFloatValue(rhs.floatValueID()).value
						);
					}
				} break;

				case Token::lookupKind("+%"): {
					return constexpr_intrinsic_evaluator.add(
						lhs_type.value_or(TypeManager::getTypeI256()),
						true,
						this->context.sema_buffer.getIntValue(lhs.intValueID()).value,
						this->context.sema_buffer.getIntValue(rhs.intValueID()).value
					).value();
				} break;

				case Token::lookupKind("+|"): {
					return constexpr_intrinsic_evaluator.addSat(
						lhs_type.value_or(TypeManager::getTypeI256()),
						this->context.sema_buffer.getIntValue(lhs.intValueID()).value,
						this->context.sema_buffer.getIntValue(rhs.intValueID()).value
					);
				} break;

				case Token::lookupKind("-"): {
					if(lhs.kind() == sema::Expr::Kind::INT_VALUE){
						return constexpr_intrinsic_evaluator.sub(
							lhs_type.value_or(TypeManager::getTypeI256()),
							true,
							this->context.sema_buffer.getIntValue(lhs.intValueID()).value,
							this->context.sema_buffer.getIntValue(rhs.intValueID()).value
						).value();

					}else{
						return constexpr_intrinsic_evaluator.fsub(
							lhs_type.value_or(TypeManager::getTypeF128()),
							this->context.sema_buffer.getFloatValue(lhs.floatValueID()).value,
							this->context.sema_buffer.getFloatValue(rhs.floatValueID()).value
						);
					}
				} break;

				case Token::lookupKind("-%"): {
					return constexpr_intrinsic_evaluator.sub(
						lhs_type.value_or(TypeManager::getTypeI256()),
						true,
						this->context.sema_buffer.getIntValue(lhs.intValueID()).value,
						this->context.sema_buffer.getIntValue(rhs.intValueID()).value
					).value();
				} break;

				case Token::lookupKind("-|"): {
					return constexpr_intrinsic_evaluator.subSat(
						lhs_type.value_or(TypeManager::getTypeI256()),
						this->context.sema_buffer.getIntValue(lhs.intValueID()).value,
						this->context.sema_buffer.getIntValue(rhs.intValueID()).value
					);
				} break;

				case Token::lookupKind("*"): {
					if(lhs.kind() == sema::Expr::Kind::INT_VALUE){
						return constexpr_intrinsic_evaluator.mul(
							lhs_type.value_or(TypeManager::getTypeI256()),
							true,
							this->context.sema_buffer.getIntValue(lhs.intValueID()).value,
							this->context.sema_buffer.getIntValue(rhs.intValueID()).value
						).value();

					}else{
						return constexpr_intrinsic_evaluator.fmul(
							lhs_type.value_or(TypeManager::getTypeF128()),
							this->context.sema_buffer.getFloatValue(lhs.floatValueID()).value,
							this->context.sema_buffer.getFloatValue(rhs.floatValueID()).value
						);
					}
				} break;

				case Token::lookupKind("*%"): {
					return constexpr_intrinsic_evaluator.mul(
						lhs_type.value_or(TypeManager::getTypeI256()),
						true,
						this->context.sema_buffer.getIntValue(lhs.intValueID()).value,
						this->context.sema_buffer.getIntValue(rhs.intValueID()).value
					).value();
				} break;

				case Token::lookupKind("*|"): {
					return constexpr_intrinsic_evaluator.mulSat(
						lhs_type.value_or(TypeManager::getTypeI256()),
						this->context.sema_buffer.getIntValue(lhs.intValueID()).value,
						this->context.sema_buffer.getIntValue(rhs.intValueID()).value
					);
				} break;

				case Token::lookupKind("/"): {
					if(lhs.kind() == sema::Expr::Kind::INT_VALUE){
						return constexpr_intrinsic_evaluator.div(
							lhs_type.value_or(TypeManager::getTypeI256()),
							false,
							this->context.sema_buffer.getIntValue(lhs.intValueID()).value,
							this->context.sema_buffer.getIntValue(rhs.intValueID()).value
						).value();

					}else{
						return constexpr_intrinsic_evaluator.fdiv(
							lhs_type.value_or(TypeManager::getTypeF128()),
							this->context.sema_buffer.getFloatValue(lhs.floatValueID()).value,
							this->context.sema_buffer.getFloatValue(rhs.floatValueID()).value
						);
					}
				} break;

				case Token::lookupKind("%"): {
					if(lhs.kind() == sema::Expr::Kind::INT_VALUE){
						return constexpr_intrinsic_evaluator.rem(
							lhs_type.value_or(TypeManager::getTypeI256()),
							this->context.sema_buffer.getIntValue(lhs.intValueID()).value,
							this->context.sema_buffer.getIntValue(rhs.intValueID()).value
						);

					}else{
						return constexpr_intrinsic_evaluator.rem(
							lhs_type.value_or(TypeManager::getTypeF128()),
							this->context.sema_buffer.getFloatValue(lhs.floatValueID()).value,
							this->context.sema_buffer.getFloatValue(rhs.floatValueID()).value
						);
					}
				} break;

				default: {
					evo::debugFatalBreak("Invalid infix op");
				} break;
			}
		}();

		if(lhs_type.has_value() == false){
			output.value_category = TermInfo::ValueCategory::EPHEMERAL_FLUID;
			output.type_id = TermInfo::FluidType();
		}

		return output;
	}



	auto SemanticAnalyzer::type_is_comparable(TypeInfo::ID type_id) -> bool {
		const TypeInfo& type_info = this->context.getTypeManager().getTypeInfo(type_id);
		return this->type_is_comparable(type_info);
	}

	auto SemanticAnalyzer::type_is_comparable(const TypeInfo& type_info) -> bool {
		if(type_info.qualifiers().empty() == false){
			if(type_info.qualifiers().back().isOptional){
				return this->type_is_comparable(type_info.copyWithPoppedQualifier());
			}else{
				evo::debugAssert(type_info.qualifiers().back().isPtr, "unknown type qualifier");
				return true;
			}

		}else{
			switch(type_info.baseTypeID().kind()){
				case BaseType::Kind::DUMMY: {
					evo::debugFatalBreak("Invalid type");
				} break;

				case BaseType::Kind::PRIMITIVE: {
					return true;
				} break;

				case BaseType::Kind::FUNCTION: {
					return true; // TODO(FUTURE): is this correct?
				} break;

				case BaseType::Kind::ARRAY: {
					const BaseType::Array& array_type =
						this->context.getTypeManager().getArray(type_info.baseTypeID().arrayID());

					return this->type_is_comparable(array_type.elementTypeID);
				} break;

				case BaseType::Kind::ARRAY_REF: {
					const BaseType::ArrayRef& array_ref_type =
						this->context.getTypeManager().getArrayRef(type_info.baseTypeID().arrayRefID());
						
					return this->type_is_comparable(array_ref_type.elementTypeID);
				} break;

				case BaseType::Kind::ALIAS: {
					const BaseType::Alias& alias_type =
						this->context.getTypeManager().getAlias(type_info.baseTypeID().aliasID());
						
					return this->type_is_comparable(*alias_type.aliasedType.load());
				} break;

				case BaseType::Kind::DISTINCT_ALIAS: {
					const BaseType::DistinctAlias& distinct_alias_type =
						this->context.getTypeManager().getDistinctAlias(type_info.baseTypeID().distinctAliasID());
						
					return this->type_is_comparable(*distinct_alias_type.underlyingType.load());
				} break;

				case BaseType::Kind::STRUCT: {
					const BaseType::Struct& struct_type =
						this->context.getTypeManager().getStruct(type_info.baseTypeID().structID());

					const auto [begin_overloads_range, end_overloads_range] = 
						struct_type.infixOverloads.equal_range(Token::lookupKind("=="));

					const auto overloads_range = evo::IterRange(begin_overloads_range, end_overloads_range);

					for(const auto& [_, sema_func_id] : overloads_range){
						const sema::Func& sema_func = this->context.getSemaBuffer().getFunc(sema_func_id);
						const BaseType::Function& func_type =
							this->context.getTypeManager().getFunction(sema_func.typeID);

						if(func_type.params[0].typeID == func_type.params[1].typeID){ return true; }
					}

					return false;
				} break;

				case BaseType::Kind::UNION: {
					const BaseType::Union& union_type =
						this->context.getTypeManager().getUnion(type_info.baseTypeID().unionID());
						
					if(union_type.isUntagged){ return false; }

					for(const BaseType::Union::Field& field : union_type.fields){
						if(field.typeID.isVoid()){ continue; }
						if(this->type_is_comparable(field.typeID.asTypeID()) == false){ return false; }
					}

					return true;
				} break;

				case BaseType::Kind::ENUM: {
					return true;
				} break;

				case BaseType::Kind::ARRAY_DEDUCER:           case BaseType::Kind::STRUCT_TEMPLATE:
				case BaseType::Kind::STRUCT_TEMPLATE_DEDUCER: case BaseType::Kind::TYPE_DEDUCER:
				case BaseType::Kind::INTERFACE:               case BaseType::Kind::INTERFACE_IMPL_INSTANTIATION: {
					evo::debugFatalBreak("Invalid type to check if comparing is possible");
				} break;
			}

			evo::debugFatalBreak("Unknown BaseType");
		}
	}


	template<bool IS_CONSTEXPR>
	EVO_NODISCARD auto SemanticAnalyzer::union_designated_init_new(
		const Instruction::DesignatedInitNew<IS_CONSTEXPR>& instr, TypeInfo::ID target_type_info_id
	) -> Result {
		const TypeInfo& target_type_info = this->context.getTypeManager().getTypeInfo(target_type_info_id);

		if(instr.designated_init_new.memberInits.size() != 1){
			if(instr.designated_init_new.memberInits.size() == 0){
				this->emit_error(
					Diagnostic::Code::SEMA_NEW_UNION_WRONG_NUM_FIELDS,
					instr.designated_init_new,
					"Union designated operator [new] must have a field"
				);
			}else{
				this->emit_error(
					Diagnostic::Code::SEMA_NEW_UNION_WRONG_NUM_FIELDS,
					instr.designated_init_new.memberInits[1].ident,
					"Too many fields in union designated operator [new]",
					Diagnostic::Info("Union can only hold one field at a time")
				);
				return Result::ERROR;
			}
		}


		const std::string_view used_field_name =
			this->source.getTokenBuffer()[instr.designated_init_new.memberInits[0].ident].getString();


		const BaseType::Union& union_info =
			this->context.getTypeManager().getUnion(target_type_info.baseTypeID().unionID());

		for(size_t i = 0; const BaseType::Union::Field& field : union_info.fields){
			EVO_DEFER([&](){ i += 1; });

			const std::string_view field_name = union_info.getFieldName(field, this->context.getSourceManager());

			if(used_field_name != field_name){ continue; }


			TermInfo& init_value = this->get_term_info(instr.member_init_exprs[0]);

			if(field.typeID.isVoid()){
				if(init_value.value_category != TermInfo::ValueCategory::NULL_VALUE){
					this->emit_error(
						Diagnostic::Code::SEMA_NEW_UNION_VALUE_TO_VOID_FIELD,
						instr.designated_init_new.memberInits[0].expr,
						"Initialization of a `Void` union field must be value [null]"
					);
					return Result::ERROR;
				}
				
			}else{
				if(this->type_check<true, true>(
					field.typeID.asTypeID(),
					init_value,
					"Union field initializer",
					instr.designated_init_new.memberInits[0].ident
				).ok == false){
					return Result::ERROR;
				}
			}


			const TermInfo::ValueStage value_stage = [&](){
				if constexpr(IS_CONSTEXPR){
					return TermInfo::ValueStage::CONSTEXPR;
				}else{
					if(this->currently_in_func() == false){
						return TermInfo::ValueStage::CONSTEXPR;

					}else if(this->get_current_func().isConstexpr){
						return TermInfo::ValueStage::COMPTIME;

					}else{
						return TermInfo::ValueStage::RUNTIME;
					}
				}
			}();



			if constexpr(IS_CONSTEXPR){
				if(union_info.isUntagged){
					this->return_term_info(instr.output,
						TermInfo::ValueCategory::EPHEMERAL,
						value_stage,
						TermInfo::ValueState::NOT_APPLICABLE,
						target_type_info_id,
						init_value.getExpr()
					);

				}else{
					this->emit_error(
						Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
						instr.designated_init_new,
						"Constexpr tagged union designated init `new` is unimplemented"
					);
					return Result::ERROR;
				}

			}else{
				this->return_term_info(instr.output,
					TermInfo::ValueCategory::EPHEMERAL,
					value_stage,
					TermInfo::ValueState::NOT_APPLICABLE,
					target_type_info_id,
					sema::Expr(
						this->context.sema_buffer.createUnionDesignatedInitNew(
							init_value.getExpr(), target_type_info.baseTypeID().unionID(), uint32_t(i)
						)
					)
				);
			}

			return Result::SUCCESS;
		}


		this->emit_error(
			Diagnostic::Code::SEMA_NEW_UNION_FIELD_DOESNT_EXIST,
			instr.designated_init_new.memberInits[0].ident,
			std::format("This union has no member \"{}\"", used_field_name)
		);
		return Result::ERROR;
	}




	//////////////////////////////////////////////////////////////////////
	// attributes


	auto SemanticAnalyzer::analyze_global_var_attrs(
		const AST::VarDef& var_decl, evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
	) -> evo::Result<GlobalVarAttrs> {
		auto attr_pub = ConditionalAttribute(*this, "pub");
		auto attr_global = Attribute(*this, "global");

		const AST::AttributeBlock& attribute_block = 
			this->source.getASTBuffer().getAttributeBlock(var_decl.attributeBlock);

		for(size_t i = 0; const AST::AttributeBlock::Attribute& attribute : attribute_block.attributes){
			EVO_DEFER([&](){ i += 1; });
			
			const std::string_view attribute_str = this->source.getTokenBuffer()[attribute.attribute].getString();

			if(attribute_str == "pub"){
				if(attribute_params_info[i].empty()){
					if(attr_pub.set(attribute.attribute, true).isError()){ return evo::resultError; } 

				}else if(attribute_params_info[i].size() == 1){
					TermInfo& cond_term_info = this->get_term_info(attribute_params_info[i][0]);
					if(this->check_term_isnt_type(cond_term_info, attribute.args[0]).isError()){
						return evo::resultError;
					}

					if(this->type_check<true, true>(
						this->context.getTypeManager().getTypeBool(),
						cond_term_info,
						"Condition in #pub",
						attribute.args[0]
					).ok == false){
						return evo::resultError;
					}

					const bool pub_cond = this->context.sema_buffer
						.getBoolValue(cond_term_info.getExpr().boolValueID()).value;

					if(attr_pub.set(attribute.attribute, pub_cond).isError()){ return evo::resultError; }

				}else{
					this->emit_error(
						Diagnostic::Code::SEMA_TOO_MANY_ATTRIBUTE_ARGS,
						attribute.args[1],
						"Attribute #pub does not accept more than 1 argument"
					);
					return evo::resultError;
				}

			}else if(attribute_str == "global"){
				if(attribute_params_info[i].empty() == false){
					this->emit_error(
						Diagnostic::Code::SEMA_TOO_MANY_ATTRIBUTE_ARGS,
						attribute.args.front(),
						"Attribute #global does not accept any arguments"
					);
					return evo::resultError;
				}

				if(attr_global.set(attribute.attribute).isError()){ return evo::resultError; }

			}else{
				this->emit_error(
					Diagnostic::Code::SEMA_UNKNOWN_ATTRIBUTE,
					attribute.attribute,
					std::format("Unknown global variable attribute #{}", attribute_str)
				);
				return evo::resultError;
			}
		}


		return GlobalVarAttrs(attr_pub.is_set(), attr_global.is_set());
	}


	auto SemanticAnalyzer::analyze_var_attrs(
		const AST::VarDef& var_decl, evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
	) -> evo::Result<VarAttrs> {
		auto attr_global = Attribute(*this, "global");

		const AST::AttributeBlock& attribute_block = 
			this->source.getASTBuffer().getAttributeBlock(var_decl.attributeBlock);

		for(size_t i = 0; const AST::AttributeBlock::Attribute& attribute : attribute_block.attributes){
			EVO_DEFER([&](){ i += 1; });
			
			const std::string_view attribute_str = this->source.getTokenBuffer()[attribute.attribute].getString();


			if(attribute_str == "global"){
				if(attribute_params_info[i].empty() == false){
					this->emit_error(
						Diagnostic::Code::SEMA_TOO_MANY_ATTRIBUTE_ARGS,
						attribute.args.front(),
						"Attribute #global does not accept any arguments"
					);
					return evo::resultError;
				}

				if(attr_global.set(attribute.attribute).isError()){ return evo::resultError; }

			}else if(attribute_str == "pub"){
				this->emit_error(
					Diagnostic::Code::SEMA_UNKNOWN_ATTRIBUTE,
					attribute.attribute,
					std::format("Unknown variable attribute #{}", attribute_str),
					Diagnostic::Info("Note: attribute `#pub` is not allowed on local variables")
				);
				return evo::resultError;

			}else{
				this->emit_error(
					Diagnostic::Code::SEMA_UNKNOWN_ATTRIBUTE,
					attribute.attribute,
					std::format("Unknown variable attribute #{}", attribute_str)
				);
				return evo::resultError;
			}
		}


		return VarAttrs(attr_global.is_set());
	}



	auto SemanticAnalyzer::analyze_struct_attrs(
		const AST::StructDef& struct_def, evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
	) -> evo::Result<StructAttrs> {
		auto attr_pub = ConditionalAttribute(*this, "pub");
		auto attr_packed = Attribute(*this, "packed");
		auto attr_ordered = Attribute(*this, "ordered");
		// auto attr_extern = Attribute(*this, "extern");


		const AST::AttributeBlock& attribute_block = 
			this->source.getASTBuffer().getAttributeBlock(struct_def.attributeBlock);

		for(size_t i = 0; const AST::AttributeBlock::Attribute& attribute : attribute_block.attributes){
			EVO_DEFER([&](){ i += 1; });
			
			const std::string_view attribute_str = this->source.getTokenBuffer()[attribute.attribute].getString();

			if(attribute_str == "pub"){
				if(attribute_params_info[i].empty()){
					if(attr_pub.set(attribute.attribute, true).isError()){ return evo::resultError; } 

				}else if(attribute_params_info[i].size() == 1){
					TermInfo& cond_term_info = this->get_term_info(attribute_params_info[i][0]);
					if(this->check_term_isnt_type(cond_term_info, attribute.args[0]).isError()){
						return evo::resultError;
					}

					if(this->type_check<true, true>(
						this->context.getTypeManager().getTypeBool(),
						cond_term_info,
						"Condition in #pub",
						attribute.args[0]
					).ok == false){
						return evo::resultError;
					}

					const bool pub_cond = this->context.sema_buffer
						.getBoolValue(cond_term_info.getExpr().boolValueID()).value;

					if(attr_pub.set(attribute.attribute, pub_cond).isError()){ return evo::resultError; }

				}else{
					this->emit_error(
						Diagnostic::Code::SEMA_TOO_MANY_ATTRIBUTE_ARGS,
						attribute.args[1],
						"Attribute #pub does not accept more than 1 argument"
					);
					return evo::resultError;
				}

			}else if(attribute_str == "ordered"){
				if(attribute_params_info[i].empty() == false){
					this->emit_error(
						Diagnostic::Code::SEMA_TOO_MANY_ATTRIBUTE_ARGS,
						attribute.args.front(),
						"Attribute #ordered does not accept any arguments"
					);
					return evo::resultError;
				}

				if(attr_ordered.set(attribute.attribute).isError()){ return evo::resultError; }

			}else if(attribute_str == "packed"){
				if(attribute_params_info[i].empty() == false){
					this->emit_error(
						Diagnostic::Code::SEMA_TOO_MANY_ATTRIBUTE_ARGS,
						attribute.args.front(),
						"Attribute #packed does not accept any arguments"
					);
					return evo::resultError;
				}

				if(attr_packed.set(attribute.attribute).isError()){ return evo::resultError; }

			}else{
				this->emit_error(
					Diagnostic::Code::SEMA_UNKNOWN_ATTRIBUTE,
					attribute.attribute,
					std::format("Unknown struct attribute #{}", attribute_str)
				);
				return evo::resultError;
			}
		}


		return StructAttrs(attr_pub.is_set(), attr_ordered.is_set(), attr_packed.is_set());
	}


	auto SemanticAnalyzer::analyze_union_attrs(
		const AST::UnionDef& union_def, evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
	) -> evo::Result<UnionAttrs> {
		auto attr_pub = ConditionalAttribute(*this, "pub");
		auto attr_untagged = Attribute(*this, "untagged");


		const AST::AttributeBlock& attribute_block = 
			this->source.getASTBuffer().getAttributeBlock(union_def.attributeBlock);

		for(size_t i = 0; const AST::AttributeBlock::Attribute& attribute : attribute_block.attributes){
			EVO_DEFER([&](){ i += 1; });
			
			const std::string_view attribute_str = this->source.getTokenBuffer()[attribute.attribute].getString();

			if(attribute_str == "pub"){
				if(attribute_params_info[i].empty()){
					if(attr_pub.set(attribute.attribute, true).isError()){ return evo::resultError; } 

				}else if(attribute_params_info[i].size() == 1){
					TermInfo& cond_term_info = this->get_term_info(attribute_params_info[i][0]);
					if(this->check_term_isnt_type(cond_term_info, attribute.args[0]).isError()){
						return evo::resultError;
					}

					if(this->type_check<true, true>(
						this->context.getTypeManager().getTypeBool(),
						cond_term_info,
						"Condition in #pub",
						attribute.args[0]
					).ok == false){
						return evo::resultError;
					}

					const bool pub_cond = this->context.sema_buffer
						.getBoolValue(cond_term_info.getExpr().boolValueID()).value;

					if(attr_pub.set(attribute.attribute, pub_cond).isError()){ return evo::resultError; }

				}else{
					this->emit_error(
						Diagnostic::Code::SEMA_TOO_MANY_ATTRIBUTE_ARGS,
						attribute.args[1],
						"Attribute #pub does not accept more than 1 argument"
					);
					return evo::resultError;
				}

			}else if(attribute_str == "untagged"){
				if(attribute_params_info[i].empty() == false){
					this->emit_error(
						Diagnostic::Code::SEMA_TOO_MANY_ATTRIBUTE_ARGS,
						attribute.args.front(),
						"Attribute #untagged does not accept any arguments"
					);
					return evo::resultError;
				}

				if(attr_untagged.set(attribute.attribute).isError()){ return evo::resultError; }

			}else{
				this->emit_error(
					Diagnostic::Code::SEMA_UNKNOWN_ATTRIBUTE,
					attribute.attribute,
					std::format("Unknown union attribute #{}", attribute_str)
				);
				return evo::resultError;
			}
		}


		return UnionAttrs(attr_pub.is_set(), attr_untagged.is_set());
	}



	auto SemanticAnalyzer::analyze_enum_attrs(
		const AST::EnumDef& enum_def, evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
	) -> evo::Result<EnumAttrs> {
		auto attr_pub = ConditionalAttribute(*this, "pub");


		const AST::AttributeBlock& attribute_block = 
			this->source.getASTBuffer().getAttributeBlock(enum_def.attributeBlock);

		for(size_t i = 0; const AST::AttributeBlock::Attribute& attribute : attribute_block.attributes){
			EVO_DEFER([&](){ i += 1; });
			
			const std::string_view attribute_str = this->source.getTokenBuffer()[attribute.attribute].getString();

			if(attribute_str == "pub"){
				if(attribute_params_info[i].empty()){
					if(attr_pub.set(attribute.attribute, true).isError()){ return evo::resultError; } 

				}else if(attribute_params_info[i].size() == 1){
					TermInfo& cond_term_info = this->get_term_info(attribute_params_info[i][0]);
					if(this->check_term_isnt_type(cond_term_info, attribute.args[0]).isError()){
						return evo::resultError;
					}

					if(this->type_check<true, true>(
						this->context.getTypeManager().getTypeBool(),
						cond_term_info,
						"Condition in #pub",
						attribute.args[0]
					).ok == false){
						return evo::resultError;
					}

					const bool pub_cond = this->context.sema_buffer
						.getBoolValue(cond_term_info.getExpr().boolValueID()).value;

					if(attr_pub.set(attribute.attribute, pub_cond).isError()){ return evo::resultError; }

				}else{
					this->emit_error(
						Diagnostic::Code::SEMA_TOO_MANY_ATTRIBUTE_ARGS,
						attribute.args[1],
						"Attribute #pub does not accept more than 1 argument"
					);
					return evo::resultError;
				}

			}else{
				this->emit_error(
					Diagnostic::Code::SEMA_UNKNOWN_ATTRIBUTE,
					attribute.attribute,
					std::format("Unknown enum attribute #{}", attribute_str)
				);
				return evo::resultError;
			}
		}


		return EnumAttrs(attr_pub.is_set());
	}




	auto SemanticAnalyzer::analyze_func_attrs(
		const AST::FuncDef& func_decl, evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
	) -> evo::Result<FuncAttrs> {
		auto attr_pub = ConditionalAttribute(*this, "pub");
		auto attr_rt = ConditionalAttribute(*this, "rt");
		auto attr_export = Attribute(*this, "export");
		auto attr_entry = Attribute(*this, "entry");

		auto attr_commutative = Attribute(*this, "commutative");
		auto attr_swapped = Attribute(*this, "swapped");



		const AST::AttributeBlock& attribute_block = 
			this->source.getASTBuffer().getAttributeBlock(func_decl.attributeBlock);

		for(size_t i = 0; const AST::AttributeBlock::Attribute& attribute : attribute_block.attributes){
			EVO_DEFER([&](){ i += 1; });
			
			const std::string_view attribute_str = this->source.getTokenBuffer()[attribute.attribute].getString();

			if(attribute_str == "pub"){
				if(attribute_params_info[i].empty()){
					if(attr_pub.set(attribute.attribute, true).isError()){ return evo::resultError; } 

				}else if(attribute_params_info[i].size() == 1){
					TermInfo& cond_term_info = this->get_term_info(attribute_params_info[i][0]);
					if(this->check_term_isnt_type(cond_term_info, attribute.args[0]).isError()){
						return evo::resultError;
					}

					if(this->type_check<true, true>(
						this->context.getTypeManager().getTypeBool(),
						cond_term_info,
						"Condition in #pub",
						attribute.args[0]
					).ok == false){
						return evo::resultError;
					}

					const bool pub_cond = this->context.sema_buffer
						.getBoolValue(cond_term_info.getExpr().boolValueID()).value;

					if(attr_pub.set(attribute.attribute, pub_cond).isError()){ return evo::resultError; }

				}else{
					this->emit_error(
						Diagnostic::Code::SEMA_TOO_MANY_ATTRIBUTE_ARGS,
						attribute.args[1],
						"Attribute #pub does not accept more than 1 argument"
					);
					return evo::resultError;
				}

			}else if(attribute_str == "rt"){
				if(attribute_params_info[i].empty()){
					if(attr_rt.set(attribute.attribute, true).isError()){ return evo::resultError; } 

				}else if(attribute_params_info[i].size() == 1){
					TermInfo& cond_term_info = this->get_term_info(attribute_params_info[i][0]);
					if(this->check_term_isnt_type(cond_term_info, attribute.args[0]).isError()){
						return evo::resultError;
					}

					if(this->type_check<true, true>(
						this->context.getTypeManager().getTypeBool(),
						cond_term_info,
						"Condition in #rt",
						attribute.args[0]
					).ok == false){
						return evo::resultError;
					}

					const bool rt_cond = this->context.sema_buffer
						.getBoolValue(cond_term_info.getExpr().boolValueID()).value;

					if(attr_rt.set(attribute.attribute, rt_cond).isError()){ return evo::resultError; }

				}else{
					this->emit_error(
						Diagnostic::Code::SEMA_TOO_MANY_ATTRIBUTE_ARGS,
						attribute.args[1],
						"Attribute #rt does not accept more than 1 argument"
					);
					return evo::resultError;
				}

			}else if(attribute_str == "export"){
				if(attribute_params_info[i].empty() == false){
					this->emit_error(
						Diagnostic::Code::SEMA_TOO_MANY_ATTRIBUTE_ARGS,
						attribute.args.front(),
						"Attribute #export does not accept any arguments"
					);
					return evo::resultError;
				}

				if(attr_entry.is_set()){
					this->emit_error(
						Diagnostic::Code::SEMA_THESE_ATTRIBUTES_CANNOT_BE_COMBINED,
						attribute.attribute,
						"A function cannot have both attribute #export and #entry"
					);
					return evo::resultError;
				}

				if(attr_export.set(attribute.attribute).isError()){ return evo::resultError; }


			}else if(attribute_str == "entry"){
				if(attribute_params_info[i].empty() == false){
					this->emit_error(
						Diagnostic::Code::SEMA_TOO_MANY_ATTRIBUTE_ARGS,
						attribute.args.front(),
						"Attribute #entry does not accept any arguments"
					);
					return evo::resultError;
				}

				if(attr_export.is_set()){
					this->emit_error(
						Diagnostic::Code::SEMA_THESE_ATTRIBUTES_CANNOT_BE_COMBINED,
						attribute.attribute,
						"A function cannot have both attribute #entry and #export"
					);
					return evo::resultError;
				}

				if(attr_entry.set(attribute.attribute).isError()){ return evo::resultError; }
				attr_rt.implicitly_set(attribute.attribute, true);

			}else if(attribute_str == "commutative"){
				if(attribute_params_info[i].empty() == false){
					this->emit_error(
						Diagnostic::Code::SEMA_TOO_MANY_ATTRIBUTE_ARGS,
						attribute.args.front(),
						"Attribute #commutative does not accept any arguments"
					);
					return evo::resultError;
				}

				if(attr_swapped.is_set()){
					this->emit_error(
						Diagnostic::Code::SEMA_THESE_ATTRIBUTES_CANNOT_BE_COMBINED,
						attribute.attribute,
						"A function cannot have both attribute #commutative and #swapped"
					);
					return evo::resultError;
				}
				
				if(attr_commutative.set(attribute.attribute).isError()){ return evo::resultError; }

			}else if(attribute_str == "swapped"){
				if(attribute_params_info[i].empty() == false){
					this->emit_error(
						Diagnostic::Code::SEMA_TOO_MANY_ATTRIBUTE_ARGS,
						attribute.args.front(),
						"Attribute #swapped does not accept any arguments"
					);
					return evo::resultError;
				}

				if(attr_commutative.is_set()){
					this->emit_error(
						Diagnostic::Code::SEMA_THESE_ATTRIBUTES_CANNOT_BE_COMBINED,
						attribute.attribute,
						"A function cannot have both attribute #swapped and #commutative"
					);
					return evo::resultError;
				}
				
				if(attr_swapped.set(attribute.attribute).isError()){ return evo::resultError; }

			}else{
				this->emit_error(
					Diagnostic::Code::SEMA_UNKNOWN_ATTRIBUTE,
					attribute.attribute,
					std::format("Unknown function attribute #{}", attribute_str)
				);
				return evo::resultError;
			}
		}

		return FuncAttrs{
			.is_pub         = attr_pub.is_set(),
			.is_runtime     = attr_rt.is_set(),
			.is_export      = attr_export.is_set(),
			.is_entry       = attr_entry.is_set(),
			.is_commutative = attr_commutative.is_set(),
			.is_swapped     = attr_swapped.is_set(),
		};
	}



	auto SemanticAnalyzer::analyze_interface_attrs(
		const AST::InterfaceDef& interface_def, evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
	) -> evo::Result<InterfaceAttrs> {
		auto attr_pub = ConditionalAttribute(*this, "pub");

		const AST::AttributeBlock& attribute_block = 
			this->source.getASTBuffer().getAttributeBlock(interface_def.attributeBlock);

		for(size_t i = 0; const AST::AttributeBlock::Attribute& attribute : attribute_block.attributes){
			EVO_DEFER([&](){ i += 1; });
			
			const std::string_view attribute_str = this->source.getTokenBuffer()[attribute.attribute].getString();


			if(attribute_str == "pub"){
				if(attribute_params_info[i].empty()){
					if(attr_pub.set(attribute.attribute, true).isError()){ return evo::resultError; } 

				}else if(attribute_params_info[i].size() == 1){
					TermInfo& cond_term_info = this->get_term_info(attribute_params_info[i][0]);
					if(this->check_term_isnt_type(cond_term_info, attribute.args[0]).isError()){
						return evo::resultError;
					}

					if(this->type_check<true, true>(
						this->context.getTypeManager().getTypeBool(),
						cond_term_info,
						"Condition in #pub",
						attribute.args[0]
					).ok == false){
						return evo::resultError;
					}

					const bool pub_cond = this->context.sema_buffer
						.getBoolValue(cond_term_info.getExpr().boolValueID()).value;

					if(attr_pub.set(attribute.attribute, pub_cond).isError()){ return evo::resultError; }

				}else{
					this->emit_error(
						Diagnostic::Code::SEMA_TOO_MANY_ATTRIBUTE_ARGS,
						attribute.args[1],
						"Attribute #pub does not accept more than 1 argument"
					);
					return evo::resultError;
				}

			}else{
				this->emit_error(
					Diagnostic::Code::SEMA_UNKNOWN_ATTRIBUTE,
					attribute.attribute,
					std::format("Unknown interface attribute #{}", attribute_str)
				);
				return evo::resultError;
			}
		}


		return InterfaceAttrs(attr_pub.is_set());
	}




	auto SemanticAnalyzer::propagate_finished_impl(const evo::SmallVector<SymbolProc::ID>& waited_on_by_list) -> void {
		for(const SymbolProc::ID& waited_on_id : waited_on_by_list){
			SymbolProc& waited_on = this->context.symbol_proc_manager.getSymbolProc(waited_on_id);
			const auto lock = std::scoped_lock(waited_on.waiting_for_lock);

			evo::debugAssert(waited_on.waiting_for.empty() == false, "Should never have empty list");

			for(size_t i = 0; i < waited_on.waiting_for.size() - 1; i+=1){
				if(waited_on.waiting_for[i] == this->symbol_proc_id){
					waited_on.waiting_for[i] = waited_on.waiting_for.back();
					break;
				}
			}

			waited_on.waiting_for.pop_back();

			if(waited_on.waiting_for.empty() && waited_on.isTemplateSubSymbol() == false){
				if(waited_on.hasErroredNoLock()){ continue; }

				if(waited_on.status != SymbolProc::Status::WORKING){ // prevent race condition of setting up waits
					waited_on.setStatusInQueue();
					this->context.add_task_to_work_manager(waited_on_id);
				}
			}
		}
	}


	auto SemanticAnalyzer::propagate_finished_decl() -> void {
		const auto lock = std::scoped_lock(this->symbol_proc.decl_waited_on_lock);

		this->symbol_proc.decl_done = true;
		this->propagate_finished_impl(this->symbol_proc.decl_waited_on_by);
	}


	auto SemanticAnalyzer::propagate_finished_def() -> void {
		const auto lock = std::scoped_lock(this->symbol_proc.def_waited_on_lock);

		this->symbol_proc.def_done = true;
		this->propagate_finished_impl(this->symbol_proc.def_waited_on_by);
	}



	auto SemanticAnalyzer::propagate_finished_decl_def() -> void {
		const auto lock = std::scoped_lock(this->symbol_proc.decl_waited_on_lock, this->symbol_proc.def_waited_on_lock);

		this->symbol_proc.decl_done = true;
		this->symbol_proc.def_done = true;

		this->propagate_finished_impl(this->symbol_proc.decl_waited_on_by);
		this->propagate_finished_impl(this->symbol_proc.def_waited_on_by);
	}


	auto SemanticAnalyzer::propagate_finished_pir_decl() -> void {
		const auto lock = std::scoped_lock(this->symbol_proc.pir_decl_waited_on_lock);

		this->symbol_proc.pir_decl_done = true;
		this->propagate_finished_impl(this->symbol_proc.pir_decl_waited_on_by);
	}

	auto SemanticAnalyzer::propagate_finished_pir_def() -> void {
		const auto lock = std::scoped_lock(this->symbol_proc.pir_def_waited_on_lock);

		this->symbol_proc.pir_def_done = true;
		this->propagate_finished_impl(this->symbol_proc.pir_def_waited_on_by);
	}



	//////////////////////////////////////////////////////////////////////
	// exec value gets / returns


	auto SemanticAnalyzer::get_type(SymbolProc::TypeID symbol_proc_type_id) -> TypeInfo::VoidableID {
		evo::debugAssert(
			this->symbol_proc.type_ids[symbol_proc_type_id.get()].has_value(),
			"Symbol proc type wasn't set"
		);
		return *this->symbol_proc.type_ids[symbol_proc_type_id.get()];
	}

	auto SemanticAnalyzer::return_type(SymbolProc::TypeID symbol_proc_type_id, TypeInfo::VoidableID&& id) -> void {
		this->symbol_proc.type_ids[symbol_proc_type_id.get()] = std::move(id);
	}


	auto SemanticAnalyzer::get_term_info(SymbolProc::TermInfoID symbol_proc_term_info_id) -> TermInfo& {
		evo::debugAssert(
			this->symbol_proc.term_infos[symbol_proc_term_info_id.get()].has_value(),
			"Symbol proc term info wasn't set"
		);
		return *this->symbol_proc.term_infos[symbol_proc_term_info_id.get()];
	}

	auto SemanticAnalyzer::return_term_info(SymbolProc::TermInfoID symbol_proc_term_info_id, auto&&... args) -> void {
		this->symbol_proc.term_infos[symbol_proc_term_info_id.get()]
			.emplace(std::forward<decltype(args)>(args)...);
	}



	auto SemanticAnalyzer::get_struct_instantiation(SymbolProc::StructInstantiationID instantiation_id)
	-> evo::Variant<const BaseType::StructTemplate::Instantiation*, BaseType::StructTemplateDeducer::ID> {
		const auto& output = this->symbol_proc.struct_instantiations[instantiation_id.get()];

		evo::debugAssert(
			output.is<BaseType::StructTemplateDeducer::ID>()
			|| output.as<const BaseType::StructTemplate::Instantiation*>() != nullptr,
			"Symbol proc struct instantiation wasn't set"
		);

		return output;
	}

	auto SemanticAnalyzer::return_struct_instantiation(
		SymbolProc::StructInstantiationID instantiation_id,
		const BaseType::StructTemplate::Instantiation& instantiation
	) -> void {
		this->symbol_proc.struct_instantiations[instantiation_id.get()] = &instantiation;
	}

	auto SemanticAnalyzer::return_struct_instantiation(
		SymbolProc::StructInstantiationID instantiation_id, BaseType::StructTemplateDeducer::ID deducer_id
	) -> void {
		this->symbol_proc.struct_instantiations[instantiation_id.get()] = deducer_id;
	}



	//////////////////////////////////////////////////////////////////////
	// error handling / diagnostics

	template<bool MAY_IMPLICITLY_CONVERT, bool MAY_EMIT_ERROR>
	auto SemanticAnalyzer::type_check(
		TypeInfo::ID expected_type_id,
		TermInfo& got_expr,
		std::string_view expected_type_location_name,
		const auto& location,
		std::optional<unsigned> multi_type_index
	) -> TypeCheckInfo {
		if constexpr(MAY_EMIT_ERROR){
			evo::debugAssert(
				expected_type_location_name.empty() == false, "Error emitting `type_check` requires a message"
			);
			evo::debugAssert(
				std::isupper(int(expected_type_location_name[0])),
				"first character of expected_type_location_name should be upper-case"
			);
		}else{
			evo::debugAssert(
				expected_type_location_name.empty(), "Non-error emitting `type_check` should not have a message"
			);
		}


		const TypeManager& type_manager = this->context.getTypeManager();

		const TypeInfo::ID actual_expected_type_id = this->get_actual_type<false, false>(expected_type_id);

		switch(got_expr.value_category){
			case TermInfo::ValueCategory::EPHEMERAL:
			case TermInfo::ValueCategory::CONCRETE_CONST:
			case TermInfo::ValueCategory::CONCRETE_MUT:
			case TermInfo::ValueCategory::FORWARDABLE: {
				TypeInfo::ID actual_got_type_id = TypeInfo::ID::dummy();
				if(got_expr.isMultiValue()) [[unlikely]] {
					if(multi_type_index.has_value() == false){
						this->emit_error(
							Diagnostic::Code::SEMA_MULTI_RETURN_INTO_SINGLE_VALUE,
							location,
							std::format("{} cannot accept multiple values", expected_type_location_name)
						);
						return TypeCheckInfo::fail();
					}

					actual_got_type_id = this->get_actual_type<false, false>(
						got_expr.type_id.as<evo::SmallVector<TypeInfo::ID>>()[*multi_type_index]
					);

				}else{
					actual_got_type_id = this->get_actual_type<false, true>(got_expr.type_id.as<TypeInfo::ID>());
				}


				// if types are not exact, check if implicit conversion is valid
				bool is_implicit_conversion_to_optional = false;
				if(actual_expected_type_id != actual_got_type_id){
					const TypeInfo& expected_type = type_manager.getTypeInfo(actual_expected_type_id);
					const TypeInfo& got_type      = type_manager.getTypeInfo(actual_got_type_id);


					if(type_manager.isTypeDeducer(actual_expected_type_id)){
						evo::Result<evo::SmallVector<DeducedTerm>> extracted_deducers
							= this->extract_deducers(actual_expected_type_id, actual_got_type_id);

						if(extracted_deducers.isError()){
							if constexpr(MAY_EMIT_ERROR){
								// TODO(FUTURE): better messaging
								this->emit_error(
									Diagnostic::Code::SEMA_TYPE_MISMATCH, // TODO(FUTURE): more specific code
									location,
									"Type deducer not able to deduce type",
									Diagnostic::Info(
										std::format(
											"Type of expression: {}",
											this->context.getTypeManager().printType(
												actual_got_type_id, this->context.getSourceManager()
											)
										)
									)
								);
							}
							return TypeCheckInfo::fail();
						}

						return TypeCheckInfo::success(false, std::move(extracted_deducers.value()));
					}


					if(expected_type.baseTypeID() != got_type.baseTypeID()){
						if(
							expected_type.baseTypeID().kind() != BaseType::Kind::ARRAY_REF 
							|| got_type.baseTypeID().kind() != BaseType::Kind::ARRAY_REF
						){
							if constexpr(MAY_EMIT_ERROR){
								this->error_type_mismatch(
									expected_type_id, got_expr, expected_type_location_name, location, multi_type_index
								);
							}
							return TypeCheckInfo::fail();
						}

						const BaseType::ArrayRef& expected_array_ref =
							type_manager.getArrayRef(expected_type.baseTypeID().arrayRefID());

						const BaseType::ArrayRef& got_array_ref =
							type_manager.getArrayRef(got_type.baseTypeID().arrayRefID());


						if(expected_array_ref.isReadOnly == false && got_array_ref.isReadOnly){
							if constexpr(MAY_EMIT_ERROR){
								this->error_type_mismatch(
									expected_type_id, got_expr, expected_type_location_name, location, multi_type_index
								);
							}
							return TypeCheckInfo::fail();
						}
					}


					if(expected_type.qualifiers().size() != got_type.qualifiers().size()){
						const bool is_optional_conversion = 
							expected_type.qualifiers().size() == got_type.qualifiers().size() + 1
							&& expected_type.isOptionalNotPointer();
						
						if(is_optional_conversion){
							is_implicit_conversion_to_optional = true;

						}else{
							if constexpr(MAY_EMIT_ERROR){
								this->error_type_mismatch(
									expected_type_id, got_expr, expected_type_location_name, location, multi_type_index
								);
							}
							return TypeCheckInfo::fail();
						}
					}

					// check qualifiers
					for(size_t i = 0; i < got_type.qualifiers().size(); i+=1){
						const AST::Type::Qualifier& expected_qualifier = expected_type.qualifiers()[i];
						const AST::Type::Qualifier& got_qualifier      = got_type.qualifiers()[i];

						if(expected_qualifier.isPtr != got_qualifier.isPtr){
							if constexpr(MAY_EMIT_ERROR){
								this->error_type_mismatch(
									expected_type_id, got_expr, expected_type_location_name, location, multi_type_index
								);
							}
							return TypeCheckInfo::fail();
						}
						if(expected_qualifier.isReadOnly == false && got_qualifier.isReadOnly){
							if constexpr(MAY_EMIT_ERROR){
								this->error_type_mismatch(
									expected_type_id, got_expr, expected_type_location_name, location, multi_type_index
								);
							}
							return TypeCheckInfo::fail();
						}
						if(expected_qualifier.isUninit != got_qualifier.isUninit){
							if constexpr(MAY_EMIT_ERROR){
								this->error_type_mismatch(
									expected_type_id, got_expr, expected_type_location_name, location, multi_type_index
								);
							}
							return TypeCheckInfo::fail();	
						}
						if(expected_qualifier.isOptional == false && got_qualifier.isOptional){
							if constexpr(MAY_EMIT_ERROR){
								this->error_type_mismatch(
									expected_type_id, got_expr, expected_type_location_name, location, multi_type_index
								);
							}
							return TypeCheckInfo::fail();
						}
					}
				}

				if constexpr(MAY_IMPLICITLY_CONVERT){
					EVO_DEFER([&](){
						if(multi_type_index.has_value() == false){
							got_expr.type_id.emplace<TypeInfo::ID>(expected_type_id);	
						}else{
							got_expr.type_id.as<evo::SmallVector<TypeInfo::ID>>()[*multi_type_index] = expected_type_id;
						}
					});

					if(is_implicit_conversion_to_optional){
						got_expr.getExpr() = sema::Expr(
							this->context.sema_buffer.createConversionToOptional(got_expr.getExpr(), expected_type_id)
						);
					}
				}

				if(multi_type_index.has_value() == false){
					return TypeCheckInfo::success(got_expr.type_id.as<TypeInfo::ID>() != expected_type_id);
				}else{
					return TypeCheckInfo::success(
						got_expr.type_id.as<evo::SmallVector<TypeInfo::ID>>()[*multi_type_index] != expected_type_id
					);
				}
			} break;

			case TermInfo::ValueCategory::EPHEMERAL_FLUID: {
				const TypeInfo& expected_type_info = 
					type_manager.getTypeInfo(actual_expected_type_id);


				if(expected_type_info.baseTypeID().kind() != BaseType::Kind::PRIMITIVE){
					if constexpr(MAY_EMIT_ERROR){
						if(expected_type_info.baseTypeID().kind() == BaseType::Kind::TYPE_DEDUCER){
							// TODO(FUTURE): better messaging
							this->emit_error(
								Diagnostic::Code::SEMA_CANNOT_INFER_TYPE,
								location,
								"Cannot deduce the type of a fluid value"
							);

						}else{
							this->error_type_mismatch(
								expected_type_id, got_expr, expected_type_location_name, location, multi_type_index
							);
						}
					}
					return TypeCheckInfo::fail();
				}


				bool is_implicit_conversion_to_optional = false;
				if(expected_type_info.qualifiers().empty() == false){
					if(
						expected_type_info.qualifiers().back().isOptional
						&& expected_type_info.qualifiers().back().isPtr == false
					){ // is optional not pointer
						is_implicit_conversion_to_optional = true;
					}else{
						if constexpr(MAY_EMIT_ERROR){
							this->error_type_mismatch(
								expected_type_id, got_expr, expected_type_location_name, location, multi_type_index
							);
						}
						return TypeCheckInfo::fail();
					}
				}

				const BaseType::Primitive::ID expected_type_primitive_id =
					expected_type_info.baseTypeID().primitiveID();

				const BaseType::Primitive& expected_type_primitive = 
					type_manager.getPrimitive(expected_type_primitive_id);

				if(got_expr.getExpr().kind() == sema::Expr::Kind::INT_VALUE){
					bool is_unsigned = true;

					switch(expected_type_primitive.kind()){
						case Token::Kind::TYPE_INT:
						case Token::Kind::TYPE_ISIZE:
						case Token::Kind::TYPE_I_N:
						case Token::Kind::TYPE_C_SHORT:
						case Token::Kind::TYPE_C_INT:
						case Token::Kind::TYPE_C_LONG:
						case Token::Kind::TYPE_C_LONG_LONG:
							is_unsigned = false;
							break;

						case Token::Kind::TYPE_UINT:
						case Token::Kind::TYPE_USIZE:
						case Token::Kind::TYPE_UI_N:
						case Token::Kind::TYPE_BYTE:
						case Token::Kind::TYPE_C_WCHAR:
						case Token::Kind::TYPE_C_USHORT:
						case Token::Kind::TYPE_C_UINT:
						case Token::Kind::TYPE_C_ULONG:
						case Token::Kind::TYPE_C_ULONG_LONG:
							break;

						default: {
							if constexpr(MAY_EMIT_ERROR){
								this->error_type_mismatch(
									expected_type_id, got_expr, expected_type_location_name, location, multi_type_index
								);
							}
							return TypeCheckInfo::fail();
						}
					}

					if constexpr(MAY_IMPLICITLY_CONVERT){
						const sema::IntValue::ID int_value_id = got_expr.getExpr().intValueID();
						sema::IntValue& int_value = this->context.sema_buffer.int_values[int_value_id];

						const unsigned bit_width =
							unsigned(this->context.getTypeManager().numBits(expected_type_info.baseTypeID(), false));

						core::GenericInt target_min =
							type_manager.getMin(expected_type_info.baseTypeID()).getInt(bit_width);

						core::GenericInt target_max =
							type_manager.getMax(expected_type_info.baseTypeID()).getInt(bit_width);

						if(int_value.value.getBitWidth() >= target_min.getBitWidth()){
							target_min = target_min.ext(int_value.value.getBitWidth(), is_unsigned);
							target_max = target_max.ext(int_value.value.getBitWidth(), is_unsigned);

							if(is_unsigned){
								if(int_value.value.ult(target_min) || int_value.value.ugt(target_max)){
									if constexpr(MAY_EMIT_ERROR){
										auto infos = evo::SmallVector<Diagnostic::Info>();
										this->diagnostic_print_type_info(expected_type_id, infos, "Target type: ");

										if(int_value.value.slt(target_min)){ // check for negatives
											this->emit_error(
												Diagnostic::Code::SEMA_CANNOT_CONVERT_FLUID_VALUE,
												location,
												"Cannot implicitly convert this fluid value to the target type "
													"as fluid value is negative and the target type is unsigned",
												std::move(infos)
											);
										}else{
											this->emit_error(
												Diagnostic::Code::SEMA_CANNOT_CONVERT_FLUID_VALUE,
												location,
												"Cannot implicitly convert this fluid value to the target type "
													"as it would require truncation",
												std::move(infos)
											);
										}
									}
									return TypeCheckInfo::fail();
								}
							}else{
								if(int_value.value.slt(target_min) || int_value.value.sgt(target_max)){
									if constexpr(MAY_EMIT_ERROR){
										auto infos = evo::SmallVector<Diagnostic::Info>();
										this->diagnostic_print_type_info(expected_type_id, infos, "Target type: ");
										this->emit_error(
											Diagnostic::Code::SEMA_CANNOT_CONVERT_FLUID_VALUE,
											location,
											"Cannot implicitly convert this fluid value to the target type "
												"as it would require truncation",
											std::move(infos)
										);
									}
									return TypeCheckInfo::fail();
								}
							}

							int_value.value = int_value.value.trunc(bit_width);

						}else{
							int_value.value = int_value.value.ext(target_min.getBitWidth(), is_unsigned);

						}


						int_value.typeID = type_manager.getTypeInfo(expected_type_id).baseTypeID();
					}

				}else{
					evo::debugAssert(
						got_expr.getExpr().kind() == sema::Expr::Kind::FLOAT_VALUE, "Expected float"
					);

					switch(expected_type_primitive.kind()){
						case Token::Kind::TYPE_F16:
						case Token::Kind::TYPE_BF16:
						case Token::Kind::TYPE_F32:
						case Token::Kind::TYPE_F64:
						case Token::Kind::TYPE_F80:
						case Token::Kind::TYPE_F128:
						case Token::Kind::TYPE_C_LONG_DOUBLE:
							break;

						default: {
							if constexpr(MAY_EMIT_ERROR){
								this->error_type_mismatch(
									expected_type_id, got_expr, expected_type_location_name, location, multi_type_index
								);
							}
							return TypeCheckInfo::fail();
						}
					}

					if constexpr(MAY_IMPLICITLY_CONVERT){
						const sema::FloatValue::ID float_value_id = got_expr.getExpr().floatValueID();
						sema::FloatValue& float_value = this->context.sema_buffer.float_values[float_value_id];


						const core::GenericFloat target_min = [&](){
							switch(expected_type_primitive.kind()){
								break; case Token::Kind::TYPE_F16:
									return type_manager.getMin(expected_type_info.baseTypeID()).getF16();
								break; case Token::Kind::TYPE_BF16:
									return type_manager.getMin(expected_type_info.baseTypeID()).getBF16();
								break; case Token::Kind::TYPE_F32:
									return type_manager.getMin(expected_type_info.baseTypeID()).getF32();
								break; case Token::Kind::TYPE_F64:
									return type_manager.getMin(expected_type_info.baseTypeID()).getF64();
								break; case Token::Kind::TYPE_F80:
									return type_manager.getMin(expected_type_info.baseTypeID()).getF80();
								break; case Token::Kind::TYPE_F128:
									return type_manager.getMin(expected_type_info.baseTypeID()).getF128();

								break; case Token::Kind::TYPE_C_LONG_DOUBLE: {
									if(type_manager.numBytes(expected_type_info.baseTypeID()) == 8){
										return type_manager.getMin(expected_type_info.baseTypeID()).getF64();
									}else{
										return type_manager.getMin(expected_type_info.baseTypeID()).getF128();
									}
								}
								break; default: evo::debugFatalBreak("Unknown float type");
							}
						}().asF128();


						const core::GenericFloat target_max = [&](){
							switch(expected_type_primitive.kind()){
								break; case Token::Kind::TYPE_F16:
									return type_manager.getMax(expected_type_info.baseTypeID()).getF16();
								break; case Token::Kind::TYPE_BF16:
									return type_manager.getMax(expected_type_info.baseTypeID()).getBF16();
								break; case Token::Kind::TYPE_F32:
									return type_manager.getMax(expected_type_info.baseTypeID()).getF32();
								break; case Token::Kind::TYPE_F64:
									return type_manager.getMax(expected_type_info.baseTypeID()).getF64();
								break; case Token::Kind::TYPE_F80:
									return type_manager.getMax(expected_type_info.baseTypeID()).getF80();
								break; case Token::Kind::TYPE_F128:
									return type_manager.getMax(expected_type_info.baseTypeID()).getF128();

								break; case Token::Kind::TYPE_C_LONG_DOUBLE: {
									if(type_manager.numBytes(expected_type_info.baseTypeID()) == 8){
										return type_manager.getMax(expected_type_info.baseTypeID()).getF64();
									}else{
										return type_manager.getMax(expected_type_info.baseTypeID()).getF128();
									}
								}
								break; default: evo::debugFatalBreak("Unknown float type");
							}
						}().asF128();


						const core::GenericFloat converted_literal = float_value.value.asF128();

						if(converted_literal.lt(target_min) || converted_literal.gt(target_max)){
							if constexpr(MAY_EMIT_ERROR){
								auto infos = evo::SmallVector<Diagnostic::Info>();
								this->diagnostic_print_type_info(expected_type_id, infos, "Target type: ");
								this->emit_error(
									Diagnostic::Code::SEMA_CANNOT_CONVERT_FLUID_VALUE,
									location,
									"Cannot implicitly convert this fluid value to the target type "
										"as it would require truncation",
									std::move(infos)
								);
							}
							return TypeCheckInfo::fail();
						}


						switch(expected_type_primitive.kind()){
							break; case Token::Kind::TYPE_F16:  float_value.value = float_value.value.asF16();
							break; case Token::Kind::TYPE_BF16: float_value.value = float_value.value.asBF16();
							break; case Token::Kind::TYPE_F32:  float_value.value = float_value.value.asF32();
							break; case Token::Kind::TYPE_F64:  float_value.value = float_value.value.asF64();
							break; case Token::Kind::TYPE_F80:  float_value.value = float_value.value.asF80();
							break; case Token::Kind::TYPE_F128: float_value.value = float_value.value.asF128();
							break; case Token::Kind::TYPE_C_LONG_DOUBLE: {
								if(type_manager.numBytes(expected_type_info.baseTypeID()) == 8){
									float_value.value = float_value.value.asF64();
								}else{
									float_value.value = float_value.value.asF128();
								}
							}
						}

						float_value.typeID = type_manager.getTypeInfo(expected_type_id).baseTypeID();
					}
				}

				if constexpr(MAY_IMPLICITLY_CONVERT){
					got_expr.value_category = TermInfo::ValueCategory::EPHEMERAL;
					got_expr.type_id.emplace<TypeInfo::ID>(expected_type_id);

					if(is_implicit_conversion_to_optional){
						got_expr.getExpr() = sema::Expr(
							this->context.sema_buffer.createConversionToOptional(got_expr.getExpr(), expected_type_id)
						);
					}
				}

				return TypeCheckInfo::success(true);
			} break;


			case TermInfo::ValueCategory::NULL_VALUE: {
				const TypeInfo& expected_type = type_manager.getTypeInfo(actual_expected_type_id);
				
				if(expected_type.isOptional() == false){
					this->emit_error(
						Diagnostic::Code::SEMA_ASSIGNED_NULL_TO_NON_OPTIONAL,
						location,
						"Value [null] can only be assigned to optional types"
					);
					return TypeCheckInfo::fail();
				}

				if(type_manager.isTypeDeducer(actual_expected_type_id)){
					this->emit_error(
						Diagnostic::Code::SEMA_CANNOT_EXTRACT_DEDUCERS_FROM_NULL,
						location,
						"Cannot extract deducers from [null]"
					);
					return TypeCheckInfo::fail();
				}

				if constexpr(MAY_IMPLICITLY_CONVERT){
					this->context.sema_buffer.nulls[got_expr.getExpr().nullID()].targetTypeID = expected_type_id;
				}

				return TypeCheckInfo::success(true);
			} break;

			case TermInfo::ValueCategory::EXPR_DEDUCER:
				evo::debugFatalBreak("EXPR_DEDUCER should not be compared with this function");

			case TermInfo::ValueCategory::INITIALIZER:
				evo::debugFatalBreak("INITIALIZER should not be compared with this function");

			case TermInfo::ValueCategory::MODULE:
				evo::debugFatalBreak("MODULE should not be compared with this function");

			case TermInfo::ValueCategory::FUNCTION:
				evo::debugFatalBreak("FUNCTION should not be compared with this function");

			case TermInfo::ValueCategory::INTRINSIC_FUNC:
				evo::debugFatalBreak("INTRINSIC_FUNC should not be compared with this function");

			case TermInfo::ValueCategory::TEMPLATE_INTRINSIC_FUNC:
				evo::debugFatalBreak("TEMPLATE_INTRINSIC_FUNC should not be compared with this function");

			case TermInfo::ValueCategory::TEMPLATE_TYPE:
				evo::debugFatalBreak("TEMPLATE_TYPE should not be compared with this function");

			case TermInfo::ValueCategory::TAGGED_UNION_FIELD_ACCESSOR:
				evo::debugFatalBreak("TAGGED_UNION_FIELD_ACCESSOR should not be compared with this function");
		}

		evo::debugFatalBreak("Unknown or unsupported value category");
	}


	auto SemanticAnalyzer::error_type_mismatch(
		TypeInfo::ID expected_type_id,
		const TermInfo& got_expr,
		std::string_view expected_type_location_name,
		const auto& location,
		std::optional<unsigned> multi_type_index
	) -> void {
		evo::debugAssert(
			std::isupper(int(expected_type_location_name[0])), "first character of name should be upper-case"
		);

		constexpr static bool LOCATION_IS_MULTI_ASSIGN = 
			std::is_same<std::decay_t<decltype(location)>, AST::MultiAssign>();

		std::string expected_type_str = std::string("Expected type: ");
		auto got_type_str = std::string("Expression is type: ");

		while(expected_type_str.size() < got_type_str.size()){
			expected_type_str += ' ';
		}

		while(got_type_str.size() < expected_type_str.size()){
			got_type_str += ' ';
		}

		auto infos = evo::SmallVector<Diagnostic::Info>();

		if constexpr(LOCATION_IS_MULTI_ASSIGN){
			infos.emplace_back(
				std::format("Multi-assign index: {}", *multi_type_index),
				this->get_location(location.assigns[*multi_type_index])
			);
		}


		this->diagnostic_print_type_info(expected_type_id, infos, expected_type_str);

		this->diagnostic_print_type_info(got_expr, multi_type_index, infos, got_type_str);


		const auto& actual_location = [&](){
			if constexpr(LOCATION_IS_MULTI_ASSIGN){
				return location.value;
			}else{
				return location;
			}
		}();


		this->emit_error(
			Diagnostic::Code::SEMA_TYPE_MISMATCH,
			actual_location,
			std::format(
				"{} cannot accept an expression of a different type, "
					"and this expression cannot be implicitly converted to the correct type",
				expected_type_location_name
			),
			std::move(infos)
		);
	}



	auto SemanticAnalyzer::diagnostic_print_type_info(
		TypeInfo::ID type_id, evo::SmallVector<Diagnostic::Info>& infos, std::string_view message
	) const -> void {
		auto initial_type_str = std::string();
		initial_type_str += message;
		initial_type_str += this->context.getTypeManager().printType(type_id, this->context.getSourceManager());
		infos.emplace_back(std::move(initial_type_str));

		this->diagnostic_print_type_info_impl(type_id, infos, message);
	}


	auto SemanticAnalyzer::diagnostic_print_type_info(
		const TermInfo& term_info,
		std::optional<unsigned> multi_type_index,
		evo::SmallVector<Diagnostic::Info>& infos,
		std::string_view message
	) const -> void {
		auto initial_type_str = std::string();
		initial_type_str += message;
		initial_type_str += this->print_term_type(term_info, multi_type_index);
		infos.emplace_back(std::move(initial_type_str));

		if(
			term_info.type_id.is<TypeInfo::ID>()
			|| (term_info.type_id.is<evo::SmallVector<TypeInfo::ID>>() && multi_type_index.has_value())
		){
			const TypeInfo::ID actual_got_type_id = [&](){
				if(term_info.type_id.is<TypeInfo::ID>()){
					return term_info.type_id.as<TypeInfo::ID>();
				}else{
					return term_info.type_id.as<evo::SmallVector<TypeInfo::ID>>()[*multi_type_index];
				}
			}();

			this->diagnostic_print_type_info_impl(actual_got_type_id, infos, message);
		}
	}



	auto SemanticAnalyzer::diagnostic_print_type_info_impl(
		TypeInfo::ID type_id, evo::SmallVector<Diagnostic::Info>& infos, std::string_view message
	) const -> void {
		evo::debugAssert(
			message.size() >= evo::stringSize(" > Alias of: "),
			"Message must be at least {} characters",
			evo::stringSize(" > Alias of: ")
		);


		while(true){
			const TypeInfo& actual_expected_type = this->context.getTypeManager().getTypeInfo(type_id);
			if(actual_expected_type.qualifiers().empty() == false){ break; }
			if(actual_expected_type.baseTypeID().kind() != BaseType::Kind::ALIAS){ break; }

			const BaseType::Alias& expected_alias = this->context.getTypeManager().getAlias(
				actual_expected_type.baseTypeID().aliasID()
			);

			evo::debugAssert(expected_alias.aliasedType.load().has_value(), "Definition of alias was not completed");
			type_id = *expected_alias.aliasedType.load();

			auto alias_of_str = std::string();
			alias_of_str.reserve(message.size());
			alias_of_str += " > Alias of: ";
			while(alias_of_str.size() < message.size()){
				alias_of_str += ' ';
			}

			alias_of_str += this->context.getTypeManager().printType(type_id, this->context.getSourceManager());

			infos.emplace_back(std::move(alias_of_str));
		}
	}




	auto SemanticAnalyzer::check_type_qualifiers(evo::ArrayProxy<AST::Type::Qualifier> qualifiers, const auto& location)
	-> evo::Result<> {
		bool found_read_only_ptr = false;
		for(ptrdiff_t i = qualifiers.size() - 1; i >= 0; i-=1){
			const AST::Type::Qualifier& qualifier = qualifiers[i];

			if(found_read_only_ptr){
				if(qualifier.isPtr && qualifier.isReadOnly == false){
					this->emit_error(
						Diagnostic::Code::SEMA_INVALID_TYPE_QUALIFIERS,
						location,
						"Invalid type qualifiers",
						Diagnostic::Info(
							"If one type qualifier level is a read-only pointer, "
							"all previous pointer qualifier levels must also be read-only"
						)
					);
					return evo::resultError;
				}

			}else if(qualifier.isPtr && qualifier.isReadOnly){
				found_read_only_ptr = true;
			}
		}
		return evo::Result<>();
	}



	auto SemanticAnalyzer::check_term_isnt_type(const TermInfo& term_info, const auto& location) -> evo::Result<> {
		if(term_info.value_category == TermInfo::ValueCategory::TYPE){
			this->emit_error(Diagnostic::Code::SEMA_TYPE_USED_AS_EXPR, location, "Type used as an expression");
			return evo::resultError;
		}

		return evo::Result<>();
	}



	auto SemanticAnalyzer::add_ident_to_scope(
		sema::ScopeManager::Scope& target_scope,
		std::string_view ident_str,
		const auto& ast_node,
		auto&&... ident_id_info
	) -> evo::Result<> {
		sema::ScopeLevel& current_scope_level = 
			this->context.sema_buffer.scope_manager.getLevel(target_scope.getCurrentLevel());

		const sema::ScopeLevel::AddIdentResult add_ident_result = current_scope_level.addIdent(
			ident_str, std::forward<decltype(ident_id_info)>(ident_id_info)...
		);

		if(add_ident_result.has_value() == false){
			const bool is_shadow_redef = add_ident_result.error();
			if(is_shadow_redef){
				const sema::ScopeLevel::IdentID& shadowed_ident =
					*current_scope_level.lookupDisallowedIdentForShadowing(ident_str);

				shadowed_ident.visit([&](const auto& first_decl_ident_id) -> void {
					using IdentIDType = std::decay_t<decltype(first_decl_ident_id)>;


					const Diagnostic::Location first_ident_location = [&]() -> Diagnostic::Location {
						if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::FuncOverloadList>()){
							return first_decl_ident_id.front().visit([&](const auto& func_id) -> Diagnostic::Location {
								return this->get_location(func_id);	
							});

						}else{
							return this->get_location(first_decl_ident_id);
						}
					}();
					
					this->emit_error(
						Diagnostic::Code::SEMA_IDENT_ALREADY_IN_SCOPE,
						ast_node,
						std::format("Identifier \"{}\" was already defined in this scope", ident_str),
						evo::SmallVector<Diagnostic::Info>{
							Diagnostic::Info("First defined here:", std::move(first_ident_location)),
							Diagnostic::Info("Note: shadowing is not allowed")
						}
					);
				});

			}else{
				this->error_already_defined<false>(
					ast_node,
					ident_str,
					*current_scope_level.lookupIdent(ident_str),
					std::forward<decltype(ident_id_info)>(ident_id_info)...
				);
			}

			return evo::resultError;
		}

		if(current_scope_level.doesShadowingChecks()){
			for(auto iter = std::next(target_scope.begin()); iter != target_scope.end(); ++iter){
				sema::ScopeLevel& scope_level = this->context.sema_buffer.scope_manager.getLevel(*iter);
				if(scope_level.disallowIdentForShadowing(ident_str, add_ident_result.value()) == false){
					this->error_already_defined<true>(
						ast_node,
						ident_str,
						*scope_level.lookupIdent(ident_str),
						std::forward<decltype(ident_id_info)>(ident_id_info)...
					);
					return evo::resultError;
				}
			}
		}

		return evo::Result<>();
	}


	template<bool IS_SHADOWING>
	auto SemanticAnalyzer::error_already_defined_impl(
		const auto& redef_id,
		std::string_view ident_str,
		const sema::ScopeLevel::IdentID& first_defined_id,
		std::optional<sema::Func::ID> attempted_decl_func_id
	)  -> void {
		first_defined_id.visit([&](const auto& first_decl_ident_id) -> void {
			using IdentIDType = std::decay_t<decltype(first_decl_ident_id)>;

			static constexpr bool IS_FUNC_OVERLOAD_COLLISION = 
				std::is_same<std::remove_cvref_t<std::decay_t<decltype(redef_id)>>, pcit::panther::AST::FuncDef>() 
				&& std::is_same<IdentIDType, sema::ScopeLevel::FuncOverloadList>()
				&& !IS_SHADOWING;

			auto infos = evo::SmallVector<Diagnostic::Info>();

			if constexpr(IS_FUNC_OVERLOAD_COLLISION){
				const sema::Func& attempted_decl_func = this->context.getSemaBuffer().getFunc(*attempted_decl_func_id);

				for(const evo::Variant<sema::Func::ID, sema::TemplatedFunc::ID>& overload_id : first_decl_ident_id){
					if(overload_id.is<sema::TemplatedFunc::ID>()){ continue; }

					const sema::Func& overload = this->context.sema_buffer.getFunc(overload_id.as<sema::Func::ID>());
					if(attempted_decl_func.isEquivalentOverload(overload, this->context)){
						// TODO(FUTURE): better messaging
						infos.emplace_back(
							"Overload collided with:", this->get_location(overload_id.as<sema::Func::ID>())
						);
						break;
					}
				}
				
			}else if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::FuncOverloadList>()){
				first_decl_ident_id.front().visit([&](const auto& func_id) -> void {
					if(first_decl_ident_id.size() == 1){
						infos.emplace_back("First defined here:", this->get_location(func_id));

					}else if(first_decl_ident_id.size() == 2){
						infos.emplace_back(
							"First defined here (and 1 other place):", this->get_location(func_id)
						);
					}else{
						infos.emplace_back(
							std::format(
								"First defined here (and {} other places):", first_decl_ident_id.size() - 1
							),
							this->get_location(func_id)
						);
					}
				});

			}else{
				infos.emplace_back("First defined here:", this->get_location(first_decl_ident_id));
			}


			if constexpr(IS_SHADOWING){
				infos.emplace_back("Note: shadowing is not allowed");
			}

			const std::string message = [&](){
				if constexpr(IS_FUNC_OVERLOAD_COLLISION){
					return std::format(
						"Function \"{}\" has an overload that collides with this declaration", ident_str
					);
				}else{
					return std::format("Identifier \"{}\" was already defined in this scope", ident_str);
				}
			}();


			this->emit_error(
				Diagnostic::Code::SEMA_IDENT_ALREADY_IN_SCOPE, redef_id, std::move(message), std::move(infos)
			);
		});
	};



	auto SemanticAnalyzer::print_term_type(
		const TermInfo& term_info, std::optional<unsigned> multi_type_index
	) const -> std::string {
		return term_info.type_id.visit([&](const auto& type_id) -> std::string {
			using TypeID = std::decay_t<decltype(type_id)>;

			if constexpr(std::is_same<TypeID, TermInfo::InitializerType>()){
				return "{INITIALIZER}";

			}else if constexpr(std::is_same<TypeID, TermInfo::NullType>()){	
				return "{NULL}";

			}else if constexpr(std::is_same<TypeID, TermInfo::ExprDeducerType>()){	
				return "{EXPR DEDUCER}";
				
			}else if constexpr(std::is_same<TypeID, TermInfo::FluidType>()){
				if(term_info.getExpr().kind() == sema::Expr::Kind::INT_VALUE){
					return "{FLUID INTEGRAL}";
				}else{
					evo::debugAssert(
						term_info.getExpr().kind() == sema::Expr::Kind::FLOAT_VALUE, "Unsupported fluid type"
					);
					return "{FLUID FLOAT}";
				}
				
			}else if constexpr(std::is_same<TypeID, TypeInfo::ID>()){
				return this->context.getTypeManager().printType(type_id, this->context.getSourceManager());

			}else if constexpr(std::is_same<TypeID, TermInfo::BuiltinTypeMethod>()){
				return "{BUILTIN TYPE METHOD}";

			}else if constexpr(std::is_same<TypeID, TermInfo::FuncOverloadList>()){
				// TODO(FEATURE): actual name
				return "{FUNCTION}";

			}else if constexpr(std::is_same<TypeID, TypeInfo::VoidableID>()){
				return this->context.getTypeManager().printType(type_id, this->context.getSourceManager());

			}else if constexpr(std::is_same<TypeID, evo::SmallVector<TypeInfo::ID>>()){
				return this->context.getTypeManager().printType(
					type_id[*multi_type_index], this->context.getSourceManager()
				);

			}else if constexpr(std::is_same<TypeID, Source::ID>()){
				// TODO(FEATURE): actual module name?
				return "{MODULE}";

			}else if constexpr(std::is_same<TypeID, ClangSource::ID>()){
				// TODO(FEATURE): actual name?
				return "{CLANG MODULE}";

			}else if constexpr(std::is_same<TypeID, BuiltinModule::ID>()){
				// TODO(FEATURE): actual name?
				return "{BUILTIN MODULE}";

			}else if constexpr(std::is_same<TypeID, sema::TemplatedStruct::ID>()){
				// TODO(FEATURE): actual name
				return "{TEMPLATED STRUCT}";

			}else if constexpr(std::is_same<TypeID, TemplateIntrinsicFunc::Kind>()){
				// TODO(FEATURE): actual name
				return "{TEMPLATE INTRINSIC FUNC}";

			}else if constexpr(std::is_same<TypeID, TermInfo::TemplateDeclInstantiationType>()){
				// TODO(FEATURE): actual name?
				return "{TEMPLATE DECL INSTANTIATION TYPE}";

			}else if constexpr(std::is_same<TypeID, TermInfo::ExceptParamPack>()){
				return "{EXCEPT PARAM PACK}";

			}else if constexpr(std::is_same<TypeID, TermInfo::TaggedUnionFieldAccessor>()){
				// TODO(FEATURE): actual name?
				return "{TAGGED UNION FIELD ACCESSOR}";

			}else{
				static_assert(false, "Unsupported type id kind");
			}
		});
	}



	auto SemanticAnalyzer::check_scope_isnt_terminated(const auto& location) -> evo::Result<> {
		if(this->get_current_scope_level().isTerminated() == false){ return evo::Result<>(); }

		this->emit_error(
			Diagnostic::Code::SEMA_SCOPE_IS_ALREADY_TERMINATED,
			location,
			"Scope is already terminated"
		);
		return evo::resultError;
	}


}