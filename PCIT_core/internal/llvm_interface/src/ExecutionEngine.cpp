////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/ExecutionEngine.h"

#include <LLVM.h>

#include "../include/Module.h"
#include "../include/Function.h"



namespace pcit::llvmint{

	std::atomic<bool> engine_created = false;

	
	auto ExecutionEngine::createEngine(const Module& module) -> void {
		evo::debugAssert(this->hasCreatedEngine() == false, "Execution engine already created");

		[[maybe_unused]] const bool already_created = engine_created.exchange(true);
		evo::debugAssert(!already_created, "Cannot have multiple execution engines running at once");

		this->engine = llvm::EngineBuilder(module.get_clone())
			.setEngineKind(llvm::EngineKind::JIT)
			.create();

		this->engine->DisableSymbolSearching();
	};


	auto ExecutionEngine::shutdownEngine() -> void {
		evo::debugAssert(this->hasCreatedEngine(), "Execution engine is not created and cannot be shutdown");

		delete this->engine;
		this->engine = nullptr;

		engine_created = false;
	};


	auto ExecutionEngine::registerFunction(const Function& func, void* func_call) -> void {
		this->engine->addGlobalMapping(static_cast<const llvm::GlobalValue*>(func.native()), func_call);
	}

	auto ExecutionEngine::registerFunction(std::string_view func, void* func_call_address) -> void {
		this->engine->addGlobalMapping(func, uint64_t(func_call_address));
	}



	//////////////////////////////////////////////////////////////////////
	// linked functions

	static core::Printer* runtime_funcs_printer = nullptr;
	static std::jmp_buf panic_jump;

	auto ExecutionEngine::getPanicJump() -> std::jmp_buf& {
		return panic_jump;
	}

	auto ExecutionEngine::setupLinkedFuncs(core::Printer& printer) -> void {
		runtime_funcs_printer = &printer;

		this->registerFunction("PTHR._printHelloWorld", []() -> void {
			runtime_funcs_printer->println("Hello world, I'm Panther!");
		});

		this->registerFunction("PTHR.panic", [](const char* msg) -> void {
			runtime_funcs_printer->printlnRed("<PTHR> Execution Panic: \"{}\"", msg);
			std::longjmp(panic_jump, true);
		});

		this->registerFunction(
			"PTHR.panic_with_location",
			[](const char* msg, uint32_t source_id, uint32_t line, uint32_t collumn) -> void {
				runtime_funcs_printer->printlnRed(
					"<PTHR> Execution Panic ({}:{}:{}): \"{}\"", source_id, line, collumn, msg
				);
				std::longjmp(panic_jump, true);
			}
		);
	}


	//////////////////////////////////////////////////////////////////////
	// internal

	auto ExecutionEngine::get_func_address(std::string_view func_name) const -> uint64_t {
		const std::string func_name_str = std::string(func_name);
		return this->engine->getFunctionAddress(func_name_str);
	};
	
}