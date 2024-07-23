//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "../include/TokenBuffer.h"


#include "../include/Source.h"

namespace pcit::panther{
	

	auto TokenBuffer::createToken(Token::Kind kind, Token::Location location) -> Token::ID {
		this->tokens.emplace_back(kind, location);
		return Token::ID(uint32_t(this->tokens.size()));
	}


	auto TokenBuffer::createToken(Token::Kind kind, Token::Location location, bool value) -> Token::ID {
		this->tokens.emplace_back(kind, location, value);
		return Token::ID(uint32_t(this->tokens.size()));
	}

	auto TokenBuffer::createToken(Token::Kind kind, Token::Location location, uint64_t value) -> Token::ID {
		this->tokens.emplace_back(kind, location, value);
		return Token::ID(uint32_t(this->tokens.size()));
	}

	auto TokenBuffer::createToken(Token::Kind kind, Token::Location location, float64_t value) -> Token::ID {
		this->tokens.emplace_back(kind, location, value);
		return Token::ID(uint32_t(this->tokens.size()));
	}

	auto TokenBuffer::createToken(
		Token::Kind kind, Token::Location location, const Source& source, std::string_view value
	) -> Token::ID {
		this->tokens.emplace_back(kind, location, source, value);
		return Token::ID(uint32_t(this->tokens.size()));
	}

	auto TokenBuffer::createToken(
		Token::Kind kind, Token::Location location, const Source& source, std::string&& value
	) -> Token::ID {
		const std::unique_ptr<std::string>& saved_str = 
			this->string_literals.emplace_back(std::make_unique<std::string>(value));

		return this->createToken(kind, location, source, *saved_str);
	}



	auto TokenBuffer::get(Token::ID id) const -> const Token& {
		return this->tokens[id.get()];
	}

	auto TokenBuffer::get(Token::ID id) -> Token& {
		evo::debugAssert(this->isLocked() == false, "Cannot get mutable token when TokenBuffer is locked");
		return this->tokens[id.get()];
	}



}