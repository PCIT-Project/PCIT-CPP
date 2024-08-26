//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "../include/Function.h"

#include <LLVM.h>


namespace pcit::llvmint{

	//////////////////////////////////////////////////////////////////////
	// Argument

	auto Argument::setName(std::string_view name) -> void {
		this->native()->setName(name);
	}

	Argument::operator Value() const { return Value(static_cast<llvm::Value*>(this->native())); }



	//////////////////////////////////////////////////////////////////////
	// Function

	auto Function::front() const -> const BasicBlock {
		return BasicBlock(&this->native()->front());
	}

	auto Function::front() -> BasicBlock {
		return BasicBlock(&this->native()->front());
	}

	auto Function::back() const -> const BasicBlock {
		return BasicBlock(&this->native()->back());
	}

	auto Function::back() -> BasicBlock {
		return BasicBlock(&this->native()->back());
	}


	auto Function::getArg(evo::uint index) const -> Argument {
		return Argument(this->native()->getArg(index));
	}



	auto Function::setNoThrow() -> void {
		this->native()->setDoesNotThrow();
	}

	auto Function::setCallingConv(CallingConv calling_conv) -> void {
		this->native()->setCallingConv(evo::to_underlying(calling_conv));
	}



	
}