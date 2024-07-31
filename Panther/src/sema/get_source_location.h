//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once


#include <PCIT_core.h>

#include "../../include/Source.h"
#include "../../include/AST.h"


namespace pcit::panther::sema{

	auto get_source_location(Token::ID token_id, const Source& source) -> SourceLocation;
	
	auto get_source_location(const AST::Node& node, const Source& source) -> SourceLocation;

	auto get_source_location(const AST::VarDecl& var_decl,               const Source& source) -> SourceLocation;
	auto get_source_location(const AST::FuncDecl& func_decl,             const Source& source) -> SourceLocation;
	auto get_source_location(const AST::AliasDecl& alias_decl,           const Source& source) -> SourceLocation;
	auto get_source_location(const AST::Return& return_stmt,             const Source& source) -> SourceLocation;
	// auto get_source_location(const AST::Block& block,                    const Source& source) -> SourceLocation;
	auto get_source_location(const AST::FuncCall& func_call,             const Source& source) -> SourceLocation;
	// auto get_source_location(const AST::TemplatePack& template_pack,     const Source& source) -> SourceLocation;
	auto get_source_location(const AST::TemplatedExpr& templated_expr,   const Source& source) -> SourceLocation;
	auto get_source_location(const AST::Prefix& prefix,                  const Source& source) -> SourceLocation;
	auto get_source_location(const AST::Infix& infix,                    const Source& source) -> SourceLocation;
	auto get_source_location(const AST::Postfix& postfix,                const Source& source) -> SourceLocation;
	auto get_source_location(const AST::MultiAssign& multi_assign,       const Source& source) -> SourceLocation;
	auto get_source_location(const AST::Type& type,                      const Source& source) -> SourceLocation;
	// auto get_source_location(const AST::AttributeBlock& attribute_block, const Source& source) -> SourceLocation;

}