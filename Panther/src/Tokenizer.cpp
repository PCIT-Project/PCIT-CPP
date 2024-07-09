//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "./Tokenizer.h"


namespace pcit::panther{
	

	auto Tokenizer::tokenize() noexcept -> bool {
		while(
			this->char_stream.at_end() == false && this->context.hasHitFailCondition() == false && this->can_continue
		){
			this->current_token_line_start = this->char_stream.get_line();
			this->current_token_collumn_start = this->char_stream.get_collumn();

			if(this->tokenize_whitespace()    ){ continue; }
			if(this->tokenize_comment()       ){ continue; }
			if(this->tokenize_identifier()    ){ continue; }
			if(this->tokenize_operators()     ){ continue; }
			if(this->tokenize_punctuation()   ){ continue; }
			if(this->tokenize_number_literal()){ continue; }
			if(this->tokenize_string_literal()){ continue; }
			
			this->error_unrecognized_character();
			return false;
		};

		return this->can_continue;
	};


	auto Tokenizer::tokenize_whitespace() noexcept -> bool {
		if(evo::isWhitespace(this->char_stream.peek())){
			this->char_stream.skip(1);
			return true;
		}
		return false;
	};

	auto Tokenizer::tokenize_comment() noexcept -> bool {
		if(this->char_stream.ammount_left() < 2 || this->char_stream.peek() != '/'){
			return false;
		}

		if(this->char_stream.peek(1) == '/'){ // line comment
			this->char_stream.skip(2);

			while(
				this->char_stream.at_end() == false && 
				this->char_stream.peek() != '\n' && this->char_stream.peek() != '\r'
			){
				this->char_stream.skip(1);
			};

			return true;

		}else if(this->char_stream.peek(1) == '*'){ // multi-line comment
			this->char_stream.skip(2);

			unsigned num_closes_needed = 1;
			while(num_closes_needed > 0){
				if(this->char_stream.ammount_left() < 2){
					this->context.emitError(
						Diagnostic::Code::TokUnterminatedMultilineComment,
						Source::Location(
							this->source.getID(),
							this->current_token_line_start, this->char_stream.get_line(),
							this->current_token_collumn_start, this->char_stream.get_collumn()
						),
						"Unterminated multi-line comment",
						evo::SmallVector<Diagnostic::Info>{
							Diagnostic::Info("Expected a \"*/\" before the end of the file"),
						}
					);

					return true;
				}


				if(this->char_stream.peek() == '/' && this->char_stream.peek(1) == '*'){
					this->char_stream.skip(2);
					num_closes_needed += 1;

				}else if(this->char_stream.peek() == '*' && this->char_stream.peek(1) == '/'){
					this->char_stream.skip(2);
					num_closes_needed -= 1;

				}else{
					this->char_stream.skip(1);
				}
			};

			return true;
		}

		return false;
	};



	const static auto keyword_map = std::unordered_map<std::string_view, Token::Kind>{
		// types
		{"Void", Token::Kind::TypeVoid},
		{"Type", Token::Kind::TypeType},
		{"Int",  Token::Kind::TypeInt},
		{"Bool", Token::Kind::TypeBool},

		// keywords
		{"var",    Token::Kind::KeywordVar},
		{"func",   Token::Kind::KeywordFunc},

		{"null", Token::Kind::KeywordNull},
		{"uninit", Token::Kind::KeywordUninit},

		// {"addr",   Token::Kind::KeywordAddr},
		{"copy",   Token::Kind::KeywordCopy},
		{"move",   Token::Kind::KeywordMove},
		{"as",     Token::Kind::KeywordAs},
	};


	auto Tokenizer::tokenize_identifier() noexcept -> bool {
		auto kind = Token::Kind::None;

		char peeked_char = this->char_stream.peek();
		if(evo::isLetter(peeked_char) || peeked_char == '_'){
			kind = Token::Kind::Ident;

		}else if(
			this->char_stream.ammount_left() >= 2
			&& (evo::isLetter(this->char_stream.peek(1)) || this->char_stream.peek(1) == '_')
		){
			if(this->char_stream.peek() == '@'){
				kind = Token::Kind::Intrinsic;
				this->char_stream.skip(1);

			}else if(this->char_stream.peek() == '#'){
				kind = Token::Kind::Attribute;
				this->char_stream.skip(1);
			}
		}

		if(kind == Token::Kind::None){
			return false;
		}



		const char* string_start_ptr = this->char_stream.peek_raw_ptr();

		std::string_view::size_type token_length = 0;

		do{
			this->char_stream.skip(1);
			token_length += 1;

			if(this->char_stream.at_end()){ break; }

			peeked_char = this->char_stream.peek();
		}while( (evo::isAlphaNumeric(peeked_char) || peeked_char == '_'));

		auto ident_name = std::string_view(string_start_ptr, token_length);

		if(kind == Token::Kind::Ident){
			if(ident_name == "true"){
				this->create_token(Token::Kind::LiteralBool, true);

			}else if(ident_name == "false"){
				this->create_token(Token::Kind::LiteralBool, false);

			}else{
				const auto keyword_map_iter = keyword_map.find(ident_name);

				if(keyword_map_iter == keyword_map.end()){
					this->create_token(Token::Kind::Ident, std::string(ident_name));
				}else{
					this->create_token(keyword_map_iter->second);
				}
			}

		}else{
			this->create_token(kind, std::string(ident_name));
		}
		

		return true;
	};

	auto Tokenizer::tokenize_punctuation() noexcept -> bool {
		const char peeked_char = this->char_stream.peek();
		Token::Kind tok_kind = Token::Kind::None;

		switch(peeked_char){
			break; case '(': tok_kind = Token::Kind::OpenParen;
			break; case ')': tok_kind = Token::Kind::CloseParen;
			break; case '[': tok_kind = Token::Kind::OpenBracket;
			break; case ']': tok_kind = Token::Kind::CloseBracket;
			break; case '{': tok_kind = Token::Kind::OpenBrace;
			break; case '}': tok_kind = Token::Kind::CloseBrace;

			break; case ',': tok_kind = Token::Kind::Comma;
			break; case ';': tok_kind = Token::Kind::SemiColon;
			break; case ':': tok_kind = Token::Kind::Colon;
			break; case '?': tok_kind = Token::Kind::QuestionMark;
		};

		if(tok_kind == Token::Kind::None){ return false; }

		this->char_stream.skip(1);

		this->create_token(tok_kind);

		return true;
	};

	auto Tokenizer::tokenize_operators() noexcept -> bool {
		auto is_op = [&](std::string_view op) noexcept -> bool {
			if(this->char_stream.ammount_left() < op.size()){ return false; }

			for(int i = 0; i < op.size(); i+=1){
				if(this->char_stream.peek(i) != op[i]){
					return false;
				}
			}

			return true;
		};


		auto set_op = [&](std::string_view op){
			this->char_stream.skip(ptrdiff_t(op.size()));
			this->create_token(Token::lookupKind(op.data()));
		};


		// length 4
		if(is_op("<<|=")){ set_op("<<|="); return true; }

		// length 3
		if(is_op("<<|")){ set_op("<<|"); return true; }
		if(is_op("+@=")){ set_op("+@="); return true; }
		if(is_op("+|=")){ set_op("+|="); return true; }
		if(is_op("-@=")){ set_op("-@="); return true; }
		if(is_op("-|=")){ set_op("-|="); return true; }
		if(is_op("*@=")){ set_op("*@="); return true; }
		if(is_op("*|=")){ set_op("*|="); return true; }
		if(is_op("<<=")){ set_op("<<="); return true; }
		if(is_op(">>=")){ set_op(">>="); return true; }


		// length 2
		if(is_op("->")){ set_op("->"); return true; }

		if(is_op("+=")){ set_op("+="); return true; }
		if(is_op("-=")){ set_op("-="); return true; }
		if(is_op("*=")){ set_op("*="); return true; }
		if(is_op("/=")){ set_op("/="); return true; }
		if(is_op("%=")){ set_op("%="); return true; }
		if(is_op("&=")){ set_op("&="); return true; }
		if(is_op("|=")){ set_op("|="); return true; }
		if(is_op("^=")){ set_op("^="); return true; }
		
		if(is_op("+@")){ set_op("+@"); return true; }
		if(is_op("+|")){ set_op("+|"); return true; }
		if(is_op("-@")){ set_op("-@"); return true; }
		if(is_op("-|")){ set_op("-|"); return true; }
		if(is_op("*@")){ set_op("*@"); return true; }
		if(is_op("*|")){ set_op("*|"); return true; }

		if(is_op("==")){ set_op("=="); return true; }
		if(is_op("!=")){ set_op("!="); return true; }
		if(is_op("<=")){ set_op("<="); return true; }
		if(is_op(">=")){ set_op(">="); return true; }

		if(is_op("&&")){ set_op("&&"); return true; }
		if(is_op("||")){ set_op("||"); return true; }

		if(is_op("<<")){ set_op("<<"); return true; }
		if(is_op(">>")){ set_op(">>"); return true; }

		if(is_op(".*")){ set_op(".*"); return true; }
		if(is_op(".?")){ set_op(".?"); return true; }


		// length 1
		if(is_op("=")){ set_op("="); return true; }

		if(is_op("+")){ set_op("+"); return true; }
		if(is_op("-")){ set_op("-"); return true; }
		if(is_op("*")){ set_op("*"); return true; }
		if(is_op("/")){ set_op("/"); return true; }
		if(is_op("%")){ set_op("%"); return true; }

		if(is_op("<")){ set_op("<"); return true; }
		if(is_op(">")){ set_op(">"); return true; }

		if(is_op("!")){ set_op("!"); return true; }

		if(is_op("&")){ set_op("&"); return true; }
		if(is_op("|")){ set_op("|"); return true; }
		if(is_op("^")){ set_op("^"); return true; }
		if(is_op("~")){ set_op("~"); return true; }

		if(is_op(".")){ set_op("."); return true; }

		return false;
	};


	auto Tokenizer::tokenize_number_literal() noexcept -> bool {
		if(evo::isNumber(this->char_stream.peek()) == false){ return false; }

		int base = 10;

		///////////////////////////////////
		// get number prefix

		if(this->char_stream.peek() == '0' && this->char_stream.ammount_left() >= 2){
			const char second_peek = this->char_stream.peek(1);
			if(second_peek == 'x'){
				base = 16;
				this->char_stream.skip(2);

			}else if(second_peek == 'b'){
				base = 2;
				this->char_stream.skip(2);

			}else if(second_peek == 'o'){
				base = 8;
				this->char_stream.skip(2);

			}else if(evo::isNumber(second_peek)){
				this->context.emitError(
					Diagnostic::Code::TokLiteralLeadingZero,
					Source::Location(this->source.getID(), this->char_stream.get_line(), this->char_stream.get_collumn()),
					"Leading zeros in literal numbers are not supported",
					evo::SmallVector<Diagnostic::Info>{
						Diagnostic::Info("Note: the literal integer prefix for base-8 is \"0o\""),
					}
				);

				return true;
			}
		}


		///////////////////////////////////
		// get number

		auto number_string = std::string{};

		bool has_decimal_point = false;

		while(this->char_stream.at_end() == false){
			const char peeked_char = this->char_stream.peek();

			if(peeked_char == '_'){
				this->char_stream.skip(1);
				continue;

			}else if(peeked_char == '.'){
				if(has_decimal_point){
					this->context.emitError(
						Diagnostic::Code::TokLiteralNumMultipleDecimalPoints,
						Source::Location(
							this->source.getID(), this->char_stream.get_line(), this->char_stream.get_collumn()
						),
						"Cannot have multiple decimal points in a floating-point literal"
					);
					return true;
				}

				if(base == 2){
					this->context.emitError(
						Diagnostic::Code::TokInvalidFPBase,
						Source::Location(
							this->source.getID(), this->current_token_line_start, this->current_token_collumn_start
						),
						"Base-2 floating-point literals are not supported"
					);
					return true;

				}else if(base == 8){
					this->context.emitError(
						Diagnostic::Code::TokInvalidFPBase,
						Source::Location(
							this->source.getID(), this->current_token_line_start, this->current_token_collumn_start
						),
						"Base-8 floating-point literals are not supported"
					);
					return true;
				}

				has_decimal_point = true;
				number_string += '.';

				this->char_stream.skip(1);
				continue;
			}


			if(base == 2){
				if(peeked_char == '0' || peeked_char == '1'){
					number_string += this->char_stream.next();

				}else if(evo::isHexNumber(peeked_char)){
					this->context.emitError(
						Diagnostic::Code::TokInvalidNumDigit,
						Source::Location(
							this->source.getID(), this->char_stream.get_line(), this->char_stream.get_collumn()
						),
						"Base-2 numbers should only have digits 0 and 1"
					);
					return true;

				}else{
					break;
				}

			}else if(base == 8){
				if(evo::isOctalNumber(peeked_char)){
					number_string += this->char_stream.next();

				}else if(evo::isHexNumber(peeked_char)){
					this->context.emitError(
						Diagnostic::Code::TokInvalidNumDigit,
						Source::Location(
							this->source.getID(), this->char_stream.get_line(), this->char_stream.get_collumn()
						),
						"Base-8 numbers should only have digits 0-7"
					);
					return true;

				}else{
					break;
				}

			}else if(base == 10){
				if(evo::isNumber(peeked_char)){
					number_string += this->char_stream.next();

				}else if(peeked_char == 'e' || peeked_char == 'E'){
					break;

				}else if(evo::isHexNumber(peeked_char)){
					this->context.emitError(
						Diagnostic::Code::TokInvalidNumDigit,
						Source::Location(
							this->source.getID(), this->char_stream.get_line(), this->char_stream.get_collumn()
						),
						"Base-10 numbers should only have digits 0-9",
						evo::SmallVector<Diagnostic::Info>{
							Diagnostic::Info("Note: The prefix for hexidecimal numbers (base-16) is \"0x\"")
						}
					);
					return true;

				}else{
					break;
				}

			}else{
				// base-16
				if(evo::isHexNumber(peeked_char)){
					number_string += this->char_stream.next();

				}else{
					break;
				}
			}

		};


		///////////////////////////////////
		// get exponent (if it exsits)

		auto exponent_string = std::string{};
		if(
			this->char_stream.ammount_left() >= 2 && 
			(this->char_stream.peek() == 'e' || this->char_stream.peek() == 'E')
		){
			this->char_stream.skip(1);

			if(this->char_stream.peek() == '-' || this->char_stream.peek() == '+'){
				exponent_string += this->char_stream.next();
			}

			while(this->char_stream.at_end() == false){
				const char peeked_char = this->char_stream.peek();

				if(evo::isNumber(peeked_char)){
					exponent_string += this->char_stream.next();

				}else if(evo::isHexNumber(peeked_char)){
					this->context.emitError(
						Diagnostic::Code::TokInvalidNumDigit,
						Source::Location(
							this->source.getID(), this->char_stream.get_line(), this->char_stream.get_collumn()
						),
						"Literal number exponents should only have digits 0-9"
					);
					return true;

				}else{
					break;
				}
			};
		}



		///////////////////////////////////
		// parse exponent (if it exists)

		int64_t exponent_number = 1;

		if(exponent_string.size() != 0){
			exponent_number = std::strtoll(exponent_string.data(), nullptr, base);

			if(exponent_number == ULLONG_MAX && errno == ERANGE){
				this->context.emitError(
					Diagnostic::Code::TokLiteralNumTooBig,
					Source::Location(
						this->source.getID(),
						this->current_token_line_start, this->char_stream.get_line(),
						this->current_token_collumn_start, this->char_stream.get_collumn() - 1
					),
					"Literal number exponent too large to fit into a I64."
					"This limitation will be removed when the compiler is self hosted."
				);
				return true;

			}else if(exponent_number == 0){
				for(const char& character : exponent_string){
					if(character != '0'){
						this->context.emitFatal(
							Diagnostic::Code::TokUnknownFailureToTokenizeNum,
							Source::Location(
								this->source.getID(),
								this->current_token_line_start, this->char_stream.get_line(),
								this->current_token_collumn_start, this->char_stream.get_collumn() - 1
							),
							"Tried to convert invalid integer string for exponent"
						);
						return true;
					}
				}
			}
		}


		///////////////////////////////////
		// check exponent isn't too large

		if(exponent_number != 0 && exponent_number != 1){
			const float64_t floating_point_exponent_number = float64_t(exponent_number);

			if(has_decimal_point){
				const static float64_t max_float_exp = std::log10(std::numeric_limits<float64_t>::max()) + 1;

				if(floating_point_exponent_number > max_float_exp){
					this->context.emitError(
						Diagnostic::Code::TokLiteralNumTooBig,
						Source::Location(
							this->source.getID(),
							this->current_token_line_start, this->char_stream.get_line(),
							this->current_token_collumn_start, this->char_stream.get_collumn() - 1
						),
						"Literal floating-point number too large to fit into an F64"
					);
					return true;
				}

			}else{
				const static float64_t max_int_exp = std::log10(std::numeric_limits<uint64_t>::max()) + 1;

				if(floating_point_exponent_number > max_int_exp){
					this->context.emitError(
						Diagnostic::Code::TokLiteralNumTooBig,
						Source::Location(
							this->source.getID(),
							this->current_token_line_start, this->char_stream.get_line(),
							this->current_token_collumn_start, this->char_stream.get_collumn() - 1
						),
						"Literal number integer too large to fit into a UI64. "
						"This limitation will be removed when the compiler is self hosted."
					);
					return true;
				}
			}
		}



		///////////////////////////////////
		// parse / save number (with some checking)

		if(has_decimal_point){
			if(base == 16){
				number_string = "0x" + number_string;
			}

			char* str_end;
			const float64_t parsed_number = std::strtod(number_string.data(), &str_end);

			if(parsed_number == HUGE_VALL){
				this->context.emitError(
					Diagnostic::Code::TokLiteralNumTooBig,
					Source::Location(
						this->source.getID(),
						this->current_token_line_start, this->char_stream.get_line(),
						this->current_token_collumn_start, this->char_stream.get_collumn() - 1
					),
					"Literal floating-point too large to fit into an F64"
				);
				return true;

			}else if(parsed_number == 0.0L && str_end == number_string.data()){
				this->context.emitFatal(
					Diagnostic::Code::TokUnknownFailureToTokenizeNum,
					Source::Location(
						this->source.getID(),
						this->current_token_line_start, this->char_stream.get_line(),
						this->current_token_collumn_start, this->char_stream.get_collumn() - 1
					),
					"Tried to convert invalid literal floating-point number"
				);
				return true;
			}

			if(
				parsed_number == 0.0 && 
				std::numeric_limits<float64_t>::max() / parsed_number < std::pow(10, exponent_number)
			){
				this->context.emitError(
					Diagnostic::Code::TokLiteralNumTooBig,
					Source::Location(
						this->source.getID(),
						this->current_token_line_start, this->char_stream.get_line(),
						this->current_token_collumn_start, this->char_stream.get_collumn() - 1
					),
					"Literal number integer too large to fit into an F64."
				);
				return true;
			}


			float64_t output_number = parsed_number;
			     if(exponent_number == 0){ output_number = 0; }
			else if(exponent_number != 1){ output_number *= std::pow(10, exponent_number); }

			this->create_token(Token::Kind::LiteralFloat, output_number);


		}else{
			const uint64_t parsed_number = std::strtoull(number_string.data(), nullptr, base);

			if(parsed_number == ULLONG_MAX && errno == ERANGE){
				this->context.emitError(
					Diagnostic::Code::TokLiteralNumTooBig,
					Source::Location(
						this->source.getID(),
						this->current_token_line_start, this->char_stream.get_line(),
						this->current_token_collumn_start, this->char_stream.get_collumn() - 1
					),
					"Literal integer too large to fit into a UI64. "
					"This limitation will be removed when the compiler is self hosted."
				);
				return true;

			}else if(parsed_number == 0){
				for(const char& character : number_string){
					if(character != '0'){
						this->context.emitFatal(
							Diagnostic::Code::TokUnknownFailureToTokenizeNum,
							Source::Location(
								this->source.getID(),
								this->current_token_line_start, this->char_stream.get_line(),
								this->current_token_collumn_start, this->char_stream.get_collumn() - 1
							),
							"Tried to convert invalid literal integer"
						);
						return true;
					}
				}
			}


			uint64_t output_number = parsed_number;
			     if(exponent_number == 0){ output_number = 0; }
			else if(exponent_number != 1){ output_number *= uint64_t(std::pow(10, exponent_number)); }

			this->create_token(Token::Kind::LiteralInt, output_number);
		}

		return true;
	};


	auto Tokenizer::tokenize_string_literal() noexcept -> bool {
		if(this->char_stream.peek() != '"' && this->char_stream.peek() != '\''){ return false; }

		const char delimiter = this->char_stream.next();

		auto literal_value = std::string();

		while(this->char_stream.peek() != delimiter){
			bool unexpected_at_end = false;

			if(this->char_stream.at_end()){
				unexpected_at_end = true;

			}else if(this->char_stream.peek() == '\\'){
				switch(this->char_stream.peek(1)){
					break; case '0': literal_value += '\0';
					break; case 'a': literal_value += '\a';
					break; case 'b': literal_value += '\b';
					break; case 't': literal_value += '\t';
					break; case 'n': literal_value += '\n';
					break; case 'v': literal_value += '\v';
					break; case 'f': literal_value += '\f';
					break; case 'r': literal_value += '\r';

					break; case '\'': literal_value += '\'';
					break; case '"':  literal_value += '"';
					break; case '\\': literal_value += '\\';

					break; default: {
						this->context.emitError(
							Diagnostic::Code::TokUnterminatedTextEscapeSequence,
							Source::Location(
								this->source.getID(),
								this->char_stream.get_line(), this->char_stream.get_line(),
								this->char_stream.get_collumn(), this->char_stream.get_collumn() + 1
							),
							std::format("Unknown string escape code '\\{}'", this->char_stream.peek(1))
						);
						return true;
					}
				};

				this->char_stream.skip(2);

			}else{
				literal_value += this->char_stream.next();
			}

			// needed because some code above may have called next() or skip()
			if(this->char_stream.at_end()){
				unexpected_at_end = true;
			}

			if(unexpected_at_end){
				const char* string_type_name = [&]() noexcept {
					if(delimiter == '"'){ return "string"; }
					if(delimiter == '\''){ return "character"; }
					evo::debugFatalBreak("Unknown delimiter");
				}();

				this->context.emitError(
					Diagnostic::Code::TokUnterminatedMultilineComment,
					Source::Location(
						this->source.getID(),
						this->current_token_line_start, this->char_stream.get_line(),
						this->current_token_collumn_start, this->char_stream.get_collumn()
					),
					std::format("Unterminated {} literal", string_type_name),
					evo::SmallVector<Diagnostic::Info>{
						Diagnostic::Info(std::format("Expected a {} before the end of the file", delimiter)),
					}
				);
				return true;	
			}

		};


		this->char_stream.skip(1);

		if(delimiter == '\''){
			this->create_token(Token::Kind::LiteralChar, std::move(literal_value));
		}else{
			this->create_token(Token::Kind::LiteralString, std::move(literal_value));
		}


		return true;
	};


	//////////////////////////////////////////////////////////////////////
	// create tokens


	auto Tokenizer::create_token(Token::Kind kind) noexcept -> void {
		if(this->file_too_big()){ return; }

		this->source.token_buffer.createToken(
			kind,
			Token::Location(
				this->current_token_line_start,
				this->char_stream.get_line(),
				this->current_token_collumn_start,
				this->char_stream.get_collumn() - 1
			)
		);
	};


	auto Tokenizer::create_token(Token::Kind kind, auto&& val) noexcept -> void {
		if(this->file_too_big()){ return; }

		this->source.token_buffer.createToken(
			kind,
			Token::Location(
				this->current_token_line_start,
				this->char_stream.get_line(),
				this->current_token_collumn_start,
				this->char_stream.get_collumn() - 1
			),
			std::forward<decltype(val)>(val)
		);
	};



	//////////////////////////////////////////////////////////////////////
	// errors

	auto Tokenizer::file_too_big() noexcept -> bool {
		constexpr static size_t MAX_TOKENS = std::numeric_limits<uint32_t>::max();

		if(this->source.token_buffer.size() >= MAX_TOKENS){
			this->context.emitError(
				Diagnostic::Code::TokFileTooLarge,
				Source::Location(
					this->source.getID(),
					this->current_token_line_start, this->char_stream.get_line(),
					this->current_token_collumn_start, this->char_stream.get_collumn() - 1
				),
				"File too large",
				evo::SmallVector<Diagnostic::Info>{
					Diagnostic::Info(std::format("Source files can have a maximum of {} tokens", MAX_TOKENS)),
				}
			);

			this->can_continue = false;

			return true;
		}

		return false;
	};



	EVO_NODISCARD static constexpr auto hex_from_4_bits(char num) noexcept -> char {
		switch(num){
			case 0: return '0';
			case 1: return '1';
			case 2: return '2';
			case 3: return '3';
			case 4: return '4';
			case 5: return '5';
			case 6: return '6';
			case 7: return '7';
			case 8: return '8';
			case 9: return '9';
			case 10: return 'A';
			case 11: return 'B';
			case 12: return 'C';
			case 13: return 'D';
			case 14: return 'E';
			case 15: return 'F';
			default: evo::debugFatalBreak("Not valid num (must be 4 bits)");
		};
	};


	auto Tokenizer::error_unrecognized_character() noexcept -> void {
		const char peeked_char = this->char_stream.peek();

		if(peeked_char >= 0){
			this->context.emitError(
				Diagnostic::Code::TokUnrecognizedCharacter,
				Source::Location(
					this->source.getID(),
					this->char_stream.get_line(), this->char_stream.get_line(),
					this->char_stream.get_collumn(), this->char_stream.get_collumn()
				),
				std::format(
					"Unrecognized or unexpected character \"{}\" (charcode: {})",
					evo::printCharName(peeked_char),
					int(peeked_char)
				)
			);
			return;
		}


		// detect utf-8
		// https://en.wikipedia.org/wiki/UTF-8

		auto utf8_str = evo::StaticString<4>();

		const size_t num_chars_of_utf8 = std::countl_one(static_cast<unsigned char>(this->char_stream.peek()));

		if(num_chars_of_utf8 > 4 || this->char_stream.ammount_left() < num_chars_of_utf8){
			this->context.emitError(
				Diagnostic::Code::TokUnrecognizedCharacter,
				Source::Location(
					this->source.getID(),
					this->char_stream.get_line(), this->char_stream.get_line(),
					this->char_stream.get_collumn(), this->char_stream.get_collumn()
				),
				std::format(
					"Unrecognized character (non-standard utf-8 character)",
					evo::printCharName(peeked_char),
					int(peeked_char)
				)
			);
			return;
		}

		
		for(size_t i = 0; i < num_chars_of_utf8; i+=1){
			utf8_str.push_back(this->char_stream.peek(i));
		}

		auto utf8_charcodes_str = evo::StaticString<8>("U+");
		switch(num_chars_of_utf8){
			case 2: {
				utf8_charcodes_str.push_back('0');

				char charcode = utf8_str[0] >> 2;
				charcode &= 0b0111;
				utf8_charcodes_str.push_back(hex_from_4_bits(charcode));

				charcode = utf8_str[0] & 0b11;
				charcode <<= 2;
				charcode |= (utf8_str[1] >> 4) & 0b0011;
				utf8_charcodes_str.push_back(hex_from_4_bits(charcode));

				charcode = utf8_str[1] & 0b1111;
				utf8_charcodes_str.push_back(hex_from_4_bits(charcode));
			} break;

			case 3: {
				char charcode = utf8_str[0] & 0b1111;
				utf8_charcodes_str.push_back(hex_from_4_bits(charcode));

				charcode = utf8_str[1] >> 2;
				charcode &= 0b1111;
				utf8_charcodes_str.push_back(hex_from_4_bits(charcode));

				charcode = utf8_str[1] & 0b11;
				charcode <<= 2;
				charcode |= (utf8_str[2] >> 4) & 0b0011;
				utf8_charcodes_str.push_back(hex_from_4_bits(charcode));

				charcode = utf8_str[2] & 0b1111;
				utf8_charcodes_str.push_back(hex_from_4_bits(charcode));
			} break;


			case 4: {
				char charcode = utf8_str[0] >> 2;
				charcode &= 0b1;
				utf8_charcodes_str.push_back(hex_from_4_bits(charcode));

				charcode = utf8_str[0] & 0b11;
				charcode <<= 2;
				charcode |= (utf8_str[1] >> 4) & 0b0011;
				utf8_charcodes_str.push_back(hex_from_4_bits(charcode));

				charcode = utf8_str[1] & 0b1111;
				utf8_charcodes_str.push_back(hex_from_4_bits(charcode));

				charcode = utf8_str[2] >> 2;
				charcode &= 0b1111;
				utf8_charcodes_str.push_back(hex_from_4_bits(charcode));

				charcode = utf8_str[2] & 0b11;
				charcode <<= 2;
				charcode |= (utf8_str[3] >> 4) & 0b0011;
				utf8_charcodes_str.push_back(hex_from_4_bits(charcode));

				charcode = utf8_str[3] & 0b1111;
				utf8_charcodes_str.push_back(hex_from_4_bits(charcode));
			} break;
		};

		this->context.emitError(
			Diagnostic::Code::TokUnrecognizedCharacter,
			Source::Location(this->source.getID(), this->char_stream.get_line(), this->char_stream.get_collumn()),
			std::format("Unrecognized character \"{}\" (UTF-8 code: {})", utf8_str, utf8_charcodes_str)
		);
	};


};