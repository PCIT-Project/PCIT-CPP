////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.hpp>

#include "./MacroToken.hpp"
#include "../include/MacroExpr.hpp"



namespace pcit::clangint{

	

	class MacroParser{
		public:
			MacroParser(evo::ArrayProxy<MacroToken> tokens, class API& _api) : _tokens(tokens), api(_api) {}
			~MacroParser() = default;

			[[nodiscard]] auto parse() -> MacroExpr;


		private:
			[[nodiscard]] auto parse_expr() -> evo::Result<MacroExpr>;




			[[nodiscard]] auto at_end() const -> uint32_t {
				return this->cursor >= this->_tokens.size();
			}

			[[nodiscard]] auto peek(ptrdiff_t offset = 0) const -> const MacroToken& {
				const ptrdiff_t peek_location = ptrdiff_t(this->cursor) + offset;

				evo::debugAssert(peek_location >= 0, "cannot peek past beginning of buffer");
				evo::debugAssert(
					size_t(peek_location) < this->_tokens.size(),
					"cannot peek past beginning of buffer (peek index: {}, buffer size: {})",
					size_t(peek_location),
					this->_tokens.size()
				);

				return this->_tokens[peek_location];
			}

			auto skip() -> void {
				evo::debugAssert(this->at_end() == false, "already at end");

				this->cursor += 1;
			}

			[[nodiscard]] auto skip_token(MacroToken::Kind kind) -> evo::Result<> {
				if(this->peek().kind() != kind){ return evo::resultError; }
				this->skip();
				return evo::Result<>();
			}

			[[nodiscard]] auto next() -> const MacroToken& {
				EVO_DEFER([&](){ this->skip(); });
				return this->peek();
			}
	
		private:
			evo::ArrayProxy<MacroToken> _tokens;
			size_t cursor = 0;

			API& api;
	};


	[[nodiscard]] inline auto parse_macro(evo::ArrayProxy<MacroToken> tokens, class API& api) -> MacroExpr {
		return MacroParser(tokens, api).parse();
	}


}