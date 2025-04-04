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
		llvmint::OrcJIT orc_jit{};
	};


	auto JITEngine::init(const InitConfig& config) -> evo::Expected<void, evo::SmallVector<std::string>> {
		evo::debugAssert(this->isInitialized() == false, "JITEngine already initialized");

		this->data = new Data();

		evo::Expected<void, evo::SmallVector<std::string>> res = this->data->orc_jit.init(
			llvmint::OrcJIT::InitConfig{
				.allowDefaultSymbolLinking = config.allowDefaultSymbolLinking,
			}
		);

		if(res.has_value() == false){
			delete this->data;
			this->data = nullptr;
			return res;
		}

		return {};
	}


	auto JITEngine::deinit() -> void {
		evo::debugAssert(this->isInitialized(), "JITEngine not initialized");

		this->data->orc_jit.deinit();

		delete this->data;
		this->data = nullptr;
	}



	auto JITEngine::addModule(const class Module& module) -> evo::Expected<void, evo::SmallVector<std::string>> {
		evo::debugAssert(this->isInitialized(), "JITEngine not initialized");

		auto llvm_context = llvmint::LLVMContext();
		llvm_context.init();

		auto llvm_module = llvmint::Module();
		llvm_module.init(module.getName(), llvm_context);

		auto lowerer = PIRToLLVMIR(module, llvm_context, llvm_module);
		lowerer.lower();

		return this->data->orc_jit.addModule(std::move(llvm_context), std::move(llvm_module));
	}


	auto JITEngine::addModuleSubset(const class Module& module, const ModuleSubsets& module_subsets)
	-> evo::Expected<void, evo::SmallVector<std::string>> {
		evo::debugAssert(this->isInitialized(), "JITEngine not initialized");

		auto llvm_context = llvmint::LLVMContext();
		llvm_context.init();

		auto llvm_module = llvmint::Module();
		llvm_module.init(module.getName(), llvm_context);

		auto lowerer = PIRToLLVMIR(module, llvm_context, llvm_module);
		lowerer.lowerSubset(PIRToLLVMIR::Subsets{
			.structs     = nullptr,
			.globalVars  = nullptr,
			.externFuncs = module_subsets.externFuncs,
			.funcDecls   = module_subsets.funcDecls,
			.funcs       = module_subsets.funcs,
		});

		return this->data->orc_jit.addModule(std::move(llvm_context), std::move(llvm_module));
	}





	auto JITEngine::runFunc(const Module& module, Function::ID func_id, std::span<core::GenericValue> args)
	-> core::GenericValue {
		evo::debugAssert(this->isInitialized(), "JITEngine not initialized");

		const Function& func = module.getFunction(func_id);

		auto return_value = core::GenericValue();

		this->getFuncPtr<void(*)(core::GenericValue*)>(func.getName())(&return_value);

		return return_value;
	}




	auto JITEngine::get_func_ptr(std::string_view name) -> void* {
		evo::debugAssert(this->isInitialized(), "JITEngine not initialized");

		return this->data->orc_jit.lookupFunc(name);
	}



	auto JITEngine::registerFuncs(evo::ArrayProxy<FuncRegisterInfo> func_register_infos)
	-> evo::Expected<void, evo::SmallVector<std::string>> {
		evo::debugAssert(this->isInitialized(), "JITEngine not initialized");

		return this->data->orc_jit.registerFuncs(
			evo::bitCast<evo::ArrayProxy<llvmint::OrcJIT::FuncRegisterInfo>>(func_register_infos)
		);
	}

	auto JITEngine::registerFunc(std::string_view name, void* func_call_address)
	-> evo::Expected<void, evo::SmallVector<std::string>> {
		evo::debugAssert(this->isInitialized(), "JITEngine not initialized");

		return this->data->orc_jit.registerFunc(name, func_call_address);
	}


	

}