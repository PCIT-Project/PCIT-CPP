//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "../include/LLVMContext.h"

#include <LLVM.h>


namespace pcit::llvmint{

	
	auto LLVMContext::init() -> void {
		evo::debugAssert(this->isInitialized() == false, "LLVMContext is already initialized");

		// LLVMLinkInInterpreter();
		// auto force = ForceMCJITLinking();
		LLVMLinkInMCJIT();
		llvm::InitializeNativeTarget();
		llvm::InitializeNativeTargetAsmPrinter();
		llvm::InitializeNativeTargetAsmParser();


		// llvm::InitializeAllTargetInfos();
		// llvm::InitializeAllTargets();
		// llvm::InitializeAllTargetMCs();
		// llvm::InitializeAllAsmParsers();
		// llvm::InitializeAllAsmPrinters();


		this->_native = new llvm::LLVMContext();
	}

	auto LLVMContext::deinit() -> void {
		evo::debugAssert(this->isInitialized(), "Cannot deinit LLVMContext when not initialized");

		delete this->_native;
		this->_native = nullptr;
	}
	
}