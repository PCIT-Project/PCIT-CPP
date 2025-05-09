////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>
#include <PCIT_core.h>

#include "../../include/Context.h"
#include "../../include/source/source_data.h"
#include "../../include/tokens/TokenBuffer.h"
#include "./CharStream.h"

namespace pcit::panther{


	class Tokenizer{
		public:
			Tokenizer(Context& _context, Source::ID source_id) :
				context(_context),
				source(this->context.getSourceManager()[source_id]),
				char_stream(this->source.getData())
				{}

			~Tokenizer() = default;

			EVO_NODISCARD auto tokenize() -> evo::Result<>;

			
		private:
			// these functions return true if they consumed any of the source file
			EVO_NODISCARD auto tokenize_whitespace() -> bool;
			EVO_NODISCARD auto tokenize_comment() -> bool;
			EVO_NODISCARD auto tokenize_identifier() -> bool;
			EVO_NODISCARD auto tokenize_punctuation() -> bool;
			EVO_NODISCARD auto tokenize_operators() -> bool;
			EVO_NODISCARD auto tokenize_number_literal() -> bool;
			EVO_NODISCARD auto tokenize_string_literal() -> bool;

			auto create_token(Token::Kind kind, auto&&... value) -> void;


			EVO_NODISCARD auto file_too_big() -> bool;

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


	EVO_NODISCARD inline auto tokenize(Context& context, Source::ID source_id) -> evo::Result<> {
		auto tokenizer = Tokenizer(context, source_id);
		return tokenizer.tokenize();
	}


}
