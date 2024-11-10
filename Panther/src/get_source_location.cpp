////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/get_source_location.h"

#include "../include/Source.h"
#include "../include/TypeManager.h"
#include "../include/SourceManager.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::panther{
	
	//////////////////////////////////////////////////////////////////////
	// tokens

	auto get_source_location(Token::ID token_id, const Source& src) -> Source::Location {
		return src.getTokenBuffer().getSourceLocation(token_id, src.getID());
	}



	//////////////////////////////////////////////////////////////////////
	// AST

	auto get_source_location(const AST::Node& node, const Source& src) -> SourceLocation {
		const ASTBuffer& ast_buffer = src.getASTBuffer();

		switch(node.kind()){
			case AST::Kind::None:            evo::debugFatalBreak("Cannot get location of AST::Kind::None");
			case AST::Kind::VarDecl:         return get_source_location(ast_buffer.getVarDecl(node), src);
			case AST::Kind::FuncDecl:        return get_source_location(ast_buffer.getFuncDecl(node), src);
			case AST::Kind::AliasDecl:       return get_source_location(ast_buffer.getAliasDecl(node), src);
			case AST::Kind::Return:          return get_source_location(ast_buffer.getReturn(node), src);
			case AST::Kind::Unreachable:     return get_source_location(ast_buffer.getUnreachable(node), src);
			case AST::Kind::Conditional:     return get_source_location(ast_buffer.getConditional(node), src);
			case AST::Kind::WhenConditional: return get_source_location(ast_buffer.getWhenConditional(node), src);
			case AST::Kind::While:           return get_source_location(ast_buffer.getWhile(node), src);
			case AST::Kind::Block:           return get_source_location(ast_buffer.getBlock(node), src);
			case AST::Kind::FuncCall:        return get_source_location(ast_buffer.getFuncCall(node), src);
			case AST::Kind::TemplatePack:    evo::debugFatalBreak("Cannot get location of AST::Kind::TemplatePack");
			case AST::Kind::TemplatedExpr:   return get_source_location(ast_buffer.getTemplatedExpr(node), src);
			case AST::Kind::Prefix:          return get_source_location(ast_buffer.getPrefix(node), src);
			case AST::Kind::Infix:           return get_source_location(ast_buffer.getInfix(node), src);
			case AST::Kind::Postfix:         return get_source_location(ast_buffer.getPostfix(node), src);
			case AST::Kind::MultiAssign:     return get_source_location(ast_buffer.getMultiAssign(node), src);
			case AST::Kind::Type:            return get_source_location(ast_buffer.getType(node), src);
			case AST::Kind::TypeIDConverter: return get_source_location(ast_buffer.getTypeIDConverter(node), src);
			case AST::Kind::AttributeBlock:  evo::debugFatalBreak("Cannot get location of AST::Kind::AttributeBlock");
			case AST::Kind::Attribute:       return get_source_location(ast_buffer.getAttribute(node), src);
			case AST::Kind::PrimitiveType:   return get_source_location(ast_buffer.getPrimitiveType(node), src);
			case AST::Kind::Ident:           return get_source_location(ast_buffer.getIdent(node), src);
			case AST::Kind::Intrinsic:       return get_source_location(ast_buffer.getIntrinsic(node), src);
			case AST::Kind::Literal:         return get_source_location(ast_buffer.getLiteral(node), src);
			case AST::Kind::Uninit:          return get_source_location(ast_buffer.getUninit(node), src);
			case AST::Kind::Zeroinit:        return get_source_location(ast_buffer.getZeroinit(node), src);
			case AST::Kind::This:            return get_source_location(ast_buffer.getThis(node), src);
			case AST::Kind::Discard:         return get_source_location(ast_buffer.getDiscard(node), src);
		}

		evo::debugFatalBreak("Unknown or unsupported AST::Kind");
	}


	auto get_source_location(const AST::VarDecl& var_decl, const Source& src) -> SourceLocation {
		return get_source_location(var_decl.ident, src);
	}

	auto get_source_location(const AST::FuncDecl& func_decl, const Source& src) -> SourceLocation {
		return get_source_location(func_decl.name, src);
	}

	auto get_source_location(const AST::AliasDecl& alias_decl, const Source& src) -> SourceLocation {
		return get_source_location(alias_decl.ident, src);
	}

	auto get_source_location(const AST::Return& return_stmt, const Source& src) -> SourceLocation {
		return get_source_location(return_stmt.keyword, src);
	}

	auto get_source_location(const AST::Conditional& conditional, const Source& src) -> SourceLocation {
		return get_source_location(conditional.keyword, src);
	}

	auto get_source_location(const AST::WhenConditional& when_cond, const Source& src) -> SourceLocation {
		return get_source_location(when_cond.keyword, src);
	}

	auto get_source_location(const AST::While& while_loop, const Source& src) -> SourceLocation {
		return get_source_location(while_loop.keyword, src);
	}

	auto get_source_location(const AST::Block& block, const Source& src) -> SourceLocation {
		return get_source_location(block.openBrace, src);
	}

	auto get_source_location(const AST::FuncCall& func_call, const Source& src) -> SourceLocation {
		return get_source_location(func_call.target, src);
	}

	auto get_source_location(const AST::TemplatedExpr& templated_expr, const Source& src) -> SourceLocation {
		return get_source_location(templated_expr.base, src);
	}

	auto get_source_location(const AST::Prefix& prefix, const Source& src) -> SourceLocation {
		return get_source_location(prefix.opTokenID, src);
	}

	auto get_source_location(const AST::Infix& infix, const Source& src) -> SourceLocation {
		const Token& infix_op_token = src.getTokenBuffer()[infix.opTokenID];
		if(infix_op_token.kind() == Token::lookupKind(".")){
			return get_source_location(infix.rhs, src);
		}else{
			return get_source_location(infix.opTokenID, src);
		}
	}

	auto get_source_location(const AST::Postfix& postfix, const Source& src) -> SourceLocation {
		return get_source_location(postfix.opTokenID, src);
	}

	auto get_source_location(const AST::MultiAssign& multi_assign, const Source& src) -> SourceLocation {
		return get_source_location(multi_assign.openBracketLocation, src);
	}

	auto get_source_location(const AST::Type& type, const Source& src) -> SourceLocation {
		return get_source_location(type.base, src);
	}

	auto get_source_location(const AST::TypeIDConverter& type, const Source& src) -> SourceLocation {
		return get_source_location(type.expr, src);
	}

	auto get_source_location(const AST::AttributeBlock::Attribute& attr, const Source& src) -> SourceLocation {
		return get_source_location(attr.attribute, src);
	}


	//////////////////////////////////////////////////////////////////////
	// ASG

	auto get_source_location(ASG::Func::ID func_id, const Source& src) -> SourceLocation {
		const ASG::Func& asg_func = src.getASGBuffer().getFunc(func_id);
		return get_source_location(asg_func.name, src);
	}

	auto get_source_location(ASG::Func::LinkID func_link_id, const SourceManager& source_manager) -> SourceLocation {
		const Source& lookup_source = source_manager[func_link_id.sourceID()];
		const ASG::Func& asg_func = lookup_source.getASGBuffer().getFunc(func_link_id.funcID());

		evo::debugAssert(asg_func.name.kind() == AST::Kind::Ident, "func name was assumed to be ident");
		const Token::ID ident_token_id = lookup_source.getASTBuffer().getIdent(asg_func.name);
		return lookup_source.getTokenBuffer().getSourceLocation(ident_token_id, func_link_id.sourceID());
	}

	auto get_source_location(ASG::TemplatedFunc::ID templated_func_id, const Source& src) -> SourceLocation {
		const ASG::TemplatedFunc& asg_templated_func = src.getASGBuffer().getTemplatedFunc(templated_func_id);
		return get_source_location(asg_templated_func.funcDecl.name, src);
	}


	auto get_source_location(ASG::Var::ID var_id, const Source& src) -> SourceLocation {
		const ASG::Var& asg_var = src.getASGBuffer().getVar(var_id);
		return get_source_location(asg_var.ident, src);
	}

	auto get_source_location(ASG::Param::ID param_id, const Source& src, const TypeManager& type_manager)
	-> SourceLocation {
		const ASG::Param& asg_param = src.getASGBuffer().getParam(param_id);
		const ASG::Func& asg_func = src.getASGBuffer().getFunc(asg_param.func);

		const BaseType::Function& func_type = type_manager.getFunction(asg_func.baseTypeID.funcID());

		evo::debugAssert(
			func_type.params[asg_param.index].ident.is<Token::ID>(), "Cannot get location of intrinsic param ident"
		);

		return get_source_location(func_type.params[asg_param.index].ident.as<Token::ID>(), src);
	}

	auto get_source_location(ASG::ReturnParam::ID ret_param_id, const Source& src, const TypeManager& type_manager)
	-> SourceLocation {
		const ASG::ReturnParam& asg_ret_param = src.getASGBuffer().getReturnParam(ret_param_id);
		const ASG::Func& asg_func = src.getASGBuffer().getFunc(asg_ret_param.func);

		const BaseType::Function& func_type = type_manager.getFunction(asg_func.baseTypeID.funcID());

		return get_source_location(*func_type.returnParams[asg_ret_param.index].ident, src);
	}

	auto get_source_location(ScopeManager::Level::ImportInfo import_info, const Source& src) -> SourceLocation {
		return get_source_location(import_info.tokenID, src);
	}

	auto get_source_location(BaseType::Alias::ID alias_id, const Source& src, const TypeManager& type_manager)
	-> SourceLocation {
		const BaseType::Alias& alias = type_manager.getAlias(alias_id);
		return get_source_location(alias.identTokenID, src);
	}


}