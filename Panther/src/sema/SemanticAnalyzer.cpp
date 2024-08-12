//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "./SemanticAnalyzer.h"

#include "../../include/AST.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::panther::sema{


	SemanticAnalyzer::SemanticAnalyzer(Context& _context, Source::ID source_id) 
		: context(_context), source(this->context.getSourceManager()[source_id]) {

		this->scope.pushScopeLevel(this->source.global_scope_level);
	};

	
	auto SemanticAnalyzer::analyze_global_declarations() -> bool {
		for(const AST::Node& global_stmt : this->source.getASTBuffer().getGlobalStmts()){

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
					return this->may_recover();
				};


				case AST::Kind::TemplatePack:   case AST::Kind::Prefix:    case AST::Kind::Type:
				case AST::Kind::AttributeBlock: case AST::Kind::Attribute: case AST::Kind::BuiltinType:
				case AST::Kind::Uninit:         case AST::Kind::Discard: {
					// TODO: message the exact kind
					this->context.emitFatal(
						Diagnostic::Code::SemaInvalidGlobalStmt,
						std::nullopt,
						Diagnostic::createFatalMessage("Invalid global statement")
					);
					return false;
				};

			}
		}

		return this->context.errored();
	}


	auto SemanticAnalyzer::analyze_global_stmts() -> bool {
		for(const GlobalScope::Func& global_func : this->source.global_scope.getFuncs()){
			if(this->analyze_func_body(global_func.ast_func, global_func.asg_func) == false){ return false; }
		}

		return this->context.errored();
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
		const Token::ID func_ident_tok_id = this->source.getASTBuffer().getIdent(func_decl.name);
		const std::string_view func_ident = this->source.getTokenBuffer()[func_ident_tok_id].getString();

		if(this->already_defined(func_ident, func_decl)){ return false; }


		///////////////////////////////////
		// template pack

		if(func_decl.templatePack.has_value()){
			this->context.emitError(
				Diagnostic::Code::MiscUnimplementedFeature,
				this->get_source_location(func_decl),
				"function declarations with template packs are currently unsupported"
			);
			return false;
		}


		///////////////////////////////////
		// params

		if(func_decl.params.empty() == false){
			this->context.emitError(
				Diagnostic::Code::MiscUnimplementedFeature,
				this->get_source_location(func_decl),
				"function declarations with parameters are currently unsupported"
			);
			return false;
		}


		///////////////////////////////////
		// attributes

		const AST::AttributeBlock& attr_block = this->source.getASTBuffer().getAttributeBlock(func_decl.attributeBlock);
		if(attr_block.attributes.empty() == false){
			this->context.emitError(
				Diagnostic::Code::MiscUnimplementedFeature,
				this->get_source_location(func_decl),
				"function declarations with attributes are currently unsupported"
			);
			return false;
		}


		///////////////////////////////////
		// returns

		auto return_params = evo::SmallVector<BaseType::Function::ReturnParam>{};
		for(size_t i = 0; const AST::FuncDecl::Return& return_param : func_decl.returns){
			const AST::Type& return_param_ast_type = this->source.getASTBuffer().getType(return_param.type);
			const evo::Result<TypeInfo::VoidableID> type_info = this->get_type_id(return_param_ast_type);
			if(type_info.isError()){ return false; }

			if(return_param.ident.has_value() && type_info.value().isVoid()){
				this->context.emitError(
					Diagnostic::Code::SemaNamedReturnParamIsTypeVoid,
					this->get_source_location(return_param.type),
					"The type of a named function return parameter cannot be \"Void\""
				);
				return false;
			}

			return_params.emplace_back(return_param.ident, type_info.value());

			i += 1;
		}


		if(return_params.size() > 1){
			this->context.emitError(
				Diagnostic::Code::MiscUnimplementedFeature,
				this->get_source_location(*func_decl.returns[1].ident),
				"multiple return parameters are currently unsupported"
			);
			return false;
		}else if(return_params[0].typeID.isVoid() == false){
			this->context.emitError(
				Diagnostic::Code::MiscUnimplementedFeature,
				this->get_source_location(func_decl.returns[0].type),
				"functions with return types other than \"Void\" are currently unsupported"
			);
			return false;
		}


		///////////////////////////////////
		// create

		const BaseType::ID base_type_id = this->context.getTypeManager().getOrCreateFunction(
			BaseType::Function(this->source.getID(), std::move(return_params))
		);

		const ASG::Func::ID asg_func_id = [&](){
			if constexpr(IS_GLOBAL){
				return this->source.asg_buffer.createFunc(func_decl.name, base_type_id);
			}else{
				evo::debugAssert(this->scope.inObjectScope(), "expected to be in object scope");
				return this->scope.getCurrentObjectScope().visit([&](auto obj_scope_id) -> ASG::Func::ID {
					using ObjScopeID = std::decay_t<decltype(obj_scope_id)>;

					if constexpr(std::is_same_v<ObjScopeID, ASG::Func::ID>){
						return this->source.asg_buffer.createFunc(func_decl.name, base_type_id, obj_scope_id);
					}
				});
			}
		}();
		
		this->getCurrentScopeLevel().addFunc(func_ident, asg_func_id);

		if constexpr(IS_GLOBAL){
			this->source.global_scope.addFunc(func_decl, asg_func_id);
			return true;
		}else{
			return this->analyze_func_body(func_decl, asg_func_id);
		}
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





	auto SemanticAnalyzer::analyze_func_body(const AST::FuncDecl& ast_func, ASG::Func::ID asg_func) -> bool {
		this->scope.pushScopeLevel(this->context.getScopeManager().createScopeLevel(), asg_func);
		EVO_DEFER([&](){ this->scope.popScopeLevel(); });

		if(this->analyze_block(this->source.getASTBuffer().getBlock(ast_func.block)) == false){
			return this->may_recover();
		}

		return true;
	}



	auto SemanticAnalyzer::analyze_block(const AST::Block& block) -> bool {
		for(const AST::Node& stmt : block.stmts){
			if(this->analyze_stmt(stmt) == false){ return false; }
		}

		return true;
	}




	auto SemanticAnalyzer::analyze_stmt(const AST::Node& node) -> bool {
		switch(node.getKind()){
			case AST::Kind::None: {
				this->context.emitFatal(
					Diagnostic::Code::SemaEncounteredKindNone,
					std::nullopt,
					Diagnostic::createFatalMessage("Encountered AST node kind of None")
				);
				return false;
			} break;


			case AST::Kind::FuncDecl: {
				return this->analyze_func_decl<false>(this->source.getASTBuffer().getFuncDecl(node));
			} break;

			case AST::Kind::VarDecl:                                 case AST::Kind::AliasDecl:
			case AST::Kind::Return:        case AST::Kind::Block:    case AST::Kind::FuncCall:
			case AST::Kind::TemplatedExpr: case AST::Kind::Infix:    case AST::Kind::MultiAssign: {
				this->context.emitError(
					Diagnostic::Code::MiscUnimplementedFeature,
					this->get_source_location(node),
					"This stmt kind is currently unsupported"
				);
				return false;
			} break;


			case AST::Kind::Literal:   case AST::Kind::This:    case AST::Kind::Ident:
			case AST::Kind::Intrinsic: case AST::Kind::Postfix: {
				// TODO: message the exact kind
				this->context.emitError(
					Diagnostic::Code::SemaInvalidStmt,
					this->get_source_location(node),
					"Invalid statement"
				);
				return this->may_recover();
			} break;


			case AST::Kind::TemplatePack:   case AST::Kind::Prefix:    case AST::Kind::Type:
			case AST::Kind::AttributeBlock: case AST::Kind::Attribute: case AST::Kind::BuiltinType:
			case AST::Kind::Uninit:         case AST::Kind::Discard: {
				// TODO: message the exact kind
				this->context.emitFatal(
					Diagnostic::Code::SemaInvalidStmt,
					std::nullopt,
					Diagnostic::createFatalMessage("Invalid statement")
				);
				return false;
			} break;
		}

		return true;
	}




	auto SemanticAnalyzer::get_type_id(const AST::Type& ast_type) -> evo::Result<TypeInfo::VoidableID> {
		auto base_type = std::optional<BaseType::ID>();

		if(ast_type.base.getKind() == AST::Kind::BuiltinType){
			const Token::ID builtin_type_token_id = ASTBuffer::getBuiltinType(ast_type.base);
			const Token& builtin_type_token = this->source.getTokenBuffer()[builtin_type_token_id];

			switch(builtin_type_token.getKind()){
				case Token::Kind::TypeVoid: {
					if(ast_type.qualifiers.empty() == false){
						this->context.emitError(
							Diagnostic::Code::SemaVoidWithQualifiers,
							this->get_source_location(ast_type.base),
							"Type \"Void\" cannot have qualifiers"
						);
						return evo::resultError;
					}
					return TypeInfo::VoidableID::Void();
				} break;

				case Token::Kind::TypeType:   case Token::Kind::TypeThis:      case Token::Kind::TypeInt:
				case Token::Kind::TypeISize:  case Token::Kind::TypeUInt:      case Token::Kind::TypeUSize:
				case Token::Kind::TypeF16:    case Token::Kind::TypeBF16:      case Token::Kind::TypeF32:
				case Token::Kind::TypeF64:    case Token::Kind::TypeF80:       case Token::Kind::TypeF128:
				case Token::Kind::TypeByte:   case Token::Kind::TypeBool:      case Token::Kind::TypeChar:
				case Token::Kind::TypeRawPtr: case Token::Kind::TypeCShort:    case Token::Kind::TypeCUShort:
				case Token::Kind::TypeCInt:   case Token::Kind::TypeCUInt:     case Token::Kind::TypeCLong:
				case Token::Kind::TypeCULong: case Token::Kind::TypeCLongLong: case Token::Kind::TypeCULongLong:
				case Token::Kind::TypeCLongDouble: {
					base_type = this->context.getTypeManager().getOrCreateBuiltinBaseType(builtin_type_token.getKind());
				} break;

				case Token::Kind::TypeI_N: case Token::Kind::TypeUI_N: {
					base_type = this->context.getTypeManager().getOrCreateBuiltinBaseType(
						builtin_type_token.getKind(), builtin_type_token.getBitWidth()
					);
				} break;

				default: {
					evo::debugFatalBreak("Unknown or unsupported BuiltinType: {}", builtin_type_token.getKind());
				} break;
			}

		}else{
			this->context.emitError(
				Diagnostic::Code::MiscUnimplementedFeature,
				this->get_source_location(ast_type.base),
				"Only builtin types are currently supported"
			);

			return evo::resultError;
		}

		evo::debugAssert(base_type.has_value(), "base type was not set");

		auto qualifiers = evo::SmallVector<TypeInfo::Qualifier>();
		for(const AST::Type::Qualifier& qualifier : ast_type.qualifiers){
			TypeInfo::Qualifier& new_qualifier = qualifiers.emplace_back();

			if(qualifier.isPtr){ new_qualifier.set(TypeInfo::QualifierFlag::Ptr); }
			if(qualifier.isReadOnly){ new_qualifier.set(TypeInfo::QualifierFlag::ReadOnly); }
			if(qualifier.isOptional){ new_qualifier.set(TypeInfo::QualifierFlag::Optional); }
		}

		return TypeInfo::VoidableID(
			this->context.getTypeManager().getOrCreateTypeInfo(
				TypeInfo(*base_type, std::move(qualifiers))
			)
		);
	}




	template<typename NODE_T>
	auto SemanticAnalyzer::already_defined(std::string_view ident, const NODE_T& node) const -> bool {
		static constexpr bool IS_FUNC = std::is_same_v<NODE_T, AST::FuncDecl>;

		for(ScopeManager::ScopeLevel::ID scope_level_id : this->scope){
			const ScopeManager::ScopeLevel& scope_level = this->context.getScopeManager()[scope_level_id];

			const std::optional<ASG::Func::ID> lookup_func = scope_level.lookupFunc(ident);
			if(lookup_func.has_value()){
				auto infos = evo::SmallVector<Diagnostic::Info>{
					Diagnostic::Info("First defined here:", this->get_source_location(lookup_func.value())),
				};

				if(scope_level_id != this->scope.getCurrentScopeLevel()){
					infos.emplace_back("Note: shadowing is not allowed");
				}

				this->context.emitError(
					Diagnostic::Code::SemaAlreadyDefined,
					this->get_source_location(node),
					std::format("Identifier \"{}\" was already defined in this scope", ident),
					infos
				);
				return true;
			}
		}

		return false;
	}


	auto SemanticAnalyzer::getCurrentScopeLevel() const -> ScopeManager::ScopeLevel& {
		return this->context.getScopeManager()[this->scope.getCurrentScopeLevel()];
	}



	auto SemanticAnalyzer::may_recover() const -> bool {
		return !this->context.hasHitFailCondition() && this->context.getConfig().mayRecover;
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
		return this->get_source_location(func_decl.name);
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
		return this->get_source_location(asg_func.name);
	}

}