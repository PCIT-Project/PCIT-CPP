//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "./SemanticAnalyzer.h"

#include "../include/AST.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::panther{


	SemanticAnalyzer::SemanticAnalyzer(Context& _context, Source::ID source_id) 
		: context(_context), source(this->context.getSourceManager()[source_id]), scope(), template_parents() {

		this->scope.pushScopeLevel(this->source.global_scope_level);
	};

	SemanticAnalyzer::SemanticAnalyzer(
		Context& _context,
		Source& _source,
		const ScopeManager::Scope& _scope,
		evo::SmallVector<SourceLocation>&& _template_parents
	) : context(_context), source(_source), scope(_scope), template_parents(std::move(_template_parents)) {
		this->scope.pushScopeLevel(this->source.global_scope_level);
	};

	
	auto SemanticAnalyzer::analyze_global_declarations() -> bool {
		for(const AST::Node& global_stmt : this->source.getASTBuffer().getGlobalStmts()){

			switch(global_stmt.kind()){
				case AST::Kind::None: {
					this->emit_fatal(
						Diagnostic::Code::SemaEncounteredASTKindNone,
						std::nullopt,
						Diagnostic::createFatalMessage("Encountered AST node kind of None")
					);
					return false;
				} break;

				case AST::Kind::VarDecl: {
					// if(this->analyze_var_decl<true>(this->source.getASTBuffer().getVarDecl(global_stmt)) == false){
					// 	return false;
					// }
					this->emit_error(
						Diagnostic::Code::MiscUnimplementedFeature,
						global_stmt,
						"global variable declarations are currently unsupported"
					);
					return false;
				} break;

				case AST::Kind::FuncDecl: {
					const AST::FuncDecl& func_decl = this->source.getASTBuffer().getFuncDecl(global_stmt);
					if(this->analyze_func_decl<true>(func_decl).isError()){
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
					this->emit_error(
						Diagnostic::Code::SemaInvalidGlobalStmtKind,
						global_stmt,
						"Invalid global statement"
					);
					return this->may_recover();
				};


				case AST::Kind::TemplatePack:   case AST::Kind::Prefix:    case AST::Kind::Type:
				case AST::Kind::AttributeBlock: case AST::Kind::Attribute: case AST::Kind::BuiltinType:
				case AST::Kind::Uninit:         case AST::Kind::Discard: {
					// TODO: message the exact kind
					this->emit_fatal(
						Diagnostic::Code::SemaInvalidGlobalStmtKind,
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
		if constexpr(IS_GLOBAL){ evo::debugAssert("Don't call this function for global scope yet"); }

		const std::string_view var_ident = this->source.getTokenBuffer()[var_decl.ident].getString();
		if(this->already_defined(var_ident, var_decl)){ return false; }


		///////////////////////////////////
		// type

		auto var_type_id = std::optional<TypeInfo::ID>();
		if(var_decl.type.has_value()){
			const evo::Result<TypeInfo::VoidableID> var_type_result = 
				this->get_type_id(this->source.getASTBuffer().getType(*var_decl.type));
			if(var_type_result.isError()){ return false; }

			if(var_type_result.value().isVoid()){
				this->emit_error(
					Diagnostic::Code::SemaImproperUseOfTypeVoid,
					*var_decl.type,
					"Variables cannot be of type \"Void\""
				);
				return false;
			}

			var_type_id = var_type_result.value().typeID();
		}


		///////////////////////////////////
		// value

		if(var_decl.value.has_value() == false){
			this->emit_error(Diagnostic::Code::SemaVarWithNoValue, var_decl.ident, "Variables must have values");
			return false;	
		}

		evo::Result<ExprInfo> expr_info_result = [&](){
			if(var_decl.kind == AST::VarDecl::Kind::Def){
				return this->analyze_expr<ExprValueKind::ConstEval>(*var_decl.value);
			}else{
				return this->analyze_expr<ExprValueKind::Runtime>(*var_decl.value);
			}
		}();
		if(expr_info_result.isError()){ return false; }


		///////////////////////////////////
		// type checking

		if(expr_info_result.value().is_concrete()){
			// TODO: better messaging?
			this->emit_error(
				Diagnostic::Code::SemaIncorrectExprValueType,
				*var_decl.value,
				"Variable must be declared with an ephemeral expression value"
			);
			return false;

		}else if(expr_info_result.value().value_type == ExprInfo::ValueType::Import){
			if(var_decl.kind != AST::VarDecl::Kind::Def){
				this->emit_error(
					Diagnostic::Code::SemaIncorrectImportDecl,
					var_decl,
					"Imports must be declared as \"def\""
				);
				return false;
			}

			if(var_type_id.has_value()){
				// TODO: better error message
				this->emit_error(
					Diagnostic::Code::SemaIncorrectImportDecl,
					var_decl,
					"Imports must be declared with type inference"
				);
				return false;
			}

			this->get_current_scope_level().addImport(
				var_ident, expr_info_result.value().type_id.as<Source::ID>(), var_decl.ident
			);
			return true;
		}


		if(expr_info_result.value().value_type == ExprInfo::ValueType::FluidLiteral){
			if(var_type_id.has_value() == false){
				this->emit_error(
					Diagnostic::Code::SemaCannotInferType, *var_decl.value, "Cannot infer the type of a fluid literal"
				);
				return false;
			}

			if(this->type_check_and_set_fluid_literal_type(
				"Variable", *var_decl.value, *var_type_id, expr_info_result.value()
			) == false){
				return false;	
			}

			
		}else if(var_type_id.has_value()){
			const TypeInfo::VoidableID expr_type = expr_info_result.value().type_id.as<TypeInfo::VoidableID>();
			if(expr_type.isVoid() == false && *var_type_id != expr_type.typeID()){
				this->type_mismatch("Variable", *var_decl.value, *var_type_id, expr_info_result.value());
				return false;
			}

		}else{
			const TypeInfo::VoidableID expr_type = expr_info_result.value().type_id.as<TypeInfo::VoidableID>();
			if(expr_type.isVoid()){
				this->emit_error(
					Diagnostic::Code::SemaImproperUseOfTypeVoid,
					*var_decl.value,
					"Variables cannot be of type \"Void\""
				);
				return false;
			}

			var_type_id = expr_type.typeID();
		}


		///////////////////////////////////
		// create

		const ASG::Var::ID asg_var_id = this->source.asg_buffer.createVar(
			var_decl.kind, var_decl.ident, *var_type_id, *expr_info_result.value().expr
		);

		this->get_current_scope_level().addVar(var_ident, asg_var_id);

		const ScopeManager::Scope::ObjectScope& current_object_scope = this->scope.getCurrentObjectScope();
		ASG::Func& current_func = this->source.asg_buffer.funcs[current_object_scope.as<ASG::Func::ID>().get()];
		current_func.stmts.emplace_back(asg_var_id);
		
		return true;
	};



	template<bool IS_GLOBAL>
	auto SemanticAnalyzer::analyze_func_decl(const AST::FuncDecl& func_decl, ASG::Func::InstanceID instance_id)
	-> evo::Result<std::optional<ASG::Func::ID>> {
		const bool instantiate_template = instance_id.has_value();

		const Token::ID func_ident_tok_id = this->source.getASTBuffer().getIdent(func_decl.name);
		const std::string_view func_ident = this->source.getTokenBuffer()[func_ident_tok_id].getString();
		if(this->already_defined(func_ident, func_decl)){ return evo::resultError; }


		///////////////////////////////////
		// template pack

		auto template_params = evo::SmallVector<ASG::TemplatedFunc::TemplateParam>{};
		if(instantiate_template == false){
			if(func_decl.templatePack.has_value()){
				const AST::TemplatePack& ast_template_pack = 
					this->source.getASTBuffer().getTemplatePack(*func_decl.templatePack);

				if(ast_template_pack.params.empty()){
					this->emit_error(
						Diagnostic::Code::SemaEmptyTemplatePackDeclaration,
						func_decl,
						"Template pack declarations cannot be empty"
					);
					return evo::resultError;
				}


				this->scope.pushFakeObjectScope();
				EVO_DEFER([&](){ this->scope.popFakeObjectScope(); });

				for(const AST::TemplatePack::Param& template_param : ast_template_pack.params){
					const Token& template_param_ident_token = this->source.getTokenBuffer()[template_param.ident];
					const std::string_view template_param_ident = template_param_ident_token.getString();

					if(this->already_defined(template_param_ident, template_param.ident)){
						return evo::resultError;
					}

					const AST::Type& param_ast_type = this->source.getASTBuffer().getType(template_param.type);

					const evo::Result<bool> param_type_is_generic = this->is_type_generic(param_ast_type);
					if(param_type_is_generic.isError()){ return evo::resultError; }

					if(param_type_is_generic.value()){
						template_params.emplace_back(template_param.ident, std::nullopt);

					}else{
						const evo::Result<TypeInfo::VoidableID> param_type_id = this->get_type_id(param_ast_type);
						if(param_type_id.isError()){ return evo::resultError; }

						if(param_type_id.value().isVoid()){
							this->emit_error(
								Diagnostic::Code::SemaImproperUseOfTypeVoid,
								param_ast_type,
								"Template parameter cannot be of type \"Void\""
							);
							return evo::resultError;
						}

						template_params.emplace_back(template_param.ident, param_type_id.value().typeID());
					}
				}
			}
		}

		///////////////////////////////////
		// params

		if(func_decl.params.empty() == false){
			this->emit_error(
				Diagnostic::Code::MiscUnimplementedFeature,
				func_decl,
				"function declarations with parameters are currently unsupported"
			);
			return evo::resultError;
		}


		///////////////////////////////////
		// attributes

		bool is_pub = false;

		const AST::AttributeBlock& attr_block = this->source.getASTBuffer().getAttributeBlock(func_decl.attributeBlock);
		for(const AST::AttributeBlock::Attribute& attribute : attr_block.attributes){
			const std::string_view attribute_str = this->source.getTokenBuffer()[attribute.attribute].getString();

			if(attribute_str == "pub"){
				is_pub = true;
			}else{
				this->emit_error(
					Diagnostic::Code::SemaUnknownAttribute,
					attribute.attribute,
					std::format("Unkonwn function attribute \"#{}\"", attribute_str)
				);
			}
		}


		///////////////////////////////////
		// returns

		auto return_params = evo::SmallVector<BaseType::Function::ReturnParam>{};
		for(size_t i = 0; const AST::FuncDecl::Return& return_param : func_decl.returns){
			const AST::Type& return_param_ast_type = this->source.getASTBuffer().getType(return_param.type);
			const evo::Result<TypeInfo::VoidableID> type_info = this->get_type_id(return_param_ast_type);
			if(type_info.isError()){ return evo::resultError; }

			if(return_param.ident.has_value() && type_info.value().isVoid()){
				this->emit_error(
					Diagnostic::Code::SemaImproperUseOfTypeVoid,
					return_param.type,
					"The type of a named function return parameter cannot be \"Void\""
				);
				return evo::resultError;
			}

			return_params.emplace_back(return_param.ident, type_info.value());

			i += 1;
		}


		if(return_params.size() > 1){
			this->emit_error(
				Diagnostic::Code::MiscUnimplementedFeature,
				*func_decl.returns[1].ident,
				"multiple return parameters are currently unsupported"
			);
			return evo::resultError;
		}else if(return_params[0].typeID.isVoid() == false){
			this->emit_error(
				Diagnostic::Code::MiscUnimplementedFeature,
				func_decl.returns[0].type,
				"functions with return types other than \"Void\" are currently unsupported"
			);
			return evo::resultError;
		}


		///////////////////////////////////
		// create

		const ASG::Parent parent = this->get_parent<IS_GLOBAL>();

		if(instantiate_template == false){
			if(template_params.size() >= 1){
				const ASG::TemplatedFunc::ID asg_templated_func_id = this->source.asg_buffer.createTemplatedFunc(
					func_decl, parent, std::move(template_params), this->scope, is_pub
				);

				this->get_current_scope_level().addTemplatedFunc(func_ident, asg_templated_func_id);

				return std::optional<ASG::Func::ID>();
			}
		}


		const BaseType::ID base_type_id = this->context.getTypeManager().getOrCreateFunction(
			BaseType::Function(this->source.getID(), std::move(return_params))
		);

		const ASG::Func::ID asg_func_id = this->source.asg_buffer.createFunc(
			func_decl.name, base_type_id, parent, instance_id, is_pub
		);
		
		if(instantiate_template == false){
			this->get_current_scope_level().addFunc(func_ident, asg_func_id);
		}


		if(instantiate_template){
			return std::optional<ASG::Func::ID>(asg_func_id);

		}else{
			if constexpr(IS_GLOBAL){
				this->source.global_scope.addFunc(func_decl, asg_func_id);
				return std::optional<ASG::Func::ID>(asg_func_id);
			}else{
				if(this->analyze_func_body(func_decl, asg_func_id)){
					return std::optional<ASG::Func::ID>(asg_func_id);
				}else{
					return evo::resultError;
				}
			}
		}
	};


	template<bool IS_GLOBAL>
	auto SemanticAnalyzer::analyze_alias_decl(const AST::AliasDecl& alias_decl) -> bool {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature, alias_decl, "alias declarations are currently unsupported"
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
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();

		// TODO: order cases correctly
		switch(node.kind()){
			case AST::Kind::None: {
				this->emit_fatal(
					Diagnostic::Code::SemaEncounteredASTKindNone,
					std::nullopt,
					Diagnostic::createFatalMessage("Encountered AST node kind of None")
				);
				return false;
			} break;

			case AST::Kind::VarDecl: {
				return this->analyze_var_decl<false>(ast_buffer.getVarDecl(node));
			} break;

			case AST::Kind::FuncDecl: {
				return this->analyze_func_decl<false>(ast_buffer.getFuncDecl(node)).isSuccess();
			} break;

            case AST::Kind::AliasDecl: {
            	return this->analyze_alias_decl<false>(ast_buffer.getAliasDecl(node));
        	} break;

        	case AST::Kind::FuncCall: {
        		return this->analyze_func_call(ast_buffer.getFuncCall(node));
    		} break;

    		case AST::Kind::Infix: {
    			return this->analyze_infix_stmt(ast_buffer.getInfix(node));
			} break;


			case AST::Kind::Return:        case AST::Kind::Block:    
			case AST::Kind::TemplatedExpr: case AST::Kind::MultiAssign: {
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature, node, "This stmt kind is currently unsupported"
				);
				return false;
			} break;


			case AST::Kind::Literal:   case AST::Kind::This:    case AST::Kind::Ident:
			case AST::Kind::Intrinsic: case AST::Kind::Postfix: {
				// TODO: message the exact kind
				this->emit_error(Diagnostic::Code::SemaInvalidStmtKind, node, "Invalid statement");
				return this->may_recover();
			} break;


			case AST::Kind::TemplatePack:   case AST::Kind::Prefix:    case AST::Kind::Type:
			case AST::Kind::AttributeBlock: case AST::Kind::Attribute: case AST::Kind::BuiltinType:
			case AST::Kind::Uninit:         case AST::Kind::Discard: {
				// TODO: message the exact kind
				this->emit_fatal(
					Diagnostic::Code::SemaInvalidStmtKind,
					std::nullopt,
					Diagnostic::createFatalMessage("Invalid statement")
				);
				return false;
			} break;
		}

		return true;
	}



	auto SemanticAnalyzer::analyze_func_call(const AST::FuncCall& func_call) -> bool {
		const evo::Result<ExprInfo> target_info_res = this->analyze_expr<ExprValueKind::Runtime>(func_call.target);
		if(target_info_res.isError()){ return this->may_recover(); }

		if(target_info_res.value().type_id.is<TypeInfo::VoidableID>() == false){
			this->emit_error(
				Diagnostic::Code::SemaCannotCallLikeFunction,
				func_call.target,
				"Cannot call this expression like a function"
			);
			return this->may_recover();
		}


		const TypeInfo::VoidableID target_type_id = target_info_res.value().type_id.as<TypeInfo::VoidableID>();
		if(target_type_id.isVoid()){
			this->emit_error(
				Diagnostic::Code::SemaCannotCallLikeFunction,
				func_call.target,
				"Cannot call this expression like a function"
			);
			return this->may_recover();
		}

		const TypeInfo& target_type_info = this->context.getTypeManager().getTypeInfo(target_type_id.typeID());

		if(
			target_type_info.qualifiers().empty() == false ||
			target_type_info.baseTypeID().kind() != BaseType::Kind::Function
		){
			this->emit_error(
				Diagnostic::Code::SemaCannotCallLikeFunction,
				func_call.target,
				"Cannot call this expression like a function"
			);
			return this->may_recover();
		}


		///////////////////////////////////
		// create

		const ASG::FuncCall::ID asg_func_id = this->source.asg_buffer.createFuncCall(
			target_info_res.value().expr->funcLinkID()
		);

		const ScopeManager::Scope::ObjectScope& current_object_scope = this->scope.getCurrentObjectScope();
		ASG::Func& current_func = this->source.asg_buffer.funcs[current_object_scope.as<ASG::Func::ID>().get()];
		current_func.stmts.emplace_back(asg_func_id);

		return true;
	}



	auto SemanticAnalyzer::analyze_infix_stmt(const AST::Infix& infix) -> bool {
		if(this->source.getTokenBuffer()[infix.opTokenID].kind() != Token::lookupKind("=")){
			// TODO: better messaging
			this->emit_error(Diagnostic::Code::SemaInvalidStmtKind, infix, "Invalid stmt kind");
			return false;
		}

		///////////////////////////////////
		// assignment

		// lhs

		const evo::Result<ExprInfo> lhs_info = this->analyze_expr<ExprValueKind::Runtime>(infix.lhs);
		if(lhs_info.isError()){ return this->may_recover(); }

		if(lhs_info.value().value_type != ExprInfo::ValueType::ConcreteMutable){
			if(lhs_info.value().value_type == ExprInfo::ValueType::ConcreteConst){
				this->emit_error(
					Diagnostic::Code::SemaAssignmentDstNotConcreteMutable,
					infix.lhs,
					"Cannot assign to a constant value"
				);
				return this->may_recover();

			}else{
				this->emit_error(
					Diagnostic::Code::SemaAssignmentDstNotConcreteMutable,
					infix.lhs,
					"Cannot assign to a non-concrete value"
				);
				return this->may_recover();
			}
		}


		// rhs

		evo::Result<ExprInfo> rhs_info = this->analyze_expr<ExprValueKind::Runtime>(infix.rhs);
		if(rhs_info.isError()){ return this->may_recover(); }

		if(rhs_info.value().is_ephemeral() == false){
			this->emit_error(
				Diagnostic::Code::SemaAssignmentValueNotEphemeral,
				infix.rhs,
				"Assignment value must be ephemeral"
			);
			return this->may_recover();
		}

		const TypeInfo::VoidableID lhs_type = lhs_info.value().type_id.as<TypeInfo::VoidableID>();
		if(rhs_info.value().value_type == ExprInfo::ValueType::FluidLiteral){
			if(
				this->type_check_and_set_fluid_literal_type(
					"Assignment", infix.rhs, lhs_type.typeID(), rhs_info.value()
				) == false
			){
				return this->may_recover();
			}

		}else{ // ExprInfo::ValueType::Ephemeral
			const TypeInfo::VoidableID rhs_type = rhs_info.value().type_id.as<TypeInfo::VoidableID>();
			if(lhs_type != rhs_type){
				this->type_mismatch(
					"Assignment", infix.rhs, lhs_type.typeID(), rhs_info.value()
				);
				return this->may_recover();
			}
		}


		const ASG::Assign::ID asg_assign_id = this->source.asg_buffer.createAssign(
			*lhs_info.value().expr, *rhs_info.value().expr
		);

		const ScopeManager::Scope::ObjectScope& current_object_scope = this->scope.getCurrentObjectScope();
		ASG::Func& current_func = this->source.asg_buffer.funcs[current_object_scope.as<ASG::Func::ID>().get()];
		current_func.stmts.emplace_back(asg_assign_id);

		return true;
	}




	auto SemanticAnalyzer::get_type_id(const AST::Type& ast_type) -> evo::Result<TypeInfo::VoidableID> {
		auto base_type = std::optional<BaseType::ID>();

		switch(ast_type.base.kind()){
			case AST::Kind::BuiltinType: {
				const Token::ID builtin_type_token_id = ASTBuffer::getBuiltinType(ast_type.base);
				const Token& builtin_type_token = this->source.getTokenBuffer()[builtin_type_token_id];

				switch(builtin_type_token.kind()){
					case Token::Kind::TypeVoid: {
						if(ast_type.qualifiers.empty() == false){
							this->emit_error(
								Diagnostic::Code::SemaVoidWithQualifiers,
								ast_type.base,
								"Type \"Void\" cannot have qualifiers"
							);
							return evo::resultError;
						}
						return TypeInfo::VoidableID::Void();
					} break;

					case Token::Kind::TypeThis:       case Token::Kind::TypeInt:        case Token::Kind::TypeISize:
					case Token::Kind::TypeUInt:       case Token::Kind::TypeUSize:      case Token::Kind::TypeF16:
					case Token::Kind::TypeBF16:       case Token::Kind::TypeF32:        case Token::Kind::TypeF64:
					case Token::Kind::TypeF128:       case Token::Kind::TypeByte:       case Token::Kind::TypeBool:
					case Token::Kind::TypeChar:       case Token::Kind::TypeRawPtr:     case Token::Kind::TypeCShort:
					case Token::Kind::TypeCUShort:    case Token::Kind::TypeCInt:       case Token::Kind::TypeCUInt:
					case Token::Kind::TypeCLong:      case Token::Kind::TypeCULong:     case Token::Kind::TypeCLongLong:
					case Token::Kind::TypeCULongLong: case Token::Kind::TypeCLongDouble: {
						base_type = this->context.getTypeManager().getOrCreateBuiltinBaseType(
							builtin_type_token.kind()
						);
					} break;

					case Token::Kind::TypeI_N: case Token::Kind::TypeUI_N: {
						base_type = this->context.getTypeManager().getOrCreateBuiltinBaseType(
							builtin_type_token.kind(), builtin_type_token.getBitWidth()
						);
					} break;


					case Token::Kind::TypeType: {
						this->emit_error(
							Diagnostic::Code::SemaGenericTypeNotInTemplatePackDecl,
							ast_type,
							"Type \"Type\" may only be used in a template pack declaration"
						);
						return evo::resultError;
					} break;

					default: {
						evo::debugFatalBreak("Unknown or unsupported BuiltinType: {}", builtin_type_token.kind());
					} break;
				}
			} break;

			case AST::Kind::Ident: {
				const Token::ID base_type_token_id = ASTBuffer::getIdent(ast_type.base);
				const evo::Result<TypeInfo::VoidableID> ident_type_id = this->get_type_id(base_type_token_id);
				if(ident_type_id.isError()){ return evo::resultError; }

				if(ident_type_id.value().isVoid()){
					if(ast_type.qualifiers.empty() == false){
						this->emit_error(
							Diagnostic::Code::SemaVoidWithQualifiers,
							ast_type.base,
							"Type \"Void\" cannot have qualifiers"
						);
						return evo::resultError;
					}
					return TypeInfo::VoidableID::Void();
				}

				const TypeInfo& type_info = this->context.getTypeManager().getTypeInfo(ident_type_id.value().typeID());

				if(type_info.qualifiers().empty() == false){
					this->emit_error(
						Diagnostic::Code::MiscUnimplementedFeature,
						base_type_token_id,
						"Ident types with qualifiers are not supported"
					);
					return evo::resultError;
				}

				base_type = type_info.baseTypeID();
			} break;

			// TODO: separate out into more kinds to be more specific (errors vs fatal)
			default: {
				this->emit_error(
					Diagnostic::Code::SemaInvalidBaseType,
					ast_type.base,
					"Unknown or unsupported base type"
				);
				return evo::resultError;
			} break;
		}

		evo::debugAssert(base_type.has_value(), "base type was not set");

		return TypeInfo::VoidableID(
			this->context.getTypeManager().getOrCreateTypeInfo(TypeInfo(*base_type, ast_type.qualifiers))
		);
	}


	auto SemanticAnalyzer::get_type_id(const Token::ID& ident_token_id) -> evo::Result<TypeInfo::VoidableID> {
		const Token& type_token = this->source.getTokenBuffer()[ident_token_id];
		const std::string_view ident_str = type_token.getString();

		const auto template_type_find = this->template_arg_types.find(ident_str);
		if(template_type_find != this->template_arg_types.end()){
			return template_type_find->second;
		}

		this->emit_error(
			Diagnostic::Code::SemaIdentNotInScope,
			ident_token_id,
			std::format("Type \"{}\" was not defined in this scope", ident_str)
		);
		return evo::resultError;
	}



	auto SemanticAnalyzer::is_type_generic(const AST::Type& ast_type) -> evo::Result<bool> {
		if(ast_type.base.kind() != AST::Kind::BuiltinType){ return false; }

		const Token::ID base_type_token_id = this->source.getASTBuffer().getBuiltinType(ast_type.base);
		const Token& base_type_token = this->source.getTokenBuffer()[base_type_token_id];
		
		if(base_type_token.kind() != Token::Kind::TypeType){ return false; }

		if(ast_type.qualifiers.empty()){ return true; }

		this->emit_error(
			Diagnostic::Code::SemaGenericTypeWithQualifiers, ast_type, "Type \"Type\" cannot have qualifiers"
		);
		return evo::resultError;
	}



	auto SemanticAnalyzer::get_current_scope_level() const -> ScopeManager::ScopeLevel& {
		return this->context.getScopeManager()[this->scope.getCurrentScopeLevel()];
	}


	template<bool IS_GLOBAL>
	auto SemanticAnalyzer::get_parent() const -> ASG::Parent {
		if constexpr(IS_GLOBAL){
			return std::monostate();

		}else{
			evo::debugAssert(this->scope.inObjectScope(), "expected to be in object scope");
			return this->scope.getCurrentObjectScope().visit([&](auto obj_scope_id) -> ASG::Parent {
				using ObjScopeID = std::decay_t<decltype(obj_scope_id)>;

				if constexpr(std::is_same_v<ObjScopeID, ASG::Func::ID>){
					return obj_scope_id;
				}else{
					evo::debugFatalBreak("Unknown or unsupported object scope type");
				}
			});
		}
	}




	//////////////////////////////////////////////////////////////////////
	// expr


	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr(const AST::Node& node) -> evo::Result<ExprInfo> {
		switch(node.kind()){
			case AST::Kind::Block: {
				return this->analyze_expr_block<EXPR_VALUE_KIND>(this->source.getASTBuffer().getBlock(node));
			} break;
			
			case AST::Kind::FuncCall: {
				return this->analyze_expr_func_call<EXPR_VALUE_KIND>(this->source.getASTBuffer().getFuncCall(node));
			} break;
			
			case AST::Kind::TemplatedExpr: {
				return this->analyze_expr_templated_expr<EXPR_VALUE_KIND>(
					this->source.getASTBuffer().getTemplatedExpr(node)
				);
			} break;
			
			case AST::Kind::Prefix: {
				return this->analyze_expr_prefix<EXPR_VALUE_KIND>(this->source.getASTBuffer().getPrefix(node));
			} break;
			
			case AST::Kind::Infix: {
				return this->analyze_expr_infix<EXPR_VALUE_KIND>(this->source.getASTBuffer().getInfix(node));
			} break;
			
			case AST::Kind::Postfix: {
				return this->analyze_expr_postfix<EXPR_VALUE_KIND>(this->source.getASTBuffer().getPostfix(node));
			} break;
			
			case AST::Kind::Ident: {
				return this->analyze_expr_ident<EXPR_VALUE_KIND>(this->source.getASTBuffer().getIdent(node));
			} break;
			
			case AST::Kind::Intrinsic: {
				return this->analyze_expr_intrinsic<EXPR_VALUE_KIND>(this->source.getASTBuffer().getIntrinsic(node));
			} break;
			
			case AST::Kind::Literal: {
				return this->analyze_expr_literal<EXPR_VALUE_KIND>(this->source.getASTBuffer().getLiteral(node));
			} break;
			
			case AST::Kind::Uninit: {
				return this->analyze_expr_uninit<EXPR_VALUE_KIND>(this->source.getASTBuffer().getUninit(node));
			} break;
			
			case AST::Kind::This: {
				return this->analyze_expr_this<EXPR_VALUE_KIND>(this->source.getASTBuffer().getThis(node));
			} break;
			

			case AST::Kind::None: {
				this->emit_fatal(
					Diagnostic::Code::SemaEncounteredASTKindNone,
					std::nullopt,
					Diagnostic::createFatalMessage("Encountered AST node kind of None")
				);
				return evo::resultError;
			} break;

			case AST::Kind::VarDecl:     case AST::Kind::FuncDecl:       case AST::Kind::AliasDecl:
			case AST::Kind::Return:      case AST::Kind::TemplatePack:   case AST::Kind::MultiAssign:
			case AST::Kind::Type:        case AST::Kind::AttributeBlock: case AST::Kind::Attribute:
			case AST::Kind::BuiltinType: case AST::Kind::Discard: {
				// TODO: better messaging (specify what kind)
				this->emit_fatal(
					Diagnostic::Code::SemaInvalidExprKind,
					std::nullopt,
					Diagnostic::createFatalMessage("Encountered expr of invalid AST kind")
				);
				return evo::resultError;
			} break;
		}

		return evo::resultError;
	}



	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_block(const AST::Block& block) -> evo::Result<ExprInfo> {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature, block, "block expressions are currently unsupported"
		);
		return evo::resultError;
	}

	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_func_call(const AST::FuncCall& func_call) -> evo::Result<ExprInfo> {
		if(func_call.target.kind() != AST::Kind::Intrinsic){
			this->emit_error(
				Diagnostic::Code::MiscUnimplementedFeature,
				func_call,
				"function call expressions (that arent \"@import\") are currently unsupported"
			);
			return evo::resultError;
		}

		const Token::ID intrinsic_ident_token_id = this->source.getASTBuffer().getIntrinsic(func_call.target);
		const std::string_view intrinsic_ident = this->source.getTokenBuffer()[intrinsic_ident_token_id].getString();
		if(intrinsic_ident != "import"){
			this->emit_error(
				Diagnostic::Code::MiscUnimplementedFeature,
				func_call,
				"function call expressions (that arent \"@import\") are currently unsupported"
			);
			return evo::resultError;
		}

		const Token::ID lookup_path_token_id = this->source.getASTBuffer().getLiteral(func_call.args[0].value);
		const std::string_view lookup_path = this->source.getTokenBuffer()[lookup_path_token_id].getString();

		if(this->source.locationIsPath()){
			const evo::Expected<Source::ID, Context::LookupSourceIDError> lookup_import =
				this->context.lookupRelativeSourceID(this->source.getLocationPath(), lookup_path);

			if(lookup_import.has_value() == false){
				switch(lookup_import.error()){
					case Context::LookupSourceIDError::EmptyPath: {
						this->emit_error(
							Diagnostic::Code::SemaFailedToImportFile,
							func_call.args[0].value,
							"Empty path is an invalid lookup location"
						);
						return evo::resultError;
					} break;

					case Context::LookupSourceIDError::SameAsCaller: {
						// TODO: better messaging
						this->emit_error(
							Diagnostic::Code::SemaFailedToImportFile,
							func_call.args[0].value,
							"Cannot import self"
						);
						return evo::resultError;
					} break;

					case Context::LookupSourceIDError::NotOneOfSources: {
						this->emit_error(
							Diagnostic::Code::SemaFailedToImportFile,
							func_call.args[0].value,
							std::format("File \"{}\" is not one of the files being compiled", lookup_path)
						);
						return evo::resultError;
					} break;

					case Context::LookupSourceIDError::DoesntExist: {
						this->emit_error(
							Diagnostic::Code::SemaFailedToImportFile,
							func_call.args[0].value,
							std::format("Couldn't find file \"{}\"", lookup_path)
						);
						return evo::resultError;
					} break;
				};

				evo::debugFatalBreak("Unkonwn or unsupported error code");
			}

			return ExprInfo(ExprInfo::ValueType::Import, lookup_import.value(), std::nullopt);
			
		}else{
			const evo::Result<Source::ID> lookup_import = this->context.lookupSourceID(lookup_path);
			if(lookup_import.isError()){
				this->emit_error(
					Diagnostic::Code::SemaFailedToImportFile,
					func_call.args[0].value,
					std::format("Unknown file \"{}\"", lookup_path)
				);
				return evo::resultError;
			}

			return ExprInfo(ExprInfo::ValueType::Import, lookup_import.value(), std::nullopt);
		}
	}


	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_templated_expr(const AST::TemplatedExpr& templated_expr)
	-> evo::Result<ExprInfo> {
		const evo::Result<ExprInfo> base_info = this->analyze_expr<ExprValueKind::None>(templated_expr.base);
		if(base_info.isError()){ return evo::resultError; }

		if(base_info.value().value_type != ExprInfo::ValueType::Templated){
			// TODO: better messaging
			this->emit_error(
				Diagnostic::Code::SemaUnexpectedTemplateArgs,
				templated_expr.base,
				"Expression does not accept template arguments"
			);
			return evo::resultError;
		}

		evo::debugAssert(
			base_info.value().type_id.is<ASG::TemplatedFunc::LinkID>(),
			"currently unsupported type of templated type"
		);

		const ASG::TemplatedFunc::LinkID templated_func_link_id = 
			base_info.value().type_id.as<ASG::TemplatedFunc::LinkID>();

		Source& declared_source = this->context.getSourceManager()[templated_func_link_id.sourceID()];
		ASG::TemplatedFunc& templated_func = *declared_source.getASGBuffer().templated_funcs[
			templated_func_link_id.templatedFuncID().get()
		];

		if(templated_expr.args.size() != templated_func.templateParams.size()){
			// TODO: give exact number of arguments
			this->emit_error(
				Diagnostic::Code::SemaIncorrectTemplateInstantiation,
				templated_expr,
				"Incorrect number of template arguments"
			);
			return evo::resultError;
		}

		auto instantiation_args = evo::SmallVector<ASG::TemplatedFunc::Arg>();
		instantiation_args.reserve(templated_func.templateParams.size());
		auto value_args = evo::SmallVector<evo::Variant<TypeInfo::VoidableID, ExprInfo>>();
		value_args.reserve(templated_func.templateParams.size());
		for(size_t i = 0; i < templated_func.templateParams.size(); i+=1){
			const ASG::TemplatedFunc::TemplateParam& template_param = templated_func.templateParams[i];
			const AST::Node& template_arg = templated_expr.args[i];

			if(template_param.typeID.has_value()){ // is expression
				if(template_arg.kind() == AST::Kind::Type){
					// TODO: show declaration of template
					this->emit_error(
						Diagnostic::Code::SemaIncorrectTemplateInstantiation,
						templated_expr.args[i],
						std::format("Expected expression in template argument index {}", i),
						evo::SmallVector<Diagnostic::Info>{
							Diagnostic::Info(
								"Template parameter declaration:", this->get_source_location(template_param.ident)
							)
						}
					);
					return evo::resultError;
				}

				evo::Result<ExprInfo> arg_expr_info = this->analyze_expr<ExprValueKind::ConstEval>(template_arg);
				if(arg_expr_info.isError()){ return evo::resultError; }


				switch(arg_expr_info.value().value_type){
					case ExprInfo::ValueType::Import: case ExprInfo::ValueType::Templated: {
						this->emit_error(
							Diagnostic::Code::SemaIncorrectTemplateArgValueType,
							template_arg,
							"Invalid template argument"
						);
						return evo::resultError;
					} break;

					case ExprInfo::ValueType::FluidLiteral: {
						if(this->type_check_and_set_fluid_literal_type(
							"Template parameter", template_arg, *template_param.typeID, arg_expr_info.value()
						) == false){
							return evo::resultError;
						}
					} break;

					default: {
						if(*template_param.typeID != arg_expr_info.value().type_id.as<TypeInfo::VoidableID>()){
							this->type_mismatch(
								"Template parameter", template_arg, *template_param.typeID, arg_expr_info.value()
							);
							return evo::resultError;
						}
					} break;
				}

				evo::debugAssert(
					arg_expr_info.value().value_type == ExprInfo::ValueType::Ephemeral,
					"consteval expr is not ephemeral"
				);

				value_args.emplace_back(arg_expr_info.value());

				switch(arg_expr_info.value().expr->kind()){
					case ASG::Expr::Kind::LiteralInt: {
						const ASG::LiteralInt::ID literal_id = arg_expr_info.value().expr->literalIntID();
						instantiation_args.emplace_back(this->source.getASGBuffer().getLiteralInt(literal_id).value);
					} break;

					case ASG::Expr::Kind::LiteralFloat: {
						const ASG::LiteralFloat::ID literal_id = arg_expr_info.value().expr->literalFloatID();
						instantiation_args.emplace_back(this->source.getASGBuffer().getLiteralFloat(literal_id).value);
					} break;

					case ASG::Expr::Kind::LiteralBool: {
						const ASG::LiteralBool::ID literal_id = arg_expr_info.value().expr->literalBoolID();
						instantiation_args.emplace_back(this->source.getASGBuffer().getLiteralBool(literal_id).value);
					} break;

					case ASG::Expr::Kind::LiteralChar: {
						const ASG::LiteralChar::ID literal_id = arg_expr_info.value().expr->literalCharID();
						instantiation_args.emplace_back(this->source.getASGBuffer().getLiteralChar(literal_id).value);
					} break;

					default: {
						this->emit_fatal(
							Diagnostic::Code::SemaExpectedConstEvalValue,
							template_arg,
							Diagnostic::createFatalMessage("Evaluated consteval value was not actually consteval")
						);
						return evo::resultError;
					} break;
				}

			}else{ // is type
				switch(template_arg.kind()){
					case AST::Kind::Type: {
						const AST::Type& arg_ast_type = this->source.getASTBuffer().getType(template_arg);
						const evo::Result<TypeInfo::VoidableID> arg_type = this->get_type_id(arg_ast_type);
						if(arg_type.isError()){ return evo::resultError; }

						instantiation_args.emplace_back(arg_type.value());
						value_args.emplace_back(arg_type.value());
					} break;

					case AST::Kind::Ident: {
						const Token::ID arg_ast_type_token_id = this->source.getASTBuffer().getIdent(template_arg);
						const evo::Result<TypeInfo::VoidableID> arg_type = this->get_type_id(arg_ast_type_token_id);
						if(arg_type.isError()){ return evo::resultError; }

						instantiation_args.emplace_back(arg_type.value());
						value_args.emplace_back(arg_type.value());
					} break;

					default: {
						// TODO: show declaration of template
						this->emit_error(
							Diagnostic::Code::SemaIncorrectTemplateInstantiation,
							templated_expr.args[i],
							std::format("Expected type in template argument index {}", i),
							evo::SmallVector<Diagnostic::Info>{
								Diagnostic::Info(
									"Template parameter declaration:", this->get_source_location(template_param.ident)
								)
							}
						);
						return evo::resultError;
					} break;
				}

			}
		}

		ASG::TemplatedFunc::LookupInfo lookup_info = templated_func.lookupInstance(std::move(instantiation_args));

		auto func_id = std::optional<ASG::Func::ID>();
		if(lookup_info.needToGenerate){
			evo::SmallVector<SourceLocation> template_instance_template_parents = this->template_parents;
			template_instance_template_parents.emplace_back(this->get_source_location(templated_expr));

			auto template_sema = SemanticAnalyzer(
				this->context, declared_source, templated_func.scope, std::move(template_instance_template_parents)
			);

			// if sub-template, add parent's template params to scope
			if(
				templated_func.parent.is<std::monostate>() == false &&
				templated_func.parent == this->scope.getCurrentObjectScope()
			){
				template_sema.template_arg_exprs = this->template_arg_exprs;
				template_sema.template_arg_types = this->template_arg_types;
			}

			// add template params to scope
			for(size_t i = 0; i < templated_func.templateParams.size(); i+=1){
				const ASG::TemplatedFunc::TemplateParam& template_param = templated_func.templateParams[i];
				const evo::Variant<TypeInfo::VoidableID, ExprInfo> value_arg = value_args[i];

				const std::string_view param_ident_str = 
					declared_source.getTokenBuffer()[template_param.ident].getString();


				value_arg.visit([&](const auto& value) -> void {
					using ValueT = std::decay_t<decltype(value)>;

					if constexpr(std::is_same_v<ValueT, TypeInfo::VoidableID>){
						template_sema.template_arg_types.emplace(param_ident_str, value);
					}else{
						if(this->source.getID() == declared_source.getID()){
							template_sema.template_arg_exprs.emplace(param_ident_str, value);

						}else{ // if not the same source, need to copy over the expression to the ASGBuffer
							switch(value.expr->kind()){
								case ASG::Expr::Kind::LiteralInt: {
									const ASG::LiteralInt& literal_int =
										this->source.getASGBuffer().getLiteralInt(value.expr->literalIntID());

									const ASG::LiteralInt::ID copied_value = 
										declared_source.asg_buffer.createLiteralInt(
											literal_int.value, literal_int.typeID
										);

									template_sema.template_arg_exprs.emplace(
										param_ident_str,
										ExprInfo(value.value_type, value.type_id, ASG::Expr(copied_value))
									);
								} break;

								case ASG::Expr::Kind::LiteralFloat: {
									const ASG::LiteralFloat& literal_float =
										this->source.getASGBuffer().getLiteralFloat(value.expr->literalFloatID());

									const ASG::LiteralFloat::ID copied_value = 
										declared_source.asg_buffer.createLiteralFloat(
											literal_float.value, literal_float.typeID
										);

									template_sema.template_arg_exprs.emplace(
										param_ident_str,
										ExprInfo(value.value_type, value.type_id, ASG::Expr(copied_value))
									);
								} break;

								case ASG::Expr::Kind::LiteralBool: {
									const ASG::LiteralBool& literal_bool =
										this->source.getASGBuffer().getLiteralBool(value.expr->literalBoolID());

									const ASG::LiteralBool::ID copied_value = 
										declared_source.asg_buffer.createLiteralBool(literal_bool.value);

									template_sema.template_arg_exprs.emplace(
										param_ident_str,
										ExprInfo(value.value_type, value.type_id, ASG::Expr(copied_value))
									);
								} break;

								case ASG::Expr::Kind::LiteralChar: {
									const ASG::LiteralChar& literal_char =
										this->source.getASGBuffer().getLiteralChar(value.expr->literalCharID());

									const ASG::LiteralChar::ID copied_value = 
										declared_source.asg_buffer.createLiteralChar(literal_char.value);

									template_sema.template_arg_exprs.emplace(
										param_ident_str,
										ExprInfo(value.value_type, value.type_id, ASG::Expr(copied_value))
									);
								} break;

								default: {
									evo::debugFatalBreak("Unknown or unsupported consteval expr kind");
								} break;
							}
						}
					}
				});
			}


			// analyze func decl and get instantiation
			const evo::Result<std::optional<ASG::Func::ID>> instantiation = [&](){
				if(templated_func.scope.inObjectScope()){
					return template_sema.analyze_func_decl<false>(
						templated_func.funcDecl, lookup_info.instanceID
					);

				}else{
					return template_sema.analyze_func_decl<true>(
						templated_func.funcDecl, lookup_info.instanceID
					);
					
				}
			}();

			// set instantiation
			if(instantiation.isError()){ return evo::resultError; }
			func_id = *instantiation.value();
			lookup_info.store(*func_id);

			// analyze func body
			if(template_sema.analyze_func_body(templated_func.funcDecl, *func_id) == false){ return evo::resultError; }

		}else{
			func_id = lookup_info.waitForAndGetID();
		}

		const ASG::Func& instantiated_func = declared_source.getASGBuffer().getFunc(*func_id);

		const TypeInfo::ID instantiated_func_type = this->context.getTypeManager().getOrCreateTypeInfo(
			TypeInfo(instantiated_func.baseTypeID)
		);

		if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
			return ExprInfo(ExprInfo::ValueType::ConcreteConst, instantiated_func_type, std::nullopt);

		}else{
			return ExprInfo(
				ExprInfo::ValueType::ConcreteConst,
				instantiated_func_type,
				ASG::Expr(ASG::Func::LinkID(declared_source.getID(), *func_id))
			);
		}
	}


	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_prefix(const AST::Prefix& prefix) -> evo::Result<ExprInfo> {
		const evo::Result<ExprInfo> rhs_info = this->analyze_expr<EXPR_VALUE_KIND>(prefix.rhs);
		if(rhs_info.isError()){ return evo::resultError; }

		switch(this->source.getTokenBuffer()[prefix.opTokenID].kind()){
			case Token::lookupKind("&"): {
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature, prefix, "prefix [&] expression is currently unsupported"
				);
				return evo::resultError;
			} break;

			case Token::Kind::KeywordCopy: {
				if(rhs_info.value().is_concrete() == false){
					this->emit_error(
						Diagnostic::Code::SemaCopyExprNotConcrete,
						prefix.rhs,
						"rhs of [copy] expression must be concrete"
					);
					return evo::resultError;
				}

				if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
					return ExprInfo(ExprInfo::ValueType::Ephemeral, rhs_info.value().type_id, std::nullopt);
				}else{
					const ASG::Copy::ID asg_copy_id = this->source.asg_buffer.createCopy(*rhs_info.value().expr);

					return ExprInfo(ExprInfo::ValueType::Ephemeral, rhs_info.value().type_id, ASG::Expr(asg_copy_id));
				}
			} break;

			case Token::Kind::KeywordMove: {
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature, prefix, "prefix [move] is currently unsupported"
				);
				return evo::resultError;
			} break;

			case Token::lookupKind("-"): {
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature, prefix, "prefix [-] expression is currently unsupported"
				);
				return evo::resultError;
			} break;

			case Token::lookupKind("!"): {
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature, prefix, "prefix [!] expression is currently unsupported"
				);
				return evo::resultError;
			} break;

			case Token::lookupKind("~"): {
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature, prefix, "prefix [~] expression is currently unsupported"
				);
				return evo::resultError;
			} break;


			default: {
				evo::debugFatalBreak("Unknown or unsupported infix operator");
			} break;
		}
	}

	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_infix(const AST::Infix& infix) -> evo::Result<ExprInfo> {
		switch(this->source.getTokenBuffer()[infix.opTokenID].kind()){
			case Token::lookupKind("."): {
				const evo::Result<ExprInfo> lhs_expr_info = this->analyze_expr<EXPR_VALUE_KIND>(infix.lhs);
				if(lhs_expr_info.isError()){ return evo::resultError; }

				if(lhs_expr_info.value().value_type != ExprInfo::ValueType::Import){
					this->emit_error(
						Diagnostic::Code::SemaUnsupportedOperator,
						infix,
						"This type does not support the accessor operator ([.])",
						evo::SmallVector<Diagnostic::Info>{
							Diagnostic::Info(std::format("Type of lhs: {}", this->print_type(lhs_expr_info.value())))
						}
					);
					return evo::resultError;
				}

				const Token::ID rhs_ident_token_id = this->source.getASTBuffer().getIdent(infix.rhs);
				const std::string_view rhs_ident_str = this->source.getTokenBuffer()[rhs_ident_token_id].getString();

				const Source::ID import_target_source_id = lhs_expr_info.value().type_id.as<Source::ID>();
				const Source& import_target_source = this->context.getSourceManager()[import_target_source_id];

				const ScopeManager::ScopeLevel& scope_level = this->context.getScopeManager()[
					import_target_source.global_scope_level
				];


				// functions
				const std::optional<ASG::Func::ID> lookup_func_id = scope_level.lookupFunc(rhs_ident_str);
				if(lookup_func_id.has_value()){
					const ASG::Func& asg_func = import_target_source.getASGBuffer().getFunc(*lookup_func_id);

					if(asg_func.isPub == false){
						this->emit_error(
							Diagnostic::Code::SemaImportMemberIsntPub,
							rhs_ident_token_id,
							std::format("Function \"{}\" isn't marked as public", rhs_ident_str),
							evo::SmallVector<Diagnostic::Info>{
								Diagnostic::Info("To mark a function as public, add the attribute \"#pub\"")
							}
						);
						return evo::resultError;
					}

					const TypeInfo::ID type_id = this->context.getTypeManager().getOrCreateTypeInfo(
						TypeInfo(asg_func.baseTypeID)
					);


					if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
						return ExprInfo(ExprInfo::ValueType::ConcreteConst, type_id, std::nullopt);
					}else{
						return ExprInfo(
							ExprInfo::ValueType::ConcreteConst,
							type_id,
							ASG::Expr(ASG::Func::LinkID(import_target_source_id, *lookup_func_id))
						);
					}
				}

				// templated functions
				const std::optional<ASG::TemplatedFunc::ID> lookup_templated_func_id = 
					scope_level.lookupTemplatedFunc(rhs_ident_str);
				if(lookup_templated_func_id.has_value()){
					const ASG::TemplatedFunc& asg_templated_func = 
						import_target_source.getASGBuffer().getTemplatedFunc(*lookup_templated_func_id);

					if(asg_templated_func.isPub == false){
						this->emit_error(
							Diagnostic::Code::SemaImportMemberIsntPub,
							rhs_ident_token_id,
							std::format("Templated function \"{}\" isn't marked as public", rhs_ident_str),
							evo::SmallVector<Diagnostic::Info>{
								Diagnostic::Info("To mark a function as public, add the attribute \"#pub\"")
							}
						);
						return evo::resultError;
					}

					if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
						return ExprInfo(
							ExprInfo::ValueType::Templated,
							ASG::TemplatedFunc::LinkID(import_target_source_id, *lookup_templated_func_id),
							std::nullopt
						);

					}else{
						this->emit_error(
							Diagnostic::Code::SemaExpectedTemplateArgs,
							rhs_ident_token_id,
							std::format(
								"Identifier \"{}\" is a templated function and requires template arguments",
								rhs_ident_str
							),
							evo::SmallVector<Diagnostic::Info>{
								Diagnostic::Info("Defined here:", this->get_source_location(*lookup_templated_func_id)),
							}
						);
						return evo::resultError;
					}
				}


				this->emit_error(
					Diagnostic::Code::SemaImportMemberDoesntExist,
					rhs_ident_token_id,
					std::format(
						"Imported source doesn't have Identifier \"{}\" declared in global scope", rhs_ident_str
					)
				);
				return evo::resultError;
			} break;

			default: {
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature,
					infix,
					std::format(
						"Infix [{}] expressions are currently unsupported", 
						this->source.getTokenBuffer()[infix.opTokenID].kind()
					)
				);
				return evo::resultError;
			} break;
		}
	}

	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_postfix(const AST::Postfix& postfix) -> evo::Result<ExprInfo> {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature, postfix, "postfix expressions are currently unsupported"
		);
		return evo::resultError;
	}

	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_ident(const Token::ID& ident) -> evo::Result<ExprInfo> {
		const std::string_view ident_str = this->source.getTokenBuffer()[ident].getString();

		// template exprs
		const auto template_expr_find = this->template_arg_exprs.find(ident_str);
		if(template_expr_find != this->template_arg_exprs.end()){
			if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
				const ExprInfo& template_expr_info = template_expr_find->second;
				return ExprInfo(template_expr_info.value_type, template_expr_info.type_id, std::nullopt);
			}else{
				return template_expr_find->second;
			}
		}

		for(size_t i = this->scope.size() - 1; ScopeManager::ScopeLevel::ID scope_level_id : this->scope){
			const evo::Result<std::optional<ExprInfo>> scope_level_lookup = 
				this->analyze_expr_ident_in_scope_level<EXPR_VALUE_KIND>(
					this->source.getID(),
					ident,
					ident_str,
					scope_level_id,
					i >= this->scope.getCurrentObjectScopeIndex() || i == 0
				);

			if(scope_level_lookup.isError()){ return evo::resultError; }
			if(scope_level_lookup.value().has_value()){ return *scope_level_lookup.value(); }

			i -= 1;
		}

		this->emit_error(
			Diagnostic::Code::SemaIdentNotInScope,
			ident,
			std::format("Identifier \"{}\" was not defined in this scope", ident_str)
		);
		return evo::resultError;
	}



	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_ident_in_scope_level(
		Source::ID source_id,
		const Token::ID& ident,
		std::string_view ident_str,
		ScopeManager::ScopeLevel::ID scope_level_id,
		bool variables_in_scope
	) -> evo::Result<std::optional<ExprInfo>> {
		const ScopeManager::ScopeLevel& scope_level = this->context.getScopeManager()[scope_level_id];

		// functions
		const std::optional<ASG::Func::ID> lookup_func_id = scope_level.lookupFunc(ident_str);
		if(lookup_func_id.has_value()){
			const ASG::Func& asg_func = this->source.getASGBuffer().getFunc(*lookup_func_id);

			const TypeInfo::ID type_id = this->context.getTypeManager().getOrCreateTypeInfo(
				TypeInfo(asg_func.baseTypeID)
			);

			if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
				return std::optional<ExprInfo>(ExprInfo(ExprInfo::ValueType::ConcreteConst, type_id, std::nullopt));
			}else{
				return std::optional<ExprInfo>(
					ExprInfo(
						ExprInfo::ValueType::ConcreteConst,
						type_id,
						ASG::Expr(ASG::Func::LinkID(source_id, *lookup_func_id))
					)
				);
			}
		}

		// templated functions
		const std::optional<ASG::TemplatedFunc::ID> lookup_templated_func_id = 
			scope_level.lookupTemplatedFunc(ident_str);
		if(lookup_templated_func_id.has_value()){
			if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
				return std::optional<ExprInfo>(
					ExprInfo(
						ExprInfo::ValueType::Templated,
						ASG::TemplatedFunc::LinkID(source_id, *lookup_templated_func_id),
						std::nullopt
					)
				);

			}else{
				this->emit_error(
					Diagnostic::Code::SemaExpectedTemplateArgs,
					ident,
					std::format(
						"Identifier \"{}\" is a templated function and requires template arguments", ident_str
					),
					evo::SmallVector<Diagnostic::Info>{
						Diagnostic::Info("Defined here:", this->get_source_location(*lookup_templated_func_id)),
					}
				);
				return evo::resultError;
			}
		}

		// variables
		const std::optional<ASG::Var::ID> lookup_var_id = scope_level.lookupVar(ident_str);
		if(lookup_var_id.has_value()){
			if(variables_in_scope){
				const ASG::Var& asg_var = this->source.getASGBuffer().getVar(*lookup_var_id);

				const ExprInfo::ValueType value_type = asg_var.kind == AST::VarDecl::Kind::Var 
				                                       ? ExprInfo::ValueType::ConcreteMutable 
				                                       : ExprInfo::ValueType::ConcreteConst;

				if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
					return std::optional<ExprInfo>(ExprInfo(value_type, asg_var.typeID, std::nullopt));

				}else if constexpr(EXPR_VALUE_KIND == ExprValueKind::ConstEval){
					if(asg_var.kind != AST::VarDecl::Kind::Def){
						// TODO: better messaging
						this->emit_error(
							Diagnostic::Code::SemaConstEvalVarNotDef,
							ident,
							"Cannot get a consteval value from a variable that isn't def",
							evo::SmallVector<Diagnostic::Info>{
								Diagnostic::Info("Declared here:", this->get_source_location(*lookup_var_id))
							}
						);
						return evo::resultError;
					}

					return std::optional<ExprInfo>(
						ExprInfo(ExprInfo::ValueType::Ephemeral, asg_var.typeID, asg_var.expr)
					);

				}else{
					return std::optional<ExprInfo>(
						ExprInfo(value_type, asg_var.typeID, ASG::Expr(ASG::Var::LinkID(source_id, *lookup_var_id)))
					);
				}
			}else{
				// TODO: better messaging
				this->emit_error(
					Diagnostic::Code::SemaIdentNotInScope,
					ident,
					std::format("Identifier \"{}\" was not defined in this scope", ident_str),
					evo::SmallVector<Diagnostic::Info>{
						Diagnostic::Info(
							"Local variables / members cannot be accessed inside a sub-object scope. Defined here:",
							this->get_source_location(*lookup_var_id)
						),
					}
				);
				return evo::resultError;
			}
		}


		// imports
		const std::optional<Source::ID> lookup_import = scope_level.lookupImport(ident_str);
		if(lookup_import.has_value()){
			return std::optional<ExprInfo>(ExprInfo(ExprInfo::ValueType::Import, lookup_import.value(), std::nullopt));
		}

		return std::optional<ExprInfo>();
	}




	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_intrinsic(const Token::ID& intrinsic) -> evo::Result<ExprInfo> {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature, intrinsic, "intrinsic expressions are currently unsupported"
		);
		return evo::resultError;
	}

	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_literal(const Token::ID& literal) -> evo::Result<ExprInfo> {
		const Token& token = this->source.getTokenBuffer()[literal];
		
		auto expr_info = ExprInfo(ExprInfo::ValueType::Ephemeral, std::monostate(), std::nullopt);

		switch(token.kind()){
			case Token::Kind::LiteralInt: {
				expr_info.value_type = ExprInfo::ValueType::FluidLiteral;

				if constexpr(EXPR_VALUE_KIND != ExprValueKind::None){
					expr_info.expr = ASG::Expr(
						this->source.asg_buffer.createLiteralInt(token.getInt(), std::nullopt)
					);
				}
			} break;

			case Token::Kind::LiteralFloat: {
				expr_info.value_type = ExprInfo::ValueType::FluidLiteral;

				if constexpr(EXPR_VALUE_KIND != ExprValueKind::None){
					expr_info.expr = ASG::Expr(
						this->source.asg_buffer.createLiteralFloat(token.getFloat(), std::nullopt)
					);
				}
			} break;

			case Token::Kind::LiteralBool: {
				expr_info.type_id = this->context.getTypeManager().getTypeBool();

				if constexpr(EXPR_VALUE_KIND != ExprValueKind::None){
					expr_info.expr = ASG::Expr(this->source.asg_buffer.createLiteralBool(token.getBool()));
				}
			} break;

			case Token::Kind::LiteralString: {
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature, literal, "literal strings are currently unsupported"
				);
				return evo::resultError;
			} break;

			case Token::Kind::LiteralChar: {
				expr_info.type_id = this->context.getTypeManager().getTypeChar();

				if constexpr(EXPR_VALUE_KIND != ExprValueKind::None){
					expr_info.expr = ASG::Expr(this->source.asg_buffer.createLiteralChar(token.getString()[0]));
				}
			} break;

			default: {
				evo::debugFatalBreak("Token is not a literal");
			} break;
		}

		return expr_info;
	}

	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_uninit(const Token::ID& uninit) -> evo::Result<ExprInfo> {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature, uninit, "[uninit] expressions are currently unsupported"
		);
		return evo::resultError;
	}

	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_this(const Token::ID& this_expr) -> evo::Result<ExprInfo> {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature, this_expr, "[this] expressions are currently unsupported"
		);
		return evo::resultError;
	}





	//////////////////////////////////////////////////////////////////////
	// error handling

	template<typename NODE_T>
	auto SemanticAnalyzer::type_check_and_set_fluid_literal_type(
		std::string_view name, const NODE_T& location, TypeInfo::ID target_type_id, ExprInfo& expr_info
	) -> bool {
		evo::debugAssert(expr_info.value_type == ExprInfo::ValueType::FluidLiteral, "epxr_info is not a fluid literal");

		const TypeInfo& var_type_info = this->context.getTypeManager().getTypeInfo(target_type_id);

		if(
			var_type_info.qualifiers().empty() == false || 
			var_type_info.baseTypeID().kind() != BaseType::Kind::Builtin
		){
			this->type_mismatch(name, location, target_type_id, expr_info);
			return false;
		}


		const BaseType::Builtin::ID var_type_builtin_id = var_type_info.baseTypeID().id<BaseType::Builtin::ID>();
		const BaseType::Builtin& var_type_builtin = this->context.getTypeManager().getBuiltin(var_type_builtin_id);

		if(expr_info.expr->kind() == ASG::Expr::Kind::LiteralInt){
			switch(var_type_builtin.kind()){
				case Token::Kind::TypeInt:
				case Token::Kind::TypeISize:
				case Token::Kind::TypeI_N:
				case Token::Kind::TypeUInt:
				case Token::Kind::TypeUSize:
				case Token::Kind::TypeUI_N:
				case Token::Kind::TypeByte:
				case Token::Kind::TypeCShort:
				case Token::Kind::TypeCUShort:
				case Token::Kind::TypeCInt:
				case Token::Kind::TypeCUInt:
				case Token::Kind::TypeCLong:
				case Token::Kind::TypeCULong:
				case Token::Kind::TypeCLongLong:
				case Token::Kind::TypeCULongLong:
				case Token::Kind::TypeCLongDouble:
					break;

				default: {
					this->type_mismatch(name, location, target_type_id, expr_info);
					return false;
				}
			}

			const ASG::LiteralInt::ID literal_int_id = expr_info.expr->literalIntID();
			this->source.asg_buffer.literal_ints[literal_int_id.get()].typeID = target_type_id;


		}else{
			evo::debugAssert(
				expr_info.expr->kind() == ASG::Expr::Kind::LiteralFloat, "Expected literal float"
			);

			switch(var_type_builtin.kind()){
				case Token::Kind::TypeF16:
				case Token::Kind::TypeBF16:
				case Token::Kind::TypeF32:
				case Token::Kind::TypeF64:
				case Token::Kind::TypeF128:
					break;

				default: {
					this->type_mismatch(name, location, target_type_id, expr_info);
					return false;
				}
			}

			const ASG::LiteralFloat::ID literal_float_id = expr_info.expr->literalFloatID();
			this->source.asg_buffer.literal_floats[literal_float_id.get()].typeID = target_type_id;
		}

		expr_info.value_type = ExprInfo::ValueType::Ephemeral;
		expr_info.type_id = TypeInfo::VoidableID(target_type_id);

		return true;
	}



	template<typename NODE_T>
	auto SemanticAnalyzer::already_defined(std::string_view ident_str, const NODE_T& node) const -> bool {
		const auto template_type_find = this->template_arg_types.find(ident_str);
		if(template_type_find != this->template_arg_types.end()){
			this->emit_error(
				Diagnostic::Code::SemaAlreadyDefined,
				node,
				std::format("Identifier \"{}\" was already defined in this scope", ident_str),
				evo::SmallVector<Diagnostic::Info>{
					Diagnostic::Info(std::format("\"{}\" is a template parameter", ident_str)), // TODO: definition location
					Diagnostic::Info("Note: shadowing is not allowed")
				}
			);
			return true;
		}

		for(size_t i = this->scope.size() - 1; ScopeManager::ScopeLevel::ID scope_level_id : this->scope){
			const ScopeManager::ScopeLevel& scope_level = this->context.getScopeManager()[scope_level_id];

			// functions
			const std::optional<ASG::Func::ID> lookup_func = scope_level.lookupFunc(ident_str);
			if(lookup_func.has_value()){
				auto infos = evo::SmallVector<Diagnostic::Info>{
					Diagnostic::Info("First defined here:", this->get_source_location(lookup_func.value())),
				};

				if(scope_level_id != this->scope.getCurrentScopeLevel()){
					infos.emplace_back("Note: shadowing is not allowed");
				}

				this->emit_error(
					Diagnostic::Code::SemaAlreadyDefined,
					node,
					std::format("Identifier \"{}\" was already defined in this scope", ident_str),
					std::move(infos)
				);
				return true;
			}

			// variables
			if(i >= this->scope.getCurrentObjectScopeIndex() || i == 0){
				const std::optional<ASG::Var::ID> lookup_var = scope_level.lookupVar(ident_str);
				if(lookup_var.has_value()){
					auto infos = evo::SmallVector<Diagnostic::Info>{
						Diagnostic::Info("First defined here:", this->get_source_location(lookup_var.value())),
					};

					if(scope_level_id != this->scope.getCurrentScopeLevel()){
						infos.emplace_back("Note: shadowing is not allowed");
					}

					this->emit_error(
						Diagnostic::Code::SemaAlreadyDefined,
						node,
						std::format("Identifier \"{}\" was already defined in this scope", ident_str),
						std::move(infos)
					);
					return true;
				}
			}

			// imports
			const std::optional<Source::ID> lookup_import = scope_level.lookupImport(ident_str);
			if(lookup_import.has_value()){
				auto infos = evo::SmallVector<Diagnostic::Info>{
					Diagnostic::Info(
						"First defined here:", 
						this->get_source_location(this->get_current_scope_level().getImportLocation(ident_str))
					),
				};

				if(scope_level_id != this->scope.getCurrentScopeLevel()){
					infos.emplace_back("Note: shadowing is not allowed");
				}

				this->emit_error(
					Diagnostic::Code::SemaAlreadyDefined,
					node,
					std::format("Identifier \"{}\" was already defined in this scope", ident_str),
					std::move(infos)
				);

				return true;
			}

			i -= 1;
		}

		return false;
	}



	template<typename NODE_T>
	auto SemanticAnalyzer::type_mismatch(
		std::string_view name, const NODE_T& location, TypeInfo::ID expected, const ExprInfo& got
	) -> void {
		std::string expected_type_str = std::format("{} is of type: ", name);
		std::string got_type_str = std::format("Expression is of type: ", name);

		while(expected_type_str.size() < got_type_str.size()){
			expected_type_str += ' ';
		}

		while(got_type_str.size() < expected_type_str.size()){
			got_type_str += ' ';
		}

		auto infos = evo::SmallVector<Diagnostic::Info>();
		infos.emplace_back(expected_type_str + this->context.getTypeManager().printType(expected));
		infos.emplace_back(got_type_str + this->print_type(got));

		this->emit_error(
			Diagnostic::Code::SemaTypeMismatch,
			location,
			std::format("{} cannot accept an expression of a different type, and cannot be implicitly converted", name),
			std::move(infos)
		);
	}



	auto SemanticAnalyzer::may_recover() const -> bool {
		return !this->context.hasHitFailCondition() && this->context.getConfig().mayRecover;
	}


	auto SemanticAnalyzer::print_type(const ExprInfo& expr_info) const -> std::string {
		if(expr_info.type_id.is<TypeInfo::VoidableID>()){
			return this->context.getTypeManager().printType(expr_info.type_id.as<TypeInfo::VoidableID>());

		}else if(expr_info.value_type == ExprInfo::ValueType::Import){
			return "{IMPORT}";

		}else{
			evo::debugAssert(expr_info.value_type == ExprInfo::ValueType::FluidLiteral, "expected fluid literal");

			if(expr_info.expr.has_value()){
				if(expr_info.expr->kind() == ASG::Expr::Kind::LiteralInt){
					return "{LITERAL INTEGER}";
					
				}else{
					evo::debugAssert(expr_info.expr->kind() == ASG::Expr::Kind::LiteralFloat, "expected literal float");
					return "{LITERAL FLOAT}";
				}

			}else{
				return "{LITERAL NUMBER}";
			}
		}
	}



	template<typename NODE_T>
	auto SemanticAnalyzer::emit_fatal(
		Diagnostic::Code code,
		const NODE_T& location,
		std::string&& msg,
		evo::SmallVector<Diagnostic::Info>&& infos
	) const -> void {
		this->add_template_location_infos(infos);
		this->context.emitFatal(code, this->get_source_location(location), std::move(msg), std::move(infos));
	}

	template<typename NODE_T>
	auto SemanticAnalyzer::emit_error(
		Diagnostic::Code code,
		const NODE_T& location,
		std::string&& msg,
		evo::SmallVector<Diagnostic::Info>&& infos
	) const -> void {
		this->add_template_location_infos(infos);
		this->context.emitError(code, this->get_source_location(location), std::move(msg), std::move(infos));
	}

	template<typename NODE_T>
	auto SemanticAnalyzer::emit_warning(
		Diagnostic::Code code,
		const NODE_T& location,
		std::string&& msg,
		evo::SmallVector<Diagnostic::Info>&& infos
	) const -> void {
		this->add_template_location_infos(infos);
		this->context.emitWarning(code, this->get_source_location(location), std::move(msg), std::move(infos));
	}


	auto SemanticAnalyzer::add_template_location_infos(evo::SmallVector<Diagnostic::Info>& infos) const -> void {
		if(this->template_parents.empty()){
			return;

		}else if(this->template_parents.size() == 1){
			infos.emplace_back("template instantiated here:", this->template_parents[0]);

		}else{
			for(size_t i = 0; const SourceLocation& template_parent : this->template_parents){
				infos.emplace_back(std::format("template instantiated here (depth: {}):", i), template_parent);
			
				i += 1;
			}
		}
	}


	//////////////////////////////////////////////////////////////////////
	// get source location

	auto SemanticAnalyzer::get_source_location(std::nullopt_t) const -> std::optional<SourceLocation> {
		return std::nullopt;
	};


	auto SemanticAnalyzer::get_source_location(Token::ID token_id) const -> SourceLocation {
		return this->source.getTokenBuffer().getSourceLocation(token_id, this->source.getID());
	}
	
	auto SemanticAnalyzer::get_source_location(const AST::Node& node) const -> SourceLocation {
		#if defined(EVO_COMPILER_MSVC)
			#pragma warning(default : 4062)
		#endif

		const ASTBuffer& ast_buffer = this->source.getASTBuffer();

		switch(node.kind()){
			case AST::Kind::None:           evo::debugFatalBreak("Cannot get location of AST::Kind::None");
			case AST::Kind::VarDecl:        return this->get_source_location(ast_buffer.getVarDecl(node));
			case AST::Kind::FuncDecl:       return this->get_source_location(ast_buffer.getFuncDecl(node));
			case AST::Kind::AliasDecl:      return this->get_source_location(ast_buffer.getAliasDecl(node));
			case AST::Kind::Return:         return this->get_source_location(ast_buffer.getReturn(node));
			case AST::Kind::Block:          return this->get_source_location(ast_buffer.getBlock(node));
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

	auto SemanticAnalyzer::get_source_location(const AST::Block& block) const -> SourceLocation {
		return this->get_source_location(block.openBrace);
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
		const Token& infix_op_token = this->source.getTokenBuffer()[infix.opTokenID];
		if(infix_op_token.kind() == Token::lookupKind(".")){
			return this->get_source_location(infix.rhs);
		}else{
			return this->get_source_location(infix.opTokenID);
		}
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

	auto SemanticAnalyzer::get_source_location(ASG::TemplatedFunc::ID templated_func_id) const -> SourceLocation {
		const ASG::TemplatedFunc& asg_templated_func = this->source.getASGBuffer().getTemplatedFunc(templated_func_id);
		return this->get_source_location(asg_templated_func.funcDecl.name);
	}


	auto SemanticAnalyzer::get_source_location(ASG::Var::ID var_id) const -> SourceLocation {
		const ASG::Var& asg_var = this->source.getASGBuffer().getVar(var_id);
		return this->get_source_location(asg_var.ident);
	}

}