////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./CharStream.h"

namespace pcit::panther{
	

	auto CharStream::peek(size_t ammount_forward) const -> char {
		evo::debugAssert(this->at_end() == false, "Cannot peek at end");
		evo::debugAssert(this->cursor + ammount_forward <= this->data.size(), "Skipping past the end of the data");
		
		return this->data[this->cursor + ammount_forward];
	}

	auto CharStream::peek_back(size_t ammount_backward) const -> char {
		evo::debugAssert(this->cursor >= ammount_backward, "Skipping past the beginning of the data");
		
		return this->data[this->cursor - ammount_backward];
	}

	auto CharStream::peek_raw_ptr() const -> const char* {
		evo::debugAssert(this->cursor <= this->data.size(), "Cannot peek past end");

		// This is done this way so can peek the end ptr
		return this->data.data() + this->cursor;
	}


	auto CharStream::next() -> char {
		evo::debugAssert(this->at_end() == false, "Already at end");

		const char current_char = this->peek();
		this->skip_single();
		return current_char;
	}

	auto CharStream::skip(size_t ammount) -> void {
		evo::debugAssert(this->at_end() == false, "Already at end");
		evo::debugAssert(this->cursor + ammount <= this->data.size(), "Skipping past the end of the data");
		evo::debugAssert(ammount != 0, "Cannot skip 0 forward");

		for(size_t i = 0; i < ammount; i+=1){
			this->skip_single();
		}
	}


	auto CharStream::skip_single() -> void {
		evo::debugAssert(this->at_end() == false, "Already at end");

		const char current_char = this->peek();

		if(current_char == '\n'){
			this->line += 1;
			this->collumn = 1;

		}else if(current_char == '\r'){
			this->line += 1;
			this->collumn = 1;

			if(this->at_end() == false && this->peek() == '\n'){
				this->cursor += 1;	
			}

		}else{
			this->collumn += 1;
		}

		this->cursor += 1;
	}


}