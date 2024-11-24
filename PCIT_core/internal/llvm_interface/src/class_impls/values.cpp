////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../../include/class_impls/values.h"

#include <LLVM.h>


namespace pcit::llvmint{

	Alloca::operator Value() const { return Value(static_cast<llvm::Value*>(this->native()));	}
	auto Alloca::getAllocatedType() const -> Type {
		return Type(this->native()->getAllocatedType());
	}

	Constant::operator Value() const { return Value(static_cast<llvm::Value*>(this->native()));	}

	ConstantInt::operator Constant() const { return Constant(static_cast<llvm::Constant*>(this->native()));	}
	ConstantInt::operator Value() const { return Value(static_cast<llvm::Value*>(this->native()));	}

	auto CallInst::setCallingConv(CallingConv calling_conv) -> void {
		this->native()->setCallingConv(evo::to_underlying(calling_conv));
	}
	CallInst::operator Value() const { return Value(static_cast<llvm::Value*>(this->native()));	}

	LoadInst::operator Value() const { return Value(static_cast<llvm::Value*>(this->native()));	}

	auto GlobalVariable::setAlignment(unsigned alignment) -> void {
		this->native()->setAlignment(llvm::Align(alignment));
	}

	auto GlobalVariable::getType() const -> Type {
		return Type(this->native()->getValueType());
	}
	GlobalVariable::operator Value() const { return Value(static_cast<llvm::Value*>(this->native()));	}
		
}