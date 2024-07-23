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

#include "./Token.h"

namespace pcit::panther{


	class TokenBuffer{
		public:
			TokenBuffer() = default;
			~TokenBuffer() = default;

			TokenBuffer(const TokenBuffer& rhs) = delete;
			auto operator=(const TokenBuffer& rhs) = delete;
			
			TokenBuffer(TokenBuffer&& rhs) = delete;
			auto operator=(TokenBuffer&& rhs) = delete;


			auto createToken(Token::Kind kind, Token::Location location) -> Token::ID;
			auto createToken(Token::Kind kind, Token::Location location, bool value) -> Token::ID;
			auto createToken(Token::Kind kind, Token::Location location, uint64_t value) -> Token::ID;
			auto createToken(Token::Kind kind, Token::Location location, float64_t value) -> Token::ID;
			auto createToken(
				Token::Kind kind, Token::Location location, const class Source& source, std::string_view value
			) -> Token::ID;
			auto createToken(
				Token::Kind kind, Token::Location location, const class Source& source, std::string&& value
			) -> Token::ID;


			EVO_NODISCARD auto get(Token::ID id) const -> const Token&;
			EVO_NODISCARD auto get(Token::ID id)       ->       Token&;

			EVO_NODISCARD auto operator[](Token::ID id) const -> const Token& { return this->get(id); }
			EVO_NODISCARD auto operator[](Token::ID id)       ->       Token& { return this->get(id); }

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
			evo::SmallVector<Token> tokens{};
			evo::SmallVector<std::unique_ptr<std::string>> string_literals{};
			bool is_locked = false;
	};


}
