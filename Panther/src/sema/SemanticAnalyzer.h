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
				Success,
				Error,
				RecoverableError,
				NeedToWait,
				NeedToWaitBeforeNextInstr,
			};

			///////////////////////////////////
			// instructions

			using Instruction = SymbolProc::Instruction;

			auto analyze_instr(const Instruction& instruction) -> Result;

			EVO_NODISCARD auto instr_var_decl(const Instruction::VarDecl& instr) -> Result;
			EVO_NODISCARD auto instr_var_def(const Instruction::VarDef& instr) -> Result;
			EVO_NODISCARD auto instr_var_decl_def(const Instruction::VarDeclDef& instr) -> Result;
			EVO_NODISCARD auto instr_when_cond(const Instruction::WhenCond& instr) -> Result;
			EVO_NODISCARD auto instr_alias_decl(const Instruction::AliasDecl& instr) -> Result;
			EVO_NODISCARD auto instr_alias_def(const Instruction::AliasDef& instr) -> Result;

			template<bool IS_INSTANTIATION>
			EVO_NODISCARD auto instr_struct_decl(const Instruction::StructDecl<IS_INSTANTIATION>& instr) -> Result;
			EVO_NODISCARD auto instr_struct_def() -> Result;
			EVO_NODISCARD auto instr_template_struct(const Instruction::TemplateStruct& instr) -> Result;

			template<bool IS_INSTANTIATION>
			EVO_NODISCARD auto instr_func_decl(const Instruction::FuncDecl<IS_INSTANTIATION>& instr) -> Result;
			EVO_NODISCARD auto instr_func_def(const Instruction::FuncDef& instr) -> Result;
			EVO_NODISCARD auto instr_template_func(const Instruction::TemplateFunc& instr) -> Result;


			EVO_NODISCARD auto instr_return(const Instruction::Return& instr) -> Result;


			EVO_NODISCARD auto instr_type_to_term(const Instruction::TypeToTerm& instr) -> Result;
			EVO_NODISCARD auto instr_func_call(const Instruction::FuncCall& instr) -> Result;
			EVO_NODISCARD auto instr_import(const Instruction::Import& instr) -> Result;
			EVO_NODISCARD auto instr_templated_term(const Instruction::TemplatedTerm& instr) -> Result;
			EVO_NODISCARD auto instr_templated_term_wait(const Instruction::TemplatedTermWait& instr)
				-> Result;

			template<bool NEEDS_DEF>
			EVO_NODISCARD auto instr_expr_accessor(const Instruction::Accessor<NEEDS_DEF>& instr) -> Result;

			EVO_NODISCARD auto instr_primitive_type(const Instruction::PrimitiveType& instr) -> Result;
			EVO_NODISCARD auto instr_user_type(const Instruction::UserType& instr) -> Result;
			EVO_NODISCARD auto instr_base_type_ident(const Instruction::BaseTypeIdent& instr) -> Result;

			template<bool NEEDS_DEF>
			EVO_NODISCARD auto instr_ident(const Instruction::Ident<NEEDS_DEF>& instr) -> Result;

			EVO_NODISCARD auto instr_intrinsic(const Instruction::Intrinsic& instr) -> Result;
			EVO_NODISCARD auto instr_literal(const Instruction::Literal& instr) -> Result;
			EVO_NODISCARD auto instr_uninit(const Instruction::Uninit& instr) -> Result;
			EVO_NODISCARD auto instr_zeroinit(const Instruction::Zeroinit& instr) -> Result;


			///////////////////////////////////
			// scope

			EVO_NODISCARD auto get_current_scope_level() const -> sema::ScopeLevel&;
			EVO_NODISCARD auto push_scope_level(sema::StmtBlock* stmt_block = nullptr) -> void;
			EVO_NODISCARD auto push_scope_level(sema::StmtBlock* stmt_block, const auto& object_scope_id) -> void;
			EVO_NODISCARD auto pop_scope_level() -> void;

			EVO_NODISCARD auto get_current_func() -> sema::Func&;


			///////////////////////////////////
			// misc


			template<bool NEEDS_DEF>
			EVO_NODISCARD auto lookup_ident_impl(Token::ID ident) -> evo::Expected<TermInfo, Result>;


			enum class AnalyzeExprIdentInScopeLevelError{
				DoesntExist,
				NeedsToWaitOnDef,
				ErrorEmitted,
			};
			template<bool NEEDS_DEF, bool PUB_REQUIRED>
			EVO_NODISCARD auto analyze_expr_ident_in_scope_level(
				const Token::ID& ident,
				std::string_view ident_str,
				const sema::ScopeLevel& scope_level,
				bool variables_in_scope,
				bool is_global_scope,
				const Source* source_module
			) -> evo::Expected<TermInfo, AnalyzeExprIdentInScopeLevelError>;


			template<bool NEEDS_DEF>
			EVO_NODISCARD auto wait_on_symbol_proc(
				evo::ArrayProxy<const SymbolProc::Namespace*> symbol_proc_namespaces,
				const auto& ident,
				std::string_view ident_str,
				std::string&& error_msg_if_ident_doesnt_exist,
				std::function<Result()> func_if_def_completed
			) -> Result;


			auto set_waiting_for_is_done(SymbolProc::ID target_id, SymbolProc::ID done_id) -> void;

			template<bool ALLOW_TYPEDEF>
			EVO_NODISCARD auto get_actual_type(TypeInfo::ID type_id) const -> TypeInfo::ID;


			struct VarAttrs{
				bool is_pub;
			};
			EVO_NODISCARD auto analyze_var_attrs(
				const AST::VarDecl& var_decl, evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
			) -> evo::Result<VarAttrs>;


			struct StructAttrs{
				bool is_pub;
			};
			EVO_NODISCARD auto analyze_struct_attrs(
				const AST::StructDecl& struct_decl, evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
			) -> evo::Result<StructAttrs>;


			struct FuncAttrs{
				bool is_pub;
				bool is_runtime;
				bool is_entry;
			};
			EVO_NODISCARD auto analyze_func_attrs(
				const AST::FuncDecl& func_decl, evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
			) -> evo::Result<FuncAttrs>;


			///////////////////////////////////
			// propogate finished

			auto propagate_finished_impl(const evo::SmallVector<SymbolProc::ID>& waited_on_by_list) -> void;
			auto propagate_finished_decl() -> void;
			auto propagate_finished_def() -> void;
			auto propagate_finished_decl_def() -> void;


			///////////////////////////////////
			// exec value gets / returns

			auto get_type(SymbolProc::TypeID symbol_proc_type_id) -> TypeInfo::VoidableID;
			auto return_type(SymbolProc::TypeID symbol_proc_type_id, TypeInfo::VoidableID&& id) -> void;

			auto get_term_info(SymbolProc::TermInfoID symbol_proc_term_info_id) -> TermInfo&;
			auto return_term_info(SymbolProc::TermInfoID symbol_proc_term_info_id, auto&&... args) -> void;

			auto get_struct_instantiation(SymbolProc::StructInstantiationID instantiation_id)
				-> const BaseType::StructTemplate::Instantiation&;
			auto return_struct_instantiation(
				SymbolProc::StructInstantiationID instantiation_id,
				const BaseType::StructTemplate::Instantiation& instantiation
			) -> void;



			///////////////////////////////////
			// error handling / diagnostics

			struct TypeCheckInfo{
				bool ok;
				bool requires_implicit_conversion; // value is undefined if .ok == false
			};

			template<bool IS_NOT_ARGUMENT>
			EVO_NODISCARD auto type_check(
				TypeInfo::ID expected_type_id,
				TermInfo& got_expr,
				std::string_view expected_type_location_name,
				const auto& location
			) -> TypeCheckInfo;

			auto error_type_mismatch(
				TypeInfo::ID expected_type_id,
				const TermInfo& got_expr,
				std::string_view expected_type_location_name,
				const auto& location
			) -> void;


			EVO_NODISCARD auto check_type_qualifiers(
				evo::ArrayProxy<AST::Type::Qualifier> qualifiers, const auto& location
			) -> bool;


			EVO_NODISCARD auto check_term_isnt_type(const TermInfo& term_info, const auto& location) -> bool;


			EVO_NODISCARD auto add_ident_to_scope(
				std::string_view ident_str, const auto& ast_node, auto&&... ident_id_info
			) -> bool {
				return this->add_ident_to_scope(
					this->scope, ident_str, ast_node, std::forward<decltype(ident_id_info)>(ident_id_info)...
				);
			}

			EVO_NODISCARD auto add_ident_to_scope(
				sema::ScopeManager::Scope& target_scope,
				std::string_view ident_str,
				const auto& ast_node,
				auto&&... ident_id_info
			) -> bool;

			template<bool IS_SHADOWING>
			auto error_already_defined(
				const auto& redef_id, std::string_view ident_str, const sema::ScopeLevel::IdentID& first_defined_id
			) -> void;

			EVO_NODISCARD auto print_type(const TermInfo& term_info) const -> std::string;


			EVO_NODISCARD auto check_scope_isnt_terminated(const auto& location) -> bool;


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

			EVO_NODISCARD auto get_location(const sema::ScopeLevel::ModuleInfo& module_info) const
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

			EVO_NODISCARD auto get_location(const sema::FuncID& func) const -> Diagnostic::Location {
				return Diagnostic::Location::get(func, this->source, this->context);
			}

			EVO_NODISCARD auto get_location(const sema::TemplatedFuncID& templated_func) const -> Diagnostic::Location {
				return Diagnostic::Location::get(templated_func, this->source, this->context);
			}

			EVO_NODISCARD auto get_location(const sema::GlobalVarID& var) const -> Diagnostic::Location {
				return Diagnostic::Location::get(var, this->source, this->context);
			}

			EVO_NODISCARD auto get_location(const sema::ParamID& param) const -> Diagnostic::Location {
				std::ignore = param;
				evo::unimplemented();
			}

			EVO_NODISCARD auto get_location(const sema::ReturnParamID& return_param) const -> Diagnostic::Location {
				std::ignore = return_param;
				evo::unimplemented();
			}


			EVO_NODISCARD auto get_location(const BaseType::Alias::ID& alias_id) const -> Diagnostic::Location {
				return Diagnostic::Location::get(alias_id, this->source, this->context);
			}

			EVO_NODISCARD auto get_location(const BaseType::Typedef::ID& typedef_id) const -> Diagnostic::Location {
				std::ignore = typedef_id;
				evo::unimplemented();
			}

			EVO_NODISCARD auto get_location(const BaseType::Struct::ID& struct_id) const -> Diagnostic::Location {
				return Diagnostic::Location::get(struct_id, this->source, this->context);
			}

			EVO_NODISCARD auto get_location(const sema::TemplatedStruct::ID& templated_struct_id) const
			-> Diagnostic::Location {
				const sema::TemplatedStruct& templated_struct =
					this->context.sema_buffer.getTemplatedStruct(templated_struct_id);

				return Diagnostic::Location::get(
					templated_struct.symbolProc.ast_node,
					this->context.getSourceManager()[templated_struct.symbolProc.source_id]
				);
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
			) : context(_context), source(_source), symbol_proc_id(sym_proc_id), symbol_proc(sym_proc), scope(_scope) {}

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
