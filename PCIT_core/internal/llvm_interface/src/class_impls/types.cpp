//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "../../include/class_impls/types.h"

#include <LLVM.h>


namespace pcit::llvmint{

	FunctionType::operator Type() const {
		return Type(static_cast<llvm::Type*>(this->native()));
	}

	IntegerType::operator Type() const {
		return Type(static_cast<llvm::Type*>(this->native()));
	}

	PointerType::operator Type() const {
		return Type(static_cast<llvm::Type*>(this->native()));
	}
	
		
}