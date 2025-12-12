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
						Diagnostic::Location::get(this->reader.peek(), this->source),
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
			case Token::Kind::KEYWORD_VAR:         return this->parse_var_def<AST::VarDef::Kind::VAR>();
			case Token::Kind::KEYWORD_CONST:       return this->parse_var_def<AST::VarDef::Kind::CONST>();
			case Token::Kind::KEYWORD_DEF:         return this->parse_var_def<AST::VarDef::Kind::DEF>();
			case Token::Kind::KEYWORD_FUNC:        return this->parse_func_def<true>();
			case Token::Kind::KEYWORD_TYPE:        return this->parse_type_def();
			case Token::Kind::KEYWORD_INTERFACE:   return this->parse_interface_def();
			case Token::Kind::KEYWORD_IMPL:        return this->parse_interface_impl<true>();
			case Token::Kind::KEYWORD_RETURN:      return this->parse_return();
			case Token::Kind::KEYWORD_ERROR:       return this->parse_error();
			case Token::Kind::KEYWORD_UNREACHABLE: return this->parse_unreachable();
			case Token::Kind::KEYWORD_BREAK:       return this->parse_break();
			case Token::Kind::KEYWORD_CONTINUE:    return this->parse_continue();
			case Token::Kind::KEYWORD_DELETE:      return this->parse_delete();
			case Token::Kind::KEYWORD_IF:          return this->parse_conditional<false>();
			case Token::Kind::KEYWORD_WHEN:        return this->parse_conditional<true>();
			case Token::Kind::KEYWORD_WHILE:       return this->parse_while();
			case Token::Kind::KEYWORD_FOR:         return this->parse_for();
			case Token::Kind::KEYWORD_DEFER:       return this->parse_defer<false>();
			case Token::Kind::KEYWORD_ERROR_DEFER: return this->parse_defer<true>();
			case Token::Kind::KEYWORD_TRY:         return this->parse_try_stmt();
		}

		Result result = this->parse_assignment();
		if(result.code() != Result::Code::WRONG_TYPE){ return result; }

		result = this->parse_term_stmt();
		if(result.code() != Result::Code::WRONG_TYPE){ return result; }

		return this->parse_block(BlockLabelRequirement::NOT_ALLOWED);
	}


	// TODO(FUTURE): check EOF
	template<AST::VarDef::Kind VAR_DEF_KIND>
	auto Parser::parse_var_def() -> Result {
		if constexpr(VAR_DEF_KIND == AST::VarDef::Kind::VAR){
			if(this->assert_token(Token::Kind::KEYWORD_VAR).isError()){ return Result::Code::ERROR; }

		}else if constexpr(VAR_DEF_KIND == AST::VarDef::Kind::CONST){
			if(this->assert_token(Token::Kind::KEYWORD_CONST).isError()){ return Result::Code::ERROR; }
			
		}else{
			if(this->assert_token(Token::Kind::KEYWORD_DEF).isError()){ return Result::Code::ERROR; }
		}

		const Result ident = this->parse_ident();
		if(this->check_result(ident, "identifier in variable definition").isError()){ return Result::Code::ERROR; }


		auto type = std::optional<AST::Node>();
		if(this->reader[this->reader.peek()].kind() == Token::lookupKind(":")){
			if(this->assert_token(Token::lookupKind(":")).isError()){ return Result::Code::ERROR; }

			const Result type_result = this->parse_type<TypeKind::EXPLICIT_MAYBE_DEDUCER>();
			if(this->check_result(type_result, "type after [:] in variable definition").isError()){
				return Result::Code::ERROR;
			}

			type = type_result.value();
		}


		const Result attributes = this->parse_attribute_block();
		if(attributes.code() == Result::Code::ERROR){ return Result::Code::ERROR; }


		auto value = std::optional<AST::Node>();
		if(this->reader[this->reader.peek()].kind() == Token::lookupKind("=")){
			if(this->assert_token(Token::lookupKind("=")).isError()){ return Result::Code::ERROR; }
				
			const Result value_result = this->parse_expr();
			// TODO(FUTURE): better messaging around block exprs missing a label
			if(this->check_result(value_result, "expression after [=] in variable definition").isError()){
				return Result::Code::ERROR;
			}

			value = value_result.value();
		}


		if(this->reader[this->reader.peek()].kind() == Token::lookupKind(";")){
			if(this->assert_token(Token::lookupKind(";")).isError()){ return Result::Code::ERROR; }

		}else if( // check for attributes in wrong place
			type.has_value() == false
			&& value.has_value() == false
			&& this->reader[this->reader.peek()].kind() == Token::lookupKind(":")
			&& this->source.getASTBuffer().getAttributeBlock(attributes.value()).attributes.empty() == false
		){
			this->context.emitError(
				Diagnostic::Code::PARSER_ATTRIBUTES_IN_WRONG_PLACE,
				Diagnostic::Location::get(this->reader.peek(-1), this->source),
				"Attributes for variable definition in the wrong place",
				evo::SmallVector<Diagnostic::Info>{
					Diagnostic::Info("If the variable is explicitly-typed, the attributes go after the type")
				}
			);
			return Result::Code::ERROR;

		}else{
			this->expected_but_got("[;] at end of variable definition", this->reader.peek());
			return Result::Code::ERROR;
		}



		return this->source.ast_buffer.createVarDef(
			VAR_DEF_KIND, ASTBuffer::getIdent(ident.value()), type, attributes.value(), value
		);
	}


	// TODO(FUTURE): check EOF
	template<bool MUST_HAVE_BODY>
	auto Parser::parse_func_def() -> Result {
		if(this->assert_token(Token::Kind::KEYWORD_FUNC).isError()){ return Result::Code::ERROR; }

		const Token::ID name = this->reader.next();

		switch(this->reader[name].kind()){
			case Token::Kind::IDENT:        case Token::Kind::KEYWORD_COPY:   case Token::Kind::KEYWORD_MOVE:
			case Token::Kind::KEYWORD_NEW:  case Token::Kind::KEYWORD_DELETE: case Token::Kind::KEYWORD_AS:

			case Token::lookupKind("+"):    case Token::lookupKind("+%"):     case Token::lookupKind("+|"):
			case Token::lookupKind("-"):    case Token::lookupKind("-%"):     case Token::lookupKind("-|"):
			case Token::lookupKind("*"):    case Token::lookupKind("*%"):     case Token::lookupKind("*|"):
			case Token::lookupKind("/"):    case Token::lookupKind("%"):      case Token::lookupKind("=="):
			case Token::lookupKind("!="):   case Token::lookupKind("<"):      case Token::lookupKind("<="):
			case Token::lookupKind(">"):    case Token::lookupKind(">="):     case Token::lookupKind("!"):
			case Token::lookupKind("&&"):   case Token::lookupKind("||"):     case Token::lookupKind("<<"):
			case Token::lookupKind("<<|"):  case Token::lookupKind(">>"):     case Token::lookupKind("&"):
			case Token::lookupKind("|"):    case Token::lookupKind("^"):      case Token::lookupKind("~"):

			case Token::lookupKind("+="):   case Token::lookupKind("+%="):    case Token::lookupKind("+|="):
			case Token::lookupKind("-="):   case Token::lookupKind("-%="):    case Token::lookupKind("-|="):
			case Token::lookupKind("*="):   case Token::lookupKind("*%="):    case Token::lookupKind("*|="):
			case Token::lookupKind("/="):   case Token::lookupKind("%="):     case Token::lookupKind("<<="):
			case Token::lookupKind("<<|="): case Token::lookupKind(">>="):    case Token::lookupKind("&="):
			case Token::lookupKind("|="):   case Token::lookupKind("^="): {
				break;
			}

			case Token::lookupKind("["): {
				if(this->expect_token(Token::lookupKind("]"), "[]] at end of indexer overload name").isError()){
					return Result::Code::ERROR;
				}
			} break;

			default: {
				this->expected_but_got(
					"identifier or overloadable operator after [func] in function definition", this->reader.peek(-1)
				);
				return Result::Code::ERROR;
			}
		}


		const Result attributes = this->parse_attribute_block();
		if(this->check_result(attributes, "attributes in function definition").isError()){ return Result::Code::ERROR; }


		if(this->expect_token(Token::lookupKind("="), "after identifier in function definition").isError()){
			return Result::Code::ERROR;
		}


		if(this->reader[this->reader.peek()].kind() == Token::Kind::KEYWORD_DELETE){
			const AST::AttributeBlock& attribute_block = this->source.ast_buffer.getAttributeBlock(attributes.value());
			if(attribute_block.attributes.empty() == false){
				this->context.emitError(
					Diagnostic::Code::PARSER_INVALID_DELETED_SPECIAL_METHOD,
					Diagnostic::Location::get(attribute_block.attributes[0].attribute, this->source),
					"Deleted special members cannot accept attributes"
				);
				return Result::Code::ERROR;
			}

			if(this->assert_token(Token::Kind::KEYWORD_DELETE).isError()){ return Result::Code::ERROR; }

			const Token::Kind member_token_kind = this->reader[name].kind();

			if(member_token_kind != Token::Kind::KEYWORD_COPY && member_token_kind != Token::Kind::KEYWORD_MOVE){
				if(member_token_kind == Token::Kind::IDENT){
					this->context.emitError(
						Diagnostic::Code::PARSER_INVALID_DELETED_SPECIAL_METHOD,
						Diagnostic::Location::get(name, this->source),
						"Invalid deleted special method (not a special method)",
						evo::SmallVector<Diagnostic::Info>{Diagnostic::Info("Deleted overloads require parameters")}
					);
				}else{
					this->context.emitError(
						Diagnostic::Code::PARSER_INVALID_DELETED_SPECIAL_METHOD,
						Diagnostic::Location::get(name, this->source),
						"Invalid deleted special method"
					);
				}
				return Result::Code::ERROR;
			}

			if(this->expect_token(Token::lookupKind(";"), "at end of deleted special method").isError()){
				return Result::Code::ERROR;
			}

			return this->source.ast_buffer.createDeletedSpecialMethod(name);

		}else if(this->reader[this->reader.peek()].kind() == Token::Kind::KEYWORD_ALIAS){
			this->reader.skip();

			if(this->reader[name].kind() != Token::Kind::IDENT){
				this->context.emitError(
					Diagnostic::Code::PARSER_FUNC_ALIAS_CANNOT_BE_AN_OPERATOR,
					Diagnostic::Location::get(name, this->source),
					"Function alias cannot be an operator"
				);
				return Result::Code::ERROR;
			}

			const Result aliased_func = this->parse_expr();
			if(this->check_result(aliased_func, "aliased function").isError()){
				return Result::Code::ERROR;
			}

			if(this->expect_token(Token::lookupKind(";"), "at end of function alias").isError()){
				return Result::Code::ERROR;
			}

			return this->source.ast_buffer.createFuncAliasDef(name, attributes.value(), aliased_func.value());
		}


		if(this->source.ast_buffer.getAttributeBlock(attributes.value()).attributes.empty() == false){
			this->context.emitError(
				Diagnostic::Code::PARSER_ATTRIBUTES_IN_WRONG_PLACE,
				Diagnostic::Location::get(this->reader.peek(), this->source),
				"Attributes for function definition in the wrong place",
				evo::SmallVector<Diagnostic::Info>{
					Diagnostic::Info("Attributes should be after the parameters block")
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

		evo::Result<evo::SmallVector<AST::FuncDef::Param>> params = this->parse_func_params();
		if(params.isError()){ return Result::Code::ERROR; }


		const Result attribute_block = this->parse_attribute_block();
		if(attribute_block.code() == Result::Code::ERROR){ return Result::Code::ERROR; }


		if(this->expect_token(Token::lookupKind("->"), "in function definition").isError()){
			return Result::Code::ERROR;
		}

		if(this->reader[this->reader.peek()].kind() == Token::Kind::KEYWORD_DELETE){
			this->context.emitError(
				Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
				Diagnostic::Location::get(this->reader.peek(), this->source),
				"Deleted overloads are currently unimplemented"
			);
			return Result::Code::ERROR;
		}

		evo::Result<evo::SmallVector<AST::FuncDef::Return>> returns = this->parse_func_returns<!MUST_HAVE_BODY>();
		if(returns.isError()){ return Result::Code::ERROR; }

		evo::Result<evo::SmallVector<AST::FuncDef::Return>> error_returns = this->parse_func_error_returns();
		if(error_returns.isError()){ return Result::Code::ERROR; }

		const Result block = this->parse_block(BlockLabelRequirement::NOT_ALLOWED);

		if constexpr(MUST_HAVE_BODY){
			// TODO(FUTURE): better messaging 
			if(this->check_result(block, "statement block in function definition").isError()){
				return Result::Code::ERROR;
			}

			return this->source.ast_buffer.createFuncDef(
				name,
				template_pack_node,
				std::move(params.value()),
				attribute_block.value(),
				std::move(returns.value()),
				std::move(error_returns.value()),
				block.value()
			);

		}else{
			switch(block.code()){
				case Result::Code::SUCCESS: {
					return this->source.ast_buffer.createFuncDef(
						name,
						template_pack_node,
						std::move(params.value()),
						attribute_block.value(),
						std::move(returns.value()),
						std::move(error_returns.value()),
						block.value()
					);
				} break;

				case Result::Code::WRONG_TYPE: {
					if(this->expect_token(
						Token::lookupKind(";"), "or function block at end of function definition"
					).isError()){
						return Result::Code::ERROR;
					}

					return this->source.ast_buffer.createFuncDef(
						name,
						template_pack_node,
						std::move(params.value()),
						attribute_block.value(),
						std::move(returns.value()),
						std::move(error_returns.value()),
						std::nullopt
					);
				} break;

				case Result::Code::ERROR: {
					return Result::Code::ERROR;
				} break;
			}

			evo::unreachable();
		}

	}


	// TODO(FUTURE): check EOF
	auto Parser::parse_type_def() -> Result {
		if(this->assert_token(Token::Kind::KEYWORD_TYPE).isError()){ return Result::Code::ERROR; }

		const Result ident = this->parse_ident();
		if(this->check_result(ident, "identifier in type definition").isError()){ return Result::Code::ERROR; }

		const Result attributes = this->parse_attribute_block();
		if(attributes.code() == Result::Code::ERROR){ return Result::Code::ERROR; }

		if(this->expect_token(Token::lookupKind("="), "in type definition").isError()){ return Result::Code::ERROR; }

		switch(this->reader[this->reader.peek()].kind()){
			case Token::Kind::KEYWORD_STRUCT: return this->parse_struct_def(ident.value(), attributes.value());
			case Token::Kind::KEYWORD_UNION:  return this->parse_union_def(ident.value(), attributes.value());
			case Token::Kind::KEYWORD_ENUM:   return this->parse_enum_def(ident.value(), attributes.value());
			case Token::Kind::KEYWORD_ALIAS:  return this->parse_type_alias(ident.value(), attributes.value());
		}

		this->expected_but_got("valid type kind ([struct], [union], [enum], or [alias])", this->reader.peek());
		return Result::Code::ERROR;
	}


	// TODO(FUTURE): check EOF
	auto Parser::parse_struct_def(const AST::Node& ident, const AST::Node& attrs_pre_equals) -> Result {
		if(this->assert_token(Token::Kind::KEYWORD_STRUCT).isError()){ return Result::Code::ERROR; }

		if(this->source.getASTBuffer().getAttributeBlock(attrs_pre_equals).attributes.empty() == false){
			this->context.emitError(
				Diagnostic::Code::PARSER_ATTRIBUTES_IN_WRONG_PLACE,
				Diagnostic::Location::get(
					this->source.getASTBuffer().getAttributeBlock(attrs_pre_equals).attributes.front().attribute,
					this->source
				),
				"Attributes for struct definition in the wrong place",
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
			case Result::Code::SUCCESS:    template_pack_node = template_pack_result.value(); break;
			case Result::Code::WRONG_TYPE: break;
			case Result::Code::ERROR:      return Result::Code::ERROR;	
		}

		const Result attributes = this->parse_attribute_block();
		if(attributes.code() == Result::Code::ERROR){ return Result::Code::ERROR; }

		const Result block = this->parse_block(BlockLabelRequirement::NOT_ALLOWED);
		if(this->check_result(block, "statement block in struct definition").isError()){// TODO(FUTURE): better message
			return Result::Code::ERROR;
		}

		return this->source.ast_buffer.createStructDef(
			ASTBuffer::getIdent(ident), template_pack_node, std::move(attributes.value()), block.value()
		);
	}


	// TODO(FUTURE): check EOF
	auto Parser::parse_union_def(const AST::Node& ident, const AST::Node& attrs_pre_equals) -> Result {
		if(this->assert_token(Token::Kind::KEYWORD_UNION).isError()){ return Result::Code::ERROR; }

		if(this->source.getASTBuffer().getAttributeBlock(attrs_pre_equals).attributes.empty() == false){
			this->context.emitError(
				Diagnostic::Code::PARSER_ATTRIBUTES_IN_WRONG_PLACE,
				Diagnostic::Location::get(
					this->source.getASTBuffer().getAttributeBlock(attrs_pre_equals).attributes.front().attribute,
					this->source
				),
				"Attributes for union definition in the wrong place",
				evo::SmallVector<Diagnostic::Info>{
					Diagnostic::Info("Attributes should be after the [union] keyword")
				}
			);
			return Result::Code::ERROR;
		}

		const Result attributes = this->parse_attribute_block();
		if(attributes.code() == Result::Code::ERROR){ return Result::Code::ERROR; }

		if(this->expect_token(Token::lookupKind("{"), "to begin union block").isError()){ return Result::Code::ERROR; }

		auto fields = evo::SmallVector<AST::UnionDef::Field>();
		auto statements = evo::SmallVector<AST::Node>();

		while(true){
			if(this->reader[this->reader.peek()].kind() == Token::lookupKind("}")){
				this->reader.skip();
				break;
			}


			const Result field_ident = this->parse_ident();
			// TODO(PERF): remove? parse_ident can't error
			if(field_ident.code() == Result::Code::ERROR){ return Result::Code::ERROR; } 


			if(field_ident.code() == Result::Code::SUCCESS){ // is field
				if(this->expect_token(Token::lookupKind(":"), "after identifier in union field").isError()){
					return Result::Code::ERROR;
				}

				const Result type = this->parse_type<TypeKind::EXPLICIT>();
				if(this->check_result(type, "type after [:] in union definition").isError()){
					return Result::Code::ERROR;
				}

				if(this->expect_token(Token::lookupKind(","), "after type in union field").isError()){
					return Result::Code::ERROR;
				}

				fields.emplace_back(ASTBuffer::getIdent(field_ident.value()), type.value());
				
			}else{ // is statement
				const Result stmt = this->parse_stmt();
				if(this->check_result(
					stmt, "field or statement in union definition, or [}] at end of union definition block"
				).isError()){
					return Result::Code::ERROR;
				}

				statements.emplace_back(stmt.value());
			}
		}


		if(fields.empty()){
			this->context.emitError(
				Diagnostic::Code::PARSER_UNION_WITH_NO_FIELDS,
				Diagnostic::Location::get(ASTBuffer::getIdent(ident), this->source),
				"Enum must be defined with at least one field"
			);
			return Result::Code::ERROR;
		}


		return this->source.ast_buffer.createUnionDef(
			ASTBuffer::getIdent(ident), attributes.value(), std::move(fields), std::move(statements)
		);
	}


	// TODO(FUTURE): check EOF
	auto Parser::parse_enum_def(const AST::Node& ident, const AST::Node& attrs_pre_equals) -> Result {
		if(this->assert_token(Token::Kind::KEYWORD_ENUM).isError()){ return Result::Code::ERROR; }

		if(this->source.getASTBuffer().getAttributeBlock(attrs_pre_equals).attributes.empty() == false){
			this->context.emitError(
				Diagnostic::Code::PARSER_ATTRIBUTES_IN_WRONG_PLACE,
				Diagnostic::Location::get(
					this->source.getASTBuffer().getAttributeBlock(attrs_pre_equals).attributes.front().attribute,
					this->source
				),
				"Attributes for enum definition in the wrong place",
				evo::SmallVector<Diagnostic::Info>{
					Diagnostic::Info("Attributes should be after the [enum] keyword")
				}
			);
			return Result::Code::ERROR;
		}

		auto underlying_type = std::optional<AST::Node>();
		if(this->reader[this->reader.peek()].kind() == Token::lookupKind("(")){
			if(this->assert_token(Token::lookupKind("(")).isError()){ return Result::Code::ERROR; }

			const Result underlying_type_result = this->parse_type<TypeKind::EXPLICIT>();
			if(this->check_result(underlying_type_result, "enum underlying type").isError()){
				return Result::Code::ERROR;
			}

			underlying_type = underlying_type_result.value();

			if(this->expect_token(Token::lookupKind(")"), "after enum underlying type").isError()){
				return Result::Code::ERROR;
			}
		}

		const Result attributes = this->parse_attribute_block();
		if(attributes.code() == Result::Code::ERROR){ return Result::Code::ERROR; }

		if(this->expect_token(Token::lookupKind("{"), "to begin enum block").isError()){ return Result::Code::ERROR; }

		auto enumerators = evo::SmallVector<AST::EnumDef::Enumerator>();
		auto statements = evo::SmallVector<AST::Node>();

		while(true){
			if(this->reader[this->reader.peek()].kind() == Token::lookupKind("}")){
				this->reader.skip();
				break;
			}


			const Result enumerator_ident = this->parse_ident();
			// TODO(PERF): remove? parse_ident can't error
			if(enumerator_ident.code() == Result::Code::ERROR){ return Result::Code::ERROR; } 


			if(enumerator_ident.code() == Result::Code::SUCCESS){ // is field
				auto enumerator_value = std::optional<AST::Node>();

				if(this->reader[this->reader.peek()].kind() == Token::lookupKind("=")){
					this->reader.skip();

					const Result enumerator_value_result = this->parse_expr();
					if(this->check_result(enumerator_value_result, "enumerator value").isError()){
						return Result::Code::ERROR;
					}

					enumerator_value = enumerator_value_result.value();
				}

				if(this->expect_token(Token::lookupKind(","), "after type in enum enumerator").isError()){
					return Result::Code::ERROR;
				}

				enumerators.emplace_back(ASTBuffer::getIdent(enumerator_ident.value()), enumerator_value);

			}else{ // is statement
				const Result stmt = this->parse_stmt();
				if(this->check_result(
					stmt, "field or statement in enum definition, or [}] at end of enum definition block"
				).isError()){
					return Result::Code::ERROR;
				}

				statements.emplace_back(stmt.value());
			}
		}


		if(enumerators.empty()){
			this->context.emitError(
				Diagnostic::Code::PARSER_ENUM_WITH_NO_ENUMERATORS,
				Diagnostic::Location::get(ASTBuffer::getIdent(ident), this->source),
				"Enum must be defined with at least one enumerator"
			);
			return Result::Code::ERROR;
		}


		return this->source.ast_buffer.createEnumDef(
			ASTBuffer::getIdent(ident),
			underlying_type,
			attributes.value(),
			std::move(enumerators),
			std::move(statements)
		);
	}



	auto Parser::parse_type_alias(const AST::Node& ident, const AST::Node& attrs_pre_equals) -> Result {
		if(this->assert_token(Token::Kind::KEYWORD_ALIAS).isError()){ return Result::Code::ERROR; }

		const Result type = this->parse_type<TypeKind::EXPLICIT>();
		if(this->check_result(type, "type in type alias definition").isError()){ return Result::Code::ERROR; }

		if(this->expect_token(Token::lookupKind(";"), "at end of type alias definition").isError()){
			return Result::Code::ERROR;
		}

		return this->source.ast_buffer.createAliasDef(ASTBuffer::getIdent(ident), attrs_pre_equals, type.value());
	}



	// TODO(FUTURE): check EOF
	auto Parser::parse_interface_def() -> Result {
		if(this->assert_token(Token::Kind::KEYWORD_INTERFACE).isError()){ return Result::Code::ERROR; }

		const Result ident = this->parse_ident();
		if(this->check_result(ident, "identifier in interface definition").isError()){ return Result::Code::ERROR; }

		if(this->expect_token(Token::lookupKind("="), "after identifier in interface definition").isError()){
			return Result::Code::ERROR;
		}

		const Result attributes = this->parse_attribute_block();
		if(attributes.code() == Result::Code::ERROR){ return Result::Code::ERROR; }

		if(this->expect_token(Token::lookupKind("{"), "after [=] in interface definition").isError()){
			return Result::Code::ERROR;
		}


		auto methods = evo::SmallVector<AST::Node>();
		auto impls = evo::SmallVector<AST::Node>();

		while(this->reader[this->reader.peek()].kind() != Token::lookupKind("}")){
			switch(this->reader[this->reader.peek()].kind()){
				case Token::Kind::KEYWORD_FUNC: {
					const Result method = this->parse_func_def<false>();
					if(this->check_result(
						method, "interface method definition, impl, or end of interface definition"
					).isError()){
						return Result::Code::ERROR;
					}

					methods.emplace_back(method.value());
				} break;

				case Token::Kind::KEYWORD_IMPL: {
					const Result impl_res = this->parse_interface_impl<false>();

					if(this->check_result(
						impl_res, "interface method definition, impl, or end of interface definition"
					).isError()){
						return Result::Code::ERROR;
					}

					impls.emplace_back(impl_res.value());
				} break;

				default: {
					this->expected_but_got(
						"interface method definition, impl, or end of interface definition", this->reader.peek()
					);
					return Result::Code::ERROR;
				} break;
			}
		}

		if(this->expect_token(Token::lookupKind("}"), "at end of interface definition").isError()){
			return Result::Code::ERROR;
		}

		return this->source.ast_buffer.createInterfaceDef(
			ASTBuffer::getIdent(ident.value()), attributes.value(), std::move(methods), std::move(impls)
		);
	}


	// TODO(FUTURE): check EOF
	template<bool ALLOW_METHOD_IDENTS>
	auto Parser::parse_interface_impl() -> Result {
		if(this->assert_token(Token::Kind::KEYWORD_IMPL).isError()){ return Result::Code::ERROR; }

		const Result target = this->parse_type<TypeKind::EXPLICIT_MAYBE_ANONYMOUS_DEDUCER>();
		if(this->check_result(target, "interface impl target").isError()){ return Result::Code::ERROR; }

		const Result impl_attributes = this->parse_attribute_block();
		if(impl_attributes.code() == Result::Code::ERROR){ return Result::Code::ERROR; }

		if(this->expect_token(Token::lookupKind("{"), "after target interface in interface impl").isError()){
			return Result::Code::ERROR;
		}


		auto methods = evo::SmallVector<AST::InterfaceImpl::Method>();

		while(true){
			if(this->reader[this->reader.peek()].kind() == Token::lookupKind("}")){
				if(this->assert_token(Token::lookupKind("}")).isError()){ return Result::Code::ERROR; }
				break;
			}

			const Result method_ident = this->parse_ident();
			if(this->check_result(method_ident, "method identifier in interface impl").isError()){
				return Result::Code::ERROR;
			}

			if(this->expect_token(Token::lookupKind("="), "after method identifier in interface impl").isError()){
				return Result::Code::ERROR;
			}


			switch(this->reader[this->reader.peek()].kind()){
				case Token::Kind::IDENT: {
					if constexpr(ALLOW_METHOD_IDENTS){
						methods.emplace_back(ASTBuffer::getIdent(method_ident.value()), this->reader.next());
					}else{
						this->expected_but_got(
							"method value in interface impl",
							this->reader.peek(),
							evo::SmallVector<Diagnostic::Info>{
								Diagnostic::Info(
									"Method identitiers are only allowed if the impl is defined within the target type"
								)
							}
						);
						return Result::Code::ERROR;
					}
				} break;

				case Token::lookupKind("("): {
					evo::Result<evo::SmallVector<AST::FuncDef::Param>> params = this->parse_func_params();
					if(params.isError()){ return Result::Code::ERROR; }


					const Result attribute_block = this->parse_attribute_block();
					if(attribute_block.code() == Result::Code::ERROR){ return Result::Code::ERROR; }


					if(this->expect_token(Token::lookupKind("->"), "in function definition").isError()){
						return Result::Code::ERROR;
					}

					evo::Result<evo::SmallVector<AST::FuncDef::Return>> returns = this->parse_func_returns<false>();
					if(returns.isError()){ return Result::Code::ERROR; }

					evo::Result<evo::SmallVector<AST::FuncDef::Return>> error_returns =
						this->parse_func_error_returns();
					if(error_returns.isError()){ return Result::Code::ERROR; }

					const Result block = this->parse_block(BlockLabelRequirement::NOT_ALLOWED);
					if(this->check_result(block, "block in interface impl").isError()){ return Result::Code::ERROR; }

					const AST::Node created_func_def_node = this->source.ast_buffer.createFuncDef(
						ASTBuffer::getIdent(method_ident.value()),
						std::nullopt,
						std::move(params.value()),
						attribute_block.value(),
						std::move(returns.value()),
						std::move(error_returns.value()),
						block.value()
					);

					methods.emplace_back(ASTBuffer::getIdent(method_ident.value()), created_func_def_node);
				} break;

				default: {
					this->expected_but_got("method value in interface impl", this->reader.peek());
					return Result::Code::ERROR;
				} break;
			}


			// check if ending or should continue
			const Token::Kind after_arg_next_token_kind = this->reader[this->reader.next()].kind();
			if(after_arg_next_token_kind != Token::lookupKind(",")){
				if(after_arg_next_token_kind != Token::lookupKind("}")){
					this->expected_but_got(
						"[,] at end of interface impl method or [}] at end of interface impl",
						this->reader.peek(-1)
					);
					return Result::Code::ERROR;
				}

				break;
			}
		}

		return this->source.ast_buffer.createInterfaceImpl(target.value(), impl_attributes.value(), std::move(methods));
	}




	// TODO(FUTURE): check EOF
	auto Parser::parse_return() -> Result {
		const Token::ID start_location = this->reader.peek();
		if(this->assert_token(Token::Kind::KEYWORD_RETURN).isError()){ return Result::Code::ERROR; }

		auto label = std::optional<AST::Node>();
		auto expr = evo::Variant<std::monostate, AST::Node, Token::ID>();

		if(this->reader[this->reader.peek()].kind() == Token::lookupKind("->")){
			if(this->assert_token(Token::lookupKind("->")).isError()){ return Result::Code::ERROR; }

			const Result label_result = this->parse_ident();
			if(this->check_result(label_result, "identifier in return block label").isError()){
				return Result::Code::ERROR;
			}
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

		if(this->expect_token(Token::lookupKind(";"), "at the end of a [return] statement").isError()){
			return Result::Code::ERROR;
		}

		return this->source.ast_buffer.createReturn(start_location, label, expr);
	}

	// TODO(FUTURE): check EOF
	auto Parser::parse_error() -> Result {
		const Token::ID start_location = this->reader.peek();
		if(this->assert_token(Token::Kind::KEYWORD_ERROR).isError()){ return Result::Code::ERROR; }

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

		if(this->expect_token(Token::lookupKind(";"), "at the end of a [error] statement").isError()){
			return Result::Code::ERROR;
		}

		return this->source.ast_buffer.createError(start_location, expr);
	}


	// TODO(FUTURE): check EOF
	auto Parser::parse_unreachable() -> Result {
		const Token::ID start_location = this->reader.peek();
		if(this->assert_token(Token::Kind::KEYWORD_UNREACHABLE).isError()){ return Result::Code::ERROR; }

		if(this->expect_token(Token::lookupKind(";"), "at the end of a [unreachable] statement").isError()){
			return Result::Code::ERROR;
		}

		return AST::Node(AST::Kind::UNREACHABLE, start_location);
	}



	auto Parser::parse_break() -> Result {
		const Token::ID keyword_token_id = this->reader.peek();
		if(this->assert_token(Token::Kind::KEYWORD_BREAK).isError()){ return Result::Code::ERROR; }

		auto label = std::optional<Token::ID>();
		if(this->reader[this->reader.peek()].kind() == Token::lookupKind("->")){
			if(this->assert_token(Token::lookupKind("->")).isError()){ return Result::Code::ERROR; }

			const Result label_result = this->parse_ident();
			if(this->check_result(label_result, "label after [->] in break statement").isError()){
				return Result::Code::ERROR;
			}

			label = ASTBuffer::getIdent(label_result.value());
		}

		if(this->expect_token(Token::lookupKind(";"), "at the end of a [break] statement").isError()){
			return Result::Code::ERROR;
		}

		return this->source.ast_buffer.createBreak(keyword_token_id, label);
	}


	auto Parser::parse_continue() -> Result {
		const Token::ID keyword_token_id = this->reader.peek();
		if(this->assert_token(Token::Kind::KEYWORD_CONTINUE).isError()){ return Result::Code::ERROR; }

		auto label = std::optional<Token::ID>();
		if(this->reader[this->reader.peek()].kind() == Token::lookupKind("->")){
			if(this->assert_token(Token::lookupKind("->")).isError()){ return Result::Code::ERROR; }

			const Result label_result = this->parse_ident();
			if(this->check_result(label_result, "label after [->] in break statement").isError()){
				return Result::Code::ERROR;
			}

			label = ASTBuffer::getIdent(label_result.value());
		}

		if(this->expect_token(Token::lookupKind(";"), "at the end of a [break] statement").isError()){
			return Result::Code::ERROR;
		}

		return this->source.ast_buffer.createContinue(keyword_token_id, label);
	}



	auto Parser::parse_delete() -> Result {
		const Token::ID keyword_token_id = this->reader.peek();
		if(this->assert_token(Token::Kind::KEYWORD_DELETE).isError()){ return Result::Code::ERROR; }

		const Result expr = this->parse_expr();
		if(this->check_result(expr, "expression in [delete] statement").isError()){ return Result::Code::ERROR; }

		if(this->expect_token(Token::lookupKind(";"), "at the end of a [delete] statement").isError()){
			return Result::Code::ERROR;
		}

		return this->source.ast_buffer.createDelete(keyword_token_id, expr.value());
	}



	// TODO(FUTURE): check EOF
	template<bool IS_WHEN>
	auto Parser::parse_conditional() -> Result {
		const Token::ID keyword_token_id = this->reader.peek();

		static constexpr Token::Kind COND_TOKEN_KIND = IS_WHEN ? Token::Kind::KEYWORD_WHEN : Token::Kind::KEYWORD_IF;
		if(this->assert_token(COND_TOKEN_KIND).isError()){ return Result::Code::ERROR; }


		if(this->expect_token(Token::lookupKind("("), "in conditional statement").isError()){
			return Result::Code::ERROR;
		}

		const Result cond = this->parse_expr();

		if(this->expect_token(Token::lookupKind(")"), "in conditional statement").isError()){
			return Result::Code::ERROR;
		}

		const Result then_block = this->parse_block(BlockLabelRequirement::NOT_ALLOWED);
		if(this->check_result(then_block, "statement block in conditional statement").isError()){
			return Result::Code::ERROR;
		}

		auto else_block = std::optional<AST::Node>();
		if(this->reader.at_end() == false && this->reader[this->reader.peek()].kind() == Token::Kind::KEYWORD_ELSE){
			if(this->assert_token(Token::Kind::KEYWORD_ELSE).isError()){ return Result::Code::ERROR; }

			const Token::Kind else_if_kind = this->reader[this->reader.peek()].kind();

			if(else_if_kind == Token::lookupKind("{")){
				const Result else_block_result = this->parse_block(BlockLabelRequirement::NOT_ALLOWED);
				if(this->check_result(else_block_result, "statement block in conditional statement").isError()){
					return Result::Code::ERROR;
				}

				else_block = else_block_result.value();

			}else if(else_if_kind != COND_TOKEN_KIND){
				if constexpr(IS_WHEN){
					if(else_if_kind == Token::Kind::KEYWORD_IF){
						this->expected_but_got(
							"[when] after [else]",
							this->reader.peek(),
							evo::SmallVector<Diagnostic::Info>{ // TODO(FUTURE): better messaging
								Diagnostic::Info("Cannot mix [if] and [when] in a chain"),
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
							evo::SmallVector<Diagnostic::Info>{ // TODO(FUTURE): better messaging
								Diagnostic::Info("Cannot mix [if] and [when] in a chain"),
							}
						);
					}else{
						this->expected_but_got("[if] or [{] after [else]", this->reader.peek());
					}
				}
				return Result::Code::ERROR;

			}else{
				const Result else_block_result = this->parse_conditional<IS_WHEN>();
				if(this->check_result(else_block_result, "statement block in conditional statement").isError()){
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


	// TODO(FUTURE): check EOF
	auto Parser::parse_while() -> Result {
		const Token::ID start_location = this->reader.peek();
		if(this->assert_token(Token::Kind::KEYWORD_WHILE).isError()){ return Result::Code::ERROR; }

		if(this->expect_token(Token::lookupKind("("), "in while loop").isError()){ return Result::Code::ERROR; }

		const Result cond = this->parse_expr();
		if(this->check_result(cond, "condition in while loop").isError()){ return Result::Code::ERROR; }

		if(this->expect_token(Token::lookupKind(")"), "in while loop").isError()){ return Result::Code::ERROR; }

		const Result block = this->parse_block(BlockLabelRequirement::OPTIONAL);
		if(this->check_result(block, "statement block in while loop").isError()){ return Result::Code::ERROR; }

		return this->source.ast_buffer.createWhile(start_location, cond.value(), block.value());
	}


	// TODO(FUTURE): check EOF
	auto Parser::parse_for() -> Result {
		const Token::ID keyword = this->reader.peek();
		if(this->assert_token(Token::Kind::KEYWORD_FOR).isError()){ return Result::Code::ERROR; }


		//////////////////
		// iterables

		auto iterables = evo::SmallVector<AST::Node>();

		if(this->expect_token(Token::lookupKind("("), "at beginning of for loop iterables block").isError()){
			return Result::Code::ERROR;
		}

		while(true){
			if(this->reader[this->reader.peek()].kind() == Token::lookupKind(")")){
				if(this->assert_token(Token::lookupKind(")")).isError()){ return Result::Code::ERROR; }
				break;
			}

			const Result expr_result = this->parse_expr();
			if(this->check_result(expr_result, "iterable expression in for loop iterables block").isError()){
				return Result::Code::ERROR;
			}

			iterables.emplace_back(expr_result.value());

			// check if ending or should continue
			const Token::Kind after_arg_next_token_kind = this->reader[this->reader.next()].kind();
			if(after_arg_next_token_kind != Token::lookupKind(",")){
				if(after_arg_next_token_kind != Token::lookupKind(")")){
					this->expected_but_got(
						"[,] at end of iterable expression or [)] at end of for loop iterables block",
						this->reader.peek(-1)
					);
					return Result::Code::ERROR;
				}

				break;
			}
		}

		if(iterables.empty()){
			this->context.emitError(
				Diagnostic::Code::PARSER_FOR_NO_ITERABLES,
				Diagnostic::Location::get(this->reader.peek(-1), this->source),
				"For loop must have at least 1 iterable expression"
			);
			return Result::Code::ERROR;
		}


		//////////////////
		// parameters

		if(this->expect_token(Token::lookupKind("["), "at beginning of for loop parameters block").isError()){
			return Result::Code::ERROR;
		}

		auto index = std::optional<AST::For::Param>();

		if(this->reader[this->reader.peek()].kind() == Token::lookupKind("_")){
			this->reader.skip();

		}else{
			const Result ident = this->parse_ident();
			if(this->check_result(ident, "identifier or `_` in for loop index parameter").isError()){
				return Result::Code::ERROR;
			}

			if(this->expect_token(Token::lookupKind(":"), "after for loop index parameter identifier").isError()){
				return Result::Code::ERROR;
			}

			const Result type = this->parse_type<TypeKind::EXPLICIT>();
			if(this->check_result(type, "type of for loop index parameter").isError()){ return Result::Code::ERROR; }

			index = AST::For::Param(ASTBuffer::getIdent(ident.value()), type.value());
		}

		if(this->expect_token(Token::lookupKind(";"), "after for loop index parameter").isError()){
			return Result::Code::ERROR;
		}


		auto values = evo::SmallVector<AST::For::Param>();
		values.reserve(iterables.size());
		while(true){
			if(this->reader[this->reader.peek()].kind() == Token::lookupKind("]")){
				if(this->assert_token(Token::lookupKind("]")).isError()){ return Result::Code::ERROR; }
				break;
			}


			const Result param_ident = this->parse_ident();
			if(this->check_result(param_ident, "identifier in for loop value parameter").isError()){
				return Result::Code::ERROR;
			}

			if(this->expect_token(Token::lookupKind(":"), "after for loop value parameter identifier").isError()){
				return Result::Code::ERROR;
			}

			const Result param_type = this->parse_type<TypeKind::EXPLICIT_MAYBE_DEDUCER>();
			if(this->check_result(param_type, "type after [:] in for loop value parameter definition").isError()){
				return Result::Code::ERROR;
			}


			bool is_mut = false;

			switch(this->reader[this->reader.peek()].kind()){
				case Token::Kind::KEYWORD_READ: {
					// do nothing...
				} break;

				case Token::Kind::KEYWORD_MUT: {
					this->reader.skip();
					is_mut = true;
				} break;

				case Token::Kind::KEYWORD_IN: {
					this->context.emitError(
						Diagnostic::Code::PARSER_INVALID_KIND_FOR_A_FOR_PARAM,
						Diagnostic::Location::get(this->reader.peek(), this->source),
						"For value parameters cannot have the kind [in]"
					);
					return Result::Code::ERROR;
				} break;

				default: {
					// do nothing...
				} break;
			}


			values.emplace_back(ASTBuffer::getIdent(param_ident.value()), param_type.value(), is_mut);


			// check if ending or should continue
			const Token::Kind after_arg_next_token_kind = this->reader[this->reader.next()].kind();
			if(after_arg_next_token_kind != Token::lookupKind(",")){
				if(after_arg_next_token_kind != Token::lookupKind("]")){
					this->expected_but_got(
						"[,] at end of for loop parameter or []] at end of for loop parameter block",
						this->reader.peek(-1)
					);
					return Result::Code::ERROR;
				}

				break;
			}
		}


		//////////////////
		// checking of iterables / values

		if(iterables.size() != values.size()){
			const Diagnostic::Location location = [&]() -> Diagnostic::Location {
				if(iterables.size() < values.size()){
					return Diagnostic::Location::get(values[iterables.size()].ident, this->source);

				}else if(values.empty()){
					return Diagnostic::Location::get(this->reader.peek(-1), this->source);

				}else{
					return Diagnostic::Location::get(values.back().ident, this->source);
				}
			}();

			this->context.emitError(
				Diagnostic::Code::PARSER_FOR_NUM_ITERABLES_NEQ_NUM_PARAMS,
				location,
				"For loop mismatching number of iterables and value parameters",
				evo::SmallVector<Diagnostic::Info>{
					Diagnostic::Info(std::format("Number of iterables: {}", iterables.size())),
					Diagnostic::Info(std::format("Number of values:    {}", values.size()))
				}
			);

			return Result::Code::ERROR;
		}


		//////////////////
		// block

		const Result block = this->parse_block(BlockLabelRequirement::OPTIONAL);
		if(this->check_result(block, "statement block in for loop").isError()){ return Result::Code::ERROR; }


		//////////////////
		// done

		return this->source.ast_buffer.createFor(
			keyword, std::move(iterables), index, std::move(values), block.value()
		);
	}


	// TODO(FUTURE): check EOF
	template<bool IS_ERROR_DEFER>
	auto Parser::parse_defer() -> Result {
		const Token::ID start_location = this->reader.peek();

		if constexpr(IS_ERROR_DEFER){
			if(this->assert_token(Token::Kind::KEYWORD_ERROR_DEFER).isError()){ return Result::Code::ERROR; }
		}else{
			if(this->assert_token(Token::Kind::KEYWORD_DEFER).isError()){ return Result::Code::ERROR; }
		}

		const Result block = this->parse_block(BlockLabelRequirement::NOT_ALLOWED);
		if(this->check_result(block, "block in defer").isError()){ return Result::Code::ERROR; }

		return this->source.ast_buffer.createDefer(start_location, block.value());
	}


	auto Parser::parse_try_stmt() -> Result {
		if(this->assert_token(Token::Kind::KEYWORD_TRY).isError()){ return Result::Code::ERROR; }
			
		const Result attempt_expr = this->parse_term<TermKind::EXPR>();
		if(this->check_result(attempt_expr, "attempt expression in try/else statement").isError()){
			return Result::Code::ERROR;
		}

		if(this->reader[this->reader.peek()].kind() == Token::Kind::KEYWORD_ELSE){
			const Token::ID else_token_id = this->reader.next();

			auto except_params = evo::SmallVector<Token::ID>();

			if(this->reader[this->reader.peek()].kind() == Token::lookupKind("<")){
				if(this->assert_token(Token::lookupKind("<")).isError()){ return Result::Code::ERROR; }

				while(true){
					if(this->reader[this->reader.peek()].kind() == Token::lookupKind(">")){
						if(this->assert_token(Token::lookupKind(">")).isError()){ return Result::Code::ERROR; }
						break;
					}


					if(this->reader[this->reader.peek()].kind() == Token::lookupKind("_")){
						except_params.emplace_back(this->reader.next());

					}else{
						const Result ident = this->parse_ident();
						if(this->check_result(ident, "identifier in except parameter block").isError()){
							return Result::Code::ERROR;
						}

						except_params.emplace_back(ASTBuffer::getIdent(ident.value()));
					}

					// check if ending or should continue
					const Token::Kind after_arg_next_token_kind = this->reader[this->reader.next()].kind();
					if(after_arg_next_token_kind != Token::lookupKind(",")){
						if(after_arg_next_token_kind != Token::lookupKind(">")){
							this->expected_but_got(
								"[,] at end of except parameter or [>] at end of except parameter block",
								this->reader.peek(-1)
							);
							return Result::Code::ERROR;
						}

						break;
					}
				}
			}

			const Result except_block = this->parse_block(BlockLabelRequirement::NOT_ALLOWED);
			if(this->check_result(except_block, "except block in try/else statement").isError()){
				return Result::Code::ERROR;
			}


			if(this->expect_token(Token::lookupKind(";"), "at end of try/else statement").isError()){
				return Result::Code::ERROR;
			}


			return this->source.ast_buffer.createTryElse(
				attempt_expr.value(), except_block.value(), std::move(except_params), else_token_id
			);
		}

		this->context.emitError(
			Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
			Diagnostic::Location::get(attempt_expr.value(), this->source),
			"[try] statements without [else] are currently unsupported"
		);
		return Result::Code::ERROR;
	}



	// TODO(FUTURE): check EOF
	auto Parser::parse_assignment() -> Result {
		const Token::ID start_location = this->reader.peek();

		{ // special assignments
			const Token::Kind peeked_kind = this->reader[this->reader.peek()].kind();

			if(peeked_kind == Token::lookupKind("_")){
				const Token::ID discard_token_id = this->reader.next();
				
				const Token::ID op_token_id = this->reader.peek();
				if(this->expect_token(Token::lookupKind("="), "in discard assignment").isError()){
					return Result::Code::ERROR;
				}

				const Result value = this->parse_expr();
				if(this->check_result(value, "expression value in discard assignment").isError()){
					return Result::Code::ERROR;
				}

				if(this->expect_token(Token::lookupKind(";"), "at end of discard assignment").isError()){
					return Result::Code::ERROR;
				}

				return this->source.ast_buffer.createInfix(
					AST::Node(AST::Kind::DISCARD, discard_token_id), op_token_id, value.value()
				);

			}else if(peeked_kind == Token::lookupKind("[")){ // multi assign
				if(this->assert_token(Token::lookupKind("[")).isError()){ return Result::Code::ERROR; }

				auto assignments = evo::SmallVector<AST::Node>();
				while(true){
					if(this->reader[this->reader.peek()].kind() == Token::lookupKind("]")){
						if(this->assert_token(Token::lookupKind("]")).isError()){ return Result::Code::ERROR; }
						break;
					}

					const Result assignment = [&]() -> Result {
						if(this->reader[this->reader.peek()].kind() == Token::lookupKind("_")){
							return AST::Node(AST::Kind::DISCARD, this->reader.next());
						}

						return this->parse_expr();
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
						Diagnostic::Location::get(this->reader.peek(-1), this->source),
						"Multiple-assignment statements cannot assign to 0 values"
					);
				}

				if(this->expect_token(Token::lookupKind("="), "in multiple-assignment").isError()){
					return Result::Code::ERROR;
				}

				const Result value = this->parse_expr();
				if(this->check_result(value, "expression value in multiple-assignment").isError()){
					return Result::Code::ERROR;
				}

				if(this->expect_token(Token::lookupKind(";"), "at end of multiple-assignment").isError()){
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
		if(this->check_result(rhs, "expression value in assignment").isError()){ return Result::Code::ERROR; }

		if(this->expect_token(Token::lookupKind(";"), "at end of assignment").isError()){ return Result::Code::ERROR; }

		return this->source.ast_buffer.createInfix(lhs.value(), op_token_id, rhs.value());
	}




	// TODO(FUTURE): check EOF
	auto Parser::parse_block(BlockLabelRequirement label_requirement) -> Result {
		const Token::ID start_location = this->reader.peek();

		if(this->reader[this->reader.peek()].kind() != Token::lookupKind("{")){ return Result::Code::WRONG_TYPE; }
		if(this->assert_token(Token::lookupKind("{")).isError()){ return Result::Code::ERROR; }

		auto label = std::optional<Token::ID>();
		auto outputs = evo::SmallVector<AST::Block::Output>();

		if(this->reader[this->reader.peek()].kind() == Token::lookupKind("->")){
			if(label_requirement == BlockLabelRequirement::NOT_ALLOWED){
				this->reader.go_back(start_location);
				return Result::Code::WRONG_TYPE;
			}

			if(this->assert_token(Token::lookupKind("->")).isError()){ return Result::Code::ERROR; }

			const Result label_result = this->parse_ident();
			if(this->check_result(label_result, "identifier in block label definition").isError()){
				return Result::Code::ERROR;
			}
			label = ASTBuffer::getIdent(label_result.value());

			if(this->reader[this->reader.peek()].kind() == Token::lookupKind(":")){
				if(label_requirement == BlockLabelRequirement::OPTIONAL){
					this->context.emitError(
						Diagnostic::Code::PARSER_INCORRECT_STMT_CONTINUATION,
						Diagnostic::Location::get(this->reader.peek(), this->source),
						"This labeled block is not allowed to have outputs",
						evo::SmallVector<Diagnostic::Info>{
							Diagnostic::Info("Note: Only expression blocks may have outputs")
						}
					);
					return Result::Code::ERROR;
				}

				if(this->assert_token(Token::lookupKind(":")).isError()){ return Result::Code::ERROR; }

				if(this->reader[this->reader.peek()].kind() == Token::lookupKind("(")){
					const Token::ID open_paren_loc = this->reader.peek();
					if(this->assert_token(Token::lookupKind("(")).isError()){ return Result::Code::ERROR; }

					while(true){
						if(this->reader[this->reader.peek()].kind() == Token::lookupKind(")")){
							if(this->assert_token(Token::lookupKind(")")).isError()){ return Result::Code::ERROR; }
							break;
						}

						const Result ident = this->parse_ident();
						if(this->check_result(ident, "identifier in expression block output parameter").isError()){
							return Result::Code::ERROR;
						}

						if(this->expect_token(
							Token::lookupKind(":"), "after identifier in expression block output parameter"
						).isError()){
							return Result::Code::ERROR;
						}


						const Result type = this->parse_type<TypeKind::EXPLICIT>();
						if(this->check_result(type, "type in expression block output parameter").isError()){
							return Result::Code::ERROR;
						}

						outputs.emplace_back(ASTBuffer::getIdent(ident.value()), type.value());

						// check if ending or should continue
						const Token::Kind after_arg_next_token_kind = this->reader[this->reader.next()].kind();
						if(after_arg_next_token_kind != Token::lookupKind(",")){
							if(after_arg_next_token_kind != Token::lookupKind(")")){
								this->expected_but_got(
									"[,] at end of expression block output parameter"
									" or [)] at end of expression block output parameter block",
									this->reader.peek(-1)
								);
								return Result::Code::ERROR;
							}

							break;
						}
					}


					if(outputs.empty()){
						this->context.emitError(
							Diagnostic::Code::PARSER_BLOCK_EXPR_EMPTY_OUTPUTS_BLOCK,
							Diagnostic::Location::get(open_paren_loc, this->source),
							"Cannot have empty block expression output block"
						);
					}
					
				}else{
					const Result type_result = this->parse_type<TypeKind::EXPLICIT>();
					if(this->check_result(type_result, "type in explicitly-typed labeled expression block").isError()){
						return Result::Code::ERROR;
					}

					outputs.emplace_back(std::nullopt,type_result.value());
				}

			}else if(label_requirement == BlockLabelRequirement::REQUIRED){
				this->expected_but_got(
					"[:] after label in block expression",
					this->reader.peek(),
					evo::SmallVector<Diagnostic::Info>{
						Diagnostic::Info("Note: block expressions must declare the return parameters")
				}
				);
				return Result::Code::ERROR;
			}

		}else if(label_requirement == BlockLabelRequirement::REQUIRED){
			this->reader.go_back(start_location);
			return Result::Code::WRONG_TYPE;
		}

		auto statements = evo::SmallVector<AST::Node>();

		auto end_location = std::optional<Token::ID>();

		while(true){
			if(this->reader[this->reader.peek()].kind() == Token::lookupKind("}")){
				end_location = this->reader.next();
				break;
			}

			const Result stmt = this->parse_stmt();
			if(this->check_result(stmt, "statement in statement block").isError()){ return Result::Code::ERROR; }

			statements.emplace_back(stmt.value());
		}

		return this->source.ast_buffer.createBlock(
			start_location, *end_location, std::move(label), std::move(outputs), std::move(statements)
		);
	}


	// TODO(FUTURE): check EOF
	template<Parser::TypeKind KIND>
	auto Parser::parse_type() -> Result {
		const Token::ID start_location = this->reader.peek();
		bool is_primitive = true;
		bool is_type_deducer = false;
		switch(this->reader[start_location].kind()){
			case Token::Kind::TYPE_VOID:         case Token::Kind::TYPE_THIS:        case Token::Kind::TYPE_INT:
			case Token::Kind::TYPE_ISIZE:        case Token::Kind::TYPE_I_N:         case Token::Kind::TYPE_UINT:
			case Token::Kind::TYPE_USIZE:        case Token::Kind::TYPE_UI_N:        case Token::Kind::TYPE_F16:
			case Token::Kind::TYPE_BF16:         case Token::Kind::TYPE_F32:         case Token::Kind::TYPE_F64:
			case Token::Kind::TYPE_F80:          case Token::Kind::TYPE_F128:        case Token::Kind::TYPE_BYTE:
			case Token::Kind::TYPE_BOOL:         case Token::Kind::TYPE_CHAR:        case Token::Kind::TYPE_RAWPTR:
			case Token::Kind::TYPE_TYPEID:       case Token::Kind::TYPE_C_WCHAR:     case Token::Kind::TYPE_C_SHORT:
			case Token::Kind::TYPE_C_USHORT:     case Token::Kind::TYPE_C_INT:       case Token::Kind::TYPE_C_UINT:
			case Token::Kind::TYPE_C_LONG:       case Token::Kind::TYPE_C_ULONG:     case Token::Kind::TYPE_C_LONG_LONG:
			case Token::Kind::TYPE_C_ULONG_LONG: case Token::Kind::TYPE_C_LONG_DOUBLE:
				break;

			case Token::Kind::KEYWORD_TYPE: {
				is_primitive = false;

				if(this->reader[this->reader.peek(1)].kind() != Token::lookupKind("(")){
					this->expected_but_got(
						"[(] after type ID converter keyword [type]",
						this->reader.peek(1),
						evo::SmallVector<Diagnostic::Info>{Diagnostic::Info("Did you mean `Type`?")}
					);
					return Result::Code::ERROR;
				}
			} break;

			case Token::Kind::TYPE_TYPE: {
				if(this->reader[this->reader.peek(1)].kind() == Token::lookupKind("(")){
					this->context.emitError(
						Diagnostic::Code::PARSER_TYPE_CONVERTER_LOWER_CASE,
						Diagnostic::Location::get(this->reader.peek(1), this->source),
						"Unexpected [(] after `Type`",
						Diagnostic::Info("Did you mean `type`?")
					);
					return Result::Code::ERROR;
				}
			} break;

			case Token::Kind::IDENT: case Token::Kind::INTRINSIC: {
				is_primitive = false;
			} break;

			case Token::Kind::DEDUCER: case Token::Kind::ANONYMOUS_DEDUCER: {
				is_primitive = false;
				is_type_deducer = true;
			} break;

			case Token::lookupKind("["): {
				is_primitive = false;
			} break;

			case Token::Kind::KEYWORD_IMPL: {
				is_primitive = false;
			} break;

			default: return Result::Code::WRONG_TYPE;
		}

		const Result base_type_res = [&]() {
			if(is_primitive){
				const Token::ID base_type_token_id = this->reader.next();

				const Token& peeked_token = this->reader[this->reader.peek()];
				if(peeked_token.kind() == Token::lookupKind(".*")){
					const Diagnostic::Info diagnostics_info = [](){
						if constexpr(
							KIND == TypeKind::AS_TYPE
							|| KIND == TypeKind::TEMPLATE_ARG
							|| KIND == TypeKind::TEMPLATE_ARG_MAYBE_DEDUCER
						){
							return Diagnostic::Info(
								"Did you mean to put parentheses around the preceding [as] operation?"
							);
						}else{
							return Diagnostic::Info("Did you mean pointer ([*]) instead?");
						}
					}();

					this->context.emitError(
						Diagnostic::Code::PARSER_DEREFERENCE_OR_UNWRAP_ON_TYPE,
						Diagnostic::Location::get(this->reader.peek(), this->source),
						"A dereference operator ([.*]) should not follow a type",
						evo::SmallVector<Diagnostic::Info>{diagnostics_info}
					);
					return Result(Result::Code::ERROR);

				}else if(peeked_token.kind() == Token::lookupKind(".?")){
					const Diagnostic::Info diagnostics_info = [](){
						if constexpr(
							KIND == TypeKind::AS_TYPE
							|| KIND == TypeKind::TEMPLATE_ARG
							|| KIND == TypeKind::TEMPLATE_ARG_MAYBE_DEDUCER
						){
							return Diagnostic::Info(
								"Did you mean to put parentheses around the preceding [as] operation?"
							);
						}else{
							return Diagnostic::Info("Did you mean optional ([?]) instead?");
						}
					}();

					this->context.emitError(
						Diagnostic::Code::PARSER_DEREFERENCE_OR_UNWRAP_ON_TYPE,
						Diagnostic::Location::get(this->reader.peek(), this->source),
						"A unwrap operator ([.?]) should not follow a type",
						evo::SmallVector<Diagnostic::Info>{diagnostics_info}
					);
					return Result(Result::Code::ERROR);
				}

				return Result(AST::Node(AST::Kind::PRIMITIVE_TYPE, base_type_token_id));

			}else if(is_type_deducer){
				if constexpr(
					KIND == TypeKind::EXPLICIT_MAYBE_ANONYMOUS_DEDUCER
					|| KIND == TypeKind::TEMPLATE_ARG_MAYBE_ANONYMOUS_DEDUCER
				){
					if(this->reader[this->reader.peek()].kind() == Token::Kind::DEDUCER){
						this->context.emitError(
							Diagnostic::Code::PARSER_DEDUCER_INVALID_IN_THIS_CONTEXT,
							Diagnostic::Location::get(this->reader.peek(), this->source),
							"Named type deducers are not allowed here"
						);
						return Result(Result::Code::ERROR);
					}

					return Result(AST::Node(AST::Kind::DEDUCER, this->reader.next()));

				}else if constexpr(
					KIND == TypeKind::EXPLICIT_MAYBE_DEDUCER || KIND == TypeKind::TEMPLATE_ARG_MAYBE_DEDUCER
				){
					return Result(AST::Node(AST::Kind::DEDUCER, this->reader.next()));

				}else{
					this->context.emitError(
						Diagnostic::Code::PARSER_DEDUCER_INVALID_IN_THIS_CONTEXT,
						Diagnostic::Location::get(this->reader.peek(), this->source),
						"Type deducers are not allowed here"
					);
					return Result(Result::Code::ERROR);
				}

			}else if(this->reader[start_location].kind() == Token::Kind::KEYWORD_TYPE){
				if(this->assert_token(Token::Kind::KEYWORD_TYPE).isError()){ return Result(Result::Code::ERROR); }
				if(this->assert_token(Token::lookupKind("(")).isError()){ return Result(Result::Code::ERROR); }

				const Result type_id_expr = this->parse_expr();
				if(this->check_result(type_id_expr, "expression in TypeID converter").isError()){
					return Result(Result::Code::ERROR);
				}

				if(this->expect_token(Token::lookupKind(")"), "after expression in TypeID converter").isError()){
					return Result(Result::Code::ERROR);
				}

				return Result(this->source.ast_buffer.createTypeIDConverter(start_location, type_id_expr.value()));

			}else if(this->reader[start_location].kind() == Token::lookupKind("[")){
				const Token::ID open_bracket = this->reader.next();

				const Result elem_type = [&](){
					if constexpr(
						KIND == TypeKind::EXPLICIT_MAYBE_DEDUCER || KIND == TypeKind::EXPLICIT_MAYBE_ANONYMOUS_DEDUCER
					){
						return this->parse_type<KIND>();
					}else{
						return this->parse_type<TypeKind::EXPLICIT>();
					}
				}();
				if(this->check_result(elem_type, "element type in array type").isError()){
					return Result(Result::Code::ERROR);
				}

				if(this->expect_token(Token::lookupKind(":"), "after element type in array type").isError()){
					return Result(Result::Code::ERROR);
				}


				bool is_ref = false;
				bool is_mutable_ref = false;
				auto last_ptr_dimension = std::optional<Token::ID>();

				auto dimensions = evo::SmallVector<std::optional<AST::Node>>();

				while(true){
					switch(this->reader[this->reader.peek()].kind()){
						case Token::lookupKind("*"): {
							is_ref = true;
							dimensions.emplace_back(std::nullopt);

							if(last_ptr_dimension.has_value() && is_mutable_ref){
								this->context.emitError(
									Diagnostic::Code::PARSER_ARRAY_REF_MUTABILITY_DOESNT_MATCH,
									Diagnostic::Location::get(
										this->reader.peek(), this->source
									),
									"All pointer dimensions in an array reference must match mutability",
									evo::SmallVector<Diagnostic::Info>{
										Diagnostic::Info(
											"Last pointer dimension was here:",
											Diagnostic::Location::get(
												*last_ptr_dimension, this->source
											)
										)
									}
								);
								return Result(Result::Code::ERROR);
							}
							last_ptr_dimension = this->reader.next();
						} break;

						case Token::lookupKind("*mut"): {
							is_ref = true;
							last_ptr_dimension = this->reader.next();
							dimensions.emplace_back(std::nullopt);
							is_mutable_ref = true;
						} break;

						case Token::Kind::DEDUCER: {
							if constexpr(
								KIND == TypeKind::EXPLICIT_MAYBE_DEDUCER || KIND == TypeKind::TEMPLATE_ARG_MAYBE_DEDUCER
							){
								dimensions.emplace_back(AST::Node(AST::Kind::DEDUCER, this->reader.next()));
							}else{
								this->context.emitError(
									Diagnostic::Code::PARSER_DEDUCER_INVALID_IN_THIS_CONTEXT,
									Diagnostic::Location::get(this->reader.peek(), this->source),
									"Type deducers are not allowed here"
								);
								return Result(Result::Code::ERROR);
							}
						} break;

						case Token::Kind::ANONYMOUS_DEDUCER: {
							if constexpr(
								KIND == TypeKind::EXPLICIT_MAYBE_DEDUCER
								|| KIND == TypeKind::EXPLICIT_MAYBE_ANONYMOUS_DEDUCER
								|| KIND == TypeKind::TEMPLATE_ARG_MAYBE_DEDUCER
							){
								dimensions.emplace_back(AST::Node(AST::Kind::DEDUCER, this->reader.next()));
							}else{
								this->context.emitError(
									Diagnostic::Code::PARSER_DEDUCER_INVALID_IN_THIS_CONTEXT,
									Diagnostic::Location::get(this->reader.peek(), this->source),
									"Anonymous Type deducers are not allowed here"
								);
								return Result(Result::Code::ERROR);
							}
						} break;

						default: {
							const Result length = this->parse_expr();
							if(this->check_result(length, "length(s) in array type").isError()){
								return Result(Result::Code::ERROR);
							}

							dimensions.emplace_back(length.value());
						} break;
					}


					if(this->reader[this->reader.peek()].kind() != Token::lookupKind(",")){
						break;
					}else{
						this->reader.skip();
					}
				}

				
				auto terminator = std::optional<AST::Node>();
				if(this->reader[this->reader.peek()].kind() == Token::lookupKind(";")){
					if(this->assert_token(Token::lookupKind(";")).isError()){ return Result(Result::Code::ERROR); }

					if(
						this->reader[this->reader.peek()].kind() == Token::Kind::DEDUCER
						|| this->reader[this->reader.peek()].kind() == Token::Kind::ANONYMOUS_DEDUCER
					){
						terminator = AST::Node(AST::Kind::DEDUCER, this->reader.next());

					}else{
						const Result terminator_result = this->parse_expr();
						if(this->check_result(terminator_result, "array terminator expression after [;]").isError()){
							return Result(Result::Code::ERROR);
						}

						if(dimensions.size() > 1){
							this->context.emitError(
								Diagnostic::Code::PARSER_MULTI_DIM_ARR_WITH_TERMINATOR,
								Diagnostic::Location::get(terminator_result.value(), this->source),
								"Multi-dimentional array type cannot have a terminator"
							);
							return Result(Result::Code::ERROR);
						}

						terminator = terminator_result.value();
					}
				}


				if(this->expect_token(Token::lookupKind("]"), "at end of array type").isError()){
					return Result(Result::Code::ERROR);
				}

				if(is_ref){
					return Result(
						this->source.ast_buffer.createArrayType(
							open_bracket, elem_type.value(), std::move(dimensions), terminator, is_mutable_ref
						)
					);

				}else{
					return Result(
						this->source.ast_buffer.createArrayType(
							open_bracket, elem_type.value(), std::move(dimensions), terminator, std::nullopt
						)
					);
				}

			}else if(this->reader[start_location].kind() == Token::Kind::KEYWORD_IMPL){
				if(this->assert_token(Token::Kind::KEYWORD_IMPL).isError()){ return Result(Result::Code::ERROR); }

				if(this->expect_token(Token::lookupKind("("), "in interface map").isError()){
					return Result(Result::Code::ERROR);
				}


				auto underlying_type = std::optional<evo::Variant<Token::ID, AST::Node>>();
				switch(this->reader[this->reader.peek()].kind()){
					case Token::lookupKind("*"): case Token::lookupKind("*mut"): {
						underlying_type = this->reader.next();
					} break;

					default: {
						const Result underlying_type_result = [&](){
							if constexpr(
								KIND == TypeKind::EXPLICIT_MAYBE_DEDUCER
								|| KIND == TypeKind::EXPLICIT_MAYBE_ANONYMOUS_DEDUCER
							){
								return this->parse_type<KIND>();
							}else{
								return this->parse_type<TypeKind::EXPLICIT>();
							}
						}();

						if(this->check_result(underlying_type_result, "underlying type in interface map").isError()){
							return Result(Result::Code::ERROR);
						}

						underlying_type = underlying_type_result.value();
					} break;
				}


				const Token::ID colon_token = this->reader.next();
				if(this->reader[colon_token].kind() != Token::lookupKind(":")){
					this->expected_but_got("[:] after underlying type in interface map", colon_token);
					return Result(Result::Code::ERROR);
				}


				const Result target_interface = [&](){
					if constexpr(
						KIND == TypeKind::EXPLICIT_MAYBE_DEDUCER
						|| KIND == TypeKind::EXPLICIT_MAYBE_ANONYMOUS_DEDUCER
					){
						return this->parse_type<KIND>();
					}else{
						return this->parse_type<TypeKind::EXPLICIT>();
					}
				}();

				if(this->check_result(target_interface, "target interface in interface map").isError()){
					return Result(Result::Code::ERROR);
				}


				if(this->expect_token(Token::lookupKind(")"), "at end of interface map").isError()){
					return Result(Result::Code::ERROR);
				}

				return Result(
					this->source.ast_buffer.createInterfaceMap(*underlying_type, colon_token, target_interface.value())
				);

			}else{
				if constexpr(KIND == TypeKind::EXPLICIT){
					return this->parse_term<TermKind::EXPLICIT_TYPE>();

				}else if constexpr(KIND == TypeKind::EXPLICIT_MAYBE_DEDUCER){
					return this->parse_term<TermKind::EXPLICIT_TYPE_MAYBE_DEDUCER>();

				}else if constexpr(KIND == TypeKind::EXPLICIT_MAYBE_ANONYMOUS_DEDUCER){
					return this->parse_term<TermKind::EXPLICIT_TYPE_MAYBE_ANONYMOUS_DEDUCER>();

				}else if constexpr(KIND == TypeKind::AS_TYPE){
					return this->parse_term<TermKind::AS_TYPE>();

				}else if constexpr(KIND == TypeKind::TEMPLATE_ARG){
					return this->parse_term<TermKind::TEMPLATE_ARG>();

				}else if constexpr(KIND == TypeKind::TEMPLATE_ARG_MAYBE_DEDUCER){
					return this->parse_term<TermKind::TEMPLATE_ARG_MAYBE_DEDUCER>();

				}else if constexpr(KIND == TypeKind::TEMPLATE_ARG_MAYBE_ANONYMOUS_DEDUCER){
					return this->parse_term<TermKind::TEMPLATE_ARG_MAYBE_ANONYMOUS_DEDUCER>();

				}else{
					static_assert(false, "Unknown TypeKind");
				}
			}
		}();

		if(base_type_res.code() == Result::Code::ERROR){
			return Result::Code::ERROR;
		}else if(base_type_res.code() == Result::Code::WRONG_TYPE){
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
			bool is_mut = false;
			bool is_optional = false;
			bool is_uninit = false;

			if(this->reader[this->reader.peek()].kind() == Token::lookupKind("*")){
				continue_looking_for_qualifiers = true;
				is_ptr = true;
				potential_backup_location = this->reader.peek();
				if(this->assert_token(Token::lookupKind("*")).isError()){ return Result::Code::ERROR; }
				
				if(this->reader[this->reader.peek()].kind() == Token::lookupKind("!")){
					is_uninit = true;
					potential_backup_location = this->reader.peek();
					if(this->assert_token(Token::lookupKind("!")).isError()){ return Result::Code::ERROR; }
				}

			}else if(this->reader[this->reader.peek()].kind() == Token::lookupKind("*mut")){
				continue_looking_for_qualifiers = true;
				is_ptr = true;
				is_mut = true;
				potential_backup_location = this->reader.peek();
				if(this->assert_token(Token::lookupKind("*mut")).isError()){ return Result::Code::ERROR; }
			}

			if(this->reader[this->reader.peek()].kind() == Token::lookupKind("?")){
				continue_looking_for_qualifiers = true;
				is_optional = true;
				if(this->assert_token(Token::lookupKind("?")).isError()){ return Result::Code::ERROR; }
			}

			if(continue_looking_for_qualifiers){
				qualifiers.emplace_back(is_ptr, is_mut, is_uninit, is_optional);
			}
		}


		if constexpr(KIND == TypeKind::AS_TYPE || KIND == TypeKind::TEMPLATE_ARG){
			// make sure exprs like `a as Int * b` gets parsed like `(a as Int) * b`
			if(qualifiers.empty() == false && qualifiers.back().isOptional == false){
				switch(this->reader[this->reader.peek()].kind()){
					case Token::Kind::IDENT:
					case Token::lookupKind("("):
					case Token::Kind::LITERAL_BOOL:
					case Token::Kind::LITERAL_INT:
					case Token::Kind::LITERAL_FLOAT:
					case Token::Kind::LITERAL_STRING:
					case Token::Kind::LITERAL_CHAR:
					case Token::Kind::KEYWORD_NEW: 
					case Token::lookupKind("{"): {
						qualifiers.pop_back();
						this->reader.go_back(potential_backup_location);
					} break;
				}
			}

			// just an ident
			if constexpr(KIND == TypeKind::TEMPLATE_ARG){
				if(base_type_res.value().kind() == AST::Kind::IDENT && qualifiers.empty()){
					this->reader.go_back(start_location);
					return Result::Code::WRONG_TYPE;
				}
			}
		}

		return this->source.ast_buffer.createType(base_type_res.value(), std::move(qualifiers));
	}



	// TODO(FUTURE): check EOF
	auto Parser::parse_expr() -> Result {
		Result result = this->parse_uninit();
		if(result.code() != Result::Code::WRONG_TYPE){ return result; }

		result = this->parse_zeroinit();
		if(result.code() != Result::Code::WRONG_TYPE){ return result; }

		return this->parse_sub_expr();
	}

	// TODO(FUTURE): check EOF
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

	// TODO(FUTURE): check EOF
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

		if(this->assert_token(peeked_op_kind).isError()){ return Result::Code::ERROR; }

		const Result rhs_result = [&](){
			if(peeked_op_kind == Token::Kind::KEYWORD_AS){
				const Result type_result = this->parse_type<TypeKind::AS_TYPE>();
				if(this->check_result(type_result, "type after [as] operator").isError()){
					return Result(Result::Code::ERROR);
				}

				if(this->reader[this->reader.peek()].kind() == Token::lookupKind(".*")){
					this->context.emitError(
						Diagnostic::Code::PARSER_DEREFERENCE_OR_UNWRAP_ON_TYPE,
						Diagnostic::Location::get(this->reader.peek(), this->source),
						"A dereference operator ([.*]) should not follow a type",
						evo::SmallVector<Diagnostic::Info>{
							Diagnostic::Info("Did you mean to put parentheses around the preceding [as] operation?")
						}
					);
					return Result(Result::Code::ERROR);

				}else if(this->reader[this->reader.peek()].kind() == Token::lookupKind(".?")){
					this->context.emitError(
						Diagnostic::Code::PARSER_DEREFERENCE_OR_UNWRAP_ON_TYPE,
						Diagnostic::Location::get(this->reader.peek(), this->source),
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



	// TODO(FUTURE): check EOF
	auto Parser::parse_prefix_expr() -> Result {
		const Token::ID op_token_id = this->reader.peek();
		const Token::Kind op_token_kind = this->reader[op_token_id].kind();

		switch(op_token_kind){
			case Token::lookupKind("&"):
			case Token::Kind::KEYWORD_COPY:
			case Token::Kind::KEYWORD_MOVE:
			case Token::Kind::KEYWORD_FORWARD:
			case Token::lookupKind("-"):
			case Token::lookupKind("!"):
			case Token::lookupKind("~"):
				break;

			case Token::Kind::KEYWORD_NEW:
				return this->parse_new_expr();

			case Token::Kind::KEYWORD_TRY:
				return this->parse_try_expr();

			default:
				return this->parse_term<TermKind::EXPR>();
		}

		if(this->assert_token(op_token_kind).isError()){ return Result::Code::ERROR; }

		const Result rhs = this->parse_term<TermKind::EXPR>();
		if(this->check_result(
			rhs, std::format("valid sub-expression on right-hand side of prefix [{}] operator", op_token_kind)
		).isError()){
			return Result::Code::ERROR;
		}

		return this->source.ast_buffer.createPrefix(op_token_id, rhs.value());
	}


	auto Parser::parse_new_expr() -> Result {
		const Token::ID keyword_new = this->reader.peek();
		if(this->assert_token(Token::Kind::KEYWORD_NEW).isError()){ return Result::Code::ERROR; }
			
		const Result type = this->parse_type<TypeKind::EXPLICIT>();
		if(this->check_result(type, "type in new expression").isError()){ return Result::Code::ERROR; }

		switch(this->reader[this->reader.peek()].kind()){
			case Token::lookupKind("("): {
				evo::Result<evo::SmallVector<AST::FuncCall::Arg>> args = this->parse_func_call_args();
				if(args.isError()){ return Result::Code::ERROR; }

				return this->source.ast_buffer.createNew(keyword_new, type.value(), std::move(args.value()));
			} break;

			case Token::lookupKind("["): {
				evo::Result<evo::SmallVector<AST::Node>> index_inits = this->parse_array_init();
				if(index_inits.isError()){ return Result::Code::ERROR; }

				return this->source.ast_buffer.createArrayInitNew(
					keyword_new, type.value(), std::move(index_inits.value())
				);
			} break;

			case Token::lookupKind("{"): {
				evo::Result<evo::SmallVector<AST::DesignatedInitNew::MemberInit>> member_inits = 
					this->parse_designated_init();
				if(member_inits.isError()){ return Result::Code::ERROR; }

				return this->source.ast_buffer.createDesignatedInitNew(
					keyword_new, type.value(), std::move(member_inits.value())
				);
			} break;
		}

		this->expected_but_got(
			"function call arguments or struct initializer list after type in operator [new]", this->reader.peek()
		);
		return Result::Code::ERROR;
	}



	auto Parser::parse_try_expr() -> Result {
		if(this->assert_token(Token::Kind::KEYWORD_TRY).isError()){ return Result::Code::ERROR; }
			
		const Result attempt_expr = this->parse_term<TermKind::EXPR>();
		if(this->check_result(attempt_expr, "attempt expression in try/else expression").isError()){
			return Result::Code::ERROR;
		}

		if(this->reader[this->reader.peek()].kind() == Token::Kind::KEYWORD_ELSE){
			const Token::ID else_token_id = this->reader.next();

			auto except_params = evo::SmallVector<Token::ID>();

			if(this->reader[this->reader.peek()].kind() == Token::lookupKind("<")){
				if(this->assert_token(Token::lookupKind("<")).isError()){ return Result::Code::ERROR; }

				while(true){
					if(this->reader[this->reader.peek()].kind() == Token::lookupKind(">")){
						if(this->assert_token(Token::lookupKind(">")).isError()){ return Result::Code::ERROR; }
						break;
					}


					if(this->reader[this->reader.peek()].kind() == Token::lookupKind("_")){
						except_params.emplace_back(this->reader.next());

					}else{
						const Result ident = this->parse_ident();
						if(this->check_result(ident, "identifier in except parameter block").isError()){
							return Result::Code::ERROR;
						}

						except_params.emplace_back(ASTBuffer::getIdent(ident.value()));
					}

					// check if ending or should continue
					const Token::Kind after_arg_next_token_kind = this->reader[this->reader.next()].kind();
					if(after_arg_next_token_kind != Token::lookupKind(",")){
						if(after_arg_next_token_kind != Token::lookupKind(">")){
							this->expected_but_got(
								"[,] at end of except parameter or [>] at end of except parameter block",
								this->reader.peek(-1)
							);
							return Result::Code::ERROR;
						}

						break;
					}
				}
			}

			const Result except_expr = this->parse_expr();
			if(this->check_result(except_expr, "except expression in try/else expression").isError()){
				return Result::Code::ERROR;
			}

			return this->source.ast_buffer.createTryElse(
				attempt_expr.value(), except_expr.value(), std::move(except_params), else_token_id
			);
		}

		this->context.emitError(
			Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
			Diagnostic::Location::get(attempt_expr.value(), this->source),
			"[try] expressions without [else] are currently unsupported"
		);
		return Result::Code::ERROR;
	}



	// TODO(FUTURE): check EOF
	template<Parser::TermKind TERM_KIND>
	auto Parser::parse_term() -> Result {
		Result output = [&](){
			if constexpr(TERM_KIND == TermKind::EXPLICIT_TYPE || TERM_KIND == TermKind::AS_TYPE){
				const Result ident_res = this->parse_ident();
				if(ident_res.code() != Result::Code::WRONG_TYPE){ return ident_res; }
				return this->parse_intrinsic();
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
						if(this->check_result(rhs_result, "identifier or intrinsic after [.] accessor").isError()){
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
							Diagnostic::Location::get(this->reader.peek(), this->source),
							"A dereference operator ([.*]) should not follow a type",
							evo::SmallVector<Diagnostic::Info>{std::move(diagnostics_info)}
						);
						return Result::Code::ERROR;

					}else if constexpr(
						TERM_KIND == TermKind::TEMPLATE_ARG || TERM_KIND == TermKind::TEMPLATE_ARG_MAYBE_DEDUCER
					){
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
							Diagnostic::Location::get(this->reader.peek(), this->source),
							"An unwrap operator ([.?]) should not follow a type",
							evo::SmallVector<Diagnostic::Info>{diagnostics_info}
						);
						return Result::Code::ERROR;

					}else if constexpr(
						TERM_KIND == TermKind::TEMPLATE_ARG || TERM_KIND == TermKind::TEMPLATE_ARG_MAYBE_DEDUCER
					){
						return Result::Code::WRONG_TYPE;

					}else{
						output = this->source.ast_buffer.createPostfix(output.value(), this->reader.next());
					}

				} break;


				case Token::lookupKind("<{"): {
					if(this->assert_token(Token::lookupKind("<{")).isError()){ return Result::Code::ERROR; }
					
					auto args = evo::SmallVector<AST::Node>();

					bool is_first_expr = true;

					while(true){
						if(this->reader[this->reader.peek()].kind() == Token::lookupKind("}>")){
							if(this->assert_token(Token::lookupKind("}>")).isError()){ return Result::Code::ERROR; }
							break;
						}
						
						Result arg = [&](){
							if constexpr(
								TERM_KIND == TermKind::EXPLICIT_TYPE_MAYBE_DEDUCER
								|| TERM_KIND == TermKind::TEMPLATE_ARG_MAYBE_DEDUCER
							){
								return this->parse_type<TypeKind::TEMPLATE_ARG_MAYBE_DEDUCER>();

							}else if constexpr(
								TERM_KIND == TermKind::EXPLICIT_TYPE_MAYBE_ANONYMOUS_DEDUCER
								|| TERM_KIND == TermKind::TEMPLATE_ARG_MAYBE_ANONYMOUS_DEDUCER
							){
								return this->parse_type<TypeKind::TEMPLATE_ARG_MAYBE_ANONYMOUS_DEDUCER>();

							}else{
								return this->parse_type<TypeKind::TEMPLATE_ARG>();
							}
						}();


						if(arg.code() == Result::Code::ERROR){
							return Result::Code::ERROR;

						}else if(arg.code() == Result::Code::WRONG_TYPE){
							arg = this->parse_expr();
							if(this->check_result(arg, "argument inside template pack").isError()){
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

				case Token::lookupKind("["): {
					if constexpr(TERM_KIND == TermKind::EXPLICIT_TYPE || TERM_KIND == TermKind::AS_TYPE){
						should_continue = false;
						break;

					}else{
						const Token::ID open_bracket = this->reader.next();

						auto indices = evo::SmallVector<AST::Node>();
						while(true){
							if(this->reader[this->reader.peek()].kind() == Token::lookupKind("]")){
								if(this->assert_token(Token::lookupKind("]")).isError()){ return Result::Code::ERROR; }
								break;
							}


							const Result index = this->parse_expr();
							if(this->check_result(index, "index inside indexer").isError()){
								return Result::Code::ERROR;
							}

							indices.emplace_back(index.value());

							// check if ending or should continue
							const Token::Kind after_arg_next_token_kind = this->reader[this->reader.next()].kind();
							if(after_arg_next_token_kind != Token::lookupKind(",")){
								if(after_arg_next_token_kind != Token::lookupKind("]")){
									this->expected_but_got(
										"[,] at end of index or []] at end of indexer block",
										this->reader.peek(-1)
									);
									return Result::Code::ERROR;
								}

								break;
							}
						}

						output =
							this->source.ast_buffer.createIndexer(output.value(), std::move(indices), open_bracket);
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

		if(this->expect_token(Token::lookupKind(";"), "after term statement").isError()){ return Result::Code::ERROR; }

		return term;
	}


	// TODO(FUTURE): check EOF
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
			this->expected_but_got(
				"either closing [)] around expression or continuation of sub-expression",
				this->reader.peek(),
				evo::SmallVector<Diagnostic::Info>{
					Diagnostic::Info("parenthesis opened here", Diagnostic::Location::get(open_token_id, this->source)),
				}
			);

			return Result::Code::ERROR;
		}

		if(this->assert_token(Token::lookupKind(")")).isError()){ return Result::Code::ERROR; }

		return inner_expr;
	}

	// TODO(FUTURE): check EOF
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
				if(this->assert_token(Token::lookupKind("(")).isError()){ return Result::Code::ERROR; }

				const Result argument_result = this->parse_expr();
				if(this->check_result(argument_result, "argument in attribute after [(]").isError()){
					return Result::Code::ERROR;
				}
				arguments.emplace_back(argument_result.value());

				if(this->reader[this->reader.peek()].kind() == Token::lookupKind(",")){
					if(this->assert_token(Token::lookupKind(",")).isError()){ return Result::Code::ERROR; }

					const Result argument2_result = this->parse_expr();
					if(this->check_result(argument2_result, "argument in attribute after [,]").isError()){
						return Result::Code::ERROR;
					}
					arguments.emplace_back(argument2_result.value());
				}

				if(this->expect_token(Token::lookupKind(")"), "after attribute argument").isError()){
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
				if(this->assert_token(Token::lookupKind("}>")).isError()){ return Result::Code::ERROR; }
				break;
			}

			const Result ident = this->parse_ident();
			if(ident.code() == Result::Code::ERROR){
				return Result::Code::ERROR;
			}else if(ident.code() == Result::Code::WRONG_TYPE){
				this->expected_but_got("identifier in template parameter", this->reader.peek());
				return Result::Code::ERROR;
			}

			if(this->expect_token(Token::lookupKind(":"), "after template parameter identifier").isError()){
				return Result::Code::ERROR;
			}
			
			const Result type = this->parse_type<TypeKind::EXPLICIT>();
			if(this->check_result(type, "type in template parameter definition").isError()){
				return Result::Code::ERROR;
			}


			auto default_value = std::optional<AST::Node>();
			if(this->reader[this->reader.peek()].kind() == Token::lookupKind("=")){
				if(this->assert_token(Token::lookupKind("=")).isError()){ return Result::Code::ERROR; }

				Result default_value_res = this->parse_type<TypeKind::TEMPLATE_ARG>();
				if(default_value_res.code() == Result::Code::ERROR){
					return Result::Code::ERROR;

				}else if(default_value_res.code() == Result::Code::WRONG_TYPE){
					default_value_res = this->parse_expr();
					if(this->check_result(default_value_res, "default value of template parameter").isError()){
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


	auto Parser::parse_func_params() -> evo::Result<evo::SmallVector<AST::FuncDef::Param>> {
		auto params = evo::SmallVector<AST::FuncDef::Param>();
		if(this->expect_token(Token::lookupKind("("), "to open parameter block in function definition").isError()){
			return evo::resultError;
		}

		bool param_has_default_value = false;
		while(true){
			if(this->reader[this->reader.peek()].kind() == Token::lookupKind(")")){
				if(this->assert_token(Token::lookupKind(")")).isError()){ return evo::resultError; }
				break;
			}

			
			auto param_ident = std::optional<AST::Node>();
			auto param_type = std::optional<AST::Node>();
			using ParamKind = AST::FuncDef::Param::Kind;
			auto param_kind = std::optional<ParamKind>();

			if(this->reader[this->reader.peek()].kind() == Token::Kind::KEYWORD_THIS){
				const Token::ID this_token_id = this->reader.next();
				param_ident = AST::Node(AST::Kind::THIS, this_token_id);

				switch(this->reader[this->reader.peek()].kind()){
					case Token::Kind::KEYWORD_READ: {
						this->reader.skip();
						param_kind = ParamKind::READ;
					} break;

					case Token::Kind::KEYWORD_MUT: {
						this->reader.skip();
						param_kind = ParamKind::MUT;
					} break;

					case Token::Kind::KEYWORD_IN: {
						this->context.emitError(
							Diagnostic::Code::PARSER_INVALID_KIND_FOR_A_THIS_PARAM,
							Diagnostic::Location::get(
								this->reader.peek(), this->source
							),
							"[this] parameters cannot have the kind [in]",
							evo::SmallVector<Diagnostic::Info>{
								Diagnostic::Info("Note: valid kinds are [read] and [mut]"),
							}
						);
						return evo::resultError;
					} break;

					default: {
						param_kind = ParamKind::READ;
					} break;
				}

			}else{
				const Result param_ident_result = this->parse_ident();
				if(this->check_result(param_ident_result, "identifier or [this] in function parameter").isError()){
					return evo::resultError;
				}
				param_ident = param_ident_result.value();

				if(this->expect_token(Token::lookupKind(":"), "after function parameter identifier").isError()){
					return evo::resultError;
				}

				const Result type = this->parse_type<TypeKind::EXPLICIT_MAYBE_DEDUCER>();
				if(this->check_result(type, "type after [:] in function parameter definition").isError()){
					return evo::resultError;
				}
				param_type = type.value();

				switch(this->reader[this->reader.peek()].kind()){
					case Token::Kind::KEYWORD_READ: {
						this->reader.skip();
						param_kind = ParamKind::READ;
					} break;

					case Token::Kind::KEYWORD_MUT: {
						this->reader.skip();
						param_kind = ParamKind::MUT;
					} break;

					case Token::Kind::KEYWORD_IN: {
						this->reader.skip();
						param_kind = ParamKind::IN;
					} break;

					default: {
						param_kind = ParamKind::READ;
					} break;
				}
			}

			const Result attributes = this->parse_attribute_block();
			if(attributes.code() == Result::Code::ERROR){ return evo::resultError; }

			auto default_value = std::optional<AST::Node>();
			if(this->reader[this->reader.peek()].kind() == Token::lookupKind("=")){
				if(this->assert_token(Token::lookupKind("=")).isError()){ return evo::resultError; }

				const Result default_value_res = this->parse_expr();
				if(this->check_result(default_value_res, "function parameter default value").isError()){
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


	template<bool ALLOW_RETURN_DEDUCERS>
	auto Parser::parse_func_returns() -> evo::Result<evo::SmallVector<AST::FuncDef::Return>> {
		auto returns = evo::SmallVector<AST::FuncDef::Return>();

		static constexpr TypeKind RETURN_TYPE_KIND = ALLOW_RETURN_DEDUCERS
			? TypeKind::EXPLICIT_MAYBE_ANONYMOUS_DEDUCER
			: TypeKind::EXPLICIT;
			

		if(this->reader[this->reader.peek()].kind() != Token::lookupKind("(")){
			const Result type = this->parse_type<RETURN_TYPE_KIND>();
			if(this->check_result(type, "Return type in function definition").isError()){ return evo::resultError; }	

			returns.emplace_back(std::nullopt, type.value());

			return returns;
		}


		const Token::ID start_location = this->reader.peek();
		if(this->assert_token(Token::lookupKind("(")).isError()){ return evo::resultError; }

		while(true){
			if(this->reader[this->reader.peek()].kind() == Token::lookupKind(")")){
				if(this->assert_token(Token::lookupKind(")")).isError()){ return evo::resultError; }
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

			if(this->expect_token(Token::lookupKind(":"), "after function return parameter identifier").isError()){
				return evo::resultError;
			}
			
			const Result type = this->parse_type<RETURN_TYPE_KIND>();
			if(this->check_result(type, "type in function return parameter definition").isError()){
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
				Diagnostic::Location::get(start_location, this->source),
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


	auto Parser::parse_func_error_returns() -> evo::Result<evo::SmallVector<AST::FuncDef::Return>> {
		auto error_returns = evo::SmallVector<AST::FuncDef::Return>();

		if(this->reader[this->reader.peek()].kind() != Token::lookupKind("<")){
			return error_returns;
		}

		if(this->assert_token(Token::lookupKind("<")).isError()){ return evo::resultError; }

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
							"the error return parameter block should only contain [Void]"
					),
				}
			);
			return evo::resultError;
		}




		if(this->reader[this->reader.peek(1)].kind() != Token::lookupKind(":")){
			const Result single_type = this->parse_type<TypeKind::EXPLICIT>();
			if(this->check_result(single_type, "single type in function error return parameter block").isError()){
				evo::resultError;
			}
			error_returns.emplace_back(std::nullopt, single_type.value());

			if(this->expect_token(
				Token::lookupKind(">"), "at end of single type in function error return parameter block"
			).isError()){ return evo::resultError; }

			return error_returns;
		}

		while(true){
			if(this->reader[this->reader.peek()].kind() == Token::lookupKind(">")){
				if(this->assert_token(Token::lookupKind(">")).isError()){ return evo::resultError; }
				break;
			}

			const Result ident = this->parse_ident();
			if(this->check_result(ident, "identifier in function error return parameter").isError()){
				return evo::resultError;
			}

			if(this->expect_token(
				Token::lookupKind(":"), "after function error return parameter identifier"
			).isError()){
				return evo::resultError;
			}
			
			const Result type = this->parse_type<TypeKind::EXPLICIT>();
			if(this->check_result(type, "type in function error return parameter definition").isError()){
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
		if(this->assert_token(Token::lookupKind("(")).isError()){ return evo::resultError; }
		
		auto args = evo::SmallVector<AST::FuncCall::Arg>();

		while(true){
			if(this->reader[this->reader.peek()].kind() == Token::lookupKind(")")){
				if(this->assert_token(Token::lookupKind(")")).isError()){ return evo::resultError; }
				break;
			}

			auto arg_ident = std::optional<Token::ID>();

			if(
				this->reader[this->reader.peek()].kind() == Token::Kind::IDENT &&
				this->reader[this->reader.peek(1)].kind() == Token::lookupKind(":")
			){
				arg_ident = this->reader.next();
				if(this->assert_token(Token::lookupKind(":")).isError()){ return evo::resultError; }
			}

			const Result expr_result = this->parse_expr();
			if(this->check_result(expr_result, "expression argument inside function call").isError()){
				return evo::resultError;
			}

			args.emplace_back(arg_ident, expr_result.value());

			// check if ending or should continue
			const Token::Kind after_arg_next_token_kind = this->reader[this->reader.next()].kind();
			if(after_arg_next_token_kind != Token::lookupKind(",")){
				if(after_arg_next_token_kind != Token::lookupKind(")")){
					this->expected_but_got(
						"[,] at end of function call argument or [)] at end of function call argument block",
						this->reader.peek(-1)
					);
					return evo::resultError;
				}

				break;
			}
		}

		return args;
	}


	auto Parser::parse_array_init() -> evo::Result<evo::SmallVector<AST::Node>> {
		if(this->assert_token(Token::lookupKind("[")).isError()){ return evo::resultError; }
		
		auto init_values = evo::SmallVector<AST::Node>();

		while(true){
			if(this->reader[this->reader.peek()].kind() == Token::lookupKind("]")){
				if(this->assert_token(Token::lookupKind("]")).isError()){ return evo::resultError; }
				break;
			}

			const Result expr = this->parse_expr();
			if(this->check_result(expr, "expression in array initializer value").isError()){
				return evo::resultError;
			}

			init_values.emplace_back(expr.value());

			// check if ending or should continue
			const Token::Kind after_arg_next_token_kind = this->reader[this->reader.next()].kind();
			if(after_arg_next_token_kind != Token::lookupKind(",")){
				if(after_arg_next_token_kind != Token::lookupKind("]")){
					this->expected_but_got(
						"[,] at end of array initializer value or []] at end of array initializer",
						this->reader.peek(-1)
					);
					return evo::resultError;
				}

				break;
			}
		}

		return init_values;
	}


	auto Parser::parse_designated_init() -> evo::Result<evo::SmallVector<AST::DesignatedInitNew::MemberInit>> {
		if(this->assert_token(Token::lookupKind("{")).isError()){ return evo::resultError; }
		
		auto init_values = evo::SmallVector<AST::DesignatedInitNew::MemberInit>();

		while(true){
			if(this->reader[this->reader.peek()].kind() == Token::lookupKind("}")){
				if(this->assert_token(Token::lookupKind("}")).isError()){ return evo::resultError; }
				break;
			}


			const Result ident = this->parse_ident();
			if(this->check_result(ident, "identifier in struct initializer value").isError()){
				return evo::resultError;
			}
			
			if(this->expect_token(Token::lookupKind("="), "after identifier in struct initializer value").isError()){
				return evo::resultError;
			}

			const Result expr = this->parse_expr();
			if(this->check_result(expr, "expression after [=] in struct initializer value").isError()){
				return evo::resultError;
			}

			init_values.emplace_back(ASTBuffer::getIdent(ident.value()), expr.value());

			// check if ending or should continue
			const Token::Kind after_arg_next_token_kind = this->reader[this->reader.next()].kind();
			if(after_arg_next_token_kind != Token::lookupKind(",")){
				if(after_arg_next_token_kind != Token::lookupKind("}")){
					this->expected_but_got(
						"[,] at end of struct initializer value"
						" or [}] at end of struct initializer",
						this->reader.peek(-1)
					);
					return evo::resultError;
				}

				break;
			}
		}

		return init_values;
	}


	




	//////////////////////////////////////////////////////////////////////
	// checking

	auto Parser::expected_but_got(
		std::string_view location_str, Token::ID token_id, evo::SmallVector<Diagnostic::Info>&& infos
	) -> void {
		this->context.emitError(
			Diagnostic::Code::PARSER_INCORRECT_STMT_CONTINUATION,
			Diagnostic::Location::get(token_id, this->source),
			std::format("Expected {}, got [{}] instead", location_str, this->reader[token_id].kind()), 
			std::move(infos)
		);
	}


	auto Parser::check_result(const Result& result, std::string_view location_str) -> evo::Result<> {
		switch(result.code()){
			case Result::Code::SUCCESS: {
				return evo::Result<>();
			} break;

			case Result::Code::WRONG_TYPE: {
				this->expected_but_got(location_str, this->reader.peek());
				return evo::resultError;
			} break;

			case Result::Code::ERROR: {
				return evo::resultError;
			} break;
		}

		evo::debugFatalBreak("Unknown or unsupported result code ({})", evo::to_underlying(result.code()));
	}


	auto Parser::assert_token(Token::Kind kind) -> evo::Result<> {
		#if defined(PCIT_CONFIG_DEBUG)
			const Token::ID next_token_id = this->reader.next();

			if(this->reader[next_token_id].kind() == kind){ return evo::Result<>(); }

			this->context.emitFatal(
				Diagnostic::Code::PARSER_ASSUMED_TOKEN_NOT_PRESET,
				Diagnostic::Location::get(next_token_id, this->source),
				Diagnostic::createFatalMessage(
					std::format("Expected [{}], got [{}] instead", kind, this->reader[next_token_id].kind())
				)
			);

			return evo::resultError;

		#else
			this->reader.skip();
			return evo::Result<>();
		#endif
	}

	auto Parser::expect_token(Token::Kind kind, std::string_view location_str) -> evo::Result<> {
		const Token::ID next_token_id = this->reader.next();
		if(this->reader[next_token_id].kind() == kind){ return evo::Result<>(); }

		this->expected_but_got(std::format("[{}] {}", kind, location_str), next_token_id);
		return evo::resultError;
	}


}