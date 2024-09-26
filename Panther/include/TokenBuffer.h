//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once

#include <deque>

#include <Evo.h>
#include <PCIT_core.h>

#include "./source_data.h"
#include "./Token.h"

namespace pcit::panther{


	class TokenBuffer{
		public:
			TokenBuffer() = default;
			~TokenBuffer();

			TokenBuffer(const TokenBuffer& rhs) = delete;
			auto operator=(const TokenBuffer& rhs) = delete;
			
			TokenBuffer(TokenBuffer&& rhs) = delete;
			auto operator=(TokenBuffer&& rhs) = delete;


			auto createToken(Token::Kind kind, Token::Location location) -> Token::ID;
			auto createToken(Token::Kind kind, Token::Location location, bool value) -> Token::ID;
			auto createToken(Token::Kind kind, Token::Location location, uint64_t value) -> Token::ID;
			auto createToken(Token::Kind kind, Token::Location location, float64_t value) -> Token::ID;
			auto createToken(Token::Kind kind, Token::Location location, std::string_view value) -> Token::ID;
			auto createToken(Token::Kind kind, Token::Location location, const std::string& value) -> Token::ID;


			EVO_NODISCARD auto get(Token::ID id) const -> const Token&;
			EVO_NODISCARD auto get(Token::ID id)       ->       Token&;

			EVO_NODISCARD auto operator[](Token::ID id) const -> const Token& { return this->get(id); }
			EVO_NODISCARD auto operator[](Token::ID id)       ->       Token& { return this->get(id); }

			EVO_NODISCARD auto getLocation(Token::ID id) const -> const Token::Location&;
			EVO_NODISCARD auto getSourceLocation(Token::ID id, SourceID source) const -> SourceLocation;


			EVO_NODISCARD auto size() const -> size_t { return this->tokens.size(); }

			EVO_NODISCARD auto begin() const -> Token::ID::Iterator {
				return Token::ID::Iterator(Token::ID(0));
			}

			EVO_NODISCARD auto end() const -> Token::ID::Iterator {
				return Token::ID::Iterator(Token::ID(uint32_t(this->tokens.size())));
			}


			auto lock() -> void { this->is_locked = true; }
			EVO_NODISCARD auto isLocked() const -> bool { return this->is_locked; }

		
		private:
			core::LinearStepAlloc<Token, Token::ID> tokens{};
			core::LinearStepAlloc<Token::Location, Token::ID> token_locations{};
			std::vector<char*> string_literals{};
			bool is_locked = false;

			friend Token;
	};


}
