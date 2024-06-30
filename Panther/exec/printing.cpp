//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "./printing.h"

namespace pthr{
	

	auto printTokens(pcit::core::Printer& printer, const panther::Source& source) noexcept -> void {
		const panther::TokenBuffer& token_buffer = source.getTokenBuffer();

		///////////////////////////////////
		// print header

		printer.printGray("------------------------------\n");

		printer.printCyan( std::format("Tokens: {}\n", source.getLocationAsString()) );

		if(token_buffer.size() == 0){
			printer.printGray("(NONE)\n");

		}else if(token_buffer.size() == 1){
			printer.printGray("(1 token)\n");

		}else{
			printer.printGray( std::format("({} tokens)\n", token_buffer.size()) );
		}


		///////////////////////////////////
		// create location strings

		auto location_strings = std::vector<std::string>();

		for(panther::Token::ID token_id : token_buffer){
			const panther::Token& token = token_buffer[token_id];
			const panther::Token::Location& location = token.getLocation();

			location_strings.emplace_back(std::format("<{}:{}>", location.lineStart, location.collumnStart));
		}

		const size_t longest_location_string_length = std::ranges::max_element(
			location_strings,
			[](const std::string& lhs, const std::string& rhs) noexcept -> bool {
				return lhs.size() < rhs.size();
			}
		)->size();

		for(std::string& str : location_strings){
			while(str.size() < longest_location_string_length){
				str += ' ';
			};

			str += ' ';
		}


		///////////////////////////////////
		// print out tokens

		for(size_t i = 0; panther::Token::ID token_id : token_buffer){
			const panther::Token& token = token_buffer[token_id];
			
			printer.printGray(location_strings[i]);
			printer.printInfo(std::format("[{}]", token.getKind()));


			const std::string data_str = [&]() noexcept {
				switch(token.getKind()){
					break; case panther::Token::Ident:         return std::format(" {}", token.getString());
					break; case panther::Token::Intrinsic:     return std::format(" @{}", token.getString());
					break; case panther::Token::Attribute:     return std::format(" #{}", token.getString());

					break; case panther::Token::LiteralBool:   return std::format(" {}", token.getBool());
					break; case panther::Token::LiteralInt:    return std::format(" {}", token.getInt());
					break; case panther::Token::LiteralFloat:  return std::format(" {}", token.getFloat());
					break; case panther::Token::LiteralChar:   return std::format(" \'{}\'", token.getString());
					break; case panther::Token::LiteralString: return std::format(" \"{}\"", token.getString());

					break; default: return std::string();
				};
			}();

			printer.printMagenta(data_str + '\n');

			i += 1;
		}
		
	};


};