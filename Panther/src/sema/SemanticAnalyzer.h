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


#include "../../include/Context.h"


namespace pcit::panther{


	class SemanticAnalyzer{
		public:
			EVO_NODISCARD static auto create(Context& context, SymbolProc::ID symbol_proc_id)
			-> SemanticAnalyzer {
				SymbolProc& symbol_proc = context.symbol_proc_manager.getSymbolProc(symbol_proc_id);
				Source& source = context.getSourceManager()[symbol_proc.getSourceID()];

				if(symbol_proc.sema_scope_id.has_value() == false){
					symbol_proc.sema_scope_id = context.sema_buffer.scope_manager.copyScope(*source.sema_scope_id);
				}

				sema::ScopeManager::Scope& scope = context.sema_buffer.scope_manager.getScope(
					*symbol_proc.sema_scope_id
				);

				return SemanticAnalyzer(context, source, symbol_proc_id, symbol_proc, scope);
			}

			~SemanticAnalyzer() = default;

			auto analyze() -> void;

		private:
			enum class Result{
				SUCCESS,
				ERROR,
				RECOVERABLE_ERROR,
				ERROR_NO_REPORT,
				NEED_TO_WAIT,
				NEED_TO_WAIT_BEFORE_NEXT_INSTR,
				SUSPEND,
			};


			///////////////////////////////////
			// instructions

			using Instruction = SymbolProc::Instruction;

			auto analyze_instr(const Instruction& instruction) -> Result;

			EVO_NODISCARD auto instr_suspend_symbol_proc() -> Result;

			EVO_NODISCARD auto instr_non_local_var_decl(const Instruction::NonLocalVarDecl& instr) -> Result;
			EVO_NODISCARD auto instr_non_local_var_def(const Instruction::NonLocalVarDef& instr) -> Result;
			EVO_NODISCARD auto instr_non_local_var_decl_def(const Instruction::NonLocalVarDeclDef& instr) -> Result;
			EVO_NODISCARD auto instr_when_cond(const Instruction::WhenCond& instr) -> Result;
			EVO_NODISCARD auto instr_alias(const Instruction::Alias& instr) -> Result;

			template<bool IS_INSTANTIATION>
			EVO_NODISCARD auto instr_struct_decl(const Instruction::StructDecl<IS_INSTANTIATION>& instr) -> Result;
			EVO_NODISCARD auto instr_struct_def() -> Result;
			EVO_NODISCARD auto instr_struct_created_sepcial_members_pir_if_needed() -> Result;
			EVO_NODISCARD auto instr_template_struct(const Instruction::TemplateStruct& instr) -> Result;
			EVO_NODISCARD auto instr_union_decl(const Instruction::UnionDecl& instr) -> Result;
			EVO_NODISCARD auto instr_union_add_fields(const Instruction::UnionAddFields& instr) -> Result;
			EVO_NODISCARD auto instr_union_def() -> Result;
			EVO_NODISCARD auto instr_enum_decl(const Instruction::EnumDecl& instr) -> Result;
			EVO_NODISCARD auto instr_enum_add_enumerators(const Instruction::EnumAddEnumerators& instr) -> Result;
			EVO_NODISCARD auto instr_enum_def() -> Result;

			EVO_NODISCARD auto instr_func_decl_extract_deducers(const Instruction::FuncDeclExtractDeducers& instr)
				-> Result;
			template<bool IS_INSTANTIATION>
			EVO_NODISCARD auto instr_func_decl(const Instruction::FuncDecl<IS_INSTANTIATION>& instr) -> Result;
			EVO_NODISCARD auto instr_func_pre_body(const Instruction::FuncPreBody& instr) -> Result;
			EVO_NODISCARD auto instr_func_def(const Instruction::FuncDef& instr) -> Result;
			EVO_NODISCARD auto instr_func_prepare_comptime_pir_if_needed(
				const Instruction::FuncPrepareComptimePIRIfNeeded& instr
			) -> Result;
			EVO_NODISCARD auto instr_func_comptime_pir_ready_if_needed() -> Result;

			EVO_NODISCARD auto instr_template_func_begin(const Instruction::TemplateFuncBegin& instr) -> Result;
			EVO_NODISCARD auto instr_template_set_param_is_deducer(
				const Instruction::TemplateFuncSetParamIsDeducer& instr
			) -> Result;
			EVO_NODISCARD auto instr_template_func_end(const Instruction::TemplateFuncEnd& instr) -> Result;
			EVO_NODISCARD auto instr_deleted_special_method(const Instruction::DeletedSpecialMethod& instr) -> Result;

			EVO_NODISCARD auto instr_interface_prepare(const Instruction::InterfacePrepare& instr) -> Result;
			EVO_NODISCARD auto instr_func_alias_def(const Instruction::FuncAliasDef& instr) -> Result;
			EVO_NODISCARD auto instr_interface_decl() -> Result;
			EVO_NODISCARD auto instr_interface_def() -> Result;
			EVO_NODISCARD auto instr_interface_func_def(const Instruction::InterfaceFuncDef& instr) -> Result;
			EVO_NODISCARD auto instr_interface_impl_decl(const Instruction::InterfaceImplDecl& instr) -> Result;
			EVO_NODISCARD auto instr_interface_in_def_impl_decl(const Instruction::InterfaceInDefImplDecl& instr)
				-> Result;
			EVO_NODISCARD auto instr_interface_deducer_impl_instantiation_decl(
				const Instruction::InterfaceDeducerImplInstantiationDecl& instr
			) -> Result;
			EVO_NODISCARD auto instr_interface_impl_method_lookup(const Instruction::InterfaceImplMethodLookup& instr)
				-> Result;
			EVO_NODISCARD auto instr_interface_in_def_impl_method(const Instruction::InterfaceInDefImplMethod& instr)
				-> Result;
			EVO_NODISCARD auto instr_interface_impl_def(const Instruction::InterfaceImplDef& instr) -> Result;
			EVO_NODISCARD auto instr_interface_impl_comptime_pir() -> Result;



			EVO_NODISCARD auto instr_local_var(const Instruction::LocalVar& instr) -> Result;
			EVO_NODISCARD auto instr_local_func_alias(const Instruction::LocalFuncAlias& instr) -> Result;
			EVO_NODISCARD auto instr_local_alias(const Instruction::LocalAlias& instr) -> Result;
			EVO_NODISCARD auto instr_return(const Instruction::Return& instr) -> Result;
			EVO_NODISCARD auto instr_labeled_return(const Instruction::LabeledReturn& instr) -> Result;
			EVO_NODISCARD auto instr_error(const Instruction::Error& instr) -> Result;
			EVO_NODISCARD auto instr_unreachable(const Instruction::Unreachable& instr) -> Result;
			EVO_NODISCARD auto instr_break(const Instruction::Break& instr) -> Result;
			EVO_NODISCARD auto instr_continue(const Instruction::Continue& instr) -> Result;
			EVO_NODISCARD auto instr_delete(const Instruction::Delete& instr) -> Result;
			EVO_NODISCARD auto instr_begin_cond(const Instruction::BeginCond& instr) -> Result;
			EVO_NODISCARD auto instr_cond_no_else() -> Result;
			EVO_NODISCARD auto instr_cond_else() -> Result;
			EVO_NODISCARD auto instr_cond_else_if() -> Result;
			EVO_NODISCARD auto instr_end_cond() -> Result;
			EVO_NODISCARD auto instr_end_cond_set(const Instruction::EndCondSet& instr) -> Result;
			EVO_NODISCARD auto instr_begin_local_when_cond(const Instruction::BeginLocalWhenCond& instr) -> Result;
			EVO_NODISCARD auto instr_end_local_when_cond(const Instruction::EndLocalWhenCond& instr) -> Result;
			EVO_NODISCARD auto instr_begin_while(const Instruction::BeginWhile& instr) -> Result;
			EVO_NODISCARD auto instr_end_while(const Instruction::EndWhile& instr) -> Result;
			EVO_NODISCARD auto instr_begin_for(const Instruction::BeginFor& instr) -> Result;
			EVO_NODISCARD auto instr_end_for(const Instruction::EndFor& instr) -> Result;
			EVO_NODISCARD auto instr_begin_for_unroll(const Instruction::BeginForUnroll& instr) -> Result;
			EVO_NODISCARD auto instr_for_unroll_cond(const Instruction::ForUnrollCond& instr) -> Result;
			EVO_NODISCARD auto instr_for_unroll_continue(const Instruction::ForUnrollContinue& instr) -> Result;
			EVO_NODISCARD auto instr_begin_switch(const Instruction::BeginSwitch& instr) -> Result;
			EVO_NODISCARD auto instr_begin_case(const Instruction::BeginCase& instr) -> Result;
			EVO_NODISCARD auto instr_end_case() -> Result;
			EVO_NODISCARD auto instr_end_switch(const Instruction::EndSwitch& instr) -> Result;
			EVO_NODISCARD auto instr_begin_defer(const Instruction::BeginDefer& instr) -> Result;
			EVO_NODISCARD auto instr_end_defer(const Instruction::EndDefer& instr) -> Result;
			EVO_NODISCARD auto instr_begin_stmt_block(const Instruction::BeginStmtBlock& instr) -> Result;
			EVO_NODISCARD auto instr_end_stmt_block(const Instruction::EndStmtBlock& instr) -> Result;
			EVO_NODISCARD auto instr_func_call(const Instruction::FuncCall& instr) -> Result;
			EVO_NODISCARD auto instr_assignment(const Instruction::Assignment& instr) -> Result;
			EVO_NODISCARD auto instr_assignment_new(const Instruction::AssignmentNew& instr) -> Result;
			EVO_NODISCARD auto instr_assignment_copy(const Instruction::AssignmentCopy& instr) -> Result;
			EVO_NODISCARD auto instr_assignment_move(const Instruction::AssignmentMove& instr) -> Result;
			EVO_NODISCARD auto instr_assignment_forward(const Instruction::AssignmentForward& instr) -> Result;
			EVO_NODISCARD auto instr_multi_assign(const Instruction::MultiAssign& instr) -> Result;
			EVO_NODISCARD auto instr_discarding_assignment(const Instruction::DiscardingAssignment& instr) -> Result;
			EVO_NODISCARD auto instr_try_else_begin(const Instruction::TryElseBegin& instr) -> Result;
			EVO_NODISCARD auto instr_try_else_end() -> Result;
			EVO_NODISCARD auto instr_begin_unsafe(const Instruction::BeginUnsafe& instr) -> Result;
			EVO_NODISCARD auto instr_end_unsafe() -> Result;


			EVO_NODISCARD auto instr_type_to_term(const Instruction::TypeToTerm& instr) -> Result;
			EVO_NODISCARD auto instr_require_this_def() -> Result;
			EVO_NODISCARD auto instr_wait_on_sub_symbol_proc_decl(const Instruction::WaitOnSubSymbolProcDecl& instr)
				-> Result;
			EVO_NODISCARD auto instr_wait_on_sub_symbol_proc_def(const Instruction::WaitOnSubSymbolProcDef& instr)
				-> Result;

			template<bool IS_COMPTIME, bool ERRORS>
			EVO_NODISCARD auto instr_func_call_expr(const Instruction::FuncCallExpr<IS_COMPTIME, ERRORS>& instr)
				-> Result;

			template<bool IS_COMPTIME>
			EVO_NODISCARD auto builtin_type_method_call(
				const TermInfo& target_term_info,
				evo::SmallVector<sema::Expr>&& args,
				SymbolProc::TermInfoID output,
				const AST::FuncCall& ast_func_call
			) -> Result;

			template<bool IS_COMPTIME>
			EVO_NODISCARD auto interface_func_call(
				const TermInfo& target_term_info,
				evo::SmallVector<sema::Expr>&& args,
				sema::Func::ID selected_func_call_id,
				SymbolProc::TermInfoID output
			) -> Result;

			EVO_NODISCARD auto instr_comptime_func_call_run(const Instruction::ComptimeFuncCallRun& instr) -> Result;

			template<Instruction::Language LANGUAGE>
			EVO_NODISCARD auto instr_import(const Instruction::Import<LANGUAGE>& instr) -> Result;

			EVO_NODISCARD auto instr_is_macro_defined(const Instruction::IsMacroDefined& instr) -> Result;
			EVO_NODISCARD auto instr_make_init_ptr(const Instruction::MakeInitPtr& instr) -> Result;
			EVO_NODISCARD auto instr_comptime_error(const Instruction::ComptimeError& instr) -> Result;
			EVO_NODISCARD auto instr_comptime_assert(const Instruction::ComptimeAssert& instr) -> Result;

			EVO_NODISCARD auto instr_template_intrinsic_func_call(const Instruction::TemplateIntrinsicFuncCall& instr)
				-> Result;

			template<bool IS_COMPTIME>
			EVO_NODISCARD auto instr_template_intrinsic_func_call_expr(
				const Instruction::TemplateIntrinsicFuncCallExpr<IS_COMPTIME>& instr
			) -> Result;

			template<bool IS_COMPTIME>
			EVO_NODISCARD auto instr_indexer(const Instruction::Indexer<IS_COMPTIME>& instr) -> Result;

			EVO_NODISCARD auto instr_templated_term(const Instruction::TemplatedTerm& instr) -> Result;

			template<bool WAIT_FOR_DEF>
			EVO_NODISCARD auto instr_templated_term_wait(const Instruction::TemplatedTermWait<WAIT_FOR_DEF>& instr)
				-> Result;

			EVO_NODISCARD auto instr_push_template_decl_instantiation_types_scope() -> Result;
			EVO_NODISCARD auto instr_pop_template_decl_instantiation_types_scope() -> Result;
			EVO_NODISCARD auto instr_add_template_decl_instantiation_type(
				const Instruction::AddTemplateDeclInstantiationType& instr
			) -> Result;
			EVO_NODISCARD auto instr_copy(const Instruction::Copy& instr) -> Result;
			EVO_NODISCARD auto instr_move(const Instruction::Move& instr) -> Result;
			EVO_NODISCARD auto instr_forward(const Instruction::Forward& instr) -> Result;
			EVO_NODISCARD auto instr_addr_of(const Instruction::AddrOf& instr) -> Result;

			template<bool IS_COMPTIME>
			EVO_NODISCARD auto instr_prefix_negate(const Instruction::PrefixNegate<IS_COMPTIME>& instr) -> Result;

			template<bool IS_COMPTIME>
			EVO_NODISCARD auto instr_prefix_not(const Instruction::PrefixNot<IS_COMPTIME>& instr) -> Result;
			
			template<bool IS_COMPTIME>
			EVO_NODISCARD auto instr_prefix_bitwise_not(const Instruction::PrefixBitwiseNot<IS_COMPTIME>& instr)
				-> Result;

			EVO_NODISCARD auto instr_deref(const Instruction::Deref& instr) -> Result;
			EVO_NODISCARD auto instr_unwrap(const Instruction::Unwrap& instr) -> Result;

			template<bool IS_COMPTIME, bool ERRORS>
			EVO_NODISCARD auto instr_new(const Instruction::New<IS_COMPTIME, ERRORS>& instr) -> Result;

			template<bool IS_COMPTIME>
			EVO_NODISCARD auto instr_array_init_new(const Instruction::ArrayInitNew<IS_COMPTIME>& instr) -> Result;

			template<bool IS_COMPTIME>
			EVO_NODISCARD auto instr_designated_init_new(const Instruction::DesignatedInitNew<IS_COMPTIME>& instr)
				-> Result;

			EVO_NODISCARD auto instr_prepare_try_handler(const Instruction::PrepareTryHandler& instr) -> Result;
			EVO_NODISCARD auto instr_try_else_expr(const Instruction::TryElseExpr& instr) -> Result;
			EVO_NODISCARD auto instr_begin_expr_block(const Instruction::BeginExprBlock& instr) -> Result;
			EVO_NODISCARD auto instr_end_expr_block(const Instruction::EndExprBlock& instr) -> Result;

			template<bool IS_COMPTIME>
			EVO_NODISCARD auto instr_expr_as(const Instruction::As<IS_COMPTIME>& instr) -> Result;

			// return nullopt to type has no interface maps 
			EVO_NODISCARD auto operator_as_check_interface_map(
				const TermInfo& from_expr,
				const TypeInfo::ID from_type_id,
				const TypeInfo::ID to_type_id,
				SymbolProc::TermInfoID output_id,
				const AST::Infix& ast_infix
			) -> std::optional<Result>;



			EVO_NODISCARD auto instr_optional_null_check(const Instruction::OptionalNullCheck& instr) -> Result;			

			template<bool IS_COMPTIME, Instruction::MathInfixKind MATH_INFIX_KIND>
			EVO_NODISCARD auto instr_expr_math_infix(const Instruction::MathInfix<IS_COMPTIME, MATH_INFIX_KIND>& instr)
				-> Result;

			template<bool IS_COMPTIME>
			EVO_NODISCARD auto instr_expr_accessor(const Instruction::Accessor<IS_COMPTIME>& instr) -> Result;

			EVO_NODISCARD auto instr_primitive_type(const Instruction::PrimitiveType& instr) -> Result;
			EVO_NODISCARD auto instr_primitive_type_term(const Instruction::PrimitiveTypeTerm& instr) -> Result;
			EVO_NODISCARD auto instr_array_type(const Instruction::ArrayType& instr) -> Result;
			EVO_NODISCARD auto instr_array_ref(const Instruction::ArrayRef& instr) -> Result;
			EVO_NODISCARD auto instr_interface_map(const Instruction::InterfaceMap& instr) -> Result;
			EVO_NODISCARD auto instr_type_id_converter(const Instruction::TypeIDConverter& instr) -> Result;
			EVO_NODISCARD auto instr_qualified_type(const Instruction::QualifiedType& instr) -> Result;
			EVO_NODISCARD auto instr_qualified_type_term(const Instruction::QualifiedTypeTerm& instr) -> Result;
			EVO_NODISCARD auto instr_base_type_ident(const Instruction::BaseTypeIdent& instr) -> Result;

			template<bool NEEDS_DEF>
			EVO_NODISCARD auto instr_ident(const Instruction::Ident<NEEDS_DEF>& instr) -> Result;

			EVO_NODISCARD auto instr_intrinsic(const Instruction::Intrinsic& instr) -> Result;

			template<bool NEEDS_DEF>
			EVO_NODISCARD auto instr_type_this(const Instruction::TypeThis<NEEDS_DEF>& instr) -> Result;

			EVO_NODISCARD auto instr_literal(const Instruction::Literal& instr) -> Result;
			EVO_NODISCARD auto instr_uninit(const Instruction::Uninit& instr) -> Result;
			EVO_NODISCARD auto instr_zeroinit(const Instruction::Zeroinit& instr) -> Result;
			EVO_NODISCARD auto instr_this(const Instruction::This& instr) -> Result;
			EVO_NODISCARD auto instr_type_deducer(const Instruction::TypeDeducer& instr) -> Result;
			EVO_NODISCARD auto instr_expr_deducer(const Instruction::ExprDeducer& instr) -> Result;



			///////////////////////////////////
			// accessors

			template<bool NEEDS_DEF>
			EVO_NODISCARD auto module_accessor(
				const Instruction::Accessor<NEEDS_DEF>& instr,
				std::string_view rhs_ident_str,
				const TermInfo& lhs
			) -> Result;

			template<bool NEEDS_DEF>
			EVO_NODISCARD auto clang_module_accessor(
				const Instruction::Accessor<NEEDS_DEF>& instr,
				std::string_view rhs_ident_str,
				const TermInfo& lhs
			) -> Result;

			template<bool NEEDS_DEF>
			EVO_NODISCARD auto type_accessor(
				const Instruction::Accessor<NEEDS_DEF>& instr,
				std::string_view rhs_ident_str,
				const TermInfo& lhs
			) -> Result;

			template<bool NEEDS_DEF>
			EVO_NODISCARD auto builtin_module_accessor(
				const Instruction::Accessor<NEEDS_DEF>& instr,
				std::string_view rhs_ident_str,
				const TermInfo& lhs
			) -> Result;


			template<bool NEEDS_DEF>
			EVO_NODISCARD auto interface_accessor(
				const Instruction::Accessor<NEEDS_DEF>& instr,
				std::string_view rhs_ident_str,
				const TermInfo& lhs,
				TypeInfo::ID decayed_lhs_type_id,
				const TypeInfo& decayed_lhs_type,
				bool is_ref
			) -> Result;


			template<bool NEEDS_DEF>
			EVO_NODISCARD auto optional_accessor(
				const Instruction::Accessor<NEEDS_DEF>& instr,
				std::string_view rhs_ident_str,
				const TermInfo& lhs,
				TypeInfo::ID decayed_lhs_type_id,
				const TypeInfo& decayed_lhs_type,
				bool is_pointer
			) -> Result;


			template<bool NEEDS_DEF>
			EVO_NODISCARD auto struct_accessor(
				const Instruction::Accessor<NEEDS_DEF>& instr,
				std::string_view rhs_ident_str,
				const TermInfo& lhs,
				TypeInfo::ID decayed_lhs_type_id,
				const TypeInfo& decayed_lhs_type,
				bool is_pointer
			) -> Result;

			template<bool NEEDS_DEF>
			EVO_NODISCARD auto union_accessor(
				const Instruction::Accessor<NEEDS_DEF>& instr,
				std::string_view rhs_ident_str,
				const TermInfo& lhs,
				TypeInfo::ID decayed_lhs_type_id,
				const TypeInfo& decayed_lhs_type,
				bool is_pointer
			) -> Result;

			template<bool NEEDS_DEF>
			EVO_NODISCARD auto enum_accessor(
				const Instruction::Accessor<NEEDS_DEF>& instr,
				std::string_view rhs_ident_str,
				const TermInfo& lhs,
				TypeInfo::ID decayed_lhs_type_id,
				const TypeInfo& decayed_lhs_type,
				bool is_pointer
			) -> Result;

			template<bool NEEDS_DEF>
			EVO_NODISCARD auto array_accessor(
				const Instruction::Accessor<NEEDS_DEF>& instr,
				std::string_view rhs_ident_str,
				const TermInfo& lhs,
				TypeInfo::ID decayed_lhs_type_id,
				const TypeInfo& decayed_lhs_type,
				bool is_pointer
			) -> Result;

			template<bool NEEDS_DEF>
			EVO_NODISCARD auto array_ref_accessor(
				const Instruction::Accessor<NEEDS_DEF>& instr,
				std::string_view rhs_ident_str,
				const TermInfo& lhs,
				TypeInfo::ID decayed_lhs_type_id,
				const TypeInfo& decayed_lhs_type,
				bool is_pointer
			) -> Result;


			///////////////////////////////////
			// scope

			EVO_NODISCARD auto get_current_scope_level() const -> sema::ScopeLevel&;
			EVO_NODISCARD auto push_scope_level(sema::StmtBlock* stmt_block = nullptr) -> void;
			EVO_NODISCARD auto push_scope_level(
				sema::StmtBlock& stmt_block, Token::ID label, sema::ScopeLevel::LabelNode label_node
			) -> void;
			EVO_NODISCARD auto push_scope_level(sema::StmtBlock* stmt_block, const auto& encapsulating_symbol_id)
				-> void;

			enum class PopScopeLevelKind{
				LABEL_TERMINATE,
				NORMAL,
				SYMBOL_END,
			};
			template<PopScopeLevelKind POP_SCOPE_LEVEL_KIND = PopScopeLevelKind::NORMAL>
			EVO_NODISCARD auto pop_scope_level() -> evo::Result<>;


			enum class SpecialMemberKind{
				DELETE,
				COPY_INIT,
				COPY_ASSIGN,
				MOVE_INIT,
				MOVE_ASSIGN,
			};
			template<SpecialMemberKind SPECIAL_MEMBER_KIND, bool CHECK_VALIDITY>
			EVO_NODISCARD auto get_special_member_call_dependents(
				TypeInfo::ID type_info_id,
				TermInfo::ValueCategory value_category,
				std::unordered_set<sema::Func::ID>& dependent_funcs,
				const auto& location
			) -> evo::Result<>;

			template<SpecialMemberKind SPECIAL_MEMBER_KIND, bool CHECK_VALIDITY>
			EVO_NODISCARD auto get_special_member_call_dependents(
				const TermInfo& term_info,
				std::unordered_set<sema::Func::ID>& dependent_funcs,
				const auto& location
			) -> evo::Result<> {
				return this->get_special_member_call_dependents<SPECIAL_MEMBER_KIND, CHECK_VALIDITY>(
					term_info.type_id.as<TypeInfo::ID>(),
					term_info.value_category,
					dependent_funcs,
					location
				);
			}


			EVO_NODISCARD auto currently_in_unsafe() const -> bool;


			EVO_NODISCARD auto currently_in_func() const -> bool;

			EVO_NODISCARD auto get_current_func() -> sema::Func&;
			EVO_NODISCARD auto get_current_func() const -> const sema::Func&;

			EVO_NODISCARD auto get_current_func_value_stage() const -> TermInfo::ValueStage;


			auto end_sub_scopes(Diagnostic::Location&& location) -> evo::Result<>;

			auto end_sub_scopes(const Diagnostic::Location& location) -> evo::Result<> {
				return this->end_sub_scopes(evo::copy(location));
			}



			//////////////////
			// value states

			auto add_ident_value_state(
				sema::ScopeLevel::ValueStateID value_state_id, sema::ScopeLevel::ValueState value_state
			) -> void;

			EVO_NODISCARD auto get_ident_value_state(sema::ScopeLevel::ValueStateID value_state_id)
				-> TermInfo::ValueState;

			auto set_ident_value_state(
				sema::ScopeLevel::ValueStateID value_state_id, sema::ScopeLevel::ValueState value_state
			) -> void;

			auto set_ident_value_state_if_needed(
				sema::Expr target, sema::ScopeLevel::ValueState value_state, Diagnostic::Location location
			) -> evo::Result<>;
			auto set_ident_value_state_if_needed(
				sema::Expr target, sema::ScopeLevel::ValueState value_state, const auto& location
			) -> evo::Result<> {
				return this->set_ident_value_state_if_needed(target, value_state, this->get_location(location));
			}



			///////////////////////////////////
			// misc

			template<bool NEEDS_DEF>
			EVO_NODISCARD auto lookup_ident_impl(Token::ID ident) -> evo::Expected<TermInfo, Result>;


			enum class ScopeAccessRequirement{
				NONE,
				PUB,
				NOT_PRIV,
			};
			enum class AnalyzeExprIdentInScopeLevelError{
				DOESNT_EXIST,
				NEEDS_TO_WAIT_ON_DEF,
				ERROR_EMITTED,
			};
			template<bool NEEDS_DEF, ScopeAccessRequirement SCOPE_ACCESS_REQUIREMENT>
			EVO_NODISCARD auto analyze_expr_ident_in_scope_level(
				const Token::ID& ident,
				std::string_view ident_str,
				const sema::ScopeLevel& scope_level,
				bool variables_in_scope,
				bool is_global_scope,
				const Source* source_module // optional
			) -> evo::Expected<TermInfo, AnalyzeExprIdentInScopeLevelError>;




			enum class WaitOnSymbolProcResult{
				NOT_FOUND,
				CIRCULAR_DEP_DETECTED,
				EXISTS_BUT_ERRORED,
				ERROR_PASSED_BY_WHEN_COND,
				NEED_TO_WAIT,
				SEMAS_READY,
			};

			template<bool NEEDS_DEF>
			EVO_NODISCARD auto wait_on_symbol_proc(
				evo::ArrayProxy<const SymbolProc::Namespace*> symbol_proc_namespaces, std::string_view ident_str
			) -> WaitOnSymbolProcResult;


			auto wait_on_symbol_proc_emit_error(WaitOnSymbolProcResult result, const auto& ident, std::string&& msg)
				-> void;


			auto set_waiting_for_is_done(SymbolProc::ID target_id, SymbolProc::ID done_id) -> void;



			struct SelectFuncOverloadFuncInfo{
				struct IntrinsicFlag{};
				struct BuiltinTypeMethodFlag{};
				using FuncID = evo::Variant<
					IntrinsicFlag, BuiltinTypeMethodFlag, sema::Func::ID, sema::TemplatedFunc::InstantiationInfo
				>;

				FuncID func_id;
				const BaseType::Function& func_type;
			};

			struct SelectFuncOverloadArgInfo{
				TermInfo& term_info;
				AST::Node ast_node;
				std::optional<Token::ID> label;
			};

			EVO_NODISCARD auto select_func_overload(
				evo::ArrayProxy<SelectFuncOverloadFuncInfo> func_infos,
				std::span<SelectFuncOverloadArgInfo> arg_infos,
				const auto& call_node,
				bool is_member_call,
				evo::SmallVector<Diagnostic::Info>&& instantiation_error_infos
			) -> evo::Result<size_t>; // returns index of selected overload


			struct FuncCallImplData{
				std::optional<sema::Func::ID> selected_func_id; // nullopt if is intrinsic/builtin-type method
				const sema::Func* selected_func; // nullptr if is intrinsic/builtin-type method
				const BaseType::Function& selected_func_type;

				EVO_NODISCARD auto is_src_func() const -> bool { return this->selected_func_id.has_value(); }
			};
			template<bool IS_COMPTIME, bool ERRORS>
			EVO_NODISCARD auto func_call_impl(
				const AST::FuncCall& func_call,
				const TermInfo& target_term_info,
				evo::ArrayProxy<SymbolProcTermInfoID> args,
				evo::ArrayProxy<SymbolProcTermInfoID> template_args
			) -> evo::Expected<FuncCallImplData, bool>; // unexpected true == error, unexpected false == need to wait

			struct TemplateOverloadMatchFail{
				using Handled = std::monostate;
				struct TooFewTemplateArgs{ size_t min_num; size_t got_num; bool accepts_different_nums; };
				struct TooManyTemplateArgs{ size_t max_num; size_t got_num; bool accepts_different_nums; };
				struct TemplateArgWrongKind{ size_t arg_index; bool supposed_to_be_expr; };
				struct TemplateWrongExprType{
					size_t arg_index; TypeInfo::ID expected_type_id; SymbolProc::TermInfoID got_term_id;
				};
				struct WrongNumArgs{ size_t expected_num; size_t got_num; };
				struct CantDeduceArgType{ size_t arg_index; };

				using Reason = evo::Variant<
					Handled,
					TooFewTemplateArgs,
					TooManyTemplateArgs,
					TemplateArgWrongKind,
					TemplateWrongExprType,
					WrongNumArgs,
					CantDeduceArgType
				>;

				Reason reason;
			};

			EVO_NODISCARD auto get_select_func_overload_func_info_for_template(
				const AST::FuncCall& func_call,
				sema::TemplatedFunc::ID func_id,
				evo::ArrayProxy<SymbolProcTermInfoID> args,
				evo::ArrayProxy<SymbolProcTermInfoID> template_args,
				bool is_member_call
			) -> evo::Expected<sema::TemplatedFunc::InstantiationInfo, TemplateOverloadMatchFail>;


			EVO_NODISCARD auto expr_in_func_is_valid_value_stage(
				const TermInfo& term_info, const auto& node_location
			) -> bool;


			EVO_NODISCARD auto resolve_type(const AST::Type& type) -> evo::Result<TypeInfo::VoidableID>;


			EVO_NODISCARD auto generic_value_to_sema_expr(const core::GenericValue& value, const TypeInfo& target_type)
				-> sema::Expr;

			EVO_NODISCARD auto sema_expr_to_generic_value(const sema::Expr& expr) -> core::GenericValue;

			EVO_NODISCARD auto extract_string_from_sema_expr(sema::Expr expr) -> std::string_view;


			EVO_NODISCARD auto get_package() const -> const Source::Package&;



			// success of returned expected is the generated type (or nullopt if interface doesn't match) 
			// error of returned expected should just be the returned Result of the current instruction
			EVO_NODISCARD auto type_implements_interface(
				BaseType::Interface& interface_type, TypeInfo::ID target_type_id, Diagnostic::Location location
			) -> evo::Expected<bool, Result>;



			struct DeducerMatchOutput{
				struct DeducedTerm{
					struct Expr{
						TypeInfo::ID type_id;
						sema::Expr expr;
					};

					evo::Variant<TypeInfo::VoidableID, Expr> value;
					Token::ID tokenID;
				};

				enum class Outcome{
					MATCH,
					NO_MATCH,
					RESULT,
				};

				DeducerMatchOutput(evo::SmallVector<DeducedTerm>&& deduced_terms, TypeInfo::ID resultant_type_id) :
					_result(),
					_deduced_terms(std::move(deduced_terms)),
					_resultant_type_id(resultant_type_id),
					_outcome(Outcome::MATCH)
				{}

				DeducerMatchOutput(evo::ResultError_t) :
					_result(),
					_deduced_terms(),
					_resultant_type_id(),
					_outcome(Outcome::NO_MATCH)
				{}

				DeducerMatchOutput(Result result) :
					_result(result),
					_deduced_terms(),
					_resultant_type_id(),
					_outcome(Outcome::RESULT)
				{}

				EVO_NODISCARD auto outcome() const -> Outcome { return this->_outcome; }

				EVO_NODISCARD auto result() const -> Result {
					evo::debugAssert(this->outcome() == Outcome::RESULT, "Incorrect outcome");
					return *this->_result;
				}

				EVO_NODISCARD auto deducedTerms() const& -> evo::ArrayProxy<DeducedTerm> {
					evo::debugAssert(this->outcome() == Outcome::MATCH, "Incorrect outcome");
					return this->_deduced_terms;
				}

				EVO_NODISCARD auto deducedTerms() & -> evo::SmallVector<DeducedTerm> {
					evo::debugAssert(this->outcome() == Outcome::MATCH, "Incorrect outcome");
					return this->_deduced_terms;
				}

				EVO_NODISCARD auto deducedTerms() && -> evo::SmallVector<DeducedTerm>&& {
					evo::debugAssert(this->outcome() == Outcome::MATCH, "Incorrect outcome");
					return std::move(this->_deduced_terms);
				}

				EVO_NODISCARD auto resultantTypeID() const -> TypeInfo::ID {
					evo::debugAssert(this->outcome() == Outcome::MATCH, "Incorrect outcome");
					return *this->_resultant_type_id;
				}

				private:
					std::optional<Result> _result;
					evo::SmallVector<DeducedTerm> _deduced_terms;
					std::optional<TypeInfo::ID> _resultant_type_id;
					Outcome _outcome;
			};

			EVO_NODISCARD auto deducer_matches_and_extract(TypeInfo::ID deducer_id, TypeInfo::ID got_type_id)
				-> DeducerMatchOutput;

			EVO_NODISCARD auto add_deduced_terms_to_scope(
				evo::ArrayProxy<DeducerMatchOutput::DeducedTerm> deduced_terms
			) -> evo::Result<>;


			EVO_NODISCARD auto create_prefix_overload(
				const BaseType::Function& created_func_type,
				BaseType::Struct& current_struct,
				const AST::FuncDef& ast_func_def,
				sema::Func::ID created_func_id,
				sema::Func& created_func,
				bool is_commutative,
				bool is_swapped,
				const Token& name_token
			) -> evo::Result<>;

			EVO_NODISCARD auto infix_overload_impl(
				const std::unordered_multimap<Token::Kind, sema::Func::ID>& infix_overloads,
				TermInfo& lhs,
				TermInfo& rhs,
				const AST::Infix& ast_infix
			) -> evo::Expected<sema::FuncCall::ID, Result>;

			EVO_NODISCARD auto prefix_overload_impl(
				const std::unordered_multimap<Token::Kind, sema::Func::ID>& prefix_overloads,
				TermInfo& expr,
				const AST::Prefix& ast_prefix,
				SymbolProc::TermInfoID output
			) -> Result;

			EVO_NODISCARD auto comptime_infix_math(
				Token::Kind op, sema::Expr lhs, sema::Expr rhs, std::optional<TypeInfo::ID> lhs_type
			) -> TermInfo;


			template<bool IS_COMPTIME>
			EVO_NODISCARD auto union_designated_init_new(
				const Instruction::DesignatedInitNew<IS_COMPTIME>& instr, TypeInfo::ID target_type_info_id
			) -> Result;



			EVO_NODISCARD auto impl_instr_interface_impl_def_non_deducer(const Instruction::InterfaceImplDef& instr)
				-> Result;
			EVO_NODISCARD auto impl_instr_interface_impl_def_deducer() -> Result;


			///////////////////////////////////
			// attributes

			struct GlobalVarAttrs{
				bool is_pub;
				bool is_priv;
				bool is_global;
			};
			EVO_NODISCARD auto analyze_global_var_attrs(
				const AST::VarDef& var_def, evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
			) -> evo::Result<GlobalVarAttrs>;


			struct VarAttrs{
				bool is_global;
			};
			EVO_NODISCARD auto analyze_var_attrs(
				const AST::VarDef& var_def, evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
			) -> evo::Result<VarAttrs>;


			struct FuncAliasAttrs{
				bool is_pub;
				bool is_priv;
			};
			EVO_NODISCARD auto analyze_func_alias_attrs(
				const AST::FuncAliasDef& func_alias_def,
				evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
			) -> evo::Result<FuncAliasAttrs>;


			struct AliasAttrs{
				bool is_pub;
				bool is_priv;
				bool is_distinct;
			};
			EVO_NODISCARD auto analyze_alias_attrs(
				const AST::AliasDef& alias_def, evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
			) -> evo::Result<AliasAttrs>;

			struct LocalAliasAttrs{
				bool is_distinct;
			};
			EVO_NODISCARD auto analyze_local_alias_attrs(
				const AST::AliasDef& alias_def, evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
			) -> evo::Result<LocalAliasAttrs>;


			struct StructAttrs{
				bool is_pub;
				bool is_priv;
				bool is_ordered;
				bool is_packed;
			};
			EVO_NODISCARD auto analyze_struct_attrs(
				const AST::StructDef& struct_def, evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
			) -> evo::Result<StructAttrs>;


			struct UnionAttrs{
				bool is_pub;
				bool is_priv;
				bool is_untagged;
			};
			EVO_NODISCARD auto analyze_union_attrs(
				const AST::UnionDef& union_def,
				evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
			) -> evo::Result<UnionAttrs>;


			struct EnumAttrs{
				bool is_pub;
				bool is_priv;
			};
			EVO_NODISCARD auto analyze_enum_attrs(
				const AST::EnumDef& enum_def,
				evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
			) -> evo::Result<EnumAttrs>;


			struct FuncAttrs{
				bool is_pub;
				bool is_priv;
				bool is_runtime;
				bool is_unsafe;
				bool is_export;
				bool is_no_return;
				bool is_entry;
				bool is_commutative;
				bool is_swapped;
				bool is_implicit;
			};
			EVO_NODISCARD auto analyze_func_attrs(
				const AST::FuncDef& func_def, evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
			) -> evo::Result<FuncAttrs>;



			struct InterfaceAttrs{
				bool is_pub;
				bool is_priv;
				bool is_polymorphic;
			};
			EVO_NODISCARD auto analyze_interface_attrs(
				const AST::InterfaceDef& interface_def,
				evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
			) -> evo::Result<InterfaceAttrs>;



			struct SwitchAttrs{
				bool is_partial;
			};
			EVO_NODISCARD auto analyze_switch_attrs(
				const AST::Switch& switch_stmt,
				evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
			) -> evo::Result<SwitchAttrs>;


			///////////////////////////////////
			// propogate finished

			auto propagate_finished_impl(
				evo::SmallVector<SymbolProc::ID>& waited_on_by_list,
				evo::SmallVector<SymbolProc::ID, 256>& symbol_procs_to_put_in_work_queue
			) -> void;

			auto propagate_finished_decl() -> void;
			auto propagate_finished_def() -> void;
			auto propagate_finished_decl_def() -> void;
			auto propagate_finished_pir_decl() -> void;
			auto propagate_finished_pir_def() -> void;

			auto put_propogated_symbol_procs_into_work_queue(evo::ArrayProxy<SymbolProc::ID> symbol_procs) -> void;




			///////////////////////////////////
			// exec value gets / returns

			auto get_type(SymbolProc::TypeID symbol_proc_type_id) -> TypeInfo::VoidableID;
			auto return_type(SymbolProc::TypeID symbol_proc_type_id, TypeInfo::VoidableID&& id) -> void;

			auto get_term_info(SymbolProc::TermInfoID symbol_proc_term_info_id) -> TermInfo&;
			auto return_term_info(SymbolProc::TermInfoID symbol_proc_term_info_id, auto&&... args) -> void;

			auto get_struct_instantiation(SymbolProc::StructInstantiationID instantiation_id)
				-> const SymbolProc::StructInstantiationInfo&;
			auto return_struct_instantiation(
				SymbolProc::StructInstantiationID instantiation_id,
				const BaseType::StructTemplate::Instantiation& instantiation,
				bool requires_pub
			) -> void;
			auto return_struct_instantiation(
				SymbolProc::StructInstantiationID instantiation_id, BaseType::StructTemplateDeducer::ID deducer_id
			) -> void;



			///////////////////////////////////
			// error handling / diagnostics

			struct TypeCheckInfo{
				bool ok;
				bool requires_implicit_conversion; // value is undefined if .ok == false

				evo::SmallVector<DeducerMatchOutput::DeducedTerm> deduced_terms;
				std::optional<Result> special_result_from_interface_match; // only set if should be checked

				struct InitAssignFunc{ sema::Func::ID func_id; };
				struct AssignFunc{ sema::Func::ID func_id; };
				using NonAutoImplicitConversionTarget = evo::Variant<std::monostate, InitAssignFunc, AssignFunc>;
				NonAutoImplicitConversionTarget non_auto_implicit_conversion_target;

				#if defined(PCIT_CONFIG_DEBUG)
					~TypeCheckInfo(){
						evo::debugAssert(
							this->special_result_from_interface_match.has_value() == false,
							"If a special result from interface match has value, it should be checked and cleared"
						);
						evo::debugAssert(
							this->non_auto_implicit_conversion_target.is<std::monostate>(),
							"If a non auto implicit conversion target has value, handle_non_auto_implicit_conversion()"
						);
					}
				#endif

				public:
					EVO_NODISCARD static auto fail() -> TypeCheckInfo {
						return TypeCheckInfo(false, false, {}, std::nullopt, std::monostate());
					}
					EVO_NODISCARD static auto fail(Result result) -> TypeCheckInfo {
						return TypeCheckInfo(false, false, {}, result, std::monostate());
					}

					EVO_NODISCARD static auto success(bool requires_implicit_conversion) -> TypeCheckInfo {
						return TypeCheckInfo(true, requires_implicit_conversion, {}, std::nullopt, std::monostate());
					}
					EVO_NODISCARD static auto success(
						bool requires_implicit_conversion,
						evo::SmallVector<DeducerMatchOutput::DeducedTerm>&& deduced_terms
					) -> TypeCheckInfo {
						return TypeCheckInfo(
							true, requires_implicit_conversion, std::move(deduced_terms), std::nullopt, std::monostate()
						);
					}
					EVO_NODISCARD static auto success(InitAssignFunc init_func) -> TypeCheckInfo {
						return TypeCheckInfo(true, true, {}, std::nullopt, init_func);
					}
					EVO_NODISCARD static auto success(AssignFunc assign_func) -> TypeCheckInfo {
						return TypeCheckInfo(true, true, {}, std::nullopt, assign_func);
					}

					auto extractSpecialResultFromInterfaceMatch() -> Result {
						const Result output = *this->special_result_from_interface_match;
						this->special_result_from_interface_match.reset();
						return output;
					}

				private:
					TypeCheckInfo(
						bool _ok,
						bool _requires_implicit_conversion,
						evo::SmallVector<DeducerMatchOutput::DeducedTerm>&& _deduced_terms,
						std::optional<Result> _special_result_from_interface_match,
						evo::Variant<std::monostate, InitAssignFunc, AssignFunc> _non_auto_implicit_conversion_target
					) : ok(_ok),
						requires_implicit_conversion(_requires_implicit_conversion),
						deduced_terms(std::move(_deduced_terms)),
						special_result_from_interface_match(_special_result_from_interface_match),
						non_auto_implicit_conversion_target(_non_auto_implicit_conversion_target)
					{}
			};


			auto type_qualifiers_check(
				evo::ArrayProxy<TypeInfo::Qualifier> expected_qualifiers,
				evo::ArrayProxy<TypeInfo::Qualifier> got_qualifiers
			) -> evo::Result<bool>; // bool is if is implicit conversion to optional


			template<bool MAY_DO_IMPLICIT_CONVERSION, bool MAY_EMIT_ERROR>
			EVO_NODISCARD auto type_check(
				TypeInfo::ID expected_type_id,
				TermInfo& got_expr,
				std::string_view expected_type_location_name,
				const auto& location,
				bool is_initialization = true,
				std::optional<unsigned> multi_type_index = std::nullopt
			) -> TypeCheckInfo;


			EVO_NODISCARD auto handle_non_auto_implicit_conversion(
				TypeCheckInfo::NonAutoImplicitConversionTarget& non_auto_implicit_conversion_target,
				const TermInfo& lhs,
				sema::Expr value,
				const AST::Infix& infix
			) -> Result;


			auto error_type_mismatch(
				TypeInfo::ID expected_type_id,
				const TermInfo& got_expr,
				std::string_view expected_type_location_name,
				const auto& location,
				std::optional<unsigned> multi_type_index = std::nullopt
			) -> void;

			auto diagnostic_print_type_info(
				TypeInfo::VoidableID type_id,
				evo::SmallVector<Diagnostic::Info>& infos,
				std::string_view message
			) const -> void;

			auto diagnostic_print_type_info(
				const TermInfo& term_info,
				std::optional<unsigned> multi_type_index,
				evo::SmallVector<Diagnostic::Info>& infos,
				std::string_view message
			) const -> void;


			auto diagnostic_print_type_info_impl(
				TypeInfo::ID type_id,
				evo::SmallVector<Diagnostic::Info>& infos,
				std::string_view message
			) const -> void;



			enum class SpecialMemberFailKind{
				COPY,
				MOVE,
			};

			template<SpecialMemberFailKind MEMBER>
			auto diagnostic_print_special_member_fail(
				TypeInfo::ID type_id, evo::SmallVector<Diagnostic::Info>& infos
			) -> void;



			EVO_NODISCARD auto check_type_qualifiers(
				evo::ArrayProxy<TypeInfo::Qualifier> qualifiers, const auto& location
			) -> evo::Result<>;


			EVO_NODISCARD auto check_term_isnt_type(const TermInfo& term_info, const auto& location) -> evo::Result<>;


			EVO_NODISCARD auto add_ident_to_scope(
				std::string_view ident_str, const auto& ast_node, bool include_shadow_checks, auto&&... ident_id_info
			) -> evo::Result<> {
				return this->add_ident_to_scope(
					this->scope,
					ident_str,
					ast_node,
					include_shadow_checks,
					std::forward<decltype(ident_id_info)>(ident_id_info)...
				);
			}

			EVO_NODISCARD auto add_ident_to_scope(
				sema::ScopeManager::Scope& target_scope,
				std::string_view ident_str,
				const auto& ast_node,
				bool include_shadow_checks,
				auto&&... ident_id_info
			) -> evo::Result<>;


			template<bool IS_SHADOWING>
			auto error_already_defined(
				const auto& redef_id,
				std::string_view ident_str,
				const sema::ScopeLevel::IdentID& first_defined_id,
				sema::Func::ID attempted_decl_func_id
			) -> void {
				this->error_already_defined_impl<IS_SHADOWING>(
					redef_id, ident_str, first_defined_id, attempted_decl_func_id
				);
			}

			template<bool IS_SHADOWING>
			auto error_already_defined(
				const auto& redef_id,
				std::string_view ident_str,
				const sema::ScopeLevel::IdentID& first_defined_id
			) -> void {
				this->error_already_defined_impl<IS_SHADOWING>(redef_id, ident_str, first_defined_id, std::nullopt);
			}

			template<bool IS_SHADOWING>
			auto error_already_defined_impl(
				const auto& redef_id,
				std::string_view ident_str,
				const sema::ScopeLevel::IdentID& first_defined_id,
				std::optional<sema::Func::ID> attempted_decl_func_id
			) -> void;


			EVO_NODISCARD auto print_term_type(
				const TermInfo& term_info, std::optional<unsigned> multi_type_index = std::nullopt
			) const -> std::string;


			EVO_NODISCARD auto check_scope_isnt_terminated(const auto& location) -> evo::Result<>;


			auto add_instantiation_locations_to_infos(evo::SmallVector<Diagnostic::Info>& infos) -> void {
				if(this->symbol_proc.instantiation_locations.empty() == false){
					auto sub_infos = evo::SmallVector<Diagnostic::Info>();
					sub_infos.reserve(this->symbol_proc.instantiation_locations.size());

					for(
						size_t i = this->symbol_proc.instantiation_locations.size() - 1;
						const Diagnostic::Location& instantiation_location
						: this->symbol_proc.instantiation_locations | std::views::reverse
					){
						sub_infos.emplace_back(std::format("Instantiation depth {}:", i), instantiation_location);
						i -= 1;
					}

					infos.emplace_back(
						"Template instantiation context:", Diagnostic::Location::NONE, std::move(sub_infos)
					);
				}
			}


			auto emit_fatal(
				Diagnostic::Code code,
				const auto& node,
				std::string&& string,
				evo::SmallVector<Diagnostic::Info>&& infos
			) -> void {
				this->add_instantiation_locations_to_infos(infos);
				this->context.emitFatal(code, this->get_location(node), std::move(string), std::move(infos));
			}
			auto emit_fatal(
				Diagnostic::Code code,
				const auto& node,
				std::string&& string,
				Diagnostic::Info info
			) -> void {
				this->emit_fatal(code, node, std::move(string), evo::SmallVector<Diagnostic::Info>{info});
			}
			auto emit_fatal(
				Diagnostic::Code code,
				const auto& node,
				std::string&& string
			) -> void {
				this->emit_fatal(code, node, std::move(string), evo::SmallVector<Diagnostic::Info>{});
			}


			auto emit_error(
				Diagnostic::Code code,
				const auto& node,
				std::string&& string,
				evo::SmallVector<Diagnostic::Info>&& infos
			) -> void {
				this->add_instantiation_locations_to_infos(infos);
				this->context.emitError(code, this->get_location(node), std::move(string), std::move(infos));
			}
			auto emit_error(
				Diagnostic::Code code,
				const auto& node,
				std::string&& string,
				Diagnostic::Info info
			) -> void {
				this->emit_error(code, node, std::move(string), evo::SmallVector<Diagnostic::Info>{info});
			}
			auto emit_error(
				Diagnostic::Code code,
				const auto& node,
				std::string&& string
			) -> void {
				this->emit_error(code, node, std::move(string), evo::SmallVector<Diagnostic::Info>{});
			}


			auto emit_warning(
				Diagnostic::Code code,
				const auto& node,
				std::string&& string,
				evo::SmallVector<Diagnostic::Info>&& infos
			) -> void {
				this->add_instantiation_locations_to_infos(infos);
				this->context.emitWarning(code, this->get_location(node), std::move(string), std::move(infos));
			}
			auto emit_warning(
				Diagnostic::Code code,
				const auto& node,
				std::string&& string,
				Diagnostic::Info info
			) -> void {
				this->emit_warning(code, node, std::move(string), evo::SmallVector<Diagnostic::Info>{info});
			}
			auto emit_warning(
				Diagnostic::Code code,
				const auto& node,
				std::string&& string
			) -> void {
				this->emit_warning(code, node, std::move(string), evo::SmallVector<Diagnostic::Info>{});
			}



			///////////////////////////////////
			// get location

			EVO_NODISCARD auto get_location(Diagnostic::Location::None) const -> Diagnostic::Location {
				return Diagnostic::Location::NONE;
			}

			EVO_NODISCARD auto get_location(const Diagnostic::Location& location) const -> Diagnostic::Location {
				return location;
			}

			EVO_NODISCARD auto get_location(const sema::ScopeLevel::ModuleInfo& module_info) const
			-> Diagnostic::Location {
				return this->get_location(module_info.tokenID);
			}

			EVO_NODISCARD auto get_location(const sema::ScopeLevel::ClangModuleInfo& module_info) const
			-> Diagnostic::Location {
				return this->get_location(module_info.tokenID);
			}

			EVO_NODISCARD auto get_location(const sema::ScopeLevel::TemplateTypeParam& template_type_param) const
			-> Diagnostic::Location {
				return this->get_location(template_type_param.location);
			}

			EVO_NODISCARD auto get_location(const sema::ScopeLevel::TemplateExprParam& template_expr_param) const
			-> Diagnostic::Location {
				return this->get_location(template_expr_param.location);
			}

			EVO_NODISCARD auto get_location(const sema::ScopeLevel::DeducedType& deduced_type) const
			-> Diagnostic::Location {
				return this->get_location(deduced_type.location);
			}

			EVO_NODISCARD auto get_location(const sema::ScopeLevel::DeducedExpr& deduced_expr) const
			-> Diagnostic::Location {
				return this->get_location(deduced_expr.location);
			}

			EVO_NODISCARD auto get_location(const sema::ScopeLevel::MemberVar& member_var) const
			-> Diagnostic::Location {
				return this->get_location(member_var.location);
			}

			EVO_NODISCARD auto get_location(const sema::ScopeLevel::UnionField& union_field) const
			-> Diagnostic::Location {
				return this->get_location(union_field.location);
			}

			EVO_NODISCARD auto get_location(const sema::ScopeLevel::ForUnrollIndex& for_unroll_index) const
			-> Diagnostic::Location {
				return this->get_location(for_unroll_index.location);
			}

			EVO_NODISCARD auto get_location(sema::Func::ID func_id) const -> Diagnostic::Location {
				return Diagnostic::Location::get(func_id, this->context);
			}

			EVO_NODISCARD auto get_location(const sema::Func& func) const -> Diagnostic::Location {
				return Diagnostic::Location::get(func, this->context);
			}

			EVO_NODISCARD auto get_location(sema::FuncAlias::ID func_alias_id) const -> Diagnostic::Location {
				return Diagnostic::Location::get(func_alias_id, this->context);
			}

			EVO_NODISCARD auto get_location(const sema::FuncAlias& func_alias) const -> Diagnostic::Location {
				return Diagnostic::Location::get(func_alias, this->context);
			}

			EVO_NODISCARD auto get_location(const sema::TemplatedFuncID& templated_func) const -> Diagnostic::Location {
				return Diagnostic::Location::get(templated_func, this->context);
			}

			EVO_NODISCARD auto get_location(const sema::GlobalVarID& var) const -> Diagnostic::Location {
				return Diagnostic::Location::get(var, this->context);
			}

			EVO_NODISCARD auto get_location(const sema::VarID& var) const -> Diagnostic::Location {
				return Diagnostic::Location::get(var, this->source, this->context);
			}

			EVO_NODISCARD auto get_location(sema::ParamID param_id) const -> Diagnostic::Location {
				const sema::Param& param = this->context.getSemaBuffer().getParam(param_id);

				const AST::FuncDef& current_ast_func =
					this->source.getASTBuffer().getFuncDef(this->symbol_proc.ast_node);

				return this->get_location(current_ast_func.params[param.index].name);
			}

			EVO_NODISCARD auto get_location(sema::VariadicParamID param_id) const -> Diagnostic::Location {
				const sema::VariadicParam& param = this->context.getSemaBuffer().getVariadicParam(param_id);

				const AST::FuncDef& current_ast_func =
					this->source.getASTBuffer().getFuncDef(this->symbol_proc.ast_node);

				return this->get_location(current_ast_func.params[param.startIndex].name);
			}

			EVO_NODISCARD auto get_location(sema::ScopeLevel::ExtractedVariadicParam extracted_variadic_param) const
			-> Diagnostic::Location {
				return this->get_location(extracted_variadic_param.param_id);
			}

			EVO_NODISCARD auto get_location(sema::ReturnParam::ID return_param_id) const -> Diagnostic::Location {
				const sema::ReturnParam& return_param = this->context.getSemaBuffer().getReturnParam(return_param_id);

				return this->get_location(this->get_current_func().returnParamIdents[return_param.index]);
			}

			EVO_NODISCARD auto get_location(sema::ErrorReturnParam::ID error_param_id) const -> Diagnostic::Location {
				const sema::ErrorReturnParam& error_param =
					this->context.getSemaBuffer().getErrorReturnParam(error_param_id);

				return this->get_location(this->get_current_func().errorParamIdents[error_param.index]);
			}

			EVO_NODISCARD auto get_location(sema::BlockExprOutput::ID block_expr_output_id) const
			-> Diagnostic::Location {
				const sema::BlockExprOutput& block_expr_output =
					this->context.getSemaBuffer().getBlockExprOutput(block_expr_output_id);
				return this->get_location(block_expr_output.ident);
			}

			EVO_NODISCARD auto get_location(sema::ExceptParamID except_param_id) const -> Diagnostic::Location {
				const sema::ExceptParam& except_param = this->context.getSemaBuffer().getExceptParam(except_param_id);
				return this->get_location(except_param.ident);
			}

			EVO_NODISCARD auto get_location(sema::ForParamID for_param_id) const -> Diagnostic::Location {
				const sema::ForParam& for_param = this->context.getSemaBuffer().getForParam(for_param_id);
				return this->get_location(for_param.ident);
			}


			EVO_NODISCARD auto get_location(BaseType::Alias::ID alias_id) const -> Diagnostic::Location {
				return Diagnostic::Location::get(alias_id, this->context);
			}
			EVO_NODISCARD auto get_location(const BaseType::Alias& alias_type) const -> Diagnostic::Location {
				return Diagnostic::Location::get(alias_type, this->context);
			}

			EVO_NODISCARD auto get_location(BaseType::DistinctAlias::ID distinct_alias_id) const
			-> Diagnostic::Location {
				return Diagnostic::Location::get(distinct_alias_id, this->context);
			}
			EVO_NODISCARD auto get_location(const BaseType::DistinctAlias& distinct_alias_type) const
			-> Diagnostic::Location {
				return Diagnostic::Location::get(distinct_alias_type, this->context);
			}

			EVO_NODISCARD auto get_location(BaseType::Struct::ID struct_id) const -> Diagnostic::Location {
				return Diagnostic::Location::get(struct_id, this->context);
			}
			EVO_NODISCARD auto get_location(const BaseType::Struct& struct_type) const -> Diagnostic::Location {
				return Diagnostic::Location::get(struct_type, this->context);
			}

			EVO_NODISCARD auto get_location(BaseType::Union::ID union_id) const -> Diagnostic::Location {
				return Diagnostic::Location::get(union_id, this->context);
			}
			EVO_NODISCARD auto get_location(const BaseType::Union& union_type) const -> Diagnostic::Location {
				return Diagnostic::Location::get(union_type, this->context);
			}

			EVO_NODISCARD auto get_location(BaseType::Enum::ID enum_id) const -> Diagnostic::Location {
				return Diagnostic::Location::get(enum_id, this->context);
			}
			EVO_NODISCARD auto get_location(const BaseType::Enum& enum_type) const -> Diagnostic::Location {
				return Diagnostic::Location::get(enum_type, this->context);
			}

			EVO_NODISCARD auto get_location(BaseType::Interface::ID interface_id) const -> Diagnostic::Location {
				return Diagnostic::Location::get(interface_id, this->context);
			}

			EVO_NODISCARD auto get_location(const BaseType::Interface& interface_type) const -> Diagnostic::Location {
				return Diagnostic::Location::get(interface_type, this->context);
			}

			EVO_NODISCARD auto get_location(sema::TemplatedStruct::ID templated_struct_id) const
			-> Diagnostic::Location {
				const sema::TemplatedStruct& templated_struct =
					this->context.sema_buffer.getTemplatedStruct(templated_struct_id);

				return Diagnostic::Location::get(
					templated_struct.symbolProc.ast_node,
					this->context.getSourceManager()[templated_struct.symbolProc.source_id]
				);
			}

			EVO_NODISCARD auto get_location(sema::StructTemplateAlias::ID struct_template_alias_id) const
			-> Diagnostic::Location {
				const sema::StructTemplateAlias& struct_template_alias =
					this->context.sema_buffer.getStructTemplateAlias(struct_template_alias_id);					

				return Diagnostic::Location::get(
					struct_template_alias.ident,
					this->context.getSourceManager()[struct_template_alias.sourceID]
				);
			}




			EVO_NODISCARD auto get_location(sema::ScopeLevel::ValueStateID value_state_id) const
			-> Diagnostic::Location {
				return value_state_id.visit([&](const auto& id) -> Diagnostic::Location {
					using IDType = std::decay_t<decltype(id)>;

					if constexpr(std::is_same<IDType, sema::Var::ID>()){
						return this->get_location(id);

					}else if constexpr(std::is_same<IDType, sema::Param::ID>()){
						return this->get_location(id);

					}else if constexpr(std::is_same<IDType, sema::ReturnParam::ID>()){
						return this->get_location(id);

					}else if constexpr(std::is_same<IDType, sema::ErrorReturnParam::ID>()){
						return this->get_location(id);

					}else if constexpr(std::is_same<IDType, sema::BlockExprOutput::ID>()){
						return this->get_location(id);

					}else if constexpr(std::is_same<IDType, sema::ExceptParam::ID>()){
						return this->get_location(id);

					}else if constexpr(std::is_same<IDType, sema::ForParam::ID>()){
						return this->get_location(id);

					}else if constexpr(std::is_same<IDType, sema::ReturnParamAccessorValueStateID>()){
						return this->get_location(id.id);

					}else if constexpr(std::is_same<IDType, sema::OpDeleteThisAccessorValueStateID>()){
						return this->get_location(this->get_current_func().params[0].ident.as<Token::ID>());

					}else if constexpr(std::is_same<IDType, sema::UninitPtrLocalVar>()){
						return this->get_location(id.varID);

					}else{
						static_assert(false, "Unknown value state ID");
					}
				});
			}


			EVO_NODISCARD auto get_location(const auto& node) const -> Diagnostic::Location {
				return Diagnostic::Location:: get(node, this->source);
			}


			///////////////////////////////////
			// constructor

			SemanticAnalyzer(
				Context& _context,
				Source& _source,
				SymbolProc::ID sym_proc_id,
				SymbolProc& sym_proc,
				sema::ScopeManager::Scope& _scope
			) : context(_context),
				source(_source),
				symbol_proc_id(sym_proc_id),
				symbol_proc(sym_proc),
				scope(_scope)
			{}

		private:
			Context& context;
			Source& source;
			SymbolProc::ID symbol_proc_id;
			SymbolProc& symbol_proc;
			sema::ScopeManager::Scope& scope;


			friend class Attribute;
			friend class ConditionalAttribute;
	};



	inline auto analyze_semantics(Context& context, SymbolProc::ID symbol_proc_id) -> void {
		return SemanticAnalyzer::create(context, symbol_proc_id).analyze();
	}


}
