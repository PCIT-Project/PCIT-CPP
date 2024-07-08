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

		return Result::Code::WrongType;
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

		// TODO: function parameters
		if(this->expect_token_fail(Token::lookupKind("("), "in FuncDecl")){ return Result::Code::Error; }
		if(this->expect_token_fail(Token::lookupKind(")"), "in FuncDecl")){ return Result::Code::Error; }
		if(this->expect_token_fail(Token::lookupKind("->"), "in FuncDecl")){ return Result::Code::Error; }

		const Result return_type = this->parse_type();
		if(this->check_result_fail(return_type, "return type in function declaration")){
			return Result::Code::Error;
		}

		const Result block = this->parse_block();
		if(this->check_result_fail(block, "statement block in function declaration")){ return Result::Code::Error; }

		return this->source.ast_buffer.createFuncDecl(ident.value(), return_type.value(), block.value());
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



	auto Parser::parse_type() noexcept -> Result {
		switch(this->reader[this->reader.peek()].getKind()){
			case Token::Kind::TypeVoid:
			case Token::Kind::TypeInt:
			case Token::Kind::TypeBool:
				break;

			default: return Result::Code::WrongType;
		};

		const AST::Node base_type = AST::Node(AST::Kind::BuiltinType, this->reader.next());

		return this->source.ast_buffer.createType(base_type);
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
			case Token::Kind::KeywordAddr:
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
	auto Parser::parse_term() noexcept -> Result {
		const Result type = this->parse_type();
		if(type.code() != Result::Code::WrongType){ return type; }

		return this->parse_paren_expr();
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

		return Result::Code::WrongType;
	};


	auto Parser::parse_ident() noexcept -> Result {
		if(this->reader[this->reader.peek()].getKind() != Token::Kind::Ident){
			return Result::Code::WrongType;
		}

		return AST::Node(AST::Kind::Ident, this->reader.next());
	};

	auto Parser::parse_literal() noexcept -> Result {
		switch(this->reader[this->reader.peek()].getKind()){
			case Token::Kind::LiteralBool:
			case Token::Kind::LiteralInt:
			case Token::Kind::LiteralFloat:
			case Token::Kind::LiteralString:
			case Token::Kind::LiteralChar:
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