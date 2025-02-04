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
				sema::ScopeManager::Scope& scope = context.sema_buffer.scope_manager.getScope(*source.sema_scope_id);

				return SemanticAnalyzer(context, source, symbol_proc_id, symbol_proc, scope);
			}

			~SemanticAnalyzer() = default;

			auto analyze() -> void;

		private:
			enum class Result{
				Success,
				Error,
				NeedToWait,
			};

			using Instruction = SymbolProc::Instruction;

			auto analyze_instr(const Instruction& instruction) -> Result;

			EVO_NODISCARD auto instr_finish_decl() -> Result;
			EVO_NODISCARD auto instr_global_var_decl(const Instruction::GlobalVarDecl& instr) -> Result;
			EVO_NODISCARD auto instr_func_call(const Instruction::FuncCall& instr) -> Result;
			EVO_NODISCARD auto instr_import(const Instruction::Import& instr) -> Result;

			template<bool IS_COMPTIME>
			EVO_NODISCARD auto instr_expr_accessor(
				const AST::Infix& infix, SymbolProcExprInfoID lhs_id, Token::ID rhs_ident, SymbolProcExprInfoID output
			) -> Result;

			EVO_NODISCARD auto instr_primitive_type(const Instruction::PrimitiveType& instr) -> Result;
			EVO_NODISCARD auto instr_comptime_ident(const Instruction::ComptimeIdent& instr) -> Result;

			template<bool IS_COMPTIME>
			EVO_NODISCARD auto instr_ident(Token::ID ident, SymbolProc::ExprInfoID output) -> Result;

			EVO_NODISCARD auto instr_intrinsic(const Instruction::Intrinsic& instr) -> Result;
			EVO_NODISCARD auto instr_literal(const Instruction::Literal& instr) -> Result;


			///////////////////////////////////
			// misc

			EVO_NODISCARD auto get_current_scope_level() const -> sema::ScopeLevel&;

			// TODO: does this need to be separate function
			EVO_NODISCARD auto analyze_expr_ident_in_scope_level(
				const Token::ID& ident,
				std::string_view ident_str,
				sema::ScopeLevel::ID scope_level_id,
				bool variables_in_scope,
				bool is_global_scope
			) -> evo::Result<std::optional<ExprInfo>>;


			///////////////////////////////////
			// exec value gets / returns

			auto get_type(SymbolProc::TypeID symbol_proc_type_id) -> TypeInfo::VoidableID;
			auto return_type(SymbolProc::TypeID symbol_proc_type_id, TypeInfo::VoidableID&& id) -> void;

			auto get_expr_info(SymbolProc::ExprInfoID symbol_proc_expr_info_id) -> ExprInfo&;
			auto return_expr_info(SymbolProc::ExprInfoID symbol_proc_expr_info_id, auto&&... args) -> void;



			///////////////////////////////////
			// error handling / diagnostics

			struct TypeCheckInfo{
				bool ok;
				bool requires_implicit_conversion; // only may be true if .ok is true
			};

			template<bool IS_NOT_TEMPLATE_ARG>
			EVO_NODISCARD auto type_check(
				TypeInfo::ID expected_type_id,
				ExprInfo& got_expr,
				std::string_view expected_type_location_name,
				const auto& location
			) -> TypeCheckInfo;

			auto error_type_mismatch(
				TypeInfo::ID expected_type_id,
				const ExprInfo& got_expr,
				std::string_view expected_type_location_name,
				const auto& location
			) -> void;


			EVO_NODISCARD auto check_type_qualifiers(
				evo::ArrayProxy<AST::Type::Qualifier> qualifiers, const auto& location
			) -> bool;


			EVO_NODISCARD auto add_ident_to_scope(
				std::string_view ident_str, const auto& ast_node, auto&&... ident_id_info
			) -> bool;

			EVO_NODISCARD auto print_type(const ExprInfo& expr_info) const -> std::string;


			auto emit_fatal(Diagnostic::Code code, const auto& node, auto&&... args) -> void {
				this->symbol_proc.errored = true;
				this->context.emitFatal(code, this->get_location(node), std::forward<decltype(args)>(args)...);
			}

			auto emit_error(Diagnostic::Code code, const auto& node, auto&&... args) -> void {
				this->symbol_proc.errored = true;
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

			EVO_NODISCARD auto get_location(const sema::FuncID& func) const -> Diagnostic::Location {
				std::ignore = func;
				evo::unimplemented();
			}

			EVO_NODISCARD auto get_location(const sema::TemplatedFuncID& templated_func) const -> Diagnostic::Location {
				std::ignore = templated_func;
				evo::unimplemented();
			}

			EVO_NODISCARD auto get_location(const sema::VarID& var) const -> Diagnostic::Location {
				return Diagnostic::Location::get(var, this->source, this->context);
			}

			EVO_NODISCARD auto get_location(const sema::StructID& struct_id) const -> Diagnostic::Location {
				std::ignore = struct_id;
				evo::unimplemented();
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
				std::ignore = alias_id;
				evo::unimplemented();
			}

			EVO_NODISCARD auto get_location(const BaseType::Typedef::ID& typedef_id) const -> Diagnostic::Location {
				std::ignore = typedef_id;
				evo::unimplemented();
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
