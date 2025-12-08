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
			case AST::Kind::NONE:              evo::debugFatalBreak("Cannot get location of AST::Kind::None");
			case AST::Kind::VAR_DEF:           return Location::get(ast_buffer.getVarDef(node), src);
			case AST::Kind::FUNC_DEF:          return Location::get(ast_buffer.getFuncDef(node), src);
			case AST::Kind::DELETED_SPECIAL_METHOD: return Location::get(ast_buffer.getDeletedSpecialMethod(node), src);
			case AST::Kind::ALIAS_DEF:         return Location::get(ast_buffer.getAliasDef(node), src);
			case AST::Kind::DISTINCT_ALIAS_DEF: return Location::get(ast_buffer.getDistinctAliasDef(node), src);
			case AST::Kind::STRUCT_DEF:        return Location::get(ast_buffer.getStructDef(node), src);
			case AST::Kind::UNION_DEF:         return Location::get(ast_buffer.getUnionDef(node), src);
			case AST::Kind::ENUM_DEF:          return Location::get(ast_buffer.getEnumDef(node), src);
			case AST::Kind::INTERFACE_DEF:     return Location::get(ast_buffer.getInterfaceDef(node), src);
			case AST::Kind::INTERFACE_IMPL:    return Location::get(ast_buffer.getInterfaceImpl(node), src);
			case AST::Kind::RETURN:            return Location::get(ast_buffer.getReturn(node), src); 
			case AST::Kind::ERROR:             return Location::get(ast_buffer.getError(node), src);
			case AST::Kind::BREAK:             return Location::get(ast_buffer.getBreak(node), src);
			case AST::Kind::CONTINUE:          return Location::get(ast_buffer.getContinue(node), src);
			case AST::Kind::DELETE:            return Location::get(ast_buffer.getDelete(node), src);
			case AST::Kind::CONDITIONAL:       return Location::get(ast_buffer.getConditional(node), src);
			case AST::Kind::WHEN_CONDITIONAL:  return Location::get(ast_buffer.getWhenConditional(node), src);
			case AST::Kind::WHILE:             return Location::get(ast_buffer.getWhile(node), src);
			case AST::Kind::FOR:               return Location::get(ast_buffer.getFor(node), src);
			case AST::Kind::DEFER:             return Location::get(ast_buffer.getDefer(node), src);
			case AST::Kind::UNREACHABLE:       return Location::get(ast_buffer.getUnreachable(node), src);
			case AST::Kind::BLOCK:             return Location::get(ast_buffer.getBlock(node), src);
			case AST::Kind::FUNC_CALL:         return Location::get(ast_buffer.getFuncCall(node), src);
			case AST::Kind::INDEXER:           return Location::get(ast_buffer.getIndexer(node), src);
			case AST::Kind::TEMPLATE_PACK:     evo::debugFatalBreak("Cannot get location of AST::Kind::TemplatePack");
			case AST::Kind::TEMPLATED_EXPR:    return Location::get(ast_buffer.getTemplatedExpr(node), src);
			case AST::Kind::PREFIX:            return Location::get(ast_buffer.getPrefix(node), src);
			case AST::Kind::INFIX:             return Location::get(ast_buffer.getInfix(node), src);
			case AST::Kind::POSTFIX:           return Location::get(ast_buffer.getPostfix(node), src);
			case AST::Kind::MULTI_ASSIGN:      return Location::get(ast_buffer.getMultiAssign(node), src);
			case AST::Kind::NEW:               return Location::get(ast_buffer.getNew(node), src);
			case AST::Kind::ARRAY_INIT_NEW:    return Location::get(ast_buffer.getArrayInitNew(node), src);
			case AST::Kind::DESIGNATED_INIT_NEW: return Location::get(ast_buffer.getDesignatedInitNew(node), src);
			case AST::Kind::TRY_ELSE:          return Location::get(ast_buffer.getTryElse(node), src);
			case AST::Kind::DEDUCER:           return Location::get(ast_buffer.getDeducer(node), src);
			case AST::Kind::ARRAY_TYPE:        return Location::get(ast_buffer.getArrayType(node), src);
			case AST::Kind::INTERFACE_MAP:     return Location::get(ast_buffer.getInterfaceMap(node), src);
			case AST::Kind::TYPE:              return Location::get(ast_buffer.getType(node), src);
			case AST::Kind::TYPEID_CONVERTER:  return Location::get(ast_buffer.getTypeIDConverter(node), src);
			case AST::Kind::ATTRIBUTE_BLOCK:   evo::debugFatalBreak("Cannot get location of AST::Kind::AttributeBlock");
			case AST::Kind::ATTRIBUTE:         return Location::get(ast_buffer.getAttribute(node), src);
			case AST::Kind::PRIMITIVE_TYPE:    return Location::get(ast_buffer.getPrimitiveType(node), src);
			case AST::Kind::IDENT:             return Location::get(ast_buffer.getIdent(node), src);
			case AST::Kind::INTRINSIC:         return Location::get(ast_buffer.getIntrinsic(node), src);
			case AST::Kind::LITERAL:           return Location::get(ast_buffer.getLiteral(node), src);
			case AST::Kind::UNINIT:            return Location::get(ast_buffer.getUninit(node), src);
			case AST::Kind::ZEROINIT:          return Location::get(ast_buffer.getZeroinit(node), src);
			case AST::Kind::THIS:              return Location::get(ast_buffer.getThis(node), src);
			case AST::Kind::DISCARD:           return Location::get(ast_buffer.getDiscard(node), src);
		}

		evo::debugFatalBreak("Unknown or unsupported AST::Kind");
	}


	auto Diagnostic::Location::get(const AST::VarDef& var_def, const Source& src) -> Location {
		return Location::get(var_def.ident, src);
	}

	auto Diagnostic::Location::get(const AST::FuncDef& func_def, const Source& src) -> Location {
		return Location::get(func_def.name, src);
	}

	auto Diagnostic::Location::get(const AST::FuncDef::Param& param, const Source& src) -> Location {
		return Location::get(param.name, src);
	}

	auto Diagnostic::Location::get(const AST::FuncDef::Return& ret, const Source& src) -> Location {
		if(ret.ident.has_value()){
			return Location::get(*ret.ident, src);
		}else{
			return Location::get(ret.type, src);
		}
	}

	auto Diagnostic::Location::get(
		const AST::DeletedSpecialMethod& deleted_special_method, const Source& src
	) -> Location {
		return Location::get(deleted_special_method.memberToken, src);
	}

	auto Diagnostic::Location::get(const AST::AliasDef& alias_def, const Source& src) -> Location {
		return Location::get(alias_def.ident, src);
	}

	auto Diagnostic::Location::get(const AST::DistinctAliasDef& distinct_alias_def, const Source& src) -> Location {
		return Location::get(distinct_alias_def.ident, src);
	}

	auto Diagnostic::Location::get(const AST::StructDef& struct_def, const Source& src) -> Location {
		return Location::get(struct_def.ident, src);
	}

	auto Diagnostic::Location::get(const AST::UnionDef& union_def, const Source& src) -> Location {
		return Location::get(union_def.ident, src);
	}

	auto Diagnostic::Location::get(const AST::EnumDef& enum_def, const Source& src) -> Location {
		return Location::get(enum_def.ident, src);
	}

	auto Diagnostic::Location::get(const AST::InterfaceDef& interface_def, const Source& src) -> Location {
		return Location::get(interface_def.ident, src);
	}

	auto Diagnostic::Location::get(const AST::InterfaceImpl& interface_impl, const Source& src) -> Location {
		return Location::get(interface_impl.target, src);
	}

	auto Diagnostic::Location::get(const AST::Return& return_stmt, const Source& src) -> Location {
		return Location::get(return_stmt.keyword, src);
	}

	auto Diagnostic::Location::get(const AST::Error& error_stmt, const Source& src) -> Location {
		return Location::get(error_stmt.keyword, src);
	}

	auto Diagnostic::Location::get(const AST::Break& break_stmt, const Source& src) -> Location {
		return Location::get(break_stmt.keyword, src);
	}

	auto Diagnostic::Location::get(const AST::Continue& continue_stmt, const Source& src) -> Location {
		return Location::get(continue_stmt.keyword, src);
	}

	auto Diagnostic::Location::get(const AST::Delete& delete_stmt, const Source& src) -> Location {
		return Location::get(delete_stmt.keyword, src);
	}

	auto Diagnostic::Location::get(const AST::Conditional& conditional_stmt, const Source& src) -> Location {
		return Location::get(conditional_stmt.keyword, src);
	}

	auto Diagnostic::Location::get(const AST::WhenConditional& when_cond, const Source& src) -> Location {
		return Location::get(when_cond.keyword, src);
	}

	auto Diagnostic::Location::get(const AST::While& while_loop, const Source& src) -> Location {
		return Location::get(while_loop.keyword, src);
	}

	auto Diagnostic::Location::get(const AST::For& for_loop, const Source& src) -> Location {
		return Location::get(for_loop.keyword, src);
	}

	auto Diagnostic::Location::get(const AST::Defer& defer, const Source& src) -> Location {
		return Location::get(defer.keyword, src);
	}

	auto Diagnostic::Location::get(const AST::Block& block, const Source& src) -> Location {
		return Location::get(block.openBrace, src);
	}

	auto Diagnostic::Location::get(const AST::FuncCall& func_call, const Source& src) -> Location {
		return Location::get(func_call.target, src);
	}

	auto Diagnostic::Location::get(const AST::Indexer& indexer, const Source& src) -> Location {
		return Location::get(indexer.openBracket, src);
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
		return Location::get(new_expr.keyword, src);
	}

	auto Diagnostic::Location::get(const AST::ArrayInitNew& new_expr, const Source& src) -> Location {
		return Location::get(new_expr.keyword, src);
	}

	auto Diagnostic::Location::get(const AST::DesignatedInitNew& new_expr, const Source& src) -> Location {
		return Location::get(new_expr.keyword, src);
	}

	auto Diagnostic::Location::get(const AST::TryElse& try_expr, const Source& src) -> Location {
		return Location::get(try_expr.attemptExpr, src);
	}

	auto Diagnostic::Location::get(const AST::ArrayType& type, const Source& src) -> Location {
		return Location::get(type.openBracket, src);
	}

	auto Diagnostic::Location::get(const AST::InterfaceMap& type, const Source& src) -> Location {
		return Location::get(type.colonToken, src);
	}

	auto Diagnostic::Location::get(const AST::Type& type, const Source& src) -> Location {
		return Location::get(type.base, src);
	}

	auto Diagnostic::Location::get(const AST::TypeIDConverter& type, const Source& src) -> Location {
		return Location::get(type.keyword, src);
	}

	auto Diagnostic::Location::get(const AST::AttributeBlock::Attribute& attr, const Source& src) -> Location {
		return Location::get(attr.attribute, src);
	}



	//////////////////////////////////////////////////////////////////////
	// sema

	auto Diagnostic::Location::get(const sema::Var::ID& sema_var_id, const Source& src, const Context& context)
	-> Location {
		return Location::get(context.getSemaBuffer().getVar(sema_var_id).ident, src);
	}


	auto Diagnostic::Location::get(sema::GlobalVar::ID global_var_id, const Context& context) -> Location {
		return Location::get(context.getSemaBuffer().getGlobalVar(global_var_id), context);
	}

	auto Diagnostic::Location::get(const sema::GlobalVar& global_var, const Context& context) -> Location {
		if(global_var.isClangVar()){
			const ClangSource& clang_source = context.getSourceManager()[global_var.sourceID.as<ClangSource::ID>()];
			return clang_source.getDeclInfo(global_var.ident.as<ClangSource::DeclInfoID>()).location;
			
		}else{
			return Location::get(
				global_var.ident.as<Token::ID>(), context.getSourceManager()[global_var.sourceID.as<Source::ID>()]
			);
		}
	}



	auto Diagnostic::Location::get(sema::Func::ID func_id, const Context& context) -> Location {
		return Location::get(context.getSemaBuffer().getFunc(func_id), context);
	}

	auto Diagnostic::Location::get(const sema::Func& func, const Context& context) -> Location {
		return func.name.visit([&](const auto& name) -> Location {
			using NameType = std::decay_t<decltype(name)>;

			if constexpr(std::is_same<NameType, Token::ID>()){
				return Location::get(name, context.getSourceManager()[func.sourceID.as<Source::ID>()]);

			}else if constexpr(std::is_same<NameType, ClangSource::DeclInfoID>()){
				const ClangSource& clang_source = context.getSourceManager()[func.sourceID.as<ClangSource::ID>()];
				return clang_source.getDeclInfo(name).location;

			}else if constexpr(std::is_same<NameType, sema::Func::CompilerCreatedOpOverload>()){
				return Location::get(name.parentName, context.getSourceManager()[func.sourceID.as<Source::ID>()]);

			}else if constexpr(std::is_same<NameType, BuiltinModule::StringID>()){
				return Location::BUILTIN;

			}else{
				static_assert(false, "Unknown name kind");
			}
		});
	}

	auto Diagnostic::Location::get(const sema::TemplatedFunc::ID& func_id, const Source& src, const Context& context)
	-> Location {
		return Location::get(
			Diagnostic::get_ast_node_from_symbol_proc(context.getSemaBuffer().getTemplatedFunc(func_id).symbolProc),
			src
		);
	}


	auto Diagnostic::Location::get(BaseType::Alias::ID alias_id, const Context& context) -> Location {
		return Location::get(context.getTypeManager().getAlias(alias_id), context);
	}
	auto Diagnostic::Location::get(const BaseType::Alias& alias_type, const Context& context) -> Location {
		if(alias_type.isPTHRSourceType()){
			return Location::get(
				alias_type.name.as<Token::ID>(), context.getSourceManager()[alias_type.sourceID.as<Source::ID>()]
			);
		}else if(alias_type.isClangType()){
			const ClangSource& clang_source = context.getSourceManager()[alias_type.sourceID.as<ClangSource::ID>()];
			return clang_source.getDeclInfo(alias_type.name.as<ClangSource::DeclInfoID>()).location;
			
		}else{
			return Location::BUILTIN;
		}
	}


	auto Diagnostic::Location::get(BaseType::DistinctAlias::ID distinct_alias_id, const Context& context) -> Location {
		return Location::get(context.getTypeManager().getDistinctAlias(distinct_alias_id), context);
	}
	auto Diagnostic::Location::get(const BaseType::DistinctAlias& distinct_alias_type, const Context& context)
	-> Location {
		return Location::get(
			distinct_alias_type.identTokenID, context.getSourceManager()[distinct_alias_type.sourceID]
		);
	}


	auto Diagnostic::Location::get(BaseType::Struct::ID struct_id, const Context& context) -> Location {
		return Location::get(context.getTypeManager().getStruct(struct_id), context);

	}
	auto Diagnostic::Location::get(const BaseType::Struct& struct_type, const Context& context) -> Location {
		if(struct_type.isPTHRSourceType()){
			return Location::get(
				struct_type.name.as<Token::ID>(), context.getSourceManager()[struct_type.sourceID.as<Source::ID>()]
			);
		}else if(struct_type.isClangType()){
			const ClangSource& clang_source = context.getSourceManager()[struct_type.sourceID.as<ClangSource::ID>()];
			return clang_source.getDeclInfo(struct_type.name.as<ClangSource::DeclInfoID>()).location;
			
		}else{
			return Location::BUILTIN;
		}
	}


	auto Diagnostic::Location::get(BaseType::Union::ID union_id, const Context& context) -> Location {
		return Location::get(context.getTypeManager().getUnion(union_id), context);
	}
	auto Diagnostic::Location::get(const BaseType::Union& union_type, const Context& context) -> Location {
		if(union_type.isClangType()){
			const ClangSource& clang_source = context.getSourceManager()[union_type.sourceID.as<ClangSource::ID>()];
			return clang_source.getDeclInfo(union_type.location.as<ClangSource::DeclInfoID>()).location;
			
		}else{
			return Location::get(
				union_type.location.as<Token::ID>(), context.getSourceManager()[union_type.sourceID.as<Source::ID>()]
			);
		}
	}


	auto Diagnostic::Location::get(BaseType::Enum::ID enum_id, const Context& context) -> Location {
		return Location::get(context.getTypeManager().getEnum(enum_id), context);
	}
	auto Diagnostic::Location::get(const BaseType::Enum& enum_type, const Context& context) -> Location {
		if(enum_type.isClangType()){
			const ClangSource& clang_source = context.getSourceManager()[enum_type.sourceID.as<ClangSource::ID>()];
			return clang_source.getDeclInfo(enum_type.location.as<ClangSource::DeclInfoID>()).location;
			
		}else{
			return Location::get(
				enum_type.location.as<Token::ID>(), context.getSourceManager()[enum_type.sourceID.as<Source::ID>()]
			);
		}
	}


	auto Diagnostic::Location::get(BaseType::Interface::ID interface_id, const Context& context) -> Location {
		return Diagnostic::Location::get(context.getTypeManager().getInterface(interface_id), context);
	}

	auto Diagnostic::Location::get(const BaseType::Interface& interface_type, const Context& context) -> Location {
		if(interface_type.sourceID.is<SourceID>()){
			return Location::get(
				interface_type.name.as<Token::ID>(),
				context.getSourceManager()[interface_type.sourceID.as<Source::ID>()]
			);
			
		}else{
			return Location::BUILTIN;
		}
	}



	auto Diagnostic::get_ast_node_from_symbol_proc(const SymbolProc& symbol_proc) -> AST::Node {
		return symbol_proc.getASTNode();
	}


}