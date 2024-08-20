//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once


#include <PCIT_core.h>

#include "../include/source_data.h"
#include "../include/Context.h"
#include "../include/Source.h"
#include "../include/TypeManager.h"

namespace pcit::panther{

	class SemanticAnalyzer{
		public:
			SemanticAnalyzer(Context& _context, Source::ID source_id);

			// for template instantiation
			SemanticAnalyzer(
				Context& _context,
				Source& _source,
				const ScopeManager::Scope& _scope,
				evo::SmallVector<SourceLocation>&& _template_parents
			);

			~SemanticAnalyzer() = default;

			SemanticAnalyzer(const SemanticAnalyzer&) = delete;


			auto analyze_global_declarations() -> bool;
			auto analyze_global_stmts() -> bool;


		private:
			///////////////////////////////////
			// analyze

			template<bool IS_GLOBAL>
			EVO_NODISCARD auto analyze_var_decl(const AST::VarDecl& var_decl) -> bool;

			template<bool IS_GLOBAL>
			EVO_NODISCARD auto analyze_func_decl(
				const AST::FuncDecl& func_decl, ASG::Func::InstanceID instance_id = ASG::Func::InstanceID()
			) -> evo::Result<std::optional<ASG::Func::ID>>;

			template<bool IS_GLOBAL>
			EVO_NODISCARD auto analyze_alias_decl(const AST::AliasDecl& alias_decl) -> bool;


			EVO_NODISCARD auto analyze_func_body(const AST::FuncDecl& ast_func, ASG::Func::ID asg_func_id) -> bool;

			EVO_NODISCARD auto analyze_block(const AST::Block& block) -> bool;


			EVO_NODISCARD auto analyze_stmt(const AST::Node& node) -> bool;

			EVO_NODISCARD auto analyze_func_call(const AST::FuncCall& func_call) -> bool;
			EVO_NODISCARD auto analyze_infix_stmt(const AST::Infix& infix) -> bool;
			EVO_NODISCARD auto analyze_return_stmt(const AST::Return& return_stmt) -> bool;


			///////////////////////////////////
			// misc

			EVO_NODISCARD auto get_type_id(const AST::Type& ast_type) -> evo::Result<TypeInfo::VoidableID>;
			EVO_NODISCARD auto get_type_id(const Token::ID& ident_token_id) -> evo::Result<TypeInfo::VoidableID>;
			EVO_NODISCARD auto is_type_generic(const AST::Type& ast_type) -> evo::Result<bool>; // is it type "Type"


			EVO_NODISCARD auto get_current_scope_level() const -> ScopeManager::ScopeLevel&;

			template<bool IS_GLOBAL>
			EVO_NODISCARD auto get_parent() const -> ASG::Parent;

			EVO_NODISCARD auto get_current_func() const -> ASG::Func&;



			///////////////////////////////////
			// expr

			struct ExprInfo{
				enum class ValueType{
					ConcreteConst,
					ConcreteMutable,
					Ephemeral,
					FluidLiteral, // only if actual type is unknown
					Import,
					Templated,
				};

				ValueType value_type;
				evo::Variant<
					std::monostate,             // ValueType::FluidLiteral
					TypeInfo::VoidableID,       // ValueType::[ConcreteConst|ConcreteMutable|Ephemeral]
					ASG::TemplatedFunc::LinkID, // ValueType::Templated
					Source::ID                  // ValueType::Import
				> type_id;
				std::optional<ASG::Expr> expr; // nullopt if from ExprValueKind::None or ValueType::Import

				EVO_NODISCARD constexpr auto is_ephemeral() const -> bool {
					return this->value_type == ValueType::Ephemeral || this->value_type == ValueType::FluidLiteral;
				}

				EVO_NODISCARD constexpr auto is_concrete() const -> bool {
					return this->value_type == ValueType::ConcreteConst || 
						   this->value_type == ValueType::ConcreteMutable;
				}
			};

			enum class ExprValueKind{
				None, // If you just want to get the type
				Runtime,
				ConstEval,
			};

			template<ExprValueKind EXPR_VALUE_KIND>
			EVO_NODISCARD auto analyze_expr(const AST::Node& node) -> evo::Result<ExprInfo>;


			template<ExprValueKind EXPR_VALUE_KIND>
			EVO_NODISCARD auto analyze_expr_block(const AST::Block& block) -> evo::Result<ExprInfo>;

			template<ExprValueKind EXPR_VALUE_KIND>
			EVO_NODISCARD auto analyze_expr_func_call(const AST::FuncCall& func_call) -> evo::Result<ExprInfo>;

			template<ExprValueKind EXPR_VALUE_KIND>
			EVO_NODISCARD auto analyze_expr_templated_expr(const AST::TemplatedExpr& templated_expr)
				-> evo::Result<ExprInfo>;

			template<ExprValueKind EXPR_VALUE_KIND>
			EVO_NODISCARD auto analyze_expr_prefix(const AST::Prefix& prefix) -> evo::Result<ExprInfo>;

			template<ExprValueKind EXPR_VALUE_KIND>
			EVO_NODISCARD auto analyze_expr_infix(const AST::Infix& infix) -> evo::Result<ExprInfo>;

			template<ExprValueKind EXPR_VALUE_KIND>
			EVO_NODISCARD auto analyze_expr_postfix(const AST::Postfix& postfix) -> evo::Result<ExprInfo>;

			template<ExprValueKind EXPR_VALUE_KIND>
			EVO_NODISCARD auto analyze_expr_ident(const Token::ID& ident) -> evo::Result<ExprInfo>;

			// TODO: does this need to be separate function
			template<ExprValueKind EXPR_VALUE_KIND>
			EVO_NODISCARD auto analyze_expr_ident_in_scope_level(
				Source::ID source, // is this needed, or does `this->source.getID()` suffice?
				const Token::ID& ident,
				std::string_view ident_str,
				ScopeManager::ScopeLevel::ID scope_level_id,
				bool variables_in_scope
			) -> evo::Result<std::optional<ExprInfo>>;

			template<ExprValueKind EXPR_VALUE_KIND>
			EVO_NODISCARD auto analyze_expr_intrinsic(const Token::ID& intrinsic) -> evo::Result<ExprInfo>;

			template<ExprValueKind EXPR_VALUE_KIND>
			EVO_NODISCARD auto analyze_expr_literal(const Token::ID& literal) -> evo::Result<ExprInfo>;

			template<ExprValueKind EXPR_VALUE_KIND>
			EVO_NODISCARD auto analyze_expr_uninit(const Token::ID& uninit) -> evo::Result<ExprInfo>;

			template<ExprValueKind EXPR_VALUE_KIND>
			EVO_NODISCARD auto analyze_expr_this(const Token::ID& this_expr) -> evo::Result<ExprInfo>;


			///////////////////////////////////
			// error handling

			template<typename NODE_T>
			EVO_NODISCARD auto type_check_and_set_fluid_literal_type(
				std::string_view name, const NODE_T& location, TypeInfo::ID target_type_id, ExprInfo& expr_info
			) -> bool;

			template<typename NODE_T>
			EVO_NODISCARD auto already_defined(std::string_view ident_str, const NODE_T& node) const -> bool;

			template<typename NODE_T>
			auto type_mismatch(
				std::string_view name, const NODE_T& location, TypeInfo::ID expected, const ExprInfo& got
			) -> void;

			EVO_NODISCARD auto may_recover() const -> bool;

			auto print_type(const ExprInfo& expr_info) const -> std::string;

			template<typename NODE_T>
			auto emit_fatal(
				Diagnostic::Code code,
				const NODE_T& location,
				std::string&& msg,
				evo::SmallVector<Diagnostic::Info>&& infos = evo::SmallVector<Diagnostic::Info>()
			) const -> void;

			template<typename NODE_T>
			auto emit_error(
				Diagnostic::Code code,
				const NODE_T& location,
				std::string&& msg,
				evo::SmallVector<Diagnostic::Info>&& infos = evo::SmallVector<Diagnostic::Info>()
			) const -> void;

			template<typename NODE_T>
			auto emit_warning(
				Diagnostic::Code code,
				const NODE_T& location,
				std::string&& msg,
				evo::SmallVector<Diagnostic::Info>&& infos = evo::SmallVector<Diagnostic::Info>()
			) const -> void;

			auto add_template_location_infos(evo::SmallVector<Diagnostic::Info>& infos) const -> void;


			///////////////////////////////////
			// get source location

			auto get_source_location(std::nullopt_t) const -> std::optional<SourceLocation>;

			auto get_source_location(Token::ID token_id) const -> SourceLocation;
			
			auto get_source_location(const AST::Node& node) const -> SourceLocation;

			auto get_source_location(const AST::VarDecl& var_decl) const -> SourceLocation;
			auto get_source_location(const AST::FuncDecl& func_decl) const -> SourceLocation;
			auto get_source_location(const AST::AliasDecl& alias_decl) const -> SourceLocation;
			auto get_source_location(const AST::Return& return_stmt) const -> SourceLocation;
			auto get_source_location(const AST::Block& block) const -> SourceLocation;
			auto get_source_location(const AST::FuncCall& func_call) const -> SourceLocation;
			auto get_source_location(const AST::TemplatedExpr& templated_expr) const -> SourceLocation;
			auto get_source_location(const AST::Prefix& prefix) const -> SourceLocation;
			auto get_source_location(const AST::Infix& infix) const -> SourceLocation;
			auto get_source_location(const AST::Postfix& postfix) const -> SourceLocation;
			auto get_source_location(const AST::MultiAssign& multi_assign) const -> SourceLocation;
			auto get_source_location(const AST::Type& type) const -> SourceLocation;

			auto get_source_location(ASG::Func::ID func_id) const -> SourceLocation;
			auto get_source_location(ASG::TemplatedFunc::ID templated_func_id) const -> SourceLocation;
			auto get_source_location(ASG::Var::ID var_id) const -> SourceLocation;

	
		private:
			Context& context;
			Source& source;

			ScopeManager::Scope scope;
			evo::SmallVector<SourceLocation> template_parents;

			std::unordered_map<std::string_view, ExprInfo> template_arg_exprs{};
			std::unordered_map<std::string_view, TypeInfo::VoidableID> template_arg_types{};
	};
	
	
	
}