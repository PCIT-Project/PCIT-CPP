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
		if(this->symbol_proc.passed_on_by_when_cond){ return; }

		while(this->symbol_proc.being_worked_on.exchange(true)){
			std::this_thread::yield();
		}
		EVO_DEFER([&](){
			evo::debugAssert(this->symbol_proc.being_worked_on == false, "Symbol Proc being worked on should be false");
		});

		while(this->symbol_proc.isAtEnd() == false){
			evo::debugAssert(
				this->symbol_proc.passed_on_by_when_cond == false,
				"symbol was passed on by when cond - should not be analyzed"
			);

			evo::debugAssert(
				this->symbol_proc.errored == false,
				"symbol was errored - should not be analyzed"
			);

			switch(this->analyze_instr(this->symbol_proc.getInstruction())){
				case Result::SUCCESS: {
					this->symbol_proc.nextInstruction();
				} break;

				case Result::ERROR: {
					this->context.symbol_proc_manager.symbol_proc_done();
					this->symbol_proc.errored = true;
					if(this->symbol_proc.extra_info.is<SymbolProc::StructInfo>()){
						SymbolProc::StructInfo& struct_info = this->symbol_proc.extra_info.as<SymbolProc::StructInfo>();
						if(struct_info.instantiation != nullptr){ struct_info.instantiation->errored = true; }
					}
					this->symbol_proc.being_worked_on = false;
					return;
				} break;

				case Result::RECOVERABLE_ERROR: {
					this->symbol_proc.errored = true;
					if(this->symbol_proc.extra_info.is<SymbolProc::StructInfo>()){
						SymbolProc::StructInfo& struct_info = this->symbol_proc.extra_info.as<SymbolProc::StructInfo>();
						if(struct_info.instantiation != nullptr){ struct_info.instantiation->errored = true; }
					}
					this->symbol_proc.nextInstruction();
				} break;

				case Result::NEED_TO_WAIT: {
					const auto lock = std::scoped_lock(this->symbol_proc.waiting_for_lock);
					if(this->symbol_proc.waiting_for.empty()){ continue; } // prevent race condition

					this->symbol_proc.being_worked_on = false;
					return;
				} break;

				case Result::NEED_TO_WAIT_BEFORE_NEXT_INSTR: {
					const auto lock = std::scoped_lock(this->symbol_proc.waiting_for_lock);
					if(this->symbol_proc.waiting_for.empty()){ continue; } // prevent race condition

					this->symbol_proc.nextInstruction();
					this->symbol_proc.being_worked_on = false;
					return;
				} break;
			}
		}

		this->context.trace("Finished semantic analysis of symbol: \"{}\"", this->symbol_proc.ident);
		this->symbol_proc.being_worked_on = false;
	}


	auto SemanticAnalyzer::analyze_instr(const Instruction& instruction) -> Result {
		return instruction.visit([&](const auto& instr) -> Result {
			using InstrType = std::decay_t<decltype(instr)>;


			if constexpr(std::is_same<InstrType, Instruction::NonLocalVarDecl>()){
				return this->instr_non_local_var_decl(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::NonLocalVarDef>()){
				return this->instr_non_local_var_def(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::NonLocalVarDeclDef>()){
				return this->instr_non_local_var_decl_def(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::WhenCond>()){
				return this->instr_when_cond(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::AliasDecl>()){
				return this->instr_alias_decl(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::AliasDef>()){
				return this->instr_alias_def(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::StructDecl<false>>()){
				return this->instr_struct_decl<false>(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::StructDecl<true>>()){
				return this->instr_struct_decl<true>(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::StructDef>()){
				return this->instr_struct_def();

			}else if constexpr(std::is_same<InstrType, Instruction::TemplateStruct>()){
				return this->instr_template_struct(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::FuncDecl<false>>()){
				return this->instr_func_decl<false>(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::FuncPrepareScopeAndPIRDecl>()){
				return this->instr_func_prepare_scope_and_pir_decl(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::FuncDef>()){
				return this->instr_func_def(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::FuncPrepareConstexprPIRIfNeeded>()){
				return this->instr_func_prepare_constexpr_pir_if_needed(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::FuncConstexprPIRReadyIfNeeded>()){
				return this->instr_func_constexpr_pir_ready_if_needed();

			}else if constexpr(std::is_same<InstrType, Instruction::TemplateFunc>()){
				return this->instr_template_func(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::LocalVar>()){
				return this->instr_local_var(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Return>()){
				return this->instr_return(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::LabeledReturn>()){
				return this->instr_labeled_return(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Error>()){
				return this->instr_error(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::BeginDefer>()){
				return this->instr_begin_defer(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::EndDefer>()){
				return this->instr_end_defer();

			}else if constexpr(std::is_same<InstrType, Instruction::Unreachable>()){
				return this->instr_unreachable(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::BeginStmtBlock>()){
				return this->instr_begin_stmt_block(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::EndStmtBlock>()){
				return this->instr_end_stmt_block();

			}else if constexpr(std::is_same<InstrType, Instruction::FuncCall>()){
				return this->instr_func_call(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Assignment>()){
				return this->instr_assignment(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::MultiAssign>()){
				return this->instr_multi_assign(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::DiscardingAssignment>()){
				return this->instr_discarding_assignment(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::TypeToTerm>()){
				return this->instr_type_to_term(instr);

			// }else if constexpr(std::is_same<InstrType, Instruction::FuncCallExpr<true, true>>()){
			// 	return this->instr_func_call_expr<true, true>(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::FuncCallExpr<true, false>>()){
				return this->instr_func_call_expr<true, false>(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::FuncCallExpr<false, true>>()){
				return this->instr_func_call_expr<false, true>(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::FuncCallExpr<false, false>>()){
				return this->instr_func_call_expr<false, false>(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::ConstexprFuncCallRun>()){
				return this->instr_constexpr_func_call_run(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Import>()){
				return this->instr_import(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::TemplateIntrinsicFuncCall<true>>()){
				return this->instr_template_intrinsic_func_call<true>(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::TemplateIntrinsicFuncCall<false>>()){
				return this->instr_template_intrinsic_func_call<false>(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::TemplatedTerm>()){
				return this->instr_templated_term(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::TemplatedTermWait>()){
				return this->instr_templated_term_wait(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::PushTemplateDeclInstantiationTypesScope>()){
				return this->instr_push_template_decl_instantiation_types_scope();

			}else if constexpr(std::is_same<InstrType, Instruction::PopTemplateDeclInstantiationTypesScope>()){
				return this->instr_pop_template_decl_instantiation_types_scope();

			}else if constexpr(std::is_same<InstrType, Instruction::AddTemplateDeclInstantiationType>()){
				return this->instr_add_template_decl_instantiation_type(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Copy>()){
				return this->instr_copy(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Move>()){
				return this->instr_move(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::AddrOf<true>>()){
				return this->instr_addr_of(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::AddrOf<false>>()){
				return this->instr_addr_of(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Deref>()){
				return this->instr_deref(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::StructInitNew<true>>()){
				return this->instr_struct_init_new<true>(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::StructInitNew<false>>()){
				return this->instr_struct_init_new<false>(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::PrepareTryHandler>()){
				return this->instr_prepare_try_handler(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::TryElse>()){
				return this->instr_try_else(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::BeginExprBlock>()){
				return this->instr_begin_expr_block(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::EndExprBlock>()){
				return this->instr_end_expr_block(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Accessor<true>>()){
				return this->instr_expr_accessor<true>(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Accessor<false>>()){
				return this->instr_expr_accessor<false>(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::PrimitiveType>()){
				return this->instr_primitive_type(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::UserType>()){
				return this->instr_user_type(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::BaseTypeIdent>()){
				return this->instr_base_type_ident(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Ident<true>>()){
				return this->instr_ident<true>(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Ident<false>>()){
				return this->instr_ident<false>(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Intrinsic>()){
				return this->instr_intrinsic(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Literal>()){
				return this->instr_literal(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Uninit>()){
				return this->instr_uninit(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Zeroinit>()){
				return this->instr_zeroinit(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::TypeDeducer>()){
				return this->instr_type_deducer(instr);

			}else{
				static_assert(false, "Unsupported instruction type");
			}
		});
	}



	auto SemanticAnalyzer::instr_non_local_var_decl(const Instruction::NonLocalVarDecl& instr) -> Result {
		const std::string_view var_ident = this->source.getTokenBuffer()[instr.var_decl.ident].getString();

		EVO_DEFER([&](){ this->context.trace("SemanticAnalyzer::instr_var_decl: {}", this->symbol_proc.ident); });

		const evo::Result<GlobalVarAttrs> var_attrs =
			this->analyze_global_var_attrs(instr.var_decl, instr.attribute_params_info);
		if(var_attrs.isError()){ return Result::ERROR; }


		const TypeInfo::VoidableID got_type_info_id = this->get_type(instr.type_id);

		if(got_type_info_id.isVoid()){
			this->emit_error(
				Diagnostic::Code::SEMA_VAR_TYPE_VOID,
				*instr.var_decl.type,
				"Variables cannot be type [Void]"
			);
			return Result::ERROR;
		}
		
		bool is_global = true;
		if(instr.var_decl.kind == AST::VarDecl::Kind::DEF){
			if(var_attrs.value().is_global){
				this->emit_error(
					Diagnostic::Code::SEMA_VAR_DEF_WITH_ATTR_GLOBAL,
					instr.var_decl,
					"A [def] variable should not have the attribute `#global`"
				);
				return Result::ERROR;
			}

		}else if(this->scope.isGlobalScope()){
			if(var_attrs.value().is_global){
				this->emit_error(
					Diagnostic::Code::SEMA_VAR_GLOBAL_VAR_WITH_ATTR_GLOBAL,
					instr.var_decl,
					"Global variable should not have the attribute `#global`"
				);
				return Result::ERROR;
			}
			
		}else{
			is_global = var_attrs.value().is_global;
		}


		if(is_global){
			const sema::GlobalVar::ID new_sema_var = this->context.sema_buffer.createGlobalVar(
				instr.var_decl.kind,
				instr.var_decl.ident,
				this->source.getID(),
				std::optional<sema::Expr>(),
				got_type_info_id.asTypeID(),
				var_attrs.value().is_pub,
				this->symbol_proc,
				this->symbol_proc_id
			);

			if(this->add_ident_to_scope(var_ident, instr.var_decl, new_sema_var).isError()){ return Result::ERROR; }

			this->symbol_proc.extra_info.emplace<SymbolProc::NonLocalVarInfo>(new_sema_var);

			if(instr.var_decl.kind == AST::VarDecl::Kind::CONST){
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
					instr.var_decl.kind, instr.var_decl.ident, got_type_info_id.asTypeID()
				);
				return uint32_t(current_struct.memberVars.size() - 1);
			}();

			if(this->add_ident_to_scope(
				var_ident, instr.var_decl.ident, instr.var_decl.ident, sema::ScopeLevel::MemberVarFlag{}
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
				if(instr.var_decl.kind != AST::VarDecl::Kind::VAR){
					this->emit_error(
						Diagnostic::Code::SEMA_VAR_INITIALIZER_ON_NON_VAR,
						instr.var_decl,
						"Only [var] variables can be defined with an initializer value"
					);
					return Result::ERROR;
				}

			}else{
				if(value_term_info.is_ephemeral() == false){
					if(this->check_term_isnt_type(value_term_info, *instr.var_decl.value).isError()){
						return Result::ERROR;
					}

					if(value_term_info.value_category == TermInfo::ValueCategory::MODULE){
						this->error_type_mismatch(
							var_type_id, value_term_info, "Variable definition", *instr.var_decl.value
						);
						return Result::ERROR;
					}

					this->emit_error(
						Diagnostic::Code::SEMA_VAR_DEF_NOT_EPHEMERAL,
						*instr.var_decl.value,
						"Cannot define a variable with a non-ephemeral value"
					);
					return Result::ERROR;
				}
				
				if(this->type_check<true>(
					var_type_id, value_term_info, "Variable definition", *instr.var_decl.value
				).ok == false){
					return Result::ERROR;
				}
			}

		}else if(is_global){
			this->emit_error(
				Diagnostic::Code::SEMA_VAR_GLOBAL_LIFETIME_VAR_WITHOUT_VALUE,
				instr.var_decl,
				"Varibales with global lifetime must be declared with a value"
			);
			return Result::ERROR;
		}



		if(is_global){
			const sema::GlobalVar::ID sema_var_id =
				this->symbol_proc.extra_info.as<SymbolProc::NonLocalVarInfo>().sema_id.as<sema::GlobalVar::ID>();
			sema::GlobalVar& sema_var = this->context.sema_buffer.global_vars[sema_var_id];

			sema_var.expr = this->get_term_info(*instr.value_id).getExpr();

			if(instr.var_decl.kind == AST::VarDecl::Kind::CONST){
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
						instr.var_decl,
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

			const auto lock = std::scoped_lock(current_struct.memberVarsLock);
			current_struct.memberVars[member_index].defaultValue = this->get_term_info(*instr.value_id).getExpr();
		}
		

		this->propagate_finished_def();
		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_non_local_var_decl_def(const Instruction::NonLocalVarDeclDef& instr) -> Result {
		const std::string_view var_ident = this->source.getTokenBuffer()[instr.var_decl.ident].getString();

		EVO_DEFER([&](){ this->context.trace("SemanticAnalyzer::instr_var_decl_def: {}", this->symbol_proc.ident); });

		const evo::Result<GlobalVarAttrs> var_attrs =
			this->analyze_global_var_attrs(instr.var_decl, instr.attribute_params_info);
		if(var_attrs.isError()){ return Result::ERROR; }


		TermInfo& value_term_info = this->get_term_info(instr.value_id);
		if(value_term_info.value_category == TermInfo::ValueCategory::MODULE){
			if(instr.var_decl.kind != AST::VarDecl::Kind::DEF){
				this->emit_error(
					Diagnostic::Code::SEMA_MODULE_VAR_MUST_BE_DEF,
					*instr.var_decl.value,
					"Variable that has a module value must be declared as [def]"
				);
				return Result::ERROR;
			}

			const evo::Result<> add_ident_result = this->add_ident_to_scope(
				var_ident,
				instr.var_decl,
				value_term_info.type_id.as<Source::ID>(),
				instr.var_decl.ident,
				var_attrs.value().is_pub
			);

			// TODO(FUTURE): propgate if `add_ident_result` errored?
			this->propagate_finished_decl_def();
			return add_ident_result.isError() ? Result::ERROR : Result::SUCCESS;
		}


		if(value_term_info.value_category == TermInfo::ValueCategory::INITIALIZER){
			if(instr.var_decl.kind != AST::VarDecl::Kind::VAR){
				this->emit_error(
					Diagnostic::Code::SEMA_VAR_INITIALIZER_ON_NON_VAR,
					instr.var_decl,
					"Only [var] variables can be defined with an initializer value"
				);
				return Result::ERROR;
			}else{
				this->emit_error(
					Diagnostic::Code::SEMA_VAR_INITIALIZER_WITHOUT_EXPLICIT_TYPE,
					*instr.var_decl.value,
					"Cannot define a variable with an initializer value without an explicit type"
				);
				return Result::ERROR;
			}
		}


		if(value_term_info.is_ephemeral() == false){
			if(this->check_term_isnt_type(value_term_info, *instr.var_decl.value).isError()){ return Result::ERROR; }

			this->emit_error(
				Diagnostic::Code::SEMA_VAR_DEF_NOT_EPHEMERAL,
				*instr.var_decl.value,
				"Cannot define a variable with a non-ephemeral value"
			);
			return Result::ERROR;
		}

			
		if(value_term_info.isMultiValue()){
			this->emit_error(
				Diagnostic::Code::SEMA_MULTI_RETURN_INTO_SINGLE_VALUE,
				*instr.var_decl.value,
				"Cannot define a variable with multiple values"
			);
			return Result::ERROR;
		}

		if(
			instr.var_decl.kind != AST::VarDecl::Kind::DEF &&
			value_term_info.value_category == TermInfo::ValueCategory::EPHEMERAL_FLUID
		){
			this->emit_error(
				Diagnostic::Code::SEMA_CANNOT_INFER_TYPE,
				*instr.var_decl.value,
				"Cannot infer the type of a fluid literal",
				Diagnostic::Info("Did you mean this variable to be [def]? If not, give the variable an explicit type")
			);
			return Result::ERROR;
		}


		if(instr.type_id.has_value()){
			const TypeInfo::VoidableID got_type_info_id = this->get_type(*instr.type_id);

			if(got_type_info_id.isVoid()){
				this->emit_error(
					Diagnostic::Code::SEMA_VAR_TYPE_VOID, *instr.var_decl.type, "Variables cannot be type [Void]"
				);
				return Result::ERROR;
			}


			const TypeCheckInfo type_check_info = this->type_check<true>(
				got_type_info_id.asTypeID(), value_term_info, "Variable definition", *instr.var_decl.value
			);

			if(type_check_info.ok == false){ return Result::ERROR; }

			if(type_check_info.deduced_types.empty() == false){
				if(this->scope.isGlobalScope()){
					this->emit_error(
						Diagnostic::Code::SEMA_TYPE_DEDUCER_IN_GLOBAL_VAR,
						*instr.var_decl.type,
						"Global variables cannot have type deducers"
					);
					return Result::ERROR;
				}

				for(const DeducedType& deduced_type : type_check_info.deduced_types){
					if(
						this->add_ident_to_scope(
							this->source.getTokenBuffer()[deduced_type.tokenID].getString(),
							deduced_type.tokenID,
							deduced_type.typeID,
							deduced_type.tokenID,
							sema::ScopeLevel::DeducedTypeFlag{}
						).isError()
					){
						return Result::ERROR;
					}
				}
			}

		}

		const std::optional<TypeInfo::ID> type_id = [&](){
			if(value_term_info.type_id.is<TypeInfo::ID>()){
				return std::optional<TypeInfo::ID>(value_term_info.type_id.as<TypeInfo::ID>());
			}
			return std::optional<TypeInfo::ID>();
		}();



		bool is_global = true;
		if(instr.var_decl.kind == AST::VarDecl::Kind::DEF){
			if(var_attrs.value().is_global){
				this->emit_error(
					Diagnostic::Code::SEMA_VAR_DEF_WITH_ATTR_GLOBAL,
					instr.var_decl,
					"A [def] variable should not have the attribute `#global`"
				);
				return Result::ERROR;
			}

		}else if(this->scope.isGlobalScope()){
			if(var_attrs.value().is_global){
				this->emit_error(
					Diagnostic::Code::SEMA_VAR_GLOBAL_VAR_WITH_ATTR_GLOBAL,
					instr.var_decl,
					"Global variable should not have the attribute `#global`"
				);
				return Result::ERROR;
			}
			
		}else{
			is_global = var_attrs.value().is_global;
		}


		if(is_global){
			const sema::GlobalVar::ID new_sema_var = this->context.sema_buffer.createGlobalVar(
				instr.var_decl.kind,
				instr.var_decl.ident,
				this->source.getID(),
				std::optional<sema::Expr>(value_term_info.getExpr()),
				type_id,
				var_attrs.value().is_pub,
				this->symbol_proc,
				this->symbol_proc_id
			);

			if(this->add_ident_to_scope(var_ident, instr.var_decl, new_sema_var).isError()){ return Result::ERROR; }


			if(instr.var_decl.kind == AST::VarDecl::Kind::CONST){
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
						instr.var_decl,
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
					instr.var_decl.kind, instr.var_decl.ident, *type_id, value_term_info.getExpr()
				);
			}

			if(this->add_ident_to_scope(
				var_ident, instr.var_decl.ident, instr.var_decl.ident, sema::ScopeLevel::MemberVarFlag{}
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

		if(this->type_check<true>(
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
			passed_symbol.passed_on_by_when_cond = true;


			{
				const auto lock = std::scoped_lock(passed_symbol.decl_waited_on_lock, passed_symbol.def_waited_on_lock);
				this->context.symbol_proc_manager.symbol_proc_done();

				for(const SymbolProc::ID& decl_waited_on_id : passed_symbol.decl_waited_on_by){
					this->set_waiting_for_is_done(decl_waited_on_id, passed_symbol_id);
				}
				for(const SymbolProc::ID& def_waited_on_id : passed_symbol.def_waited_on_by){
					this->set_waiting_for_is_done(def_waited_on_id, passed_symbol_id);
				}
			}


			passed_symbol.extra_info.visit([&](const auto& extra_info) -> void {
				using ExtraInfo = std::decay_t<decltype(extra_info)>;

				if constexpr(std::is_same<ExtraInfo, std::monostate>()){
					return;

				}else if constexpr(std::is_same<ExtraInfo, SymbolProc::NonLocalVarInfo>()){
					return;

				}else if constexpr(std::is_same<ExtraInfo, SymbolProc::WhenCondInfo>()){
					for(const SymbolProc::ID& then_id : extra_info.then_ids){
						passed_symbols.push(then_id);
					}

					for(const SymbolProc::ID& else_id : extra_info.else_ids){
						passed_symbols.push(else_id);
					}

				}else if constexpr(std::is_same<ExtraInfo, SymbolProc::AliasInfo>()){
					return;

				}else if constexpr(std::is_same<ExtraInfo, SymbolProc::StructInfo>()){
					return;

				}else if constexpr(std::is_same<ExtraInfo, SymbolProc::FuncInfo>()){
					return;

				}else{
					static_assert(false, "Unsupported extra info");
				}
			});
		}

		this->propagate_finished_def();
		return Result::SUCCESS;
	}



	auto SemanticAnalyzer::instr_alias_decl(const Instruction::AliasDecl& instr) -> Result {
		EVO_DEFER([&](){ this->context.trace("SemanticAnalyzer::instr_alias_decl: {}", this->symbol_proc.ident); });

		auto attr_pub = ConditionalAttribute(*this, "pub");

		const AST::AttributeBlock& attribute_block = 
			this->source.getASTBuffer().getAttributeBlock(instr.alias_decl.attributeBlock);

		for(size_t i = 0; const AST::AttributeBlock::Attribute& attribute : attribute_block.attributes){
			EVO_DEFER([&](){ i += 1; });
			
			const std::string_view attribute_str = this->source.getTokenBuffer()[attribute.attribute].getString();

			if(attribute_str == "pub"){
				if(instr.attribute_params_info[i].empty()){
					if(attr_pub.set(attribute.attribute, true).isError()){ return Result::ERROR; } 

				}else if(instr.attribute_params_info[i].size() == 1){
					TermInfo& cond_term_info = this->get_term_info(instr.attribute_params_info[i][0]);
					if(this->check_term_isnt_type(cond_term_info, attribute.args[0]).isError()){ return Result::ERROR; }

					if(this->type_check<true>(
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
				this->source.getID(), instr.alias_decl.ident, std::optional<TypeInfoID>(), attr_pub.is_set()
			)
		);

		this->symbol_proc.extra_info.emplace<SymbolProc::AliasInfo>(created_alias.aliasID());

		const std::string_view ident_str = this->source.getTokenBuffer()[instr.alias_decl.ident].getString();
		if(this->add_ident_to_scope(ident_str, instr.alias_decl, created_alias.aliasID()).isError()){
			return Result::ERROR;
		}

		this->propagate_finished_decl();
		return Result::SUCCESS;
	}



	auto SemanticAnalyzer::instr_alias_def(const Instruction::AliasDef& instr) -> Result {
		EVO_DEFER([&](){ this->context.trace("SemanticAnalyzer::instr_var_def: {}", this->symbol_proc.ident); });

		BaseType::Alias& alias_info = this->context.type_manager.getAlias(
			this->symbol_proc.extra_info.as<SymbolProc::AliasInfo>().alias_id
		);

		const TypeInfo::VoidableID aliased_type = this->get_type(instr.aliased_type);
		if(aliased_type.isVoid()){
			this->emit_error(
				Diagnostic::Code::SEMA_ALIAS_CANNOT_BE_VOID,
				instr.alias_decl.type,
				"Alias cannot be type [Void]"
			);
			return Result::ERROR;
		}


		alias_info.aliasedType = aliased_type.asTypeID();

		this->propagate_finished_def();
		return Result::SUCCESS;
	};


	template<bool IS_INSTANTIATION>
	auto SemanticAnalyzer::instr_struct_decl(const Instruction::StructDecl<IS_INSTANTIATION>& instr) -> Result {
		EVO_DEFER([&](){ this->context.trace("SemanticAnalyzer::instr_struct_decl: {}", this->symbol_proc.ident); });

		const evo::Result<StructAttrs> struct_attrs =
			this->analyze_struct_attrs(instr.struct_decl, instr.attribute_params_info);
		if(struct_attrs.isError()){ return Result::ERROR; }


		///////////////////////////////////
		// create

		SymbolProc::StructInfo& struct_info = this->symbol_proc.extra_info.as<SymbolProc::StructInfo>();


		const BaseType::ID created_struct = this->context.type_manager.getOrCreateStruct(
			BaseType::Struct{
				.sourceID          = this->source.getID(),
				.identTokenID      = instr.struct_decl.ident,
				.instantiation     = instr.instantiation_id,
				.memberVars        = evo::SmallVector<BaseType::Struct::MemberVar>(),
				.memberVarsABI     = evo::SmallVector<BaseType::Struct::MemberVar*>(),
				.namespacedMembers = struct_info.member_symbols,
				.scopeLevel        = nullptr,
				.isPub             = struct_attrs.value().is_pub,
				.isOrdered         = struct_attrs.value().is_ordered,
				.isPacked          = struct_attrs.value().is_packed,
			}
		);

		struct_info.struct_id = created_struct.structID();

		if constexpr(IS_INSTANTIATION == false){
			const std::string_view ident_str = this->source.getTokenBuffer()[instr.struct_decl.ident].getString();
			if(this->add_ident_to_scope(ident_str, instr.struct_decl, created_struct.structID()).isError()){
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

			if(member_stmt.ast_node.kind() == AST::Kind::FUNC_DECL){
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

		this->propagate_finished_decl();

		if(struct_info.stmts.empty()){
			return Result::SUCCESS;
		}else{
			return Result::NEED_TO_WAIT_BEFORE_NEXT_INSTR;
		}
	}


	auto SemanticAnalyzer::instr_struct_def() -> Result {
		EVO_DEFER([&](){ this->context.trace("SemanticAnalyzer::instr_struct_def: {}", this->symbol_proc.ident); });

		this->pop_scope_level<>(); // TODO(FUTURE): needed?


		const BaseType::Struct::ID created_struct_id =
			this->symbol_proc.extra_info.as<SymbolProc::StructInfo>().struct_id;
		BaseType::Struct& created_struct = this->context.type_manager.getStruct(created_struct_id);


		const auto sorting_func = [](
			const BaseType::Struct::MemberVar& lhs, const BaseType::Struct::MemberVar& rhs
		) -> bool {
			return lhs.identTokenID.get() < rhs.identTokenID.get();
		};

		std::sort(created_struct.memberVars.begin(), created_struct.memberVars.end(), sorting_func);

		// TODO(FEATURE): optimal ordering (when not #ordered)

		for(BaseType::Struct::MemberVar& member_var : created_struct.memberVars){
			created_struct.memberVarsABI.emplace_back(&member_var);
		}


		auto sema_to_pir = SemaToPIR(
			this->context, this->context.constexpr_pir_module, this->context.constexpr_sema_to_pir_data
		);

		created_struct.constexprJITType = sema_to_pir.lowerStruct(created_struct_id);

		this->propagate_finished_def();

		this->context.type_manager.getStruct(
			this->symbol_proc.extra_info.as<SymbolProc::StructInfo>().struct_id
		).defCompleted = true;

		return Result::SUCCESS;
	}



	auto SemanticAnalyzer::instr_template_struct(const Instruction::TemplateStruct& instr) -> Result {
		EVO_DEFER([&](){
			this->context.trace("SemanticAnalyzer::instr_template_struct: {}", this->symbol_proc.ident);
		});


		size_t minimum_num_template_args = 0;
		auto params = evo::SmallVector<BaseType::StructTemplate::Param>();

		const AST::TemplatePack& ast_template_pack = 
			this->source.getASTBuffer().getTemplatePack(*instr.struct_decl.templatePack);

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
						"Template parameter cannot be type [Void]"
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

					const TypeCheckInfo type_check_info = this->type_check<true>(
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
				this->source.getID(), instr.struct_decl.ident, std::move(params), minimum_num_template_args
			)
		);
		
		const sema::TemplatedStruct::ID new_templated_struct = this->context.sema_buffer.createTemplatedStruct(
			created_struct_type_id.structTemplateID(), this->symbol_proc
		);

		const std::string_view ident_str = this->source.getTokenBuffer()[instr.struct_decl.ident].getString();
		if(this->add_ident_to_scope(ident_str, instr.struct_decl, new_templated_struct).isError()){
			return Result::ERROR;
		}

		this->propagate_finished_decl_def();

		return Result::SUCCESS;
	};



	template<bool IS_INSTANTIATION>
	auto SemanticAnalyzer::instr_func_decl(const Instruction::FuncDecl<IS_INSTANTIATION>& instr) -> Result {
		EVO_DEFER([&](){ this->context.trace("SemanticAnalyzer::instr_func_decl: {}", this->symbol_proc.ident); });

		const evo::Result<FuncAttrs> func_attrs =
			this->analyze_func_attrs(instr.func_decl, instr.attribute_params_info);
		if(func_attrs.isError()){ return Result::ERROR; }


		///////////////////////////////////
		// create func type

		const ASTBuffer& ast_buffer = this->source.getASTBuffer();

		auto params = evo::SmallVector<BaseType::Function::Param>();
		auto sema_params = evo::SmallVector<sema::Func::Param>();
		uint32_t min_num_args = 0;
		bool has_in_param = false;

		for(size_t i = 0; const std::optional<SymbolProc::TypeID>& symbol_proc_param_type_id : instr.params()){
			EVO_DEFER([&](){ i += 1; });

			const AST::FuncDecl::Param& param = instr.func_decl.params[i];
			
			evo::debugAssert(
				symbol_proc_param_type_id.has_value() == (param.name.kind() != AST::Kind::THIS),
				"[this] is the only must not have a type, and everything else must have a type"
			);


			if(symbol_proc_param_type_id.has_value()){ // regular param
				const TypeInfo::VoidableID param_type_id = this->get_type(*symbol_proc_param_type_id);

				if(param_type_id.isVoid()){
					this->emit_error(
						Diagnostic::Code::SEMA_PARAM_TYPE_VOID, *param.type, "Function parameter cannot be type [Void]"
					);
					return Result::ERROR;
				}

				const bool should_copy = [&](){
					if(param.kind != AST::FuncDecl::Param::Kind::READ){ return false; }
					return this->context.getTypeManager().isTriviallyCopyable(param_type_id.asTypeID())
						&& this->context.getTypeManager().isTriviallySized(param_type_id.asTypeID());
				}();

				if(param.kind == AST::FuncDecl::Param::Kind::IN){
					has_in_param = true;
				}

				params.emplace_back(param_type_id.asTypeID(), param.kind, should_copy);

				if(instr.default_param_values[i].has_value()){
					TermInfo default_param_value = this->get_term_info(*instr.default_param_values[i]);

					if(
						this->type_check<true>(
							param_type_id.asTypeID(),
							default_param_value,
							"Default value of function parameter",
							*instr.func_decl.params[i].defaultValue
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
				const std::optional<sema::ScopeManager::Scope::ObjectScope> current_type_scope = 
					this->scope.getCurrentTypeScopeIfExists();

				if(current_type_scope.has_value() == false){
					// TODO(FUTURE): better messaging
					this->emit_error(
						Diagnostic::Code::SEMA_INVALID_SCOPE_FOR_THIS,
						param.name,
						"[this] parameters are only valid inside type scope"
					);
					return Result::ERROR;
				}

				current_type_scope->visit([&](const auto& type_scope) -> void {
					using TypeScope = std::decay_t<decltype(type_scope)>;

					if constexpr(std::is_same<TypeScope, BaseType::Struct::ID>()){
						const TypeInfo::ID this_type = this->context.type_manager.getOrCreateTypeInfo(
							TypeInfo(BaseType::ID(type_scope))
						);
						params.emplace_back(this_type, param.kind);
					}else{
						evo::debugFatalBreak("Invalid object scope");
					}
				});

				sema_params.emplace_back(ast_buffer.getThis(param.name), std::nullopt);
				min_num_args += 1;
			}
		}


		auto return_params = evo::SmallVector<BaseType::Function::ReturnParam>();
		for(size_t i = 0; const SymbolProc::TypeID& symbol_proc_return_param_type_id : instr.returns()){
			EVO_DEFER([&](){ i += 1; });

			const TypeInfo::VoidableID type_id = this->get_type(symbol_proc_return_param_type_id);

			const AST::FuncDecl::Return& ast_return_param = instr.func_decl.returns[i];

			if(i == 0){
				if(type_id.isVoid() && ast_return_param.ident.has_value()){
					this->emit_error(
						Diagnostic::Code::SEMA_NAMED_VOID_RETURN,
						*ast_return_param.ident,
						"A function return parameter that is type [Void] cannot be named"
					);
					return Result::ERROR;
				}
			}else{
				if(type_id.isVoid()){
					this->emit_error(
						Diagnostic::Code::SEMA_NOT_FIRST_RETURN_VOID,
						ast_return_param.type,
						"Only the first function return parameter can be type [Void]"
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

			const AST::FuncDecl::Return& ast_error_return_param = instr.func_decl.errorReturns[i];

			if(i == 0){
				if(type_id.isVoid() && ast_error_return_param.ident.has_value()){
					this->emit_error(
						Diagnostic::Code::SEMA_NAMED_VOID_RETURN,
						*ast_error_return_param.ident,
						"A function error return parameter that is type [Void] cannot be named"
					);
					return Result::ERROR;
				}
			}else{
				if(type_id.isVoid()){
					this->emit_error(
						Diagnostic::Code::SEMA_NOT_FIRST_RETURN_VOID,
						ast_error_return_param.type,
						"Only the first function error return parameter can be type [Void]"
					);
					return Result::ERROR;
				}
			}

			error_return_params.emplace_back(ast_error_return_param.ident, type_id);
		}


		///////////////////////////////////
		// checking attributes

		if(func_attrs.value().is_entry){
			if(params.empty() == false){
				this->emit_error(
					Diagnostic::Code::SEMA_INVALID_ENTRY,
					instr.func_decl.params[0],
					"Functions with the [#entry] attribute cannot have parameters"
				);
				return Result::ERROR;
			}

			if(instr.func_decl.returns[0].ident.has_value()){
				this->emit_error(
					Diagnostic::Code::SEMA_INVALID_ENTRY,
					instr.func_decl.returns[0],
					"Functions with the [#entry] attribute cannot have named returns"
				);
				return Result::ERROR;
			}

			if(return_params[0].typeID.isVoid() || return_params[0].typeID != TypeManager::getTypeUI8()){
				this->emit_error(
					Diagnostic::Code::SEMA_INVALID_ENTRY,
					instr.func_decl.returns[0].type,
					"Functions with the [#entry] attribute must return [UI8]"
				);
				return Result::ERROR;
			}

			if(error_return_params.empty() == false){
				this->emit_error(
					Diagnostic::Code::SEMA_INVALID_ENTRY,
					instr.func_decl.errorReturns[0],
					"Functions with the [#entry] attribute cannot have error returns"
				);
				return Result::ERROR;
			}
		}



		///////////////////////////////////
		// create func

		const BaseType::ID created_func_base_type = this->context.type_manager.getOrCreateFunction(
			BaseType::Function(std::move(params), std::move(return_params), std::move(error_return_params))
		);

		const bool is_constexpr = !func_attrs.value().is_runtime;

		const sema::Func::ID created_func_id = this->context.sema_buffer.createFunc(
			instr.func_decl.name,
			this->source.getID(),
			created_func_base_type.funcID(),
			std::move(sema_params),
			this->symbol_proc,
			this->symbol_proc_id,
			min_num_args,
			func_attrs.value().is_pub,
			is_constexpr,
			has_in_param,
			instr.instantiation_id
		);

		if(func_attrs.value().is_entry){
			this->context.entry = created_func_id;
		}


		if constexpr(IS_INSTANTIATION == false){
			// TODO(FUTURE): manage overloads
			const Token::ID ident = this->source.getASTBuffer().getIdent(instr.func_decl.name);
			const std::string_view ident_str = this->source.getTokenBuffer()[ident].getString();
			if(this->add_ident_to_scope(ident_str, instr.func_decl, created_func_id, this->context).isError()){
				return Result::ERROR;
			}
		}


		sema::Func& created_func = this->context.sema_buffer.funcs[created_func_id];
		this->push_scope_level(&created_func.stmtBlock, created_func_id);

		///////////////////////////////////
		// done

		this->propagate_finished_decl();

		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_func_prepare_scope_and_pir_decl(const Instruction::FuncPrepareScopeAndPIRDecl& instr) 
	-> Result {
		const sema::Func::ID current_func_id = this->scope.getCurrentObjectScope().as<sema::Func::ID>();
		sema::Func& current_func = this->context.sema_buffer.funcs[current_func_id];


		//////////////////
		// prepare pir

		if(current_func.isConstexpr){
			this->symbol_proc.extra_info.emplace<SymbolProc::FuncInfo>();

			auto sema_to_pir = SemaToPIR(
				this->context, this->context.constexpr_pir_module, this->context.constexpr_sema_to_pir_data
			);

			current_func.constexprJITFunc = sema_to_pir.lowerFuncDecl(current_func_id);

			this->propagate_finished_pir_decl();
		}


		//////////////////
		// adding params to scope

		const BaseType::Function& func_type = this->context.getTypeManager().getFunction(current_func.typeID);

		uint32_t abi_index = 0;

		for(uint32_t i = 0; const AST::FuncDecl::Param& param : instr.func_decl.params){
			EVO_DEFER([&](){ i += 1; });

			const std::string_view param_name = this->source.getTokenBuffer()[
				this->source.getASTBuffer().getIdent(param.name)
			].getString();

			if(this->add_ident_to_scope(
				param_name, param, this->context.sema_buffer.createParam(i, abi_index)
			).isError()){
				return Result::ERROR;
			}

			abi_index += 1;
		}


		if(func_type.hasNamedReturns()){
			for(uint32_t i = 0; const AST::FuncDecl::Return& return_param : instr.func_decl.returns){
				EVO_DEFER([&](){ i += 1; });

				const std::string_view return_param_name =
					this->source.getTokenBuffer()[*return_param.ident].getString();

				if(this->add_ident_to_scope(
					return_param_name, return_param, this->context.sema_buffer.createReturnParam(i, abi_index)
				).isError()){
					return Result::ERROR;
				}

				abi_index += 1;
			}
		}


		if(func_type.hasNamedErrorReturns()){
			// account for the RET param
			if(func_type.returnsVoid() == false && func_type.hasNamedReturns() == false){
				abi_index += 1;
			}

			
			for(uint32_t i = 0; const AST::FuncDecl::Return& error_return_param : instr.func_decl.errorReturns){
				EVO_DEFER([&](){ i += 1; });

				if(this->add_ident_to_scope(
					this->source.getTokenBuffer()[*error_return_param.ident].getString(),
					error_return_param,
					this->context.sema_buffer.createErrorReturnParam(i, abi_index)
				).isError()){
					return Result::ERROR;
				}
			}
		}

		return Result::SUCCESS;
	}



	auto SemanticAnalyzer::instr_func_def(const Instruction::FuncDef& instr) -> Result {
		EVO_DEFER([&](){ this->context.trace("SemanticAnalyzer::instr_func_def: {}", this->symbol_proc.ident); });

		const sema::Func& current_func = this->get_current_func();
		const BaseType::Function& func_type = this->context.getTypeManager().getFunction(current_func.typeID);


		if(this->get_current_scope_level().isTerminated()){
			this->get_current_func().isTerminated = true;

		}else{
			if(func_type.returnsVoid() == false){
				this->emit_error(
					Diagnostic::Code::SEMA_FUNC_ISNT_TERMINATED,
					instr.func_decl,
					"Function isn't terminated",
					Diagnostic::Info(
						"A function is terminated when all control paths end in a [return], [error], [unreachable], "
						"or a function call that has the attribute [#noReturn]"
					)
				);
				return Result::ERROR;
			}
		}

		this->get_current_func().defCompleted = true;
		this->propagate_finished_def();


		if(current_func.isConstexpr){
			bool any_waiting = false;
			for(
				sema::Func::ID dependent_func_id
				: this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>().dependent_funcs
			){
				const sema::Func& dependent_func = this->context.getSemaBuffer().getFunc(dependent_func_id);
				const SymbolProc::WaitOnResult wait_on_result = dependent_func.symbolProc.waitOnPIRDeclIfNeeded(
					this->symbol_proc_id, this->context, dependent_func.symbolProcID
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


				auto module_subset_funcs = evo::StaticVector<pir::Function::ID, 2>();
				module_subset_funcs.emplace_back(*sema_func.constexprJITFunc);
				if(func_type.returnsVoid() == false){
					sema_func.constexprJITInterfaceFunc = sema_to_pir.createFuncJITInterface(
						sema_func_id, *sema_func.constexprJITFunc
					);
					module_subset_funcs.emplace_back(*sema_func.constexprJITInterfaceFunc);
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
						instr.func_decl,
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
				const SymbolProc::WaitOnResult wait_on_result = dependent_func.symbolProc.waitOnPIRDefIfNeeded(
					this->symbol_proc_id, this->context, dependent_func.symbolProcID
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
				const SymbolProc::WaitOnResult wait_on_result = dependent_var.symbolProc.waitOnDefIfNeeded(
					this->symbol_proc_id, this->context, dependent_var.symbolProcID
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

		this->pop_scope_level<>();

		return Result::SUCCESS;
	}



	// TODO(FUTURE): condense this with template struct somehow?
	auto SemanticAnalyzer::instr_template_func(const Instruction::TemplateFunc& instr) -> Result {
		EVO_DEFER([&](){
			this->context.trace("SemanticAnalyzer::instr_template_func: {}", this->symbol_proc.ident);
		});


		size_t minimum_num_template_args = 0;
		auto params = evo::SmallVector<sema::TemplatedFunc::Param>();

		for(const SymbolProc::Instruction::TemplateParamInfo& template_param_info : instr.template_param_infos){
			auto type_id = std::optional<TypeInfo::ID>();
			if(template_param_info.type_id.has_value()){
				const TypeInfo::VoidableID type_info_voidable_id = this->get_type(*template_param_info.type_id);
				if(type_info_voidable_id.isVoid()){
					this->emit_error(
						Diagnostic::Code::SEMA_TEMPLATE_PARAM_CANNOT_BE_TYPE_VOID,
						template_param_info.param.type,
						"Template parameter cannot be type [Void]"
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

					const TypeCheckInfo type_check_info = this->type_check<true>(
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
				params.emplace_back(type_id, std::monostate());

			}else if(default_value->value_category == TermInfo::ValueCategory::TYPE){
				params.emplace_back(type_id, default_value->type_id.as<TypeInfo::VoidableID>());

			}else{
				params.emplace_back(type_id, default_value->getExpr());
			}
		}

		evo::debugAssert(instr.func_decl.name.kind() == AST::Kind::IDENT, "templated overloads are not allowed");
		const Token::ID ident = this->source.getASTBuffer().getIdent(instr.func_decl.name);
		const std::string_view ident_str = this->source.getTokenBuffer()[ident].getString();
		
		const sema::TemplatedFunc::ID new_templated_func = this->context.sema_buffer.createTemplatedFunc(
			this->symbol_proc, minimum_num_template_args, std::move(params)
		);

		if(this->add_ident_to_scope(ident_str, instr.func_decl, new_templated_func).isError()){
			return Result::ERROR;
		}

		this->propagate_finished_decl_def();

		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_local_var(const Instruction::LocalVar& instr) -> Result {
		if(this->check_scope_isnt_terminated(instr.var_decl).isError()){ return Result::ERROR; }

		const std::string_view var_ident = this->source.getTokenBuffer()[instr.var_decl.ident].getString();

		const evo::Result<VarAttrs> var_attrs = this->analyze_var_attrs(instr.var_decl, instr.attribute_params_info);
		if(var_attrs.isError()){ return Result::ERROR; }

		if(var_attrs.value().is_global){
			this->emit_error(
				Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
				instr.var_decl,
				"Static variables are currently unimplemented"
			);
			return Result::ERROR;
		}


		TermInfo& value_term_info = this->get_term_info(instr.value);
		if(value_term_info.value_category == TermInfo::ValueCategory::MODULE){
			if(instr.var_decl.kind != AST::VarDecl::Kind::DEF){
				this->emit_error(
					Diagnostic::Code::SEMA_MODULE_VAR_MUST_BE_DEF,
					*instr.var_decl.value,
					"Variable that has a module value must be declared as [def]"
				);
				return Result::ERROR;
			}

			const evo::Result<> add_ident_result = this->add_ident_to_scope(
				var_ident,
				instr.var_decl,
				value_term_info.type_id.as<Source::ID>(),
				instr.var_decl.ident,
				false
			);

			return add_ident_result.isError() ? Result::ERROR : Result::SUCCESS;
		}


		if(value_term_info.value_category == TermInfo::ValueCategory::INITIALIZER){
			if(instr.var_decl.kind != AST::VarDecl::Kind::VAR){
				this->emit_error(
					Diagnostic::Code::SEMA_VAR_INITIALIZER_ON_NON_VAR,
					instr.var_decl,
					"Only [var] variables can be defined with an initializer value"
				);
				return Result::ERROR;
			}else if(instr.type_id.has_value() == false){
				this->emit_error(
					Diagnostic::Code::SEMA_VAR_INITIALIZER_WITHOUT_EXPLICIT_TYPE,
					*instr.var_decl.value,
					"Cannot define a variable with an initializer value without an explicit type"
				);
				return Result::ERROR;
			}
		}else if(value_term_info.is_ephemeral() == false){
			if(this->check_term_isnt_type(value_term_info, *instr.var_decl.value).isError()){ return Result::ERROR; }

			this->emit_error(
				Diagnostic::Code::SEMA_VAR_DEF_NOT_EPHEMERAL,
				*instr.var_decl.value,
				"Cannot define a variable with a value that is not ephemeral or an initializer value"
			);
			return Result::ERROR;
		}

			
		if(value_term_info.isMultiValue()){
			this->emit_error(
				Diagnostic::Code::SEMA_MULTI_RETURN_INTO_SINGLE_VALUE,
				*instr.var_decl.value,
				"Cannot define a variable with multiple values"
			);
			return Result::ERROR;
		}


		if(instr.type_id.has_value()){
			const TypeInfo::VoidableID got_type_info_id = this->get_type(*instr.type_id);

			if(got_type_info_id.isVoid()){
				this->emit_error(
					Diagnostic::Code::SEMA_VAR_TYPE_VOID, *instr.var_decl.type, "Variables cannot be type [Void]"
				);
				return Result::ERROR;
			}


			if(value_term_info.value_category != TermInfo::ValueCategory::INITIALIZER){
				const TypeCheckInfo type_check_info = this->type_check<true>(
					got_type_info_id.asTypeID(), value_term_info, "Variable definition", *instr.var_decl.value
				);

				if(type_check_info.ok == false){ return Result::ERROR; }

				for(const DeducedType& deduced_type : type_check_info.deduced_types){
					const std::string_view deduced_type_ident_str = 
						this->source.getTokenBuffer()[deduced_type.tokenID].getString();

					if(this->add_ident_to_scope(
						deduced_type_ident_str,
						deduced_type.tokenID,
						deduced_type.typeID,
						deduced_type.tokenID,
						sema::ScopeLevel::DeducedTypeFlag{}
					).isError()){ return Result::ERROR; }
				}
			}

		}else if(
			instr.var_decl.kind != AST::VarDecl::Kind::DEF &&
			value_term_info.value_category == TermInfo::ValueCategory::EPHEMERAL_FLUID
		){
			this->emit_error(
				Diagnostic::Code::SEMA_CANNOT_INFER_TYPE,
				*instr.var_decl.value,
				"Cannot infer the type of a fluid literal",
				Diagnostic::Info("Did you mean this variable to be [def]? If not, give the variable an explicit type")
			);
			return Result::ERROR;
		}

		const std::optional<TypeInfo::ID> type_id = [&]() -> std::optional<TypeInfo::ID> {
			if(value_term_info.type_id.is<TypeInfo::ID>()){
				return std::optional<TypeInfo::ID>(value_term_info.type_id.as<TypeInfo::ID>());
			}

			if(value_term_info.value_category == TermInfo::ValueCategory::INITIALIZER){
				return this->get_type(*instr.type_id).asTypeID();
			}

			return std::optional<TypeInfo::ID>();
		}();

		const sema::Var::ID new_sema_var = this->context.sema_buffer.createVar(
			instr.var_decl.kind, instr.var_decl.ident, value_term_info.getExpr(), type_id
		);
		this->get_current_scope_level().stmtBlock().emplace_back(new_sema_var);

		if(this->add_ident_to_scope(var_ident, instr.var_decl, new_sema_var).isError()){ return Result::ERROR; }

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
					"Functions that have a return type other than [Void] must return a value"
				);
				return Result::ERROR;
			}

			if(current_func_type.hasNamedReturns()){
				this->emit_error(
					Diagnostic::Code::SEMA_INCORRECT_RETURN_STMT_KIND,
					instr.return_stmt,
					"Incorrect return statement kind for a function named return parameters",
					Diagnostic::Info("Initialize/set all return values and use \"return...;\" instead")
				);
				return Result::ERROR;
			}
			
		}else if(instr.return_stmt.value.is<AST::Node>()){ // return {EXPRESSION};
			evo::debugAssert(instr.value.has_value(), "Return value needs to have value analyzed");

			if(current_func_type.returnsVoid()){
				this->emit_error(
					Diagnostic::Code::SEMA_INCORRECT_RETURN_STMT_KIND,
					instr.return_stmt,
					"Functions that have a return type of [Void] cannot return a value"
				);
				return Result::ERROR;
			}

			if(current_func_type.hasNamedReturns()){
				this->emit_error(
					Diagnostic::Code::SEMA_INCORRECT_RETURN_STMT_KIND,
					instr.return_stmt,
					"Incorrect return statement kind for a function with named return parameters",
					Diagnostic::Info("Initialize/set all return values and use \"return...;\" instead")
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

			if(this->type_check<true>(
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
					"Functions that have a return type of [Void] cannot return a value"
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
		}

		const sema::Return::ID sema_return_id = this->context.sema_buffer.createReturn(return_value, std::nullopt);

		this->get_current_scope_level().stmtBlock().emplace_back(sema_return_id);
		this->get_current_scope_level().setTerminated();

		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_labeled_return(const Instruction::LabeledReturn& instr) -> Result {
		evo::debugAssert(instr.return_stmt.label.has_value(), "Not a labeled return");

		if(this->check_scope_isnt_terminated(instr.return_stmt).isError()){ return Result::ERROR; }

		const Token::ID target_label_id = ASTBuffer::getIdent(*instr.return_stmt.label);
		sema::ScopeLevel::ID scope_level_id = sema::ScopeLevel::ID::dummy();
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
					Diagnostic::Info("Initialize/set all return values and use \"return...;\" instead")
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

			if(this->type_check<true>(
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
		}


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
					"Value of error return statement is not ephemeral"
				);
				return Result::ERROR;
			}

			if(this->type_check<true>(
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
		}

		const sema::Error::ID sema_error_id = this->context.sema_buffer.createError(error_value);

		this->get_current_scope_level().stmtBlock().emplace_back(sema_error_id);
		this->get_current_scope_level().setTerminated();

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


	auto SemanticAnalyzer::instr_end_defer() -> Result {
		this->pop_scope_level();
		this->get_current_scope_level().resetSubScopes();
		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_unreachable(const Instruction::Unreachable& instr) -> Result {
		if(this->check_scope_isnt_terminated(instr.keyword).isError()){ return Result::ERROR; }
		
		this->get_current_scope_level().stmtBlock().emplace_back(sema::Stmt::createUnreachable(instr.keyword));
		this->get_current_scope_level().setTerminated();

		return Result::SUCCESS;
	}



	auto SemanticAnalyzer::instr_begin_stmt_block(const Instruction::BeginStmtBlock& instr) -> Result {
		if(this->check_scope_isnt_terminated(instr.stmt_block).isError()){ return Result::ERROR; }

		this->push_scope_level(&this->get_current_scope_level().stmtBlock());
		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_end_stmt_block() -> Result {
		this->pop_scope_level();
		this->get_current_scope_level().resetSubScopes();
		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_func_call(const Instruction::FuncCall& instr) -> Result {
		if(this->check_scope_isnt_terminated(instr.func_call).isError()){ return Result::ERROR; }

		const TermInfo& target_term_info = this->get_term_info(instr.target);

		const evo::Result<FuncCallImplData> func_call_impl_res = this->func_call_impl<false, false>(
			instr.func_call, target_term_info, instr.args, std::nullopt
		);
		if(func_call_impl_res.isError()){ return Result::ERROR; }

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

		if(this->symbol_proc.extra_info.is<SymbolProc::FuncInfo>() && !func_call_impl_res.value().is_intrinsic()){
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
		for(const SymbolProc::TermInfoID& arg : instr.args){
			sema_args.emplace_back(this->get_term_info(arg).getExpr());
		}



		if(func_call_impl_res.value().is_intrinsic()) [[unlikely]] {
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

			const sema::FuncCall::ID sema_func_call_id = this->context.sema_buffer.createFuncCall(
				*func_call_impl_res.value().selected_func_id, std::move(sema_args)
			);

			this->get_current_scope_level().stmtBlock().emplace_back(sema_func_call_id);
		}

		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_assignment(const Instruction::Assignment& instr) -> Result {
		if(this->check_scope_isnt_terminated(instr.infix).isError()){ return Result::ERROR; }

		const TermInfo& lhs = this->get_term_info(instr.lhs);
		TermInfo& rhs = this->get_term_info(instr.rhs);

		if(lhs.is_concrete() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_ASSIGN_LHS_NOT_CONCRETE,
				instr.infix.lhs,
				"LHS of assignment must be concrete"
			);
			return Result::ERROR;
		}

		if(lhs.is_const()){
			this->emit_error(
				Diagnostic::Code::SEMA_ASSIGN_LHS_NOT_MUTABLE,
				instr.infix.lhs,
				"LHS of assignment must be mutable"
			);
			return Result::ERROR;
		}

		if(rhs.is_ephemeral() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_ASSIGN_RHS_NOT_EPHEMERAL,
				instr.infix.rhs,
				"RHS of assignment must be ephemeral"
			);
			return Result::ERROR;
		}


		if(this->type_check<true>(
			lhs.type_id.as<TypeInfo::ID>(), rhs, "RHS of assignment", instr.infix.rhs
		).ok == false){
			return Result::ERROR;
		}

		this->get_current_scope_level().stmtBlock().emplace_back(
			this->context.sema_buffer.createAssign(lhs.getExpr(), rhs.getExpr())
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

			if(target.is_const()){
				this->emit_error(
					Diagnostic::Code::SEMA_ASSIGN_LHS_NOT_MUTABLE,
					instr.multi_assign.assigns[i],
					"LHS of assignment must be mutable"
				);
				return Result::ERROR;
			}

			if(this->type_check<true>(
				target.type_id.as<TypeInfo::ID>(), value, "RHS of assignment", instr.multi_assign, unsigned(i)
			).ok == false){
				return Result::ERROR;
			}

			targets.emplace_back(target.getExpr());
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





	auto SemanticAnalyzer::instr_type_to_term(const Instruction::TypeToTerm& instr) -> Result {
		this->return_term_info(instr.to,
			TermInfo::ValueCategory::TYPE, TermInfo::ValueStage::CONSTEXPR, this->get_type(instr.from), std::nullopt
		);
		return Result::SUCCESS;
	}


	template<bool IS_CONSTEXPR, bool ERRORS>
	auto SemanticAnalyzer::instr_func_call_expr(const Instruction::FuncCallExpr<IS_CONSTEXPR, ERRORS>& instr)
	-> Result {
		const TermInfo& target_term_info = this->get_term_info(instr.target);

		const evo::Result<FuncCallImplData> func_call_impl_res = this->func_call_impl<IS_CONSTEXPR, ERRORS>(
			instr.func_call, target_term_info, instr.args, std::nullopt
		);
		if(func_call_impl_res.isError()){ return Result::ERROR; }


		auto sema_args = evo::SmallVector<sema::Expr>();
		for(const SymbolProc::TermInfoID& arg : instr.args){
			sema_args.emplace_back(this->get_term_info(arg).getExpr());
		}

		for(size_t i = sema_args.size(); i < func_call_impl_res.value().selected_func->params.size(); i+=1){
			sema_args.emplace_back(*func_call_impl_res.value().selected_func->params[i].defaultValue);
		}

		const sema::FuncCall::ID sema_func_call_id = this->context.sema_buffer.createFuncCall(
			*func_call_impl_res.value().selected_func_id, std::move(sema_args)
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
				TermInfo(
					TermInfo::ValueCategory::EPHEMERAL,
					value_stage,
					selected_func_type_return_params[0].typeID.asTypeID(),
					sema::Expr(sema_func_call_id)
				)
			);
			
		}else{ // multi-return
			auto return_types = evo::SmallVector<TypeInfo::ID>();
			return_types.reserve(selected_func_type_return_params.size());
			for(const BaseType::Function::ReturnParam& return_param : selected_func_type_return_params){
				return_types.emplace_back(return_param.typeID.asTypeID());
			}

			this->return_term_info(instr.output,
				TermInfo(
					TermInfo::ValueCategory::EPHEMERAL,
					value_stage,
					std::move(return_types),
					sema::Expr(sema_func_call_id)
				)
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


			const SymbolProc::WaitOnResult wait_on_result = func_call_impl_res.value().selected_func
				->symbolProc.waitOnPIRDefIfNeeded(
					this->symbol_proc_id, this->context, func_call_impl_res.value().selected_func->symbolProcID
				);

			switch(wait_on_result){
				case SymbolProc::WaitOnResult::NOT_NEEDED:
					break;

				case SymbolProc::WaitOnResult::WAITING:
					return Result::NEED_TO_WAIT_BEFORE_NEXT_INSTR;

				case SymbolProc::WaitOnResult::WAS_ERRORED:
					return Result::ERROR;

				case SymbolProc::WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND:
					evo::debugFatalBreak("Shouldn't be possible");

				case SymbolProc::WaitOnResult::CIRCULAR_DEP_DETECTED:
					evo::debugFatalBreak("Shouldn't be possible");
			}

			return Result::SUCCESS;

		}else{
			if(this->symbol_proc.extra_info.is<SymbolProc::FuncInfo>()){
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



	auto SemanticAnalyzer::instr_constexpr_func_call_run(const Instruction::ConstexprFuncCallRun& instr) -> Result {
		const TermInfo& func_call_term = this->get_term_info(instr.target);

		const sema::FuncCall& sema_func_call =
			this->context.getSemaBuffer().getFuncCall(func_call_term.getExpr().funcCallID());

		const sema::Func& target_func = 
			this->context.getSemaBuffer().getFunc(sema_func_call.target.as<sema::Func::ID>()); 

		const BaseType::Function& target_func_type = this->context.getTypeManager().getFunction(target_func.typeID);

		evo::debugAssert(target_func_type.returnsVoid() == false, "Constexpr function call expr cannot return void");
		evo::debugAssert(target_func.defCompleted.load(), "def of func not completed");

		auto jit_args = evo::SmallVector<core::GenericValue>();
		jit_args.reserve(instr.args.size() && size_t(target_func_type.hasNamedReturns()));
		for(size_t i = 0; const SymbolProc::TermInfoID& arg_id : instr.args){
			const TermInfo& arg = this->get_term_info(arg_id);

			switch(arg.getExpr().kind()){
				case sema::Expr::Kind::INT_VALUE: {
					jit_args.emplace_back(
						evo::copy(this->context.getSemaBuffer().getIntValue(arg.getExpr().intValueID()).value)
					);
				} break;

				case sema::Expr::Kind::FLOAT_VALUE: {
					jit_args.emplace_back(
						evo::copy(this->context.getSemaBuffer().getFloatValue(arg.getExpr().floatValueID()).value)
					);
				} break;

				case sema::Expr::Kind::BOOL_VALUE: {
					jit_args.emplace_back(
						evo::copy(this->context.getSemaBuffer().getBoolValue(arg.getExpr().boolValueID()).value)
					);
				} break;

				case sema::Expr::Kind::STRING_VALUE: {
					evo::unimplemented();
				} break;

				case sema::Expr::Kind::AGGREGATE_VALUE: {
					evo::unimplemented();
				} break;

				case sema::Expr::Kind::CHAR_VALUE: {
					jit_args.emplace_back(
						evo::copy(this->context.getSemaBuffer().getCharValue(arg.getExpr().charValueID()).value)
					);
				} break;

				default: evo::debugFatalBreak("Invalid constexpr value");
			}

			i += 1;
		}

		if(target_func_type.hasNamedReturns()){
			jit_args.emplace_back();
		}

		// {
		// 	auto printer = core::Printer::createConsole();
		// 	pir::printModule(this->context.constexpr_pir_module, printer);
		// }

		core::GenericValue run_result = this->context.constexpr_jit_engine.runFunc(
			this->context.constexpr_pir_module, *target_func.constexprJITInterfaceFunc, jit_args
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

			if(target_func_type.hasNamedErrorReturns()){
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


			const sema::Expr return_sema_expr = this->genericValueToSemaExpr(run_result, target_func_return_type);

			this->return_term_info(instr.output,
				TermInfo(
					TermInfo::ValueCategory::EPHEMERAL,
					TermInfo::ValueStage::CONSTEXPR,
					func_call_term.type_id,
					return_sema_expr
				)
			);
			return Result::SUCCESS;
		}

	}



	auto SemanticAnalyzer::instr_import(const Instruction::Import& instr) -> Result {
		const TermInfo& location = this->get_term_info(instr.location);

		// TODO(FUTURE): type checking of location

		const std::string_view lookup_path = this->context.getSemaBuffer().getStringValue(
			location.getExpr().stringValueID()
		).value;

		const evo::Expected<Source::ID, Context::LookupSourceIDError> import_lookup = 
			this->context.lookupSourceID(lookup_path, this->source);

		if(import_lookup.has_value()){
			this->return_term_info(instr.output, 
				TermInfo(
					TermInfo::ValueCategory::MODULE,
					TermInfo::ValueStage::CONSTEXPR,
					import_lookup.value(),
					std::nullopt
				)
			);
			return Result::SUCCESS;
		}

		switch(import_lookup.error()){
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
		}

		evo::unreachable();
	}


	template<bool IS_CONSTEXPR>
	auto SemanticAnalyzer::instr_template_intrinsic_func_call(
		const Instruction::TemplateIntrinsicFuncCall<IS_CONSTEXPR>& instr
	) -> Result {
		const TermInfo& target_term_info = this->get_term_info(instr.target);

		const evo::Result<FuncCallImplData> selected_func = this->func_call_impl<IS_CONSTEXPR, false>(
			instr.func_call,
			target_term_info,
			instr.args,
			instr.template_args
		);
		if(selected_func.isError()){ return Result::ERROR; }


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
						evo::unimplemented("String values");
						// template_args.emplace_back(
						// 	core::GenericValue(
						// 		this->context.sema_buffer.getStringValue(value_expr.stringValueID()).value
						// 	)
						// );
					} break;
					case sema::Expr::Kind::AGGREGATE_VALUE: {
						evo::unimplemented("Aggregate values");
						// template_args.emplace_back(
						// 	core::GenericValue(
						// 		this->context.sema_buffer.getStringValue(value_expr.stringValueID()).value
						// 	)
						// );
					} break;
					case sema::Expr::Kind::CHAR_VALUE: {
						template_args.emplace_back(
							core::GenericValue(
								this->context.sema_buffer.getCharValue(value_expr.charValueID()).value
							)
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
					return_types[0],
					sema::Expr(this->context.sema_buffer.createFuncCall(intrinsic_target, std::move(args)))
				);

			}else{
				this->return_term_info(instr.output,
					TermInfo::ValueCategory::EPHEMERAL,
					TermInfo::ValueStage::CONSTEXPR,
					std::move(return_types),
					sema::Expr(this->context.sema_buffer.createFuncCall(intrinsic_target, std::move(args)))
				);
			}
		};


		auto constexpr_intrinsic_evaluator = ConstexprIntrinsicEvaluator(
			this->context.type_manager, this->context.sema_buffer
		);

		switch(target_term_info.type_id.as<TemplateIntrinsicFunc::Kind>()){
			case TemplateIntrinsicFunc::Kind::SIZE_OF: {
				this->return_term_info(
					instr.output,
					constexpr_intrinsic_evaluator.sizeOf(template_args[0].as<TypeInfo::VoidableID>().asTypeID())
				);
			} break;

			case TemplateIntrinsicFunc::Kind::BIT_WIDTH: {
				this->return_term_info(
					instr.output,
					constexpr_intrinsic_evaluator.bitWidth(template_args[0].as<TypeInfo::VoidableID>().asTypeID())
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
				}(selected_func.value().selected_func_type.returnParams);
			} break;

			case TemplateIntrinsicFunc::Kind::UI_TO_F: {
				if constexpr(IS_CONSTEXPR){
					this->return_term_info(instr.output, constexpr_intrinsic_evaluator.uiToF(
						template_args[1].as<TypeInfo::VoidableID>().asTypeID(),
						this->context.sema_buffer.getIntValue(args[0].intValueID()).value
					));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}(selected_func.value().selected_func_type.returnParams);
			} break;

			case TemplateIntrinsicFunc::Kind::F_TO_I: {
				if constexpr(IS_CONSTEXPR){
					this->return_term_info(instr.output, constexpr_intrinsic_evaluator.fToUI(
						template_args[1].as<TypeInfo::VoidableID>().asTypeID(),
						this->context.sema_buffer.getFloatValue(args[0].floatValueID()).value
					));
				}else{
					create_runtime_call(selected_func.value().selected_func_type.returnParams);
				}(selected_func.value().selected_func_type.returnParams);
			} break;

			case TemplateIntrinsicFunc::Kind::F_TO_UI: {
				if constexpr(IS_CONSTEXPR){
					this->return_term_info(instr.output, constexpr_intrinsic_evaluator.fToUI(
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
						template_args[1].as<core::GenericValue>().as<bool>(),
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
						template_args[1].as<core::GenericValue>().as<bool>(),
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
						template_args[1].as<core::GenericValue>().as<bool>(),
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
						template_args[1].as<core::GenericValue>().as<bool>(),
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
						template_args[2].as<core::GenericValue>().as<bool>(),
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
						template_args[2].as<core::GenericValue>().as<bool>(),
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


		this->return_term_info(instr.output,
			TermInfo::ValueCategory::EPHEMERAL,
			target.value_stage,
			target.type_id,
			sema::Expr(this->context.sema_buffer.createCopy(target.getExpr()))
		);

		return Result::SUCCESS;
	}

	auto SemanticAnalyzer::instr_move(const Instruction::Move& instr) -> Result {
		const TermInfo& target = this->get_term_info(instr.target);

		if(target.value_category != TermInfo::ValueCategory::CONCRETE_MUT){
			if(target.is_concrete() == false){
				this->emit_error(
					Diagnostic::Code::SEMA_MOVE_ARG_NOT_CONCRETE,
					instr.prefix,
					"Argument of operator [move] must be concrete"
				);
			}else{
				this->emit_error(
					Diagnostic::Code::SEMA_MOVE_ARG_NOT_MUTABLE,
					instr.prefix,
					"Argument of operator [move] must be mutable"
				);
			}

			return Result::ERROR;
		}


		this->return_term_info(instr.output,
			TermInfo::ValueCategory::EPHEMERAL,
			target.value_stage,
			target.type_id,
			sema::Expr(this->context.sema_buffer.createMove(target.getExpr()))
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
		resultant_qualifiers.emplace_back(true, is_read_only, false);

		const TypeInfo::ID resultant_type_id = this->context.type_manager.getOrCreateTypeInfo(
			TypeInfo(target_type.baseTypeID(), std::move(resultant_qualifiers))
		);


		this->return_term_info(instr.output,
			TermInfo::ValueCategory::EPHEMERAL,
			target.value_stage,
			resultant_type_id,
			sema::Expr(this->context.sema_buffer.createAddrOf(target.getExpr()))
		);

		return Result::SUCCESS;
	}



	auto SemanticAnalyzer::instr_deref(const Instruction::Deref& instr) -> Result {
		const TermInfo& target = this->get_term_info(instr.target);

		if(target.type_id.is<TypeInfo::ID>() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_DEREF_ARG_NOT_PTR,
				instr.postfix,
				"Argument of operator postfix [.*] must be a pointer"
			);
			return Result::ERROR;
		}

		const TypeInfo& target_type = this->context.getTypeManager().getTypeInfo(target.type_id.as<TypeInfo::ID>());

		if(target_type.isPointer() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_DEREF_ARG_NOT_PTR,
				instr.postfix,
				"Argument of operator postfix [.*] must be a pointer"
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


		using ValueCategory = TermInfo::ValueCategory;

		this->return_term_info(instr.output,
			target_type.qualifiers().back().isReadOnly ? ValueCategory::CONCRETE_CONST : ValueCategory::CONCRETE_MUT,
			target.value_stage,
			resultant_type_id,
			sema::Expr(this->context.sema_buffer.createDeref(target.getExpr(), resultant_type_id))
		);

		return Result::SUCCESS;
	}

	template<bool IS_CONSTEXPR>
	auto SemanticAnalyzer::instr_struct_init_new(const Instruction::StructInitNew<IS_CONSTEXPR>& instr) -> Result {
		const TypeInfo::VoidableID target_type_id = this->get_type(instr.type_id);
		if(target_type_id.isVoid()){
			this->emit_error(
				Diagnostic::Code::SEMA_NEW_TYPE_VOID,
				instr.struct_init_new.type,
				"Operator [new] cannot accept type [Void]"
			);
			return Result::ERROR;
		}

		const TypeInfo& target_type_info = this->context.getTypeManager().getTypeInfo(target_type_id.asTypeID());
		if(
			target_type_info.qualifiers().empty() == false
			|| target_type_info.baseTypeID().kind() != BaseType::Kind::STRUCT
		){
			this->emit_error(
				Diagnostic::Code::SEMA_NEW_STRUCT_INIT_NOT_STRUCT,
				instr.struct_init_new.type,
				"Struct initializer operator [new] cannot accept a type that's not a struct"
			);
			return Result::ERROR;
		}

		const BaseType::Struct& target_type = this->context.getTypeManager().getStruct(
			target_type_info.baseTypeID().structID()
		);

		const Source& target_type_source = this->context.getSourceManager()[target_type.sourceID];


		if(target_type.memberVars.empty()){
			if(instr.struct_init_new.memberInits.empty() == false){
				const AST::StructInitNew::MemberInit& member_init = instr.struct_init_new.memberInits[0];

				const std::string_view member_init_ident =
					target_type_source.getTokenBuffer()[member_init.ident].getString();

				this->emit_error(
					Diagnostic::Code::SEMA_NEW_STRUCT_MEMBER_DOESNT_EXIST,
					member_init.ident,
					std::format("This struct has no member \"{}\"", member_init_ident),
					evo::SmallVector<Diagnostic::Info>{
						Diagnostic::Info("Struct is empty"),
						Diagnostic::Info(
							"Struct was declared here:",
							Diagnostic::Location::get(target_type.identTokenID, target_type_source)
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
					if(
						this->scope.inObjectScope() == false
						|| this->scope.getCurrentObjectScope().is<sema::Func::ID>() == false
					){
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
				target_type_id.asTypeID(),
				sema::Expr(created_aggregate_value)
			);
			return Result::SUCCESS;
		}


		const auto struct_has_member = [&](std::string_view ident) -> bool {
			for(const BaseType::Struct::MemberVar& member_var : target_type.memberVars){
				const std::string_view member_var_ident =
					target_type_source.getTokenBuffer()[member_var.identTokenID].getString();
			
				if(member_var_ident == ident){ return true; }
			}

			return false;
		};

		auto values = evo::SmallVector<sema::Expr>();
		values.reserve(target_type.memberVars.size());

		size_t member_init_i = 0;
		for(const BaseType::Struct::MemberVar* member_var : target_type.memberVarsABI){
			const std::string_view member_var_ident =
				target_type_source.getTokenBuffer()[member_var->identTokenID].getString();

			if(member_init_i >= instr.struct_init_new.memberInits.size()){
				if(member_var->defaultValue.has_value()){
					values.emplace_back(*member_var->defaultValue);
					continue;
				}

				if(instr.struct_init_new.memberInits.empty()){
					this->emit_error(
						Diagnostic::Code::SEMA_NEW_STRUCT_MEMBER_NOT_SET,
						instr.struct_init_new,
						std::format("Member \"{}\" was not set in struct initializer operator [new]", member_var_ident)
					);
				}else{
					this->emit_error(
						Diagnostic::Code::SEMA_NEW_STRUCT_MEMBER_NOT_SET,
						instr.struct_init_new,
						std::format("Member \"{}\" was not set in struct initializer operator [new]", member_var_ident),
						Diagnostic::Info(
							std::format("Member initializer for \"{}\" should go after this one", member_var_ident),
							this->get_location(instr.struct_init_new.memberInits[member_init_i - 1].ident)
						)
					);
				}

				return Result::ERROR;

			}else{
				const AST::StructInitNew::MemberInit& member_init =
					instr.struct_init_new.memberInits[member_init_i];


				const std::string_view member_init_ident =
					target_type_source.getTokenBuffer()[member_init.ident].getString();

				if(member_var_ident != member_init_ident){
					if(member_var->defaultValue.has_value()){
						values.emplace_back(*member_var->defaultValue);
						continue;
					}

					if(struct_has_member(member_init_ident)){
						this->emit_error(
							Diagnostic::Code::SEMA_NEW_STRUCT_MEMBER_NOT_SET,
							instr.struct_init_new,
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
								Diagnostic::Location::get(target_type.identTokenID, target_type_source)
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

				if(this->type_check<true>(
					member_var->typeID, member_init_expr, "Member initializer", member_init.expr
				).ok == false){
					return Result::ERROR;
				}

				values.emplace_back(member_init_expr.getExpr());

				member_init_i += 1;
			}
		}


		if(member_init_i < instr.struct_init_new.memberInits.size()){
			const AST::StructInitNew::MemberInit& member_init = instr.struct_init_new.memberInits[member_init_i];

			const std::string_view member_init_ident =
				target_type_source.getTokenBuffer()[member_init.ident].getString();

			this->emit_error(
				Diagnostic::Code::SEMA_NEW_STRUCT_MEMBER_DOESNT_EXIST,
				member_init.ident,
				std::format("This struct has no member \"{}\"", member_init_ident),
				Diagnostic::Info(
					"Struct was declared here:", Diagnostic::Location::get(target_type.identTokenID, target_type_source)
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
				if(
					this->scope.inObjectScope() == false
					|| this->scope.getCurrentObjectScope().is<sema::Func::ID>() == false
				){
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
			target_type_id.asTypeID(),
			sema::Expr(created_aggregate_value)
		);
		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_prepare_try_handler(const Instruction::PrepareTryHandler& instr) -> Result {
		this->push_scope_level();

		const SemaBuffer& sema_buffer = this->context.getSemaBuffer();

		const TermInfo& attempt_expr = this->get_term_info(instr.attempt_expr);
		const sema::FuncCall& attempt_func_call = sema_buffer.getFuncCall(attempt_expr.getExpr().funcCallID());
		const BaseType::Function& attempt_func_type = attempt_func_call.target.visit(
			[&](const auto& target) -> const BaseType::Function& {
			using Target = std::decay_t<decltype(target)>;

			if constexpr(std::is_same<Target, sema::Func::ID>()){
				return this->context.getTypeManager().getFunction(sema_buffer.getFunc(target).typeID);
				
			}else if constexpr(std::is_same<Target, IntrinsicFunc::Kind>()){
				const TypeInfo::ID type_info_id = this->context.getIntrinsicFuncInfo(target).typeID;
				const TypeInfo& type_info = this->context.getTypeManager().getTypeInfo(type_info_id);
				return this->context.getTypeManager().getFunction(type_info.baseTypeID().funcID());
				
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

				return this->context.getTypeManager().getFunction(
					this->context.type_manager.getOrCreateFunction(
						template_intrinsic_func_info.getTypeInstantiation(instantiation_args)
					).funcID()
				);
				
			}else{
				static_assert(false, "Unsupported func call target");
			}
		});


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
		}

		this->return_term_info(instr.output_except_params,
			TermInfo::ValueCategory::EXCEPT_PARAM_PACK,
			this->get_current_func().isConstexpr ? TermInfo::ValueStage::COMPTIME : TermInfo::ValueStage::RUNTIME,
			TermInfo::ExceptParamPack{},
			std::move(except_params)
		);
		return Result::SUCCESS;
	}


	auto SemanticAnalyzer::instr_try_else(const Instruction::TryElse& instr) -> Result {
		const TermInfo& attempt_expr = this->get_term_info(instr.attempt_expr);
		TermInfo& except_expr = this->get_term_info(instr.except_expr);

		EVO_DEFER([&](){ this->pop_scope_level(); });

		if(attempt_expr.value_category != TermInfo::ValueCategory::EPHEMERAL){
			this->emit_error(
				Diagnostic::Code::SEMA_TRY_ELSE_ATTEMPT_NOT_FUNC_CALL,
				instr.try_else.attemptExpr,
				"Attempt in try/else expression is not function call"
			);
			return Result::ERROR;
		}

		if(attempt_expr.getExpr().kind() != sema::Expr::Kind::FUNC_CALL){
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

		if(this->type_check<true>(
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

		this->return_term_info(instr.output,
			TermInfo::ValueCategory::EPHEMERAL,
			value_stage,
			attempt_expr.type_id,
			sema::Expr(
				this->context.sema_buffer.createTryElse(
					attempt_expr.getExpr(), except_expr.getExpr(), std::move(except_params)
				)
			)
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
					"Block expression output cannot be type [Void]"
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
				std::move(types),
				sema::Expr(sema_block_expr_id)
			);
		}


		this->pop_scope_level<true>();
		this->get_current_scope_level().resetSubScopes();
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
					if(struct_template.params[i].isExpr()){
						const ASTBuffer& ast_buffer = this->source.getASTBuffer();
						const AST::StructDecl& ast_struct =
							ast_buffer.getStructDecl(sema_templated_struct.symbolProc.ast_node);
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

					instantiation_lookup_args.emplace_back(arg_term_info.type_id.as<TypeInfo::VoidableID>());
					instantiation_args.emplace_back(arg_term_info.type_id.as<TypeInfo::VoidableID>());
					continue;
				}

				if(struct_template.params[i].isType()){
					const ASTBuffer& ast_buffer = this->source.getASTBuffer();
					const AST::StructDecl& ast_struct =
						ast_buffer.getStructDecl(sema_templated_struct.symbolProc.ast_node);
					const AST::TemplatePack& ast_template_pack = ast_buffer.getTemplatePack(*ast_struct.templatePack);

					this->emit_error(
						Diagnostic::Code::SEMA_TEMPLATE_INVALID_ARG,
						instr.templated_expr.args[i],
						"Expected a type template argument, got an expression",
						Diagnostic::Info(
							"Parameter declared here:", this->get_location(ast_template_pack.params[i].ident)
						)
					);
					return Result::ERROR;
				}



				const evo::Result<TypeInfo::ID> expr_type_id = [&]() -> evo::Result<TypeInfo::ID> {
					if(struct_template.params[i].typeID->isTemplateDeclInstantiation()){
						const AST::StructDecl& ast_struct =
							this->source.getASTBuffer().getStructDecl(sema_templated_struct.symbolProc.ast_node);
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
								"Template parameter cannot be type [Void]"
							);
							return evo::resultError;
						}

						return resolved_type.value().asTypeID();
					}else{
						return *struct_template.params[i].typeID;
					}
				}();
				if(expr_type_id.isError()){ return Result::ERROR; }
				
			
				if(this->type_check<true>(
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
						evo::debugFatalBreak(
							"String value template args are not supported yet (getting here should be impossible)"
						);
					} break;

					case sema::Expr::Kind::AGGREGATE_VALUE: {
						evo::debugFatalBreak(
							"Aggregate value template args are not supported yet (getting here should be impossible)"
						);
					} break;

					case sema::Expr::Kind::CHAR_VALUE: {
						instantiation_lookup_args.emplace_back(
							core::GenericValue(evo::copy(sema_buffer.getCharValue(arg_expr.charValueID()).value))
						);
					} break;

					default: evo::debugFatalBreak("Invalid template argument value");
				}
				
			}else{
				const ASTBuffer& ast_buffer = this->source.getASTBuffer();
				const AST::StructDecl& ast_struct = ast_buffer.getStructDecl(sema_templated_struct.symbolProc.ast_node);
				const AST::TemplatePack& ast_template_pack = ast_buffer.getTemplatePack(*ast_struct.templatePack);

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
				const TypeInfo::VoidableID type_id = this->get_type(arg.as<SymbolProc::TypeID>());
				instantiation_lookup_args.emplace_back(type_id);
				instantiation_args.emplace_back(type_id);

				this->scope.addTemplateDeclInstantiationType(
					this->source.getTokenBuffer()[ast_template_pack.params[i].ident].getString(), type_id
				);
			}
		}


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
							evo::debugFatalBreak(
								"String value template args are not supported yet (getting here should be impossible)"
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
									evo::copy(sema_buffer.getCharValue(default_value.charValueID()).value)
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



		///////////////////////////////////
		// lookup / create instantiation

		const BaseType::StructTemplate::InstantiationInfo instantiation_info =
			struct_template.lookupInstantiation(std::move(instantiation_lookup_args));

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
				*instantiation_info.instantiationID
			);
			if(instantiation_symbol_proc_id.isError()){ return Result::ERROR; }

			instantiation_info.instantiation.symbolProcID = instantiation_symbol_proc_id.value();


			///////////////////////////////////
			// add instantiation args to scope

			sema::ScopeManager::Scope& instantiation_sema_scope = scope_manager.getScope(instantiation_sema_scope_id);

			instantiation_sema_scope.pushLevel(scope_manager.createLevel());

			const AST::StructDecl& struct_template_decl = 
				this->source.getASTBuffer().getStructDecl(sema_templated_struct.symbolProc.ast_node);

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
							arg.as<TypeInfo::VoidableID>(),
							ast_template_pack.params[i].ident,
							sema::ScopeLevel::TemplateTypeParamFlag{}
						);

					}else{
						const TypeInfo::ID expr_type_id = [&]() -> TypeInfo::ID {
							if(struct_template.params[i].typeID->isTemplateDeclInstantiation()){
								const AST::StructDecl& ast_struct = this->source.getASTBuffer().getStructDecl(
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
							expr_type_id,
							arg.as<sema::Expr>(),
							ast_template_pack.params[i].ident	
						);
					}
				}();

				if(add_ident_result.isError()){ return Result::ERROR; }
			}

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



	auto SemanticAnalyzer::instr_templated_term_wait(const Instruction::TemplatedTermWait& instr) -> Result {
		const BaseType::StructTemplate::Instantiation& instantiation =
			this->get_struct_instantiation(instr.instantiation);

		if(instantiation.errored.load()){ return Result::ERROR; }
		// if(instantiation.structID.load().has_value() == false){ return Result::NEED_TO_WAITOnInstantiation; }
		evo::debugAssert(instantiation.structID.has_value(), "Should already be completed");

		this->return_term_info(instr.output,
			TermInfo::ValueCategory::TYPE,
			TermInfo::ValueStage::CONSTEXPR,
			TypeInfo::VoidableID(
				this->context.type_manager.getOrCreateTypeInfo(TypeInfo(BaseType::ID(*instantiation.structID)))
			),
			std::nullopt
		);

		return Result::SUCCESS;
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





	template<bool NEEDS_DEF>
	auto SemanticAnalyzer::instr_expr_accessor(const Instruction::Accessor<NEEDS_DEF>& instr) -> Result {
		const std::string_view rhs_ident_str = this->source.getTokenBuffer()[instr.rhs_ident].getString();
		const TermInfo& lhs = this->get_term_info(instr.lhs);

		if(lhs.type_id.is<Source::ID>()){
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


		}else if(lhs.type_id.is<TypeInfo::VoidableID>()){
			if(lhs.type_id.as<TypeInfo::VoidableID>().isVoid()){
				this->emit_error(
					Diagnostic::Code::SEMA_INVALID_ACCESSOR_RHS,
					instr.infix.lhs,
					"Accessor operator of type [Void] is invalid"
				);
				return Result::ERROR;
			}

			const TypeInfo::ID actual_lhs_type_id = this->get_actual_type<true>(
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

			if(actual_lhs_type.baseTypeID().kind() != BaseType::Kind::STRUCT){
				// TODO(FUTURE): better message
				this->emit_error(
					Diagnostic::Code::SEMA_INVALID_ACCESSOR_RHS,
					instr.infix.lhs,
					"Accessor operator of this LHS is unsupported"
				);
				return Result::ERROR;
			}


			const BaseType::Struct& lhs_struct = this->context.getTypeManager().getStruct(
				actual_lhs_type.baseTypeID().structID()
			);

			const Source& struct_source = this->context.getSourceManager()[lhs_struct.sourceID];

			const WaitOnSymbolProcResult wait_on_symbol_proc_result = this->wait_on_symbol_proc<NEEDS_DEF>(
				&lhs_struct.namespacedMembers, rhs_ident_str
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


			const evo::Expected<TermInfo, AnalyzeExprIdentInScopeLevelError> expr_ident = 
				this->analyze_expr_ident_in_scope_level<NEEDS_DEF, false>(
					instr.rhs_ident, rhs_ident_str, *lhs_struct.scopeLevel, true, true, &struct_source
				);

			if(expr_ident.has_value()){
				this->return_term_info(instr.output, std::move(expr_ident.value()));
				return Result::SUCCESS;
			}

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

			evo::unreachable();
		}

		if(lhs.type_id.is<TypeInfo::ID>() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_INVALID_ACCESSOR_RHS,
				instr.infix.lhs,
				"Accessor operator of this LHS is invalid"
			);
			return Result::ERROR;
		}


		const TypeInfo::ID actual_lhs_type_id = this->get_actual_type<true>(lhs.type_id.as<TypeInfo::ID>());
		const TypeInfo& actual_lhs_type = this->context.getTypeManager().getTypeInfo(actual_lhs_type_id);

		if(actual_lhs_type.qualifiers().empty() == false){
			this->emit_error(
				Diagnostic::Code::SEMA_INVALID_ACCESSOR_RHS,
				instr.infix.lhs,
				"Accessor operator of this LHS is unimplemented"
			);
			return Result::ERROR;
		}

		if(actual_lhs_type.baseTypeID().kind() != BaseType::Kind::STRUCT){
			this->emit_error(
				Diagnostic::Code::SEMA_INVALID_ACCESSOR_RHS,
				instr.infix.lhs,
				"Accessor operator of this LHS is unimplemented"
			);
			return Result::ERROR;
		}


		const BaseType::Struct& lhs_type_struct = this->context.getTypeManager().getStruct(
			actual_lhs_type.baseTypeID().structID()
		);

		const Source& struct_source = this->context.getSourceManager()[lhs_type_struct.sourceID];

		const auto lock = std::scoped_lock(lhs_type_struct.memberVarsLock);
		for(size_t i = 0; const BaseType::Struct::MemberVar* member_var : lhs_type_struct.memberVarsABI){
			const std::string_view member_ident_str = 
				struct_source.getTokenBuffer()[member_var->identTokenID].getString();

			if(member_ident_str == rhs_ident_str){
				const TermInfo::ValueCategory value_category = [&](){
					if(lhs.is_ephemeral()){ return lhs.value_category; }

					if(lhs.value_category == TermInfo::ValueCategory::CONCRETE_CONST){
						return TermInfo::ValueCategory::CONCRETE_CONST;
					}

					if(member_var->kind == AST::VarDecl::Kind::CONST){
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
						member_var->typeID,
						lhs_aggregate_value.values[i]
					);
					
				}else{
					this->return_term_info(instr.output,
						value_category,
						this->get_current_func().isConstexpr ? ValueStage::COMPTIME : ValueStage::RUNTIME,
						member_var->typeID,
						sema::Expr(
							this->context.sema_buffer.createAccessor(lhs.getExpr(), actual_lhs_type_id, uint32_t(i))
						)
					);
				}

				return Result::SUCCESS;
			}

			i += 1;
		}

		// TODO(FUTURE): better messaging (looking at namespacedMembers for passed by when cond, not a member var)
		this->emit_error(
			Diagnostic::Code::SEMA_NO_SYMBOL_IN_SCOPE_WITH_THAT_IDENT,
			instr.rhs_ident,
			std::format("Struct doesn't have a member variable named \"{}\"", rhs_ident_str)
		);
		return Result::ERROR;
	}


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
				this->emit_error(
					Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
					instr.ast_type,
					"Type [This] is unimplemented"
				);
				return Result::ERROR;
			} break;

			case Token::Kind::TYPE_INT:         case Token::Kind::TYPE_ISIZE:        case Token::Kind::TYPE_UINT:
			case Token::Kind::TYPE_USIZE:       case Token::Kind::TYPE_F16:          case Token::Kind::TYPE_BF16:
			case Token::Kind::TYPE_F32:         case Token::Kind::TYPE_F64:          case Token::Kind::TYPE_F80:
			case Token::Kind::TYPE_F128:        case Token::Kind::TYPE_BYTE:         case Token::Kind::TYPE_BOOL:
			case Token::Kind::TYPE_CHAR:        case Token::Kind::TYPE_RAWPTR:       case Token::Kind::TYPE_TYPEID:
			case Token::Kind::TYPE_C_SHORT:     case Token::Kind::TYPE_C_USHORT:     case Token::Kind::TYPE_C_INT:
			case Token::Kind::TYPE_C_UINT:      case Token::Kind::TYPE_C_LONG:       case Token::Kind::TYPE_C_ULONG:
			case Token::Kind::TYPE_C_LONG_LONG: case Token::Kind::TYPE_C_ULONG_LONG:
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


	auto SemanticAnalyzer::instr_user_type(const Instruction::UserType& instr) -> Result {
		auto base_type_id = std::optional<TypeInfo::ID>();
		const TermInfo& term_info = this->get_term_info(instr.base_type);
		switch(term_info.value_category){
			case TermInfo::ValueCategory::TYPE: {
				evo::debugAssert(
					this->get_term_info(instr.base_type).type_id.as<TypeInfo::VoidableID>().isVoid() == false,
					"[Void] is not a user-type"
				);
				base_type_id = term_info.type_id.as<TypeInfo::VoidableID>().asTypeID();
			} break;

			case TermInfo::ValueCategory::TEMPLATE_TYPE: {
				const sema::TemplatedStruct& sema_templated_struct = this->context.sema_buffer.getTemplatedStruct(
					term_info.type_id.as<sema::TemplatedStruct::ID>()
				);

				base_type_id = this->context.type_manager.getOrCreateTypeInfo(
					TypeInfo(BaseType::ID(sema_templated_struct.templateID))
				);
			} break;

			case TermInfo::ValueCategory::TEMPLATE_DECL_INSTANTIATION_TYPE: {
				this->return_type(
					instr.output,
					TypeInfo::VoidableID(TypeInfo::ID::createTemplateDeclInstantiation())
				);
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
				intrinsic_type,
				sema::Expr(*intrinsic_kind)
			);
			return Result::SUCCESS;
		}

		const std::optional<TemplateIntrinsicFunc::Kind> template_intrinsic_kind = 
			TemplateIntrinsicFunc::lookupKind(intrinsic_name);
		if(template_intrinsic_kind.has_value()){
			this->return_term_info(instr.output,
				TermInfo::ValueCategory::TEMPLATE_INTRINSIC_FUNC,
				TermInfo::ValueStage::CONSTEXPR,
				*template_intrinsic_kind,
				std::nullopt
			);
			return Result::SUCCESS;
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
					this->context.getTypeManager().getTypeBool(),
					sema::Expr(this->context.sema_buffer.createBoolValue(literal_token.getBool()))
				);
				return Result::SUCCESS;
			} break;

			case Token::Kind::LITERAL_STRING: {
				this->return_term_info(instr.output,
					TermInfo::ValueCategory::EPHEMERAL,
					TermInfo::ValueStage::CONSTEXPR,
					this->context.type_manager.getOrCreateTypeInfo(
						TypeInfo(
							this->context.type_manager.getOrCreateArray(
								BaseType::Array(
									this->context.getTypeManager().getTypeChar(),
									evo::SmallVector<uint64_t>{literal_token.getString().size()},
									core::GenericValue('\0')
								)
							)
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
					this->context.getTypeManager().getTypeChar(),
					sema::Expr(this->context.sema_buffer.createCharValue(literal_token.getChar()))
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
			TermInfo::InitializerType(),
			sema::Expr(this->context.sema_buffer.createUninit(instr.uninit_token))
		);
		return Result::SUCCESS;
	}

	auto SemanticAnalyzer::instr_zeroinit(const Instruction::Zeroinit& instr) -> Result {
		this->return_term_info(instr.output,
			TermInfo::ValueCategory::INITIALIZER,
			TermInfo::ValueStage::CONSTEXPR,
			TermInfo::InitializerType(),
			sema::Expr(this->context.sema_buffer.createZeroinit(instr.zeroinit_token))
		);
		return Result::SUCCESS;
	}

	auto SemanticAnalyzer::instr_type_deducer(const Instruction::TypeDeducer& instr) -> Result {
		const BaseType::ID new_type_deducer = this->context.type_manager.getOrCreateTypeDeducer(
			BaseType::TypeDeducer(instr.type_deducer_token, this->source.getID())
		);

		this->return_term_info(instr.output,
			TermInfo::ValueCategory::TYPE,
			TermInfo::ValueStage::CONSTEXPR,
			TypeInfo::VoidableID(this->context.type_manager.getOrCreateTypeInfo(TypeInfo(new_type_deducer))),
			std::nullopt
		);
		return Result::SUCCESS;
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


	template<bool IS_LABEL_TERMINATE>
	auto SemanticAnalyzer::pop_scope_level() -> void {
		sema::ScopeLevel& current_scope_level = this->get_current_scope_level();
		const bool current_scope_is_terminated = current_scope_level.isTerminated();

		if(
			current_scope_level.hasStmtBlock()
			&& current_scope_level.stmtBlock().isTerminated() == false
			&& current_scope_level.isTerminated()
		){
			current_scope_level.stmtBlock().setTerminated();
		}

		if constexpr(IS_LABEL_TERMINATE){
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
		}else{
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



	auto SemanticAnalyzer::get_current_func() -> sema::Func& {
		return this->context.sema_buffer.funcs[this->scope.getCurrentObjectScope().as<sema::Func::ID>()];
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
			if(scope_level_lookup.error() == AnalyzeExprIdentInScopeLevelError::ERROR_EMITTED){
				return evo::Unexpected(Result::ERROR);
			}
			if(scope_level_lookup.error() == AnalyzeExprIdentInScopeLevelError::NEEDS_TO_WAIT_ON_DEF){ break; }

			i -= 1;
		}


		///////////////////////////////////
		// look in template decl instantiation types

		const evo::Result<std::optional<TypeInfo::VoidableID>> template_decl_instantiation = 
			this->scope.lookupTemplateDeclInstantiationType(ident_str);
		if(template_decl_instantiation.isSuccess()){
			if(template_decl_instantiation.value().has_value()){
				return TermInfo(
					TermInfo::ValueCategory::TYPE,
					TermInfo::ValueStage::CONSTEXPR,
					template_decl_instantiation.value().value(),
					std::nullopt
				);
			}else{
				return TermInfo(
					TermInfo::ValueCategory::TEMPLATE_DECL_INSTANTIATION_TYPE,
					TermInfo::ValueStage::CONSTEXPR,
					TermInfo::TemplateDeclInstantiationType(),
					std::nullopt
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
				return ReturnType(
					TermInfo(TermInfo::ValueCategory::FUNCTION, TermInfo::ValueStage::CONSTEXPR, ident_id, std::nullopt)
				);

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

				switch(sema_var.kind){
					case AST::VarDecl::Kind::VAR: {
						return ReturnType(TermInfo(
							ValueCategory::CONCRETE_MUT,
							this->get_current_func().isConstexpr ? ValueStage::COMPTIME : ValueStage::RUNTIME,
							*sema_var.typeID,
							sema::Expr(ident_id)
						));
					} break;

					case AST::VarDecl::Kind::CONST: {
						return ReturnType(TermInfo(
							ValueCategory::CONCRETE_CONST,
							this->get_current_func().isConstexpr ? ValueStage::COMPTIME : ValueStage::RUNTIME,
							*sema_var.typeID,
							sema::Expr(ident_id)
						));
					} break;

					case AST::VarDecl::Kind::DEF: {
						if(sema_var.typeID.has_value()){
							return ReturnType(TermInfo(
								ValueCategory::EPHEMERAL, ValueStage::CONSTEXPR, *sema_var.typeID, sema_var.expr
							));
						}else{
							return ReturnType(TermInfo(
								ValueCategory::EPHEMERAL_FLUID,
								ValueStage::CONSTEXPR,
								TermInfo::FluidType{},
								sema_var.expr
							));
						}
					} break;
				}

				evo::debugFatalBreak("Unknown or unsupported AST::VarDecl::Kind");

			}else if constexpr(std::is_same<IdentIDType, sema::GlobalVar::ID>()){
				const sema::GlobalVar& sema_var = this->context.getSemaBuffer().getGlobalVar(ident_id);

				if constexpr(PUB_REQUIRED){
					if(sema_var.isPub == false){
						this->emit_error(
							Diagnostic::Code::SEMA_SYMBOL_NOT_PUB,
							ident,
							std::format("Variable \"{}\" does not have the #pub attribute", ident_str),
							Diagnostic::Info(
								"Variable defined here:", 
								Diagnostic::Location::get(ident_id, *source_module, this->context)
							)
						);
						return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::ERROR_EMITTED));
					}

				}

				using ValueCategory = TermInfo::ValueCategory;
				using ValueStage = TermInfo::ValueStage;

				switch(sema_var.kind){
					case AST::VarDecl::Kind::VAR: {
						if constexpr(NEEDS_DEF){
							if(sema_var.expr.load().has_value() == false){
								return ReturnType(
									evo::Unexpected(AnalyzeExprIdentInScopeLevelError::NEEDS_TO_WAIT_ON_DEF)
								);
							}
						}

						const ValueStage value_stage = [&](){
							if(is_global_scope){ return ValueStage::RUNTIME; }

							if(this->scope.getCurrentObjectScope().is<sema::Func::ID>() == false){
								return ValueStage::RUNTIME;
							}
							
							if(this->get_current_func().isConstexpr){
								return ValueStage::COMPTIME;
							}else{
								return ValueStage::RUNTIME;
							}
						}();
						
						return ReturnType(TermInfo(
							ValueCategory::CONCRETE_MUT, value_stage, *sema_var.typeID, sema::Expr(ident_id)
						));
					} break;

					case AST::VarDecl::Kind::CONST: {
						if constexpr(NEEDS_DEF){
							if(sema_var.expr.load().has_value() == false){
								return ReturnType(
									evo::Unexpected(AnalyzeExprIdentInScopeLevelError::NEEDS_TO_WAIT_ON_DEF)
								);
							}
						}

						const ValueStage value_stage = [&](){
							if(is_global_scope){ return ValueStage::COMPTIME; }

							if(this->scope.getCurrentObjectScope().is<sema::Func::ID>() == false){
								return ValueStage::COMPTIME;
							}

							if(this->get_current_func().isConstexpr){
								return ValueStage::COMPTIME;
							}else{
								return ValueStage::RUNTIME;
							}
						}();

						if(this->symbol_proc.extra_info.is<SymbolProc::FuncInfo>()){
							this->symbol_proc.extra_info.as<SymbolProc::FuncInfo>().dependent_vars.emplace(ident_id);
						}

						return ReturnType(TermInfo(
							ValueCategory::CONCRETE_CONST, value_stage, *sema_var.typeID, sema::Expr(ident_id)
						));
					} break;

					case AST::VarDecl::Kind::DEF: {
						if(sema_var.typeID.has_value()){
							return ReturnType(TermInfo(
								ValueCategory::EPHEMERAL, ValueStage::CONSTEXPR, *sema_var.typeID, *sema_var.expr.load()
							));
						}else{
							return ReturnType(TermInfo(
								ValueCategory::EPHEMERAL_FLUID,
								ValueStage::CONSTEXPR,
								TermInfo::FluidType{},
								*sema_var.expr.load()
							));
						}
					};
				}

				evo::debugFatalBreak("Unknown or unsupported AST::VarDecl::Kind");

			}else if constexpr(std::is_same<IdentIDType, sema::ParamID>()){
				const sema::Func& current_func = this->get_current_func();
				const BaseType::Function& current_func_type = 
					this->context.getTypeManager().getFunction(current_func.typeID);
				const BaseType::Function::Param& param = current_func_type.params[
					this->context.getSemaBuffer().getParam(ident_id).index
				];

				const TermInfo::ValueCategory value_category = [&](){
					switch(param.kind){
						case AST::FuncDecl::Param::Kind::READ: return TermInfo::ValueCategory::CONCRETE_CONST;
						case AST::FuncDecl::Param::Kind::MUT:  return TermInfo::ValueCategory::CONCRETE_MUT;
						case AST::FuncDecl::Param::Kind::IN:   return TermInfo::ValueCategory::CONCRETE_FORWARDABLE;
					}

					evo::unreachable();
				}();

				return ReturnType(
					TermInfo(
						value_category,
						current_func.isConstexpr ? TermInfo::ValueStage::COMPTIME : TermInfo::ValueStage::RUNTIME,
						param.typeID,
						sema::Expr(ident_id)
					)
				);

			}else if constexpr(std::is_same<IdentIDType, sema::ReturnParamID>()){
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
						return_param.typeID.asTypeID(),
						sema::Expr(ident_id)
					)
				);

			}else if constexpr(std::is_same<IdentIDType, sema::ErrorReturnParamID>()){
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
						ident_id.sourceID,
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
								Diagnostic::Location::get(ident_id, *source_module, this->context)
							)
						);
						return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::ERROR_EMITTED));
					}
				}

				return ReturnType(
					TermInfo(
						TermInfo::ValueCategory::TYPE,
						TermInfo::ValueStage::CONSTEXPR,
						TypeInfo::VoidableID(
							this->context.type_manager.getOrCreateTypeInfo(TypeInfo(BaseType::ID(ident_id)))
						),
						std::nullopt
					)
				);

			}else if constexpr(std::is_same<IdentIDType, BaseType::Typedef::ID>()){
				this->emit_error(
					Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
					ident,
					"Using typedefs is currently unimplemented"
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
								Diagnostic::Location::get(ident_id, *source_module, this->context)
							)
						);
						return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::ERROR_EMITTED));
					}
				}

				return ReturnType(
					TermInfo(
						TermInfo::ValueCategory::TYPE,
						TermInfo::ValueStage::CONSTEXPR,
						TypeInfo::VoidableID(
							this->context.type_manager.getOrCreateTypeInfo(TypeInfo(BaseType::ID(ident_id)))
						),
						std::nullopt
					)
				);

			}else if constexpr(std::is_same<IdentIDType, sema::TemplatedStruct::ID>()){
				return ReturnType(
					TermInfo(
						TermInfo::ValueCategory::TEMPLATE_TYPE, TermInfo::ValueStage::CONSTEXPR, ident_id, std::nullopt
					)
				);

			}else if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::TemplateTypeParam>()){
				return ReturnType(
					TermInfo(
						TermInfo::ValueCategory::TYPE, TermInfo::ValueStage::CONSTEXPR, ident_id.typeID, std::nullopt
					)
				);

			}else if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::TemplateExprParam>()){
				return ReturnType(
					TermInfo(
						TermInfo::ValueCategory::EPHEMERAL,
						TermInfo::ValueStage::CONSTEXPR,
						ident_id.typeID,
						ident_id.value
					)
				);

			}else if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::DeducedType>()){
				return ReturnType(
					TermInfo(
						TermInfo::ValueCategory::TYPE, TermInfo::ValueStage::CONSTEXPR, ident_id.typeID, std::nullopt
					)
				);

			}else if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::MemberVar>()){
				this->emit_error(
					Diagnostic::Code::SEMA_IDENT_NOT_IN_SCOPE,
					ident,
					std::format("Variable \"{}\" is not accessable in this scope", ident_str),
					Diagnostic::Info(std::format("Did you mean `this.{}`?", ident_str))
				);
				return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::ERROR_EMITTED));

			}else{
				static_assert(false, "Unsupported IdentID");
			}
		});
	}


	template<bool NEEDS_DEF>
	auto SemanticAnalyzer::wait_on_symbol_proc(
		evo::ArrayProxy<const SymbolProc::Namespace*> symbol_proc_namespaces, std::string_view ident_str
	) -> WaitOnSymbolProcResult {
		auto found_range = std::optional<core::IterRange<SymbolProc::Namespace::const_iterator>>();
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
			target.waiting_for.empty() && 
			target.passed_on_by_when_cond == false && 
			target.errored == false && 
			target.isTemplateSubSymbol() == false &&
			target.being_worked_on == false // prevent race condition of target actively adding more to wait on
		){
			this->context.add_task_to_work_manager(target_id);
		}
	}


	template<bool LOOK_THROUGH_TYPEDEF>
	auto SemanticAnalyzer::get_actual_type(TypeInfo::ID type_id) const -> TypeInfo::ID {
		const TypeManager& type_manager = this->context.getTypeManager();

		while(true){
			const TypeInfo& type_info = type_manager.getTypeInfo(type_id);
			if(type_info.qualifiers().empty() == false){ return type_id; }


			if(type_info.baseTypeID().kind() == BaseType::Kind::ALIAS){
				const BaseType::Alias& alias = type_manager.getAlias(type_info.baseTypeID().aliasID());

				evo::debugAssert(alias.aliasedType.load().has_value(), "Definition of alias was not completed");
				type_id = *alias.aliasedType.load();

			}else if(type_info.baseTypeID().kind() == BaseType::Kind::TYPEDEF){
				if constexpr(LOOK_THROUGH_TYPEDEF){
					const BaseType::Typedef& typedef_info = type_manager.getTypedef(type_info.baseTypeID().typedefID());

					evo::debugAssert(
						typedef_info.underlyingType.load().has_value(), "Definition of typedef was not completed"
					);
					type_id = *typedef_info.underlyingType.load();

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
		const auto& call_node
	) -> evo::Result<size_t> {
		evo::debugAssert(func_infos.empty() == false, "need at least 1 function");

		struct OverloadScore{
			using Success = std::monostate;
			struct TooFewArgs{ size_t min_num; size_t got_num; bool accepts_different_nums; };
			struct TooManyArgs{ size_t max_num; size_t got_num; bool accepts_different_nums; };
			struct IntrinsicWrongNumArgs{ size_t required_num; size_t got_num; };
			struct TypeMismatch{ size_t arg_index; };
			struct ValueKindMismatch{ size_t arg_index; };
			struct IncorrectLabel{ size_t arg_index; };
			struct IntrinsicArgWithLabel{ size_t arg_index; };

			using Reason = evo::Variant<
				Success,
				TooFewArgs,
				TooManyArgs,
				IntrinsicWrongNumArgs,
				TypeMismatch,
				ValueKindMismatch,
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

			const sema::Func* sema_func = nullptr;

			if(func_info.func_id.has_value()){ // isn't intrinsic
				sema_func = &this->context.getSemaBuffer().getFunc(*func_info.func_id);

				if(arg_infos.size() < sema_func->minNumArgs){
					scores.emplace_back(OverloadScore::TooFewArgs(
						sema_func->minNumArgs,
						arg_infos.size(),
						sema_func->minNumArgs != func_info.func_type.params.size())
					);
					continue;
				}

				if(arg_infos.size() > func_info.func_type.params.size()){
					scores.emplace_back(OverloadScore::TooManyArgs(
						func_info.func_type.params.size(),
						arg_infos.size(),
						sema_func->minNumArgs != func_info.func_type.params.size())
					);
					continue;
				}
			}else{
				if(arg_infos.size() != func_info.func_type.params.size()){
					scores.emplace_back(
						OverloadScore::IntrinsicWrongNumArgs(func_info.func_type.params.size(), arg_infos.size())
					);
					continue;
				}
			}


			bool arg_checking_failed = false;
			for(size_t arg_i = 0; SelectFuncOverloadArgInfo& arg_info : arg_infos){
				EVO_DEFER([&](){ arg_i += 1; });


				///////////////////////////////////
				// check type mismatch

				const TypeCheckInfo& type_check_info = this->type_check<false>(
					func_info.func_type.params[arg_i].typeID,
					arg_info.term_info,
					"Function call argument",
					arg_info.ast_arg.value
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
					case AST::FuncDecl::Param::Kind::READ: {
						// accepts any value kind
					} break;

					case AST::FuncDecl::Param::Kind::MUT: {
						if(arg_info.term_info.is_const()){
							scores.emplace_back(OverloadScore::ValueKindMismatch(arg_i));
							arg_checking_failed = true;
							break;
						}

						if(arg_info.term_info.is_concrete() == false){
							scores.emplace_back(OverloadScore::ValueKindMismatch(arg_i));
							arg_checking_failed = true;
							break;
						}

						current_score += 1; // add 1 to prefer mut over read
					} break;

					case AST::FuncDecl::Param::Kind::IN: {
						if(arg_info.term_info.is_ephemeral() == false){
							scores.emplace_back(OverloadScore::ValueKindMismatch(arg_i));
							arg_checking_failed = true;
							break;
						}
					} break;
				}


				///////////////////////////////////
				// check label

				if(arg_info.ast_arg.label.has_value()){
					if(sema_func != nullptr){ // isn't intrinsic
						const std::string_view arg_label = 
							this->source.getTokenBuffer()[*arg_info.ast_arg.label].getString();

						const std::string_view param_name = this->context.getSourceManager()[sema_func->sourceID]
							.getTokenBuffer()[sema_func->params[arg_i].ident].getString();

						if(arg_label != param_name){
							scores.emplace_back(OverloadScore::IncorrectLabel(arg_i));
							arg_checking_failed = true;
							break;
						}

					}else{
						scores.emplace_back(OverloadScore::IntrinsicArgWithLabel(arg_i));
						arg_checking_failed = true;
						break;
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
					if(func_infos[i].func_id.has_value()){ return this->get_location(*func_infos[i].func_id); }
					return Diagnostic::Location::NONE;
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
							)
						);

					}else if constexpr(std::is_same<ReasonT, OverloadScore::TypeMismatch>()){
						const TypeInfo::ID expected_type_id = func_infos[i].func_type.params[reason.arg_index].typeID;
						const TermInfo& got_arg = arg_infos[reason.arg_index].term_info;

						infos.emplace_back(
							std::format("Failed to match: argument (index: {}) type mismatch", reason.arg_index),
							get_func_location(),
							evo::SmallVector<Diagnostic::Info>{
								Diagnostic::Info(
									"This argument:", this->get_location(arg_infos[reason.arg_index].ast_arg.value)
								),
								Diagnostic::Info(std::format("Argument type:  {}", this->print_type(got_arg))),
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
						auto sub_infos = evo::SmallVector<Diagnostic::Info>();
						sub_infos.emplace_back(
							"This argument:", this->get_location(arg_infos[reason.arg_index].ast_arg.value)
						);

						switch(func_infos[i].func_type.params[reason.arg_index].kind){
							case AST::FuncDecl::Param::Kind::READ: {
								evo::debugFatalBreak("Read parameters should never fail to accept value kind");
							} break;

							case AST::FuncDecl::Param::Kind::MUT: {
								sub_infos.emplace_back(
									"[mut] parameters can only accept values that are concrete and mutable"
								);
							} break;

							case AST::FuncDecl::Param::Kind::IN: {
								sub_infos.emplace_back("[in] parameters can only accept ephemeral values");
							} break;
						}

						infos.emplace_back(
							std::format("Failed to match: argument (index: {}) value kind mismatch", reason.arg_index),
							get_func_location(),
							std::move(sub_infos)
						);

					}else if constexpr(std::is_same<ReasonT, OverloadScore::IncorrectLabel>()){
						const sema::Func& sema_func = this->context.getSemaBuffer().getFunc(*func_infos[i].func_id);

						infos.emplace_back(
							std::format("Failed to match: argument (index: {}) has incorrect label", reason.arg_index),
							get_func_location(),
							evo::SmallVector<Diagnostic::Info>{
								Diagnostic::Info(
									"This label:", this->get_location(*arg_infos[reason.arg_index].ast_arg.label)
								),
								Diagnostic::Info(
									std::format(
										"Expected label: \"{}\"", 
										this->context.getSourceManager()[sema_func.sourceID]
											.getTokenBuffer()[sema_func.params[reason.arg_index].ident].getString()
									)
								),
							}
						);


					}else if constexpr(std::is_same<ReasonT, OverloadScore::IntrinsicArgWithLabel>()){
						infos.emplace_back(
							std::format("Failed to match: argument (index: {}) has a label", reason.arg_index),
							this->get_location(*arg_infos[reason.arg_index].ast_arg.label),
							evo::SmallVector<Diagnostic::Info>{
								Diagnostic::Info("Arguments to intrinsic functions cannot have labels"),
							}
						);

					}else{
						static_assert(false, "Unsupported overload score reason");
					}
				});
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
					if(func_infos[i].func_id.has_value()){
						infos.emplace_back("Could be this one:", this->get_location(*func_infos[i].func_id));
					}else{
						infos.emplace_back("Could be this one:", Diagnostic::Location::NONE);
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

		for(size_t i = 0; SelectFuncOverloadArgInfo& arg_info : arg_infos){
			if(this->type_check<true>( // this is to implicitly convert all the required args
				selected_func.func_type.params[i].typeID,
				arg_info.term_info,
				"Function call argument",
				arg_info.ast_arg.value
			).ok == false){
				evo::debugFatalBreak("This should not be able to fail");
			}
		
			i += 1;
		}

		return best_score_index;
	}


	template<bool IS_CONSTEXPR, bool ERRORS>
	auto SemanticAnalyzer::func_call_impl(
		const AST::FuncCall& func_call,
		const TermInfo& target_term_info,
		evo::ArrayProxy<SymbolProcTermInfoID> args,
		std::optional<evo::ArrayProxy<SymbolProcTermInfoID>> template_args
	) -> evo::Result<FuncCallImplData> {
		TypeManager& type_manager = this->context.type_manager;

		auto func_infos = evo::SmallVector<SelectFuncOverloadFuncInfo>();

		if(target_term_info.value_category == TermInfo::ValueCategory::FUNCTION){
			using FuncOverload = evo::Variant<sema::Func::ID, sema::TemplatedFunc::ID>;
			for(const FuncOverload& func_overload : target_term_info.type_id.as<TermInfo::FuncOverloadList>()){
				if(func_overload.is<sema::Func::ID>()){
					const sema::Func& sema_func =
						this->context.getSemaBuffer().getFunc(func_overload.as<sema::Func::ID>());
					const BaseType::Function& func_type = type_manager.getFunction(sema_func.typeID);
					func_infos.emplace_back(func_overload.as<sema::Func::ID>(), func_type);
				}
			}

		}else if(target_term_info.value_category == TermInfo::ValueCategory::INTRINSIC_FUNC){
			const TypeInfo::ID type_info_id = target_term_info.type_id.as<TypeInfo::ID>();
			const TypeInfo& type_info = type_manager.getTypeInfo(type_info_id);
			const BaseType::Function& func_type = type_manager.getFunction(type_info.baseTypeID().funcID());
			func_infos.emplace_back(std::nullopt, func_type);

		}else if(target_term_info.value_category == TermInfo::ValueCategory::TEMPLATE_INTRINSIC_FUNC){
			auto instantiation_args = evo::SmallVector<std::optional<TypeInfo::VoidableID>>();
			for(const SymbolProc::TermInfoID& arg : *template_args){
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
				std::nullopt, this->context.getTypeManager().getFunction(instantiated_type.funcID())
			);

		}else{
			this->emit_error(
				Diagnostic::Code::SEMA_CANNOT_CALL_LIKE_FUNCTION,
				func_call.target,
				"Cannot call expression like a function"
			);
			return evo::resultError;
		}


		auto arg_infos = evo::SmallVector<SelectFuncOverloadArgInfo>();
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
					return evo::resultError;
				}
			}else{
				if(this->expr_in_func_is_valid_value_stage(arg_term_info, func_call.args[i].value) == false){
					return evo::resultError;
				}
			}

			arg_infos.emplace_back(arg_term_info, func_call.args[i]);
			i += 1;
		}


		const evo::Result<size_t> selected_func_overload_index = this->select_func_overload(
			func_infos, arg_infos, func_call.target
		);
		if(selected_func_overload_index.isError()){ return evo::resultError; }


		if constexpr(ERRORS){
			if(func_infos[selected_func_overload_index.value()].func_type.hasErrorReturn() == false){
				this->emit_error(
					Diagnostic::Code::SEMA_FUNC_DOESNT_ERROR,
					func_call,
					"Function doesn't error"
				);
				return evo::resultError;
			}
		}else{
			if(func_infos[selected_func_overload_index.value()].func_type.hasErrorReturn()){
				this->emit_error(
					Diagnostic::Code::SEMA_FUNC_DOESNT_ERROR,
					func_call,
					"Function error not handled"
				);
				return evo::resultError;
			}
		}


		switch(target_term_info.value_category){
			case TermInfo::ValueCategory::FUNCTION: {
				const std::optional<sema::Func::ID> selected_func_id = 
					func_infos[selected_func_overload_index.value()].func_id;

				return FuncCallImplData(
					selected_func_id,
					&this->context.sema_buffer.getFunc(*selected_func_id),
					func_infos[selected_func_overload_index.value()].func_type
				);
			} break;

			case TermInfo::ValueCategory::INTRINSIC_FUNC: case TermInfo::ValueCategory::TEMPLATE_INTRINSIC_FUNC: {
				const BaseType::Function& selected_func_type = 
					func_infos[selected_func_overload_index.value()].func_type;

				return FuncCallImplData(std::nullopt, nullptr, selected_func_type);
			} break;

			default: evo::debugFatalBreak("Should have already been caught that value category is not callable func");
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
				evo::unimplemented();
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

			case AST::Kind::TYPE_DEDUCER: {
				evo::unimplemented();
			} break;

			case AST::Kind::TEMPLATED_EXPR: {
				evo::unimplemented();
			} break;

			case AST::Kind::INFIX: {
				evo::unimplemented();
			} break;

			case AST::Kind::TYPEID_CONVERTER: {
				evo::unimplemented();
			} break;

			default: evo::debugFatalBreak("Should not ever fail");
		}


		return TypeInfo::VoidableID(this->context.type_manager.getOrCreateTypeInfo(TypeInfo(*base_type_id)));
	}




	auto SemanticAnalyzer::genericValueToSemaExpr(core::GenericValue& value, const TypeInfo& target_type)
	-> sema::Expr {
		switch(target_type.baseTypeID().kind()){
			case BaseType::Kind::DUMMY: evo::debugFatalBreak("Invalid type");

			case BaseType::Kind::PRIMITIVE: {
				const BaseType::Primitive& primitive_type = this->context.getTypeManager().getPrimitive(
					target_type.baseTypeID().primitiveID()
				);

				switch(primitive_type.kind()){
					case Token::Kind::TYPE_INT:         case Token::Kind::TYPE_ISIZE:
					case Token::Kind::TYPE_I_N:         case Token::Kind::TYPE_UINT:
					case Token::Kind::TYPE_USIZE:       case Token::Kind::TYPE_UI_N:
					case Token::Kind::TYPE_BYTE:        case Token::Kind::TYPE_TYPEID:
					case Token::Kind::TYPE_C_SHORT:     case Token::Kind::TYPE_C_USHORT:
					case Token::Kind::TYPE_C_INT:       case Token::Kind::TYPE_C_UINT:
					case Token::Kind::TYPE_C_LONG:      case Token::Kind::TYPE_C_ULONG:
					case Token::Kind::TYPE_C_LONG_LONG: case Token::Kind::TYPE_C_ULONG_LONG: {
						return sema::Expr(
							this->context.sema_buffer.createIntValue(
								std::move(value.as<core::GenericInt>()), target_type.baseTypeID()
							)
						);
					} break;

					case Token::Kind::TYPE_F16:        case Token::Kind::TYPE_BF16: case Token::Kind::TYPE_F32:
					case Token::Kind::TYPE_F64:        case Token::Kind::TYPE_F80:  case Token::Kind::TYPE_F128:
					case Token::Kind::TYPE_C_LONG_DOUBLE: {
						return sema::Expr(
							this->context.sema_buffer.createFloatValue(
								std::move(value.as<core::GenericFloat>()), target_type.baseTypeID()
							)
						);
					} break;

					case Token::Kind::TYPE_BOOL: {
						return sema::Expr(this->context.sema_buffer.createBoolValue(value.as<bool>()));
					} break;

					case Token::Kind::TYPE_CHAR: {
						return sema::Expr(
							this->context.sema_buffer.createCharValue(static_cast<char>(value.as<core::GenericInt>()))
						);
					} break;

					case Token::Kind::TYPE_RAWPTR: evo::unimplemented("Token::Kind::TYPE_RAWPTR");

					default: evo::debugFatalBreak("Invalid type");
				}
			} break;

			case BaseType::Kind::FUNCTION: {
				evo::unimplemented("BaseType::Kind::FUNCTION");
			} break;

			case BaseType::Kind::ARRAY: {
				evo::unimplemented("BaseType::Kind::ARRAY");
			} break;

			case BaseType::Kind::ALIAS: {
				evo::unimplemented("BaseType::Kind::ALIAS");
			} break;

			case BaseType::Kind::TYPEDEF: {
				evo::unimplemented("BaseType::Kind::TYPEDEF");
			} break;

			case BaseType::Kind::STRUCT: {
				const BaseType::Struct& struct_type =
					this->context.getTypeManager().getStruct(target_type.baseTypeID().structID());

				auto values = evo::SmallVector<sema::Expr>();
				values.reserve(value.as<evo::SmallVector<core::GenericValue>>().size());


				if(struct_type.memberVars.empty() == false){
					evo::SmallVector<core::GenericValue>& member_values =
						value.as<evo::SmallVector<core::GenericValue>>();

					for(size_t i = 0; core::GenericValue& member_value : member_values){
						values.emplace_back(
							this->genericValueToSemaExpr(
								member_value,
								this->context.getTypeManager().getTypeInfo(struct_type.memberVars[i].typeID)
							)
						);
					
						i += 1;
					}
				}


				return sema::Expr(
					this->context.sema_buffer.createAggregateValue(std::move(values), target_type.baseTypeID())
				);
			} break;

			case BaseType::Kind::STRUCT_TEMPLATE: {
				evo::debugFatalBreak("Function cannot return a struct template");
			} break;

			case BaseType::Kind::TYPE_DEDUCER: {
				evo::debugFatalBreak("Function cannot return a type deducer");
			} break;
		}

		evo::unreachable();
	}





	//////////////////////////////////////////////////////////////////////
	// attributes


	auto SemanticAnalyzer::analyze_global_var_attrs(
		const AST::VarDecl& var_decl, evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
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

					if(this->type_check<true>(
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
		const AST::VarDecl& var_decl, evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
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
		const AST::StructDecl& struct_decl, evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
	) -> evo::Result<StructAttrs> {
		auto attr_pub = ConditionalAttribute(*this, "pub");
		auto attr_packed = Attribute(*this, "packed");
		auto attr_ordered = Attribute(*this, "ordered");
		// auto attr_extern = Attribute(*this, "extern");


		const AST::AttributeBlock& attribute_block = 
			this->source.getASTBuffer().getAttributeBlock(struct_decl.attributeBlock);

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

					if(this->type_check<true>(
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


	auto SemanticAnalyzer::analyze_func_attrs(
		const AST::FuncDecl& func_decl, evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
	) -> evo::Result<FuncAttrs> {
		auto attr_pub = ConditionalAttribute(*this, "pub");
		auto attr_rt = ConditionalAttribute(*this, "rt");
		auto attr_entry = Attribute(*this, "entry");

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

					if(this->type_check<true>(
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

					if(this->type_check<true>(
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

			}else if(attribute_str == "entry"){
				if(attribute_params_info[i].empty() == false){
					this->emit_error(
						Diagnostic::Code::SEMA_TOO_MANY_ATTRIBUTE_ARGS,
						attribute.args.front(),
						"Attribute #entry does not accept any arguments"
					);
					return evo::resultError;
				}

				if(attr_entry.set(attribute.attribute).isError()){ return evo::resultError; }
				attr_rt.implicitly_set(attribute.attribute, true);

			}else{
				this->emit_error(
					Diagnostic::Code::SEMA_UNKNOWN_ATTRIBUTE,
					attribute.attribute,
					std::format("Unknown function attribute #{}", attribute_str)
				);
				return evo::resultError;
			}
		}

		return FuncAttrs(attr_pub.is_set(), attr_rt.is_set(), attr_entry.is_set());
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
				this->context.add_task_to_work_manager(waited_on_id);
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

		this->context.symbol_proc_manager.symbol_proc_done();
	}



	auto SemanticAnalyzer::propagate_finished_decl_def() -> void {
		const auto lock = std::scoped_lock(this->symbol_proc.decl_waited_on_lock, this->symbol_proc.def_waited_on_lock);

		this->symbol_proc.decl_done = true;
		this->symbol_proc.def_done = true;

		this->propagate_finished_impl(this->symbol_proc.decl_waited_on_by);
		this->propagate_finished_impl(this->symbol_proc.def_waited_on_by);

		this->context.symbol_proc_manager.symbol_proc_done();
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
	-> const BaseType::StructTemplate::Instantiation& {
		evo::debugAssert(
			this->symbol_proc.struct_instantiations[instantiation_id.get()] != nullptr,
			"Symbol proc struct instantiation wasn't set"
		);
		return *this->symbol_proc.struct_instantiations[instantiation_id.get()];
	}

	auto SemanticAnalyzer::return_struct_instantiation(
		SymbolProc::StructInstantiationID instantiation_id,
		const BaseType::StructTemplate::Instantiation& instantiation
	) -> void {
		this->symbol_proc.struct_instantiations[instantiation_id.get()] = &instantiation;
	}



	//////////////////////////////////////////////////////////////////////
	// error handling / diagnostics

	template<bool MAY_IMPLICITLY_CONVERT_AND_ERROR>
	auto SemanticAnalyzer::type_check(
		TypeInfo::ID expected_type_id,
		TermInfo& got_expr,
		std::string_view expected_type_location_name,
		const auto& location,
		std::optional<unsigned> multi_type_index
	) -> TypeCheckInfo {
		evo::debugAssert(
			std::isupper(int(expected_type_location_name[0])),
			"first character of expected_type_location_name should be upper-case"
		);

		const TypeManager& type_manager = this->context.getTypeManager();

		const TypeInfo::ID actual_expected_type_id = this->get_actual_type<false>(expected_type_id);

		switch(got_expr.value_category){
			case TermInfo::ValueCategory::EPHEMERAL:
			case TermInfo::ValueCategory::CONCRETE_CONST:
			case TermInfo::ValueCategory::CONCRETE_MUT:
			case TermInfo::ValueCategory::CONCRETE_FORWARDABLE: {
				TypeInfo::ID actual_got_type_id = TypeInfo::ID::dummy();
				if(got_expr.isMultiValue()){
					if(multi_type_index.has_value() == false){
						this->emit_error(
							Diagnostic::Code::SEMA_MULTI_RETURN_INTO_SINGLE_VALUE,
							location,
							std::format("{} cannot accept multiple values", expected_type_location_name)
						);
						return TypeCheckInfo::fail();
					}

					actual_got_type_id = this->get_actual_type<false>(
						got_expr.type_id.as<evo::SmallVector<TypeInfo::ID>>()[*multi_type_index]
					);

				}else{
					actual_got_type_id = this->get_actual_type<false>(got_expr.type_id.as<TypeInfo::ID>());
				}


				// if types are not exact, check if implicit conversion is valid
				if(actual_expected_type_id != actual_got_type_id){
					const TypeInfo& expected_type = type_manager.getTypeInfo(actual_expected_type_id);
					const TypeInfo& got_type      = type_manager.getTypeInfo(actual_got_type_id);


					if(expected_type.baseTypeID().kind() == BaseType::Kind::TYPE_DEDUCER){
						evo::Result<evo::SmallVector<DeducedType>> extracted_type_deducers
							= this->extract_type_deducers(actual_expected_type_id, actual_got_type_id);

						if(extracted_type_deducers.isError()){
							if constexpr(MAY_IMPLICITLY_CONVERT_AND_ERROR){
								// TODO(FUTURE): better messaging
								this->emit_error(
									Diagnostic::Code::SEMA_TYPE_MISMATCH, // TODO(FUTURE): more specific code
									location,
									"Type deducer not able to deduce type"
								);
							}
							return TypeCheckInfo::fail();
						}

						return TypeCheckInfo::success(false, std::move(extracted_type_deducers.value()));
					}


					if(
						expected_type.baseTypeID()        != got_type.baseTypeID() || 
						expected_type.qualifiers().size() != got_type.qualifiers().size()
					){	

						if constexpr(MAY_IMPLICITLY_CONVERT_AND_ERROR){
							this->error_type_mismatch(
								expected_type_id, got_expr, expected_type_location_name, location, multi_type_index
							);
						}
						return TypeCheckInfo::fail();
					}

					// TODO(PERF): optimze this?
					for(size_t i = 0; i < expected_type.qualifiers().size(); i+=1){
						const AST::Type::Qualifier& expected_qualifier = expected_type.qualifiers()[i];
						const AST::Type::Qualifier& got_qualifier      = got_type.qualifiers()[i];

						if(expected_qualifier.isPtr != got_qualifier.isPtr){
							if constexpr(MAY_IMPLICITLY_CONVERT_AND_ERROR){
								this->error_type_mismatch(
									expected_type_id, got_expr, expected_type_location_name, location, multi_type_index
								);
							}
							return TypeCheckInfo::fail();
						}
						if(expected_qualifier.isReadOnly == false && got_qualifier.isReadOnly){
							if constexpr(MAY_IMPLICITLY_CONVERT_AND_ERROR){
								this->error_type_mismatch(
									expected_type_id, got_expr, expected_type_location_name, location, multi_type_index
								);
							}
							return TypeCheckInfo::fail();
						}
					}
				}

				if constexpr(MAY_IMPLICITLY_CONVERT_AND_ERROR){
					EVO_DEFER([&](){
						if(multi_type_index.has_value() == false){
							got_expr.type_id.emplace<TypeInfo::ID>(expected_type_id);	
						}else{
							got_expr.type_id.as<evo::SmallVector<TypeInfo::ID>>()[*multi_type_index] = expected_type_id;
						}
					});
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

				if(
					expected_type_info.qualifiers().empty() == false || 
					expected_type_info.baseTypeID().kind() != BaseType::Kind::PRIMITIVE
				){
					if constexpr(MAY_IMPLICITLY_CONVERT_AND_ERROR){
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
						case Token::Kind::TYPE_C_USHORT:
						case Token::Kind::TYPE_C_UINT:
						case Token::Kind::TYPE_C_ULONG:
						case Token::Kind::TYPE_C_ULONG_LONG:
							break;

						default: {
							if constexpr(MAY_IMPLICITLY_CONVERT_AND_ERROR){
								this->error_type_mismatch(
									expected_type_id, got_expr, expected_type_location_name, location, multi_type_index
								);
							}
							return TypeCheckInfo::fail();
						}
					}

					if constexpr(MAY_IMPLICITLY_CONVERT_AND_ERROR){
						const sema::IntValue::ID int_value_id = got_expr.getExpr().intValueID();
						sema::IntValue& int_value = this->context.sema_buffer.int_values[int_value_id];

						if(is_unsigned){
							if(int_value.value.slt(core::GenericInt(256, 0, true))){
								this->emit_error(
									Diagnostic::Code::SEMA_CANNOT_CONVERT_FLUID_VALUE,
									location,
									"Cannot implicitly convert this fluid value to the target type",
									Diagnostic::Info("Fluid value is negative and target type is unsigned")
								);
								return TypeCheckInfo::fail();
							}
						}

						core::GenericInt target_min = type_manager.getMin(expected_type_info.baseTypeID())
							.as<core::GenericInt>();

						core::GenericInt target_max = type_manager.getMax(expected_type_info.baseTypeID())
							.as<core::GenericInt>();

						if(int_value.value.getBitWidth() >= target_min.getBitWidth()){
							target_min = target_min.ext(int_value.value.getBitWidth(), is_unsigned);
							target_max = target_max.ext(int_value.value.getBitWidth(), is_unsigned);

							if(is_unsigned){
								if(int_value.value.ult(target_min) || int_value.value.ugt(target_max)){
									this->emit_error(
										Diagnostic::Code::SEMA_CANNOT_CONVERT_FLUID_VALUE,
										location,
										"Cannot implicitly convert this fluid value to the target type",
										Diagnostic::Info("Requires truncation (maybe use [as] operator)")
									);
									return TypeCheckInfo::fail();
								}
							}else{
								if(int_value.value.slt(target_min) || int_value.value.sgt(target_max)){
									this->emit_error(
										Diagnostic::Code::SEMA_CANNOT_CONVERT_FLUID_VALUE,
										location,
										"Cannot implicitly convert this fluid value to the target type",
										Diagnostic::Info("Requires truncation (maybe use [as] operator)")
									);
									return TypeCheckInfo::fail();
								}
							}

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
							if constexpr(MAY_IMPLICITLY_CONVERT_AND_ERROR){
								this->error_type_mismatch(
									expected_type_id, got_expr, expected_type_location_name, location, multi_type_index
								);
							}
							return TypeCheckInfo::fail();
						}
					}

					if constexpr(MAY_IMPLICITLY_CONVERT_AND_ERROR){
						const sema::FloatValue::ID float_value_id = got_expr.getExpr().floatValueID();
						sema::FloatValue& float_value = this->context.sema_buffer.float_values[float_value_id];


						const core::GenericFloat target_min = type_manager.getMin(expected_type_info.baseTypeID())
							.as<core::GenericFloat>().asF128();

						const core::GenericFloat target_max = type_manager.getMax(expected_type_info.baseTypeID())
							.as<core::GenericFloat>().asF128();


						const core::GenericFloat converted_literal = float_value.value.asF128();

						if(converted_literal.lt(target_min) || converted_literal.gt(target_max)){
							this->emit_error(
								Diagnostic::Code::SEMA_CANNOT_CONVERT_FLUID_VALUE,
								location,
								"Cannot implicitly convert this fluid value to the target type",
								Diagnostic::Info("Requires truncation (maybe use [as] operator)")
							);
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
								if(type_manager.sizeOf(expected_type_info.baseTypeID()) == 8){
									float_value.value = float_value.value.asF64();
								}else{
									float_value.value = float_value.value.asF128();
								}
							}
						}

						float_value.typeID = type_manager.getTypeInfo(expected_type_id).baseTypeID();
					}
				}

				if constexpr(MAY_IMPLICITLY_CONVERT_AND_ERROR){
					got_expr.value_category = TermInfo::ValueCategory::EPHEMERAL;
					got_expr.type_id.emplace<TypeInfo::ID>(expected_type_id);
				}

				return TypeCheckInfo::success(true);
			} break;

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
		}

		evo::unreachable();
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

		infos.emplace_back(
			expected_type_str + 
			this->context.getTypeManager().printType(expected_type_id, this->context.getSourceManager())
		);

		TypeInfo::ID actual_expected_type_id = expected_type_id;
		// TODO(PERF): improve perf
		while(true){
			const TypeInfo& actual_expected_type = this->context.getTypeManager().getTypeInfo(actual_expected_type_id);
			if(actual_expected_type.qualifiers().empty() == false){ break; }
			if(actual_expected_type.baseTypeID().kind() != BaseType::Kind::ALIAS){ break; }

			const BaseType::Alias& expected_alias = this->context.getTypeManager().getAlias(
				actual_expected_type.baseTypeID().aliasID()
			);

			evo::debugAssert(expected_alias.aliasedType.load().has_value(), "Definition of alias was not completed");
			actual_expected_type_id = *expected_alias.aliasedType.load();

			auto alias_of_str = std::string("\\-> Alias of: ");
			while(alias_of_str.size() < got_type_str.size()){
				alias_of_str += ' ';
			}

			infos.emplace_back(
				alias_of_str + 
				this->context.getTypeManager().printType(actual_expected_type_id, this->context.getSourceManager())
			);
		}


		infos.emplace_back(got_type_str + this->print_type(got_expr, multi_type_index));

		if(
			got_expr.type_id.is<TypeInfo::ID>()
			|| (got_expr.type_id.is<evo::SmallVector<TypeInfo::ID>>() && multi_type_index.has_value())
		){
			TypeInfo::ID actual_got_type_id = [&](){
				if(got_expr.type_id.is<TypeInfo::ID>()){
					return got_expr.type_id.as<TypeInfo::ID>();
				}else{
					return got_expr.type_id.as<evo::SmallVector<TypeInfo::ID>>()[*multi_type_index];
				}
			}();

			// TODO(PERF): improve perf
			while(true){
				const TypeInfo& actual_got_type = this->context.getTypeManager().getTypeInfo(actual_got_type_id);
				if(actual_got_type.qualifiers().empty() == false){ break; }
				if(actual_got_type.baseTypeID().kind() != BaseType::Kind::ALIAS){ break; }

				const BaseType::Alias& got_alias = this->context.getTypeManager().getAlias(
					actual_got_type.baseTypeID().aliasID()
				);

				evo::debugAssert(got_alias.aliasedType.load().has_value(), "Definition of alias was not completed");
				actual_got_type_id = *got_alias.aliasedType.load();

				auto alias_of_str = std::string("\\-> Alias of: ");
				while(alias_of_str.size() < got_type_str.size()){
					alias_of_str += ' ';
				}

				infos.emplace_back(
					alias_of_str + 
					this->context.getTypeManager().printType(actual_got_type_id, this->context.getSourceManager())
				);
			}
		}


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



	auto SemanticAnalyzer::extract_type_deducers(TypeInfo::ID deducer_id, TypeInfo::ID got_type_id)
	-> evo::Result<evo::SmallVector<DeducedType>> {
		const TypeManager& type_manager = this->context.getTypeManager();

		auto output = evo::SmallVector<DeducedType>();

		const TypeInfo& deducer  = type_manager.getTypeInfo(deducer_id);
		const TypeInfo& got_type = type_manager.getTypeInfo(got_type_id);

		if(deducer.qualifiers() != got_type.qualifiers()){ return evo::resultError; }


		const BaseType::TypeDeducer& type_deducer = type_manager.getTypeDeducer(deducer.baseTypeID().typeDeducerID());

		const Token& type_deducer_token = this->source.getTokenBuffer()[type_deducer.tokenID];

		if(type_deducer_token.kind() == Token::Kind::ANONYMOUS_TYPE_DEDUCER){
			return output;
		}

		if(deducer.qualifiers().empty()){
			output.emplace_back(got_type_id, type_deducer.tokenID);
		}else{
			output.emplace_back(
				this->context.type_manager.getOrCreateTypeInfo(TypeInfo(got_type.baseTypeID())), type_deducer.tokenID
			);
		}
		return output;
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

		const sema::ScopeLevel::AddIdentResult add_ident_result = current_scope_level.
			addIdent(
			ident_str, std::forward<decltype(ident_id_info)>(ident_id_info)...
		);

		if(add_ident_result.has_value() == false){
			const bool is_shadow_redef = add_ident_result.error();
			if(is_shadow_redef){
				const sema::ScopeLevel::IdentID& shadowed_ident =
					*current_scope_level.lookupDisallowedIdentForShadowing(ident_str);

				shadowed_ident.visit([&](const auto& first_decl_ident_id) -> void {
					using IdentIDType = std::decay_t<decltype(first_decl_ident_id)>;

					auto infos = evo::SmallVector<Diagnostic::Info>();

					infos.emplace_back("First defined here:", this->get_location(ast_node));

					const Diagnostic::Location first_ident_location = [&]() -> Diagnostic::Location {
						if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::FuncOverloadList>()){
							return first_decl_ident_id.front().visit([&](const auto& func_id) -> Diagnostic::Location {
								return this->get_location(func_id);	
							});

						}else{
							return this->get_location(first_decl_ident_id);
						}
					}();
					
					infos.emplace_back("Note: shadowing is not allowed");
					
					this->emit_error(
						Diagnostic::Code::SEMA_IDENT_ALREADY_IN_SCOPE,
						ast_node,
						std::format("Identifier \"{}\" was already defined in this scope", ident_str),
						std::move(infos)
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
				std::is_same<std::remove_cvref_t<std::decay_t<decltype(redef_id)>>, pcit::panther::AST::FuncDecl>() 
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



	auto SemanticAnalyzer::print_type(
		const TermInfo& term_info, std::optional<unsigned> multi_type_index
	) const -> std::string {
		return term_info.type_id.visit([&](const auto& type_id) -> std::string {
			using TypeID = std::decay_t<decltype(type_id)>;

			if constexpr(std::is_same<TypeID, TermInfo::InitializerType>()){
				return "{INITIALIZER}";

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
				return "{MODULE}";

			}else if constexpr(std::is_same<TypeID, sema::TemplatedStruct::ID>()){
				// TODO(FEATURE): actual name
				return "{TEMPLATED_STRUCT}";

			}else if constexpr(std::is_same<TypeID, TemplateIntrinsicFunc::Kind>()){
				// TODO(FEATURE): actual name
				return "{TEMPLATE_INTRINSIC_FUNC}";

			}else if constexpr(std::is_same<TypeID, TermInfo::TemplateDeclInstantiationType>()){
				// TODO(FEATURE): actual name?
				return "{TEMPLATE_DECL_INSTANTIATION_TYPE}";

			}else if constexpr(std::is_same<TypeID, TermInfo::ExceptParamPack>()){
				return "{EXCEPT_PARAM_PACK}";

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