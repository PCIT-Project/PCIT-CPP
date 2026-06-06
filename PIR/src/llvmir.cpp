////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/llvmir.hpp"


#include "../include/Module.hpp"
#include "../include/InstrReader.hpp"
#include "./PIRToLLVMIR.hpp"

#include <llvm_interface.hpp>


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::pir{

	struct LoweringData{
		llvmint::LLVMContext context{};
		llvmint::Module module{};

		LoweringData(
			const Module& pir_module,
			OptMode opt_mode,
			bool add_debug_info,
			llvm::LLVMContext* llvm_context,
			evo::SmallVector<llvm::Module*>&& modules
		){
			this->context.init(llvm_context);
			EVO_DEFER([&](){ if(llvm_context != nullptr){ this->context.steal(); } });

			this->module.init(pir_module.getName(), this->context);

			const llvmint::Module::OptLevel opt_level = [&](){
				switch(opt_mode){
					case OptMode::NONE:            return llvmint::Module::OptLevel::NONE;
					case OptMode::SPEED_MINOR:     return llvmint::Module::OptLevel::LESS;
					case OptMode::SPEED:           return llvmint::Module::OptLevel::DEFAULT;
					case OptMode::SPEED_AGRESSIVE: return llvmint::Module::OptLevel::AGGRESSIVE;
					case OptMode::SIZE:            return llvmint::Module::OptLevel::DEFAULT;
					case OptMode::SIZE_AGRESSIVE:  return llvmint::Module::OptLevel::DEFAULT;
				}

				evo::unreachable();
			}();


			//////////////////////////////////////////////////////////////////////
			// 

			const std::string data_layout_error = this->module.setTargetAndDataLayout(
				pir_module.getTarget(),
				llvmint::Module::Relocation::DEFAULT,
				llvmint::Module::CodeSize::DEFAULT,
				opt_level,
				false // is_jit
			);

			evo::debugAssert(data_layout_error.empty(), data_layout_error);

			// 
			//////////////////////////////////////////////////////////////////////



			auto lowerer = PIRToLLVMIR(pir_module, this->context, this->module, add_debug_info);

			if(add_debug_info){
				lowerer.addModuleLevelDebugInfo();
			}

			lowerer.lower();

			switch(opt_mode){
				break; case OptMode::NONE:            // do nothing...
				break; case OptMode::SPEED_MINOR:     this->module.optimize(llvmint::Module::OptMode::O1);
				break; case OptMode::SPEED:           this->module.optimize(llvmint::Module::OptMode::O2);
				break; case OptMode::SPEED_AGRESSIVE: this->module.optimize(llvmint::Module::OptMode::O3);
				break; case OptMode::SIZE:            this->module.optimize(llvmint::Module::OptMode::OS);
				break; case OptMode::SIZE_AGRESSIVE:  this->module.optimize(llvmint::Module::OptMode::OZ);
			}



			///////////////////////////////////
			// link

			for(llvm::Module* llvm_module : modules){
				this->module.merge(llvm_module);
			}
		}


		~LoweringData(){
			if(this->module.isInitialized()){ this->module.deinit(); }
			if(this->context.isInitialized()){ this->context.deinit(); }
		}
	};


	

	auto lowerToLLVMIR(
		const Module& module,
		OptMode opt_mode,
		bool add_debug_info,
		llvm::LLVMContext* llvm_context,
		evo::SmallVector<llvm::Module*>&& modules
	) -> std::string {
		auto lowering_data = LoweringData(module, opt_mode, add_debug_info, llvm_context, std::move(modules));
		return lowering_data.module.print();
	}


	auto lowerToAssembly(
		const Module& module,
		OptMode opt_mode,
		bool add_debug_info,
		llvm::LLVMContext* llvm_context,
		evo::SmallVector<llvm::Module*>&& modules
	) -> evo::Result<std::string> {
		auto lowering_data = LoweringData(module, opt_mode, add_debug_info, llvm_context, std::move(modules));
		return lowering_data.module.lowerToAssembly();
	}

	auto lowerToObject(
		const Module& module,
		OptMode opt_mode,
		bool add_debug_info,
		llvm::LLVMContext* llvm_context,
		evo::SmallVector<llvm::Module*>&& modules
	) -> evo::Result<std::vector<evo::byte>> {
		auto lowering_data = LoweringData(module, opt_mode, add_debug_info, llvm_context, std::move(modules));
		return lowering_data.module.lowerToObject();
	}
	

}