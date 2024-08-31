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
			EVO_NODISCARD auto analyze_multi_assign_stmt(const AST::MultiAssign& multi_assign) -> bool;


			///////////////////////////////////
			// misc

			EVO_NODISCARD auto get_type_id(const AST::Type& ast_type) -> evo::Result<TypeInfo::VoidableID>;
			EVO_NODISCARD auto get_type_id(const Token::ID& ident_token_id) -> evo::Result<TypeInfo::VoidableID>;
			EVO_NODISCARD auto is_type_generic(const AST::Type& ast_type) -> evo::Result<bool>; // is it type "Type"


			EVO_NODISCARD auto get_current_scope_level() const -> ScopeManager::Level&;

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
					EpemeralFluid, // only if actual type is unknown
					Import,
					Templated,
					Initializer, // uninit / zeroinit
				};

				ValueType value_type;
				evo::Variant<
					std::monostate,                 // ValueType::[EpemeralFluid|Initializer]
					evo::SmallVector<TypeInfo::ID>, // ValueType::[ConcreteConst|ConcreteMutable|Ephemeral]
					ASG::TemplatedFunc::LinkID,     // ValueType::Templated
					Source::ID                      // ValueType::Import
				> type_id;
				evo::SmallVector<ASG::Expr> expr; // empty if from ExprValueKind::None or ValueType::Import
				                                  // may only be multiple if multiple possible function overloads


				ExprInfo(ValueType vt, auto&& _type_id, evo::SmallVector<ASG::Expr>&& expr_list)
					: value_type(vt), type_id(std::move(_type_id)), expr(std::move(expr_list)) {};

				ExprInfo(ValueType vt, auto&& _type_id, ASG::Expr&& expr_single)
					: value_type(vt), type_id(std::move(_type_id)), expr{std::move(expr_single)} {};

				ExprInfo(ValueType vt, auto&& _type_id, std::nullopt_t)
					: value_type(vt), type_id(std::move(_type_id)), expr() {};



				EVO_NODISCARD constexpr auto is_ephemeral() const -> bool {
					return this->value_type == ValueType::Ephemeral || this->value_type == ValueType::EpemeralFluid;
				}

				EVO_NODISCARD constexpr auto is_concrete() const -> bool {
					return this->value_type == ValueType::ConcreteConst || 
						   this->value_type == ValueType::ConcreteMutable;
				}


				// TODO: remove these after MSVC bug is fixed
				EVO_NODISCARD static auto generateExprInfoTypeIDs(auto&&... args) -> decltype(type_id) {
					auto type_id_variant = decltype(type_id)();
					type_id_variant.emplace<evo::SmallVector<TypeInfo::ID>>(std::forward<decltype(args)>(args)...);
					return type_id_variant;
				}

				EVO_NODISCARD static auto generateExprInfoTypeIDs(TypeInfo::ID type_info_id) -> decltype(type_id) {
					return generateExprInfoTypeIDs(evo::SmallVector<TypeInfo::ID>{type_info_id});
				}


				// TODO: better safety around value (for example, getter for expr to get front only if makes sense)
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
			EVO_NODISCARD auto analyze_expr_intrin_func_call(const AST::FuncCall& func_call) -> evo::Result<ExprInfo>;

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
				ScopeManager::Level::ID scope_level_id,
				bool variables_in_scope
			) -> evo::Result<std::optional<ExprInfo>>;

			template<ExprValueKind EXPR_VALUE_KIND>
			EVO_NODISCARD auto analyze_expr_intrinsic(const Token::ID& intrinsic) -> evo::Result<ExprInfo>;

			template<ExprValueKind EXPR_VALUE_KIND>
			EVO_NODISCARD auto analyze_expr_literal(const Token::ID& literal) -> evo::Result<ExprInfo>;

			template<ExprValueKind EXPR_VALUE_KIND>
			EVO_NODISCARD auto analyze_expr_uninit(const Token::ID& uninit) -> evo::Result<ExprInfo>;

			template<ExprValueKind EXPR_VALUE_KIND>
			EVO_NODISCARD auto analyze_expr_zeroinit(const Token::ID& zeroinit) -> evo::Result<ExprInfo>;

			template<ExprValueKind EXPR_VALUE_KIND>
			EVO_NODISCARD auto analyze_expr_this(const Token::ID& this_expr) -> evo::Result<ExprInfo>;




			struct ArgInfo{
				std::optional<Token::ID> explicit_ident;
				ExprInfo expr_info;
				const AST::Node& ast_node;
			};
			template<typename NODE_T>
			EVO_NODISCARD auto select_func_overload(
				const NODE_T& location,
				evo::ArrayProxy<ASG::Func::LinkID> asg_funcs,
				evo::ArrayProxy<const BaseType::Function*> funcs,
				evo::SmallVector<ArgInfo>& arg_infos,
				evo::ArrayProxy<AST::FuncCall::Arg> args
			) -> evo::Result<size_t>; // returns index of selected func


			///////////////////////////////////
			// error handling

			struct TypeCheckInfo{
				bool ok;
				bool requires_implicit_conversion; // only may be true if .ok is true
			};

			template<bool IMPLICITLY_CONVERT, typename NODE_T>
			EVO_NODISCARD auto type_check(
				TypeInfo::ID expected_type_id, ExprInfo& got_expr, std::string_view name, const NODE_T& location
			) -> TypeCheckInfo;

			template<typename NODE_T>
			auto error_type_mismatch(
				TypeInfo::ID expected_type_id, const ExprInfo& got_expr, std::string_view name, const NODE_T& location
			) -> void;

			template<typename NODE_T>
			EVO_NODISCARD auto check_type_qualifiers(
				evo::ArrayProxy<AST::Type::Qualifier> qualifiers, const NODE_T& location
			) -> bool;

			template<typename NODE_T>
			EVO_NODISCARD auto already_defined(std::string_view ident_str, const NODE_T& node) const -> bool;

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
			auto emit_fatal(
				Diagnostic::Code code, const NODE_T& location, std::string&& msg, Diagnostic::Info&& info
			) const -> void {
				this->emit_fatal(code, location, std::move(msg), evo::SmallVector<Diagnostic::Info>{std::move(info)});
			}


			template<typename NODE_T>
			auto emit_error(
				Diagnostic::Code code,
				const NODE_T& location,
				std::string&& msg,
				evo::SmallVector<Diagnostic::Info>&& infos = evo::SmallVector<Diagnostic::Info>()
			) const -> void;

			template<typename NODE_T>
			auto emit_error(
				Diagnostic::Code code, const NODE_T& location, std::string&& msg, Diagnostic::Info&& info
			) const -> void {
				this->emit_error(code, location, std::move(msg), evo::SmallVector<Diagnostic::Info>{std::move(info)});
			}


			template<typename NODE_T>
			auto emit_warning(
				Diagnostic::Code code,
				const NODE_T& location,
				std::string&& msg,
				evo::SmallVector<Diagnostic::Info>&& infos = evo::SmallVector<Diagnostic::Info>()
			) const -> void;

			template<typename NODE_T>
			auto emit_warning(
				Diagnostic::Code code, const NODE_T& location, std::string&& msg, Diagnostic::Info&& info
			) const -> void {
				this->emit_warning(code, location, std::move(msg), evo::SmallVector<Diagnostic::Info>{std::move(info)});
			}




			auto add_template_location_infos(evo::SmallVector<Diagnostic::Info>& infos) const -> void;


			///////////////////////////////////
			// get source location

			EVO_NODISCARD auto get_source_location(std::nullopt_t) const -> std::optional<SourceLocation>;

			EVO_NODISCARD auto get_source_location(Token::ID token_id, const Source& src) const -> SourceLocation;
			EVO_NODISCARD auto get_source_location(Token::ID token_id) const -> SourceLocation {
				return this->get_source_location(token_id, this->source);
			}
			
			EVO_NODISCARD auto get_source_location(const AST::Node& node, const Source& src) const -> SourceLocation;
			EVO_NODISCARD auto get_source_location(const AST::Node& node) const -> SourceLocation {
				return this->get_source_location(node, this->source);
			}

			EVO_NODISCARD auto get_source_location(const AST::VarDecl& var_decl, const Source& src) const
				-> SourceLocation;
			EVO_NODISCARD auto get_source_location(const AST::VarDecl& var_decl) const -> SourceLocation {
				return this->get_source_location(var_decl, this->source);
			}
			EVO_NODISCARD auto get_source_location(const AST::FuncDecl& func_decl, const Source& src) const
				-> SourceLocation;
			EVO_NODISCARD auto get_source_location(const AST::FuncDecl& func_decl) const -> SourceLocation {
				return this->get_source_location(func_decl, this->source);
			}
			EVO_NODISCARD auto get_source_location(const AST::AliasDecl& alias_decl, const Source& src) const
				-> SourceLocation;
			EVO_NODISCARD auto get_source_location(const AST::AliasDecl& alias_decl) const -> SourceLocation {
				return this->get_source_location(alias_decl, this->source);
			}
			EVO_NODISCARD auto get_source_location(const AST::Return& return_stmt, const Source& src) const
				-> SourceLocation;
			EVO_NODISCARD auto get_source_location(const AST::Return& return_stmt) const -> SourceLocation {
				return this->get_source_location(return_stmt, this->source);
			}
			EVO_NODISCARD auto get_source_location(const AST::Block& block, const Source& src) const -> SourceLocation;
			EVO_NODISCARD auto get_source_location(const AST::Block& block) const -> SourceLocation {
				return this->get_source_location(block, this->source);
			}
			EVO_NODISCARD auto get_source_location(const AST::FuncCall& func_call, const Source& src) const
				-> SourceLocation;
			EVO_NODISCARD auto get_source_location(const AST::FuncCall& func_call) const -> SourceLocation {
				return this->get_source_location(func_call, this->source);
			}
			EVO_NODISCARD auto get_source_location(const AST::TemplatedExpr& templated_expr, const Source& src) const
				-> SourceLocation;
			EVO_NODISCARD auto get_source_location(const AST::TemplatedExpr& templated_expr) const -> SourceLocation {
				return this->get_source_location(templated_expr, this->source);
			}
			EVO_NODISCARD auto get_source_location(const AST::Prefix& prefix, const Source& src) const
				-> SourceLocation;
			EVO_NODISCARD auto get_source_location(const AST::Prefix& prefix) const -> SourceLocation {
				return this->get_source_location(prefix, this->source);
			}
			EVO_NODISCARD auto get_source_location(const AST::Infix& infix, const Source& src) const -> SourceLocation;
			EVO_NODISCARD auto get_source_location(const AST::Infix& infix) const -> SourceLocation {
				return this->get_source_location(infix, this->source);
			}
			EVO_NODISCARD auto get_source_location(const AST::Postfix& postfix, const Source& src) const
				-> SourceLocation;
			EVO_NODISCARD auto get_source_location(const AST::Postfix& postfix) const -> SourceLocation {
				return this->get_source_location(postfix, this->source);
			}
			EVO_NODISCARD auto get_source_location(const AST::MultiAssign& multi_assign, const Source& src) const
				-> SourceLocation;
			EVO_NODISCARD auto get_source_location(const AST::MultiAssign& multi_assign) const -> SourceLocation {
				return this->get_source_location(multi_assign, this->source);
			}
			EVO_NODISCARD auto get_source_location(const AST::Type& type, const Source& src) const -> SourceLocation;
			EVO_NODISCARD auto get_source_location(const AST::Type& type) const -> SourceLocation {
				return this->get_source_location(type, this->source);
			}


			EVO_NODISCARD auto get_source_location(ASG::Func::ID func_id, const Source& src) const -> SourceLocation;
			EVO_NODISCARD auto get_source_location(ASG::Func::ID func_id) const -> SourceLocation {
				return this->get_source_location(func_id, this->source);
			}
			EVO_NODISCARD auto get_source_location(ASG::Func::LinkID func_id) const -> SourceLocation;

			EVO_NODISCARD auto get_source_location(ASG::TemplatedFunc::ID templated_func_id, const Source& src) const
				-> SourceLocation;
			EVO_NODISCARD auto get_source_location(ASG::TemplatedFunc::ID templated_func_id) const -> SourceLocation {
				return this->get_source_location(templated_func_id, this->source);
			}
			EVO_NODISCARD auto get_source_location(ASG::Var::ID var_id, const Source& src) const -> SourceLocation;
			EVO_NODISCARD auto get_source_location(ASG::Var::ID var_id) const -> SourceLocation {
				return this->get_source_location(var_id, this->source);
			}
			EVO_NODISCARD auto get_source_location(ASG::Param::ID param_id, const Source& src) const -> SourceLocation;
			EVO_NODISCARD auto get_source_location(ASG::Param::ID param_id) const -> SourceLocation {
				return this->get_source_location(param_id, this->source);
			}
			EVO_NODISCARD auto get_source_location(ASG::ReturnParam::ID ret_param_id, const Source& src) const
				-> SourceLocation;
			EVO_NODISCARD auto get_source_location(ASG::ReturnParam::ID ret_param_id) const -> SourceLocation {
				return this->get_source_location(ret_param_id, this->source);
			}
			EVO_NODISCARD auto get_source_location(ScopeManager::Level::ImportInfo import_info, const Source& src) const
				-> SourceLocation;
			EVO_NODISCARD auto get_source_location(ScopeManager::Level::ImportInfo import_info) const 
			-> SourceLocation {
				return this->get_source_location(import_info, this->source);
			}

	
		private:
			Context& context;
			Source& source;

			ScopeManager::Scope scope;
			evo::SmallVector<SourceLocation> template_parents;

			std::unordered_map<std::string_view, ExprInfo> template_arg_exprs{};
			std::unordered_map<std::string_view, TypeInfo::VoidableID> template_arg_types{};
	};
	
	
	
}