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
		this->tokens.emplace_back(kind);
		this->token_locations.emplace_back(location);
		return Token::ID(uint32_t(this->tokens.size()));
	}


	auto TokenBuffer::createToken(Token::Kind kind, Token::Location location, bool value) -> Token::ID {
		this->tokens.emplace_back(kind, value);
		this->token_locations.emplace_back(location);
		return Token::ID(uint32_t(this->tokens.size()));
	}

	auto TokenBuffer::createToken(Token::Kind kind, Token::Location location, uint64_t value) -> Token::ID {
		this->tokens.emplace_back(kind, value);
		this->token_locations.emplace_back(location);
		return Token::ID(uint32_t(this->tokens.size()));
	}

	auto TokenBuffer::createToken(Token::Kind kind, Token::Location location, float64_t value) -> Token::ID {
		this->tokens.emplace_back(kind, value);
		this->token_locations.emplace_back(location);
		return Token::ID(uint32_t(this->tokens.size()));
	}

	auto TokenBuffer::createToken(Token::Kind kind, Token::Location location, std::string_view value) -> Token::ID {
		this->tokens.emplace_back(kind, value);
		this->token_locations.emplace_back(location);
		return Token::ID(uint32_t(this->tokens.size()));
	}

	auto TokenBuffer::createToken(Token::Kind kind, Token::Location location, std::string&& value) -> Token::ID {
		const std::string& saved_str = this->string_literals.emplace_back(std::move(value));

		return this->createToken(kind, location, saved_str);
	}



	auto TokenBuffer::get(Token::ID id) const -> const Token& {
		return this->tokens[id.get()];
	}

	auto TokenBuffer::get(Token::ID id) -> Token& {
		evo::debugAssert(this->isLocked() == false, "Cannot get mutable token when TokenBuffer is locked");
		return this->tokens[id.get()];
	}


	auto TokenBuffer::getLocation(Token::ID id) const -> const Token::Location& {
		return this->token_locations[id.get()];
	};

	auto TokenBuffer::getSourceLocation(Token::ID id, Source::ID source_id) const -> SourceLocation {
		const Token::Location& source_location = this->token_locations[id.get()];
		return SourceLocation(
			source_id,
			source_location.lineStart,
			source_location.lineEnd,
			source_location.collumnStart,
			source_location.collumnEnd
		);
	};




}