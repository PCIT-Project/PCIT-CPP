////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/LegacyJITEngine.h"


#include "../include/Module.h"
#include "./PIRToLLVMIR.h"

#include <llvm_interface.h>


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

namespace pcit::pir{
	
	struct LegacyJITEngine::Data{
		llvmint::LLVMContext llvm_context{};
		llvmint::Module module{};
		llvmint::ExecutionEngine execution_engine{};
	};


	auto LegacyJITEngine::init(const Module& _module) -> void {
		evo::debugAssert(this->isInitialized() == false, "LegacyJITEngine already initialized");

		this->module = &_module;

		this->data = new Data();

		this->data->llvm_context.init();	
		this->data->module.init("PIR-LegacyJITEngine", this->data->llvm_context);

		const std::string data_layout_error = this->data->module.setTargetAndDataLayout(
			core::Platform::getCurrent(),
			llvmint::Module::Relocation::DEFAULT,
			llvmint::Module::CodeSize::DEFAULT,
			llvmint::Module::OptLevel::NONE,
			false
		);

		evo::Assert(data_layout_error.empty(), "Failed to set data layout with message: {}", data_layout_error);

		auto lowerer = PIRToLLVMIR(*this->module, this->data->llvm_context, this->data->module);
		lowerer.lower();

		this->data->execution_engine.createEngine(this->data->module);
	}

	auto LegacyJITEngine::deinit() -> void {
		evo::debugAssert(this->isInitialized(), "LegacyJITEngine not initialized");

		this->data->execution_engine.shutdownEngine();
		this->data->module.deinit();
		this->data->llvm_context.deinit();

		delete this->data;
		this->data = nullptr;
	}


	auto LegacyJITEngine::registerFunction(std::string_view extern_func_name, void* func) -> void {
		evo::debugAssert(this->isInitialized(), "LegacyJITEngine not initialized");

		this->data->execution_engine.registerFunction(extern_func_name, func);
	}


	auto LegacyJITEngine::registerFunction(const ExternalFunction::ID extern_func_id, void* func) -> void {
		evo::debugAssert(this->isInitialized(), "LegacyJITEngine not initialized");

		this->registerFunction(this->module->getExternalFunction(extern_func_id).name, func);
	}


	auto LegacyJITEngine::registerFunction(const ExternalFunction& extern_func, void* func) -> void {
		evo::debugAssert(this->isInitialized(), "LegacyJITEngine not initialized");

		this->registerFunction(extern_func.name, func);
	}




	auto LegacyJITEngine::runFunc(std::string_view func_name) const -> evo::Result<core::GenericValue> {
		evo::debugAssert(this->isInitialized(), "LegacyJITEngine not initialized");

		for(const Function& func : this->module->getFunctionIter()){
			if(func.getName() == func_name){
				return this->runFunc(func);
			}
		}

		evo::debugFatalBreak("No function \"{}\" exists", func_name);
	}

	auto LegacyJITEngine::runFunc(Function::ID func_id) const -> evo::Result<core::GenericValue> {
		evo::debugAssert(this->isInitialized(), "LegacyJITEngine not initialized");

		return this->runFunc(this->module->getFunction(func_id));
	}

	auto LegacyJITEngine::runFunc(const Function& func) const -> evo::Result<core::GenericValue> {
		evo::debugAssert(this->isInitialized(), "LegacyJITEngine not initialized");

		evo::debugAssert(
			func.getReturnType().isAggregate() == false, "Cannot run functions that return a aggregate value"
		);

		evo::debugAssert(
			func.getReturnType().kind() != Type::Kind::FUNCTION, "Cannot run functions that return a function"
		);


		switch(func.getReturnType().kind()){
			case Type::Kind::VOID: {
				const evo::Result<void> result = this->data->execution_engine.runFunctionDirectly<void>(func.getName());
				if(result.isError()){ return evo::resultError; }
				return core::GenericValue();
			} break;

			case Type::Kind::INTEGER: {
				const size_t size_of_func_return_base_type = this->module->getSize(func.getReturnType());

				switch(size_of_func_return_base_type){
					case 1: {
						const evo::Result<uint8_t> result =
							this->data->execution_engine.runFunctionDirectly<uint8_t>(func.getName());
						if(result.isError()){ return evo::resultError; }
						return core::GenericValue(
							core::GenericInt(func.getReturnType().getWidth(), result.value(), true)
						);
					} break;

					case 2: {
						const evo::Result<uint16_t> result =
							this->data->execution_engine.runFunctionDirectly<uint16_t>(func.getName());
						if(result.isError()){ return evo::resultError; }
						return core::GenericValue(
							core::GenericInt(func.getReturnType().getWidth(), result.value(), true)
						);
					} break;

					case 4: {
						const evo::Result<uint32_t> result =
							this->data->execution_engine.runFunctionDirectly<uint32_t>(func.getName());
						if(result.isError()){ return evo::resultError; }
						return core::GenericValue(
							core::GenericInt(func.getReturnType().getWidth(), result.value(), true)
						);
					} break;

					case 8: {
						const evo::Result<uint64_t> result =
							this->data->execution_engine.runFunctionDirectly<uint64_t>(func.getName());
						if(result.isError()){ return evo::resultError; }
						return core::GenericValue(
							core::GenericInt(func.getReturnType().getWidth(), result.value(), true)
						);
					} break;

					default: {
						evo::debugFatalBreak(
							"LegacyJITEngine currently cannot run functions that return Integers larger than 64"
						);
					} break;
				}
			} break;

			case Type::Kind::BOOL: {
				const evo::Result<bool> result =
					this->data->execution_engine.runFunctionDirectly<bool>(func.getName());
				if(result.isError()){ return evo::resultError; }
				return core::GenericValue(result.value());
			} break;


			case Type::Kind::FLOAT: {
				switch(func.getReturnType().getWidth()){
					case 16: {
						evo::debugFatalBreak("LegacyJITEngine currently cannot run functions that return F16");
					} break;

					case 32: {
						const evo::Result<evo::float32_t> result =
							this->data->execution_engine.runFunctionDirectly<evo::float32_t>(func.getName());
						if(result.isError()){ return evo::resultError; }
						return core::GenericValue(core::GenericFloat(result.value()));
					} break;

					case 64: {
						const evo::Result<evo::float64_t> result =
							this->data->execution_engine.runFunctionDirectly<evo::float64_t>(func.getName());
						if(result.isError()){ return evo::resultError; }
						return core::GenericValue(core::GenericFloat(result.value()));
					} break;

					case 80: {
						evo::debugFatalBreak("LegacyJITEngine currently cannot run functions that return F80");
					} break;

					case 128: {
						evo::debugFatalBreak("LegacyJITEngine currently cannot run functions that return F128");
					} break;
				}
			} break;

			case Type::Kind::BFLOAT: {
				evo::debugFatalBreak("LegacyJITEngine currently cannot run functions that return BFloat (bugged in LLVM)");
			} break;

			case Type::Kind::PTR:      evo::debugFatalBreak("LegacyJITEngine cannot run functions that return a pointer");
			case Type::Kind::ARRAY:    evo::debugFatalBreak("LegacyJITEngine cannot run functions that return an array");
			case Type::Kind::STRUCT:   evo::debugFatalBreak("LegacyJITEngine cannot run functions that return a struct");
			case Type::Kind::FUNCTION: evo::debugFatalBreak("LegacyJITEngine cannot run functions that return a function");
		}

		evo::unreachable();
	}



	auto LegacyJITEngine::panicJump() -> void {
		std::longjmp(llvmint::ExecutionEngine::getPanicJump(), true);
	}


}