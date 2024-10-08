//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


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

	CallInst::operator Value() const { return Value(static_cast<llvm::Value*>(this->native()));	}

	LoadInst::operator Value() const { return Value(static_cast<llvm::Value*>(this->native()));	}

	GlobalVariable::operator Value() const { return Value(static_cast<llvm::Value*>(this->native()));	}
	auto GlobalVariable::getType() const -> Type {
		return Type(this->native()->getValueType());
	}
		
}