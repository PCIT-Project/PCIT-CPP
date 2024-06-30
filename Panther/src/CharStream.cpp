//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "./CharStream.h"

namespace pcit::panther{
	

	auto CharStream::peek(size_t ammount_forward) const noexcept -> char {
		evo::debugAssert(this->at_end() == false, "Cannot peek at end");
		evo::debugAssert(this->cursor + ammount_forward <= this->data.size(), "Skipping past the end of the data");
		
		return this->data[this->cursor + ammount_forward];
	};

	auto CharStream::peek_raw_ptr() const noexcept -> const char* {
		evo::debugAssert(this->at_end() == false, "Cannot peek at end");

		return &this->data[this->cursor];
	};


	auto CharStream::next() noexcept -> char {
		evo::debugAssert(this->at_end() == false, "Already at end");

		const char current_char = this->peek();
		this->skip_single();
		return current_char;
	};

	auto CharStream::skip(size_t ammount) noexcept -> void {
		evo::debugAssert(this->at_end() == false, "Already at end");
		evo::debugAssert(this->cursor + ammount <= this->data.size(), "Skipping past the end of the data");
		evo::debugAssert(ammount != 0, "Cannot skip 0 forward");

		for(size_t i = 0; i < ammount; i+=1){
			this->skip_single();
		}
	};


	auto CharStream::skip_single() noexcept -> void {
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
	};


};