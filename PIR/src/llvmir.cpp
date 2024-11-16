////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/llvmir.h"


#include "../include/Module.h"
#include "../include/ReaderAgent.h"
#include "./PIRToLLVMIR.h"

#include <llvm_interface.h>


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::pir{
	

	auto lowerToLLVMIR(const Module& module) -> evo::Expected<std::string, std::string> {
		auto llvm_context = llvmint::LLVMContext();
		llvm_context.init();
		EVO_DEFER([&](){ llvm_context.deinit(); });

		auto llvm_module = llvmint::Module();
		llvm_module.init(module.getName(), llvm_context);

		const std::string target_triple = llvm_module.generateTargetTriple(module.getOS(), module.getArchitecture());

		const std::string data_layout_error = llvm_module.setDataLayout(
			target_triple,
			llvmint::Module::Relocation::Default,
			llvmint::Module::CodeSize::Default,
			llvmint::Module::OptLevel::None,
			false
		);

		if(!data_layout_error.empty()){
			return evo::Unexpected<std::string>(data_layout_error);
		}

		llvm_module.setTargetTriple(target_triple);

		auto lowerer = PIRToLLVMIR(module, llvm_context, llvm_module);
		lowerer.lower();

		std::string output = llvm_module.print();

		return output;
	}
	

}