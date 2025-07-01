////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <stack>

#include <Evo.h>
#include <PCIT_core.h>

#include "../../include/tokens/TokenBuffer.h"

namespace pcit::panther{


	class TokenReader{
		public:
			TokenReader(const TokenBuffer& token_buffer) : buffer(token_buffer) {}
			~TokenReader() = default;

			EVO_NODISCARD auto at_end() const -> uint32_t {
				return this->cursor >= this->buffer.size();
			}

			EVO_NODISCARD auto peek(ptrdiff_t offset = 0) const -> Token::ID {
				const ptrdiff_t peek_location = ptrdiff_t(this->cursor) + offset;

				evo::debugAssert(peek_location >= 0, "cannot peek past beginning of buffer");
				evo::debugAssert(
					size_t(peek_location) < this->buffer.size(),
					"cannot peek past beginning of buffer (peek index: {}, buffer size: {})",
					size_t(peek_location),
					this->buffer.size()
				);

				return Token::ID(uint32_t(peek_location));
			}

			auto skip() -> void {
				evo::debugAssert(this->at_end() == false, "already at end");

				this->cursor += 1;
			}

			EVO_NODISCARD auto next() -> Token::ID {
				EVO_DEFER([&](){ this->skip(); });
				return this->peek();
			}


			EVO_NODISCARD auto get(Token::ID id) const -> const Token& {
				return this->buffer[id];
			}

			EVO_NODISCARD auto operator[](Token::ID id) const -> const Token& {
				return this->get(id);
			}

			EVO_NODISCARD auto go_back(Token::ID id) -> void {
				evo::debugAssert(id.get() < this->cursor, "id is not before current location");
				this->cursor = id.get();
			}

	
		private:
			const TokenBuffer& buffer;
			uint32_t cursor = 0;
	};


}
