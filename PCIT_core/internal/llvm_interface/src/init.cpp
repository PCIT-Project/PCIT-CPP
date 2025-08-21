////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/init.h"

#include <LLVM.h>


namespace pcit::llvmint{


	bool is_initialized = false;
	
	auto init() -> void {
		evo::debugAssert(isInitialized() == false, "LLVM was already initialized");

		// TODO(FUTURE): is this all needed?
		
		// LLVMLinkInInterpreter();
		// auto force = ForceMCJITLinking();
		// LLVMLinkInMCJIT();
		llvm::InitializeNativeTarget();
		llvm::InitializeNativeTargetAsmPrinter();
		llvm::InitializeNativeTargetAsmParser();


		// llvm::InitializeAllTargetInfos();
		// llvm::InitializeAllTargets();
		// llvm::InitializeAllTargetMCs();
		// llvm::InitializeAllAsmParsers();
		// llvm::InitializeAllAsmPrinters();
	}

	
	auto isInitialized() -> bool {
		return is_initialized;
	}
	
}