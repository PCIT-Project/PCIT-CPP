////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <Evo.h>
// #include <PCIT_core.h>

#include "./source_data.h"
#include "./Token.h"
#include "./AST.h"
#include "./ASG.h"

namespace pcit::panther{

	// These are convinence functions to get the source locations of a specific item.
	// It is not necesarily the fastest way of getting the source location, so don't use in performance-critical code.
	

	///////////////////////////////////
	// Tokens

	EVO_NODISCARD auto get_source_location(Token::ID token_id, const class Source& src) -> SourceLocation;



	///////////////////////////////////
	// AST

	EVO_NODISCARD auto get_source_location(const AST::Node& node, const class Source& src) -> SourceLocation;
	
	EVO_NODISCARD auto get_source_location(const AST::VarDecl& var_decl, const class Source& src) -> SourceLocation;
	
	EVO_NODISCARD auto get_source_location(const AST::FuncDecl& func_decl, const class Source& src) -> SourceLocation;
	
	EVO_NODISCARD auto get_source_location(const AST::AliasDecl& alias_decl, const class Source& src) -> SourceLocation;
	
	EVO_NODISCARD auto get_source_location(const AST::Return& return_stmt, const class Source& src) -> SourceLocation;
	
	EVO_NODISCARD auto get_source_location(const AST::Conditional& conditional, const class Source& src)
		-> SourceLocation;
	
	EVO_NODISCARD auto get_source_location(const AST::WhenConditional& when_cond, const class Source& src)
		-> SourceLocation;

	EVO_NODISCARD auto get_source_location(const AST::While& while_loop, const class Source& src)
		-> SourceLocation;
	
	EVO_NODISCARD auto get_source_location(const AST::Block& block, const class Source& src) -> SourceLocation;
	
	EVO_NODISCARD auto get_source_location(const AST::FuncCall& func_call, const class Source& src) -> SourceLocation;
	
	EVO_NODISCARD auto get_source_location(const AST::TemplatedExpr& templated_expr, const class Source& src)
		-> SourceLocation;
	
	EVO_NODISCARD auto get_source_location(const AST::Prefix& prefix, const class Source& src) -> SourceLocation;
	
	EVO_NODISCARD auto get_source_location(const AST::Infix& infix, const class Source& src) -> SourceLocation;
	
	EVO_NODISCARD auto get_source_location(const AST::Postfix& postfix, const class Source& src) -> SourceLocation;
	
	EVO_NODISCARD auto get_source_location(const AST::MultiAssign& multi_assign, const class Source& src)
		-> SourceLocation;
	
	EVO_NODISCARD auto get_source_location(const AST::Type& type, const class Source& src) -> SourceLocation;
	
	EVO_NODISCARD auto get_source_location(const AST::TypeIDConverter& type, const class Source& src) -> SourceLocation;
	
	EVO_NODISCARD auto get_source_location(const AST::AttributeBlock::Attribute& attr, const class Source& src)
		-> SourceLocation;



	//////////////////////////////////////////////////////////////////////
	// ASG

	EVO_NODISCARD auto get_source_location(ASG::Func::ID func_id, const class Source& src) -> SourceLocation;

	EVO_NODISCARD auto get_source_location(ASG::Func::LinkID func_link_id, const class SourceManager& source_manager)
		-> SourceLocation;

	EVO_NODISCARD auto get_source_location(ASG::TemplatedFunc::ID templated_func_id, const class Source& src)
		-> SourceLocation;

	EVO_NODISCARD auto get_source_location(ASG::Var::ID var_id, const class Source& src) -> SourceLocation;

	EVO_NODISCARD auto get_source_location(
		ASG::Param::ID param_id, const class Source& src, const class TypeManager& type_manager
	) -> SourceLocation;

	EVO_NODISCARD auto get_source_location(
		ASG::ReturnParam::ID ret_param_id, const class Source& src, const class TypeManager& type_manager
	) -> SourceLocation;

	EVO_NODISCARD auto get_source_location(
		ScopeManager::Level::ImportInfo import_info, const class Source& src
	) -> SourceLocation;

	EVO_NODISCARD auto get_source_location(
		BaseType::Alias::ID alias_id, const class Source& src, const class TypeManager& type_manager
	) -> SourceLocation;

}