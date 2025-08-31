////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/LLVMContext.h"

#include "../include/init.h"

#include <LLVM.h>


namespace pcit::llvmint{

	
	auto LLVMContext::init() -> void {
		evo::debugAssert(this->isInitialized() == false, "LLVMContext is already initialized");

		if(llvmint::isInitialized() == false){ llvmint::init(); }

		this->_native = new llvm::LLVMContext();
	}

	auto LLVMContext::init(llvm::LLVMContext* context) -> void {
		evo::debugAssert(this->isInitialized() == false, "LLVMContext is already initialized");

		if(llvmint::isInitialized() == false){ llvmint::init(); }

		if(context == nullptr){
			this->_native = new llvm::LLVMContext();
		}else{
			this->_native = context;
		}
	}

	auto LLVMContext::deinit() -> void {
		evo::debugAssert(this->isInitialized(), "Cannot deinit LLVMContext when not initialized");

		delete this->_native;
		this->_native = nullptr;
	}
	
}