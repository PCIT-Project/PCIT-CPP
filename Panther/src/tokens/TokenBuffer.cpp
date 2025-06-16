////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../../include/tokens/TokenBuffer.h"


#include "../source/Source.h"

namespace pcit::panther{

	TokenBuffer::~TokenBuffer(){
		for(char* string_literal : this->string_literals){
			delete string_literal;
		}
	}

	

	auto TokenBuffer::createToken(Token::Kind kind, Token::Location location) -> Token::ID {
		evo::debugAssert(this->is_locked == false, "Cannot add tokens as buffer is locked");

		this->tokens.emplace_back(kind);
		this->token_locations.emplace_back(location);
		return Token::ID(uint32_t(this->tokens.size()));
	}


	auto TokenBuffer::createToken(Token::Kind kind, Token::Location location, bool value) -> Token::ID {
		evo::debugAssert(this->is_locked == false, "Cannot add tokens as buffer is locked");

		this->tokens.emplace_back(kind, value);
		this->token_locations.emplace_back(location);
		return Token::ID(uint32_t(this->tokens.size()));
	}

	auto TokenBuffer::createToken(Token::Kind kind, Token::Location location, char value) -> Token::ID {
		evo::debugAssert(this->is_locked == false, "Cannot add tokens as buffer is locked");

		this->tokens.emplace_back(kind, value);
		this->token_locations.emplace_back(location);
		return Token::ID(uint32_t(this->tokens.size()));
	}

	auto TokenBuffer::createToken(Token::Kind kind, Token::Location location, uint64_t value) -> Token::ID {
		evo::debugAssert(this->is_locked == false, "Cannot add tokens as buffer is locked");

		this->tokens.emplace_back(kind, value);
		this->token_locations.emplace_back(location);
		return Token::ID(uint32_t(this->tokens.size()));
	}

	auto TokenBuffer::createToken(Token::Kind kind, Token::Location location, evo::float64_t value) -> Token::ID {
		evo::debugAssert(this->is_locked == false, "Cannot add tokens as buffer is locked");

		this->tokens.emplace_back(kind, value);
		this->token_locations.emplace_back(location);
		return Token::ID(uint32_t(this->tokens.size()));
	}

	auto TokenBuffer::createToken(Token::Kind kind, Token::Location location, std::string_view value) -> Token::ID {
		evo::debugAssert(this->is_locked == false, "Cannot add tokens as buffer is locked");

		this->tokens.emplace_back(kind, value);
		this->token_locations.emplace_back(location);
		return Token::ID(uint32_t(this->tokens.size()));
	}

	auto TokenBuffer::createToken(Token::Kind kind, Token::Location location, const std::string& value) -> Token::ID {
		evo::debugAssert(this->is_locked == false, "Cannot add tokens as buffer is locked");
		
		char* new_string = this->string_literals.emplace_back(new char[value.size() + 1]);
		std::memcpy(new_string, value.data(), value.size() + 1);
		return this->createToken(kind, location, std::string_view(new_string, value.size()));
	}



	auto TokenBuffer::get(Token::ID id) const -> const Token& {
		return this->tokens[id];
	}

	auto TokenBuffer::get(Token::ID id) -> Token& {
		evo::debugAssert(this->isLocked() == false, "Cannot get mutable token when TokenBuffer is locked");
		return this->tokens[id];
	}


	auto TokenBuffer::getLocation(Token::ID id) const -> const Token::Location& {
		return this->token_locations[id];
	};

	auto TokenBuffer::getSourceLocation(Token::ID id, Source::ID source_id) const -> SourceLocation {
		const Token::Location& source_location = this->token_locations[id];
		return SourceLocation(
			source_id,
			source_location.lineStart,
			source_location.lineEnd,
			source_location.collumnStart,
			source_location.collumnEnd
		);
	};

}