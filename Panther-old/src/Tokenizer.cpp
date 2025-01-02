////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./Tokenizer.h"


namespace pcit::panther{


	enum class StrToNumError{
		OutOfRange,
		Invalid,
	};

	template<class NumericType>
	EVO_NODISCARD auto str_to_num(std::string_view str, int base) 
	-> evo::Expected<NumericType, StrToNumError> requires(std::is_integral_v<NumericType>) {
		NumericType result;
		auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result, base);

		if(ptr != str.data() + str.size()){ return evo::Unexpected(StrToNumError::Invalid); }

		if(ec == std::errc())                         { return result; }
		else if(ec == std::errc::result_out_of_range) { return evo::Unexpected(StrToNumError::OutOfRange); }
		else if(ec == std::errc::invalid_argument)    { return evo::Unexpected(StrToNumError::Invalid);    }
		else                                          { evo::debugFatalBreak("Unknown error"); }
	}

	template<class NumericType>
	EVO_NODISCARD auto str_to_num(std::string_view str, int base) 
	-> evo::Expected<NumericType, StrToNumError> requires(std::is_floating_point_v<NumericType>) {
		const std::chars_format fmt = [&]() {
			switch(base){
				case 10: return std::chars_format::general;
				case 16: return std::chars_format::hex;
				default: evo::debugFatalBreak("Unsupported floating-point base");
			}
		}();

		NumericType result;
		auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result, fmt);

		if(ptr != str.data() + str.size()){ return evo::Unexpected(StrToNumError::Invalid); }

		if(ec == std::errc())                         { return result; }
		else if(ec == std::errc::result_out_of_range) { return evo::Unexpected(StrToNumError::OutOfRange); }
		else if(ec == std::errc::invalid_argument)    { return evo::Unexpected(StrToNumError::Invalid);    }
		else                                          { evo::debugFatalBreak("Unknown error"); }
	}

	

	auto Tokenizer::tokenize() -> bool {
		while(
			this->char_stream.at_end() == false && this->context.hasHitFailCondition() == false && this->can_continue
		){
			const evo::Result<uint32_t> line_result = this->char_stream.get_line();
			if(line_result.isError()){ this->error_line_too_big(); return false; }

			const evo::Result<uint32_t> collumn_result = this->char_stream.get_collumn();
			if(collumn_result.isError()){ this->error_collumn_too_big(); return false; }

			this->current_token_line_start = line_result.value();
			this->current_token_collumn_start = collumn_result.value();


			if(this->tokenize_whitespace()    ){ continue; }
			if(this->tokenize_comment()       ){ continue; }
			if(this->tokenize_identifier()    ){ continue; }
			if(this->tokenize_operators()     ){ continue; }
			if(this->tokenize_punctuation()   ){ continue; }
			if(this->tokenize_number_literal()){ continue; }
			if(this->tokenize_string_literal()){ continue; }
			
			this->error_unrecognized_character();
			return false;
		}

		return this->can_continue;
	}


	auto Tokenizer::tokenize_whitespace() -> bool {
		if(evo::isWhitespace(this->char_stream.peek())){
			this->char_stream.skip(1);
			return true;
		}
		return false;
	}

	auto Tokenizer::tokenize_comment() -> bool {
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
			}

			return true;

		}else if(this->char_stream.peek(1) == '*'){ // multi-line comment
			this->char_stream.skip(2);

			unsigned num_closes_needed = 1;
			while(num_closes_needed > 0){
				if(this->char_stream.ammount_left() < 2){
					const evo::Result<Source::Location> current_location = this->get_current_location_token();
					if(current_location.isError()){ return true; }

					this->emit_error(
						Diagnostic::Code::TokUnterminatedMultilineComment,
						current_location.value(),
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
			}

			return true;
		}

		return false;
	}


	// TODO: change to not use global initialization
	const static auto keyword_map = std::unordered_map<std::string_view, Token::Kind>{
		// types
		{"Void",        Token::Kind::TypeVoid},
		{"Type",        Token::Kind::TypeType},
		{"This",        Token::Kind::TypeThis},

		{"Int",         Token::Kind::TypeInt},
		{"ISize",       Token::Kind::TypeISize},

		{"UInt",        Token::Kind::TypeUInt},
		{"USize",       Token::Kind::TypeUSize},

		{"F16",         Token::Kind::TypeF16},
		{"BF16",        Token::Kind::TypeBF16},
		{"F32",         Token::Kind::TypeF32},
		{"F64",         Token::Kind::TypeF64},
		{"F80",         Token::Kind::TypeF80},
		{"F128",        Token::Kind::TypeF128},

		{"Byte",        Token::Kind::TypeByte},
		{"Bool",        Token::Kind::TypeBool},
		{"Char",        Token::Kind::TypeChar},
		{"RawPtr",      Token::Kind::TypeRawPtr},
		{"TypeID",      Token::Kind::TypeTypeID},

		{"CShort",      Token::Kind::TypeCShort},
		{"CUShort",     Token::Kind::TypeCUShort},
		{"CInt",        Token::Kind::TypeCInt},
		{"CUInt",       Token::Kind::TypeCUInt},
		{"CLong",       Token::Kind::TypeCLong},
		{"CULong",      Token::Kind::TypeCULong},
		{"CLongLong",   Token::Kind::TypeCLongLong},
		{"CULongLong",  Token::Kind::TypeCULongLong},
		{"CLongDouble", Token::Kind::TypeCLongDouble},


		// keywords
		{"var",         Token::Kind::KeywordVar},
		{"const",       Token::Kind::KeywordConst},
		{"def",         Token::Kind::KeywordDef},
		{"func",        Token::Kind::KeywordFunc},
		{"alias",       Token::Kind::KeywordAlias},
		{"type",        Token::Kind::KeywordType},
		{"struct",      Token::Kind::KeywordStruct},

		{"return",      Token::Kind::KeywordReturn},
		{"unreachable", Token::Kind::KeywordUnreachable},

		{"null",        Token::Kind::KeywordNull},
		{"uninit",      Token::Kind::KeywordUninit},
		{"zeroinit",    Token::Kind::KeywordZeroinit},
		{"this",        Token::Kind::KeywordThis},

		{"read",        Token::Kind::KeywordRead},
		{"mut",         Token::Kind::KeywordMut},
		{"in",          Token::Kind::KeywordIn},

		{"copy",        Token::Kind::KeywordCopy},
		{"move",        Token::Kind::KeywordMove},
		{"new",         Token::Kind::KeywordNew},
		{"as",          Token::Kind::KeywordAs},

		{"if",          Token::Kind::KeywordIf},
		{"else",        Token::Kind::KeywordElse},
		{"when",        Token::Kind::KeywordWhen},
		{"while",       Token::Kind::KeywordWhile},

		// discard
		{"_", Token::lookupKind("_")},
	};

	const static auto keyword_end = keyword_map.end();


	auto Tokenizer::tokenize_identifier() -> bool {
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
			}else{
				return false;
			}
		}else{
			return false;	
		}


		const char* string_start_ptr = this->char_stream.peek_raw_ptr();

		do{
			this->char_stream.skip(1);

			if(this->char_stream.at_end()){ break; }

			peeked_char = this->char_stream.peek();
		}while(evo::isAlphaNumeric(peeked_char) || peeked_char == '_');

		auto ident_name = std::string_view(string_start_ptr, this->char_stream.peek_raw_ptr() - string_start_ptr);

		if(kind == Token::Kind::Ident){
			if(ident_name == "true") [[unlikely]] {
				this->create_token(Token::Kind::LiteralBool, true);

			}else if(ident_name == "false") [[unlikely]] {
				this->create_token(Token::Kind::LiteralBool, false);

			}else{
				bool is_integer = false;

				auto parse_integer = [&](Token::Kind kind, size_t bitwidth_start_index) -> void {
					const std::string_view bitwidth_str = ident_name.substr(bitwidth_start_index);
					
					for(char character : bitwidth_str){
						if(evo::isNumber(character) == false){
							return;
						}
					}

					is_integer = true;

					const evo::Expected<uint32_t, StrToNumError> bitwidth = str_to_num<uint32_t>(bitwidth_str, 10);

					if(bitwidth.has_value()){
						if(bitwidth.value() > std::pow(2, 23)){
							const evo::Result<Source::Location> current_location = this->get_current_location_token();
							if(current_location.isError()){ return; }

							this->emit_error(
								Diagnostic::Code::TokInvalidIntegerWidth,
								current_location.value(),
								"Integer bit-width is too large",
								evo::SmallVector<Diagnostic::Info>{
									Diagnostic::Info("Maximum bitwidth is 2^23 (8,388,608)")
								}
							);

						}else if(bitwidth.value() == 0){
							const evo::Result<Source::Location> current_location = this->get_current_location_token();
							if(current_location.isError()){ return; }

							this->emit_error(
								Diagnostic::Code::TokInvalidIntegerWidth,
								current_location.value(),
								"Integer bit-width cannot be 0"
							);
						}

						this->create_token(kind, uint64_t(bitwidth.value()));
						return;
					}

					switch(bitwidth.error()){
						case StrToNumError::OutOfRange: {
							const evo::Result<Source::Location> current_location = this->get_current_location_token();
							if(current_location.isError()){ return; }

							this->emit_error(
								Diagnostic::Code::TokInvalidIntegerWidth,
								current_location.value(),
								"Integer bit-width is too large",
								evo::SmallVector<Diagnostic::Info>{
									Diagnostic::Info("Maximum bitwidth is 2^23 (8,388,608)")
								}
							);
						} break;

						case StrToNumError::Invalid: {
							const evo::Result<Source::Location> current_location = this->get_current_location_token();
							if(current_location.isError()){ return; }

							this->emit_fatal(
								Diagnostic::Code::TokUnknownFailureToTokenizeNum,
								current_location.value(),
								Diagnostic::createFatalMessage("Attempted to tokenize invalid integer bit-width")
							);
						} break;
					}
				};

				if(ident_name.size() > 1){
					if(ident_name[0] == 'I'){
						parse_integer(Token::Kind::TypeI_N, 1);
					}else if(ident_name.size() > 2 && ident_name[0] == 'U' && ident_name[1] == 'I'){
						parse_integer(Token::Kind::TypeUI_N, 2);
					}
				}

				if(this->can_continue == false){ return false; }

				if(is_integer == false){
					const auto keyword_map_iter = keyword_map.find(ident_name);

					if(keyword_map_iter == keyword_end){
						this->create_token(Token::Kind::Ident, ident_name);

					}else{
						this->create_token(keyword_map_iter->second);
					}
				}
			}

		}else{
			this->create_token(kind, ident_name);
		}
		

		return true;
	}

	auto Tokenizer::tokenize_punctuation() -> bool {
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
		}

		if(tok_kind == Token::Kind::None){ return false; }

		this->char_stream.skip(1);

		this->create_token(tok_kind);

		return true;
	}

	// TODO: improve the perf of this by having separate lookups based on the ammount left
		//   (no need to check length 3 ops if it's known to only have 1 char left)
	auto Tokenizer::tokenize_operators() -> bool {
		const size_t ammount_left = this->char_stream.ammount_left();

		switch(this->char_stream.peek()){
			case '=': {
				if(ammount_left > 1 && this->char_stream.peek(1) == '='){
					this->char_stream.skip(evo::stringSize("=="));
					this->create_token(Token::lookupKind("=="));
					return true;
				}else{
					this->char_stream.skip(evo::stringSize("="));
					this->create_token(Token::lookupKind("="));
					return true;
				}
			} break;

			case '+': {
				if(ammount_left > 1){
					switch(this->char_stream.peek(1)){
						case '=': {
							this->char_stream.skip(evo::stringSize("+="));
							this->create_token(Token::lookupKind("+="));
							return true;
						} break;

						case '%': {
							if(this->char_stream.peek(2) == '='){
								this->char_stream.skip(evo::stringSize("+%="));
								this->create_token(Token::lookupKind("+%="));
								return true;
							}else{
								this->char_stream.skip(evo::stringSize("+%"));
								this->create_token(Token::lookupKind("+%"));
								return true;
							}
						} break;

						case '|': {
							if(this->char_stream.peek(2) == '='){
								this->char_stream.skip(evo::stringSize("+|="));
								this->create_token(Token::lookupKind("+|="));
								return true;
							}else{
								this->char_stream.skip(evo::stringSize("+|"));
								this->create_token(Token::lookupKind("+|"));
								return true;
							}
						} break;
					}
				}

				this->char_stream.skip(evo::stringSize("+"));
				this->create_token(Token::lookupKind("+"));
				return true;
			} break;

			case '-': {
				if(ammount_left > 1){
					switch(this->char_stream.peek(1)){
						case '>': {
							this->char_stream.skip(evo::stringSize("->"));
							this->create_token(Token::lookupKind("->"));
							return true;
						} break;

						case '=': {
							this->char_stream.skip(evo::stringSize("-="));
							this->create_token(Token::lookupKind("-="));
							return true;
						} break;

						case '%': {
							if(this->char_stream.peek(2) == '='){
								this->char_stream.skip(evo::stringSize("-%="));
								this->create_token(Token::lookupKind("-%="));
								return true;
							}else{
								this->char_stream.skip(evo::stringSize("-%"));
								this->create_token(Token::lookupKind("-%"));
								return true;
							}
						} break;

						case '|': {
							if(this->char_stream.peek(2) == '='){
								this->char_stream.skip(evo::stringSize("-|="));
								this->create_token(Token::lookupKind("-|="));
								return true;
							}else{
								this->char_stream.skip(evo::stringSize("-|"));
								this->create_token(Token::lookupKind("-|"));
								return true;
							}
						} break;

					}
				}

				this->char_stream.skip(evo::stringSize("-"));
				this->create_token(Token::lookupKind("-"));
				return true;
			} break;

			case '*': {
				if(ammount_left > 1){
					switch(this->char_stream.peek(1)){
						case '=': {
							this->char_stream.skip(evo::stringSize("*="));
							this->create_token(Token::lookupKind("*="));
							return true;
						} break;

						case '%': {
							if(this->char_stream.peek(2) == '='){
								this->char_stream.skip(evo::stringSize("*%="));
								this->create_token(Token::lookupKind("*%="));
								return true;
							}else{
								this->char_stream.skip(evo::stringSize("*%"));
								this->create_token(Token::lookupKind("*%"));
								return true;
							}
						} break;

						case '|': {
							if(this->char_stream.peek(2) == '='){
								this->char_stream.skip(evo::stringSize("*|="));
								this->create_token(Token::lookupKind("*|="));
								return true;
							}else{
								this->char_stream.skip(evo::stringSize("*|"));
								this->create_token(Token::lookupKind("*|"));
								return true;
							}
						} break;
					}
				}

				this->char_stream.skip(evo::stringSize("*"));
				this->create_token(Token::lookupKind("*"));
				return true;
			} break;


			case '/': {
				if(ammount_left > 1 && this->char_stream.peek(1) == '='){
					this->char_stream.skip(evo::stringSize("/="));
					this->create_token(Token::lookupKind("/="));
					return true;
				}else{
					this->char_stream.skip(evo::stringSize("/"));
					this->create_token(Token::lookupKind("/"));
					return true;
				}
			} break;

			case '%': {
				if(ammount_left > 1 && this->char_stream.peek(1) == '='){
					this->char_stream.skip(evo::stringSize("%="));
					this->create_token(Token::lookupKind("%="));
					return true;
				}else{
					this->char_stream.skip(evo::stringSize("%"));
					this->create_token(Token::lookupKind("%"));
					return true;
				}
			} break;

			case '<': {
				if(ammount_left > 1){
					switch(this->char_stream.peek(1)){
						case '<': {
							if(ammount_left > 2){
								switch(this->char_stream.peek(2)){
									case '=': {
										this->char_stream.skip(evo::stringSize("<<="));
										this->create_token(Token::lookupKind("<<="));
										return true;
									} break;

									case '|': {
										if(ammount_left > 3 && this->char_stream.peek(3) == '='){
											this->char_stream.skip(evo::stringSize("<<|="));
											this->create_token(Token::lookupKind("<<|="));
											return true;
										}else{
											this->char_stream.skip(evo::stringSize("<<|"));
											this->create_token(Token::lookupKind("<<|"));
											return true;
										}
									} break;
								}

								this->char_stream.skip(evo::stringSize("<<"));
								this->create_token(Token::lookupKind("<<"));
								return true;
							}
						} break;

						case '=': {
							this->char_stream.skip(evo::stringSize("<="));
							this->create_token(Token::lookupKind("<="));
							return true;
						} break;

						case '{': {
							this->char_stream.skip(evo::stringSize("<{"));
							this->create_token(Token::lookupKind("<{"));
							return true;
						} break;
					}
				}

				this->char_stream.skip(evo::stringSize("<"));
				this->create_token(Token::lookupKind("<"));
				return true;
			} break;

			case '>': {
				if(ammount_left > 1){
					switch(this->char_stream.peek(1)){
						case '>': {
							if(ammount_left > 2 && this->char_stream.peek(2) == '='){
								this->char_stream.skip(evo::stringSize(">>="));
								this->create_token(Token::lookupKind(">>="));
								return true;
							}else{
								this->char_stream.skip(evo::stringSize(">>"));
								this->create_token(Token::lookupKind(">>"));
								return true;
							}
						} break;

						case '=': {
							this->char_stream.skip(evo::stringSize(">="));
							this->create_token(Token::lookupKind(">="));
							return true;
						} break;
					}
				}

				this->char_stream.skip(evo::stringSize(">"));
				this->create_token(Token::lookupKind(">"));
				return true;
			} break;

			case '&': {
				if(ammount_left > 1){
					switch(this->char_stream.peek(1)){
						case '=': {
							this->char_stream.skip(evo::stringSize("&="));
							this->create_token(Token::lookupKind("&="));
							return true;
						} break;

						case '&': {
							this->char_stream.skip(evo::stringSize("&&"));
							this->create_token(Token::lookupKind("&&"));
							return true;
						} break;

						case '|': {
							this->char_stream.skip(evo::stringSize("&|"));
							this->create_token(Token::lookupKind("&|"));
							return true;
						} break;
					}
				}

				this->char_stream.skip(evo::stringSize("&"));
				this->create_token(Token::lookupKind("&"));
				return true;
			} break;

			case '|': {
				if(ammount_left > 1){
					switch(this->char_stream.peek(1)){
						case '=': {
							this->char_stream.skip(evo::stringSize("|="));
							this->create_token(Token::lookupKind("|="));
							return true;
						} break;

						case '|': {
							this->char_stream.skip(evo::stringSize("||"));
							this->create_token(Token::lookupKind("||"));
							return true;
						} break;
					}
				}

				this->char_stream.skip(evo::stringSize("|"));
				this->create_token(Token::lookupKind("|"));
				return true;
			} break;

			case '^': {
				if(ammount_left > 1 && this->char_stream.peek(1) == '='){
					this->char_stream.skip(evo::stringSize("^="));
					this->create_token(Token::lookupKind("^="));
					return true;
				}else{
					this->char_stream.skip(evo::stringSize("^"));
					this->create_token(Token::lookupKind("^"));
					return true;
				}
			} break;

			case '!': {
				if(ammount_left > 1 && this->char_stream.peek(1) == '='){
					this->char_stream.skip(evo::stringSize("!="));
					this->create_token(Token::lookupKind("!="));
					return true;
				}else{
					this->char_stream.skip(evo::stringSize("!"));
					this->create_token(Token::lookupKind("!"));
					return true;
				}
			} break;

			case '.': {
				if(ammount_left > 1){
					switch(this->char_stream.peek(1)){
						case '*': {
							this->char_stream.skip(evo::stringSize(".*"));
							this->create_token(Token::lookupKind(".*"));
							return true;
						} break;

						case '?': {
							this->char_stream.skip(evo::stringSize(".?"));
							this->create_token(Token::lookupKind(".?"));
							return true;
						} break;

						case '.': {
							if(ammount_left > 2 && this->char_stream.peek(2) == '.'){
								this->char_stream.skip(evo::stringSize("..."));
								this->create_token(Token::lookupKind("..."));
								return true;	
							}
						} break;
					}
				}

				this->char_stream.skip(evo::stringSize("."));
				this->create_token(Token::lookupKind("."));
				return true;
			} break;

			case '~': {
				this->char_stream.skip(evo::stringSize("~"));
				this->create_token(Token::lookupKind("~"));
				return true;
			} break;

			case '}': {
				if(ammount_left > 1 && this->char_stream.peek(1) == '>'){
					this->char_stream.skip(evo::stringSize("}>"));
					this->create_token(Token::lookupKind("}>"));
					return true;
				}
			} break;
		}
		
		return false;
	}


	auto Tokenizer::tokenize_number_literal() -> bool {
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
				const evo::Result<Source::Location> current_location = this->get_current_location_point();
				if(current_location.isError()){ return true; }

				this->emit_error(
					Diagnostic::Code::TokLiteralLeadingZero,
					current_location.value(),
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

		auto number_string = std::string();

		bool has_decimal_point = false;

		while(this->char_stream.at_end() == false){
			const char peeked_char = this->char_stream.peek();

			if(peeked_char == '_'){
				this->char_stream.skip(1);
				continue;

			}else if(peeked_char == '.'){
				if(has_decimal_point){
					const evo::Result<Source::Location> current_location = this->get_current_location_point();
					if(current_location.isError()){ return true; }

					this->emit_error(
						Diagnostic::Code::TokLiteralNumMultipleDecimalPoints,
						current_location.value(),
						"Cannot have multiple decimal points in a floating-point literal"
					);
					return true;
				}

				if(base == 2){
					this->emit_error(
						Diagnostic::Code::TokInvalidFPBase,
						Source::Location(
							this->source.getID(), this->current_token_line_start, this->current_token_collumn_start
						),
						"Base-2 floating-point literals are not supported"
					);
					return true;

				}else if(base == 8){
					this->emit_error(
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
					const evo::Result<Source::Location> current_location = this->get_current_location_point();
					if(current_location.isError()){ return true; }

					this->emit_error(
						Diagnostic::Code::TokInvalidNumDigit,
						current_location.value(),
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
					const evo::Result<Source::Location> current_location = this->get_current_location_point();
					if(current_location.isError()){ return true; }

					this->emit_error(
						Diagnostic::Code::TokInvalidNumDigit,
						current_location.value(),
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
					const evo::Result<Source::Location> current_location = this->get_current_location_point();
					if(current_location.isError()){ return true; }

					this->emit_error(
						Diagnostic::Code::TokInvalidNumDigit,
						current_location.value(),
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

		}


		///////////////////////////////////
		// get exponent (if it exsits)

		auto exponent_string = std::string();
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
					const evo::Result<Source::Location> current_location = this->get_current_location_token();
					if(current_location.isError()){ return true; }

					this->emit_error(
						Diagnostic::Code::TokInvalidNumDigit,
						current_location.value(),
						"Literal number exponents should only have digits 0-9"
					);
					return true;

				}else{
					break;
				}
			}
		}



		///////////////////////////////////
		// parse exponent (if it exists)

		int64_t exponent_number = 1;

		if(exponent_string.size() != 0){
			const evo::Expected<int64_t, StrToNumError> converted_exponent_number = 
				str_to_num<int64_t>(exponent_string, base);

			if(converted_exponent_number.has_value()){
				exponent_number = converted_exponent_number.value();	
			}else{
				switch(converted_exponent_number.error()){
					case StrToNumError::OutOfRange: {
						const evo::Result<Source::Location> current_location = this->get_current_location_token();
						if(current_location.isError()){ return true; }

						this->emit_error(
							Diagnostic::Code::TokLiteralNumTooBig,
							current_location.value(),
							"Literal number exponent too large to fit into a I64."
							"This limitation will be removed when the compiler is self hosted."
						);
						return true;
					} break;

					case StrToNumError::Invalid: {
						const evo::Result<Source::Location> current_location = this->get_current_location_token();
						if(current_location.isError()){ return true; }

						this->emit_fatal(
							Diagnostic::Code::TokUnknownFailureToTokenizeNum,
							current_location.value(),
							Diagnostic::createFatalMessage("Tried to convert invalid integer string for exponent")
						);
					} break;
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
					const evo::Result<Source::Location> current_location = this->get_current_location_token();
					if(current_location.isError()){ return true; }

					this->emit_error(
						Diagnostic::Code::TokLiteralNumTooBig,
						current_location.value(),
						"Literal floating-point number too large to fit into an F64"
					);
					return true;
				}

			}else{
				const static float64_t max_int_exp = std::log10(std::numeric_limits<uint64_t>::max()) + 1;

				if(floating_point_exponent_number > max_int_exp){
					const evo::Result<Source::Location> current_location = this->get_current_location_token();
					if(current_location.isError()){ return true; }

					this->emit_error(
						Diagnostic::Code::TokLiteralNumTooBig,
						current_location.value(),
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
			const evo::Expected<float64_t, StrToNumError> converted_parsed_number = 
				str_to_num<float64_t>(number_string, base);

			if(converted_parsed_number.has_value() == false){
				switch(converted_parsed_number.error()){
					case StrToNumError::OutOfRange: {
						const evo::Result<Source::Location> current_location = this->get_current_location_token();
						if(current_location.isError()){ return true; }

						this->emit_error(
							Diagnostic::Code::TokLiteralNumTooBig,
							current_location.value(),
							"Literal floating-point too large to fit into an F64"
						);
						return true;
					} break;

					case StrToNumError::Invalid: {
						const evo::Result<Source::Location> current_location = this->get_current_location_token();
						if(current_location.isError()){ return true; }

						this->emit_fatal(
							Diagnostic::Code::TokUnknownFailureToTokenizeNum,
							current_location.value(),
							Diagnostic::createFatalMessage("Tried to convert invalid literal floating-point number")
						);
					} break;
				}
			}

			const float64_t parsed_number = converted_parsed_number.value();


			if(
				parsed_number == 0.0 && 
				std::numeric_limits<float64_t>::max() / parsed_number < std::pow(10, exponent_number)
			){
				const evo::Result<Source::Location> current_location = this->get_current_location_token();
				if(current_location.isError()){ return true; }

				this->emit_error(
					Diagnostic::Code::TokLiteralNumTooBig,
					current_location.value(),
					"Literal number integer too large to fit into an F64."
				);
				return true;
			}


			float64_t output_number = parsed_number;
			     if(exponent_number == 0){ output_number = 0; }
			else if(exponent_number != 1){ output_number *= std::pow(10, exponent_number); }

			this->create_token(Token::Kind::LiteralFloat, output_number);


		}else{
			const evo::Expected<uint64_t, StrToNumError> converted_parsed_number = 
				str_to_num<uint64_t>(number_string, base);

			if(converted_parsed_number.has_value() == false){
				switch(converted_parsed_number.error()){
					case StrToNumError::OutOfRange: {
						const evo::Result<Source::Location> current_location = this->get_current_location_token();
						if(current_location.isError()){ return true; }

						this->emit_error(
							Diagnostic::Code::TokLiteralNumTooBig,
							current_location.value(),
							"Literal integer too large to fit into a UI64. "
							"This limitation will be removed when the compiler is self hosted."
						);
						return true;
					} break;

					case StrToNumError::Invalid: {
						const evo::Result<Source::Location> current_location = this->get_current_location_token();
						if(current_location.isError()){ return true; }

						this->emit_fatal(
							Diagnostic::Code::TokUnknownFailureToTokenizeNum,
							current_location.value(),
							Diagnostic::createFatalMessage("Tried to convert invalid literal integer")
						);
						return true;
					} break;
				}
			}


			uint64_t output_number = converted_parsed_number.value();
			     if(exponent_number == 0){ output_number = 0; }
			else if(exponent_number != 1){ output_number *= uint64_t(std::pow(10, exponent_number)); }

			this->create_token(Token::Kind::LiteralInt, output_number);
		}

		return true;
	}


	auto Tokenizer::tokenize_string_literal() -> bool {
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
						const evo::Result<Source::Location> current_location = this->get_current_location_token();
						if(current_location.isError()){ return true; }

						this->emit_error(
							Diagnostic::Code::TokUnterminatedTextEscapeSequence,
							current_location.value(),
							std::format("Unknown string escape code '\\{}'", this->char_stream.peek(1))
						);
						return true;
					}
				}

				this->char_stream.skip(2);

			}else{
				literal_value += this->char_stream.next();
			}

			// needed because some code above may have called next() or skip()
			if(this->char_stream.at_end()){
				unexpected_at_end = true;
			}

			if(unexpected_at_end){
				const char* string_type_name = [&]() {
					if(delimiter == '"'){ return "string"; }
					if(delimiter == '\''){ return "character"; }
					evo::debugFatalBreak("Unknown delimiter");
				}();

				const evo::Result<uint32_t> line_result = this->char_stream.get_line();
				if(line_result.isError()){ this->error_line_too_big(); return true; }

				const evo::Result<uint32_t> collumn_result = this->char_stream.get_collumn();
				if(collumn_result.isError()){ this->error_collumn_too_big(); return true; }

				this->emit_error(
					Diagnostic::Code::TokUnterminatedMultilineComment,
					Source::Location(
						this->source.getID(),
						this->current_token_line_start, line_result.value(),
						this->current_token_collumn_start, collumn_result.value()
					),
					std::format("Unterminated {} literal", string_type_name),
					evo::SmallVector<Diagnostic::Info>{
						Diagnostic::Info(std::format("Expected a {} before the end of the file", delimiter)),
					}
				);
				return true;	
			}

		}


		this->char_stream.skip(1);

		if(delimiter == '\''){
			if(literal_value.empty()){
				const evo::Result<Source::Location> current_location = this->get_current_location_token();
				if(current_location.isError()){ return true; }

				this->emit_error(
					Diagnostic::Code::TokInvalidChar,
					current_location.value(),
					"Literal character cannot be empty"
				);
				return true;

			}else if(literal_value.size() > 1){
				const evo::Result<Source::Location> current_location = this->get_current_location_token();
				if(current_location.isError()){ return true; }

				this->emit_error(
					Diagnostic::Code::TokInvalidChar,
					current_location.value(),
					"Literal character must be only 1 character"
				);
				return true;
			}

			this->create_token(Token::Kind::LiteralChar, literal_value);
		}else{
			this->create_token(Token::Kind::LiteralString, literal_value);
		}


		return true;
	}


	//////////////////////////////////////////////////////////////////////
	// create tokens


	auto Tokenizer::create_token(Token::Kind kind, auto&&... val) -> void {
		if(this->file_too_big()){ return; }

		const evo::Result<uint32_t> line_result = this->char_stream.get_line();
		if(line_result.isError()){ this->error_line_too_big(); return; }

		const evo::Result<uint32_t> collumn_result = this->char_stream.get_collumn();
		if(collumn_result.isError()){ this->error_collumn_too_big(); return; }

		this->source.token_buffer.createToken(
			kind,
			Token::Location(
				this->current_token_line_start,
				line_result.value(),
				this->current_token_collumn_start,
				collumn_result.value() - 1
			),
			std::forward<decltype(val)>(val)...
		);
	}



	//////////////////////////////////////////////////////////////////////
	// errors

	auto Tokenizer::file_too_big() -> bool {
		constexpr static size_t MAX_TOKENS = std::numeric_limits<uint32_t>::max() - 1;

		if(this->source.token_buffer.size() < MAX_TOKENS){ return false; }


		const evo::Result<Source::Location> current_location = this->get_current_location_token();
		if(current_location.isError()){ return true; }

		this->emit_error(
			Diagnostic::Code::TokFileTooLarge,
			current_location.value(),
			"File too large",
			evo::SmallVector<Diagnostic::Info>{
				Diagnostic::Info(std::format("Source files can have a maximum of {} (2^32-2) tokens", MAX_TOKENS)),
			}
		);

		this->can_continue = false;

		return true;
	}


	auto Tokenizer::emit_warning(auto&&... args) -> void {
		this->context.emitWarning(std::forward<decltype(args)>(args)...);
	}

	auto Tokenizer::emit_error(auto&&... args) -> void {
		this->context.emitError(std::forward<decltype(args)>(args)...);
		this->can_continue = false;
	}

	auto Tokenizer::emit_fatal(auto&&... args) -> void {
		this->context.emitFatal(std::forward<decltype(args)>(args)...);
		this->can_continue = false;
	}



	EVO_NODISCARD static constexpr auto hex_from_4_bits(char num) -> char {
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
		}
	}


	auto Tokenizer::error_unrecognized_character() -> void {
		const char peeked_char = this->char_stream.peek();

		if(peeked_char >= 0){
			const evo::Result<Source::Location> current_location = this->get_current_location_point();
			if(current_location.isError()){ return; }

			this->emit_error(
				Diagnostic::Code::TokUnrecognizedCharacter,
				current_location.value(),
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
			const evo::Result<Source::Location> current_location = this->get_current_location_point();
			if(current_location.isError()){ return; }

			this->emit_error(
				Diagnostic::Code::TokUnrecognizedCharacter,
				current_location.value(),
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
		}

		const evo::Result<Source::Location> current_location = this->get_current_location_point();
		if(current_location.isError()){ return; }

		this->emit_error(
			Diagnostic::Code::TokUnrecognizedCharacter,
			current_location.value(),
			std::format("Unrecognized character \"{}\" (UTF-8 code: {})", utf8_str, utf8_charcodes_str)
		);
	}



	auto Tokenizer::error_line_too_big() -> void {
		this->emit_error(
			Diagnostic::Code::TokFileLocationLimitOOB,
			Source::Location(
				this->source.getID(), this->current_token_line_start, this->current_token_collumn_start
			),
			"Line number is too large",
			evo::SmallVector<Diagnostic::Info>{
				Diagnostic::Info(
					std::format("Maximum line number is: {}", std::numeric_limits<uint32_t>::max())
				),
				Diagnostic::Info("Note: given source location pointing to the beginning of the previous token")
			}
		);
	}


	auto Tokenizer::error_collumn_too_big() -> void {
		this->emit_error(
			Diagnostic::Code::TokFileLocationLimitOOB,
			Source::Location(
				this->source.getID(), this->current_token_line_start, this->current_token_collumn_start
			),
			"Collumn number is too large",
			evo::SmallVector<Diagnostic::Info>{
				Diagnostic::Info(
					std::format("Maximum collumn number is: {}", std::numeric_limits<uint32_t>::max())
				),
				Diagnostic::Info("Note: given source location pointing to the beginning of the previous token")
			}
		);
	}



	auto Tokenizer::get_current_location_point() -> evo::Result<Source::Location> {
		const evo::Result<uint32_t> line_result = this->char_stream.get_line();
		if(line_result.isError()){ this->error_line_too_big(); return evo::resultError; }

		const evo::Result<uint32_t> collumn_result = this->char_stream.get_collumn();
		if(collumn_result.isError()){ this->error_collumn_too_big(); return evo::resultError; }

		return Source::Location(this->source.getID(), line_result.value(), collumn_result.value());
	}

	auto Tokenizer::get_current_location_token() -> evo::Result<Source::Location> {
		const evo::Result<uint32_t> line_result = this->char_stream.get_line();
		if(line_result.isError()){ this->error_line_too_big(); return evo::resultError; }

		const evo::Result<uint32_t> collumn_result = this->char_stream.get_collumn();
		if(collumn_result.isError()){ this->error_collumn_too_big(); return evo::resultError; }

		return Source::Location(
			this->source.getID(),
			this->current_token_line_start, line_result.value(),
			this->current_token_collumn_start, collumn_result.value() - 1
		);
	}


}