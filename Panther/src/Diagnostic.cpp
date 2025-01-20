////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/Diagnostic.h"

#include "../include/Context.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::panther{
	
	//////////////////////////////////////////////////////////////////////
	// tokens

	auto Diagnostic::Location::get(Token::ID token_id, const Source& src) -> Location {
		return src.getTokenBuffer().getSourceLocation(token_id, src.getID());
	}



	//////////////////////////////////////////////////////////////////////
	// AST

	auto Diagnostic::Location::get(const AST::Node& node, const Source& src) -> Location {
		const ASTBuffer& ast_buffer = src.getASTBuffer();

		switch(node.kind()){
			case AST::Kind::None:            evo::debugFatalBreak("Cannot get location of AST::Kind::None");
			case AST::Kind::VarDecl:         return Location::get(ast_buffer.getVarDecl(node), src);
			case AST::Kind::FuncDecl:        return Location::get(ast_buffer.getFuncDecl(node), src);
			case AST::Kind::AliasDecl:       return Location::get(ast_buffer.getAliasDecl(node), src);
			case AST::Kind::TypedefDecl:     return Location::get(ast_buffer.getTypedefDecl(node), src);
			case AST::Kind::StructDecl:      return Location::get(ast_buffer.getStructDecl(node), src);
			case AST::Kind::Return:          return Location::get(ast_buffer.getReturn(node), src);
			case AST::Kind::Unreachable:     return Location::get(ast_buffer.getUnreachable(node), src);
			case AST::Kind::Conditional:     return Location::get(ast_buffer.getConditional(node), src);
			case AST::Kind::WhenConditional: return Location::get(ast_buffer.getWhenConditional(node), src);
			case AST::Kind::While:           return Location::get(ast_buffer.getWhile(node), src);
			case AST::Kind::Block:           return Location::get(ast_buffer.getBlock(node), src);
			case AST::Kind::FuncCall:        return Location::get(ast_buffer.getFuncCall(node), src);
			case AST::Kind::TemplatePack:    evo::debugFatalBreak("Cannot get location of AST::Kind::TemplatePack");
			case AST::Kind::TemplatedExpr:   return Location::get(ast_buffer.getTemplatedExpr(node), src);
			case AST::Kind::Prefix:          return Location::get(ast_buffer.getPrefix(node), src);
			case AST::Kind::Infix:           return Location::get(ast_buffer.getInfix(node), src);
			case AST::Kind::Postfix:         return Location::get(ast_buffer.getPostfix(node), src);
			case AST::Kind::MultiAssign:     return Location::get(ast_buffer.getMultiAssign(node), src);
			case AST::Kind::New:             return Location::get(ast_buffer.getNew(node), src);
			case AST::Kind::Type:            return Location::get(ast_buffer.getType(node), src);
			case AST::Kind::TypeIDConverter: return Location::get(ast_buffer.getTypeIDConverter(node), src);
			case AST::Kind::AttributeBlock:  evo::debugFatalBreak("Cannot get location of AST::Kind::AttributeBlock");
			case AST::Kind::Attribute:       return Location::get(ast_buffer.getAttribute(node), src);
			case AST::Kind::PrimitiveType:   return Location::get(ast_buffer.getPrimitiveType(node), src);
			case AST::Kind::Ident:           return Location::get(ast_buffer.getIdent(node), src);
			case AST::Kind::Intrinsic:       return Location::get(ast_buffer.getIntrinsic(node), src);
			case AST::Kind::Literal:         return Location::get(ast_buffer.getLiteral(node), src);
			case AST::Kind::Uninit:          return Location::get(ast_buffer.getUninit(node), src);
			case AST::Kind::Zeroinit:        return Location::get(ast_buffer.getZeroinit(node), src);
			case AST::Kind::This:            return Location::get(ast_buffer.getThis(node), src);
			case AST::Kind::Discard:         return Location::get(ast_buffer.getDiscard(node), src);
		}

		evo::debugFatalBreak("Unknown or unsupported AST::Kind");
	}


	auto Diagnostic::Location::get(const AST::VarDecl& var_decl, const Source& src) -> Location {
		return Location::get(var_decl.ident, src);
	}

	auto Diagnostic::Location::get(const AST::FuncDecl& func_decl, const Source& src) -> Location {
		return Location::get(func_decl.name, src);
	}

	auto Diagnostic::Location::get(const AST::AliasDecl& alias_decl, const Source& src) -> Location {
		return Location::get(alias_decl.ident, src);
	}

	auto Diagnostic::Location::get(const AST::TypedefDecl& typedef_decl, const Source& src) -> Location {
		return Location::get(typedef_decl.ident, src);
	}

	auto Diagnostic::Location::get(const AST::StructDecl& struct_decl, const Source& src) -> Location {
		return Location::get(struct_decl.ident, src);
	}

	auto Diagnostic::Location::get(const AST::Return& return_stmt, const Source& src) -> Location {
		return Location::get(return_stmt.keyword, src);
	}

	auto Diagnostic::Location::get(const AST::Conditional& conditional, const Source& src) -> Location {
		return Location::get(conditional.keyword, src);
	}

	auto Diagnostic::Location::get(const AST::WhenConditional& when_cond, const Source& src) -> Location {
		return Location::get(when_cond.keyword, src);
	}

	auto Diagnostic::Location::get(const AST::While& while_loop, const Source& src) -> Location {
		return Location::get(while_loop.keyword, src);
	}

	auto Diagnostic::Location::get(const AST::Block& block, const Source& src) -> Location {
		return Location::get(block.openBrace, src);
	}

	auto Diagnostic::Location::get(const AST::FuncCall& func_call, const Source& src) -> Location {
		return Location::get(func_call.target, src);
	}

	auto Diagnostic::Location::get(const AST::TemplatedExpr& templated_expr, const Source& src) -> Location {
		return Location::get(templated_expr.base, src);
	}

	auto Diagnostic::Location::get(const AST::Prefix& prefix, const Source& src) -> Location {
		return Location::get(prefix.opTokenID, src);
	}

	auto Diagnostic::Location::get(const AST::Infix& infix, const Source& src) -> Location {
		const Token& infix_op_token = src.getTokenBuffer()[infix.opTokenID];
		if(infix_op_token.kind() == Token::lookupKind(".")){
			return Location::get(infix.rhs, src);
		}else{
			return Location::get(infix.opTokenID, src);
		}
	}

	auto Diagnostic::Location::get(const AST::Postfix& postfix, const Source& src) -> Location {
		return Location::get(postfix.opTokenID, src);
	}

	auto Diagnostic::Location::get(const AST::MultiAssign& multi_assign, const Source& src) -> Location {
		return Location::get(multi_assign.openBracketLocation, src);
	}

	auto Diagnostic::Location::get(const AST::New& new_expr, const Source& src) -> Location {
		return Location::get(new_expr.type, src);
	}

	auto Diagnostic::Location::get(const AST::Type& type, const Source& src) -> Location {
		return Location::get(type.base, src);
	}

	auto Diagnostic::Location::get(const AST::TypeIDConverter& type, const Source& src) -> Location {
		return Location::get(type.expr, src);
	}

	auto Diagnostic::Location::get(const AST::AttributeBlock::Attribute& attr, const Source& src) -> Location {
		return Location::get(attr.attribute, src);
	}


	//////////////////////////////////////////////////////////////////////
	// DG

	auto Diagnostic::Location::get(const DG::Node::ID& id, const Context& context) -> Location {
		const DG::Node& dg_node = context.getDGBuffer()[id];
		return Location::get(dg_node.astNode, context.getSourceManager()[dg_node.sourceID]);
	}



}