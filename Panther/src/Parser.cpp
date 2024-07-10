//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "./Parser.h"



namespace pcit::panther{
	

	auto Parser::parse() noexcept -> bool {
		while(this->reader.at_end() == false){
			const Result stmt_result = this->parse_stmt();

			switch(stmt_result.code()){
				case Result::Code::Success: {
					this->source.ast_buffer.global_stmts.emplace_back(stmt_result.value());
				} break;

				case Result::Code::WrongType: {
					this->context.emitError(
						Diagnostic::Code::ParserUnknownStmtStart,
						this->reader[this->reader.peek()].getSourceLocation(this->source.getID()),
						"Unknown start to statement"
					);
					return false;
				} break;

				case Result::Code::Error: {
					return false;
				} break;
			};
		};

		return true;
	};


	auto Parser::parse_stmt() noexcept -> Result {
		const Token& peeked_token = this->reader[this->reader.peek()];
		
		switch(peeked_token.getKind()){
			case Token::Kind::KeywordVar: return this->parse_var_decl();
			case Token::Kind::KeywordFunc: return this->parse_func_decl();
		};

		Result result = this->parse_assignment();
		if(result.code() != Result::Code::WrongType){ return result; }

		return this->parse_term_stmt();
	};


	// TODO: check EOF
	auto Parser::parse_var_decl() noexcept -> Result {
		if(this->assert_token_fail(Token::Kind::KeywordVar)){ return Result::Code::Error; }

		const Result ident = this->parse_ident();
		if(this->check_result_fail(ident, "identifier in variable declaration")){ return Result::Code::Error; }


		auto type = AST::NodeOptional();
		if(this->reader[this->reader.peek()].getKind() == Token::lookupKind(":")){
			if(this->assert_token_fail(Token::lookupKind(":"))){ return Result::Code::Error; }

			const Result type_result = this->parse_type();
			if(this->check_result_fail(type_result, "type after [:] in variable declaration")){
				return Result::Code::Error;
			}

			type = type_result.value();
		}


		auto value = AST::NodeOptional();
		if(this->reader[this->reader.peek()].getKind() == Token::lookupKind("=")){
			if(this->assert_token_fail(Token::lookupKind("="))){ return Result::Code::Error; }
				
			const Result value_result = this->parse_expr();
			if(this->check_result_fail(value_result, "value after [:] in variable declaration")){
				return Result::Code::Error;
			}

			value = value_result.value();
		}


		if(this->expect_token_fail(Token::lookupKind(";"), "after variable declaration")){ return Result::Code::Error; }


		return this->source.ast_buffer.createVarDecl(ident.value(), type, value);
	};


	// TODO: check EOF
	auto Parser::parse_func_decl() noexcept -> Result {
		if(this->assert_token_fail(Token::Kind::KeywordFunc)){ return Result::Code::Error; }

		const Result ident = this->parse_ident();
		if(this->check_result_fail(ident, "identifier after [func] in function declaration")){
			return Result::Code::Error;
		}

		if(this->expect_token_fail(Token::lookupKind("="), "after identifier in function declaration")){
			return Result::Code::Error;
		}


		///////////////////////////////////
		// function parameters

		auto params = evo::SmallVector<AST::FuncDecl::Param>();
		if(this->expect_token_fail(Token::lookupKind("("), "to open parameter block in FuncDecl")){
			return Result::Code::Error;
		}

		while(true){
			if(this->reader[this->reader.peek()].getKind() == Token::lookupKind(")")){
				if(this->assert_token_fail(Token::lookupKind(")"))){ return Result::Code::Error; }
				break;
			}

			
			auto param_ident = AST::NodeOptional();
			auto param_type = AST::NodeOptional();
			using ParamKind = AST::FuncDecl::Param::Kind;
			auto param_kind = std::optional<ParamKind>();

			if(this->reader[this->reader.peek()].getKind() == Token::Kind::KeywordThis){
				const Token::ID this_token_id = this->reader.next();
				param_ident = AST::Node(AST::Kind::This, this_token_id);

				switch(this->reader[this->reader.peek()].getKind()){
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
							this->reader[this->reader.peek(-1)].getSourceLocation(this->source.getID()),
							"[this] parameters cannot have the kind [in]",
							evo::SmallVector<Diagnostic::Info>{
								Diagnostic::Info("Note: valid kinds are [read] and [mut]"),
							}
						);
						return Result::Code::Error;
					} break;

					default: {
						this->context.emitError(
							Diagnostic::Code::ParserInvalidKindForAThisParam,
							this->reader[this->reader.peek(-1)].getSourceLocation(this->source.getID()),
							"[this] parameters cannot have a default kind",
							evo::SmallVector<Diagnostic::Info>{
								Diagnostic::Info("Note: valid kinds are [read] and [mut]"),
							}
						);
						return Result::Code::Error;
					} break;
				};

			}else{
				const Result param_ident_result = this->parse_ident();
				if(this->check_result_fail(param_ident_result, "identifier or [this] in function parameter")){
					return Result::Code::Error;
				}
				param_ident = param_ident_result.value();

				if(this->expect_token_fail(Token::lookupKind(":"), "after function parameter identifier")){
					return Result::Code::Error;
				}

				const Result type = this->parse_type();
				if(this->check_result_fail(type, "type after [:] in function parameter declaration")){
					return Result::Code::Error;
				}
				param_type = type.value();

				switch(this->reader[this->reader.peek()].getKind()){
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
				};
			}


			params.emplace_back(param_ident.value(), param_type, param_kind.value());

			// check if ending or should continue
			const Token::Kind after_param_next_token_kind = this->reader[this->reader.next()].getKind();
			if(after_param_next_token_kind != Token::lookupKind(",")){
				if(after_param_next_token_kind != Token::lookupKind(")")){
					this->expected_but_got(
						"[,] at end of function parameter or [)] at end of function parameters block",
						this->reader[this->reader.peek(-1)]
					);
					return Result::Code::Error;
				}

				break;
			}
		};

		if(this->expect_token_fail(Token::lookupKind("->"), "in FuncDecl")){ return Result::Code::Error; }

		const Result return_type = this->parse_type();
		if(this->check_result_fail(return_type, "return type in function declaration")){
			return Result::Code::Error;
		}

		const Result block = this->parse_block();
		if(this->check_result_fail(block, "statement block in function declaration")){ return Result::Code::Error; }

		return this->source.ast_buffer.createFuncDecl(
			ident.value(), std::move(params), return_type.value(), block.value()
		);
	};


	// TODO: check EOF
	auto Parser::parse_assignment() noexcept -> Result {
		const Token::ID start_location = this->reader.peek();

		const Result lhs = this->parse_term();
		if(lhs.code() != Result::Code::Success){ return lhs; }

		const Token::ID op_token_id = this->reader.next();
		switch(this->reader[op_token_id].getKind()){
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
		};

		const Result rhs = this->parse_expr();
		if(this->check_result_fail(rhs, "expression value in assignment")){ return Result::Code::Error; }

		if(this->expect_token_fail(Token::lookupKind(";"), "at end of assignment")){ return Result::Code::Error; }

		return this->source.ast_buffer.createInfix(lhs.value(), op_token_id, rhs.value());
	};




	// TODO: check EOF
	auto Parser::parse_block() noexcept -> Result {
		if(this->reader[this->reader.peek()].getKind() != Token::lookupKind("{")){ return Result::Code::WrongType; }
		if(this->assert_token_fail(Token::lookupKind("{"))){ return Result::Code::Error; }

		auto statements = evo::SmallVector<AST::Node>();

		while(true){
			if(this->reader[this->reader.peek()].getKind() == Token::lookupKind("}")){
				if(this->assert_token_fail(Token::lookupKind("}"))){ return Result::Code::Error; }
				break;
			}

			const Result stmt = this->parse_stmt();
			if(this->check_result_fail(stmt, "statement in statement block")){ return Result::Code::Error; }

			statements.emplace_back(stmt.value());
		};

		return this->source.ast_buffer.createBlock(std::move(statements));
	};


	// TODO: check EOF
	auto Parser::parse_type(bool is_expr) noexcept -> Result {
		const Token::ID start_location = this->reader.peek();
		bool is_builtin = true;
		switch(this->reader[start_location].getKind()){
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
		};

		const Result base_type = [&]() noexcept {
			if(is_builtin){
				const Token::ID base_type_token_id = this->reader.next();

				const Token& peeked_token = this->reader[this->reader.peek()];
				if(peeked_token.getKind() == Token::lookupKind(".*")){
					this->context.emitError(
						Diagnostic::Code::ParserDereferenceOrUnwrapOnType,
						peeked_token.getSourceLocation(this->source.getID()),
						"A dereference operator ([.*]) should not follow a type",
						evo::SmallVector<Diagnostic::Info>{
							Diagnostic::Info("Did you mean pointer ([*]) instead?"),
						}
					);
					return Result(Result::Code::Error);

				}else if(peeked_token.getKind() == Token::lookupKind(".?")){
					this->context.emitError(
						Diagnostic::Code::ParserDereferenceOrUnwrapOnType,
						peeked_token.getSourceLocation(this->source.getID()),
						"A unwrap operator ([.?]) should not follow a type",
						evo::SmallVector<Diagnostic::Info>{
							Diagnostic::Info("Did you mean optional ([?]) instead?"),
						}
					);
					return Result(Result::Code::Error);
				}

				return Result(AST::Node(AST::Kind::BuiltinType, base_type_token_id));

			}else{
				if(is_expr){
					return this->parse_term(IsTypeTerm::Maybe);
				}else{
					return this->parse_term(IsTypeTerm::Yes);
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

			if(this->reader[this->reader.peek()].getKind() == Token::lookupKind("*")){
				continue_looking_for_qualifiers = true;
				is_ptr = true;
				potential_backup_location = this->reader.peek();
				if(this->assert_token_fail(Token::lookupKind("*"))){ return Result::Code::Error; }
				
				if(this->reader[this->reader.peek()].getKind() == Token::lookupKind("|")){
					is_read_only = true;
					potential_backup_location = this->reader.peek();
					if(this->assert_token_fail(Token::lookupKind("|"))){ return Result::Code::Error; }
				}

			}else if(this->reader[this->reader.peek()].getKind() == Token::lookupKind("*|")){
				continue_looking_for_qualifiers = true;
				is_ptr = true;
				is_read_only = true;
				potential_backup_location = this->reader.peek();
				if(this->assert_token_fail(Token::lookupKind("*|"))){ return Result::Code::Error; }
			}

			if(this->reader[this->reader.peek()].getKind() == Token::lookupKind("?")){
				continue_looking_for_qualifiers = true;
				is_optional = true;
				if(this->assert_token_fail(Token::lookupKind("?"))){ return Result::Code::Error; }
			}

			if(continue_looking_for_qualifiers){
				qualifiers.emplace_back(is_ptr, is_read_only, is_optional);
			}
		};


		if(is_expr){
			// make sure exprs like `a as Int * b` gets parsed like `(a as Int) * b`
			if(qualifiers.empty() == false && qualifiers.back().isOptional == false){
				switch(this->reader[this->reader.peek()].getKind()){
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
				};
			}

			// just an ident
			if(is_builtin == false && qualifiers.empty()){
				this->reader.go_back(start_location);
				return Result::Code::WrongType;
			}

		}

		return this->source.ast_buffer.createType(base_type.value(), std::move(qualifiers));
	};



	// TODO: check EOF
	auto Parser::parse_expr() noexcept -> Result {
		const Result uninit_result = this->parse_uninit();
		if(uninit_result.code() != Result::Code::WrongType){ return uninit_result; }

		return this->parse_sub_expr();
	};

	// TODO: check EOF
	auto Parser::parse_sub_expr() noexcept -> Result {
		return this->parse_infix_expr();
	};



	EVO_NODISCARD static constexpr auto get_infix_op_precedence(Token::Kind kind) noexcept -> int {
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
		};

		return -1;
	};

	// TODO: check EOF
	auto Parser::parse_infix_expr() noexcept -> Result {
		const Result lhs_result = this->parse_prefix_expr();
		if(lhs_result.code() != Result::Code::Success){ return lhs_result; }

		return this->parse_infix_expr_impl(lhs_result.value(), 0);
	};


	auto Parser::parse_infix_expr_impl(AST::Node lhs, int prec_level) noexcept -> Result {
		const Token::ID peeked_op_token_id = this->reader.peek();
		const Token::Kind peeked_op_kind = this->reader[peeked_op_token_id].getKind();

		const int next_op_prec = get_infix_op_precedence(peeked_op_kind);

		// if not an infix operator or is same or lower precedence
		// 		next_op_prec == -1 if its not an infix op
		//   	< to maintain operator precedence
		// 		<= to prevent `a + b + c` from being parsed as `a + (b + c)`
		if(next_op_prec <= prec_level){ return lhs; }

		if(this->assert_token_fail(peeked_op_kind)){ return Result::Code::Error; }

		const Result rhs_result = [&]() noexcept {
			
			const Result next_part_of_expr = this->parse_prefix_expr();
			if(next_part_of_expr.code() != Result::Code::Success){ return next_part_of_expr; }

			return this->parse_infix_expr_impl(next_part_of_expr.value(), next_op_prec);
		}();

		if(rhs_result.code() != Result::Code::Success){ return rhs_result; }

		const AST::Node created_infix_expr = 
			this->source.ast_buffer.createInfix(lhs, peeked_op_token_id, rhs_result.value());

		return this->parse_infix_expr_impl(created_infix_expr, prec_level);
	};



	// TODO: check EOF
	auto Parser::parse_prefix_expr() noexcept -> Result {
		const Token::ID op_token_id = this->reader.peek();
		const Token::Kind op_token_kind = this->reader[op_token_id].getKind();

		switch(op_token_kind){
			case Token::lookupKind("&"):
			case Token::Kind::KeywordCopy:
			case Token::Kind::KeywordMove:
			case Token::lookupKind("-"):
			case Token::lookupKind("!"):
			case Token::lookupKind("~"):
				break;

			default:
				return this->parse_term();
		};

		if(this->assert_token_fail(op_token_kind)){ return Result::Code::Error; }

		const Result rhs = this->parse_term();
		if(this->check_result_fail(
			rhs, std::format("valid sub-expression on right-hand side of prefix [{}] operator", op_token_kind)
		)){
			return Result::Code::Error;
		}

		return this->source.ast_buffer.createPrefix(op_token_id, rhs.value());
	};

	// TODO: check EOF
	auto Parser::parse_term(IsTypeTerm is_type_term) noexcept -> Result {
		if(is_type_term == IsTypeTerm::No){
			const Result type = this->parse_type(true);
			if(type.code() != Result::Code::WrongType){ return type; }
		}

		Result output = this->parse_paren_expr();
		if(output.code() != Result::Code::Success){ return output; }

		bool should_continue = true;
		while(should_continue){
			switch(this->reader[this->reader.peek()].getKind()){
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
					if(is_type_term == IsTypeTerm::Yes){
						this->context.emitError(
							Diagnostic::Code::ParserDereferenceOrUnwrapOnType,
							this->reader[this->reader.peek()].getSourceLocation(this->source.getID()),
							"A dereference operator ([.*]) should not follow a type",
							evo::SmallVector<Diagnostic::Info>{
								Diagnostic::Info("Did you mean pointer ([*]) instead?"),
							}
						);
						return Result::Code::Error;

					}else if(is_type_term == IsTypeTerm::Maybe){
						return Result::Code::WrongType;
					}

					output = this->source.ast_buffer.createPostfix(output.value(), this->reader.next());
				} break;

				case Token::lookupKind(".?"): {
					if(is_type_term == IsTypeTerm::Yes){
						this->context.emitError(
							Diagnostic::Code::ParserDereferenceOrUnwrapOnType,
							this->reader[this->reader.peek()].getSourceLocation(this->source.getID()),
							"An unwrap operator ([.?]) should not follow a type",
							evo::SmallVector<Diagnostic::Info>{
								Diagnostic::Info("Did you mean optional ([?]) instead?"),
							}
						);
						return Result::Code::Error;

					}else if(is_type_term == IsTypeTerm::Maybe){
						return Result::Code::WrongType;
					}

					output = this->source.ast_buffer.createPostfix(output.value(), this->reader.next());
				} break;

				case Token::lookupKind("("): {
					if(this->assert_token_fail(Token::lookupKind("("))){ return Result::Code::Error; }
					
					auto args = evo::SmallVector<AST::FuncCall::Arg>();

					while(true){
						if(this->reader[this->reader.peek()].getKind() == Token::lookupKind(")")){
							if(this->assert_token_fail(Token::lookupKind(")"))){ return Result::Code::Error; }
							break;
						}

						auto arg_ident = AST::NodeOptional();

						if(
							this->reader[this->reader.peek()].getKind() == Token::Kind::Ident &&
							this->reader[this->reader.peek(1)].getKind() == Token::lookupKind(":")
						){
							arg_ident = AST::Node(AST::Kind::Ident, this->reader.next());
							if(this->assert_token_fail(Token::lookupKind(":"))){ return Result::Code::Error; }
						}

						const Result expr_result = this->parse_expr();
						if(this->check_result_fail(expr_result, "expression argument inside function call")){
							return Result::Code::Error;
						}

						args.emplace_back(arg_ident, expr_result.value());

						// check if ending or should continue
						const Token::Kind after_arg_next_token_kind = this->reader[this->reader.next()].getKind();
						if(after_arg_next_token_kind != Token::lookupKind(",")){
							if(after_arg_next_token_kind != Token::lookupKind(")")){
								this->expected_but_got(
									"[,] at end of function call argument"
									" or [)] at end of function call argument block",
									this->reader[this->reader.peek(-1)]
								);
								return Result::Code::Error;
							}

							break;
						}
					};

					output = this->source.ast_buffer.createFuncCall(output.value(), std::move(args));
				} break;

				default: {
					should_continue = false;
				} break;
			};
		};

		return output;
	};


	auto Parser::parse_term_stmt() noexcept -> Result {
		const Result term = this->parse_term();
		if(term.code() != Result::Code::Success){ return term; }

		if(this->expect_token_fail(Token::lookupKind(";"), "after term statement")){ return Result::Code::Error; }

		return term;
	};


	// TODO: check EOF
	auto Parser::parse_paren_expr() noexcept -> Result {
		if(this->reader[this->reader.peek()].getKind() != Token::lookupKind("(")){
			return this->parse_atom();
		}

		const Token::ID open_token_id = this->reader.next();

		const Result inner_expr = this->parse_sub_expr();
		if(inner_expr.code() != Result::Code::Success){ return inner_expr; }

		if(this->reader[this->reader.peek()].getKind() != Token::lookupKind(")")){
			const Token& open_token = this->reader[open_token_id];
			const Source::Location open_location = open_token.getSourceLocation(this->source.getID());

			this->expected_but_got(
				"either closing [)] around expression or continuation of sub-expression",
				this->reader[this->reader.peek()],
				evo::SmallVector<Diagnostic::Info>{ Diagnostic::Info("parenthesis opened here", open_location), }
			);

			return Result::Code::Error;
		}

		if(this->assert_token_fail(Token::lookupKind(")"))){ return Result::Code::Error; }

		return inner_expr;
	};

	// TODO: check EOF
	auto Parser::parse_atom() noexcept -> Result {
		Result result = this->parse_ident();
		if(result.code() != Result::Code::WrongType){ return result; }

		result = this->parse_literal();
		if(result.code() != Result::Code::WrongType){ return result; }

		result = this->parse_intrinsic();
		if(result.code() != Result::Code::WrongType){ return result; }

		result = this->parse_this();
		if(result.code() != Result::Code::WrongType){ return result; }

		return Result::Code::WrongType;
	};


	auto Parser::parse_ident() noexcept -> Result {
		if(this->reader[this->reader.peek()].getKind() != Token::Kind::Ident){
			return Result::Code::WrongType;
		}

		return AST::Node(AST::Kind::Ident, this->reader.next());
	};

	auto Parser::parse_intrinsic() noexcept -> Result {
		if(this->reader[this->reader.peek()].getKind() != Token::Kind::Intrinsic){
			return Result::Code::WrongType;
		}

		return AST::Node(AST::Kind::Intrinsic, this->reader.next());
	};

	auto Parser::parse_literal() noexcept -> Result {
		switch(this->reader[this->reader.peek()].getKind()){
			case Token::Kind::LiteralBool:
			case Token::Kind::LiteralInt:
			case Token::Kind::LiteralFloat:
			case Token::Kind::LiteralString:
			case Token::Kind::LiteralChar:
			case Token::Kind::KeywordNull:
				break;

			default:
				return Result::Code::WrongType;
		};

		return AST::Node(AST::Kind::Literal, this->reader.next());
	};


	auto Parser::parse_uninit() noexcept -> Result {
		if(this->reader[this->reader.peek()].getKind() != Token::Kind::KeywordUninit){
			return Result::Code::WrongType;
		}

		return AST::Node(AST::Kind::Uninit, this->reader.next());
	};

	auto Parser::parse_this() noexcept -> Result {
		if(this->reader[this->reader.peek()].getKind() != Token::Kind::KeywordThis){
			return Result::Code::WrongType;
		}

		return AST::Node(AST::Kind::This, this->reader.next());
	};


	//////////////////////////////////////////////////////////////////////
	// checking

	auto Parser::expected_but_got(
		std::string_view location_str, const Token& token, evo::SmallVector<Diagnostic::Info>&& infos
	) noexcept -> void {
		this->context.emitError(
			Diagnostic::Code::ParserIncorrectStmtContinuation, token.getSourceLocation(this->source.getID()),
			std::format("Expected {}, got [{}] instead", location_str, token.getKind()), 
			std::move(infos)
		);
	};


	auto Parser::check_result_fail(const Result& result, std::string_view location_str) noexcept -> bool {
		switch(result.code()){
			case Result::Code::Success: {
				return false;
			} break;

			case Result::Code::WrongType: {
				this->expected_but_got(location_str, this->reader[this->reader.peek()]);
				return true;
			} break;

			case Result::Code::Error: {
				return true;
			} break;
		};

		evo::debugFatalBreak("Unknown or unsupported result code ({})", evo::to_underlying(result.code()));
	};


	auto Parser::assert_token_fail(Token::Kind kind) noexcept -> bool {
		#if defined(PCIT_CONFIG_DEBUG)
			const Token& next_token = this->reader[this->reader.next()];

			if(next_token.getKind() == kind){ return false; }

			this->context.emitFatal(
				Diagnostic::Code::ParserAssumedTokenNotPreset, next_token.getSourceLocation(this->source.getID()),
				std::format("Expected [{}], got [{}] instead", kind, next_token.getKind())
			);

			return true;

		#else
			this->reader.skip();
			return false;
		#endif
	};

	auto Parser::expect_token_fail(Token::Kind kind, std::string_view location_str) noexcept -> bool {
		const Token& next_token = this->reader[this->reader.next()];

		if(next_token.getKind() == kind){ return false; }

		this->expected_but_got(std::format("[{}] {}", kind, location_str), next_token);
		return true;
	};


};