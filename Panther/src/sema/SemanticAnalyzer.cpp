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

			switch(global_stmt.kind()){
				case AST::Kind::None: {
					this->context.emitFatal(
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
					this->context.emitError(
						Diagnostic::Code::MiscUnimplementedFeature,
						this->get_source_location(global_stmt),
						"global variable declarations are currently unsupported"
					);
					return false;
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
						Diagnostic::Code::SemaInvalidGlobalStmtKind,
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
				this->context.emitError(
					Diagnostic::Code::SemaVarOfTypeVoid,
					this->get_source_location(var_decl.type.value()),
					"Variables cannot be of type \"Void\""
				);
				return false;
			}

			var_type_id = var_type_result.value().typeID();
		}


		///////////////////////////////////
		// value

		if(var_decl.value.has_value() == false){
			this->context.emitError(
				Diagnostic::Code::SemaVarWithNoValue,
				this->get_source_location(var_decl.ident),
				"Variables must have values"
			);
			return false;	
		}

		const evo::Result<ExprInfo> expr_info_result = this->analyze_expr<ExprValueKind::Runtime>(*var_decl.value);
		if(expr_info_result.isError()){ return false; }


		///////////////////////////////////
		// type checking

		if(expr_info_result.value().is_concrete()){
			// TODO: better messaging?
			this->context.emitError(
				Diagnostic::Code::SemaIncorrectExprValueType,
				this->get_source_location(*var_decl.value),
				"Variable must be declared with an ephemeral expression value"
			);
			return false;

		}else if(expr_info_result.value().value_type == ExprInfo::ValueType::Import){
			this->context.emitError(
				Diagnostic::Code::MiscUnimplementedFeature,
				this->get_source_location(*var_decl.value),
				"Imports are currently unsupported"
			);
			return false;
		}


		if(expr_info_result.value().value_type == ExprInfo::ValueType::FluidLiteral){
			if(var_type_id.has_value() == false){
				this->context.emitError(
					Diagnostic::Code::SemaCannotInferType,
					this->get_source_location(*var_decl.value),
					"Cannot infer the type of a fluid literal"
				);
				return false;
			}

			const TypeInfo& var_type_info = this->context.getTypeManager().getTypeInfo(*var_type_id);

			if(
				var_type_info.qualifiers().empty() == false || 
				var_type_info.baseTypeID().kind() != BaseType::Kind::Builtin
			){
				this->type_mismatch("Variable", *var_decl.value, *var_type_id, expr_info_result.value());
				return false;
			}


			const BaseType::Builtin::ID var_type_builtin_id = var_type_info.baseTypeID().id<BaseType::Builtin::ID>();
			const BaseType::Builtin& var_type_builtin = this->context.getTypeManager().getBuiltin(var_type_builtin_id);

			if(expr_info_result.value().expr->kind() == ASG::Expr::Kind::LiteralInt){
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
						this->type_mismatch("Variable", *var_decl.value, *var_type_id, expr_info_result.value());
						return false;
					}
				}

				const ASG::LiteralInt::ID literal_int_id = expr_info_result.value().expr->literalIntID();
				this->source.asg_buffer.literal_ints[literal_int_id.get()].typeID = *var_type_id;


			}else{
				evo::debugAssert(
					expr_info_result.value().expr->kind() == ASG::Expr::Kind::LiteralFloat, "Expected literal float"
				);

				switch(var_type_builtin.kind()){
					case Token::Kind::TypeF16:
					case Token::Kind::TypeBF16:
					case Token::Kind::TypeF32:
					case Token::Kind::TypeF64:
					case Token::Kind::TypeF128:
						break;

					default: {
						this->type_mismatch("Variable", *var_decl.value, *var_type_id, expr_info_result.value());
						return false;
					}
				}

				const ASG::LiteralFloat::ID literal_float_id = expr_info_result.value().expr->literalFloatID();
				this->source.asg_buffer.literal_floats[literal_float_id.get()].typeID = *var_type_id;
			}

			
		}else if(var_type_id.has_value()){
			if(*var_type_id != expr_info_result.value().type_id){
				this->type_mismatch("Variable", *var_decl.value, *var_type_id, expr_info_result.value());
				return false;
			}

		}else{
			var_type_id = expr_info_result.value().type_id;
		}


		///////////////////////////////////
		// create

		const ASG::Var::ID asg_var_id = this->source.asg_buffer.createVar(
			var_decl.ident, *var_type_id, *expr_info_result.value().expr
		);

		this->get_current_scope_level().addVar(var_ident, asg_var_id);

		const ScopeManager::Scope::ObjectScope& current_object_scope = this->scope.getCurrentObjectScope();
		ASG::Func& current_func = this->source.asg_buffer.funcs[current_object_scope.as<ASG::Func::ID>().get()];
		current_func.stmts.emplace_back(asg_var_id);
		
		return true;
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
		
		this->get_current_scope_level().addFunc(func_ident, asg_func_id);

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
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();

		// TODO: order cases correctly
		switch(node.kind()){
			case AST::Kind::None: {
				this->context.emitFatal(
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
				return this->analyze_func_decl<false>(ast_buffer.getFuncDecl(node));
			} break;

            case AST::Kind::AliasDecl: {
            	return this->analyze_alias_decl<false>(ast_buffer.getAliasDecl(node));
        	} break;

        	case AST::Kind::FuncCall: {
        		return this->analyze_func_call(ast_buffer.getFuncCall(node));
    		} break;

			case AST::Kind::Return:        case AST::Kind::Block:    
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
					Diagnostic::Code::SemaInvalidStmtKind,
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
		const evo::Result<ExprInfo> call_target_info_res = this->analyze_expr<ExprValueKind::Runtime>(func_call.target);
		if(call_target_info_res.isError()){ return this->may_recover(); }

		if(
			call_target_info_res.value().value_type == ExprInfo::ValueType::Import || 
			call_target_info_res.value().value_type == ExprInfo::ValueType::FluidLiteral
		){
			this->context.emitError(
				Diagnostic::Code::SemaCannotCallLikeFunction,
				this->get_source_location(func_call.target),
				"Cannot call this expression like a function"
			);
			return this->may_recover();
		}


		const TypeInfo& call_target_type_info = 
			this->context.getTypeManager().getTypeInfo(*call_target_info_res.value().type_id);

		if(
			call_target_type_info.qualifiers().empty() == false ||
			call_target_type_info.baseTypeID().kind() != BaseType::Kind::Function
		){
			this->context.emitError(
				Diagnostic::Code::SemaCannotCallLikeFunction,
				this->get_source_location(func_call.target),
				"Cannot call this expression like a function"
			);
			return this->may_recover();
		}


		///////////////////////////////////
		// create

		const ASG::FuncCall::ID asg_func_call_id = this->source.asg_buffer.createFuncCall(
			ASG::Func::LinkID(this->source.getID(), call_target_info_res.value().expr->funcID())
		);

		const ScopeManager::Scope::ObjectScope& current_object_scope = this->scope.getCurrentObjectScope();
		ASG::Func& current_func = this->source.asg_buffer.funcs[current_object_scope.as<ASG::Func::ID>().get()];
		current_func.stmts.emplace_back(asg_func_call_id);

		return false;
	}







	auto SemanticAnalyzer::get_type_id(const AST::Type& ast_type) -> evo::Result<TypeInfo::VoidableID> {
		auto base_type = std::optional<BaseType::ID>();

		if(ast_type.base.kind() == AST::Kind::BuiltinType){
			const Token::ID builtin_type_token_id = ASTBuffer::getBuiltinType(ast_type.base);
			const Token& builtin_type_token = this->source.getTokenBuffer()[builtin_type_token_id];

			switch(builtin_type_token.kind()){
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

				case Token::Kind::TypeType:      case Token::Kind::TypeThis:       case Token::Kind::TypeInt:
				case Token::Kind::TypeISize:     case Token::Kind::TypeUInt:       case Token::Kind::TypeUSize:
				case Token::Kind::TypeF16:       case Token::Kind::TypeBF16:       case Token::Kind::TypeF32:
				case Token::Kind::TypeF64:       case Token::Kind::TypeF128:       case Token::Kind::TypeByte:
				case Token::Kind::TypeBool:      case Token::Kind::TypeChar:       case Token::Kind::TypeRawPtr:
				case Token::Kind::TypeCShort:    case Token::Kind::TypeCUShort:    case Token::Kind::TypeCInt:
				case Token::Kind::TypeCUInt:     case Token::Kind::TypeCLong:      case Token::Kind::TypeCULong:
				case Token::Kind::TypeCLongLong: case Token::Kind::TypeCULongLong: case Token::Kind::TypeCLongDouble: {
					base_type = this->context.getTypeManager().getOrCreateBuiltinBaseType(builtin_type_token.kind());
				} break;

				case Token::Kind::TypeI_N: case Token::Kind::TypeUI_N: {
					base_type = this->context.getTypeManager().getOrCreateBuiltinBaseType(
						builtin_type_token.kind(), builtin_type_token.getBitWidth()
					);
				} break;

				default: {
					evo::debugFatalBreak("Unknown or unsupported BuiltinType: {}", builtin_type_token.kind());
				} break;
			}

		}else{
			this->context.emitError(
				Diagnostic::Code::MiscUnimplementedFeature,
				this->get_source_location(ast_type.base),
				"Only built-in-types are currently supported"
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



	auto SemanticAnalyzer::get_current_scope_level() const -> ScopeManager::ScopeLevel& {
		return this->context.getScopeManager()[this->scope.getCurrentScopeLevel()];
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
				this->context.emitFatal(
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
				this->context.emitFatal(
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
		this->context.emitError(
			Diagnostic::Code::MiscUnimplementedFeature,
			this->get_source_location(block),
			"block expressions are currently unsupported"
		);
		return evo::resultError;
	}

	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_func_call(const AST::FuncCall& func_call) -> evo::Result<ExprInfo> {
		this->context.emitError(
			Diagnostic::Code::MiscUnimplementedFeature,
			this->get_source_location(func_call),
			"function call expressions are currently unsupported"
		);
		return evo::resultError;
	}

	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_templated_expr(const AST::TemplatedExpr& templated_expr)
	-> evo::Result<ExprInfo> {
		this->context.emitError(
			Diagnostic::Code::MiscUnimplementedFeature,
			this->get_source_location(templated_expr),
			"templated expression expressions are currently unsupported"
		);
		return evo::resultError;
	}

	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_prefix(const AST::Prefix& prefix) -> evo::Result<ExprInfo> {
		this->context.emitError(
			Diagnostic::Code::MiscUnimplementedFeature,
			this->get_source_location(prefix),
			"prefix expressions are currently unsupported"
		);
		return evo::resultError;
	}

	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_infix(const AST::Infix& infix) -> evo::Result<ExprInfo> {
		this->context.emitError(
			Diagnostic::Code::MiscUnimplementedFeature,
			this->get_source_location(infix),
			"infix expressions are currently unsupported"
		);
		return evo::resultError;
	}

	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_postfix(const AST::Postfix& postfix) -> evo::Result<ExprInfo> {
		this->context.emitError(
			Diagnostic::Code::MiscUnimplementedFeature,
			this->get_source_location(postfix),
			"postfix expressions are currently unsupported"
		);
		return evo::resultError;
	}

	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_ident(const Token::ID& ident) -> evo::Result<ExprInfo> {
		if constexpr(EXPR_VALUE_KIND == ExprValueKind::ConstEval){
			this->context.emitError(
				Diagnostic::Code::MiscUnimplementedFeature,
				this->get_source_location(ident),
				"consteval identifier expressions are currently unsupported"
			);

			return evo::resultError;
		}

		const std::string_view ident_str = this->source.getTokenBuffer()[ident].getString();

		for(ScopeManager::ScopeLevel::ID scope_level_id : this->scope){
			const ScopeManager::ScopeLevel& scope_level = this->context.getScopeManager()[scope_level_id];

			const std::optional<ASG::Func::ID> lookup_func_id = scope_level.lookupFunc(ident_str);
			if(lookup_func_id.has_value()){
				const ASG::Func& asg_func = this->source.getASGBuffer().getFunc(*lookup_func_id);

				const TypeInfo::ID type_id = this->context.getTypeManager().getOrCreateTypeInfo(
					TypeInfo(asg_func.baseTypeID)
				);

				if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
					return ExprInfo(ExprInfo::ValueType::ConcreteConst, type_id, std::nullopt);
				}else{
					return ExprInfo(ExprInfo::ValueType::ConcreteConst, type_id, ASG::Expr(*lookup_func_id));
				}
			}

			const std::optional<ASG::Var::ID> lookup_var_id = scope_level.lookupVar(ident_str);
			if(lookup_var_id.has_value()){
				const ASG::Var& asg_var = this->source.getASGBuffer().getVar(*lookup_var_id);

				const auto value_type = ExprInfo::ValueType::ConcreteMutable;
				if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
					return ExprInfo(value_type, asg_var.typeID, std::nullopt);
				}else{
					return ExprInfo(value_type, asg_var.typeID, ASG::Expr(*lookup_var_id));
				}
			}
		}

		this->context.emitError(
			Diagnostic::Code::SemaIdentNotInScope,
			this->get_source_location(ident),
			std::format("Identifier \"{}\" was not defined in this scope", ident_str)
		);
		return evo::resultError;
	}

	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_intrinsic(const Token::ID& intrinsic) -> evo::Result<ExprInfo> {
		this->context.emitError(
			Diagnostic::Code::MiscUnimplementedFeature,
			this->get_source_location(intrinsic),
			"intrinsic expressions are currently unsupported"
		);
		return evo::resultError;
	}

	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_literal(const Token::ID& literal) -> evo::Result<ExprInfo> {
		const Token& token = this->source.getTokenBuffer()[literal];
		
		auto expr_info = ExprInfo(ExprInfo::ValueType::Ephemeral, std::nullopt, std::nullopt);

		switch(token.kind()){
			case Token::Kind::LiteralInt: {
				expr_info.value_type = ExprInfo::ValueType::FluidLiteral;
				// expr_info.type_id = TypeInfo::VoidableID(this->context.getTypeManager().getTypeInt());

				if constexpr(EXPR_VALUE_KIND != ExprValueKind::None){
					expr_info.expr = ASG::Expr(
						this->source.asg_buffer.createLiteralInt(token.getInt(), std::nullopt)
					);
				}
			} break;

			case Token::Kind::LiteralFloat: {
				expr_info.value_type = ExprInfo::ValueType::FluidLiteral;
				// expr_info.type_id = TypeInfo::VoidableID(this->context.getTypeManager().getTypeFloat());

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
				this->context.emitError(
					Diagnostic::Code::MiscUnimplementedFeature,
					this->get_source_location(literal),
					"literal strings are currently unsupported"
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
		this->context.emitError(
			Diagnostic::Code::MiscUnimplementedFeature,
			this->get_source_location(uninit),
			"uninit expressions are currently unsupported"
		);
		return evo::resultError;
	}

	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_this(const Token::ID& this_expr) -> evo::Result<ExprInfo> {
		this->context.emitError(
			Diagnostic::Code::MiscUnimplementedFeature,
			this->get_source_location(this_expr),
			"this expressions are currently unsupported"
		);
		return evo::resultError;
	}





	//////////////////////////////////////////////////////////////////////
	// error handling

	template<typename NODE_T>
	auto SemanticAnalyzer::already_defined(std::string_view ident, const NODE_T& node) const -> bool {

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

			const std::optional<ASG::Var::ID> lookup_var = scope_level.lookupVar(ident);
			if(lookup_var.has_value()){
				auto infos = evo::SmallVector<Diagnostic::Info>{
					Diagnostic::Info("First defined here:", this->get_source_location(lookup_var.value())),
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



	template<typename NODE_T>
	auto SemanticAnalyzer::type_mismatch(
		std::string_view name, const NODE_T& location, TypeInfo::ID expected, const ExprInfo& got
	) -> void {
		const TypeManager& type_manager = this->context.getTypeManager();

		// TODO: make sure the types given in the infos line up

		auto infos = evo::SmallVector<Diagnostic::Info>();
		infos.emplace_back(std::format("{} is of type: {}", name, type_manager.printType(expected)));

		if(got.type_id.has_value()){
			infos.emplace_back(std::format("Expression is of type: {}", type_manager.printType(*got.type_id)));

		}else if(got.value_type == ExprInfo::ValueType::Import){
			infos.emplace_back("Expression is of type: {IMPORT}");

		}else{
			evo::debugAssert(got.value_type == ExprInfo::ValueType::FluidLiteral, "expected fluid literal");

			if(got.expr.has_value()){
				if(got.expr->kind() == ASG::Expr::Kind::LiteralInt){
					infos.emplace_back("Expression is of type: {LITERAL INTEGER}");
					
				}else{
					evo::debugAssert(got.expr->kind() == ASG::Expr::Kind::LiteralFloat, "expected literal float");
					infos.emplace_back("Expression is of type: {LITERAL FLOAT}");
				}

			}else{
				infos.emplace_back("Expression is of type: {LITERAL NUMBER}");
			}
		}

		this->context.emitError(
			Diagnostic::Code::SemaTypeMismatch,
			this->get_source_location(location),
			std::format("{} cannot accept an expression of a different type, and cannot be implicitly converted", name),
			std::move(infos)
		);
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

	auto SemanticAnalyzer::get_source_location(ASG::Var::ID var_id) const -> SourceLocation {
		const ASG::Var& asg_var = this->source.getASGBuffer().getVar(var_id);
		return this->get_source_location(asg_var.ident);
	}

}