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
				FINISHED_EARLY,
				ERROR,
				RECOVERABLE_ERROR,
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
			EVO_NODISCARD auto instr_alias_decl(const Instruction::AliasDecl& instr) -> Result;
			EVO_NODISCARD auto instr_alias_def(const Instruction::AliasDef& instr) -> Result;

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

			EVO_NODISCARD auto instr_func_decl_extract_deducers_if_needed(
				const Instruction::FuncDeclExtractDeducersIfNeeded& instr
			) -> Result;
			template<bool IS_INSTANTIATION>
			EVO_NODISCARD auto instr_func_decl(const Instruction::FuncDecl<IS_INSTANTIATION>& instr) -> Result;
			EVO_NODISCARD auto instr_func_pre_body(const Instruction::FuncPreBody& instr) -> Result;
			EVO_NODISCARD auto instr_func_def(const Instruction::FuncDef& instr) -> Result;
			EVO_NODISCARD auto instr_func_prepare_constexpr_pir_if_needed(
				const Instruction::FuncPrepareConstexprPIRIfNeeded& instr
			) -> Result;
			EVO_NODISCARD auto instr_func_constexpr_pir_ready_if_needed() -> Result;

			EVO_NODISCARD auto instr_template_func_begin(const Instruction::TemplateFuncBegin& instr) -> Result;
			EVO_NODISCARD auto instr_template_func_check_param_is_interface(
				const Instruction::TemplateFuncCheckParamIsInterface& instr
			) -> Result;
			EVO_NODISCARD auto instr_template_set_param_is_deducer(
				const Instruction::TemplateFuncSetParamIsDeducer& instr
			) -> Result;
			EVO_NODISCARD auto instr_template_func_end(const Instruction::TemplateFuncEnd& instr) -> Result;
			EVO_NODISCARD auto instr_deleted_special_method(const Instruction::DeletedSpecialMethod& instr) -> Result;

			EVO_NODISCARD auto instr_interface_decl(const Instruction::InterfaceDecl& instr) -> Result;
			EVO_NODISCARD auto instr_interface_def() -> Result;
			EVO_NODISCARD auto instr_interface_func_def(const Instruction::InterfaceFuncDef& instr) -> Result;
			EVO_NODISCARD auto instr_interface_impl_decl(const Instruction::InterfaceImplDecl& instr) -> Result;
			EVO_NODISCARD auto instr_interface_impl_method_lookup(const Instruction::InterfaceImplMethodLookup& instr)
				-> Result;
			EVO_NODISCARD auto instr_interface_impl_def(const Instruction::InterfaceImplDef& instr) -> Result;
			EVO_NODISCARD auto instr_interface_impl_constexpr_pir() -> Result;



			EVO_NODISCARD auto instr_local_var(const Instruction::LocalVar& instr) -> Result;
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
			EVO_NODISCARD auto instr_end_while() -> Result;
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


			EVO_NODISCARD auto instr_type_to_term(const Instruction::TypeToTerm& instr) -> Result;
			EVO_NODISCARD auto instr_require_this_def() -> Result;
			EVO_NODISCARD auto instr_wait_on_sub_symbol_proc_def(const Instruction::WaitOnSubSymbolProcDef& instr)
				-> Result;

			template<bool IS_CONSTEXPR, bool ERRORS>
			EVO_NODISCARD auto instr_func_call_expr(const Instruction::FuncCallExpr<IS_CONSTEXPR, ERRORS>& instr)
				-> Result;

			EVO_NODISCARD auto builtin_type_method_call(
				const TermInfo& target_term_info, evo::SmallVector<sema::Expr>&& args, SymbolProc::TermInfoID output
			) -> Result;

			template<bool IS_CONSTEXPR>
			EVO_NODISCARD auto interface_func_call(
				const TermInfo& target_term_info,
				evo::SmallVector<sema::Expr>&& args,
				sema::Func::ID selected_func_call_id,
				SymbolProc::TermInfoID output
			) -> Result;

			EVO_NODISCARD auto instr_constexpr_func_call_run(const Instruction::ConstexprFuncCallRun& instr) -> Result;

			template<Instruction::Language LANGUAGE>
			EVO_NODISCARD auto instr_import(const Instruction::Import<LANGUAGE>& instr) -> Result;

			EVO_NODISCARD auto instr_is_macro_defined(const Instruction::IsMacroDefined& instr) -> Result;

			template<bool IS_CONSTEXPR>
			EVO_NODISCARD auto instr_template_intrinsic_func_call(
				const Instruction::TemplateIntrinsicFuncCall<IS_CONSTEXPR>& instr
			) -> Result;

			template<bool IS_CONSTEXPR>
			EVO_NODISCARD auto instr_indexer(const Instruction::Indexer<IS_CONSTEXPR>& instr) -> Result;

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

			template<bool IS_CONSTEXPR>
			EVO_NODISCARD auto instr_prefix_negate(const Instruction::PrefixNegate<IS_CONSTEXPR>& instr) -> Result;

			template<bool IS_CONSTEXPR>
			EVO_NODISCARD auto instr_prefix_not(const Instruction::PrefixNot<IS_CONSTEXPR>& instr) -> Result;
			
			template<bool IS_CONSTEXPR>
			EVO_NODISCARD auto instr_prefix_bitwise_not(const Instruction::PrefixBitwiseNot<IS_CONSTEXPR>& instr)
				-> Result;

			EVO_NODISCARD auto instr_deref(const Instruction::Deref& instr) -> Result;
			EVO_NODISCARD auto instr_unwrap(const Instruction::Unwrap& instr) -> Result;

			template<bool IS_CONSTEXPR>
			EVO_NODISCARD auto instr_new(const Instruction::New<IS_CONSTEXPR>& instr) -> Result;

			template<bool IS_CONSTEXPR>
			EVO_NODISCARD auto instr_array_init_new(const Instruction::ArrayInitNew<IS_CONSTEXPR>& instr) -> Result;

			template<bool IS_CONSTEXPR>
			EVO_NODISCARD auto instr_designated_init_new(const Instruction::DesignatedInitNew<IS_CONSTEXPR>& instr)
				-> Result;

			EVO_NODISCARD auto instr_prepare_try_handler(const Instruction::PrepareTryHandler& instr) -> Result;
			EVO_NODISCARD auto instr_try_else_expr(const Instruction::TryElseExpr& instr) -> Result;
			EVO_NODISCARD auto instr_begin_expr_block(const Instruction::BeginExprBlock& instr) -> Result;
			EVO_NODISCARD auto instr_end_expr_block(const Instruction::EndExprBlock& instr) -> Result;

			template<bool IS_CONSTEXPR>
			EVO_NODISCARD auto instr_expr_as(const Instruction::As<IS_CONSTEXPR>& instr) -> Result;

			EVO_NODISCARD auto instr_optional_null_check(const Instruction::OptionalNullCheck& instr) -> Result;			

			template<bool IS_CONSTEXPR>
			EVO_NODISCARD auto operator_as_interface_ptr(
				const Instruction::As<IS_CONSTEXPR>& instr,
				const TermInfo& from_expr,
				const TypeInfo& from_type_info,
				const TypeInfo& to_type_info
			) -> Result;


			template<bool IS_CONSTEXPR, Instruction::MathInfixKind MATH_INFIX_KIND>
			EVO_NODISCARD auto instr_expr_math_infix(const Instruction::MathInfix<IS_CONSTEXPR, MATH_INFIX_KIND>& instr)
				-> Result;

			template<bool IS_CONSTEXPR>
			EVO_NODISCARD auto instr_expr_accessor(const Instruction::Accessor<IS_CONSTEXPR>& instr) -> Result;

			template<bool NEEDS_DEF>
			EVO_NODISCARD auto instr_primitive_type(const Instruction::PrimitiveType& instr) -> Result;

			EVO_NODISCARD auto instr_array_type(const Instruction::ArrayType& instr) -> Result;
			EVO_NODISCARD auto instr_array_ref(const Instruction::ArrayRef& instr) -> Result;
			EVO_NODISCARD auto instr_type_id_converter(const Instruction::TypeIDConverter& instr) -> Result;
			EVO_NODISCARD auto instr_user_type(const Instruction::UserType& instr) -> Result;
			EVO_NODISCARD auto instr_base_type_ident(const Instruction::BaseTypeIdent& instr) -> Result;

			template<bool NEEDS_DEF>
			EVO_NODISCARD auto instr_ident(const Instruction::Ident<NEEDS_DEF>& instr) -> Result;

			EVO_NODISCARD auto instr_intrinsic(const Instruction::Intrinsic& instr) -> Result;
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
				TypeInfo::ID actual_lhs_type_id,
				const TypeInfo& actual_lhs_type,
				bool is_pointer
			) -> Result;


			template<bool NEEDS_DEF>
			EVO_NODISCARD auto optional_accessor(
				const Instruction::Accessor<NEEDS_DEF>& instr,
				std::string_view rhs_ident_str,
				const TermInfo& lhs,
				TypeInfo::ID actual_lhs_type_id,
				const TypeInfo& actual_lhs_type,
				bool is_pointer
			) -> Result;


			template<bool NEEDS_DEF>
			EVO_NODISCARD auto struct_accessor(
				const Instruction::Accessor<NEEDS_DEF>& instr,
				std::string_view rhs_ident_str,
				const TermInfo& lhs,
				TypeInfo::ID actual_lhs_type_id,
				const TypeInfo& actual_lhs_type,
				bool is_pointer
			) -> Result;

			template<bool NEEDS_DEF>
			EVO_NODISCARD auto union_accessor(
				const Instruction::Accessor<NEEDS_DEF>& instr,
				std::string_view rhs_ident_str,
				const TermInfo& lhs,
				TypeInfo::ID actual_lhs_type_id,
				const TypeInfo& actual_lhs_type,
				bool is_pointer
			) -> Result;

			template<bool NEEDS_DEF>
			EVO_NODISCARD auto enum_accessor(
				const Instruction::Accessor<NEEDS_DEF>& instr,
				std::string_view rhs_ident_str,
				const TermInfo& lhs,
				TypeInfo::ID actual_lhs_type_id,
				const TypeInfo& actual_lhs_type,
				bool is_pointer
			) -> Result;

			template<bool NEEDS_DEF>
			EVO_NODISCARD auto array_accessor(
				const Instruction::Accessor<NEEDS_DEF>& instr,
				std::string_view rhs_ident_str,
				const TermInfo& lhs,
				TypeInfo::ID actual_lhs_type_id,
				const TypeInfo& actual_lhs_type,
				bool is_pointer
			) -> Result;

			template<bool NEEDS_DEF>
			EVO_NODISCARD auto array_ref_accessor(
				const Instruction::Accessor<NEEDS_DEF>& instr,
				std::string_view rhs_ident_str,
				const TermInfo& lhs,
				TypeInfo::ID actual_lhs_type_id,
				const TypeInfo& actual_lhs_type,
				bool is_pointer
			) -> Result;


			///////////////////////////////////
			// scope

			EVO_NODISCARD auto get_current_scope_level() const -> sema::ScopeLevel&;
			EVO_NODISCARD auto push_scope_level(sema::StmtBlock* stmt_block = nullptr) -> void;
			EVO_NODISCARD auto push_scope_level(
				sema::StmtBlock& stmt_block, Token::ID label, sema::ScopeLevel::LabelNode label_node
			) -> void;
			EVO_NODISCARD auto push_scope_level(sema::StmtBlock* stmt_block, const auto& object_scope_id) -> void;

			enum class PopScopeLevelKind{
				LABEL_TERMINATE,
				NORMAL,
				SYMBOL_END,
			};
			template<PopScopeLevelKind POP_SCOPE_LEVEL_KIND = PopScopeLevelKind::NORMAL>
			EVO_NODISCARD auto pop_scope_level() -> evo::Result<>;

			enum class AutoDeleteMode{
				NORMAL,
				RETURN,
				ERROR,
			};
			template<AutoDeleteMode AUTO_DELETE_MODE>
			auto add_auto_delete_calls() -> evo::Result<>;


			enum class SpecialMemberKind{
				DELETE,
				COPY,
				MOVE,
			};
			template<SpecialMemberKind SPECIAL_MEMBER_KIND>
			EVO_NODISCARD auto get_special_member_stmt_dependents_and_check_constexpr(
				TypeInfo::ID type_info_id,
				std::unordered_set<sema::Func::ID>& dependent_funcs,
				const auto& location
			) ->evo::Result<>;


			EVO_NODISCARD auto currently_in_func() const -> bool;

			EVO_NODISCARD auto get_current_func() -> sema::Func&;
			EVO_NODISCARD auto get_current_func() const -> const sema::Func&;


			auto end_sub_scopes(Diagnostic::Location&& location) -> evo::Result<>;

			auto end_sub_scopes(const Diagnostic::Location& location) -> evo::Result<> {
				return this->end_sub_scopes(evo::copy(location));
			}



			//////////////////
			// value states

			EVO_NODISCARD auto get_ident_value_state(sema::ScopeLevel::ValueStateID value_state_id)
				-> TermInfo::ValueState;

			auto set_ident_value_state(
				sema::ScopeLevel::ValueStateID value_state_id, sema::ScopeLevel::ValueState value_state
			) -> void;

			auto set_ident_value_state_if_needed(sema::Expr target, sema::ScopeLevel::ValueState value_state) -> void;



			///////////////////////////////////
			// misc

			template<bool NEEDS_DEF>
			EVO_NODISCARD auto lookup_ident_impl(Token::ID ident) -> evo::Expected<TermInfo, Result>;


			enum class AnalyzeExprIdentInScopeLevelError{
				DOESNT_EXIST,
				NEEDS_TO_WAIT_ON_DEF,
				ERROR_EMITTED,
			};
			template<bool NEEDS_DEF, bool PUB_REQUIRED>
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

			template<bool LOOK_THROUGH_DISTINCT_ALIAS, bool LOOK_THROUGH_INTERFACE_IMPL_INSTANTIATION>
			EVO_NODISCARD auto get_actual_type(TypeInfo::ID type_id) const -> TypeInfo::ID;



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
			template<bool IS_CONSTEXPR, bool ERRORS>
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
				struct WrongNumArgs{ size_t expected_num; size_t got_num; };
				struct CantDeduceArgType{ size_t arg_index; };

				using Reason = evo::Variant<
					Handled,
					TooFewTemplateArgs,
					TooManyTemplateArgs,
					TemplateArgWrongKind,
					WrongNumArgs,
					CantDeduceArgType
				>;

				Reason reason;
			};

			EVO_NODISCARD auto get_select_func_overload_func_info_for_template(
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


			EVO_NODISCARD auto get_project_config() const -> const Source::ProjectConfig&;


			struct DeducedTerm{
				struct Expr{
					TypeInfo::ID type_id;
					sema::Expr expr;
				};

				evo::Variant<TypeInfo::VoidableID, Expr> value;
				Token::ID tokenID;
			};

			EVO_NODISCARD auto extract_deducers(TypeInfo::VoidableID deducer_id, TypeInfo::VoidableID got_type_id)
				-> evo::Result<evo::SmallVector<DeducedTerm>>;

			EVO_NODISCARD auto extract_deducers(TypeInfo::ID deducer_id, TypeInfo::ID got_type_id)
				-> evo::Result<evo::SmallVector<DeducedTerm>>;

			EVO_NODISCARD auto add_deduced_terms_to_scope(evo::ArrayProxy<DeducedTerm> deduced_terms) -> evo::Result<>;


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

			EVO_NODISCARD auto constexpr_infix_math(
				Token::Kind op, sema::Expr lhs, sema::Expr rhs, std::optional<TypeInfo::ID> lhs_type
			) -> TermInfo;

			EVO_NODISCARD auto type_is_comparable(TypeInfo::ID type_id) -> bool;
			EVO_NODISCARD auto type_is_comparable(const TypeInfo& type_info) -> bool;


			template<bool IS_CONSTEXPR>
			EVO_NODISCARD auto union_designated_init_new(
				const Instruction::DesignatedInitNew<IS_CONSTEXPR>& instr, TypeInfo::ID target_type_info_id
			) -> Result;


			///////////////////////////////////
			// attributes

			struct GlobalVarAttrs{
				bool is_pub;
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


			struct StructAttrs{
				bool is_pub;
				bool is_ordered;
				bool is_packed;
			};
			EVO_NODISCARD auto analyze_struct_attrs(
				const AST::StructDef& struct_def, evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
			) -> evo::Result<StructAttrs>;


			struct UnionAttrs{
				bool is_pub;
				bool is_untagged;
			};
			EVO_NODISCARD auto analyze_union_attrs(
				const AST::UnionDef& union_def,
				evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
			) -> evo::Result<UnionAttrs>;


			struct EnumAttrs{
				bool is_pub;
			};
			EVO_NODISCARD auto analyze_enum_attrs(
				const AST::EnumDef& enum_def,
				evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
			) -> evo::Result<EnumAttrs>;


			struct FuncAttrs{
				bool is_pub;
				bool is_runtime;
				bool is_export;
				bool is_entry;
				bool is_commutative;
				bool is_swapped;
			};
			EVO_NODISCARD auto analyze_func_attrs(
				const AST::FuncDef& func_def, evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
			) -> evo::Result<FuncAttrs>;



			struct InterfaceAttrs{
				bool is_pub;
				bool is_polymorphic;
			};
			EVO_NODISCARD auto analyze_interface_attrs(
				const AST::InterfaceDef& interface_def,
				evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
			) -> evo::Result<InterfaceAttrs>;


			///////////////////////////////////
			// propogate finished

			auto propagate_finished_impl(
				const evo::SmallVector<SymbolProc::ID>& waited_on_by_list,
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
				-> evo::Variant<const BaseType::StructTemplate::Instantiation*, BaseType::StructTemplateDeducer::ID>;
			auto return_struct_instantiation(
				SymbolProc::StructInstantiationID instantiation_id,
				const BaseType::StructTemplate::Instantiation& instantiation
			) -> void;
			auto return_struct_instantiation(
				SymbolProc::StructInstantiationID instantiation_id, BaseType::StructTemplateDeducer::ID deducer_id
			) -> void;



			///////////////////////////////////
			// error handling / diagnostics

			struct TypeCheckInfo{
				bool ok;
				bool requires_implicit_conversion; // value is undefined if .ok == false

				evo::SmallVector<DeducedTerm> deduced_terms;

				public:
					EVO_NODISCARD static auto fail() -> TypeCheckInfo { return TypeCheckInfo(false, false, {}); }
					EVO_NODISCARD static auto success(bool requires_implicit_conversion) -> TypeCheckInfo {
						return TypeCheckInfo(true, requires_implicit_conversion, {});
					}
					EVO_NODISCARD static auto success(
						bool requires_implicit_conversion, evo::SmallVector<DeducedTerm>&& deduced_terms
					) -> TypeCheckInfo {
						return TypeCheckInfo(true, requires_implicit_conversion, std::move(deduced_terms));
					}

				private:
					TypeCheckInfo(bool _ok, bool ric, evo::SmallVector<DeducedTerm>&& _deduced_terms)
						: ok(_ok), requires_implicit_conversion(ric), deduced_terms(std::move(_deduced_terms)) {}
			};

			template<bool MAY_IMPLICITLY_CONVERT, bool MAY_EMIT_ERROR>
			EVO_NODISCARD auto type_check(
				TypeInfo::ID expected_type_id,
				TermInfo& got_expr,
				std::string_view expected_type_location_name,
				const auto& location,
				std::optional<unsigned> multi_type_index = std::nullopt
			) -> TypeCheckInfo;

			auto error_type_mismatch(
				TypeInfo::ID expected_type_id,
				const TermInfo& got_expr,
				std::string_view expected_type_location_name,
				const auto& location,
				std::optional<unsigned> multi_type_index = std::nullopt
			) -> void;

			auto diagnostic_print_type_info(
				TypeInfo::ID type_id,
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



			EVO_NODISCARD auto check_type_qualifiers(
				evo::ArrayProxy<TypeInfo::Qualifier> qualifiers, const auto& location
			) -> evo::Result<>;


			EVO_NODISCARD auto check_term_isnt_type(const TermInfo& term_info, const auto& location) -> evo::Result<>;


			EVO_NODISCARD auto add_ident_to_scope(
				std::string_view ident_str, const auto& ast_node, auto&&... ident_id_info
			) -> evo::Result<> {
				return this->add_ident_to_scope(
					this->scope, ident_str, ast_node, std::forward<decltype(ident_id_info)>(ident_id_info)...
				);
			}

			EVO_NODISCARD auto add_ident_to_scope(
				sema::ScopeManager::Scope& target_scope,
				std::string_view ident_str,
				const auto& ast_node,
				auto&&... ident_id_info
			) -> evo::Result<>;


			template<bool IS_SHADOWING>
			auto error_already_defined(
				const auto& redef_id,
				std::string_view ident_str,
				const sema::ScopeLevel::IdentID& first_defined_id,
				sema::Func::ID attempted_decl_func_id,
				[[maybe_unused]] auto&&... ident_id_info_args // TODO(FUTURE): this is seemingly unused?????
			) -> void {
				this->error_already_defined_impl<IS_SHADOWING>(
					redef_id, ident_str, first_defined_id, attempted_decl_func_id
				);
			}

			template<bool IS_SHADOWING>
			auto error_already_defined(
				const auto& redef_id,
				std::string_view ident_str,
				const sema::ScopeLevel::IdentID& first_defined_id,
				[[maybe_unused]] auto&&... ident_id_info_args // TODO(FUTURE): this is seemingly unused?????
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


			auto emit_fatal(Diagnostic::Code code, const auto& node, auto&&... args) -> void {
				this->context.emitFatal(code, this->get_location(node), std::forward<decltype(args)>(args)...);
			}

			auto emit_error(Diagnostic::Code code, const auto& node, auto&&... args) -> void {
				this->context.emitError(code, this->get_location(node), std::forward<decltype(args)>(args)...);
			}

			auto emit_warning(Diagnostic::Code code, const auto& node, auto&&... args) -> void {
				this->context.emitWarning(code, this->get_location(node), std::forward<decltype(args)>(args)...);
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

			EVO_NODISCARD auto get_location(sema::Func::ID func_id) const -> Diagnostic::Location {
				return Diagnostic::Location::get(func_id, this->context);
			}

			EVO_NODISCARD auto get_location(const sema::Func& func) const -> Diagnostic::Location {
				return Diagnostic::Location::get(func, this->context);
			}

			EVO_NODISCARD auto get_location(const sema::TemplatedFuncID& templated_func) const -> Diagnostic::Location {
				return Diagnostic::Location::get(templated_func, this->source, this->context);
			}

			EVO_NODISCARD auto get_location(const sema::GlobalVarID& var) const -> Diagnostic::Location {
				return Diagnostic::Location::get(var, this->context);
			}

			EVO_NODISCARD auto get_location(const sema::VarID& var) const -> Diagnostic::Location {
				return Diagnostic::Location::get(var, this->source, this->context);
			}

			EVO_NODISCARD auto get_location(const sema::ParamID& param) const -> Diagnostic::Location {
				// TODO(FUTURE): 
				std::ignore = param;
				evo::unimplemented();
			}

			EVO_NODISCARD auto get_location(sema::ReturnParam::ID return_param_id) const -> Diagnostic::Location {
				const sema::ReturnParam& return_param = this->context.getSemaBuffer().getReturnParam(return_param_id);

				const BaseType::Function& current_func_type = 
					this->context.getTypeManager().getFunction(this->get_current_func().typeID);

				return this->get_location(*current_func_type.returnParams[return_param.index].ident);
			}

			EVO_NODISCARD auto get_location(sema::ErrorReturnParam::ID error_param_id) const -> Diagnostic::Location {
				const sema::ErrorReturnParam& error_param =
					this->context.getSemaBuffer().getErrorReturnParam(error_param_id);

				const BaseType::Function& current_func_type = 
					this->context.getTypeManager().getFunction(this->get_current_func().typeID);

				return this->get_location(*current_func_type.errorParams[error_param.index].ident);
			}

			EVO_NODISCARD auto get_location(sema::BlockExprOutput::ID block_expr_output_id) const
			-> Diagnostic::Location {
				const sema::BlockExprOutput& block_expr_output =
					this->context.getSemaBuffer().getBlockExprOutput(block_expr_output_id);

				for(const sema::ScopeLevel::ID target_scope_level_id : this->scope){
					const sema::ScopeLevel& target_scope_level = 
						this->context.sema_buffer.scope_manager.getLevel(target_scope_level_id);

					if(target_scope_level.hasLabel() == false){ continue; }
					if(target_scope_level.getLabelNode().is<sema::BlockExpr::ID>() == false){ continue; }

					const sema::BlockExpr& block_expr = this->context.getSemaBuffer().getBlockExpr(
						target_scope_level.getLabelNode().as<sema::BlockExpr::ID>()
					);

					return this->get_location(*block_expr.outputs[block_expr_output.index].ident);
				}

				evo::debugFatalBreak("Didn't find a scope that is a block expression");
			}

			EVO_NODISCARD auto get_location(const sema::ExceptParamID& except_param) const -> Diagnostic::Location {
				// TODO(FUTURE): 
				std::ignore = except_param;
				evo::unimplemented();
			}


			EVO_NODISCARD auto get_location(BaseType::Alias::ID alias_id) const -> Diagnostic::Location {
				return Diagnostic::Location::get(alias_id, this->context);
			}

			EVO_NODISCARD auto get_location(BaseType::DistinctAlias::ID distinct_alias_id) const
			-> Diagnostic::Location {
				// TODO(FUTURE): 
				std::ignore = distinct_alias_id;
				evo::unimplemented();
			}

			EVO_NODISCARD auto get_location(BaseType::Struct::ID struct_id) const -> Diagnostic::Location {
				return Diagnostic::Location::get(struct_id, this->context);
			}

			EVO_NODISCARD auto get_location(BaseType::Union::ID union_id) const -> Diagnostic::Location {
				return Diagnostic::Location::get(union_id, this->context);
			}

			EVO_NODISCARD auto get_location(BaseType::Enum::ID enum_id) const -> Diagnostic::Location {
				return Diagnostic::Location::get(enum_id, this->context);
			}

			EVO_NODISCARD auto get_location(BaseType::Interface::ID interface_id) const -> Diagnostic::Location {
				return Diagnostic::Location::get(interface_id, this->context);
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

					}else if constexpr(std::is_same<IDType, sema::ReturnParamAccessorValueStateID>()){
						return this->get_location(id.id);

					}else if constexpr(std::is_same<IDType, sema::OpDeleteThisAccessorValueStateID>()){
						return this->get_location(this->get_current_func().params[0].ident.as<Token::ID>());

					}else{
						static_assert(false, "Unknown value state ID");
					}
				});
			}


			EVO_NODISCARD auto get_location(const auto& node) const -> Diagnostic::Location {
				return Diagnostic::Location::get(node, this->source);
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
