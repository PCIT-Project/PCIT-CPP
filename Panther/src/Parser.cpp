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
		while(this->reader.at_end() == false &&  this->context.hasHitFailCondition() == false){
			this->parse_global_stmt_dispatch();

			while(this->stack.empty() == false && this->context.hasHitFailCondition() == false){
				const StackFrame::Call call = this->stack.top().call;
				this->stack.pop();

				if(call == nullptr){
					this->frame_contexts.pop();
				}else{
					call(*this);
				}
			};
		};
	};



	//////////////////////////////////////////////////////////////////////
	// parse stmt

	/*
		Stmt:
			VarDecl
	*/

	static auto _parse_stmt_selector_caller(Parser& parser) noexcept -> void {
		parser._parse_stmt_selector();
	};
	auto Parser::_parse_stmt_selector() noexcept -> void {
		const Token& peeked_token = this->reader[this->reader.peek()];

		switch(peeked_token.getKind()){
			case Token::KeywordVar: this->parse_var_decl_dispatch(); return;
		};
	};


	static auto _parse_stmt_add_global_caller(Parser& parser) noexcept -> void {
		parser._parse_stmt_add_global();
	};
	auto Parser::_parse_stmt_add_global() noexcept -> void {
		if(this->last_result.code() == Result::Success){
			this->getASTBuffer().global_stmts.emplace_back(this->last_result.value());
		}
	};


	auto Parser::parse_stmt_dispatch() noexcept -> void {
		this->stack.emplace(&_parse_stmt_selector_caller);
	};

	auto Parser::parse_global_stmt_dispatch() noexcept -> void {
		this->stack.emplace(&_parse_stmt_add_global_caller);
		this->stack.emplace(&_parse_stmt_selector_caller);
	};


	//////////////////////////////////////////////////////////////////////
	// parse var decl

	/*
		VarDecl:
			'var' Ident ?(':' Type) ?('=' Expr) ';'
	*/

	static auto _parse_var_decl_type_caller(Parser& parser) noexcept -> void {
		parser._parse_var_decl_type();
	};
	auto Parser::_parse_var_decl_type() noexcept -> void {
		StackFrame::VarDecl& var_decl_context = this->frame_contexts.top().var_decl;

		if(this->reader[this->reader.peek()].getKind() != Token::lookupKind(":")){
			return;
		}
		this->reader.skip();

		var_decl_context.has_type = true;
		this->parse_type_dispatch();
	};


	static auto _parse_var_decl_value_caller(Parser& parser) noexcept -> void {
		parser._parse_var_decl_value();
	};
	auto Parser::_parse_var_decl_value() noexcept -> void {
		StackFrame::VarDecl& var_decl_context = this->frame_contexts.top().var_decl;

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

	static auto _parse_var_decl_end_caller(Parser& parser) noexcept -> void {
		parser._parse_var_decl_end();
	};
	auto Parser::_parse_var_decl_end() noexcept -> void {
		StackFrame::VarDecl& var_decl_context = this->frame_contexts.top().var_decl;

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
		if(this->assert_token(Token::KeywordVar, "in variable declaration")){ return; };

		// ident
		const Result ident = this->parse_ident();
		if(this->check_result(ident, "identifier in variable declaration")){ return; }


		this->frame_contexts.emplace(StackFrame::VarDecl(ident.value()));
		this->stack.emplace(nullptr);
		this->stack.emplace(&_parse_var_decl_end_caller);
		this->stack.emplace(&_parse_var_decl_value_caller);
		this->stack.emplace(&_parse_var_decl_type_caller);
	};


	//////////////////////////////////////////////////////////////////////
	// parse type


	auto Parser::parse_type_dispatch() noexcept -> void {
		switch(this->reader[this->reader.peek()].getKind()){
			case Token::TypeVoid:
			case Token::TypeInt:
				break;

			default:
				this->last_result = Result::WrongType;
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
		if(result.code() == Result::Success || result.code() == Result::Error){ return result; }

		result = this->parse_ident();
		if(result.code() == Result::Success || result.code() == Result::Error){ return result; }


		return Result::WrongType;
	};


	auto Parser::parse_literal() noexcept -> Result {
		switch(this->reader[this->reader.peek()].getKind()){
			case Token::LiteralBool:
			case Token::LiteralInt:
			case Token::LiteralFloat:
			case Token::LiteralString:
			case Token::LiteralChar:
				break;

			default:
				return Result::WrongType;
		};

		return AST::Node(AST::Kind::Literal, this->reader.next());
	};


	auto Parser::parse_ident() noexcept -> Result {
		const Token::ID next_token = this->reader.peek();

		if(this->reader[next_token].getKind() != Token::Ident){
			return Result::WrongType;
		}

		this->reader.skip();

		return AST::Node(AST::Kind::Ident, next_token);
	};



	//////////////////////////////////////////////////////////////////////
	// checking and messaging


	auto Parser::expected_but_got(std::string_view expected, const Token& token) noexcept -> bool {
		this->context.emitError(
			Diagnostic::Code::ParserIncorrectStmtContinuation, token.getSourceLocation(this->source_id),
			std::format("Expected {}, got [{}] instead", expected, token.getKind())
		);

		this->last_result = Result::Error;

		return this->context.hasHitFailCondition();
	};


	auto Parser::check_result(const Result& result, std::string_view expected) noexcept -> bool {
		if(result.code() == Result::Error){
			this->last_result = Result::Error;
			return this->context.hasHitFailCondition();
		}

		if(result.code() == Result::WrongType){ 
			this->last_result = Result::WrongType;			
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

		this->last_result = Result::Error;

		return this->context.hasHitFailCondition();
	};

	auto Parser::assert_token(Token::Kind kind, std::string_view location_str) noexcept -> bool {
		const Token& next_token = this->reader[this->reader.next()];

		if(next_token.getKind() == kind){ return false; }

		this->context.emitFatal(
			Diagnostic::Code::ParserIncorrectStmtContinuation, next_token.getSourceLocation(this->source_id),
			std::format("Expected [{}] {}, got [{}] instead", kind, location_str, next_token.getKind())
		);

		this->last_result = Result::Error;

		return true;
	};


	//////////////////////////////////////////////////////////////////////
	// misc helpers

	auto Parser::getASTBuffer() noexcept -> ASTBuffer& {
		return this->context.getSourceManager().getSource(this->source_id).ast_buffer;
	};


};