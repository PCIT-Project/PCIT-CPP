////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/JITEngine.h"

#include "../include/Module.h"
#include "./PIRToLLVMIR.h"

#include <llvm_interface.h>


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

namespace pcit::pir{
	
	struct JITEngine::Data{
		llvmint::LLVMContext llvm_context{};
		llvmint::Module module{};

		llvmint::OrcJIT orc_jit{};
	};


	auto JITEngine::init(const InitConfig& config) -> evo::Result<> {
		evo::debugAssert(this->isInitialized() == false, "LegacyJITEngine already initialized");

		this->data = new Data();

		this->data->llvm_context.init();	
		this->data->module.init("PIR-JITEngine", this->data->llvm_context);

		const std::string data_layout_error = this->data->module.setTargetAndDataLayout(
			core::getCurrentOS(),
			core::getCurrentArchitecture(),
			llvmint::Module::Relocation::DEFAULT,
			llvmint::Module::CodeSize::DEFAULT,
			llvmint::Module::OptLevel::NONE,
			false
		);

		evo::Assert(data_layout_error.empty(), "Failed to set data layout with message: {}", data_layout_error);


		return this->data->orc_jit.init(llvmint::OrcJIT::InitConfig{
			.allowDefaultSymbolLinking = config.allowDefaultSymbolLinking,
		});
	}


	auto JITEngine::deinit() -> void {
		evo::debugAssert(this->isInitialized(), "LegacyJITEngine not initialized");

		this->data->orc_jit.deinit();

		this->data->module.deinit();
		this->data->llvm_context.deinit();

		delete this->data;
		this->data = nullptr;
	}



	auto JITEngine::addModule(const class Module& module) -> evo::Result<> {
		auto llvm_context = llvmint::LLVMContext();
		llvm_context.init();

		auto llvm_module = llvmint::Module();
		llvm_module.init(module.getName(), llvm_context);

		auto lowerer = PIRToLLVMIR(module, llvm_context, llvm_module);
		lowerer.lower();

		return this->data->orc_jit.addModule(std::move(llvm_context), std::move(llvm_module));
	}

	auto JITEngine::lookupFunc(std::string_view name) -> void* {
		return this->data->orc_jit.lookupFunc(name);
	}



	auto JITEngine::registerFuncs(evo::ArrayProxy<FuncRegisterInfo> func_register_infos) -> evo::Result<> {
		return this->data->orc_jit.registerFuncs(
			evo::bitCast<evo::ArrayProxy<llvmint::OrcJIT::FuncRegisterInfo>>(func_register_infos)
		);
	}

	auto JITEngine::registerFunc(std::string_view name, void* func_call_address) -> evo::Result<> {
		return this->data->orc_jit.registerFunc(name, func_call_address);
	}


	

}