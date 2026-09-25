////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./Tokenizer.hpp"


namespace pcit::panther{


	enum class StrToNumError{
		OUT_OF_RANGE,
		INVALID,
	};

	template<class NumericType>
	[[nodiscard]] auto str_to_num(std::string_view str, int base) 
	-> evo::Expected<NumericType, StrToNumError> requires(std::is_integral_v<NumericType>) {
		NumericType result;
		auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result, base);

		if(ptr != str.data() + str.size()){ return evo::Unexpected(StrToNumError::INVALID); }

		if(ec == std::errc())                         { return result; }
		else if(ec == std::errc::result_out_of_range) { return evo::Unexpected(StrToNumError::OUT_OF_RANGE); }
		else if(ec == std::errc::invalid_argument)    { return evo::Unexpected(StrToNumError::INVALID);    }
		else                                          { evo::debugFatalBreak("Unknown error"); }
	}

	template<class NumericType>
	[[nodiscard]] auto str_to_num(std::string_view str, int base) 
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

		if(ptr != str.data() + str.size()){ return evo::Unexpected(StrToNumError::INVALID); }

		if(ec == std::errc())                         { return result; }
		else if(ec == std::errc::result_out_of_range) { return evo::Unexpected(StrToNumError::OUT_OF_RANGE); }
		else if(ec == std::errc::invalid_argument)    { return evo::Unexpected(StrToNumError::INVALID);    }
		else                                          { evo::debugFatalBreak("Unknown error"); }
	}

	
	auto Tokenizer::tokenize() -> evo::Result<> {
		auto timer = this->context.getTimers().tokenization.start();
		EVO_DEFER([&](){ timer.stop(); });

		EVO_DEFER([&](){ this->source.token_buffer.lock(); });

		while(
			this->char_stream.at_end() == false && this->context.hasHitFailCondition() == false && this->can_continue
		){
			if(this->token_start().isError()){ return evo::resultError; }
		}

		return evo::Result<>::fromBool(this->can_continue);
	}


	auto Tokenizer::token_start() -> evo::Result<> {
		const evo::Result<uint32_t> line_result = this->char_stream.get_line();
		if(line_result.isError()){ this->error_line_too_big(); return evo::resultError; }

		const evo::Result<uint32_t> collumn_result = this->char_stream.get_collumn();
		if(collumn_result.isError()){ this->error_collumn_too_big(); return evo::resultError; }

		this->current_token_line_start = line_result.value();
		this->current_token_collumn_start = collumn_result.value();


		switch(this->char_stream.peek()){
			case ' ': case '\n': case '\r': case '\t': {
				this->char_stream.skip(1);
				return evo::Result<>();
			} break;

			case '!': {
				if(this->char_stream.ammount_left() > 2 && this->char_stream.peek(1) == '='){
					this->char_stream.skip(evo::stringSize("!="));
					this->create_token(Token::lookupKind("!="));
					return evo::Result<>();
				}else{
					this->char_stream.skip(evo::stringSize("!"));
					this->create_token(Token::lookupKind("!"));
					return evo::Result<>();
				}
			} break;

			case '\"': {
				return this->tokenize_string_literal();
			} break;

			case '#': {
				this->char_stream.skip(1);
				return this->tokenize_identifier(Token::Kind::ATTRIBUTE);
			} break;

			case '$': {
				if(this->char_stream.ammount_left() == 1){
					this->error_unrecognized_character();
					return evo::resultError;
				}

				if(this->char_stream.peek(1) == '$'){
					this->char_stream.skip(evo::stringSize("$$"));
					this->create_token(Token::lookupKind("$$"));
					return evo::Result<>();
				}

				this->char_stream.skip(1);
				return this->tokenize_identifier(Token::Kind::DEDUCER);
			} break;

			case '%': {
				if(this->char_stream.ammount_left() > 2 && this->char_stream.peek(1) == '='){
					this->char_stream.skip(evo::stringSize("%="));
					this->create_token(Token::lookupKind("%="));
					return evo::Result<>();
				}else{
					this->char_stream.skip(evo::stringSize("%"));
					this->create_token(Token::lookupKind("%"));
					return evo::Result<>();
				}
			} break;

			case '&': {
				const size_t ammount_left = this->char_stream.ammount_left();

				if(ammount_left > 1){
					switch(this->char_stream.peek(1)){
						case '=': {
							this->char_stream.skip(evo::stringSize("&="));
							this->create_token(Token::lookupKind("&="));
							return evo::Result<>();
						} break;

						case '&': {
							this->char_stream.skip(evo::stringSize("&&"));
							this->create_token(Token::lookupKind("&&"));
							return evo::Result<>();
						} break;
					}
				}

				this->char_stream.skip(evo::stringSize("&"));
				this->create_token(Token::lookupKind("&"));
				return evo::Result<>();
			} break;

			case '\'': {
				return this->tokenize_string_literal();
			} break;

			case '(': {
				this->char_stream.skip(evo::stringSize("("));
				this->create_token(Token::lookupKind("("));
				return evo::Result<>();
			} break;

			case ')': {
				this->char_stream.skip(evo::stringSize(")"));
				this->create_token(Token::lookupKind(")"));
				return evo::Result<>();
			} break;

			case '*': {
				const size_t ammount_left = this->char_stream.ammount_left();

				if(ammount_left > 1){
					switch(this->char_stream.peek(1)){
						case '=': {
							this->char_stream.skip(evo::stringSize("*="));
							this->create_token(Token::lookupKind("*="));
							return evo::Result<>();
						} break;

						case '%': {
							if(ammount_left > 2 && this->char_stream.peek(2) == '='){
								this->char_stream.skip(evo::stringSize("*%="));
								this->create_token(Token::lookupKind("*%="));
								return evo::Result<>();
							}else{
								this->char_stream.skip(evo::stringSize("*%"));
								this->create_token(Token::lookupKind("*%"));
								return evo::Result<>();
							}
						} break;

						case '|': {
							if(ammount_left > 2 && this->char_stream.peek(2) == '='){
								this->char_stream.skip(evo::stringSize("*|="));
								this->create_token(Token::lookupKind("*|="));
								return evo::Result<>();
							}else{
								this->char_stream.skip(evo::stringSize("*|"));
								this->create_token(Token::lookupKind("*|"));
								return evo::Result<>();
							}
						} break;
					}
				}

				this->char_stream.skip(evo::stringSize("*"));
				this->create_token(Token::lookupKind("*"));
				return evo::Result<>();
			} break;


			case '+': {
				const size_t ammount_left = this->char_stream.ammount_left();

				if(ammount_left > 1){
					switch(this->char_stream.peek(1)){
						case '=': {
							this->char_stream.skip(evo::stringSize("+="));
							this->create_token(Token::lookupKind("+="));
							return evo::Result<>();
						} break;

						case '%': {
							if(ammount_left > 2 && this->char_stream.peek(2) == '='){
								this->char_stream.skip(evo::stringSize("+%="));
								this->create_token(Token::lookupKind("+%="));
								return evo::Result<>();
							}else{
								this->char_stream.skip(evo::stringSize("+%"));
								this->create_token(Token::lookupKind("+%"));
								return evo::Result<>();
							}
						} break;

						case '|': {
							if(ammount_left > 2 && this->char_stream.peek(2) == '='){
								this->char_stream.skip(evo::stringSize("+|="));
								this->create_token(Token::lookupKind("+|="));
								return evo::Result<>();
							}else{
								this->char_stream.skip(evo::stringSize("+|"));
								this->create_token(Token::lookupKind("+|"));
								return evo::Result<>();
							}
						} break;
					}
				}

				this->char_stream.skip(evo::stringSize("+"));
				this->create_token(Token::lookupKind("+"));
				return evo::Result<>();
			} break;

			case ',': {
				this->char_stream.skip(evo::stringSize(","));
				this->create_token(Token::lookupKind(","));
				return evo::Result<>();
			} break;

			case '-': {
				const size_t ammount_left = this->char_stream.ammount_left();

				if(ammount_left > 1){
					switch(this->char_stream.peek(1)){
						case '>': {
							this->char_stream.skip(evo::stringSize("->"));
							this->create_token(Token::lookupKind("->"));
							return evo::Result<>();
						} break;

						case '=': {
							this->char_stream.skip(evo::stringSize("-="));
							this->create_token(Token::lookupKind("-="));
							return evo::Result<>();
						} break;

						case '%': {
							if(ammount_left > 2 && this->char_stream.peek(2) == '='){
								this->char_stream.skip(evo::stringSize("-%="));
								this->create_token(Token::lookupKind("-%="));
								return evo::Result<>();
							}else{
								this->char_stream.skip(evo::stringSize("-%"));
								this->create_token(Token::lookupKind("-%"));
								return evo::Result<>();
							}
						} break;

						case '|': {
							if(ammount_left > 2 && this->char_stream.peek(2) == '='){
								this->char_stream.skip(evo::stringSize("-|="));
								this->create_token(Token::lookupKind("-|="));
								return evo::Result<>();
							}else{
								this->char_stream.skip(evo::stringSize("-|"));
								this->create_token(Token::lookupKind("-|"));
								return evo::Result<>();
							}
						} break;

					}
				}

				this->char_stream.skip(evo::stringSize("-"));
				this->create_token(Token::lookupKind("-"));
				return evo::Result<>();
			} break;

			case '.': {
				const size_t ammount_left = this->char_stream.ammount_left();

				if(ammount_left > 1){
					switch(this->char_stream.peek(1)){
						case '*': {
							this->char_stream.skip(evo::stringSize(".*"));
							this->create_token(Token::lookupKind(".*"));
							return evo::Result<>();
						} break;

						case '?': {
							this->char_stream.skip(evo::stringSize(".?"));
							this->create_token(Token::lookupKind(".?"));
							return evo::Result<>();
						} break;

						case '.': {
							if(ammount_left > 2 && this->char_stream.peek(2) == '.'){
								this->char_stream.skip(evo::stringSize("..."));
								this->create_token(Token::lookupKind("..."));
								return evo::Result<>();	
							}
						} break;
					}
				}

				this->char_stream.skip(evo::stringSize("."));
				this->create_token(Token::lookupKind("."));
				return evo::Result<>();
			} break;

			case '/': {
				const size_t ammount_left = this->char_stream.ammount_left();

				if(ammount_left > 1){
					switch(this->char_stream.peek(1)){
						case '*': {
							this->char_stream.skip(2);

							unsigned num_closes_needed = 1;
							while(num_closes_needed > 0){
								if(this->char_stream.ammount_left() < 2){
									const evo::Result<Source::Location> current_location =
										this->get_current_location_token();
									if(current_location.isError()){ return evo::resultError; }

									this->emit_error(
										"Unterminated multi-line comment",
										current_location.value(),
										Diagnostic::Info("Expected a \"*/\" before the end of the file")
									);

									return evo::resultError;
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

							return evo::Result<>();
						} break;

						case '/': {
							while(
								this->char_stream.at_end() == false && 
								this->char_stream.peek() != '\n' && this->char_stream.peek() != '\r'
							){
								this->char_stream.skip(1);
							}

							return evo::Result<>();
						} break;

						case '=': {
							this->char_stream.skip(evo::stringSize("/="));
							this->create_token(Token::lookupKind("/="));
							return evo::Result<>();

						} break;
					}
				}

				this->char_stream.skip(evo::stringSize("/"));
				this->create_token(Token::lookupKind("/"));
				return evo::Result<>();
			} break;

			case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': {
				return this->tokenize_number_literal();
				// if(this->tokenize_number_literal()){ return evo::Result<>(); }
			} break;

			case ':': {
				this->char_stream.skip(evo::stringSize(":"));
				this->create_token(Token::lookupKind(":"));
				return evo::Result<>();
			} break;

			case ';': {
				this->char_stream.skip(evo::stringSize(";"));
				this->create_token(Token::lookupKind(";"));
				return evo::Result<>();
			} break;

			case '<': {
				const size_t ammount_left = this->char_stream.ammount_left();

				if(ammount_left > 1){
					switch(this->char_stream.peek(1)){
						case '<': {
							if(ammount_left > 2){
								switch(this->char_stream.peek(2)){
									case '=': {
										this->char_stream.skip(evo::stringSize("<<="));
										this->create_token(Token::lookupKind("<<="));
										return evo::Result<>();
									} break;

									case '|': {
										if(ammount_left > 3 && this->char_stream.peek(3) == '='){
											this->char_stream.skip(evo::stringSize("<<|="));
											this->create_token(Token::lookupKind("<<|="));
											return evo::Result<>();
										}else{
											this->char_stream.skip(evo::stringSize("<<|"));
											this->create_token(Token::lookupKind("<<|"));
											return evo::Result<>();
										}
									} break;
								}

								this->char_stream.skip(evo::stringSize("<<"));
								this->create_token(Token::lookupKind("<<"));
								return evo::Result<>();
							}
						} break;

						case '=': {
							this->char_stream.skip(evo::stringSize("<="));
							this->create_token(Token::lookupKind("<="));
							return evo::Result<>();
						} break;

						case '{': {
							this->char_stream.skip(evo::stringSize("<{"));
							this->create_token(Token::lookupKind("<{"));
							return evo::Result<>();
						} break;
					}
				}

				this->char_stream.skip(evo::stringSize("<"));
				this->create_token(Token::lookupKind("<"));
				return evo::Result<>();
			} break;

			case '=': {
				const size_t ammount_left = this->char_stream.ammount_left();

				if(ammount_left > 1 && this->char_stream.peek(1) == '='){
					this->char_stream.skip(evo::stringSize("=="));
					this->create_token(Token::lookupKind("=="));
					return evo::Result<>();
				}else{
					this->char_stream.skip(evo::stringSize("="));
					this->create_token(Token::lookupKind("="));
					return evo::Result<>();
				}
			} break;

			case '>': {
				const size_t ammount_left = this->char_stream.ammount_left();

				if(ammount_left > 1){
					switch(this->char_stream.peek(1)){
						case '>': {
							if(ammount_left > 2 && this->char_stream.peek(2) == '='){
								this->char_stream.skip(evo::stringSize(">>="));
								this->create_token(Token::lookupKind(">>="));
								return evo::Result<>();
							}else{
								this->char_stream.skip(evo::stringSize(">>"));
								this->create_token(Token::lookupKind(">>"));
								return evo::Result<>();
							}
						} break;

						case '=': {
							this->char_stream.skip(evo::stringSize(">="));
							this->create_token(Token::lookupKind(">="));
							return evo::Result<>();
						} break;
					}
				}

				this->char_stream.skip(evo::stringSize(">"));
				this->create_token(Token::lookupKind(">"));
				return evo::Result<>();
			} break;

			case '?': {
				this->char_stream.skip(evo::stringSize("?"));
				this->create_token(Token::lookupKind("?"));
				return evo::Result<>();
			} break;

			case '@': {
				this->char_stream.skip(1);
				return this->tokenize_identifier(Token::Kind::INTRINSIC);
			} break;

			case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
			case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
			case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
			case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
			case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x':
			case 'y': case 'z': case '_': {
				return this->tokenize_identifier(Token::Kind::IDENT);
			} break;

			case '[': {
				this->char_stream.skip(evo::stringSize("["));
				this->create_token(Token::lookupKind("["));
				return evo::Result<>();
			} break;

			case '\\': break;

			case ']': {
				this->char_stream.skip(evo::stringSize("]"));
				this->create_token(Token::lookupKind("]"));
				return evo::Result<>();
			} break;

			case '^': {
				const size_t ammount_left = this->char_stream.ammount_left();

				if(ammount_left > 1 && this->char_stream.peek(1) == '='){
					this->char_stream.skip(evo::stringSize("^="));
					this->create_token(Token::lookupKind("^="));
					return evo::Result<>();

				}else{
					this->char_stream.skip(evo::stringSize("^"));
					this->create_token(Token::lookupKind("^"));
					return evo::Result<>();
				}
			} break;
			
			case '`': break;

			case '{': {
				this->char_stream.skip(evo::stringSize("{"));
				this->create_token(Token::lookupKind("{"));
				return evo::Result<>();
			} break;

			case '|': {
				const size_t ammount_left = this->char_stream.ammount_left();

				if(ammount_left > 1){
					switch(this->char_stream.peek(1)){
						case '=': {
							this->char_stream.skip(evo::stringSize("|="));
							this->create_token(Token::lookupKind("|="));
							return evo::Result<>();
						} break;

						case '|': {
							this->char_stream.skip(evo::stringSize("||"));
							this->create_token(Token::lookupKind("||"));
							return evo::Result<>();
						} break;
					}
				}

				this->char_stream.skip(evo::stringSize("|"));
				this->create_token(Token::lookupKind("|"));
				return evo::Result<>();
			} break;

			case '}': {
				const size_t ammount_left = this->char_stream.ammount_left();

				if(ammount_left > 1 && this->char_stream.peek(1) == '>'){
					this->char_stream.skip(evo::stringSize("}>"));
					this->create_token(Token::lookupKind("}>"));
					return evo::Result<>();

				}else{
					this->char_stream.skip(evo::stringSize("}"));
					this->create_token(Token::lookupKind("}"));
					return evo::Result<>();
				}
			} break;

			case '~': {
				this->char_stream.skip(evo::stringSize("~"));
				this->create_token(Token::lookupKind("~"));
				return evo::Result<>();
			} break;
		}

		this->error_unrecognized_character();
		return evo::resultError;
	}




	// TODO(FUTURE): change to not use global initialization
	const static auto keyword_map = std::unordered_map<std::string_view, Token::Kind>{
		// types
		{"Void",        Token::Kind::TYPE_VOID},
		{"Type",        Token::Kind::TYPE_TYPE},
		{"This",        Token::Kind::TYPE_THIS},

		{"Int",         Token::Kind::TYPE_INT},
		{"ISize",       Token::Kind::TYPE_ISIZE},

		{"UInt",        Token::Kind::TYPE_UINT},
		{"USize",       Token::Kind::TYPE_USIZE},

		{"F16",         Token::Kind::TYPE_F16},
		{"F32",         Token::Kind::TYPE_F32},
		{"F64",         Token::Kind::TYPE_F64},
		{"F80",         Token::Kind::TYPE_F80},
		{"F128",        Token::Kind::TYPE_F128},

		{"Byte",        Token::Kind::TYPE_BYTE},
		{"Bool",        Token::Kind::TYPE_BOOL},
		{"Bool32",      Token::Kind::TYPE_BOOL32},
		{"Char",        Token::Kind::TYPE_CHAR},
		{"RawPtr",      Token::Kind::TYPE_RAWPTR},
		{"TypeID",      Token::Kind::TYPE_TYPEID},

		{"CWChar",      Token::Kind::TYPE_C_WCHAR},
		{"CShort",      Token::Kind::TYPE_C_SHORT},
		{"CUShort",     Token::Kind::TYPE_C_USHORT},
		{"CInt",        Token::Kind::TYPE_C_INT},
		{"CUInt",       Token::Kind::TYPE_C_UINT},
		{"CLong",       Token::Kind::TYPE_C_LONG},
		{"CULong",      Token::Kind::TYPE_C_ULONG},
		{"CLongLong",   Token::Kind::TYPE_C_LONG_LONG},
		{"CULongLong",  Token::Kind::TYPE_C_ULONG_LONG},
		{"CLongDouble", Token::Kind::TYPE_C_LONG_DOUBLE},


		// keywords
		{"var",         Token::Kind::KEYWORD_VAR},
		{"const",       Token::Kind::KEYWORD_CONST},
		{"def",         Token::Kind::KEYWORD_DEF},
		{"func",        Token::Kind::KEYWORD_FUNC},
		{"alias",       Token::Kind::KEYWORD_ALIAS},
		{"type",        Token::Kind::KEYWORD_TYPE},
		{"struct",      Token::Kind::KEYWORD_STRUCT},
		{"interface",   Token::Kind::KEYWORD_INTERFACE},
		{"impl",        Token::Kind::KEYWORD_IMPL},
		{"union",       Token::Kind::KEYWORD_UNION},
		{"enum",        Token::Kind::KEYWORD_ENUM},

		{"return",      Token::Kind::KEYWORD_RETURN},
		{"error",       Token::Kind::KEYWORD_ERROR},
		{"unreachable", Token::Kind::KEYWORD_UNREACHABLE},
		{"break",       Token::Kind::KEYWORD_BREAK},
		{"continue",    Token::Kind::KEYWORD_CONTINUE},

		{"null",        Token::Kind::KEYWORD_NULL},
		{"uninit",      Token::Kind::KEYWORD_UNINIT},
		{"zeroinit",    Token::Kind::KEYWORD_ZEROINIT},
		{"this",        Token::Kind::KEYWORD_THIS},

		{"read",        Token::Kind::KEYWORD_READ},
		{"mut",         Token::Kind::KEYWORD_MUT},
		{"in",          Token::Kind::KEYWORD_IN},

		{"copy",        Token::Kind::KEYWORD_COPY},
		{"move",        Token::Kind::KEYWORD_MOVE},
		{"forward",     Token::Kind::KEYWORD_FORWARD},
		{"new",         Token::Kind::KEYWORD_NEW},
		{"delete",      Token::Kind::KEYWORD_DELETE},
		{"as",          Token::Kind::KEYWORD_AS},

		{"extern",      Token::Kind::KEYWORD_EXTERN},

		{"if",          Token::Kind::KEYWORD_IF},
		{"else",        Token::Kind::KEYWORD_ELSE},
		{"when",        Token::Kind::KEYWORD_WHEN},
		{"while",       Token::Kind::KEYWORD_WHILE},
		{"for",         Token::Kind::KEYWORD_FOR},
		{"switch",      Token::Kind::KEYWORD_SWITCH},
		{"case",        Token::Kind::KEYWORD_CASE},
		{"defer",       Token::Kind::KEYWORD_DEFER},
		{"errorDefer",  Token::Kind::KEYWORD_ERROR_DEFER},

		{"unsafe",      Token::Kind::KEYWORD_UNSAFE},
		{"try",         Token::Kind::KEYWORD_TRY},
		{"catch",       Token::Kind::KEYWORD_CATCH},
		{"asm",         Token::Kind::KEYWORD_ASM},

		// discard
		{"_", Token::lookupKind("_")},
	};

	const static auto keyword_end = keyword_map.end();


	auto Tokenizer::tokenize_identifier(Token::Kind ident_kind) -> evo::Result<> {
		const char* string_start_ptr = this->char_stream.peek_raw_ptr();

		char peeked_char = this->char_stream.peek();
		do{
			this->char_stream.skip(1);

			if(this->char_stream.at_end()){ break; }

			peeked_char = this->char_stream.peek();
		}while(evo::isAlphaNumeric(peeked_char) || peeked_char == '_');

		auto ident_name = std::string_view(string_start_ptr, this->char_stream.peek_raw_ptr() - string_start_ptr);

		if(ident_kind == Token::Kind::IDENT){
			if(ident_name == "true") [[unlikely]] {
				this->create_token(Token::Kind::LITERAL_BOOL, true);
				return evo::Result<>();

			}else if(ident_name == "false") [[unlikely]] {
				this->create_token(Token::Kind::LITERAL_BOOL, false);
				return evo::Result<>();

			}else{
				{
					const auto keyword_map_iter = keyword_map.find(ident_name);

					if(keyword_map_iter != keyword_end){
						this->create_token(keyword_map_iter->second);
						return evo::Result<>();
					}
				}


				enum class GetIntTypeResult{
					NOT_INT_TYPE,
					SUCCESS,
					ERROR,
				};

				auto get_int_type = [&](Token::Kind kind, size_t bitwidth_start_index) -> GetIntTypeResult {
					const std::string_view bitwidth_str = ident_name.substr(bitwidth_start_index);
					
					for(char character : bitwidth_str){
						if(evo::isNumber(character) == false){
							return GetIntTypeResult::NOT_INT_TYPE;
						}
					}

					const evo::Expected<uint32_t, StrToNumError> bitwidth = str_to_num<uint32_t>(bitwidth_str, 10);

					if(bitwidth.has_value()){
						if(bitwidth.value() > std::pow(2, 23)){
							const evo::Result<Source::Location> current_location = this->get_current_location_token();
							if(current_location.isError()){ return GetIntTypeResult::ERROR; }

							this->emit_error(
								"Integer bit-width is too large",
								current_location.value(),
								Diagnostic::Info("Maximum bitwidth is 2^23 (8,388,608)")
							);

						}else if(bitwidth.value() == 0){
							const evo::Result<Source::Location> current_location = this->get_current_location_token();
							if(current_location.isError()){ return GetIntTypeResult::ERROR; }

							this->emit_error("Integer bit-width cannot be 0", current_location.value());
						}

						this->create_token(kind, uint64_t(bitwidth.value()));
						return GetIntTypeResult::SUCCESS;
					}

					switch(bitwidth.error()){
						case StrToNumError::OUT_OF_RANGE: {
							const evo::Result<Source::Location> current_location = this->get_current_location_token();
							if(current_location.isError()){ return GetIntTypeResult::ERROR; }

							this->emit_error(
								"Integer bit-width is too large",
								current_location.value(),
								Diagnostic::Info("Maximum bitwidth is 2^23 (8,388,608)")
							);
							return GetIntTypeResult::ERROR;
						} break;

						case StrToNumError::INVALID: {
							const evo::Result<Source::Location> current_location = this->get_current_location_token();
							if(current_location.isError()){ return GetIntTypeResult::ERROR; }

							this->emit_fatal(
								Diagnostic::createFatalMessage("Attempted to tokenize invalid integer bit-width"),
								current_location.value()
							);
							return GetIntTypeResult::ERROR;
						} break;
					}
					evo::unreachable();
				};

				if(ident_name.size() > 1){
					const GetIntTypeResult get_int_type_result = [&]() -> GetIntTypeResult {
						if(ident_name[0] == 'I'){
							return get_int_type(Token::Kind::TYPE_I_N, 1);

						}else if(ident_name.size() > 2 && ident_name[0] == 'U' && ident_name[1] == 'I'){
							return get_int_type(Token::Kind::TYPE_UI_N, 2);

						}else{
							return GetIntTypeResult::NOT_INT_TYPE;
						}
					}();

					switch(get_int_type_result){
						case GetIntTypeResult::NOT_INT_TYPE: break;
						case GetIntTypeResult::SUCCESS:      return evo::Result<>();
						case GetIntTypeResult::ERROR:        return evo::resultError;
					}
				}

				// default ident
				this->create_token(Token::Kind::IDENT, ident_name);
				return evo::Result<>();
			}

		}else{
			this->create_token(ident_kind, ident_name);
			return evo::Result<>();
		}
	}



	auto Tokenizer::tokenize_number_literal() -> evo::Result<> {
		int base = 10;
		auto number_string = std::string();
		bool has_decimal_point = false;

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
				if(current_location.isError()){ return evo::Result<>(); }

				this->emit_error(
					"Leading zeros in literal numbers are not supported",
					current_location.value(),
					Diagnostic::Info("Note: the literal integer prefix for base-8 is \"0o\"")
				);

				return evo::resultError;
			}
		}


		///////////////////////////////////
		// get number

		while(this->char_stream.at_end() == false){
			const char peeked_char = this->char_stream.peek();

			if(peeked_char == '_'){
				this->char_stream.skip(1);
				continue;

			}else if(peeked_char == '.'){
				if(has_decimal_point){
					const evo::Result<Source::Location> current_location = this->get_current_location_point();
					if(current_location.isError()){ return evo::resultError; }

					this->emit_error(
						"Cannot have multiple decimal points in a floating-point literal", current_location.value()
					);
					return evo::resultError;
				}

				if(base == 2){
					this->emit_error(
						"Base-2 floating-point literals are not supported",
						Source::Location(
							this->source.getID(),
							this->current_token_line_start, this->current_token_line_start,
							this->current_token_collumn_start, this->current_token_collumn_start + 1
						)
					);
					return evo::resultError;

				}else if(base == 8){
					this->emit_error(
						"Base-8 floating-point literals are not supported",
						Source::Location(
							this->source.getID(),
							this->current_token_line_start, this->current_token_line_start,
							this->current_token_collumn_start, this->current_token_collumn_start + 1
						)
					);
					return evo::resultError;
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
					if(current_location.isError()){ return evo::resultError; }

					this->emit_error("Base-2 numbers should only have digits 0 and 1", current_location.value());
					return evo::resultError;

				}else{
					break;
				}

			}else if(base == 8){
				if(evo::isOctalNumber(peeked_char)){
					number_string += this->char_stream.next();

				}else if(evo::isHexNumber(peeked_char)){
					const evo::Result<Source::Location> current_location = this->get_current_location_point();
					if(current_location.isError()){ return evo::resultError; }

					this->emit_error("Base-8 numbers should only have digits 0-7", current_location.value());
					return evo::resultError;

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
					if(current_location.isError()){ return evo::resultError; }

					this->emit_error(
						"Base-10 numbers should only have digits 0-9",
						current_location.value(),
						Diagnostic::Info("Note: The prefix for hexidecimal numbers (base-16) is \"0x\"")
					);
					return evo::resultError;

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

		if(number_string.back() == '.'){
			const evo::Result<Source::Location> current_location = this->get_current_location_token();
			if(current_location.isError()){ return evo::resultError; }

			this->emit_error(
				"Float literal cannot end in a [.]",
				current_location.value(),
				Diagnostic::Info("Maybe add a [0] to the end")
			);
			return evo::resultError;
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
					if(current_location.isError()){ return evo::resultError; }

					this->emit_error("Literal number exponents should only have digits 0-9", current_location.value());
					return evo::resultError;

				}else{
					break;
				}
			}
		}



		///////////////////////////////////
		// parse exponent (if it exists)

		int64_t exponent_number = 0;

		if(exponent_string.size() != 0){
			const evo::Expected<int64_t, StrToNumError> converted_exponent_number = 
				str_to_num<int64_t>(exponent_string, base);

			if(converted_exponent_number.has_value()){
				exponent_number = converted_exponent_number.value();	
			}else{
				switch(converted_exponent_number.error()){
					case StrToNumError::OUT_OF_RANGE: {
						const evo::Result<Source::Location> current_location = this->get_current_location_token();
						if(current_location.isError()){ return evo::resultError; }

						this->emit_error(
							"Literal number exponent too large to fit into a I64."
								"This limitation will be removed when the compiler is self hosted.",
							current_location.value()
						);
						return evo::resultError;
					} break;

					case StrToNumError::INVALID: {
						const evo::Result<Source::Location> current_location = this->get_current_location_token();
						if(current_location.isError()){ return evo::resultError; }

						this->emit_fatal(
							Diagnostic::createFatalMessage("Tried to convert invalid integer string for exponent"),
							current_location.value()
						);
						return evo::resultError;
					} break;
				}
			}
		}


		///////////////////////////////////
		// check exponent isn't too large

		if(exponent_number != 0){
			const evo::float64_t floating_point_exponent_number = evo::float64_t(exponent_number);

			if(has_decimal_point){
				const static evo::float64_t max_float_exp = std::log10(std::numeric_limits<evo::float64_t>::max()) + 1;

				if(floating_point_exponent_number > max_float_exp){
					const evo::Result<Source::Location> current_location = this->get_current_location_token();
					if(current_location.isError()){ return evo::resultError; }

					this->emit_error(
						"Literal floating-point number too large to fit into an F64",
						current_location.value(),
						Diagnostic::Info("This limitation will be removed when the compiler is self hosted")
					);
					return evo::resultError;
				}

			}else{
				const static evo::float64_t max_int_exp = std::log10(std::numeric_limits<uint64_t>::max()) + 1;

				if(floating_point_exponent_number > max_int_exp){
					const evo::Result<Source::Location> current_location = this->get_current_location_token();
					if(current_location.isError()){ return evo::resultError; }

					this->emit_error(
						"Literal number integer too large to fit into a UI64",
						current_location.value(),
						Diagnostic::Info("This limitation will be removed when the compiler is self hosted")
					);
					return evo::resultError;
				}
			}
		}



		///////////////////////////////////
		// parse / save number (with some checking)

		if(has_decimal_point){
			const evo::Expected<evo::float64_t, StrToNumError> converted_parsed_number = 
				str_to_num<evo::float64_t>(number_string, base);

			if(converted_parsed_number.has_value() == false){
				switch(converted_parsed_number.error()){
					case StrToNumError::OUT_OF_RANGE: {
						const evo::Result<Source::Location> current_location = this->get_current_location_token();
						if(current_location.isError()){ return evo::resultError; }

						this->emit_error(
							"Literal floating-point too large to fit into an F64",
							current_location.value(),
							Diagnostic::Info("This limitation will be removed when the compiler is self hosted")
						);
						return evo::resultError;
					} break;

					case StrToNumError::INVALID: {
						const evo::Result<Source::Location> current_location = this->get_current_location_token();
						if(current_location.isError()){ return evo::resultError; }

						this->emit_fatal(
							Diagnostic::createFatalMessage("Tried to convert invalid literal floating-point number"),
							current_location.value()
						);
						return evo::resultError;
					} break;
				}
			}

			const evo::float64_t parsed_number = converted_parsed_number.value();


			if(
				parsed_number == 0.0 && 
				std::numeric_limits<evo::float64_t>::max() / parsed_number < std::pow(10, exponent_number)
			){
				const evo::Result<Source::Location> current_location = this->get_current_location_token();
				if(current_location.isError()){ return evo::resultError; }

				this->emit_error(
					"Literal number integer too large to fit into an F64",
					current_location.value(),
					Diagnostic::Info("This limitation will be removed when the compiler is self hosted")
				);
				return evo::resultError;
			}


			evo::float64_t output_number = parsed_number;
			if(exponent_number != 0){
				output_number *= std::pow(10, exponent_number);
			}

			this->create_token(Token::Kind::LITERAL_FLOAT, output_number);


		}else{
			const evo::Expected<uint64_t, StrToNumError> converted_parsed_number = 
				str_to_num<uint64_t>(number_string, base);

			if(converted_parsed_number.has_value() == false){
				switch(converted_parsed_number.error()){
					case StrToNumError::OUT_OF_RANGE: {
						const evo::Result<Source::Location> current_location = this->get_current_location_token();
						if(current_location.isError()){ return evo::resultError; }

						this->emit_error(
							"Literal integer too large to fit into a UI64",
							current_location.value(),
							Diagnostic::Info("This limitation will be removed when the compiler is self hosted")
						);
						return evo::resultError;
					} break;

					case StrToNumError::INVALID: {
						const evo::Result<Source::Location> current_location = this->get_current_location_token();
						if(current_location.isError()){ return evo::resultError; }

						this->emit_fatal(
							Diagnostic::createFatalMessage("Tried to convert invalid literal integer"),
							current_location.value()
						);
						return evo::resultError;
					} break;
				}
			}


			uint64_t output_number = converted_parsed_number.value();
			if(exponent_number != 0){
				output_number *= uint64_t(std::pow(10, exponent_number));
			}

			this->create_token(Token::Kind::LITERAL_INT, output_number);
		}

		return evo::Result<>();
	}


	auto Tokenizer::tokenize_string_literal() -> evo::Result<> {
		const char delimiter = this->char_stream.next();

		auto literal_value = std::string();

		bool contains_illegal_character_literal_character = false;

		while(this->char_stream.peek() != delimiter){
			bool unexpected_at_end = false;

			if(this->char_stream.at_end()){
				unexpected_at_end = true;

			}else if(this->char_stream.peek() == '\\'){
				switch(this->char_stream.peek(1)){
					break; case '0':  literal_value += '\0';   this->char_stream.skip(2);
					break; case 'e':  literal_value += '\x1B'; this->char_stream.skip(2);
					break; case 't':  literal_value += '\t';   this->char_stream.skip(2);
					break; case 'n':  literal_value += '\n';   this->char_stream.skip(2);
					break; case 'r':  literal_value += '\r';   this->char_stream.skip(2);

					break; case '\'': literal_value += '\'';   this->char_stream.skip(2);
					break; case '"':  literal_value += '"';    this->char_stream.skip(2);
					break; case '\\': literal_value += '\\';   this->char_stream.skip(2);

					break; case 'x': {
						this->char_stream.skip(2);

						evo::Result<Source::Location> current_location = this->get_current_location_point();
						if(current_location.isError()){ return evo::resultError; }


						char first_hex_char = this->char_stream.next();
						if(evo::isHexNumber(first_hex_char) == false){
							this->emit_error("Invalid value for hexidecimal escape sequence", current_location.value());
							return evo::resultError;
						}


						current_location = this->get_current_location_point();
						if(current_location.isError()){ return evo::resultError; }

						char second_hex_char = this->char_stream.next();
						if(evo::isHexNumber(second_hex_char) == false){
							this->emit_error(
								"Invalid value for hexidecimal escape sequence",
								current_location.value(),
								Diagnostic::Info(std::format("Did you mean '\\x0{}'?", first_hex_char))
							);
							return evo::resultError;
						}


						if(first_hex_char <= '9'){
							first_hex_char -= '0';
						}else if(first_hex_char <= 'F'){
							first_hex_char -= 'A' - 10;
						}else{
							first_hex_char -= 'a' - 10;
						}

						if(second_hex_char <= '9'){
							second_hex_char -= '0';
						}else if(second_hex_char <= 'F'){
							second_hex_char -= 'A' - 10;
						}else{
							second_hex_char -= 'a' - 10;
						}

						literal_value += (first_hex_char << 4) | second_hex_char;
					} break;

					break; default: {
						this->char_stream.skip(2);
						
						const evo::Result<uint32_t> line_result = this->char_stream.get_line();
						if(line_result.isError()){ this->error_line_too_big(); return evo::resultError; }

						const evo::Result<uint32_t> collumn_result = this->char_stream.get_collumn();
						if(collumn_result.isError()){ this->error_collumn_too_big(); return evo::resultError; }

						auto infos = evo::SmallVector<Diagnostic::Info>();

						if(evo::isNumber(this->char_stream.peek_back())){
							infos.emplace_back(
								"Note: octal escape sequences are not allowed"
									"- did you mean a hexidecimal escape sequence?"
							);
						}

						this->emit_error(
							std::format("Unknown escape code '\\{}'", this->char_stream.peek_back()),
							Source::Location(
								this->source.getID(),
								this->current_token_line_start, line_result.value(),
								this->current_token_collumn_start + 1, collumn_result.value() - 1
							)
						);
						return evo::resultError;
					}
				}

			}else{
				if(this->char_stream.peek() == '\n' || this->char_stream.peek() == '\t'){
					contains_illegal_character_literal_character = true;
				}

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
				if(line_result.isError()){ this->error_line_too_big(); return evo::resultError; }

				const evo::Result<uint32_t> collumn_result = this->char_stream.get_collumn();
				if(collumn_result.isError()){ this->error_collumn_too_big(); return evo::resultError; }

				this->emit_error(
					std::format("Unterminated {} literal", string_type_name),
					Source::Location(
						this->source.getID(),
						this->current_token_line_start, line_result.value(),
						this->current_token_collumn_start, collumn_result.value()
					),
					Diagnostic::Info(std::format("Expected a {} before the end of the file", delimiter))
				);
				return evo::resultError;	
			}

		}


		this->char_stream.skip(1);

		if(delimiter == '\''){
			if(literal_value.empty()){
				const evo::Result<Source::Location> current_location = this->get_current_location_token();
				if(current_location.isError()){ return evo::resultError; }

				this->emit_error("Literal character cannot be empty", current_location.value());
				return evo::resultError;

			}

			if(literal_value.size() > 1){
				const evo::Result<Source::Location> current_location = this->get_current_location_token();
				if(current_location.isError()){ return evo::resultError; }

				this->emit_error("Literal character must be only 1 character", current_location.value());
				return evo::resultError;
			}

			if(contains_illegal_character_literal_character){
				const evo::Result<Source::Location> current_location = this->get_current_location_token();
				if(current_location.isError()){ return evo::resultError; }

				this->emit_error("Illegal character literal character", current_location.value());
				return evo::resultError;
			}

			this->create_token(Token::Kind::LITERAL_CHAR, literal_value[0]);
		}else{
			this->create_token(Token::Kind::LITERAL_STRING, literal_value);
		}


		return evo::Result<>();
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
			"File too large",
			current_location.value(),
			Diagnostic::Info(std::format("Source files can have a maximum of {} (2^32-2) tokens", MAX_TOKENS))
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



	[[nodiscard]] static constexpr auto hex_from_4_bits(char num) -> char {
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
				std::format(
					"Unrecognized or unexpected character \"{}\" (ASCII charcode: 0x{:x})",
					evo::printCharName(peeked_char),
					int(peeked_char)
				),
				current_location.value()
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
				std::format(
					"Unrecognized character (non-standard utf-8 character)",
					evo::printCharName(peeked_char),
					int(peeked_char)
				),
				current_location.value()
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
			std::format("Unrecognized character \"{}\" (UTF-8 code: {})", utf8_str, utf8_charcodes_str),
			current_location.value()
		);
	}



	auto Tokenizer::error_line_too_big() -> void {
		this->emit_error(
			"Line number is too large",
			Source::Location(
				this->source.getID(), this->current_token_line_start, this->current_token_collumn_start
			),
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
			"Collumn number is too large",
			Source::Location(
				this->source.getID(), this->current_token_line_start, this->current_token_collumn_start
			),
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