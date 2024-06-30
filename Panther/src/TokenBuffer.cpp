//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "../include/TokenBuffer.h"

namespace pcit::panther{
	

	auto TokenBuffer::createToken(Token::Kind kind, Token::Location location) noexcept -> Token::ID {
		this->tokens.emplace_back(kind, location);
		return Token::ID(uint32_t(this->tokens.size()));
	};


	auto TokenBuffer::createToken(Token::Kind kind, Token::Location location, bool value) noexcept -> Token::ID {
		this->tokens.emplace_back(kind, location, value);
		return Token::ID(uint32_t(this->tokens.size()));
	};

	auto TokenBuffer::createToken(Token::Kind kind, Token::Location location, uint64_t value) noexcept -> Token::ID {
		this->tokens.emplace_back(kind, location, value);
		return Token::ID(uint32_t(this->tokens.size()));
	};

	auto TokenBuffer::createToken(Token::Kind kind, Token::Location location, float64_t value) noexcept -> Token::ID {
		this->tokens.emplace_back(kind, location, value);
		return Token::ID(uint32_t(this->tokens.size()));
	};

	auto TokenBuffer::createToken(Token::Kind kind, Token::Location location, std::string&& value) noexcept
	-> Token::ID {
		const std::unique_ptr<std::string>& saved_str = 
			this->string_literals.emplace_back(std::make_unique<std::string>(value));

		this->tokens.emplace_back(kind, location, std::string_view(saved_str->begin(), saved_str->end()));
		return Token::ID(uint32_t(this->tokens.size()));
	};



	auto TokenBuffer::get(Token::ID id) const noexcept -> const Token& {
		return this->tokens[id.get()];
	};

	auto TokenBuffer::get(Token::ID id) noexcept -> Token& {
		evo::debugAssert(this->isLocked() == false, "Cannot get mutable token when TokenBuffer is locked");
		return this->tokens[id.get()];
	};



};