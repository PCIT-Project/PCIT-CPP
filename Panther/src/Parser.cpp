//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "./Parser.h"



namespace pcit::panther{
	

	auto Parser::parse() noexcept -> void {
		bool should_continue = true;

		while(this->reader.at_end() == false && should_continue){
			this->parse_global_stmt_dispatch();

			while(this->stack.empty() == false){
				if(this->context.hasHitFailCondition() || this->last_result.code() == Result::Code::Error){
					should_continue = false;
					break;
				}

				const StackFrame::Call call = this->stack.top().call;
				this->stack.pop();

				call(*this);
			};
		};
	};


	static auto _pop_stack_context_caller(Parser& parser) noexcept -> void {
		parser._pop_stack_context();
	};
	auto Parser::_pop_stack_context() noexcept -> void {
		this->frame_contexts.pop();
	};

	auto Parser::_add_pop_stack_context() noexcept -> void {
		this->add_to_stack(&_pop_stack_context_caller);
	};


	//////////////////////////////////////////////////////////////////////
	// parse stmt

	/*
		Stmt:
			VarDecl
			FuncDecl
	*/

	static auto _parse_stmt_selector_caller(Parser& parser) noexcept -> void { parser._parse_stmt_selector(); };
	auto Parser::_parse_stmt_selector() noexcept -> void {
		const Token& peeked_token = this->reader[this->reader.peek()];

		switch(peeked_token.getKind()){
			case Token::Kind::KeywordVar: this->parse_var_decl_dispatch(); return;
			case Token::Kind::KeywordFunc: this->parse_func_decl_dispatch(); return;
		};

		this->context.emitError(
			Diagnostic::Code::ParserUnknownStmtStart,
			this->reader[this->reader.peek()].getSourceLocation(this->source_id),
			"Unknown start to statement"
		);
	};


	static auto _parse_stmt_add_global_caller(Parser& parser) noexcept -> void { parser._parse_stmt_add_global(); };
	auto Parser::_parse_stmt_add_global() noexcept -> void {
		if(this->last_result.code() == Result::Code::Success){
			this->getASTBuffer().global_stmts.emplace_back(this->last_result.value());
		}
	};


	auto Parser::parse_stmt_dispatch() noexcept -> void {
		this->add_to_stack(&_parse_stmt_selector_caller);
	};

	auto Parser::parse_global_stmt_dispatch() noexcept -> void {
		this->add_to_stack({&_parse_stmt_selector_caller, &_parse_stmt_add_global_caller});
	};


	//////////////////////////////////////////////////////////////////////
	// parse var decl

	/*
		VarDecl:
			'var' Ident ?(':' Type) ?('=' Expr) ';'
	*/

	static auto _parse_var_decl_type_caller(Parser& parser) noexcept -> void { parser._parse_var_decl_type();	};
	auto Parser::_parse_var_decl_type() noexcept -> void {
		StackFrame::VarDecl& var_decl_context = this->get_frame_context<StackFrame::VarDecl>();

		if(this->reader[this->reader.peek()].getKind() != Token::lookupKind(":")){
			return;
		}
		this->reader.skip();

		var_decl_context.has_type = true;
		this->parse_type_dispatch();
	};


	static auto _parse_var_decl_value_caller(Parser& parser) noexcept -> void { parser._parse_var_decl_value();	};
	auto Parser::_parse_var_decl_value() noexcept -> void {
		StackFrame::VarDecl& var_decl_context = this->get_frame_context<StackFrame::VarDecl>();

		if(var_decl_context.has_type){
			if(this->check_result(this->last_result, "type after [:] in variable declaration")){ return; }
			var_decl_context.type = this->last_result.value();
		}

		if(this->reader[this->reader.peek()].getKind() != Token::lookupKind("=")){
			return;
		}
		this->reader.skip();

		var_decl_context.has_expr = true;
		this->parse_expr_dispatch();
	};

	static auto _parse_var_decl_end_caller(Parser& parser) noexcept -> void { parser._parse_var_decl_end();	};
	auto Parser::_parse_var_decl_end() noexcept -> void {
		StackFrame::VarDecl& var_decl_context = this->get_frame_context<StackFrame::VarDecl>();

		auto expr = AST::NodeOptional();

		if(var_decl_context.has_expr){
			if(this->check_result(this->last_result, "expression after [=] in variable declaration")){ return; }
			expr = this->last_result.value();
		}

		// ;
		if(this->expect_token(Token::lookupKind(";"), "at end of variable declaration")){ return; }

		last_result = this->getASTBuffer().createVarDecl(var_decl_context.ident, var_decl_context.type, expr);
	};



	auto Parser::parse_var_decl_dispatch() noexcept -> void {
		if(this->assert_token(Token::Kind::KeywordVar, "in variable declaration")){ return; };

		// ident
		const Result ident = this->parse_ident();
		if(this->check_result(ident, "identifier in variable declaration")){ return; }

		this->add_frame_context<StackFrame::VarDecl>(ident.value());
		this->add_to_stack({&_parse_var_decl_type_caller, &_parse_var_decl_value_caller, &_parse_var_decl_end_caller});
	};


	//////////////////////////////////////////////////////////////////////
	// parse func decl

	/*
		VarDecl:
			'func' Ident '(' ')' `->` Type Block
	*/
	
	static auto _parse_func_decl_type_caller(Parser& parser) noexcept -> void { parser._parse_func_decl_type();	};
	auto Parser::_parse_func_decl_type() noexcept -> void {
		StackFrame::FuncDecl& func_decl_context = this->get_frame_context<StackFrame::FuncDecl>();

		if(this->check_result(this->last_result, "return type in function declaration")){ return; }

		func_decl_context.return_type = this->last_result.value();

		this->parse_block_dispatch();
	};


	static auto _parse_func_decl_end_caller(Parser& parser) noexcept -> void { parser._parse_func_decl_end(); };
	auto Parser::_parse_func_decl_end() noexcept -> void {
		StackFrame::FuncDecl& func_decl_context = this->get_frame_context<StackFrame::FuncDecl>();

		if(this->check_result(this->last_result, "statement block in function declaration")){ return; }
		this->last_result = this->getASTBuffer().createFuncDecl(
			func_decl_context.ident,
			func_decl_context.return_type.getValue(),
			this->last_result.value()
		);
	};



	auto Parser::parse_func_decl_dispatch() noexcept -> void {
		if(this->assert_token(Token::Kind::KeywordFunc, "in function declaration")){ return; };

		// ident
		const Result ident = this->parse_ident();
		if(this->check_result(ident, "identifier in function declaration")){ return; }

		if(this->expect_token(Token::lookupKind("="), "in function declaration")){ return; }
		if(this->expect_token(Token::lookupKind("("), "in function declaration")){ return; }
		if(this->expect_token(Token::lookupKind(")"), "in function declaration")){ return; }
		if(this->expect_token(Token::lookupKind("->"), "in function declaration")){ return; }

		this->add_frame_context<StackFrame::FuncDecl>(ident.value());
		this->add_to_stack({&_parse_func_decl_type_caller, &_parse_func_decl_end_caller});
		this->parse_type_dispatch();
	};

	//////////////////////////////////////////////////////////////////////
	// parse block

	/*
		Block:
			'{' (Stmt)* '}'
	*/


	static auto _parse_block_stmt_caller(Parser& parser) noexcept -> void { parser._parse_block_stmt(); };
	auto Parser::_parse_block_stmt() noexcept -> void {
		StackFrame::Block& block_context = this->get_frame_context<StackFrame::Block>();

		if(this->check_result(this->last_result, "statement in statement block")){ return; }
		block_context.stmts.emplace_back(this->last_result.value());

		if(this->reader[this->reader.peek()].getKind() == Token::lookupKind("}")){
			this->reader.skip();
			last_result = this->getASTBuffer().createBlock(std::move(block_context.stmts));
			return;
		}

		this->add_to_stack(&_parse_block_stmt_caller);
		this->parse_stmt_dispatch();
	};


	auto Parser::parse_block_dispatch() noexcept -> void {
		if(this->expect_token(Token::lookupKind("{"), "at beginning of statement block")){ return; }

		if(this->reader[this->reader.peek()].getKind() == Token::lookupKind("}")){
			this->reader.skip();
			last_result = this->getASTBuffer().createBlock();
			return;
		}

		this->add_frame_context<StackFrame::Block>();

		this->add_to_stack(&_parse_block_stmt_caller);
		this->parse_stmt_dispatch();
	};



	//////////////////////////////////////////////////////////////////////
	// parse type


	auto Parser::parse_type_dispatch() noexcept -> void {
		switch(this->reader[this->reader.peek()].getKind()){
			case Token::Kind::TypeVoid:
			case Token::Kind::TypeInt:
				break;

			default:
				this->last_result = Result::Code::WrongType;
				return;
		};

		const auto base = AST::Node(AST::Kind::BuiltinType, this->reader.next());

		this->last_result = this->getASTBuffer().createType(base);
	};


	//////////////////////////////////////////////////////////////////////
	// parse expr

	auto Parser::parse_expr_dispatch() noexcept -> void {
		this->last_result = this->parse_atom();
	};


	//////////////////////////////////////////////////////////////////////
	// other parsing


	auto Parser::parse_atom() noexcept -> Result {
		Result result = this->parse_literal();
		if(result.code() == Result::Code::Success || result.code() == Result::Code::Error){ return result; }

		result = this->parse_ident();
		if(result.code() == Result::Code::Success || result.code() == Result::Code::Error){ return result; }


		return Result::Code::WrongType;
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


	auto Parser::parse_ident() noexcept -> Result {
		const Token::ID next_token = this->reader.peek();

		if(this->reader[next_token].getKind() != Token::Kind::Ident){
			return Result::Code::WrongType;
		}

		this->reader.skip();

		return AST::Node(AST::Kind::Ident, next_token);
	};



	//////////////////////////////////////////////////////////////////////
	// stack

	auto Parser::add_to_stack(evo::ArrayProxy<StackFrame::Call> calls) noexcept -> void {
		for(auto i = calls.rbegin(); i != calls.rend(); ++i){
			this->stack.emplace(*i);
		}
	};


	//////////////////////////////////////////////////////////////////////
	// checking and messaging


	auto Parser::expected_but_got(std::string_view expected, const Token& token) noexcept -> bool {
		this->context.emitError(
			Diagnostic::Code::ParserIncorrectStmtContinuation, token.getSourceLocation(this->source_id),
			std::format("Expected {}, got [{}] instead", expected, token.getKind())
		);

		this->last_result = Result::Code::Error;

		return this->context.hasHitFailCondition();
	};


	auto Parser::check_result(const Result& result, std::string_view expected) noexcept -> bool {
		if(result.code() == Result::Code::Error){
			this->last_result = Result::Code::Error;
			return this->context.hasHitFailCondition();
		}

		if(result.code() == Result::Code::WrongType){ 
			this->last_result = Result::Code::WrongType;			
			return this->expected_but_got(expected, this->reader[this->reader.peek()]);
		}

		return false;
	};


	auto Parser::expect_token(Token::Kind kind, std::string_view location_str) noexcept -> bool {
		const Token& next_token = this->reader[this->reader.next()];

		if(next_token.getKind() == kind){ return false; }

		this->context.emitError(
			Diagnostic::Code::ParserIncorrectStmtContinuation, next_token.getSourceLocation(this->source_id),
			std::format("Expected [{}] {}, got [{}] instead", kind, location_str, next_token.getKind())
		);

		this->last_result = Result::Code::Error;

		return this->context.hasHitFailCondition();
	};

	auto Parser::assert_token(Token::Kind kind, std::string_view location_str) noexcept -> bool {
		const Token& next_token = this->reader[this->reader.next()];

		if(next_token.getKind() == kind){ return false; }

		this->context.emitFatal(
			Diagnostic::Code::ParserAssumedTokenNotPreset, next_token.getSourceLocation(this->source_id),
			std::format("Expected [{}] {}, got [{}] instead", kind, location_str, next_token.getKind())
		);

		this->last_result = Result::Code::Error;

		return true;
	};


	//////////////////////////////////////////////////////////////////////
	// misc helpers

	auto Parser::getASTBuffer() noexcept -> ASTBuffer& {
		return this->context.getSourceManager().getSource(this->source_id).ast_buffer;
	};


};