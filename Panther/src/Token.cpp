//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "../include/Token.h"

#include "../include/Source.h"

namespace pcit::panther{
	

	Token::Token(Kind _kind, Location _location, const Source& source, std::string_view val) noexcept :
		kind(_kind),
		location(_location), 
		value{.string = {uint32_t(val.data() - source.getData().data()), uint32_t(val.size())}}
		{};


	auto Token::getString(const Source& source) const noexcept -> std::string_view {
		evo::debugAssert(
			this->kind == Kind::LiteralString || this->kind == Kind::LiteralChar ||
			this->kind == Kind::Ident || this->kind == Kind::Intrinsic || this->kind == Kind::Attribute,
			"Token does not have a string value"
		);

		const char* data = source.getData().data();
		return std::string_view(
			data + this->value.string.index, data + this->value.string.index + this->value.string.length
		);
	};


};