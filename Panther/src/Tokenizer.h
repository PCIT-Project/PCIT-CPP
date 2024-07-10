//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>
#include <PCIT_core.h>

#include "../include/Context.h"
#include "../include/source_data.h"
#include "../include/TokenBuffer.h"
#include "./CharStream.h"

namespace pcit::panther{


	class Tokenizer{
		public:
			Tokenizer(Context& _context, Source::ID source_id) noexcept :
				context(_context),
				source(this->context.getSourceManager().getSource(source_id)),
				char_stream(this->source.getData())
				{};

			~Tokenizer() = default;

			EVO_NODISCARD auto tokenize() noexcept -> bool;

			
		private:
			// these functions return true if they consumed any of the source file
			EVO_NODISCARD auto tokenize_whitespace() noexcept -> bool;
			EVO_NODISCARD auto tokenize_comment() noexcept -> bool;
			EVO_NODISCARD auto tokenize_identifier() noexcept -> bool;
			EVO_NODISCARD auto tokenize_punctuation() noexcept -> bool;
			EVO_NODISCARD auto tokenize_operators() noexcept -> bool;
			EVO_NODISCARD auto tokenize_number_literal() noexcept -> bool;
			EVO_NODISCARD auto tokenize_string_literal() noexcept -> bool;

			auto create_token(Token::Kind kind, auto&&... value) noexcept -> void;


			auto file_too_big() noexcept -> bool;

			auto emit_warning(auto&&... args) noexcept -> void;
			auto emit_error(auto&&... args) noexcept -> void;
			auto emit_fatal(auto&&... args) noexcept -> void;


			auto error_unrecognized_character() noexcept -> void;

	
		private:
			Context& context;
			Source& source;

			CharStream char_stream;

			uint32_t current_token_line_start;
			uint16_t current_token_collumn_start;

			bool can_continue = true;
	};


};
