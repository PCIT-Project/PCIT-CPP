////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./extract_macros.h"


#include <Clang.h>

#include "./MacroToken.h"
#include "./MacroParser.h"

namespace pcit::clangint{




	EVO_NODISCARD static auto tokenize_macro(const clang::MacroInfo& macro_info)
	-> evo::Result<evo::SmallVector<MacroToken>> {
		if(macro_info.isFunctionLike()){ return evo::resultError; }

		auto tokens = evo::SmallVector<MacroToken>();

		for(const clang::Token& token : macro_info.tokens()){
			// clang/Basic/MacroTokenKinds.def
			switch(token.getKind()){
				case clang::tok::identifier: {
					tokens.emplace_back(
						MacroToken::Kind::IDENT, std::string_view(token.getIdentifierInfo()->getName())
					);
				} break;


				// TODO(PERF): loop through the string only once
				case clang::tok::numeric_constant: {
					if(token.getLength() == 1){
						tokens.emplace_back(
							MacroToken::Kind::LITERAL_INT,
							MacroToken::IntValue(
								uint64_t(token.getLiteralData()[0]) - uint64_t('0'),
								MacroToken::IntValue::Type::FLUID
							)
						);
						continue;
					}


					const auto full_number_view = std::string_view(token.getLiteralData(), token.getLength());


					size_t num_start_index = 0;
					unsigned base = 10;
					if(full_number_view[0] == '0'){
						num_start_index += 1;
						switch(full_number_view[1]){
							break; case 'x': base = 16; num_start_index += 1;
							break; case 'X': base = 16; num_start_index += 1;
							break; case 'b': base = 2; num_start_index += 1;
							break; case 'B': base = 2; num_start_index += 1;
							break; default: base = 8;
						}
					}

					const bool is_float = [&]() -> char {
						for(char character : full_number_view){
							if(character ==  '.'){ return true; }
						}
						return false;
					}();


					if(is_float){
						MacroToken::FloatValue::Type float_type = MacroToken::FloatValue::Type::FLUID;
						size_t suffix_size = 0;

						if(full_number_view.ends_with("f16") || full_number_view.ends_with("F16")){
							float_type = MacroToken::FloatValue::Type::F16;
							suffix_size = 3;

						}else if(full_number_view.ends_with("bf16") || full_number_view.ends_with("BF16")){
							float_type = MacroToken::FloatValue::Type::BF16;
							suffix_size = 4;
							
						}else if(full_number_view.ends_with("f32") || full_number_view.ends_with("F32")){
							float_type = MacroToken::FloatValue::Type::F32;
							suffix_size = 3;

						}else if(full_number_view.ends_with("f64") || full_number_view.ends_with("F64")){
							float_type = MacroToken::FloatValue::Type::F64;
							suffix_size = 3;

						}else if(full_number_view.ends_with("f128") || full_number_view.ends_with("F128")){
							// float_type = MacroToken::FloatValue::Type::F128;
							// suffix_size = 4;
							return evo::resultError;
							
						}else if(full_number_view.ends_with("f") || full_number_view.ends_with("F")){
							float_type = MacroToken::FloatValue::Type::F32;
							suffix_size = 1;
							
						}else if(full_number_view.ends_with("l") || full_number_view.ends_with("L")){
							float_type = MacroToken::FloatValue::Type::C_LONG_DOUBLE;
							suffix_size = 1;
						}

						float64_t token_value;
						const auto [ptr, ec] = std::from_chars(
							full_number_view.data() + num_start_index,
							full_number_view.data() + full_number_view.size() - suffix_size,
							token_value,
							base == 10 ? std::chars_format::general : std::chars_format::hex
						);

						if(ptr != full_number_view.data() + full_number_view.size() - suffix_size){
							return evo::resultError;
						}
						if(ec != std::errc()){ return evo::resultError; }

						tokens.emplace_back(
							MacroToken::Kind::LITERAL_FLOAT, MacroToken::FloatValue(token_value, float_type)
						);


					}else [[likely]] {
						MacroToken::IntValue::Type int_type = MacroToken::IntValue::Type::FLUID;
						size_t suffix_size = 0;

						if(
							full_number_view.ends_with("zu")
							|| full_number_view.ends_with("zU")
							|| full_number_view.ends_with("Zu")
							|| full_number_view.ends_with("ZU")
						){
							int_type = MacroToken::IntValue::Type::USIZE;
							suffix_size = 2;
							
						}else if(full_number_view.ends_with("z") || full_number_view.ends_with("Z")){
							int_type = MacroToken::IntValue::Type::ISIZE;
							suffix_size = 2;
							
						}else if(
							full_number_view.ends_with("llu")
							|| full_number_view.ends_with("llU")
							|| full_number_view.ends_with("LLu")
							|| full_number_view.ends_with("LLU")
						){
							int_type = MacroToken::IntValue::Type::C_ULONG_LONG;
							suffix_size = 3;

						}else if(full_number_view.ends_with("ll") || full_number_view.ends_with("LL")){
							int_type = MacroToken::IntValue::Type::C_LONG_LONG;
							suffix_size = 2;

						}else if(
							full_number_view.ends_with("lu")
							|| full_number_view.ends_with("lU")
							|| full_number_view.ends_with("Lu")
							|| full_number_view.ends_with("LU")
						){
							int_type = MacroToken::IntValue::Type::C_ULONG;
							suffix_size = 2;

						}else if(full_number_view.ends_with("l") || full_number_view.ends_with("L")){
							int_type = MacroToken::IntValue::Type::C_LONG;
							suffix_size = 1;

						}else if(full_number_view.ends_with("u") || full_number_view.ends_with("U")){
							int_type = MacroToken::IntValue::Type::C_UINT;
							suffix_size = 1;

						}else{
							// nothing...
						}

						uint64_t token_value;
						const auto [ptr, ec] = std::from_chars(
							full_number_view.data() + num_start_index,
							full_number_view.data() + full_number_view.size() - suffix_size,
							token_value,
							base
						);

						if(ptr != full_number_view.data() + full_number_view.size() - suffix_size){
							return evo::resultError;
						}
						if(ec != std::errc()){ return evo::resultError; }

						tokens.emplace_back(MacroToken::Kind::LITERAL_INT, MacroToken::IntValue(token_value, int_type));
					}
				} break;

				case clang::tok::char_constant: {
					if(token.getLiteralData()[1] == '\\'){
						switch(token.getLiteralData()[2]){
							break; case '\'': tokens.emplace_back(MacroToken::Kind::LITERAL_CHAR, '\'');
							break; case '\"': tokens.emplace_back(MacroToken::Kind::LITERAL_CHAR, '\"');
							break; case '\?': tokens.emplace_back(MacroToken::Kind::LITERAL_CHAR, '\?');
							break; case '\\': tokens.emplace_back(MacroToken::Kind::LITERAL_CHAR, '\\');
							break; case '\a': tokens.emplace_back(MacroToken::Kind::LITERAL_CHAR, '\a');
							break; case '\b': tokens.emplace_back(MacroToken::Kind::LITERAL_CHAR, '\b');
							break; case '\f': tokens.emplace_back(MacroToken::Kind::LITERAL_CHAR, '\f');
							break; case '\n': tokens.emplace_back(MacroToken::Kind::LITERAL_CHAR, '\n');
							break; case '\r': tokens.emplace_back(MacroToken::Kind::LITERAL_CHAR, '\r');
							break; case '\t': tokens.emplace_back(MacroToken::Kind::LITERAL_CHAR, '\t');
							break; case '\v': tokens.emplace_back(MacroToken::Kind::LITERAL_CHAR, '\v');
						}
					}else{
						tokens.emplace_back(MacroToken::Kind::LITERAL_CHAR, token.getLiteralData()[1]);
					}
				} break;

				case clang::tok::wide_char_constant: {
					if(token.getLiteralData()[1] == '\\'){
						switch(token.getLiteralData()[2]){
							break; case '\'': tokens.emplace_back(MacroToken::Kind::LITERAL_WIDE_CHAR, L'\'');
							break; case '\"': tokens.emplace_back(MacroToken::Kind::LITERAL_WIDE_CHAR, L'\"');
							break; case '\?': tokens.emplace_back(MacroToken::Kind::LITERAL_WIDE_CHAR, L'\?');
							break; case '\\': tokens.emplace_back(MacroToken::Kind::LITERAL_WIDE_CHAR, L'\\');
							break; case '\a': tokens.emplace_back(MacroToken::Kind::LITERAL_WIDE_CHAR, L'\a');
							break; case '\b': tokens.emplace_back(MacroToken::Kind::LITERAL_WIDE_CHAR, L'\b');
							break; case '\f': tokens.emplace_back(MacroToken::Kind::LITERAL_WIDE_CHAR, L'\f');
							break; case '\n': tokens.emplace_back(MacroToken::Kind::LITERAL_WIDE_CHAR, L'\n');
							break; case '\r': tokens.emplace_back(MacroToken::Kind::LITERAL_WIDE_CHAR, L'\r');
							break; case '\t': tokens.emplace_back(MacroToken::Kind::LITERAL_WIDE_CHAR, L'\t');
							break; case '\v': tokens.emplace_back(MacroToken::Kind::LITERAL_WIDE_CHAR, L'\v');
						}
					}else{
						tokens.emplace_back(MacroToken::Kind::LITERAL_WIDE_CHAR, wchar_t(token.getLiteralData()[1]));
					}
				} break;

				case clang::tok::string_literal: {
					tokens.emplace_back(
						MacroToken::Kind::LITERAL_STRING,
						std::string_view(token.getLiteralData() + 1, token.getLength() - 2)
					);
				} break;

				case clang::tok::wide_string_literal: {
					tokens.emplace_back(
						MacroToken::Kind::LITERAL_WIDE_STRING,
						std::string_view(token.getLiteralData() + 2, token.getLength() - 3)
					);
				} break;

				case clang::tok::l_paren: {
					tokens.emplace_back(MacroToken::lookupKind("("));
				} break;

				case clang::tok::r_paren: {
					tokens.emplace_back(MacroToken::lookupKind(")"));
				} break;

				case clang::tok::l_square: {
					tokens.emplace_back(MacroToken::lookupKind("["));
				} break;

				case clang::tok::r_square: {
					tokens.emplace_back(MacroToken::lookupKind("]"));
				} break;

				case clang::tok::l_brace: {
					tokens.emplace_back(MacroToken::lookupKind("{"));
				} break;

				case clang::tok::r_brace: {
					tokens.emplace_back(MacroToken::lookupKind("}"));
				} break;

				case clang::tok::period: {
					tokens.emplace_back(MacroToken::lookupKind("."));
				} break;

				case clang::tok::amp: {
					tokens.emplace_back(MacroToken::lookupKind("&"));
				} break;

				case clang::tok::ampamp: {
					tokens.emplace_back(MacroToken::lookupKind("&&"));
				} break;

				case clang::tok::star: {
					tokens.emplace_back(MacroToken::lookupKind("*"));
				} break;

				case clang::tok::plus: {
					tokens.emplace_back(MacroToken::lookupKind("+"));
				} break;

				case clang::tok::minus: {
					tokens.emplace_back(MacroToken::lookupKind("-"));
				} break;

				case clang::tok::tilde: {
					tokens.emplace_back(MacroToken::lookupKind("~"));
				} break;

				case clang::tok::exclaim: {
					tokens.emplace_back(MacroToken::lookupKind("!"));
				} break;

				case clang::tok::slash: {
					tokens.emplace_back(MacroToken::lookupKind("/"));
				} break;

				case clang::tok::percent: {
					tokens.emplace_back(MacroToken::lookupKind("%"));
				} break;

				case clang::tok::less: {
					tokens.emplace_back(MacroToken::lookupKind("<"));
				} break;

				case clang::tok::lessless: {
					tokens.emplace_back(MacroToken::lookupKind("<<"));
				} break;

				case clang::tok::lessequal: {
					tokens.emplace_back(MacroToken::lookupKind("<="));
				} break;

				case clang::tok::greater: {
					tokens.emplace_back(MacroToken::lookupKind(">"));
				} break;

				case clang::tok::greatergreater: {
					tokens.emplace_back(MacroToken::lookupKind(">>"));
				} break;

				case clang::tok::greaterequal: {
					tokens.emplace_back(MacroToken::lookupKind(">="));
				} break;

				case clang::tok::caret: {
					tokens.emplace_back(MacroToken::lookupKind("^"));
				} break;

				case clang::tok::pipe: {
					tokens.emplace_back(MacroToken::lookupKind("|"));
				} break;

				case clang::tok::pipepipe: {
					tokens.emplace_back(MacroToken::lookupKind("||"));
				} break;

				case clang::tok::question: {
					tokens.emplace_back(MacroToken::lookupKind("?"));
				} break;

				case clang::tok::colon: {
					tokens.emplace_back(MacroToken::lookupKind(":"));
				} break;

				case clang::tok::equalequal: {
					tokens.emplace_back(MacroToken::lookupKind("=="));
				} break;

				case clang::tok::coloncolon: {
					tokens.emplace_back(MacroToken::lookupKind("::"));
				} break;

				case clang::tok::kw_char: {
					tokens.emplace_back(MacroToken::Kind::TYPE_CHAR);
				} break;

				case clang::tok::kw_const: {
					tokens.emplace_back(MacroToken::Kind::KEYWORD_CONST);
				} break;

				case clang::tok::kw_double: {
					tokens.emplace_back(MacroToken::Kind::TYPE_DOUBLE);
				} break;

				case clang::tok::kw_float: {
					tokens.emplace_back(MacroToken::Kind::TYPE_FLOAT);
				} break;

				case clang::tok::kw_int: {
					tokens.emplace_back(MacroToken::Kind::TYPE_INT);
				} break;

				case clang::tok::kw_long: {
					tokens.emplace_back(MacroToken::Kind::TYPE_LONG);
				} break;

				case clang::tok::kw_short: {
					tokens.emplace_back(MacroToken::Kind::TYPE_SHORT);
				} break;

				case clang::tok::kw_sizeof: {
					tokens.emplace_back(MacroToken::Kind::KEYWORD_SIZEOF);
				} break;

				case clang::tok::kw_signed: {
					tokens.emplace_back(MacroToken::Kind::TYPE_SIGNED);
				} break;

				case clang::tok::kw_unsigned: {
					tokens.emplace_back(MacroToken::Kind::TYPE_UNSIGNED);
				} break;

				case clang::tok::kw_void: {
					tokens.emplace_back(MacroToken::Kind::TYPE_VOID);
				} break;

				case clang::tok::kw_bool: {
					tokens.emplace_back(MacroToken::Kind::TYPE_BOOL);
				} break;

				case clang::tok::kw_false: {
					tokens.emplace_back(MacroToken::Kind::LITERAL_BOOL, false);
				} break;

				case clang::tok::kw_true: {
					tokens.emplace_back(MacroToken::Kind::LITERAL_BOOL, true);
				} break;

				case clang::tok::kw_nullptr: case clang::tok::kw___null: {
					tokens.emplace_back(MacroToken::Kind::KEYWORD_NULLPTR);
				} break;

				case clang::tok::kw___int64: {
					tokens.emplace_back(MacroToken::Kind::TYPE_I64);
				} break;

				default: {
					return evo::resultError;
				} break;
			}
		}

		return tokens;
	}





	auto extract_macros(const clang::Preprocessor& preprocessor, const clang::SourceManager& source_manager, API& api)
	-> void {
		for(const auto& [identifier_info, macro_state] : preprocessor.macros()){
			const clang::MacroDirective* macro_directive = macro_state.getLatest();
			if(macro_directive->isDefined() == false){ continue; }

			const clang::MacroDirective::DefInfo macro_definition = macro_directive->getDefinition();
			const clang::MacroInfo& macro_info = *macro_definition.getMacroInfo();


			const clang::PresumedLoc presumed_loc = source_manager.getPresumedLoc(macro_info.getDefinitionLoc());

			auto path = std::filesystem::path();
			uint32_t line = 0;
			uint32_t collumn = 0;

			if(presumed_loc.isValid()){
				path = std::filesystem::path(std::string(presumed_loc.getFilename()));
				line = uint32_t(presumed_loc.getLine());
				collumn = uint32_t(presumed_loc.getColumn());
			}


			evo::Result<evo::SmallVector<MacroToken>> macro_tokens = tokenize_macro(macro_info);
			if(macro_tokens.isError()){
				api.addMacro(
					std::string(identifier_info->getName()),
					MacroExprBuffer::createNone(),
					std::move(path),
					line,
					collumn
				);
				continue;
			}

			api.addMacro(
				std::string(identifier_info->getName()),
				parse_macro(macro_tokens.value(), api),
				std::move(path),
				line,
				collumn
			);
		}
	}


	
}