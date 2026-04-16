////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.hpp>
#include <PCIT_core.hpp>

#include "../../include/Context.hpp"
#include "../../include/source/source_data.hpp"
#include "../../include/tokens/TokenBuffer.hpp"
#include "./CharStream.hpp"

namespace pcit::panther{


	class Tokenizer{
		public:
			Tokenizer(Context& _context, Source::ID source_id) :
				context(_context),
				source(this->context.getSourceManager()[source_id]),
				char_stream(this->source.getData())
				{}

			~Tokenizer() = default;

			[[nodiscard]] auto tokenize() -> evo::Result<>;

			
		private:
			// these functions return true if they consumed any of the source file
			[[nodiscard]] auto tokenize_whitespace() -> bool;
			[[nodiscard]] auto tokenize_comment() -> bool;
			[[nodiscard]] auto tokenize_identifier() -> bool;
			[[nodiscard]] auto tokenize_punctuation() -> bool;
			[[nodiscard]] auto tokenize_operators() -> bool;
			[[nodiscard]] auto tokenize_number_literal() -> bool;
			[[nodiscard]] auto tokenize_string_literal() -> bool;

			auto create_token(Token::Kind kind, auto&&... value) -> void;


			[[nodiscard]] auto file_too_big() -> bool;

			auto emit_warning(auto&&... args) -> void;
			auto emit_error(auto&&... args) -> void;
			auto emit_fatal(auto&&... args) -> void;


			auto error_unrecognized_character() -> void;

			auto error_line_too_big() -> void;
			auto error_collumn_too_big() -> void;

			auto get_current_location_point() -> evo::Result<Source::Location>;
			auto get_current_location_token() -> evo::Result<Source::Location>;


	
		private:
			Context& context;
			Source& source;

			CharStream char_stream;

			uint32_t current_token_line_start;
			uint32_t current_token_collumn_start;

			bool can_continue = true;
	};


	[[nodiscard]] inline auto tokenize(Context& context, Source::ID source_id) -> evo::Result<> {
		auto tokenizer = Tokenizer(context, source_id);
		return tokenizer.tokenize();
	}


}
