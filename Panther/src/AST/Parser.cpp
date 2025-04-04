////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./Parser.h"



namespace pcit::panther{
	

	auto Parser::parse() -> evo::Result<> {
		EVO_DEFER([&](){ this->source.ast_buffer.lock(); });

		while(this->reader.at_end() == false){
			const Result stmt_result = this->parse_stmt();

			switch(stmt_result.code()){
				case Result::Code::SUCCESS: {
					this->source.ast_buffer.global_stmts.emplace_back(stmt_result.value());
				} break;

				case Result::Code::WRONG_TYPE: {
					this->context.emitError(
						Diagnostic::Code::PARSER_UNKNOWN_STMT_START,
						this->source.getTokenBuffer().getSourceLocation(this->reader.peek(), this->source.getID()),
						"Unknown start to statement"
					);
					return evo::resultError;
				} break;

				case Result::Code::ERROR: {
					return evo::resultError;
				} break;
			}
		}

		return evo::Result<>();
	}


	auto Parser::parse_stmt() -> Result {
		const Token& peeked_token = this->reader[this->reader.peek()];
		
		switch(peeked_token.kind()){
			case Token::Kind::KEYWORD_VAR:         return this->parse_var_decl<AST::VarDecl::Kind::Var>();
			case Token::Kind::KEYWORD_CONST:       return this->parse_var_decl<AST::VarDecl::Kind::Const>();
			case Token::Kind::KEYWORD_DEF:         return this->parse_var_decl<AST::VarDecl::Kind::Def>();
			case Token::Kind::KEYWORD_FUNC:        return this->parse_func_decl();
			case Token::Kind::KEYWORD_ALIAS:       return this->parse_alias_decl();
			case Token::Kind::KEYWORD_TYPE:        return this->parse_type_decl();
			case Token::Kind::KEYWORD_RETURN:      return this->parse_return();
			case Token::Kind::KEYWORD_ERROR:       return this->parse_error();
			case Token::Kind::KEYWORD_UNREACHABLE: return this->parse_unreachable();
			case Token::Kind::KEYWORD_IF:          return this->parse_conditional<false>();
			case Token::Kind::KEYWORD_WHEN:        return this->parse_conditional<true>();
			case Token::Kind::KEYWORD_WHILE:       return this->parse_while();
		}

		Result result = this->parse_assignment();
		if(result.code() != Result::Code::WRONG_TYPE){ return result; }

		result = this->parse_term_stmt();
		if(result.code() != Result::Code::WRONG_TYPE){ return result; }

		return this->parse_block(BlockLabelRequirement::NOT_ALLOWED);
	}


	// TODO: check EOF
	template<AST::VarDecl::Kind VAR_DECL_KIND>
	auto Parser::parse_var_decl() -> Result {
		if constexpr(VAR_DECL_KIND == AST::VarDecl::Kind::Var){
			if(this->assert_token_fail(Token::Kind::KEYWORD_VAR)){ return Result::Code::ERROR; }

		}else if constexpr(VAR_DECL_KIND == AST::VarDecl::Kind::Const){
			if(this->assert_token_fail(Token::Kind::KEYWORD_CONST)){ return Result::Code::ERROR; }
			
		}else{
			if(this->assert_token_fail(Token::Kind::KEYWORD_DEF)){ return Result::Code::ERROR; }
		}

		const Result ident = this->parse_ident();
		if(this->check_result_fail(ident, "identifier in variable declaration")){ return Result::Code::ERROR; }


		auto type = std::optional<AST::Node>();
		if(this->reader[this->reader.peek()].kind() == Token::lookupKind(":")){
			if(this->assert_token_fail(Token::lookupKind(":"))){ return Result::Code::ERROR; }

			const Result type_result = this->parse_type<TypeKind::EXPLICIT>();
			if(this->check_result_fail(type_result, "type after [:] in variable declaration")){
				return Result::Code::ERROR;
			}

			type = type_result.value();
		}


		const Result attributes = this->parse_attribute_block();
		if(attributes.code() == Result::Code::ERROR){ return Result::Code::ERROR; }


		auto value = std::optional<AST::Node>();
		if(this->reader[this->reader.peek()].kind() == Token::lookupKind("=")){
			if(this->assert_token_fail(Token::lookupKind("="))){ return Result::Code::ERROR; }
				
			const Result value_result = this->parse_expr();
			// TODO: better messaging around block exprs missing a label
			if(this->check_result_fail(value_result, "expression after [=] in variable declaration")){
				return Result::Code::ERROR;
			}

			value = value_result.value();
		}


		if(this->expect_token_fail(Token::lookupKind(";"), "after variable declaration")){ return Result::Code::ERROR; }


		return this->source.ast_buffer.createVarDecl(
			VAR_DECL_KIND, ASTBuffer::getIdent(ident.value()), type, attributes.value(), value
		);
	}


	// TODO: check EOF
	auto Parser::parse_func_decl() -> Result {
		if(this->assert_token_fail(Token::Kind::KEYWORD_FUNC)){ return Result::Code::ERROR; }

		const Result ident = this->parse_ident();
		if(this->check_result_fail(ident, "identifier after [func] in function declaration")){
			return Result::Code::ERROR;
		}


		if(this->expect_token_fail(Token::lookupKind("="), "after identifier in function declaration")){
			if(this->reader[this->reader.peek()].kind() == Token::Kind::ATTRIBUTE){
				this->context.emitError(
					Diagnostic::Code::PARSER_ATTRIBUTES_IN_WRONG_PLACE,
					this->source.getTokenBuffer().getSourceLocation(this->reader.peek(), this->source.getID()),
					"Attributes for function declaration in the wrong place",
					evo::SmallVector<Diagnostic::Info>{
						Diagnostic::Info("Attributes should be after the parameters block")
					}
				);
			}
			return Result::Code::ERROR;
		}

		auto template_pack_node = std::optional<AST::Node>();
		const Result template_pack_result = this->parse_template_pack();
		switch(template_pack_result.code()){
			case Result::Code::SUCCESS:   template_pack_node = template_pack_result.value(); break;
			case Result::Code::WRONG_TYPE: break;
			case Result::Code::ERROR:     return Result::Code::ERROR;	
		}

		evo::Result<evo::SmallVector<AST::FuncDecl::Param>> params = this->parse_func_params();
		if(params.isError()){ return Result::Code::ERROR; }


		const Result attribute_block = this->parse_attribute_block();
		if(attribute_block.code() == Result::Code::ERROR){ return Result::Code::ERROR; }


		if(this->expect_token_fail(Token::lookupKind("->"), "in function declaration")){ return Result::Code::ERROR; }

		evo::Result<evo::SmallVector<AST::FuncDecl::Return>> returns = this->parse_func_returns();
		if(returns.isError()){ return Result::Code::ERROR; }

		evo::Result<evo::SmallVector<AST::FuncDecl::Return>> error_returns = this->parse_func_error_returns();
		if(error_returns.isError()){ return Result::Code::ERROR; }

		const Result block = this->parse_block(BlockLabelRequirement::NOT_ALLOWED);
		if(this->check_result_fail(block, "statement block in function declaration")){ // TODO: better messaging 
			return Result::Code::ERROR;
		}

		return this->source.ast_buffer.createFuncDecl(
			ident.value(),
			template_pack_node,
			std::move(params.value()),
			attribute_block.value(),
			std::move(returns.value()),
			std::move(error_returns.value()),
			block.value()
		);
	}


	// TODO: check EOF
	auto Parser::parse_alias_decl() -> Result {
		if(this->assert_token_fail(Token::Kind::KEYWORD_ALIAS)){ return Result::Code::ERROR; }

		const Result ident = this->parse_ident();
		if(this->check_result_fail(ident, "identifier in alias declaration")){ return Result::Code::ERROR; }

		const Result attributes = this->parse_attribute_block();
		if(attributes.code() == Result::Code::ERROR){ return Result::Code::ERROR; }

		if(this->expect_token_fail(Token::lookupKind("="), "in alias declaration")){ return Result::Code::ERROR; }

		const Result type = this->parse_type<TypeKind::EXPLICIT>();
		if(this->check_result_fail(type, "type in alias declaration")){ return Result::Code::ERROR; }

		if(this->expect_token_fail(Token::lookupKind(";"), "at end of alias declaration")){ return Result::Code::ERROR;}

		return this->source.ast_buffer.createAliasDecl(
			ASTBuffer::getIdent(ident.value()), attributes.value(), type.value()
		);
	}

	// TODO: check EOF
	auto Parser::parse_type_decl() -> Result {
		if(this->assert_token_fail(Token::Kind::KEYWORD_TYPE)){ return Result::Code::ERROR; }

		const Result ident = this->parse_ident();
		if(this->check_result_fail(ident, "identifier in type declaration")){ return Result::Code::ERROR; }

		const Result attributes = this->parse_attribute_block();
		if(attributes.code() == Result::Code::ERROR){ return Result::Code::ERROR; }

		if(this->expect_token_fail(Token::lookupKind("="), "in type declaration")){ return Result::Code::ERROR; }

		if(this->reader[this->reader.peek()].kind() == Token::Kind::KEYWORD_STRUCT){
			return this->parse_struct_decl(ident.value(), attributes.value());
		}

		const Result type = this->parse_type<TypeKind::EXPLICIT>();
		if(this->check_result_fail(type, "type in alias declaration")){ return Result::Code::ERROR; }

		if(this->expect_token_fail(Token::lookupKind(";"), "at end of alias declaration")){ return Result::Code::ERROR;}

		return this->source.ast_buffer.createTypedefDecl(
			ASTBuffer::getIdent(ident.value()), attributes.value(), type.value()
		);
	}


	// TODO: check EOF
	auto Parser::parse_struct_decl(const AST::Node& ident, const AST::Node& attrs_pre_equals) -> Result {
		if(this->assert_token_fail(Token::Kind::KEYWORD_STRUCT)){ return Result::Code::ERROR; }

		if(this->source.getASTBuffer().getAttributeBlock(attrs_pre_equals).attributes.empty() == false){
			this->context.emitError(
				Diagnostic::Code::PARSER_ATTRIBUTES_IN_WRONG_PLACE,
				this->source.getTokenBuffer().getSourceLocation(
					this->source.getASTBuffer().getAttributeBlock(attrs_pre_equals).attributes.front().attribute,
					this->source.getID()
				),
				"Attributes for struct declaration in the wrong place",
				evo::SmallVector<Diagnostic::Info>{
					Diagnostic::Info("Attributes should be after the [struct] keyword"
						" and after the template parameter block (if there is one)")
				}
			);
			return Result::Code::ERROR;
		}

		auto template_pack_node = std::optional<AST::Node>();
		const Result template_pack_result = this->parse_template_pack();
		switch(template_pack_result.code()){
			case Result::Code::SUCCESS:   template_pack_node = template_pack_result.value(); break;
			case Result::Code::WRONG_TYPE: break;
			case Result::Code::ERROR:     return Result::Code::ERROR;	
		}

		const Result attributes = this->parse_attribute_block();
		if(attributes.code() == Result::Code::ERROR){ return Result::Code::ERROR; }

		const Result block = this->parse_block(BlockLabelRequirement::NOT_ALLOWED);
		if(this->check_result_fail(block, "statement block in struct declaration")){ // TODO: better messaging 
			return Result::Code::ERROR;
		}

		return this->source.ast_buffer.createStructDecl(
			ASTBuffer::getIdent(ident), template_pack_node, std::move(attributes.value()), block.value()
		);
	}


	// TODO: check EOF
	auto Parser::parse_return() -> Result {
		const Token::ID start_location = this->reader.peek();
		if(this->assert_token_fail(Token::Kind::KEYWORD_RETURN)){ return Result::Code::ERROR; }

		auto label = std::optional<AST::Node>();
		auto expr = evo::Variant<std::monostate, AST::Node, Token::ID>();

		if(this->reader[this->reader.peek()].kind() == Token::lookupKind("->")){
			if(this->assert_token_fail(Token::lookupKind("->"))){ return Result::Code::ERROR; }

			const Result label_result = this->parse_ident();
			if(this->check_result_fail(label_result, "identifier in return block label")){ return Result::Code::ERROR; }
			label = label_result.value();
		}

		if(this->reader[this->reader.peek()].kind() == Token::lookupKind("...")){
			expr = this->reader.next();
		}else{
			const Result expr_result = this->parse_expr();
			if(expr_result.code() == Result::Code::ERROR){
				return Result::Code::ERROR;
			}else if(expr_result.code() == Result::Code::SUCCESS){
				expr = expr_result.value();
			}
		}

		if(this->expect_token_fail(Token::lookupKind(";"), "at the end of a [return] statement")){
			return Result::Code::ERROR;
		}

		return this->source.ast_buffer.createReturn(start_location, label, expr);
	}

	// TODO: check EOF
	auto Parser::parse_error() -> Result {
		const Token::ID start_location = this->reader.peek();
		if(this->assert_token_fail(Token::Kind::KEYWORD_ERROR)){ return Result::Code::ERROR; }

		auto expr = evo::Variant<std::monostate, AST::Node, Token::ID>();

		if(this->reader[this->reader.peek()].kind() == Token::lookupKind("...")){
			expr = this->reader.next();
		}else{
			const Result expr_result = this->parse_expr();
			if(expr_result.code() == Result::Code::ERROR){
				return Result::Code::ERROR;
			}else if(expr_result.code() == Result::Code::SUCCESS){
				expr = expr_result.value();
			}
		}

		if(this->expect_token_fail(Token::lookupKind(";"), "at the end of a [error] statement")){
			return Result::Code::ERROR;
		}

		return this->source.ast_buffer.createError(start_location, expr);
	}


	// TODO: check EOF
	auto Parser::parse_unreachable() -> Result {
		const Token::ID start_location = this->reader.peek();
		if(this->assert_token_fail(Token::Kind::KEYWORD_UNREACHABLE)){ return Result::Code::ERROR; }

		if(this->expect_token_fail(Token::lookupKind(";"), "at the end of a [unreachable] statement")){
			return Result::Code::ERROR;
		}

		return AST::Node(AST::Kind::UNREACHABLE, start_location);
	}


	// TODO: check EOF
	template<bool IS_WHEN>
	auto Parser::parse_conditional() -> Result {
		const Token::ID keyword_token_id = this->reader.peek();

		static constexpr Token::Kind COND_TOKEN_KIND = IS_WHEN ? Token::Kind::KEYWORD_WHEN : Token::Kind::KEYWORD_IF;
		if(this->assert_token_fail(COND_TOKEN_KIND)){ return Result::Code::ERROR; }


		if(this->expect_token_fail(Token::lookupKind("("), "in conditional statement")){ return Result::Code::ERROR; }

		const Result cond = this->parse_expr();

		if(this->expect_token_fail(Token::lookupKind(")"), "in conditional statement")){ return Result::Code::ERROR; }

		const Result then_block = this->parse_block(BlockLabelRequirement::NOT_ALLOWED);
		if(this->check_result_fail(then_block, "statement block in conditional statement")){
			return Result::Code::ERROR;
		}

		auto else_block = std::optional<AST::Node>();
		if(this->reader.at_end() == false && this->reader[this->reader.peek()].kind() == Token::Kind::KEYWORD_ELSE){
			if(this->assert_token_fail(Token::Kind::KEYWORD_ELSE)){ return Result::Code::ERROR; }

			const Token::Kind else_if_kind = this->reader[this->reader.peek()].kind();

			if(else_if_kind == Token::lookupKind("{")){
				const Result else_block_result = this->parse_block(BlockLabelRequirement::NOT_ALLOWED);
				if(this->check_result_fail(else_block_result, "statement block in conditional statement")){
					return Result::Code::ERROR;
				}

				else_block = else_block_result.value();

			}else if(else_if_kind != COND_TOKEN_KIND){
				if constexpr(IS_WHEN){
					if(else_if_kind == Token::Kind::KEYWORD_IF){
						this->expected_but_got(
							"[when] after [else]",
							this->reader.peek(),
							evo::SmallVector<Diagnostic::Info>{
								Diagnostic::Info("Cannot mix [if] and [when] in a chain"), // TODO: better messaging
							}
						);
					}else{
						this->expected_but_got("[when] or [{] after [else]", this->reader.peek());
					}
				}else{
					if(else_if_kind == Token::Kind::KEYWORD_WHEN){
						this->expected_but_got(
							"[if] after [else]",
							this->reader.peek(),
							evo::SmallVector<Diagnostic::Info>{
								Diagnostic::Info("Cannot mix [if] and [when] in a chain"), // TODO: better messaging
							}
						);
					}else{
						this->expected_but_got("[if] or [{] after [else]", this->reader.peek());
					}
				}
				return Result::Code::ERROR;

			}else{
				const Result else_block_result = this->parse_conditional<IS_WHEN>();
				if(this->check_result_fail(else_block_result, "statement block in conditional statement")){
					return Result::Code::ERROR;
				}

				else_block = else_block_result.value();
			}
		}


		if constexpr(IS_WHEN){
			return this->source.ast_buffer.createWhenConditional(
				keyword_token_id, cond.value(), then_block.value(), else_block
			);
		}else{
			return this->source.ast_buffer.createConditional(
				keyword_token_id, cond.value(), then_block.value(), else_block
			);
		}
	}


	auto Parser::parse_while() -> Result {
		const Token::ID start_location = this->reader.peek();
		
		if(this->assert_token_fail(Token::Kind::KEYWORD_WHILE)){ return Result::Code::ERROR; }

		if(this->expect_token_fail(Token::lookupKind("("), "in while loop")){ return Result::Code::ERROR; }

		const Result cond = this->parse_expr();
		if(this->check_result_fail(cond, "condition in while loop")){ return Result::Code::ERROR; }

		if(this->expect_token_fail(Token::lookupKind(")"), "in while loop")){ return Result::Code::ERROR; }

		const Result block = this->parse_block(BlockLabelRequirement::OPTIONAL);
		if(this->check_result_fail(block, "statement block in while loop")){ return Result::Code::ERROR; }

		return this->source.ast_buffer.createWhile(start_location, cond.value(), block.value());
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
					return Result::Code::ERROR;
				}

				const Result value = this->parse_expr();
				if(this->check_result_fail(value, "expression value in discard assignment")){
					return Result::Code::ERROR;
				}

				if(this->expect_token_fail(Token::lookupKind(";"), "at end of discard assignment")){
					return Result::Code::ERROR;
				}

				return this->source.ast_buffer.createInfix(
					AST::Node(AST::Kind::DISCARD, discard_token_id), op_token_id, value.value()
				);

			}else if(peeked_kind == Token::lookupKind("[")){ // multi assign
				if(this->assert_token_fail(Token::lookupKind("["))){ return Result::Code::ERROR; }

				auto assignments = evo::SmallVector<AST::Node>();
				while(true){
					if(this->reader[this->reader.peek()].kind() == Token::lookupKind("]")){
						if(this->assert_token_fail(Token::lookupKind("]"))){ return Result::Code::ERROR; }
						break;
					}

					const Result assignment = [&]() {
						const Result ident_result = this->parse_ident();
						if(ident_result.code() != Result::Code::WRONG_TYPE){ return ident_result; }

						const Token::ID peeked_token_id = this->reader.next();
						const Token::Kind peeked_kind = this->reader[peeked_token_id].kind();
						if(peeked_kind == Token::lookupKind("_")){
							return Result(AST::Node(AST::Kind::DISCARD, peeked_token_id));
						}else{
							return Result(Result::Code::WRONG_TYPE);
						}
					}();
					if(assignment.code() != Result::Code::SUCCESS){ return Result::Code::ERROR; }

					assignments.emplace_back(assignment.value());


					// check if ending or should continue
					const Token::Kind after_ident_next_token_kind = this->reader[this->reader.next()].kind();
					if(after_ident_next_token_kind != Token::lookupKind(",")){
						if(after_ident_next_token_kind != Token::lookupKind("]")){
							this->expected_but_got(
								"[,] after identifier or []] at end of multiple assignemt identifier block",
								this->reader.peek(-1)
							);
							return Result::Code::ERROR;
						}

						break;
					}
				}

				if(assignments.empty()){
					this->context.emitError(
						Diagnostic::Code::PARSER_EMPTY_MULTI_ASSIGN,
						this->source.getTokenBuffer().getSourceLocation(this->reader.peek(-1), this->source.getID()),
						"Multiple-assignment statements cannot assign to 0 values"
					);
				}

				if(this->expect_token_fail(Token::lookupKind("="), "in multiple-assignment")){
					return Result::Code::ERROR;
				}

				const Result value = this->parse_expr();
				if(this->check_result_fail(value, "expression value in multiple-assignment")){
					return Result::Code::ERROR;
				}

				if(this->expect_token_fail(Token::lookupKind(";"), "at end of multiple-assignment")){
					return Result::Code::ERROR;
				}

				return this->source.ast_buffer.createMultiAssign(start_location, std::move(assignments), value.value());
			}
		}



		const Result lhs = this->parse_term<TermKind::EXPR>();
		if(lhs.code() != Result::Code::SUCCESS){ return lhs; }

		const Token::ID op_token_id = this->reader.next();
		switch(this->reader[op_token_id].kind()){
			case Token::lookupKind("="):
			case Token::lookupKind("+="):  case Token::lookupKind("+%="):  case Token::lookupKind("+|="):
			case Token::lookupKind("-="):  case Token::lookupKind("-%="):  case Token::lookupKind("-|="):
			case Token::lookupKind("*="):  case Token::lookupKind("*%="):  case Token::lookupKind("*|="):
			case Token::lookupKind("/="):  case Token::lookupKind("%="):
			case Token::lookupKind("<<="): case Token::lookupKind("<<|="): case Token::lookupKind(">>="):
			case Token::lookupKind("&="):  case Token::lookupKind("|="):   case Token::lookupKind("^="):
				break;

			default: {
				this->reader.go_back(start_location);
				return Result::Code::WRONG_TYPE;
			} break;
		}

		const Result rhs = this->parse_expr();
		if(this->check_result_fail(rhs, "expression value in assignment")){ return Result::Code::ERROR; }

		if(this->expect_token_fail(Token::lookupKind(";"), "at end of assignment")){ return Result::Code::ERROR; }

		return this->source.ast_buffer.createInfix(lhs.value(), op_token_id, rhs.value());
	}




	// TODO: check EOF
	auto Parser::parse_block(BlockLabelRequirement label_requirement) -> Result {
		const Token::ID start_location = this->reader.peek();

		if(this->reader[this->reader.peek()].kind() != Token::lookupKind("{")){ return Result::Code::WRONG_TYPE; }
		if(this->assert_token_fail(Token::lookupKind("{"))){ return Result::Code::ERROR; }

		auto label = std::optional<AST::Node>();
		auto label_type = std::optional<AST::Node>();

		if(this->reader[this->reader.peek()].kind() == Token::lookupKind("->")){
			if(label_requirement == BlockLabelRequirement::NOT_ALLOWED){
				this->reader.go_back(start_location);
				return Result::Code::WRONG_TYPE;
			}

			if(this->assert_token_fail(Token::lookupKind("->"))){ return Result::Code::ERROR; }
			if(this->expect_token_fail(Token::lookupKind("("), "[(] around block label")){ return Result::Code::ERROR; }

			const Result label_result = this->parse_ident();
			if(this->check_result_fail(label_result, "identifier in block label")){ return Result::Code::ERROR; }
			label = label_result.value();

			if(this->reader[this->reader.peek()].kind() == Token::lookupKind(":")){
				if(label_requirement == BlockLabelRequirement::OPTIONAL){
					this->context.emitError(
						Diagnostic::Code::PARSER_INCORRECT_STMT_CONTINUATION,
						this->source.getTokenBuffer().getSourceLocation(this->reader.peek(), this->source.getID()),
						"This labeled block is not allowed to have an explicit type",
						evo::SmallVector<Diagnostic::Info>{
							Diagnostic::Info("Note: Only expression blocks may have types")
						}
					);
					return Result::Code::ERROR;
				}

				if(this->assert_token_fail(Token::lookupKind(":"))){ return Result::Code::ERROR; }

				const Result type_result = this->parse_type<TypeKind::EXPLICIT>();
				if(this->check_result_fail(type_result, "type in explicitly-typed labeled expression block")){
					return Result::Code::ERROR;
				}


				label_type = type_result.value();
			}

			if(this->expect_token_fail(
				Token::lookupKind(")"), "after type in explicitly-typed labeled expression block")
			){ return Result::Code::ERROR; }

		}else if(label_requirement == BlockLabelRequirement::REQUIRED){
			this->reader.go_back(start_location);
			return Result::Code::WRONG_TYPE;
		}

		auto statements = evo::SmallVector<AST::Node>();

		while(true){
			if(this->reader[this->reader.peek()].kind() == Token::lookupKind("}")){
				if(this->assert_token_fail(Token::lookupKind("}"))){ return Result::Code::ERROR; }
				break;
			}

			const Result stmt = this->parse_stmt();
			if(this->check_result_fail(stmt, "statement in statement block")){ return Result::Code::ERROR; }

			statements.emplace_back(stmt.value());
		}

		return this->source.ast_buffer.createBlock(
			start_location, std::move(label), std::move(label_type), std::move(statements)
		);
	}


	// TODO: check EOF
	template<Parser::TypeKind KIND>
	auto Parser::parse_type() -> Result {
		const Token::ID start_location = this->reader.peek();
		bool is_primitive = true;
		switch(this->reader[start_location].kind()){
			case Token::Kind::TYPE_VOID:
			case Token::Kind::TYPE_THIS:
			case Token::Kind::TYPE_INT:
			case Token::Kind::TYPE_ISIZE:
			case Token::Kind::TYPE_I_N:
			case Token::Kind::TYPE_UINT:
			case Token::Kind::TYPE_USIZE:
			case Token::Kind::TYPE_UI_N:
			case Token::Kind::TYPE_F16:
			case Token::Kind::TYPE_BF16:
			case Token::Kind::TYPE_F32:
			case Token::Kind::TYPE_F64:
			case Token::Kind::TYPE_F80:
			case Token::Kind::TYPE_F128:
			case Token::Kind::TYPE_BYTE:
			case Token::Kind::TYPE_BOOL:
			case Token::Kind::TYPE_CHAR:
			case Token::Kind::TYPE_RAWPTR:
			case Token::Kind::TYPE_TYPEID:
			case Token::Kind::TYPE_C_SHORT:
			case Token::Kind::TYPE_C_USHORT:
			case Token::Kind::TYPE_C_INT:
			case Token::Kind::TYPE_C_UINT:
			case Token::Kind::TYPE_C_LONG:
			case Token::Kind::TYPE_C_ULONG:
			case Token::Kind::TYPE_C_LONG_LONG:
			case Token::Kind::TYPE_C_ULONG_LONG:
			case Token::Kind::TYPE_C_LONG_DOUBLE:
				break;

			case Token::Kind::TYPE_TYPE: {
				if(this->reader[this->reader.peek(1)].kind() == Token::lookupKind("(")){
					is_primitive = false;
				}
			} break;

			case Token::Kind::IDENT: case Token::Kind::INTRINSIC: {
				is_primitive = false;
			} break;

			default: return Result::Code::WRONG_TYPE;
		}

		const Result base_type = [&]() {
			if(is_primitive){
				const Token::ID base_type_token_id = this->reader.next();

				const Token& peeked_token = this->reader[this->reader.peek()];
				if(peeked_token.kind() == Token::lookupKind(".*")){
					const Diagnostic::Info diagnostics_info = [](){
						if constexpr(KIND == TypeKind::EXPR || KIND == TypeKind::TEMPLATE_ARG){
							return Diagnostic::Info(
								"Did you mean to put parentheses around the preceding [as] operation?"
							);
						}else{
							return Diagnostic::Info("Did you mean pointer ([*]) instead?");
						}
					}();

					this->context.emitError(
						Diagnostic::Code::PARSER_DEREFERENCE_OR_UNWRAP_ON_TYPE,
						this->source.getTokenBuffer().getSourceLocation(this->reader.peek(), this->source.getID()),
						"A dereference operator ([.*]) should not follow a type",
						evo::SmallVector<Diagnostic::Info>{diagnostics_info}
					);
					return Result(Result::Code::ERROR);

				}else if(peeked_token.kind() == Token::lookupKind(".?")){
					const Diagnostic::Info diagnostics_info = [](){
						if constexpr(KIND == TypeKind::EXPR || KIND == TypeKind::TEMPLATE_ARG){
							return Diagnostic::Info(
								"Did you mean to put parentheses around the preceding [as] operation?"
							);
						}else{
							return Diagnostic::Info("Did you mean optional ([?]) instead?");
						}
					}();

					this->context.emitError(
						Diagnostic::Code::PARSER_DEREFERENCE_OR_UNWRAP_ON_TYPE,
						this->source.getTokenBuffer().getSourceLocation(this->reader.peek(), this->source.getID()),
						"A unwrap operator ([.?]) should not follow a type",
						evo::SmallVector<Diagnostic::Info>{diagnostics_info}
					);
					return Result(Result::Code::ERROR);
				}

				return Result(AST::Node(AST::Kind::PRIMITIVE_TYPE, base_type_token_id));

			}else if(this->reader[start_location].kind() == Token::Kind::TYPE_TYPE){
				if(this->assert_token_fail(Token::Kind::TYPE_TYPE)){ return Result(Result::Code::ERROR); }
				if(this->assert_token_fail(Token::lookupKind("("))){ return Result(Result::Code::ERROR); }

				const Result type_id_expr = this->parse_expr();
				if(this->check_result_fail(type_id_expr, "expression in TypeID converter")){
					return Result(Result::Code::ERROR);
				}

				if(this->expect_token_fail(Token::lookupKind(")"), "after expression in TypeID converter")){
					return Result(Result::Code::ERROR);
				}

				return Result(this->source.ast_buffer.createTypeIDConverter(type_id_expr.value()));

			}else{
				if constexpr(KIND == TypeKind::EXPLICIT){
					return this->parse_term<TermKind::EXPLICIT_TYPE>();

				}else if constexpr(KIND == TypeKind::EXPR){
					return this->parse_term<TermKind::AS_TYPE>();

				}else if constexpr(KIND == TypeKind::TEMPLATE_ARG){
					return this->parse_term<TermKind::TEMPLATE_EXPR>();

				}else{
					static_assert(false, "Unknown TypeKind");
				}
			}
		}();

		if(base_type.code() == Result::Code::ERROR){
			return Result::Code::ERROR;
		}else if(base_type.code() == Result::Code::WRONG_TYPE){
			if(this->reader.peek() != start_location){
				this->reader.go_back(start_location);
			}
			return Result::Code::WRONG_TYPE;
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
				if(this->assert_token_fail(Token::lookupKind("*"))){ return Result::Code::ERROR; }
				
				if(this->reader[this->reader.peek()].kind() == Token::lookupKind("|")){
					is_read_only = true;
					potential_backup_location = this->reader.peek();
					if(this->assert_token_fail(Token::lookupKind("|"))){ return Result::Code::ERROR; }
				}

			}else if(this->reader[this->reader.peek()].kind() == Token::lookupKind("*|")){
				continue_looking_for_qualifiers = true;
				is_ptr = true;
				is_read_only = true;
				potential_backup_location = this->reader.peek();
				if(this->assert_token_fail(Token::lookupKind("*|"))){ return Result::Code::ERROR; }
			}

			if(this->reader[this->reader.peek()].kind() == Token::lookupKind("?")){
				continue_looking_for_qualifiers = true;
				is_optional = true;
				if(this->assert_token_fail(Token::lookupKind("?"))){ return Result::Code::ERROR; }
			}

			if(continue_looking_for_qualifiers){
				qualifiers.emplace_back(is_ptr, is_read_only, is_optional);
			}
		}


		if constexpr(KIND == TypeKind::EXPR || KIND == TypeKind::TEMPLATE_ARG){
			// make sure exprs like `a as Int * b` gets parsed like `(a as Int) * b`
			if(qualifiers.empty() == false && qualifiers.back().isOptional == false){
				switch(this->reader[this->reader.peek()].kind()){
					case Token::Kind::IDENT:
					case Token::lookupKind("("):
					case Token::Kind::LITERAL_BOOL:
					case Token::Kind::LITERAL_INT:
					case Token::Kind::LITERAL_FLOAT:
					case Token::Kind::LITERAL_STRING:
					case Token::Kind::LITERAL_CHAR: {
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
			if constexpr(KIND == TypeKind::TEMPLATE_ARG){
				if(is_primitive == false && qualifiers.empty()){
					this->reader.go_back(start_location);
					return Result::Code::WRONG_TYPE;
				}
			}
		}

		return this->source.ast_buffer.createType(base_type.value(), std::move(qualifiers));
	}



	// TODO: check EOF
	auto Parser::parse_expr() -> Result {
		Result result = this->parse_uninit();
		if(result.code() != Result::Code::WRONG_TYPE){ return result; }

		result = this->parse_zeroinit();
		if(result.code() != Result::Code::WRONG_TYPE){ return result; }

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
			case Token::lookupKind("+%"):  return 6;
			case Token::lookupKind("+|"):  return 6;
			case Token::lookupKind("-"):   return 6;
			case Token::lookupKind("-%"):  return 6;
			case Token::lookupKind("-|"):  return 6;

			case Token::lookupKind("*"):   return 7;
			case Token::lookupKind("*%"):  return 7;
			case Token::lookupKind("*|"):  return 7;
			case Token::lookupKind("/"):   return 7;
			case Token::lookupKind("%"):   return 7;

			case Token::Kind::KEYWORD_AS:  return 8;
		}

		return -1;
	}

	// TODO: check EOF
	auto Parser::parse_infix_expr() -> Result {
		const Result lhs_result = this->parse_prefix_expr();
		if(lhs_result.code() != Result::Code::SUCCESS){ return lhs_result; }

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

		if(this->assert_token_fail(peeked_op_kind)){ return Result::Code::ERROR; }

		const Result rhs_result = [&](){
			if(peeked_op_kind == Token::Kind::KEYWORD_AS){
				const Result type_result = this->parse_type<TypeKind::EXPR>();
				if(this->check_result_fail(type_result, "type after [as] operator")){
					return Result(Result::Code::ERROR);
				}

				if(this->reader[this->reader.peek()].kind() == Token::lookupKind(".*")){
					this->context.emitError(
						Diagnostic::Code::PARSER_DEREFERENCE_OR_UNWRAP_ON_TYPE,
						this->source.getTokenBuffer().getSourceLocation(this->reader.peek(), this->source.getID()),
						"A dereference operator ([.*]) should not follow a type",
						evo::SmallVector<Diagnostic::Info>{
							Diagnostic::Info("Did you mean to put parentheses around the preceding [as] operation?")
						}
					);
					return Result(Result::Code::ERROR);

				}else if(this->reader[this->reader.peek()].kind() == Token::lookupKind(".?")){
					this->context.emitError(
						Diagnostic::Code::PARSER_DEREFERENCE_OR_UNWRAP_ON_TYPE,
						this->source.getTokenBuffer().getSourceLocation(this->reader.peek(), this->source.getID()),
						"A dereference operator ([.?]) should not follow a type",
						evo::SmallVector<Diagnostic::Info>{
							Diagnostic::Info("Did you mean to put parentheses around the preceding [as] operation?")
						}
					);
					return Result(Result::Code::ERROR);
				}

				return type_result;
			}
			
			const Result next_part_of_expr = this->parse_prefix_expr();
			if(next_part_of_expr.code() != Result::Code::SUCCESS){ return next_part_of_expr; }

			return this->parse_infix_expr_impl(next_part_of_expr.value(), next_op_prec);
		}();

		if(rhs_result.code() != Result::Code::SUCCESS){ return rhs_result; }

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
			case Token::lookupKind("&|"):
			case Token::Kind::KEYWORD_COPY:
			case Token::Kind::KEYWORD_MOVE:
			case Token::Kind::KEYWORD_DESTRUCTIVE_MOVE:
			case Token::Kind::KEYWORD_FORWARD:
			case Token::lookupKind("-"):
			case Token::lookupKind("!"):
			case Token::lookupKind("~"):
				break;

			case Token::Kind::KEYWORD_NEW:
				return this->parse_new_expr();

			default:
				return this->parse_term<TermKind::EXPR>();
		}

		if(this->assert_token_fail(op_token_kind)){ return Result::Code::ERROR; }

		const Result rhs = this->parse_term<TermKind::EXPR>();
		if(this->check_result_fail(
			rhs, std::format("valid sub-expression on right-hand side of prefix [{}] operator", op_token_kind)
		)){
			return Result::Code::ERROR;
		}

		return this->source.ast_buffer.createPrefix(op_token_id, rhs.value());
	}


	auto Parser::parse_new_expr() -> Result {
		if(this->assert_token_fail(Token::Kind::KEYWORD_NEW)){ return Result::Code::ERROR; }
			
		const Result type = this->parse_type<TypeKind::EXPLICIT>();
		if(this->check_result_fail(type, "type in new expression")){ return Result::Code::ERROR; }

		switch(this->reader[this->reader.peek()].kind()){
			case Token::lookupKind("("): {
				evo::Result<evo::SmallVector<AST::FuncCall::Arg>> args = this->parse_func_call_args();
				if(args.isError()){ return Result::Code::ERROR; }

				return this->source.ast_buffer.createNew(type.value(), std::move(args.value()));
			} break;

			case Token::lookupKind("{"): {
				this->context.emitError(
					Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
					this->source.getTokenBuffer().getSourceLocation(this->reader.peek(), this->source.getID()),
					"Struct initializer lists are currently unsupported"
				);
				return Result::Code::ERROR;
			} break;
		}

		this->expected_but_got(
			"function call arguments or struct initializer list after type in operator [new]", this->reader.peek()
		);
		return Result::Code::ERROR;
	}



	// TODO: check EOF
	template<Parser::TermKind TERM_KIND>
	auto Parser::parse_term() -> Result {
		Result output = [&](){
			if constexpr(TERM_KIND == TermKind::EXPLICIT_TYPE || TERM_KIND == TermKind::AS_TYPE){
				return this->parse_ident();
			}else{
				return this->parse_encapsulated_expr();
			}
		}();
		if(output.code() != Result::Code::SUCCESS){ return output; }

		bool should_continue = true;
		while(should_continue){
			switch(this->reader[this->reader.peek()].kind()){
				case Token::lookupKind("."): {
					const Token::ID accessor_op_token_id = this->reader.next();	

					Result rhs_result = this->parse_ident();
					if(rhs_result.code() == Result::Code::ERROR){
						return Result::Code::ERROR;

					}else if(rhs_result.code() == Result::Code::WRONG_TYPE){
						rhs_result = this->parse_intrinsic();
						if(this->check_result_fail(rhs_result, "identifier or intrinsic after [.] accessor")){
							return Result::Code::ERROR;
						}
					}

					output = this->source.ast_buffer.createInfix(
						output.value(), accessor_op_token_id, rhs_result.value()
					);
				} break;

				case Token::lookupKind(".*"): {
					if constexpr(TERM_KIND == TermKind::EXPLICIT_TYPE || TERM_KIND == TermKind::AS_TYPE){
						Diagnostic::Info diagnostics_info = [](){
							if constexpr(TERM_KIND == TermKind::AS_TYPE){
								return Diagnostic::Info(
									"Did you mean to put parentheses around the preceding [as] operation?"
								);
							}else{
								return Diagnostic::Info("Did you mean pointer ([*]) instead?");
							}
						}();

						this->context.emitError(
							Diagnostic::Code::PARSER_DEREFERENCE_OR_UNWRAP_ON_TYPE,
							this->source.getTokenBuffer().getSourceLocation(this->reader.peek(), this->source.getID()),
							"A dereference operator ([.*]) should not follow a type",
							evo::SmallVector<Diagnostic::Info>{std::move(diagnostics_info)}
						);
						return Result::Code::ERROR;

					}else if constexpr(TERM_KIND == TermKind::TEMPLATE_EXPR){
						return Result::Code::WRONG_TYPE;

					}else{
						output = this->source.ast_buffer.createPostfix(output.value(), this->reader.next());
					}

				} break;

				case Token::lookupKind(".?"): {
					if constexpr(TERM_KIND == TermKind::EXPLICIT_TYPE || TERM_KIND == TermKind::AS_TYPE){
						Diagnostic::Info diagnostics_info = [](){
							if constexpr(TERM_KIND == TermKind::AS_TYPE){
								return Diagnostic::Info(
									"Did you mean to put parentheses around the preceding [as] operation?"
								);
							}else{
								return Diagnostic::Info("Did you mean optional ([?]) instead?");
							}
						}();

						this->context.emitError(
							Diagnostic::Code::PARSER_DEREFERENCE_OR_UNWRAP_ON_TYPE,
							this->source.getTokenBuffer().getSourceLocation(this->reader.peek(), this->source.getID()),
							"An unwrap operator ([.?]) should not follow a type",
							evo::SmallVector<Diagnostic::Info>{diagnostics_info}
						);
						return Result::Code::ERROR;

					}else if constexpr(TERM_KIND == TermKind::TEMPLATE_EXPR){
						return Result::Code::WRONG_TYPE;

					}else{
						output = this->source.ast_buffer.createPostfix(output.value(), this->reader.next());
					}

				} break;


				case Token::lookupKind("<{"): {
					if(this->assert_token_fail(Token::lookupKind("<{"))){ return Result::Code::ERROR; }
					
					auto args = evo::SmallVector<AST::Node>();

					bool is_first_expr = true;

					while(true){
						if(this->reader[this->reader.peek()].kind() == Token::lookupKind("}>")){
							if(this->assert_token_fail(Token::lookupKind("}>"))){ return Result::Code::ERROR; }
							break;
						}
						
						Result arg = this->parse_type<TypeKind::TEMPLATE_ARG>();
						if(arg.code() == Result::Code::ERROR){
							return Result::Code::ERROR;

						}else if(arg.code() == Result::Code::WRONG_TYPE){
							arg = this->parse_expr();
							if(this->check_result_fail(arg, "argument inside template pack")){
								return Result::Code::ERROR;
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
								return Result::Code::ERROR;
							}

							is_first_expr = false;
							break;
						}
					}

					output = this->source.ast_buffer.createTemplatedExpr(output.value(), std::move(args));
				} break;


				// case Token::lookupKind("<"): {
				// 	if(TERM_KIND == TermKind::ExplciitType || TERM_KIND == TermKind::AS_TYPE){
				// 		if(this->assert_token_fail(Token::lookupKind("<"))){ return Result::Code::ERROR; }
				
				// 		auto args = evo::SmallVector<AST::Node>();

				// 		while(true){
				// 			if(this->reader[this->reader.peek()].kind() == Token::lookupKind(">")){
				// 				if(this->assert_token_fail(Token::lookupKind(">"))){ return Result::Code::ERROR; }
				// 				break;
				// 			}
							
				// 			Result arg = this->parse_type<TypeKind::TEMPLATE_ARG>();
				// 			if(arg.code() == Result::Code::ERROR){
				// 				return Result::Code::ERROR;

				// 			}else if(arg.code() == Result::Code::WRONG_TYPE){
				// 				arg = this->parse_expr();
				// 				if(this->check_result_fail(arg, "argument inside template pack")){
				// 					return Result::Code::ERROR;
				// 				}
				// 			}

				// 			args.emplace_back(arg.value());

				// 			// check if ending or should continue
				// 			const Token::Kind after_arg_next_token_kind = this->reader[this->reader.next()].kind();
				// 			if(after_arg_next_token_kind != Token::lookupKind(",")){
				// 				if(after_arg_next_token_kind != Token::lookupKind(">")){
				// 					this->expected_but_got(
				// 						"[,] at end of template argument or [>] at end of template argument block",
				// 						this->reader.peek(-1)
				// 					);
				// 					return Result::Code::ERROR;
				// 				}

				// 				break;
				// 			}
				// 		}

				// 		output = this->source.ast_buffer.createTemplatedExpr(output.value(), std::move(args));

				// 	}else if constexpr(TERM_KIND == TermKind::EXPR){
				// 		return output;

				// 	}else if constexpr(TERM_KIND == TermKind::TEMPLATE_EXPR){
				// 		const Token::ID start_location = this->reader.peek();

				// 		if(this->assert_token_fail(Token::lookupKind("<"))){ return Result::Code::ERROR; }


				// 	}else{
				// 		static_assert(false, "Unknown TermKind");
				// 	}
				// } break;


				case Token::lookupKind("("): {
					if constexpr(TERM_KIND == TermKind::EXPLICIT_TYPE || TERM_KIND == TermKind::AS_TYPE){
						should_continue = false;
						break;

					}else{
						evo::Result<evo::SmallVector<AST::FuncCall::Arg>> args = this->parse_func_call_args();
						if(args.isError()){ return Result::Code::ERROR; }

						output = this->source.ast_buffer.createFuncCall(output.value(), std::move(args.value()));
					}
				} break;

				default: {
					should_continue = false;
				} break;
			}
		}

		return output;
	}


	auto Parser::parse_term_stmt() -> Result {
		const Result term = this->parse_term<TermKind::EXPR>();
		if(term.code() != Result::Code::SUCCESS){ return term; }

		if(this->expect_token_fail(Token::lookupKind(";"), "after term statement")){ return Result::Code::ERROR; }

		return term;
	}


	// TODO: check EOF
	auto Parser::parse_encapsulated_expr() -> Result {
		const Result block_expr = this->parse_block(BlockLabelRequirement::REQUIRED);
		if(block_expr.code() != Result::Code::WRONG_TYPE){ return block_expr; }

		if(this->reader[this->reader.peek()].kind() != Token::lookupKind("(")){
			return this->parse_atom();
		}

		const Token::ID open_token_id = this->reader.next();

		const Result inner_expr = this->parse_sub_expr();
		if(inner_expr.code() != Result::Code::SUCCESS){ return inner_expr; }

		if(this->reader[this->reader.peek()].kind() != Token::lookupKind(")")){
			const Source::Location open_location = 
				this->source.getTokenBuffer().getSourceLocation(open_token_id, this->source.getID());

			this->expected_but_got(
				"either closing [)] around expression or continuation of sub-expression",
				this->reader.peek(),
				evo::SmallVector<Diagnostic::Info>{ Diagnostic::Info("parenthesis opened here", open_location), }
			);

			return Result::Code::ERROR;
		}

		if(this->assert_token_fail(Token::lookupKind(")"))){ return Result::Code::ERROR; }

		return inner_expr;
	}

	// TODO: check EOF
	auto Parser::parse_atom() -> Result {
		Result result = this->parse_ident();
		if(result.code() != Result::Code::WRONG_TYPE){ return result; }

		result = this->parse_literal();
		if(result.code() != Result::Code::WRONG_TYPE){ return result; }

		result = this->parse_intrinsic();
		if(result.code() != Result::Code::WRONG_TYPE){ return result; }

		result = this->parse_this();
		if(result.code() != Result::Code::WRONG_TYPE){ return result; }

		return Result::Code::WRONG_TYPE;
	}


	auto Parser::parse_attribute_block() -> Result {
		auto attributes = evo::SmallVector<AST::AttributeBlock::Attribute>();

		while(this->reader[this->reader.peek()].kind() == Token::Kind::ATTRIBUTE){
			const Token::ID attr_token_id = this->reader.next();
			auto arguments = evo::StaticVector<AST::Node, 2>();

			if(this->reader[this->reader.peek()].kind() == Token::lookupKind("(")){
				if(this->assert_token_fail(Token::lookupKind("("))){ return Result::Code::ERROR; }

				const Result argument_result = this->parse_expr();
				if(this->check_result_fail(argument_result, "argument in attribute after [(]")){
					return Result::Code::ERROR;
				}
				arguments.emplace_back(argument_result.value());

				if(this->reader[this->reader.peek()].kind() == Token::lookupKind(",")){
					if(this->assert_token_fail(Token::lookupKind(","))){ return Result::Code::ERROR; }

					const Result argument2_result = this->parse_expr();
					if(this->check_result_fail(argument2_result, "argument in attribute after [,]")){
						return Result::Code::ERROR;
					}
					arguments.emplace_back(argument2_result.value());
				}

				if(this->expect_token_fail(Token::lookupKind(")"), "after attribute argument")){
					return Result::Code::ERROR;
				}

			}

			attributes.emplace_back(attr_token_id, std::move(arguments));
		}

		return this->source.ast_buffer.createAttributeBlock(std::move(attributes));
	}


	auto Parser::parse_ident() -> Result {
		if(this->reader[this->reader.peek()].kind() != Token::Kind::IDENT){
			return Result::Code::WRONG_TYPE;
		}

		return AST::Node(AST::Kind::IDENT, this->reader.next());
	}

	auto Parser::parse_intrinsic() -> Result {
		if(this->reader[this->reader.peek()].kind() != Token::Kind::INTRINSIC){
			return Result::Code::WRONG_TYPE;
		}

		return AST::Node(AST::Kind::INTRINSIC, this->reader.next());
	}

	auto Parser::parse_literal() -> Result {
		switch(this->reader[this->reader.peek()].kind()){
			case Token::Kind::LITERAL_BOOL:
			case Token::Kind::LITERAL_INT:
			case Token::Kind::LITERAL_FLOAT:
			case Token::Kind::LITERAL_STRING:
			case Token::Kind::LITERAL_CHAR:
			case Token::Kind::KEYWORD_NULL:
				break;

			default:
				return Result::Code::WRONG_TYPE;
		}

		return AST::Node(AST::Kind::LITERAL, this->reader.next());
	}


	auto Parser::parse_uninit() -> Result {
		if(this->reader[this->reader.peek()].kind() != Token::Kind::KEYWORD_UNINIT){
			return Result::Code::WRONG_TYPE;
		}

		return AST::Node(AST::Kind::UNINIT, this->reader.next());
	}

	auto Parser::parse_zeroinit() -> Result {
		if(this->reader[this->reader.peek()].kind() != Token::Kind::KEYWORD_ZEROINIT){
			return Result::Code::WRONG_TYPE;
		}

		return AST::Node(AST::Kind::ZEROINIT, this->reader.next());
	}

	auto Parser::parse_this() -> Result {
		if(this->reader[this->reader.peek()].kind() != Token::Kind::KEYWORD_THIS){
			return Result::Code::WRONG_TYPE;
		}

		return AST::Node(AST::Kind::THIS, this->reader.next());
	}




	auto Parser::parse_template_pack() -> Result {
		if(this->reader[this->reader.peek()].kind() != Token::lookupKind("<{")){
			return Result::Code::WRONG_TYPE;
		}

		auto params = evo::SmallVector<AST::TemplatePack::Param>();

		const Token::ID start_location = this->reader.next();

		bool param_has_default_value = false;
		while(true){
			if(this->reader[this->reader.peek()].kind() == Token::lookupKind("}>")){
				if(this->assert_token_fail(Token::lookupKind("}>"))){ return Result::Code::ERROR; }
				break;
			}

			const Result ident = this->parse_ident();
			if(ident.code() == Result::Code::ERROR){
				return Result::Code::ERROR;
			}else if(ident.code() == Result::Code::WRONG_TYPE){
				this->expected_but_got("identifier in template parameter", this->reader.peek());
				return Result::Code::ERROR;
			}

			if(this->expect_token_fail(Token::lookupKind(":"), "after template parameter identifier")){
				return Result::Code::ERROR;
			}
			
			const Result type = this->parse_type<TypeKind::EXPLICIT>();
			if(this->check_result_fail(type, "type in template parameter declaration")){
				return Result::Code::ERROR;
			}


			auto default_value = std::optional<AST::Node>();
			if(this->reader[this->reader.peek()].kind() == Token::lookupKind("=")){
				if(this->assert_token_fail(Token::lookupKind("="))){ return Result::Code::ERROR; }

				Result default_value_res = this->parse_type<TypeKind::TEMPLATE_ARG>();
				if(default_value_res.code() == Result::Code::ERROR){
					return Result::Code::ERROR;

				}else if(default_value_res.code() == Result::Code::WRONG_TYPE){
					default_value_res = this->parse_expr();
					if(this->check_result_fail(default_value_res, "default value of template parameter")){
						return Result::Code::ERROR;
					}

					default_value = default_value_res.value();

				}else{
					default_value = default_value_res.value();
				}

				param_has_default_value = true;
			}else{
				if(param_has_default_value){
					this->context.emitError(
						Diagnostic::Code::PARSER_OOO_DEFAULT_VALUE_PARAM,
						Diagnostic::Location::get(ident.value(), this->source),
						"Out of order default value parameter",
						Diagnostic::Info("Parameters without a default value cannot be after one with a default value")
					);
					return Result::Code::ERROR;
				}
			}


			params.emplace_back(ASTBuffer::getIdent(ident.value()), type.value(), default_value);


			// check if ending or should continue
			const Token::Kind after_return_next_token_kind = this->reader[this->reader.next()].kind();
			if(after_return_next_token_kind != Token::lookupKind(",")){
				if(after_return_next_token_kind != Token::lookupKind("}>")){
					this->expected_but_got(
						"[,] at end of template parameter or [}>] at end of template pack",
						this->reader.peek(-1)
					);
					return Result::Code::ERROR;
				}

				break;
			}
		}


		if(params.empty()){
			this->context.emitError(
				Diagnostic::Code::PARSER_TEMPLATE_PARAMETER_BLOCK_EMPTY,
				Diagnostic::Location::get(start_location, this->source),
				"Template parameter blocks cannot be empty",
				Diagnostic::Info("If you don't want the symbol to be templated, remove the template parameter block")
			);
		}

		return this->source.ast_buffer.createTemplatePack(std::move(params));
	}


	auto Parser::parse_func_params() -> evo::Result<evo::SmallVector<AST::FuncDecl::Param>> {
		auto params = evo::SmallVector<AST::FuncDecl::Param>();
		if(this->expect_token_fail(Token::lookupKind("("), "to open parameter block in function declaration")){
			return evo::resultError;
		}

		bool param_has_default_value = false;
		while(true){
			if(this->reader[this->reader.peek()].kind() == Token::lookupKind(")")){
				if(this->assert_token_fail(Token::lookupKind(")"))){ return evo::resultError; }
				break;
			}

			
			auto param_ident = std::optional<AST::Node>();
			auto param_type = std::optional<AST::Node>();
			using ParamKind = AST::FuncDecl::Param::Kind;
			auto param_kind = std::optional<ParamKind>();

			if(this->reader[this->reader.peek()].kind() == Token::Kind::KEYWORD_THIS){
				const Token::ID this_token_id = this->reader.next();
				param_ident = AST::Node(AST::Kind::THIS, this_token_id);

				switch(this->reader[this->reader.peek()].kind()){
					case Token::Kind::KEYWORD_READ: {
						this->reader.skip();
						param_kind = ParamKind::Read;
					} break;

					case Token::Kind::KEYWORD_MUT: {
						this->reader.skip();
						param_kind = ParamKind::Mut;
					} break;

					case Token::Kind::KEYWORD_IN: {
						this->context.emitError(
							Diagnostic::Code::PARSER_INVALID_KIND_FOR_A_THIS_PARAM,
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
							Diagnostic::Code::PARSER_INVALID_KIND_FOR_A_THIS_PARAM,
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

				const Result type = this->parse_type<TypeKind::EXPLICIT>();
				if(this->check_result_fail(type, "type after [:] in function parameter declaration")){
					return evo::resultError;
				}
				param_type = type.value();

				switch(this->reader[this->reader.peek()].kind()){
					case Token::Kind::KEYWORD_READ: {
						this->reader.skip();
						param_kind = ParamKind::Read;
					} break;

					case Token::Kind::KEYWORD_MUT: {
						this->reader.skip();
						param_kind = ParamKind::Mut;
					} break;

					case Token::Kind::KEYWORD_IN: {
						this->reader.skip();
						param_kind = ParamKind::In;
					} break;

					default: {
						param_kind = ParamKind::Read;
					} break;
				}
			}

			const Result attributes = this->parse_attribute_block();
			if(attributes.code() == Result::Code::ERROR){ return evo::resultError; }

			auto default_value = std::optional<AST::Node>();
			if(this->reader[this->reader.peek()].kind() == Token::lookupKind("=")){
				if(this->assert_token_fail(Token::lookupKind("="))){ return evo::resultError; }

				const Result default_value_res = this->parse_expr();
				if(this->check_result_fail(default_value_res, "function parameter default value")){
					return evo::resultError;
				}

				default_value = default_value_res.value();
				param_has_default_value = true;
			}else{
				if(param_has_default_value){
					this->context.emitError(
						Diagnostic::Code::PARSER_OOO_DEFAULT_VALUE_PARAM,
						Diagnostic::Location::get(*param_ident, this->source),
						"Out of order default value parameter",
						Diagnostic::Info("Parameters without a default value cannot be after one with a default value")
					);
					return evo::resultError;
				}
			}


			params.emplace_back(*param_ident, param_type, *param_kind, attributes.value(), default_value);

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


	auto Parser::parse_func_returns() -> evo::Result<evo::SmallVector<AST::FuncDecl::Return>> {
		auto returns = evo::SmallVector<AST::FuncDecl::Return>();

		if(this->reader[this->reader.peek()].kind() != Token::lookupKind("(")){
			const Result type = this->parse_type<TypeKind::EXPLICIT>();
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
			if(ident.code() == Result::Code::ERROR){
				return evo::resultError;
			}else if(ident.code() == Result::Code::WRONG_TYPE){
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
			
			const Result type = this->parse_type<TypeKind::EXPLICIT>();
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
				Diagnostic::Code::PARSER_EMPTY_FUNC_RETURN_BLOCK,
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


	auto Parser::parse_func_error_returns() -> evo::Result<evo::SmallVector<AST::FuncDecl::Return>> {
		auto error_returns = evo::SmallVector<AST::FuncDecl::Return>();

		if(this->reader[this->reader.peek()].kind() != Token::lookupKind("<")){
			return error_returns;
		}

		if(this->assert_token_fail(Token::lookupKind("<"))){ return evo::resultError; }

		if(this->reader[this->reader.peek()].kind() == Token::lookupKind(">")){
			this->context.emitError(
				Diagnostic::Code::PARSER_EMPTY_ERROR_RETURN_PARAMS,
				Diagnostic::Location::get(this->reader.peek(), this->source),
				"Error returns parameters cannot be empty",
				evo::SmallVector<Diagnostic::Info>{
					Diagnostic::Info(
						"If you want the function to not have error returns, "
							"the function shouldn't have error return parameter block"
					),
					Diagnostic::Info(
						"If you want the function to have error return but no error return value, "
							"the error return parameter block should only contain `Void`"
					),
				}
			);
			return evo::resultError;
		}




		if(this->reader[this->reader.peek(1)].kind() != Token::lookupKind(":")){
			const Result single_type = this->parse_type<TypeKind::EXPLICIT>();
			if(this->check_result_fail(single_type, "single type in function error return parameter block")){
				evo::resultError;
			}
			error_returns.emplace_back(std::nullopt, single_type.value());

			if(this->expect_token_fail(
				Token::lookupKind(">"), "at end of single type in function error return parameter block"
			)){ return evo::resultError; }

			return error_returns;
		}

		while(true){
			if(this->reader[this->reader.peek()].kind() == Token::lookupKind(">")){
				if(this->assert_token_fail(Token::lookupKind(">"))){ return evo::resultError; }
				break;
			}

			const Result ident = this->parse_ident();
			if(this->check_result_fail(ident, "identifier in function error return parameter")){
				return evo::resultError;
			}

			if(this->expect_token_fail(Token::lookupKind(":"), "after function error return parameter identifier")){
				return evo::resultError;
			}
			
			const Result type = this->parse_type<TypeKind::EXPLICIT>();
			if(this->check_result_fail(type, "type in function error return parameter declaration")){
				return evo::resultError;
			}

			error_returns.emplace_back(ASTBuffer::getIdent(ident.value()), type.value());


			// check if ending or should continue
			const Token::Kind after_return_next_token_kind = this->reader[this->reader.next()].kind();
			if(after_return_next_token_kind != Token::lookupKind(",")){
				if(after_return_next_token_kind != Token::lookupKind(">")){
					this->expected_but_got(
						"[,] at end of function error return parameter"
							" or [>] at end of function error returns parameters block",
						this->reader.peek(-1)
					);
					return evo::resultError;
				}

				break;
			}
		}

		return error_returns;
	}



	auto Parser::parse_func_call_args() -> evo::Result<evo::SmallVector<AST::FuncCall::Arg>> {
		if(this->assert_token_fail(Token::lookupKind("("))){ return evo::resultError; }
		
		auto args = evo::SmallVector<AST::FuncCall::Arg>();

		while(true){
			if(this->reader[this->reader.peek()].kind() == Token::lookupKind(")")){
				if(this->assert_token_fail(Token::lookupKind(")"))){ return evo::resultError; }
				break;
			}

			auto arg_ident = std::optional<Token::ID>();

			if(
				this->reader[this->reader.peek()].kind() == Token::Kind::IDENT &&
				this->reader[this->reader.peek(1)].kind() == Token::lookupKind(":")
			){
				arg_ident = this->reader.next();
				if(this->assert_token_fail(Token::lookupKind(":"))){ return evo::resultError; }
			}

			const Result expr_result = this->parse_expr();
			if(this->check_result_fail(expr_result, "expression argument inside function call")){
				return evo::resultError;
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
					return evo::resultError;
				}

				break;
			}
		}

		return args;
	}


	




	//////////////////////////////////////////////////////////////////////
	// checking

	auto Parser::expected_but_got(
		std::string_view location_str, Token::ID token_id, evo::SmallVector<Diagnostic::Info>&& infos
	) -> void {
		this->context.emitError(
			Diagnostic::Code::PARSER_INCORRECT_STMT_CONTINUATION,
			this->source.getTokenBuffer().getSourceLocation(token_id, this->source.getID()),
			std::format("Expected {}, got [{}] instead", location_str, this->reader[token_id].kind()), 
			std::move(infos)
		);
	}


	auto Parser::check_result_fail(const Result& result, std::string_view location_str) -> bool {
		switch(result.code()){
			case Result::Code::SUCCESS: {
				return false;
			} break;

			case Result::Code::WRONG_TYPE: {
				this->expected_but_got(location_str, this->reader.peek());
				return true;
			} break;

			case Result::Code::ERROR: {
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
				Diagnostic::Code::PARSER_ASSUMED_TOKEN_NOT_PRESET,
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