//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "./SemanticAnalyzer.h"

#include "../../include/AST.h"


namespace pcit::panther::sema{


	SemanticAnalyzer::SemanticAnalyzer(Context& _context, Source::ID source_id) 
		: context(_context), source(this->context.getSourceManager()[source_id]) {

		this->scope.addScopeLevel(this->source.global_scope_level);
	};

	
	auto SemanticAnalyzer::analyze_global_declarations() -> bool {
		for(const AST::Node& global_stmt : this->source.getASTBuffer().getGlobalStmts()){

			#if defined(EVO_COMPILER_MSVC)
				#pragma warning(default : 4062)
			#endif

			switch(global_stmt.getKind()){
				case AST::Kind::None: {
					this->context.emitFatal(
						Diagnostic::Code::SemaEncounteredKindNone,
						std::nullopt,
						Diagnostic::createFatalMessage("Encountered AST node kind of None")
					);
					return false;
				} break;

				case AST::Kind::VarDecl: {
					if(this->analyze_var_decl<true>(this->source.getASTBuffer().getVarDecl(global_stmt)) == false){
						return false;
					}
				} break;

				case AST::Kind::FuncDecl: {
					if(this->analyze_func_decl<true>(this->source.getASTBuffer().getFuncDecl(global_stmt)) == false){
						return false;
					}
				} break;

				case AST::Kind::AliasDecl: {
					if(this->analyze_alias_decl<true>(this->source.getASTBuffer().getAliasDecl(global_stmt)) == false){
						return false;
					}
				} break;


				case AST::Kind::Return:        case AST::Kind::Block: case AST::Kind::FuncCall:
				case AST::Kind::TemplatedExpr: case AST::Kind::Infix: case AST::Kind::Postfix:
				case AST::Kind::MultiAssign:   case AST::Kind::Ident: case AST::Kind::Intrinsic:
				case AST::Kind::Literal:       case AST::Kind::This: {
					this->context.emitError(
						Diagnostic::Code::SemaInvalidGlobalStmt,
						this->get_source_location(this->source.getASTBuffer().getReturn(global_stmt)),
						"Invalid global statement"
					);
					return false;
				};


				case AST::Kind::TemplatePack:   case AST::Kind::Prefix:    case AST::Kind::Type:
				case AST::Kind::AttributeBlock: case AST::Kind::Attribute: case AST::Kind::BuiltinType:
				case AST::Kind::Uninit:         case AST::Kind::Discard: {
					this->context.emitFatal(
						Diagnostic::Code::SemaInvalidGlobalStmt,
						std::nullopt,
						Diagnostic::createFatalMessage("Invalid global statement: TemplatePack")
					);
					return false;
				};

			}
		}

		return true;
	}


	template<bool IS_GLOBAL>
	auto SemanticAnalyzer::analyze_var_decl(const AST::VarDecl& var_decl) -> bool {
		this->context.emitError(
			Diagnostic::Code::MiscUnimplementedFeature,
			this->get_source_location(var_decl),
			"variable declarations are currently unsupported"
		);
		return false;
	};

	template<bool IS_GLOBAL>
	auto SemanticAnalyzer::analyze_func_decl(const AST::FuncDecl& func_decl) -> bool {
		const Token::ID func_ident_tok_id = this->source.getASTBuffer().getIdent(func_decl.ident);
		const std::string_view func_ident = this->source.getTokenBuffer()[func_ident_tok_id].getString();

		if(this->already_defined(func_ident, func_decl)){ return false; }

		if(func_decl.templatePack.hasValue()){
			this->context.emitError(
				Diagnostic::Code::MiscUnimplementedFeature,
				this->get_source_location(func_decl),
				"function declarations with template packs are currently unsupported"
			);
			return false;
		}

		if(func_decl.params.empty() == false){
			this->context.emitError(
				Diagnostic::Code::MiscUnimplementedFeature,
				this->get_source_location(func_decl),
				"function declarations with parameters are currently unsupported"
			);
			return false;
		}

		const AST::AttributeBlock& attr_block = this->source.getASTBuffer().getAttributeBlock(func_decl.attributeBlock);
		if(attr_block.attributes.empty() == false){
			this->context.emitError(
				Diagnostic::Code::MiscUnimplementedFeature,
				this->get_source_location(func_decl),
				"function declarations with attributes are currently unsupported"
			);
			return false;
		}

		const ASG::Func::ID asg_func_id = this->source.asg_buffer.createFunc(func_decl.ident);
		this->getCurrentScopeLevel().addFunc(func_ident, asg_func_id);

		return true;
	};

	template<bool IS_GLOBAL>
	auto SemanticAnalyzer::analyze_alias_decl(const AST::AliasDecl& alias_decl) -> bool {
		this->context.emitError(
			Diagnostic::Code::MiscUnimplementedFeature,
			this->get_source_location(alias_decl),
			"alias declarations are currently unsupported"
		);
		return false;
	};



	template<typename NODE_T>
	auto SemanticAnalyzer::already_defined(std::string_view ident, const NODE_T& node) const -> bool {
		static constexpr bool IS_FUNC = std::is_same_v<NODE_T, AST::FuncDecl>;

		for(ScopeManager::ScopeLevel::ID scope_level_id : this->scope){
			const ScopeManager::ScopeLevel& scope_level = this->context.getScopeManager()[scope_level_id];

			const std::optional<ASG::Func::ID> lookup_func = scope_level.lookupFunc(ident);
			if(lookup_func.has_value()){
				this->context.emitError(
					Diagnostic::Code::SemaAlreadyDefined,
					this->get_source_location(node),
					std::format("Identifier \"{}\" was already defined", ident),
					evo::SmallVector<Diagnostic::Info>{
						Diagnostic::Info("First defined here:", this->get_source_location(lookup_func.value())),
					}
				);
				return true;
			}
		}

		return false;
	}


	auto SemanticAnalyzer::getCurrentScopeLevel() const -> ScopeManager::ScopeLevel& {
		return this->context.getScopeManager()[this->scope.getCurrentScopeLevel()];
	}



	//////////////////////////////////////////////////////////////////////
	// get source location

	auto SemanticAnalyzer::get_source_location(Token::ID token_id) const -> SourceLocation {
		return this->source.getTokenBuffer().getSourceLocation(token_id, this->source.getID());
	}
	
	auto SemanticAnalyzer::get_source_location(const AST::Node& node) const -> SourceLocation {
		#if defined(EVO_COMPILER_MSVC)
			#pragma warning(default : 4062)
		#endif

		const ASTBuffer& ast_buffer = this->source.getASTBuffer();

		switch(node.getKind()){
			case AST::Kind::None:           evo::debugFatalBreak("Cannot get location of AST::Kind::None");
			case AST::Kind::VarDecl:        return this->get_source_location(ast_buffer.getVarDecl(node));
			case AST::Kind::FuncDecl:       return this->get_source_location(ast_buffer.getFuncDecl(node));
			case AST::Kind::AliasDecl:      return this->get_source_location(ast_buffer.getAliasDecl(node));
			case AST::Kind::Return:         return this->get_source_location(ast_buffer.getReturn(node));
			case AST::Kind::Block:          evo::debugFatalBreak("Cannot get location of AST::Kind::Block");
			case AST::Kind::FuncCall:       return this->get_source_location(ast_buffer.getFuncCall(node));
			case AST::Kind::TemplatePack:   evo::debugFatalBreak("Cannot get location of AST::Kind::TemplatePack");
			case AST::Kind::TemplatedExpr:  return this->get_source_location(ast_buffer.getTemplatedExpr(node));
			case AST::Kind::Prefix:         return this->get_source_location(ast_buffer.getPrefix(node));
			case AST::Kind::Infix:          return this->get_source_location(ast_buffer.getInfix(node));
			case AST::Kind::Postfix:        return this->get_source_location(ast_buffer.getPostfix(node));
			case AST::Kind::MultiAssign:    return this->get_source_location(ast_buffer.getMultiAssign(node));
			case AST::Kind::Type:           return this->get_source_location(ast_buffer.getType(node));
			case AST::Kind::AttributeBlock: evo::debugFatalBreak("Cannot get location of AST::Kind::AttributeBlock");
			case AST::Kind::Attribute:      return this->get_source_location(ast_buffer.getAttribute(node));
			case AST::Kind::BuiltinType:    return this->get_source_location(ast_buffer.getBuiltinType(node));
			case AST::Kind::Ident:          return this->get_source_location(ast_buffer.getIdent(node));
			case AST::Kind::Intrinsic:      return this->get_source_location(ast_buffer.getIntrinsic(node));
			case AST::Kind::Literal:        return this->get_source_location(ast_buffer.getLiteral(node));
			case AST::Kind::Uninit:         return this->get_source_location(ast_buffer.getUninit(node));
			case AST::Kind::This:           return this->get_source_location(ast_buffer.getThis(node));
			case AST::Kind::Discard:        return this->get_source_location(ast_buffer.getDiscard(node));
		}

		evo::debugFatalBreak("Unknown or unsupported AST::Kind");
	}


	auto SemanticAnalyzer::get_source_location(const AST::VarDecl& var_decl) const -> SourceLocation {
		return this->get_source_location(var_decl.ident);
	}

	auto SemanticAnalyzer::get_source_location(const AST::FuncDecl& func_decl) const -> SourceLocation {
		return this->get_source_location(func_decl.ident);
	}

	auto SemanticAnalyzer::get_source_location(const AST::AliasDecl& alias_decl) const -> SourceLocation {
		return this->get_source_location(alias_decl.ident);
	}

	auto SemanticAnalyzer::get_source_location(const AST::Return& return_stmt) const -> SourceLocation {
		return this->get_source_location(return_stmt.keyword);
	}

	auto SemanticAnalyzer::get_source_location(const AST::FuncCall& func_call) const -> SourceLocation {
		return this->get_source_location(func_call.target);
	}

	auto SemanticAnalyzer::get_source_location(const AST::TemplatedExpr& templated_expr) const -> SourceLocation {
		return this->get_source_location(templated_expr.base);
	}

	auto SemanticAnalyzer::get_source_location(const AST::Prefix& prefix) const -> SourceLocation {
		return this->get_source_location(prefix.opTokenID);	
	}

	auto SemanticAnalyzer::get_source_location(const AST::Infix& infix) const -> SourceLocation {
		return this->get_source_location(infix.opTokenID);
	}

	auto SemanticAnalyzer::get_source_location(const AST::Postfix& postfix) const -> SourceLocation {
		return this->get_source_location(postfix.opTokenID);
	}

	auto SemanticAnalyzer::get_source_location(const AST::MultiAssign& multi_assign) const -> SourceLocation {
		return this->get_source_location(multi_assign.assigns[0]);
	}

	auto SemanticAnalyzer::get_source_location(const AST::Type& type) const -> SourceLocation {
		return this->get_source_location(type.base);
	}

	auto SemanticAnalyzer::get_source_location(ASG::Func::ID func_id) const -> SourceLocation {
		const ASG::Func& asg_func = this->source.getASGBuffer().getFunc(func_id);
		return this->get_source_location(asg_func.ident);
	}

}