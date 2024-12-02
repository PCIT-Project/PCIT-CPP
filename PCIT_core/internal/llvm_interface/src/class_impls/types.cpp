////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


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

	ArrayType::operator Type() const {
		return Type(static_cast<llvm::Type*>(this->native()));
	}

	StructType::operator Type() const {
		return Type(static_cast<llvm::Type*>(this->native()));
	}
	
		
}