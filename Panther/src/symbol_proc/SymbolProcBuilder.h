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


	class SymbolProcBuilder{
		public:
			SymbolProcBuilder(Context& _context, Source& _source) : context(_context), source(_source) {}
			~SymbolProcBuilder() = default;

			auto build(const AST::Node& stmt) -> bool;

		private:
			EVO_NODISCARD auto get_symbol_ident(const AST::Node& stmt) -> evo::Result<std::string_view>;

			EVO_NODISCARD auto build_var_decl(const AST::Node& stmt) -> bool;
			EVO_NODISCARD auto build_func_decl(const AST::Node& stmt) -> bool;
			EVO_NODISCARD auto build_alias_decl(const AST::Node& stmt) -> bool;
			EVO_NODISCARD auto build_typedef_decl(const AST::Node& stmt) -> bool;
			EVO_NODISCARD auto build_struct_decl(const AST::Node& stmt) -> bool;
			EVO_NODISCARD auto build_when_conditional(const AST::Node& stmt) -> bool;
			EVO_NODISCARD auto build_func_call(const AST::Node& stmt) -> bool;

			EVO_NODISCARD auto analyze_type(const AST::Type& ast_type) -> evo::Result<SymbolProc::TypeID>;

			template<bool IS_COMPTIME>
			EVO_NODISCARD auto analyze_expr(const AST::Node& expr) -> evo::Result<SymbolProc::ExprInfoID>;

			template<bool IS_COMPTIME>
			EVO_NODISCARD auto analyze_expr_block(const AST::Node& node) -> evo::Result<SymbolProc::ExprInfoID>;

			template<bool IS_COMPTIME>
			EVO_NODISCARD auto analyze_expr_func_call(const AST::Node& node) -> evo::Result<SymbolProc::ExprInfoID>;

			template<bool IS_COMPTIME>
			EVO_NODISCARD auto analyze_expr_templated(const AST::Node& node) -> evo::Result<SymbolProc::ExprInfoID>;

			template<bool IS_COMPTIME>
			EVO_NODISCARD auto analyze_expr_prefix(const AST::Node& node) -> evo::Result<SymbolProc::ExprInfoID>;

			template<bool IS_COMPTIME>
			EVO_NODISCARD auto analyze_expr_infix(const AST::Node& node) -> evo::Result<SymbolProc::ExprInfoID>;

			template<bool IS_COMPTIME>
			EVO_NODISCARD auto analyze_expr_postfix(const AST::Node& node) -> evo::Result<SymbolProc::ExprInfoID>;

			template<bool IS_COMPTIME>
			EVO_NODISCARD auto analyze_expr_new(const AST::Node& node) -> evo::Result<SymbolProc::ExprInfoID>;

			template<bool IS_COMPTIME>
			EVO_NODISCARD auto analyze_expr_ident(const AST::Node& node) -> evo::Result<SymbolProc::ExprInfoID>;

			EVO_NODISCARD auto analyze_expr_intrinsic(const AST::Node& node) -> evo::Result<SymbolProc::ExprInfoID>;
			EVO_NODISCARD auto analyze_expr_literal(const Token::ID& literal) -> evo::Result<SymbolProc::ExprInfoID>;
			EVO_NODISCARD auto analyze_expr_uninit(const AST::Node& node) -> evo::Result<SymbolProc::ExprInfoID>;
			EVO_NODISCARD auto analyze_expr_zeroinit(const AST::Node& node) -> evo::Result<SymbolProc::ExprInfoID>;
			EVO_NODISCARD auto analyze_expr_this(const AST::Node& node) -> evo::Result<SymbolProc::ExprInfoID>;



			auto add_instruction(auto&& instruction) -> void {
				this->symbol_proc->instructions.emplace_back(std::move(instruction));
			}


			auto create_expr_info() -> SymbolProc::ExprInfoID {
				EVO_DEFER([&](){ this->num_expr_infos += 1; });
				return SymbolProc::ExprInfoID(this->num_expr_infos);
			}

			auto create_type() -> SymbolProc::TypeID {
				EVO_DEFER([&](){ this->num_type_ids += 1; });
				return SymbolProc::TypeID(this->num_type_ids);
			}


			auto emit_fatal(Diagnostic::Code code, const auto& node, auto&&... args) -> void {
				this->context.emitFatal(code, this->get_location(node), std::forward<decltype(args)>(args)...);
			}

			auto emit_error(Diagnostic::Code code, const auto& node, auto&&... args) -> void {
				this->context.emitError(code, this->get_location(node), std::forward<decltype(args)>(args)...);
			}

			auto emit_warning(Diagnostic::Code code, const auto& node, auto&&... args) -> void {
				this->context.emitWarning(code, this->get_location(node), std::forward<decltype(args)>(args)...);
			}


			auto get_location(Diagnostic::Location::None) const -> Diagnostic::Location {
				return Diagnostic::Location::NONE;
			}

			auto get_location(const auto& node) const -> Diagnostic::Location {
				return Diagnostic::Location::get(node, this->source);
			}
	
		private:
			Context& context;
			Source& source;

			SymbolProc::ID symbol_proc_id = SymbolProc::ID::dummy();
			SymbolProc* symbol_proc = nullptr;
			uint32_t num_expr_infos = 0;
			uint32_t num_type_ids = 0;
	};


	EVO_NODISCARD inline auto build_symbol_procs(Context& context, Source::ID source_id) -> bool {
		Source& source = context.getSourceManager()[source_id];

		for(const AST::Node& ast_node : source.getASTBuffer().getGlobalStmts()){
			if(SymbolProcBuilder(context, source).build(ast_node) == false){ return false; }
		}

		return true;
	}


}
