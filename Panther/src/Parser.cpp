//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "./Parser.h"



namespace pcit::panther{
	

	auto Parser::parse() -> bool {
		while(this->reader.at_end() == false){
			const Result stmt_result = this->parse_stmt();

			switch(stmt_result.code()){
				case Result::Code::Success: {
					this->source.ast_buffer.global_stmts.emplace_back(stmt_result.value());
				} break;

				case Result::Code::WrongType: {
					this->context.emitError(
						Diagnostic::Code::ParserUnknownStmtStart,
						this->source.getTokenBuffer().getSourceLocation(this->reader.peek(), this->source.getID()),
						"Unknown start to statement"
					);
					return false;
				} break;

				case Result::Code::Error: {
					return false;
				} break;
			}
		}

		return true;
	}


	auto Parser::parse_stmt() -> Result {
		const Token& peeked_token = this->reader[this->reader.peek()];
		
		switch(peeked_token.kind()){
			case Token::Kind::KeywordVar:    return this->parse_var_decl<AST::VarDecl::Kind::Var>();
			case Token::Kind::KeywordConst:  return this->parse_var_decl<AST::VarDecl::Kind::Const>();
			case Token::Kind::KeywordDef:    return this->parse_var_decl<AST::VarDecl::Kind::Def>();
			case Token::Kind::KeywordFunc:   return this->parse_func_decl();
			case Token::Kind::KeywordAlias:  return this->parse_alias_decl();
			case Token::Kind::KeywordReturn: return this->parse_return();
		}

		Result result = this->parse_assignment();
		if(result.code() != Result::Code::WrongType){ return result; }

		result = this->parse_term_stmt();
		if(result.code() != Result::Code::WrongType){ return result; }

		return this->parse_block(BlockLabelRequirement::NotAllowed);
	}


	// TODO: check EOF
	template<AST::VarDecl::Kind VAR_DECL_KIND>
	auto Parser::parse_var_decl() -> Result {
		if constexpr(VAR_DECL_KIND == AST::VarDecl::Kind::Var){
			if(this->assert_token_fail(Token::Kind::KeywordVar)){ return Result::Code::Error; }

		}else if constexpr(VAR_DECL_KIND == AST::VarDecl::Kind::Const){
			if(this->assert_token_fail(Token::Kind::KeywordConst)){ return Result::Code::Error; }
			
		}else{
			if(this->assert_token_fail(Token::Kind::KeywordDef)){ return Result::Code::Error; }
		}

		const Result ident = this->parse_ident();
		if(this->check_result_fail(ident, "identifier in variable declaration")){ return Result::Code::Error; }


		auto type = std::optional<AST::Node>();
		if(this->reader[this->reader.peek()].kind() == Token::lookupKind(":")){
			if(this->assert_token_fail(Token::lookupKind(":"))){ return Result::Code::Error; }

			const Result type_result = this->parse_type<TypeKind::Explicit>();
			if(this->check_result_fail(type_result, "type after [:] in variable declaration")){
				return Result::Code::Error;
			}

			type = type_result.value();
		}


		const Result attributes = this->parse_attribute_block();
		if(attributes.code() == Result::Code::Error){ return Result::Code::Error; }


		auto value = std::optional<AST::Node>();
		if(this->reader[this->reader.peek()].kind() == Token::lookupKind("=")){
			if(this->assert_token_fail(Token::lookupKind("="))){ return Result::Code::Error; }
				
			const Result value_result = this->parse_expr();
			// TODO: better messaging around block exprs missing a label
			if(this->check_result_fail(value_result, "expression after [=] in variable declaration")){
				return Result::Code::Error;
			}

			value = value_result.value();
		}


		if(this->expect_token_fail(Token::lookupKind(";"), "after variable declaration")){ return Result::Code::Error; }


		return this->source.ast_buffer.createVarDecl(
			VAR_DECL_KIND, ASTBuffer::getIdent(ident.value()), type, attributes.value(), value
		);
	}


	// TODO: check EOF
	auto Parser::parse_func_decl() -> Result {
		if(this->assert_token_fail(Token::Kind::KeywordFunc)){ return Result::Code::Error; }

		const Result ident = this->parse_ident();
		if(this->check_result_fail(ident, "identifier after [func] in function declaration")){
			return Result::Code::Error;
		}

		if(this->expect_token_fail(Token::lookupKind("="), "after identifier in function declaration")){
			return Result::Code::Error;
		}

		auto template_pack_node = std::optional<AST::Node>();
		const Result template_pack_result = this->parse_template_pack();
		switch(template_pack_result.code()){
			case Result::Code::Success:   template_pack_node = template_pack_result.value(); break;
			case Result::Code::WrongType: break;
			case Result::Code::Error:     return Result::Code::Error;	
		}

		evo::Result<evo::SmallVector<AST::FuncDecl::Param>> params = this->parse_func_params();
		if(params.isError()){ return Result::Code::Error; }


		const Result attribute_block = this->parse_attribute_block();
		if(attribute_block.code() == Result::Code::Error){ return Result::Code::Error; }


		if(this->expect_token_fail(Token::lookupKind("->"), "in function declaration")){ return Result::Code::Error; }

		evo::Result<evo::SmallVector<AST::FuncDecl::Return>> returns = this->parse_func_returns();
		if(returns.isError()){ return Result::Code::Error; }

		const Result block = this->parse_block(BlockLabelRequirement::NotAllowed);
		if(this->check_result_fail(block, "statement block in function declaration")){ // TODO: better messaging 
			return Result::Code::Error;
		}

		return this->source.ast_buffer.createFuncDecl(
			ident.value(),
			template_pack_node,
			std::move(params.value()),
			attribute_block.value(),
			std::move(returns.value()),
			block.value()
		);
	}


	// TODO: check EOF
	auto Parser::parse_alias_decl() -> Result {
		if(this->assert_token_fail(Token::Kind::KeywordAlias)){ return Result::Code::Error; }

		const Result ident = this->parse_ident();
		if(this->check_result_fail(ident, "identifier in alias declaration")){ return Result::Code::Error; }

		if(this->expect_token_fail(Token::lookupKind("="), "in alias declaration")){ return Result::Code::Error; }

		const Result type = this->parse_type<TypeKind::Explicit>();
		if(this->check_result_fail(type, "type in alias declaration")){ return Result::Code::Error; }

		if(this->expect_token_fail(Token::lookupKind(";"), "at end of alias declaration")){ return Result::Code::Error; }

		return this->source.ast_buffer.createAliasDecl(ASTBuffer::getIdent(ident.value()), type.value());
	}


	// TODO: check EOF
	auto Parser::parse_return() -> Result {
		const Token::ID start_location = this->reader.peek();
		if(this->assert_token_fail(Token::Kind::KeywordReturn)){ return Result::Code::Error; }

		auto label = std::optional<AST::Node>();
		auto expr = evo::Variant<std::monostate, AST::Node, Token::ID>();

		if(this->reader[this->reader.peek()].kind() == Token::lookupKind("->")){
			if(this->assert_token_fail(Token::lookupKind("->"))){ return Result::Code::Error; }

			const Result label_result = this->parse_ident();
			if(this->check_result_fail(label_result, "identifier in return block label")){ return Result::Code::Error; }
			label = label_result.value();
		}

		if(this->reader[this->reader.peek()].kind() == Token::lookupKind("...")){
			expr = this->reader.next();
		}else{
			const Result expr_result = this->parse_expr();
			if(expr_result.code() == Result::Code::Error){
				return Result::Code::Error;
			}else if(expr_result.code() == Result::Code::Success){
				expr = expr_result.value();
			}
		}

		if(this->expect_token_fail(Token::lookupKind(";"), "at the end of a [return] statement")){
			return Result::Code::Error;
		}

		return this->source.ast_buffer.createReturn(start_location, label, expr);
	}


	// TODO: check EOF
	auto Parser::parse_assignment() -> Result {
		const Token::ID start_location = this->reader.peek();

		{ // special assignments
			const Token::Kind peeked_kind = this->reader[this->reader.peek()].kind();

			if(peeked_kind == Token::lookupKind("_")){
				const Token::ID discard_token_id = this->reader.next();
				
				const Token::ID op_token_id = this->reader.peek();
				if(this->expect_token_fail(Token::lookupKind("="), "in discard assignment")){
					return Result::Code::Error;
				}

				const Result value = this->parse_expr();
				if(this->check_result_fail(value, "expression value in discard assignment")){
					return Result::Code::Error;
				}

				if(this->expect_token_fail(Token::lookupKind(";"), "at end of discard assignment")){
					return Result::Code::Error;
				}

				return this->source.ast_buffer.createInfix(
					AST::Node(AST::Kind::Discard, discard_token_id), op_token_id, value.value()
				);

			}else if(peeked_kind == Token::lookupKind("[")){ // multi assign
				if(this->assert_token_fail(Token::lookupKind("["))){ return Result::Code::Error; }

				auto assignments = evo::SmallVector<AST::Node>();
				while(true){
					if(this->reader[this->reader.peek()].kind() == Token::lookupKind("]")){
						if(this->assert_token_fail(Token::lookupKind("]"))){ return Result::Code::Error; }
						break;
					}

					const Result assignment = [&]() {
						const Result ident_result = this->parse_ident();
						if(ident_result.code() != Result::Code::WrongType){ return ident_result; }

						const Token::ID peeked_token_id = this->reader.next();
						const Token::Kind peeked_kind = this->reader[peeked_token_id].kind();
						if(peeked_kind == Token::lookupKind("_")){
							return Result(AST::Node(AST::Kind::Discard, peeked_token_id));
						}else{
							return Result(Result::Code::WrongType);
						}
					}();
					if(assignment.code() != Result::Code::Success){ return Result::Code::Error; }

					assignments.emplace_back(assignment.value());


					// check if ending or should continue
					const Token::Kind after_ident_next_token_kind = this->reader[this->reader.next()].kind();
					if(after_ident_next_token_kind != Token::lookupKind(",")){
						if(after_ident_next_token_kind != Token::lookupKind("]")){
							this->expected_but_got(
								"[,] after identifier or []] at end of multiple assignemt identifier block",
								this->reader.peek(-1)
							);
							return Result::Code::Error;
						}

						break;
					}
				}

				if(assignments.empty()){
					this->context.emitError(
						Diagnostic::Code::ParserEmptyMultiAssign,
						this->source.getTokenBuffer().getSourceLocation(this->reader.peek(-1), this->source.getID()),
						"Multiple-assignment statements cannot assign to 0 values"
					);
				}

				if(this->expect_token_fail(Token::lookupKind("="), "in multiple-assignment")){
					return Result::Code::Error;
				}

				const Result value = this->parse_expr();
				if(this->check_result_fail(value, "expression value in multiple-assignment")){
					return Result::Code::Error;
				}

				if(this->expect_token_fail(Token::lookupKind(";"), "at end of multiple-assignment")){
					return Result::Code::Error;
				}

				return this->source.ast_buffer.createMultiAssign(std::move(assignments), value.value());
			}
		}



		const Result lhs = this->parse_term<IsTypeTerm::No>();
		if(lhs.code() != Result::Code::Success){ return lhs; }

		const Token::ID op_token_id = this->reader.next();
		switch(this->reader[op_token_id].kind()){
			case Token::lookupKind("="):
			case Token::lookupKind("+="):  case Token::lookupKind("+@="):  case Token::lookupKind("+|="):
			case Token::lookupKind("-="):  case Token::lookupKind("-@="):  case Token::lookupKind("-|="):
			case Token::lookupKind("*="):  case Token::lookupKind("*@="):  case Token::lookupKind("*|="):
			case Token::lookupKind("/="):  case Token::lookupKind("%="):
			case Token::lookupKind("<<="): case Token::lookupKind("<<|="): case Token::lookupKind(">>="):
			case Token::lookupKind("&="):  case Token::lookupKind("|="):   case Token::lookupKind("^="):
				break;

			default: {
				this->reader.go_back(start_location);
				return Result::Code::WrongType;
			} break;
		}

		const Result rhs = this->parse_expr();
		if(this->check_result_fail(rhs, "expression value in assignment")){ return Result::Code::Error; }

		if(this->expect_token_fail(Token::lookupKind(";"), "at end of assignment")){ return Result::Code::Error; }

		return this->source.ast_buffer.createInfix(lhs.value(), op_token_id, rhs.value());
	}




	// TODO: check EOF
	auto Parser::parse_block(BlockLabelRequirement label_requirement) -> Result {
		const Token::ID start_location = this->reader.peek();

		if(this->reader[this->reader.peek()].kind() != Token::lookupKind("{")){ return Result::Code::WrongType; }
		if(this->assert_token_fail(Token::lookupKind("{"))){ return Result::Code::Error; }

		auto label = std::optional<AST::Node>();

		if(this->reader[this->reader.peek()].kind() == Token::lookupKind("->")){
			if(label_requirement == BlockLabelRequirement::NotAllowed){
				this->reader.go_back(start_location);
				return Result::Code::WrongType;
			}

			if(this->assert_token_fail(Token::lookupKind("->"))){ return Result::Code::Error; }

			const Result label_result = this->parse_ident();
			if(this->check_result_fail(label_result, "identifier in block label")){ return Result::Code::Error; }
			label = label_result.value();

		}else if(label_requirement == BlockLabelRequirement::Required){
			this->reader.go_back(start_location);
			return Result::Code::WrongType;
		}

		auto statements = evo::SmallVector<AST::Node>();

		while(true){
			if(this->reader[this->reader.peek()].kind() == Token::lookupKind("}")){
				if(this->assert_token_fail(Token::lookupKind("}"))){ return Result::Code::Error; }
				break;
			}

			const Result stmt = this->parse_stmt();
			if(this->check_result_fail(stmt, "statement in statement block")){ return Result::Code::Error; }

			statements.emplace_back(stmt.value());
		}

		return this->source.ast_buffer.createBlock(start_location, label, std::move(statements));
	}


	// TODO: check EOF
	template<Parser::TypeKind KIND>
	auto Parser::parse_type() -> Result {
		const Token::ID start_location = this->reader.peek();
		bool is_builtin = true;
		switch(this->reader[start_location].kind()){
			case Token::Kind::TypeVoid:
			case Token::Kind::TypeType:
			case Token::Kind::TypeThis:
			case Token::Kind::TypeInt:
			case Token::Kind::TypeISize:
			case Token::Kind::TypeI_N:
			case Token::Kind::TypeUInt:
			case Token::Kind::TypeUSize:
			case Token::Kind::TypeUI_N:
			case Token::Kind::TypeF16:
			case Token::Kind::TypeBF16:
			case Token::Kind::TypeF32:
			case Token::Kind::TypeF64:
			case Token::Kind::TypeF80:
			case Token::Kind::TypeF128:
			case Token::Kind::TypeByte:
			case Token::Kind::TypeBool:
			case Token::Kind::TypeChar:
			case Token::Kind::TypeRawPtr:
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

			case Token::Kind::Ident: case Token::Kind::Intrinsic: {
				is_builtin = false;
			} break;

			default: return Result::Code::WrongType;
		}

		const Result base_type = [&]() {
			if(is_builtin){
				const Token::ID base_type_token_id = this->reader.next();

				const Token& peeked_token = this->reader[this->reader.peek()];
				if(peeked_token.kind() == Token::lookupKind(".*")){
					const Diagnostic::Info diagnostics_info = [](){
						if constexpr(KIND == TypeKind::Expr || KIND == TypeKind::TemplateArg){
							return Diagnostic::Info(
								"Did you mean to put parentheses around the preceding [as] operation?"
							);
						}else{
							return Diagnostic::Info("Did you mean pointer ([*]) instead?");
						}
					}();

					this->context.emitError(
						Diagnostic::Code::ParserDereferenceOrUnwrapOnType,
						this->source.getTokenBuffer().getSourceLocation(this->reader.peek(), this->source.getID()),
						"A dereference operator ([.*]) should not follow a type",
						evo::SmallVector<Diagnostic::Info>{diagnostics_info}
					);
					return Result(Result::Code::Error);

				}else if(peeked_token.kind() == Token::lookupKind(".?")){
					const Diagnostic::Info diagnostics_info = [](){
						if constexpr(KIND == TypeKind::Expr || KIND == TypeKind::TemplateArg){
							return Diagnostic::Info(
								"Did you mean to put parentheses around the preceding [as] operation?"
							);
						}else{
							return Diagnostic::Info("Did you mean optional ([?]) instead?");
						}
					}();

					this->context.emitError(
						Diagnostic::Code::ParserDereferenceOrUnwrapOnType,
						this->source.getTokenBuffer().getSourceLocation(this->reader.peek(), this->source.getID()),
						"A unwrap operator ([.?]) should not follow a type",
						evo::SmallVector<Diagnostic::Info>{diagnostics_info}
					);
					return Result(Result::Code::Error);
				}

				return Result(AST::Node(AST::Kind::BuiltinType, base_type_token_id));

			}else{
				if constexpr(KIND == TypeKind::TemplateArg){
					return this->parse_term<IsTypeTerm::Maybe>();
				}else{
					return this->parse_term<IsTypeTerm::YesAs>();
				}
			}
		}();

		if(base_type.code() == Result::Code::Error){
			return Result::Code::Error;
		}else if(base_type.code() == Result::Code::WrongType){
			this->reader.go_back(start_location);
			return Result::Code::WrongType;
		}


		auto qualifiers = evo::SmallVector<AST::Type::Qualifier>();
		Token::ID potential_backup_location = this->reader.peek();
		bool continue_looking_for_qualifiers = true;
		while(continue_looking_for_qualifiers){
			continue_looking_for_qualifiers = false;
			bool is_ptr = false;
			bool is_read_only = false;
			bool is_optional = false;

			if(this->reader[this->reader.peek()].kind() == Token::lookupKind("*")){
				continue_looking_for_qualifiers = true;
				is_ptr = true;
				potential_backup_location = this->reader.peek();
				if(this->assert_token_fail(Token::lookupKind("*"))){ return Result::Code::Error; }
				
				if(this->reader[this->reader.peek()].kind() == Token::lookupKind("|")){
					is_read_only = true;
					potential_backup_location = this->reader.peek();
					if(this->assert_token_fail(Token::lookupKind("|"))){ return Result::Code::Error; }
				}

			}else if(this->reader[this->reader.peek()].kind() == Token::lookupKind("*|")){
				continue_looking_for_qualifiers = true;
				is_ptr = true;
				is_read_only = true;
				potential_backup_location = this->reader.peek();
				if(this->assert_token_fail(Token::lookupKind("*|"))){ return Result::Code::Error; }
			}

			if(this->reader[this->reader.peek()].kind() == Token::lookupKind("?")){
				continue_looking_for_qualifiers = true;
				is_optional = true;
				if(this->assert_token_fail(Token::lookupKind("?"))){ return Result::Code::Error; }
			}

			if(continue_looking_for_qualifiers){
				qualifiers.emplace_back(is_ptr, is_read_only, is_optional);
			}
		}


		if constexpr(KIND == TypeKind::Expr || KIND == TypeKind::TemplateArg){
			// make sure exprs like `a as Int * b` gets parsed like `(a as Int) * b`
			if(qualifiers.empty() == false && qualifiers.back().isOptional == false){
				switch(this->reader[this->reader.peek()].kind()){
					case Token::Kind::Ident:
					case Token::lookupKind("("):
					case Token::Kind::LiteralBool:
					case Token::Kind::LiteralInt:
					case Token::Kind::LiteralFloat:
					case Token::Kind::LiteralString:
					case Token::Kind::LiteralChar: {
						if(
							qualifiers.back().isReadOnly && 
							this->reader.peek().get() - potential_backup_location.get() == 1
						){ // prevent issue with `a as Int* | b`
							qualifiers.back().isReadOnly = false;
						}else{
							qualifiers.pop_back();
						}

						this->reader.go_back(potential_backup_location);
					} break;
				}
			}

			// just an ident
			if constexpr(KIND == TypeKind::TemplateArg){
				if(is_builtin == false && qualifiers.empty()){
					this->reader.go_back(start_location);
					return Result::Code::WrongType;
				}
			}
		}

		return this->source.ast_buffer.createType(base_type.value(), std::move(qualifiers));
	}



	// TODO: check EOF
	auto Parser::parse_expr() -> Result {
		Result result = this->parse_uninit();
		if(result.code() != Result::Code::WrongType){ return result; }

		result = this->parse_zeroinit();
		if(result.code() != Result::Code::WrongType){ return result; }

		return this->parse_sub_expr();
	}

	// TODO: check EOF
	auto Parser::parse_sub_expr() -> Result {
		return this->parse_infix_expr();
	}



	EVO_NODISCARD static constexpr auto get_infix_op_precedence(Token::Kind kind) -> int {
		switch(kind){
			case Token::lookupKind("||"):  return 1;

			case Token::lookupKind("&&"):  return 2;

			case Token::lookupKind("=="):  return 3;
			case Token::lookupKind("!="):  return 3;
			case Token::lookupKind("<"):   return 3;
			case Token::lookupKind("<="):  return 3;
			case Token::lookupKind(">"):   return 3;
			case Token::lookupKind(">="):  return 3;

			case Token::lookupKind("&"):   return 4;
			case Token::lookupKind("|"):   return 4;
			case Token::lookupKind("^"):   return 4;

			case Token::lookupKind("<<"):  return 5;
			case Token::lookupKind("<<|"): return 5;
			case Token::lookupKind(">>"):  return 5;

			case Token::lookupKind("+"):   return 6;
			case Token::lookupKind("+@"):  return 6;
			case Token::lookupKind("+|"):  return 6;
			case Token::lookupKind("-"):   return 6;
			case Token::lookupKind("-@"):  return 6;
			case Token::lookupKind("-|"):  return 6;

			case Token::lookupKind("*"):   return 7;
			case Token::lookupKind("*@"):  return 7;
			case Token::lookupKind("*|"):  return 7;
			case Token::lookupKind("/"):   return 7;
			case Token::lookupKind("%"):   return 7;

			case Token::Kind::KeywordAs:   return 8;
		}

		return -1;
	}

	// TODO: check EOF
	auto Parser::parse_infix_expr() -> Result {
		const Result lhs_result = this->parse_prefix_expr();
		if(lhs_result.code() != Result::Code::Success){ return lhs_result; }

		return this->parse_infix_expr_impl(lhs_result.value(), 0);
	}


	auto Parser::parse_infix_expr_impl(AST::Node lhs, int prec_level) -> Result {
		const Token::ID peeked_op_token_id = this->reader.peek();
		const Token::Kind peeked_op_kind = this->reader[peeked_op_token_id].kind();

		const int next_op_prec = get_infix_op_precedence(peeked_op_kind);

		// if not an infix operator or is same or lower precedence
		// 		next_op_prec == -1 if its not an infix op
		//   	< to maintain operator precedence
		// 		<= to prevent `a + b + c` from being parsed as `a + (b + c)`
		if(next_op_prec <= prec_level){ return lhs; }

		if(this->assert_token_fail(peeked_op_kind)){ return Result::Code::Error; }

		const Result rhs_result = [&](){
			if(peeked_op_kind == Token::Kind::KeywordAs){
				const Result type_result = this->parse_type<TypeKind::Expr>();
				if(this->check_result_fail(type_result, "type after [as] operator")){
					return Result(Result::Code::Error);
				}

				if(this->reader[this->reader.peek()].kind() == Token::lookupKind(".*")){
					this->context.emitError(
						Diagnostic::Code::ParserDereferenceOrUnwrapOnType,
						this->source.getTokenBuffer().getSourceLocation(this->reader.peek(), this->source.getID()),
						"A dereference operator ([.*]) should not follow a type",
						evo::SmallVector<Diagnostic::Info>{
							Diagnostic::Info("Did you mean to put parentheses around the preceding [as] operation?")
						}
					);
					return Result(Result::Code::Error);

				}else if(this->reader[this->reader.peek()].kind() == Token::lookupKind(".?")){
					this->context.emitError(
						Diagnostic::Code::ParserDereferenceOrUnwrapOnType,
						this->source.getTokenBuffer().getSourceLocation(this->reader.peek(), this->source.getID()),
						"A dereference operator ([.?]) should not follow a type",
						evo::SmallVector<Diagnostic::Info>{
							Diagnostic::Info("Did you mean to put parentheses around the preceding [as] operation?")
						}
					);
					return Result(Result::Code::Error);
				}

				return type_result;
			}
			
			const Result next_part_of_expr = this->parse_prefix_expr();
			if(next_part_of_expr.code() != Result::Code::Success){ return next_part_of_expr; }

			return this->parse_infix_expr_impl(next_part_of_expr.value(), next_op_prec);
		}();

		if(rhs_result.code() != Result::Code::Success){ return rhs_result; }

		const AST::Node created_infix_expr = 
			this->source.ast_buffer.createInfix(lhs, peeked_op_token_id, rhs_result.value());

		return this->parse_infix_expr_impl(created_infix_expr, prec_level);
	}



	// TODO: check EOF
	auto Parser::parse_prefix_expr() -> Result {
		const Token::ID op_token_id = this->reader.peek();
		const Token::Kind op_token_kind = this->reader[op_token_id].kind();

		switch(op_token_kind){
			case Token::lookupKind("&"):
			case Token::Kind::KeywordCopy:
			case Token::Kind::KeywordMove:
			case Token::lookupKind("-"):
			case Token::lookupKind("!"):
			case Token::lookupKind("~"):
				break;

			default:
				return this->parse_term<IsTypeTerm::No>();
		}

		if(this->assert_token_fail(op_token_kind)){ return Result::Code::Error; }

		const Result rhs = this->parse_term<IsTypeTerm::No>();
		if(this->check_result_fail(
			rhs, std::format("valid sub-expression on right-hand side of prefix [{}] operator", op_token_kind)
		)){
			return Result::Code::Error;
		}

		return this->source.ast_buffer.createPrefix(op_token_id, rhs.value());
	}

	// TODO: check EOF
	template<Parser::IsTypeTerm IS_TYPE_TERM>
	auto Parser::parse_term() -> Result {
		Result output = [&](){
			if constexpr(IS_TYPE_TERM == IsTypeTerm::Yes || IS_TYPE_TERM == IsTypeTerm::YesAs){
				return this->parse_ident();
			}else{
				return this->parse_encapsulated_expr();
			}
		}();
		if(output.code() != Result::Code::Success){ return output; }

		bool should_continue = true;
		while(should_continue){
			switch(this->reader[this->reader.peek()].kind()){
				case Token::lookupKind("."): {
					const Token::ID accessor_op_token_id = this->reader.next();	

					Result rhs_result = this->parse_ident();
					if(rhs_result.code() == Result::Code::Error){
						return Result::Code::Error;

					}else if(rhs_result.code() == Result::Code::WrongType){
						rhs_result = this->parse_intrinsic();
						if(this->check_result_fail(rhs_result, "identifier or intrinsic after [.] accessor")){
							return Result::Code::Error;
						}
					}

					output = this->source.ast_buffer.createInfix(
						output.value(), accessor_op_token_id, rhs_result.value()
					);
				} break;

				case Token::lookupKind(".*"): {
					if constexpr(IS_TYPE_TERM == IsTypeTerm::Yes || IS_TYPE_TERM == IsTypeTerm::YesAs){
						Diagnostic::Info diagnostics_info = [](){
							if constexpr(IS_TYPE_TERM == IsTypeTerm::YesAs){
								return Diagnostic::Info(
									"Did you mean to put parentheses around the preceding [as] operation?"
								);
							}else{
								return Diagnostic::Info("Did you mean pointer ([*]) instead?");
							}
						}();

						this->context.emitError(
							Diagnostic::Code::ParserDereferenceOrUnwrapOnType,
							this->source.getTokenBuffer().getSourceLocation(this->reader.peek(), this->source.getID()),
							"A dereference operator ([.*]) should not follow a type",
							evo::SmallVector<Diagnostic::Info>{diagnostics_info}
						);
						return Result::Code::Error;

					}else if constexpr(IS_TYPE_TERM == IsTypeTerm::Maybe){
						return Result::Code::WrongType;

					}else{
						output = this->source.ast_buffer.createPostfix(output.value(), this->reader.next());
					}

				} break;

				case Token::lookupKind(".?"): {
					if constexpr(IS_TYPE_TERM == IsTypeTerm::Yes || IS_TYPE_TERM == IsTypeTerm::YesAs){
						Diagnostic::Info diagnostics_info = [](){
							if constexpr(IS_TYPE_TERM == IsTypeTerm::YesAs){
								return Diagnostic::Info(
									"Did you mean to put parentheses around the preceding [as] operation?"
								);
							}else{
								return Diagnostic::Info("Did you mean optional ([?]) instead?");
							}
						}();

						this->context.emitError(
							Diagnostic::Code::ParserDereferenceOrUnwrapOnType,
							this->source.getTokenBuffer().getSourceLocation(this->reader.peek(), this->source.getID()),
							"An unwrap operator ([.?]) should not follow a type",
							evo::SmallVector<Diagnostic::Info>{diagnostics_info}
						);
						return Result::Code::Error;

					}else if constexpr(IS_TYPE_TERM == IsTypeTerm::Maybe){
						return Result::Code::WrongType;

					}else{
						output = this->source.ast_buffer.createPostfix(output.value(), this->reader.next());
					}

				} break;


				case Token::lookupKind("<{"): {
					if(this->assert_token_fail(Token::lookupKind("<{"))){ return Result::Code::Error; }
					
					auto args = evo::SmallVector<AST::Node>();

					bool is_first_expr = true;

					while(true){
						if(this->reader[this->reader.peek()].kind() == Token::lookupKind("}>")){
							if(this->assert_token_fail(Token::lookupKind("}>"))){ return Result::Code::Error; }
							break;
						}
						
						Result arg = this->parse_type<TypeKind::TemplateArg>();
						if(arg.code() == Result::Code::Error){
							return Result::Code::Error;

						}else if(arg.code() == Result::Code::WrongType){
							arg = this->parse_expr();
							if(this->check_result_fail(arg, "argument inside template pack")){
								return Result::Code::Error;
							}
						}

						args.emplace_back(arg.value());

						// check if ending or should continue
						const Token::Kind after_arg_next_token_kind = this->reader[this->reader.next()].kind();
						if(after_arg_next_token_kind != Token::lookupKind(",")){
							if(after_arg_next_token_kind != Token::lookupKind("}>")){
								this->expected_but_got(
									"[,] at end of template argument or [}>] at end of template argument block",
									this->reader.peek(-1)
								);
								return Result::Code::Error;
							}

							is_first_expr = false;
							break;
						}
					}

					output = this->source.ast_buffer.createTemplatedExpr(output.value(), std::move(args));
				} break;


				case Token::lookupKind("("): {
					if(this->assert_token_fail(Token::lookupKind("("))){ return Result::Code::Error; }
					
					auto args = evo::SmallVector<AST::FuncCall::Arg>();

					while(true){
						if(this->reader[this->reader.peek()].kind() == Token::lookupKind(")")){
							if(this->assert_token_fail(Token::lookupKind(")"))){ return Result::Code::Error; }
							break;
						}

						auto arg_ident = std::optional<Token::ID>();

						if(
							this->reader[this->reader.peek()].kind() == Token::Kind::Ident &&
							this->reader[this->reader.peek(1)].kind() == Token::lookupKind(":")
						){
							arg_ident = this->reader.next();
							if(this->assert_token_fail(Token::lookupKind(":"))){ return Result::Code::Error; }
						}

						const Result expr_result = this->parse_expr();
						if(this->check_result_fail(expr_result, "expression argument inside function call")){
							return Result::Code::Error;
						}

						args.emplace_back(arg_ident, expr_result.value());

						// check if ending or should continue
						const Token::Kind after_arg_next_token_kind = this->reader[this->reader.next()].kind();
						if(after_arg_next_token_kind != Token::lookupKind(",")){
							if(after_arg_next_token_kind != Token::lookupKind(")")){
								this->expected_but_got(
									"[,] at end of function call argument"
									" or [)] at end of function call argument block",
									this->reader.peek(-1)
								);
								return Result::Code::Error;
							}

							break;
						}
					}

					output = this->source.ast_buffer.createFuncCall(output.value(), std::move(args));
				} break;

				default: {
					should_continue = false;
				} break;
			}
		}

		return output;
	}


	auto Parser::parse_term_stmt() -> Result {
		const Result term = this->parse_term<IsTypeTerm::No>();
		if(term.code() != Result::Code::Success){ return term; }

		if(this->expect_token_fail(Token::lookupKind(";"), "after term statement")){ return Result::Code::Error; }

		return term;
	}


	// TODO: check EOF
	auto Parser::parse_encapsulated_expr() -> Result {
		const Result block_expr = this->parse_block(BlockLabelRequirement::Required);
		if(block_expr.code() != Result::Code::WrongType){ return block_expr; }

		if(this->reader[this->reader.peek()].kind() != Token::lookupKind("(")){
			return this->parse_atom();
		}

		const Token::ID open_token_id = this->reader.next();

		const Result inner_expr = this->parse_sub_expr();
		if(inner_expr.code() != Result::Code::Success){ return inner_expr; }

		if(this->reader[this->reader.peek()].kind() != Token::lookupKind(")")){
			const Source::Location open_location = 
				this->source.getTokenBuffer().getSourceLocation(open_token_id, this->source.getID());

			this->expected_but_got(
				"either closing [)] around expression or continuation of sub-expression",
				this->reader.peek(),
				evo::SmallVector<Diagnostic::Info>{ Diagnostic::Info("parenthesis opened here", open_location), }
			);

			return Result::Code::Error;
		}

		if(this->assert_token_fail(Token::lookupKind(")"))){ return Result::Code::Error; }

		return inner_expr;
	}

	// TODO: check EOF
	auto Parser::parse_atom() -> Result {
		Result result = this->parse_ident();
		if(result.code() != Result::Code::WrongType){ return result; }

		result = this->parse_literal();
		if(result.code() != Result::Code::WrongType){ return result; }

		result = this->parse_intrinsic();
		if(result.code() != Result::Code::WrongType){ return result; }

		result = this->parse_this();
		if(result.code() != Result::Code::WrongType){ return result; }

		return Result::Code::WrongType;
	}


	auto Parser::parse_attribute_block() -> Result {
		auto attributes = evo::SmallVector<AST::AttributeBlock::Attribute>();

		while(this->reader[this->reader.peek()].kind() == Token::Kind::Attribute){
			const Token::ID attr_token_id = this->reader.next();
			auto argument = std::optional<AST::Node>();

			if(this->reader[this->reader.peek()].kind() == Token::lookupKind("(")){
				if(this->assert_token_fail(Token::lookupKind("("))){ return Result::Code::Error; }

				const Result argument_result = this->parse_expr();
				if(this->check_result_fail(argument_result, "argument in attribute after [(]")){
					return Result::Code::Error;
				}
				argument = argument_result.value();

				if(this->expect_token_fail(Token::lookupKind(")"), "after attribute argument")){
					return Result::Code::Error;
				}

			}

			attributes.emplace_back(attr_token_id, argument);
		}

		return this->source.ast_buffer.createAttributeBlock(std::move(attributes));
	}


	auto Parser::parse_ident() -> Result {
		if(this->reader[this->reader.peek()].kind() != Token::Kind::Ident){
			return Result::Code::WrongType;
		}

		return AST::Node(AST::Kind::Ident, this->reader.next());
	}

	auto Parser::parse_intrinsic() -> Result {
		if(this->reader[this->reader.peek()].kind() != Token::Kind::Intrinsic){
			return Result::Code::WrongType;
		}

		return AST::Node(AST::Kind::Intrinsic, this->reader.next());
	}

	auto Parser::parse_literal() -> Result {
		switch(this->reader[this->reader.peek()].kind()){
			case Token::Kind::LiteralBool:
			case Token::Kind::LiteralInt:
			case Token::Kind::LiteralFloat:
			case Token::Kind::LiteralString:
			case Token::Kind::LiteralChar:
			case Token::Kind::KeywordNull:
				break;

			default:
				return Result::Code::WrongType;
		}

		return AST::Node(AST::Kind::Literal, this->reader.next());
	}


	auto Parser::parse_uninit() -> Result {
		if(this->reader[this->reader.peek()].kind() != Token::Kind::KeywordUninit){
			return Result::Code::WrongType;
		}

		return AST::Node(AST::Kind::Uninit, this->reader.next());
	}

	auto Parser::parse_zeroinit() -> Result {
		if(this->reader[this->reader.peek()].kind() != Token::Kind::KeywordZeroinit){
			return Result::Code::WrongType;
		}

		return AST::Node(AST::Kind::Zeroinit, this->reader.next());
	}

	auto Parser::parse_this() -> Result {
		if(this->reader[this->reader.peek()].kind() != Token::Kind::KeywordThis){
			return Result::Code::WrongType;
		}

		return AST::Node(AST::Kind::This, this->reader.next());
	}




	auto Parser::parse_template_pack() -> Result {
		if(this->reader[this->reader.peek()].kind() != Token::lookupKind("<{")){
			return Result::Code::WrongType;
		}

		auto params = evo::SmallVector<AST::TemplatePack::Param>();

		if(this->assert_token_fail(Token::lookupKind("<{"))){ return Result::Code::Error; }

		while(true){
			if(this->reader[this->reader.peek()].kind() == Token::lookupKind("}>")){
				if(this->assert_token_fail(Token::lookupKind("}>"))){ return Result::Code::Error; }
				break;
			}

			const Result ident = this->parse_ident();
			if(ident.code() == Result::Code::Error){
				return Result::Code::Error;
			}else if(ident.code() == Result::Code::WrongType){
				this->expected_but_got("identifier in template parameter", this->reader.peek());
				return Result::Code::Error;
			}

			if(this->expect_token_fail(Token::lookupKind(":"), "after template parameter identifier")){
				return Result::Code::Error;
			}
			
			const Result type = this->parse_type<TypeKind::Explicit>();
			if(this->check_result_fail(type, "type in template parameter declaration")){
				return Result::Code::Error;
			}

			params.emplace_back(ASTBuffer::getIdent(ident.value()), type.value());


			// check if ending or should continue
			const Token::Kind after_return_next_token_kind = this->reader[this->reader.next()].kind();
			if(after_return_next_token_kind != Token::lookupKind(",")){
				if(after_return_next_token_kind != Token::lookupKind("}>")){
					this->expected_but_got(
						"[,] at end of template parameter or [}>] at end of template pack",
						this->reader.peek(-1)
					);
					return Result::Code::Error;
				}

				break;
			}
		}


		return this->source.ast_buffer.createTemplatePack(std::move(params));
	}


	auto Parser::parse_func_params() -> evo::Result<evo::SmallVector<AST::FuncDecl::Param>> {
		auto params = evo::SmallVector<AST::FuncDecl::Param>();
		if(this->expect_token_fail(Token::lookupKind("("), "to open parameter block in function declaration")){
			return evo::resultError;
		}

		while(true){
			if(this->reader[this->reader.peek()].kind() == Token::lookupKind(")")){
				if(this->assert_token_fail(Token::lookupKind(")"))){ return evo::resultError; }
				break;
			}

			
			auto param_ident = std::optional<AST::Node>();
			auto param_type = std::optional<AST::Node>();
			using ParamKind = AST::FuncDecl::Param::Kind;
			auto param_kind = std::optional<ParamKind>();

			if(this->reader[this->reader.peek()].kind() == Token::Kind::KeywordThis){
				const Token::ID this_token_id = this->reader.next();
				param_ident = AST::Node(AST::Kind::This, this_token_id);

				switch(this->reader[this->reader.peek()].kind()){
					case Token::Kind::KeywordRead: {
						this->reader.skip();
						param_kind = ParamKind::Read;
					} break;

					case Token::Kind::KeywordMut: {
						this->reader.skip();
						param_kind = ParamKind::Mut;
					} break;

					case Token::Kind::KeywordIn: {
						this->context.emitError(
							Diagnostic::Code::ParserInvalidKindForAThisParam,
							this->source.getTokenBuffer().getSourceLocation(
								this->reader.peek(), this->source.getID()
							),
							"[this] parameters cannot have the kind [in]",
							evo::SmallVector<Diagnostic::Info>{
								Diagnostic::Info("Note: valid kinds are [read] and [mut]"),
							}
						);
						return evo::resultError;
					} break;

					default: {
						this->context.emitError(
							Diagnostic::Code::ParserInvalidKindForAThisParam,
							this->source.getTokenBuffer().getSourceLocation(
								this->reader.peek(-1), this->source.getID()
							),
							"[this] parameters cannot have a default kind",
							evo::SmallVector<Diagnostic::Info>{
								Diagnostic::Info("Note: valid kinds are [read] and [mut]"),
							}
						);
						return evo::resultError;
					} break;
				}

			}else{
				const Result param_ident_result = this->parse_ident();
				if(this->check_result_fail(param_ident_result, "identifier or [this] in function parameter")){
					return evo::resultError;
				}
				param_ident = param_ident_result.value();

				if(this->expect_token_fail(Token::lookupKind(":"), "after function parameter identifier")){
					return evo::resultError;
				}

				const Result type = this->parse_type<TypeKind::Explicit>();
				if(this->check_result_fail(type, "type after [:] in function parameter declaration")){
					return evo::resultError;
				}
				param_type = type.value();

				switch(this->reader[this->reader.peek()].kind()){
					case Token::Kind::KeywordRead: {
						this->reader.skip();
						param_kind = ParamKind::Read;
					} break;

					case Token::Kind::KeywordMut: {
						this->reader.skip();
						param_kind = ParamKind::Mut;
					} break;

					case Token::Kind::KeywordIn: {
						this->reader.skip();
						param_kind = ParamKind::In;
					} break;

					default: {
						param_kind = ParamKind::Read;
					} break;
				}
			}

			const Result attributes = this->parse_attribute_block();
			if(attributes.code() == Result::Code::Error){ return evo::resultError; }

			params.emplace_back(*param_ident, param_type, *param_kind, attributes.value());

			// check if ending or should continue
			const Token::Kind after_param_next_token_kind = this->reader[this->reader.next()].kind();
			if(after_param_next_token_kind != Token::lookupKind(",")){
				if(after_param_next_token_kind != Token::lookupKind(")")){
					this->expected_but_got(
						"[,] at end of function parameter or [)] at end of function parameters block",
						this->reader.peek(-1)
					);
					return evo::resultError;
				}

				break;
			}
		}

		return params;
	}

	#include <optional>

	auto Parser::parse_func_returns() -> evo::Result<evo::SmallVector<AST::FuncDecl::Return>> {
		auto returns = evo::SmallVector<AST::FuncDecl::Return>();

		if(this->reader[this->reader.peek()].kind() != Token::lookupKind("(")){
			const Result type = this->parse_type<TypeKind::Explicit>();
			if(this->check_result_fail(type, "Return type in function declaration")){ return evo::resultError; }	

			returns.emplace_back(std::nullopt, type.value());

			return returns;
		}


		const Token::ID start_location = this->reader.peek();
		if(this->assert_token_fail(Token::lookupKind("("))){ return evo::resultError; }

		while(true){
			if(this->reader[this->reader.peek()].kind() == Token::lookupKind(")")){
				if(this->assert_token_fail(Token::lookupKind(")"))){ return evo::resultError; }
				break;
			}

			const Result ident = this->parse_ident();
			if(ident.code() == Result::Code::Error){
				return evo::resultError;
			}else if(ident.code() == Result::Code::WrongType){
				auto infos = evo::SmallVector<Diagnostic::Info>{
					Diagnostic::Info("If a function has multiple return parameters, all must be named"),
				};
				if(returns.empty()){
					infos.emplace_back(
						"If you want a single return value that's unnamed,"
						" remove the parentheses around the return type"
					);
				}

				this->expected_but_got(
					"identifier in function return parameter", this->reader.peek(), std::move(infos)
				);
				return evo::resultError;
			}

			if(this->expect_token_fail(Token::lookupKind(":"), "after function return parameter identifier")){
				return evo::resultError;
			}
			
			const Result type = this->parse_type<TypeKind::Explicit>();
			if(this->check_result_fail(type, "type in function return parameter declaration")){
				return evo::resultError;
			}

			returns.emplace_back(ASTBuffer::getIdent(ident.value()), type.value());


			// check if ending or should continue
			const Token::Kind after_return_next_token_kind = this->reader[this->reader.next()].kind();
			if(after_return_next_token_kind != Token::lookupKind(",")){
				if(after_return_next_token_kind != Token::lookupKind(")")){
					this->expected_but_got(
						"[,] at end of function return parameter or [)] at end of function returns parameters block",
						this->reader.peek(-1)
					);
					return evo::resultError;
				}

				break;
			}
		}


		if(returns.empty()){
			this->context.emitError(
				Diagnostic::Code::ParserEmptyFuncReturnBlock,
				this->source.getTokenBuffer().getSourceLocation(start_location, this->source.getID()),
				"Function return blocks must have at least one type",
				evo::SmallVector<Diagnostic::Info>{
					Diagnostic::Info(
						"For the function to return nothing, replace the return parameter block \"()\" with \"Void\""
					)
				}
			);
			return evo::resultError;
		}


		return returns;
	}


	




	//////////////////////////////////////////////////////////////////////
	// checking

	auto Parser::expected_but_got(
		std::string_view location_str, Token::ID token_id, evo::SmallVector<Diagnostic::Info>&& infos
	) -> void {
		this->context.emitError(
			Diagnostic::Code::ParserIncorrectStmtContinuation,
			this->source.getTokenBuffer().getSourceLocation(token_id, this->source.getID()),
			std::format("Expected {}, got [{}] instead", location_str, this->reader[token_id].kind()), 
			std::move(infos)
		);
	}


	auto Parser::check_result_fail(const Result& result, std::string_view location_str) -> bool {
		switch(result.code()){
			case Result::Code::Success: {
				return false;
			} break;

			case Result::Code::WrongType: {
				this->expected_but_got(location_str, this->reader.peek());
				return true;
			} break;

			case Result::Code::Error: {
				return true;
			} break;
		}

		evo::debugFatalBreak("Unknown or unsupported result code ({})", evo::to_underlying(result.code()));
	}


	auto Parser::assert_token_fail(Token::Kind kind) -> bool {
		#if defined(PCIT_CONFIG_DEBUG)
			const Token::ID next_token_id = this->reader.next();

			if(this->reader[next_token_id].kind() == kind){ return false; }

			this->context.emitFatal(
				Diagnostic::Code::ParserAssumedTokenNotPreset,
				this->source.getTokenBuffer().getSourceLocation(next_token_id, this->source.getID()),
				Diagnostic::createFatalMessage(
					std::format("Expected [{}], got [{}] instead", kind, this->reader[next_token_id].kind())
				)
			);

			return true;

		#else
			this->reader.skip();
			return false;
		#endif
	}

	auto Parser::expect_token_fail(Token::Kind kind, std::string_view location_str) -> bool {
		const Token::ID next_token_id = this->reader.next();

		if(this->reader[next_token_id].kind() == kind){ return false; }

		this->expected_but_got(std::format("[{}] {}", kind, location_str), next_token_id);
		return true;
	}


}