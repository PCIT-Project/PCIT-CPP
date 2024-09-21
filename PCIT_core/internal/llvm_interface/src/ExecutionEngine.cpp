//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "../include/ExecutionEngine.h"

#include <LLVM.h>

#include "../include/Module.h"
#include "../include/Function.h"


namespace pcit::llvmint{

	
	auto ExecutionEngine::createEngine(const Module& module) -> void {
		evo::debugAssert(this->hasCreatedEngine() == false, "Execution engine already created");

		this->engine = llvm::EngineBuilder(module.get_clone())
			.setEngineKind(llvm::EngineKind::JIT)
			.create();

		this->engine->DisableSymbolSearching();
	};


	auto ExecutionEngine::shutdownEngine() -> void {
		evo::debugAssert(this->hasCreatedEngine(), "Execution engine is not created and cannot be shutdown");

		delete this->engine;
		this->engine = nullptr;
	};


	auto ExecutionEngine::registerFunction(const Function& func, void* func_call) -> void {
		this->engine->addGlobalMapping(static_cast<const llvm::GlobalValue*>(func.native()), func_call);
	}

	auto ExecutionEngine::registerFunction(std::string_view func, uint64_t func_call_address) -> void {
		this->engine->addGlobalMapping(func, func_call_address);
	}





	auto ExecutionEngine::get_func_address(std::string_view func_name) const -> uint64_t {
		const std::string func_name_str = std::string(func_name);
		return this->engine->getFunctionAddress(func_name_str);
	};




	template<>
	auto ExecutionEngine::runFunction<void>(std::string_view func_name) -> void {
		const uint64_t func_addr = this->get_func_address(func_name);

		using FuncType = void(*)(void);
		const FuncType func = (FuncType)func_addr;
		func();
	};


	
}