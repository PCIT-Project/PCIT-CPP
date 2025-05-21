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
#include "../sema/sema.h"

namespace pcit::panther{


	class SymbolProcBuilder{
		public:
			SymbolProcBuilder(Context& _context, Source& _source) : context(_context), source(_source) {
				this->symbol_namespaces.emplace_back(&this->source.global_symbol_procs);
			}
			~SymbolProcBuilder() = default;

			EVO_NODISCARD auto build(const AST::Node& stmt) -> evo::Result<>;

			EVO_NODISCARD auto buildTemplateInstance(
				const SymbolProc& template_symbol_proc,
				BaseType::StructTemplate::Instantiation& instantiation,
				sema::ScopeManager::Scope::ID sema_scope_id,
				uint32_t instantiation_id
			) -> evo::Result<SymbolProc::ID>;

		private:
			EVO_NODISCARD auto get_symbol_ident(const AST::Node& stmt) -> evo::Result<std::string_view>;

			EVO_NODISCARD auto build_var_decl(const AST::Node& stmt) -> evo::Result<>;
			EVO_NODISCARD auto build_func_decl(const AST::Node& stmt) -> evo::Result<>;
			EVO_NODISCARD auto build_alias_decl(const AST::Node& stmt) -> evo::Result<>;
			EVO_NODISCARD auto build_typedef_decl(const AST::Node& stmt) -> evo::Result<>;
			EVO_NODISCARD auto build_struct_decl(const AST::Node& stmt) -> evo::Result<>;
			EVO_NODISCARD auto build_when_conditional(const AST::Node& stmt) -> evo::Result<>;
			EVO_NODISCARD auto build_func_call(const AST::Node& stmt) -> evo::Result<>;

			EVO_NODISCARD auto analyze_type(const AST::Type& ast_type) -> evo::Result<SymbolProc::TypeID>;
			EVO_NODISCARD auto analyze_type_base(const AST::Node& ast_type_base) -> evo::Result<SymbolProc::TermInfoID>;


			auto analyze_stmt(const AST::Node& stmt) -> evo::Result<>;
			auto analyze_local_var(const AST::VarDecl& var) -> evo::Result<>;
			auto analyze_return(const AST::Return& return_stmt) -> evo::Result<>;
			auto analyze_error(const AST::Error& error_stmt) -> evo::Result<>;
			auto analyze_defer(const AST::Defer& defer_stmt) -> evo::Result<>;
			auto analyze_unreachable(Token::ID unreachable_token) -> evo::Result<>;
			auto analyze_stmt_block(const AST::Block& stmt_block) -> evo::Result<>;
			auto analyze_func_call(const AST::FuncCall& func_call) -> evo::Result<>;
			auto analyze_assignment(const AST::Infix& infix) -> evo::Result<>;
			auto analyze_multi_assign(const AST::MultiAssign& multi_assign) -> evo::Result<>;


			template<bool IS_CONSTEXPR>
			EVO_NODISCARD auto analyze_term(const AST::Node& expr) -> evo::Result<SymbolProc::TermInfoID>;

			template<bool IS_CONSTEXPR>
			EVO_NODISCARD auto analyze_expr(const AST::Node& expr) -> evo::Result<SymbolProc::TermInfoID>;

			template<bool IS_CONSTEXPR>
			EVO_NODISCARD auto analyze_erroring_expr(const AST::Node& expr) -> evo::Result<SymbolProc::TermInfoID>;

			template<bool IS_CONSTEXPR, bool MUST_BE_EXPR, bool ERRORS>
			EVO_NODISCARD auto analyze_term_impl(const AST::Node& expr) -> evo::Result<SymbolProc::TermInfoID>;			


			template<bool IS_CONSTEXPR>
			EVO_NODISCARD auto analyze_expr_block(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID>;

			template<bool IS_CONSTEXPR, bool ERRORS>
			EVO_NODISCARD auto analyze_expr_func_call(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID>;

			template<bool IS_CONSTEXPR>
			EVO_NODISCARD auto analyze_expr_templated(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID>;

			template<bool IS_CONSTEXPR>
			EVO_NODISCARD auto analyze_expr_prefix(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID>;

			template<bool IS_CONSTEXPR>
			EVO_NODISCARD auto analyze_expr_infix(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID>;

			template<bool IS_CONSTEXPR>
			EVO_NODISCARD auto analyze_expr_postfix(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID>;

			template<bool IS_CONSTEXPR>
			EVO_NODISCARD auto analyze_expr_new(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID>;

			template<bool IS_CONSTEXPR>
			EVO_NODISCARD auto analyze_expr_try_else(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID>;

			template<bool IS_CONSTEXPR>
			EVO_NODISCARD auto analyze_expr_ident(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID>;

			EVO_NODISCARD auto analyze_expr_intrinsic(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID>;
			EVO_NODISCARD auto analyze_expr_literal(const Token::ID& literal) -> evo::Result<SymbolProc::TermInfoID>;
			EVO_NODISCARD auto analyze_expr_uninit(const Token::ID& uninit_token)
				-> evo::Result<SymbolProc::TermInfoID>;
			EVO_NODISCARD auto analyze_expr_zeroinit(const Token::ID& zeroinit_token)
				-> evo::Result<SymbolProc::TermInfoID>;
			EVO_NODISCARD auto analyze_expr_this(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID>;


			EVO_NODISCARD auto analyze_attributes(const AST::AttributeBlock& attribute_block)
				-> evo::Result<evo::SmallVector<SymbolProc::Instruction::AttributeParams>>;

			EVO_NODISCARD auto analyze_template_param_pack(const AST::TemplatePack& template_pack)
				-> evo::Result<evo::SmallVector<SymbolProc::Instruction::TemplateParamInfo>>;


			auto add_instruction(auto&& instruction) -> void {
				this->get_current_symbol().symbol_proc.instructions.emplace_back(std::move(instruction));
			}


			auto create_term_info() -> SymbolProc::TermInfoID {
				EVO_DEFER([&](){ this->get_current_symbol().num_term_infos += 1; });
				return SymbolProc::TermInfoID(this->get_current_symbol().num_term_infos);
			}

			auto create_type() -> SymbolProc::TypeID {
				EVO_DEFER([&](){ this->get_current_symbol().num_type_ids += 1; });
				return SymbolProc::TypeID(this->get_current_symbol().num_type_ids);
			}

			auto create_struct_instantiation() -> SymbolProc::StructInstantiationID {
				EVO_DEFER([&](){ this->get_current_symbol().num_struct_instantiations += 1; });
				return SymbolProc::StructInstantiationID(this->get_current_symbol().num_struct_instantiations);
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


			///////////////////////////////////
			// scoping

			struct SymbolProcInfo{
				SymbolProc::ID symbol_proc_id;
				SymbolProc& symbol_proc;
				uint32_t num_term_infos = 0;
				uint32_t num_type_ids = 0;
				uint32_t num_struct_instantiations = 0;
				bool is_template = false;

				auto operator=(const SymbolProcInfo& rhs) -> SymbolProcInfo& {
					std::destroy_at(this); // just in case destruction become non-trivial
					std::construct_at(this, rhs);
					return *this;
				}
			};

			EVO_NODISCARD auto is_child_symbol() const -> bool { return this->symbol_proc_infos.size() > 1; }

			EVO_NODISCARD auto get_parent_symbol() -> SymbolProcInfo& {
				evo::debugAssert(this->is_child_symbol(), "Not in child symbol");
				return this->symbol_proc_infos[this->symbol_proc_infos.size() - 2];
			}

			EVO_NODISCARD auto get_current_symbol() -> SymbolProcInfo& {
				return this->symbol_proc_infos.back();
			}
	
		private:
			Context& context;
			Source& source;


			evo::SmallVector<SymbolProcInfo> symbol_proc_infos{};
			using SymbolScope = evo::SmallVector<SymbolProc::ID>;
			evo::SmallVector<SymbolScope*> symbol_scopes{};
			evo::SmallVector<SymbolProc::Namespace*> symbol_namespaces{};
	};



	EVO_NODISCARD inline auto build_symbol_procs(Context& context, Source::ID source_id) -> evo::Result<> {
		Source& source = context.getSourceManager()[source_id];

		for(const AST::Node& ast_node : source.getASTBuffer().getGlobalStmts()){
			if(SymbolProcBuilder(context, source).build(ast_node).isError()){ return evo::resultError; }
		}

		return evo::Result<>();
	}


}
