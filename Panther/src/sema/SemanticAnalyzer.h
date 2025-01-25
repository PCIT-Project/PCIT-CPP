////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>

#include "./ExprInfo.h"
#include "../deps/deps.h"
#include "../../include/Diagnostic.h"

namespace pcit::panther{


	class SemanticAnalyzer{
		public:
			SemanticAnalyzer(class Context& _context, deps::Node::ID _deps_node_id);
			~SemanticAnalyzer() = default;

			auto analyzeDecl() -> bool;
			auto analyzeDef() -> bool;
			auto analyzeDeclDef() -> bool;

		private:
			template<bool PROP_FOR_DECLS>
			auto propogate_to_required_by(deps::Node::ID required_by_id) -> void;

			EVO_NODISCARD auto analyze_decl_var() -> bool;
			EVO_NODISCARD auto analyze_decl_func() -> evo::Result<sema::Func::ID>;
			EVO_NODISCARD auto analyze_decl_alias() -> evo::Result<BaseType::Alias::ID>;
			EVO_NODISCARD auto analyze_decl_typedef() -> evo::Result<BaseType::Typedef::ID>;
			EVO_NODISCARD auto analyze_decl_struct() -> evo::Result<sema::Struct::ID>;
			EVO_NODISCARD auto analyze_decl_when_cond() -> bool;

			EVO_NODISCARD auto analyze_def_var(const sema::Var::ID& sema_var_id) -> bool;
			EVO_NODISCARD auto analyze_def_func(const sema::Func::ID& sema_func_id) -> bool;
			EVO_NODISCARD auto analyze_def_alias(const BaseType::Alias::ID& sema_alias_id) -> bool;
			EVO_NODISCARD auto analyze_def_typedef(const BaseType::Typedef::ID& sema_typedef_id) -> bool;
			EVO_NODISCARD auto analyze_def_struct(const sema::Struct::ID& sema_struct_id) -> bool;

			EVO_NODISCARD auto analyze_decl_def_var() -> bool;


			struct DeclVarImplData{
				const AST::VarDecl& ast_var_decl;
				std::string_view var_ident;
				std::optional<TypeInfo::ID> type_id;
				bool is_pub;
			};
			EVO_NODISCARD auto analyze_decl_var_impl() -> evo::Result<DeclVarImplData>;

			EVO_NODISCARD auto analyze_def_var_impl(
				const AST::VarDecl& ast_var_decl, std::optional<TypeInfo::ID>& type_id
			) -> evo::Result<sema::Expr>;


			///////////////////////////////////
			// types

			EVO_NODISCARD auto get_type_id(const AST::Type& ast_type) -> evo::Result<TypeInfo::VoidableID>;
			EVO_NODISCARD auto get_type_id(const Token::ID& ident_token_id) -> evo::Result<TypeInfo::VoidableID>;


			///////////////////////////////////
			// expr

			template<bool IS_COMPTIME>
			EVO_NODISCARD auto analyze_expr(const AST::Node& node) -> evo::Result<ExprInfo>;

			EVO_NODISCARD auto analyze_expr_block(const AST::Block& block) -> evo::Result<ExprInfo>;

			template<bool IS_COMPTIME>
			EVO_NODISCARD auto analyze_expr_func_call(const AST::FuncCall& func_call) -> evo::Result<ExprInfo>;

			EVO_NODISCARD auto analyze_import(const AST::FuncCall& func_call) -> evo::Result<ExprInfo>;

			EVO_NODISCARD auto analyze_expr_templated_expr(const AST::TemplatedExpr& templated_expr)
				-> evo::Result<ExprInfo>;

			// EVO_NODISCARD auto analyze_expr_templated_intrinsic(
			// 	const AST::TemplatedExpr& templated_expr, TemplatedIntrinsic::Kind templated_intrinsic_kind
			// ) -> evo::Result<ExprInfo>;

			EVO_NODISCARD auto analyze_expr_prefix(const AST::Prefix& prefix) -> evo::Result<ExprInfo>;

			template<bool IS_CONST>
			EVO_NODISCARD auto analyze_expr_prefix_address_of(const AST::Prefix& prefix, const ExprInfo& rhs_info)
				-> evo::Result<ExprInfo>;

			EVO_NODISCARD auto analyze_expr_infix(const AST::Infix& infix) -> evo::Result<ExprInfo>;
			EVO_NODISCARD auto analyze_expr_postfix(const AST::Postfix& postfix) -> evo::Result<ExprInfo>;
			EVO_NODISCARD auto analyze_expr_new(const AST::New& new_expr) -> evo::Result<ExprInfo>;
			EVO_NODISCARD auto analyze_expr_new_impl(const AST::New& new_expr, TypeInfo::VoidableID type_id) 
				-> evo::Result<ExprInfo>;
			EVO_NODISCARD auto analyze_expr_new_impl(const AST::New& new_expr, TypeInfo::ID type_id) 
				-> evo::Result<ExprInfo>;
			EVO_NODISCARD auto analyze_expr_ident(const Token::ID& ident) -> evo::Result<ExprInfo>;

			// TODO: does this need to be separate function
			EVO_NODISCARD auto analyze_expr_ident_in_scope_level(
				const Token::ID& ident,
				std::string_view ident_str,
				sema::ScopeLevel::ID scope_level_id,
				bool variables_in_scope,
				bool is_global_scope
			) -> evo::Result<std::optional<ExprInfo>>;

			EVO_NODISCARD auto analyze_expr_intrinsic(const Token::ID& intrinsic_token_id) -> evo::Result<ExprInfo>;
			EVO_NODISCARD auto analyze_expr_literal(const Token::ID& literal) -> evo::Result<ExprInfo>;
			EVO_NODISCARD auto analyze_expr_uninit(const Token::ID& uninit) -> evo::Result<ExprInfo>;
			EVO_NODISCARD auto analyze_expr_zeroinit(const Token::ID& zeroinit) -> evo::Result<ExprInfo>;
			EVO_NODISCARD auto analyze_expr_this(const Token::ID& this_expr) -> evo::Result<ExprInfo>;



			///////////////////////////////////
			// misc

			EVO_NODISCARD auto get_current_scope_level() const -> sema::ScopeLevel&;



			///////////////////////////////////
			// error checking / handling / diagnostics

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


			// returns false if it already existed
			template<bool IS_GLOBAL, bool IS_FUNC>
			EVO_NODISCARD auto add_ident_to_scope(std::string_view ident_str, Token::ID location) -> bool;

			EVO_NODISCARD auto check_type_qualifiers(
				evo::ArrayProxy<AST::Type::Qualifier> qualifiers, const auto& location
			) -> bool;

			EVO_NODISCARD auto error_already_defined(std::string_view ident_str, const auto& redef_location) -> void;

			EVO_NODISCARD auto print_type(const ExprInfo& expr_info) const -> std::string;

			auto emit_warning(Diagnostic::Code code, const auto& location, auto&&... args) -> void;
			auto emit_error(Diagnostic::Code code, const auto& location, auto&&... args) -> void;
			auto emit_fatal(Diagnostic::Code code, const auto& location, auto&&... args) -> void;


			///////////////////////////////////
			// get source location


			EVO_NODISCARD auto get_location(const Diagnostic::Location::None& location) const -> Diagnostic::Location {
				return location;
			}

			EVO_NODISCARD auto get_location(const Diagnostic::Location& location) const -> Diagnostic::Location {
				return location;
			}

			EVO_NODISCARD auto get_location(Token::ID token_id) const -> Diagnostic::Location {
				return Diagnostic::Location::get(token_id, this->source);
			}
			
			EVO_NODISCARD auto get_location(const AST::Node& node) const -> Diagnostic::Location {
				return Diagnostic::Location::get(node, this->source);
			}

			EVO_NODISCARD auto get_location(const AST::VarDecl& var_decl) const -> Diagnostic::Location {
				return Diagnostic::Location::get(var_decl, this->source);
			}

			EVO_NODISCARD auto get_location(const AST::FuncDecl& func_decl) const -> Diagnostic::Location {
				return Diagnostic::Location::get(func_decl, this->source);
			}

			EVO_NODISCARD auto get_location(const AST::AliasDecl& alias_decl) const -> Diagnostic::Location {
				return Diagnostic::Location::get(alias_decl, this->source);
			}

			EVO_NODISCARD auto get_location(const AST::TypedefDecl& typedef_decl) const -> Diagnostic::Location {
				return Diagnostic::Location::get(typedef_decl, this->source);
			}

			EVO_NODISCARD auto get_location(const AST::Return& return_stmt) const -> Diagnostic::Location {
				return Diagnostic::Location::get(return_stmt, this->source);
			}

			EVO_NODISCARD auto get_location(const AST::Conditional& conditional) const -> Diagnostic::Location {
				return Diagnostic::Location::get(conditional, this->source);
			}

			EVO_NODISCARD auto get_location(const AST::WhenConditional& when_cond) const -> Diagnostic::Location {
				return Diagnostic::Location::get(when_cond, this->source);
			}

			EVO_NODISCARD auto get_location(const AST::Block& block) const -> Diagnostic::Location {
				return Diagnostic::Location::get(block, this->source);
			}

			EVO_NODISCARD auto get_location(const AST::FuncCall& func_call) const -> Diagnostic::Location {
				return Diagnostic::Location::get(func_call, this->source);
			}

			EVO_NODISCARD auto get_location(const AST::TemplatedExpr& templated_expr) const -> Diagnostic::Location {
				return Diagnostic::Location::get(templated_expr, this->source);
			}

			EVO_NODISCARD auto get_location(const AST::Prefix& prefix) const -> Diagnostic::Location {
				return Diagnostic::Location::get(prefix, this->source);
			}

			EVO_NODISCARD auto get_location(const AST::Infix& infix) const -> Diagnostic::Location {
				return Diagnostic::Location::get(infix, this->source);
			}

			EVO_NODISCARD auto get_location(const AST::Postfix& postfix) const -> Diagnostic::Location {
				return Diagnostic::Location::get(postfix, this->source);
			}

			EVO_NODISCARD auto get_location(const AST::MultiAssign& multi_assign) const -> Diagnostic::Location {
				return Diagnostic::Location::get(multi_assign, this->source);
			}

			EVO_NODISCARD auto get_location(const AST::New& new_expr) const -> Diagnostic::Location {
				return Diagnostic::Location::get(new_expr, this->source);
			}

			EVO_NODISCARD auto get_location(const AST::Type& type) const -> Diagnostic::Location {
				return Diagnostic::Location::get(type, this->source);
			}

			EVO_NODISCARD auto get_location(const AST::TypeIDConverter& type) const -> Diagnostic::Location {
				return Diagnostic::Location::get(type, this->source);
			}

			EVO_NODISCARD auto get_location(const AST::AttributeBlock::Attribute& attr) const -> Diagnostic::Location {
				return Diagnostic::Location::get(attr, this->source);
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
				std::ignore = var;
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


			EVO_NODISCARD auto get_location(const sema::ScopeLevel::IDNotReady& id_not_ready) const
			-> Diagnostic::Location {
				return this->get_location(id_not_ready.location);
			}

			EVO_NODISCARD auto get_location(const sema::ScopeLevel::ModuleInfo& module_info) const
			-> Diagnostic::Location {
				return this->get_location(module_info.tokenID);
			}		



		private:
			class Context& context;
			class Source& source;
			deps::Node::ID deps_node_id;
			deps::Node& deps_node;
			SemaBuffer::Scope& scope;
	};



	inline auto analyze_semantics_decl(class Context& context, deps::Node::ID deps_node_id) -> bool {
		return SemanticAnalyzer(context, deps_node_id).analyzeDecl();
	}

	inline auto analyze_semantics_def(class Context& context, deps::Node::ID deps_node_id) -> bool {
		return SemanticAnalyzer(context, deps_node_id).analyzeDef();
	}

	inline auto analyze_semantics_decl_def(class Context& context, deps::Node::ID deps_node_id) -> bool {
		return SemanticAnalyzer(context, deps_node_id).analyzeDeclDef();

	}


}