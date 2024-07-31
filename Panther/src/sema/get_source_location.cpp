//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "./get_source_location.h"



namespace pcit::panther::sema{

	
	auto get_source_location(Token::ID token_id, const Source& source) -> SourceLocation {
		return source.getTokenBuffer().getSourceLocation(token_id, source.getID());
	}
	
	auto get_source_location(const AST::Node& node, const Source& source) -> SourceLocation {
		#if defined(EVO_COMPILER_MSVC)
			#pragma warning(default : 4062)
		#endif

		const ASTBuffer& ast_buffer = source.getASTBuffer();

		switch(node.getKind()){
			case AST::Kind::None:           evo::debugFatalBreak("Cannot get location of AST::Kind::None");
			case AST::Kind::VarDecl:        return get_source_location(ast_buffer.getVarDecl(node), source);
			case AST::Kind::FuncDecl:       return get_source_location(ast_buffer.getFuncDecl(node), source);
			case AST::Kind::AliasDecl:      return get_source_location(ast_buffer.getAliasDecl(node), source);
			case AST::Kind::Return:         return get_source_location(ast_buffer.getReturn(node), source);
			case AST::Kind::Block:          evo::debugFatalBreak("Cannot get location of AST::Kind::Block");
			case AST::Kind::FuncCall:       return get_source_location(ast_buffer.getFuncCall(node), source);
			case AST::Kind::TemplatePack:   evo::debugFatalBreak("Cannot get location of AST::Kind::TemplatePack");
			case AST::Kind::TemplatedExpr:  return get_source_location(ast_buffer.getTemplatedExpr(node), source);
			case AST::Kind::Prefix:         return get_source_location(ast_buffer.getPrefix(node), source);
			case AST::Kind::Infix:          return get_source_location(ast_buffer.getInfix(node), source);
			case AST::Kind::Postfix:        return get_source_location(ast_buffer.getPostfix(node), source);
			case AST::Kind::MultiAssign:    return get_source_location(ast_buffer.getMultiAssign(node), source);
			case AST::Kind::Type:           return get_source_location(ast_buffer.getType(node), source);
			case AST::Kind::AttributeBlock: evo::debugFatalBreak("Cannot get location of AST::Kind::AttributeBlock");
			case AST::Kind::Attribute:      return get_source_location(ast_buffer.getAttribute(node), source);
			case AST::Kind::BuiltinType:    return get_source_location(ast_buffer.getBuiltinType(node), source);
			case AST::Kind::Ident:          return get_source_location(ast_buffer.getIdent(node), source);
			case AST::Kind::Intrinsic:      return get_source_location(ast_buffer.getIntrinsic(node), source);
			case AST::Kind::Literal:        return get_source_location(ast_buffer.getLiteral(node), source);
			case AST::Kind::Uninit:         return get_source_location(ast_buffer.getUninit(node), source);
			case AST::Kind::This:           return get_source_location(ast_buffer.getThis(node), source);
			case AST::Kind::Discard:        return get_source_location(ast_buffer.getDiscard(node), source);
		}

		evo::debugFatalBreak("Unknown or unsupported AST::Kind");
	}


	auto get_source_location(const AST::VarDecl& var_decl, const Source& source) -> SourceLocation {
		return get_source_location(var_decl.ident, source);
	}

	auto get_source_location(const AST::FuncDecl& func_decl, const Source& source) -> SourceLocation {
		return get_source_location(func_decl.ident, source);
	}

	auto get_source_location(const AST::AliasDecl& alias_decl, const Source& source) -> SourceLocation {
		return get_source_location(alias_decl.ident, source);
	}

	auto get_source_location(const AST::Return& return_stmt, const Source& source) -> SourceLocation {
		return get_source_location(return_stmt.keyword, source);
	}

	auto get_source_location(const AST::FuncCall& func_call, const Source& source) -> SourceLocation {
		return get_source_location(func_call.target, source);
	}

	auto get_source_location(const AST::TemplatedExpr& templated_expr, const Source& source) -> SourceLocation {
		return get_source_location(templated_expr.base, source);
	}

	auto get_source_location(const AST::Prefix& prefix, const Source& source) -> SourceLocation {
		return get_source_location(prefix.opTokenID, source);	
	}

	auto get_source_location(const AST::Infix& infix, const Source& source) -> SourceLocation {
		return get_source_location(infix.opTokenID, source);
	}

	auto get_source_location(const AST::Postfix& postfix, const Source& source) -> SourceLocation {
		return get_source_location(postfix.opTokenID, source);
	}

	auto get_source_location(const AST::MultiAssign& multi_assign, const Source& source) -> SourceLocation {
		return get_source_location(multi_assign.assigns[0], source);
	}

	auto get_source_location(const AST::Type& type, const Source& source) -> SourceLocation {
		return get_source_location(type.base, source);
	}

}