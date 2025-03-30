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

	struct LoweringData{
		llvmint::LLVMContext context{};
		llvmint::Module module{};

		~LoweringData(){
			if(this->context.isInitialized()){
				this->context.deinit();
			}
		}
	};

	static auto setup_lowering_data(const Module& module, OptMode opt_mode) -> LoweringData {
		auto lowering_data = LoweringData();

		lowering_data.context.init();

		lowering_data.module.init(module.getName(), lowering_data.context);

		const llvmint::Module::OptLevel opt_level = [&](){
			switch(opt_mode){
				case OptMode::O0: return llvmint::Module::OptLevel::NONE;
				case OptMode::O1: return llvmint::Module::OptLevel::LESS;
				case OptMode::O2: return llvmint::Module::OptLevel::DEFAULT;
				case OptMode::O3: return llvmint::Module::OptLevel::AGGRESSIVE;
				case OptMode::Os: return llvmint::Module::OptLevel::DEFAULT;
				case OptMode::Oz: return llvmint::Module::OptLevel::DEFAULT;
			}

			evo::unreachable();
		}();


		//////////////////////////////////////////////////////////////////////
		// 
		

		const std::string data_layout_error = lowering_data.module.setTargetAndDataLayout(
			module.getOS(),
			module.getArchitecture(),
			llvmint::Module::Relocation::DEFAULT,
			llvmint::Module::CodeSize::DEFAULT,
			opt_level,
			false // is_jit
		);

		evo::debugAssert(data_layout_error.empty(), data_layout_error);

		// 
		//////////////////////////////////////////////////////////////////////



		auto lowerer = PIRToLLVMIR(module, lowering_data.context, lowering_data.module);
		lowerer.lower();

		switch(opt_mode){
			break; case OptMode::O0: // do nothing...
			break; case OptMode::O1: lowering_data.module.optimize(llvmint::Module::OptMode::O1);
			break; case OptMode::O2: lowering_data.module.optimize(llvmint::Module::OptMode::O2);
			break; case OptMode::O3: lowering_data.module.optimize(llvmint::Module::OptMode::O3);
			break; case OptMode::Os: lowering_data.module.optimize(llvmint::Module::OptMode::Os);
			break; case OptMode::Oz: lowering_data.module.optimize(llvmint::Module::OptMode::Oz);
		}

		return lowering_data;
	}


	

	auto lowerToLLVMIR(const Module& module, OptMode opt_mode) -> std::string {
		LoweringData lowering_data = setup_lowering_data(module, opt_mode);
		return lowering_data.module.print();
	}


	auto lowerToAssembly(const Module& module, OptMode opt_mode) -> evo::Result<std::string> {
		LoweringData lowering_data = setup_lowering_data(module, opt_mode);
		return lowering_data.module.lowerToAssembly();
	}

	auto lowerToObject(const Module& module, OptMode opt_mode) -> evo::Result<std::vector<evo::byte>> {
		LoweringData lowering_data = setup_lowering_data(module, opt_mode);
		return lowering_data.module.lowerToObject();
	}
	

}