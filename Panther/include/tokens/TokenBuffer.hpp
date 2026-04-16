////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <deque>

#include <Evo.hpp>
#include <PCIT_core.hpp>

#include "../source/source_data.hpp"
#include "./Token.hpp"

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
			auto createToken(Token::Kind kind, Token::Location location, char value) -> Token::ID;
			auto createToken(Token::Kind kind, Token::Location location, uint64_t value) -> Token::ID;
			auto createToken(Token::Kind kind, Token::Location location, evo::float64_t value) -> Token::ID;
			auto createToken(Token::Kind kind, Token::Location location, std::string_view value) -> Token::ID;
			auto createToken(Token::Kind kind, Token::Location location, const std::string& value) -> Token::ID;


			[[nodiscard]] auto get(Token::ID id) const -> const Token&;
			[[nodiscard]] auto get(Token::ID id)       ->       Token&;

			[[nodiscard]] auto operator[](Token::ID id) const -> const Token& { return this->get(id); }
			[[nodiscard]] auto operator[](Token::ID id)       ->       Token& { return this->get(id); }

			[[nodiscard]] auto getLocation(Token::ID id) const -> const Token::Location&;
			[[nodiscard]] auto getSourceLocation(Token::ID id, SourceID source) const -> SourceLocation;


			[[nodiscard]] auto size() const -> size_t { return this->tokens.size(); }

			[[nodiscard]] auto begin() const -> Token::ID::Iterator {
				return Token::ID::Iterator(Token::ID(0));
			}

			[[nodiscard]] auto end() const -> Token::ID::Iterator {
				return Token::ID::Iterator(Token::ID(uint32_t(this->tokens.size())));
			}


			auto lock() -> void { this->is_locked = true; }
			[[nodiscard]] auto isLocked() const -> bool { return this->is_locked; }

		
		private:
			core::LinearStepAlloc<Token, Token::ID> tokens{};
			core::LinearStepAlloc<Token::Location, Token::ID> token_locations{};
			std::vector<char*> string_literals{};
			bool is_locked = false;

			friend Token;
	};


}
