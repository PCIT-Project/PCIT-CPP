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
		lowerer.lowerSubset(PIRToLLVMIR::Subset{
			.structs        = module_subsets.structs,
			.globalVars     = module_subsets.globalVars,
			.globalVarDecls = module_subsets.globalVarDecls,
			.externFuncs    = module_subsets.externFuncs,
			.funcDecls      = module_subsets.funcDecls,
			.funcs          = module_subsets.funcs,
		});

		return this->data->orc_jit.addModule(std::move(llvm_context), std::move(llvm_module));
	}


	auto JITEngine::addModuleSubsetWithWeakDependencies(const class Module& module, const ModuleSubsets& module_subsets)
	-> evo::Expected<void, evo::SmallVector<std::string>> {
		evo::debugAssert(this->isInitialized(), "JITEngine not initialized");

		auto llvm_context = llvmint::LLVMContext();
		llvm_context.init();

		auto llvm_module = llvmint::Module();
		llvm_module.init(module.getName(), llvm_context);

		auto lowerer = PIRToLLVMIR(module, llvm_context, llvm_module);
		lowerer.lowerSubsetWithWeakDependencies(PIRToLLVMIR::Subset{
			.structs        = module_subsets.structs,
			.globalVars     = module_subsets.globalVars,
			.globalVarDecls = module_subsets.globalVarDecls,
			.externFuncs    = module_subsets.externFuncs,
			.funcDecls      = module_subsets.funcDecls,
			.funcs          = module_subsets.funcs,
		});

		return this->data->orc_jit.addModule(std::move(llvm_context), std::move(llvm_module));
	}





	auto JITEngine::runFunc(const Module& module, Function::ID func_id, std::span<core::GenericValue> args)
	-> core::GenericValue {
		evo::debugAssert(this->isInitialized(), "JITEngine not initialized");

		const Function& func = module.getFunction(func_id);

		auto return_value = core::GenericValue();

		this->getFuncPtr<void(*)(core::GenericValue*, core::GenericValue*)>(func.getName())(&return_value, args.data());

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


	static constexpr auto round_up_to_nearest_multiple_of_64(size_t num) -> size_t {
		return (num + (64 - 1)) & ~(64 - 1);
	}

	auto JITEngine::registerJITInterfaceFuncs() -> evo::Expected<void, evo::SmallVector<std::string>> {
		return this->registerFuncs({
			FuncRegisterInfo(
				"PIR.JIT.return_generic_int",
				[](core::GenericValue* return_value, uint64_t* data, uint32_t bitwidth) -> void {
					*return_value = core::GenericValue(core::GenericInt(
						bitwidth,
						evo::ArrayProxy<uint64_t>(data, round_up_to_nearest_multiple_of_64(bitwidth) / 64)
					));
				}
			),
			FuncRegisterInfo(
				"PIR.JIT.return_generic_bool",
				[](core::GenericValue* return_value, bool value) -> void {
					*return_value = core::GenericValue(value);
				}
			),
			FuncRegisterInfo(
				"PIR.JIT.return_generic_f16",
				[](core::GenericValue* return_value, uint16_t* value) -> void {
					*return_value = core::GenericValue(
						core::GenericFloat::createF16(core::GenericInt::create<uint16_t>(*value))
					);
				}
			),
			FuncRegisterInfo(
				"PIR.JIT.return_generic_bf16",
				[](core::GenericValue* return_value, uint16_t* value) -> void {
					*return_value = core::GenericValue(
						core::GenericFloat::createBF16(core::GenericInt::create<uint16_t>(*value))
					);
				}
			),
			FuncRegisterInfo(
				"PIR.JIT.return_generic_f32",
				[](core::GenericValue* return_value, evo::float32_t value) -> void {
					*return_value = core::GenericValue(core::GenericFloat::createF32(value));
				}
			),
			FuncRegisterInfo(
				"PIR.JIT.return_generic_f64",
				[](core::GenericValue* return_value, evo::float64_t value) -> void {
					*return_value = core::GenericValue(core::GenericFloat::createF64(value));
				}
			),
			FuncRegisterInfo(
				"PIR.JIT.return_generic_f80",
				[](core::GenericValue* return_value, uint64_t* value) -> void {
					*return_value = core::GenericValue(
						core::GenericFloat::createF80(core::GenericInt(80, evo::ArrayProxy<uint64_t>(value, 2)))
					);
				}
			),
			FuncRegisterInfo(
				"PIR.JIT.return_generic_f128",
				[](core::GenericValue* return_value, uint64_t* value) -> void {
					*return_value = core::GenericValue(
						core::GenericFloat::createF128(core::GenericInt(128, evo::ArrayProxy<uint64_t>(value, 2)))
					);
				}
			),
			FuncRegisterInfo(
				"PIR.JIT.prepare_return_generic_aggregate",
				[](
					core::GenericValue* return_value,
					uint32_t num_elems,
					uint32_t[] target_index_arr,
					uint32_t target_num_indices
				) -> void {
					core::GenericValue* target = return_value;

					if(target_index_arr != nullptr){
						for(size_t i = 0; i < target_num_indices; i+=1){
							target = &target->as<evo::SmallVector<core::GenericValue>>()[target_index_arr[i]];
						}						
					}

					*target = core::GenericValue(core::GenericAggregateBuilder(num_elems));
				}
			),
			FuncRegisterInfo(
				"PIR.JIT.return_generic_aggregate_int",
				[](
					core::GenericValue* return_value,
					uint64_t* data,
					uint32_t bitwidth,
					uint32_t[] target_index_arr,
					uint32_t target_num_indices
				) -> void {
					core::GenericValue* target = return_value;

					if(target_index_arr != nullptr){
						for(size_t i = 0; i < target_num_indices; i+=1){
							target = &target->as<evo::SmallVector<core::GenericValue>>()[target_index_arr[i]];
						}						
					}

					*target = core::GenericValue(core::GenericInt(
						bitwidth,
						evo::ArrayProxy<uint64_t>(data, round_up_to_nearest_multiple_of_64(bitwidth) / 64)
					));
				}
			),
			FuncRegisterInfo(
				"PIR.JIT.return_generic_aggregate_bool",
				[](
					core::GenericValue* return_value,
					bool value,
					uint32_t[] target_index_arr,
					uint32_t target_num_indices
				) -> void {
					core::GenericValue* target = return_value;

					if(target_index_arr != nullptr){
						for(size_t i = 0; i < target_num_indices; i+=1){
							target = &target->as<evo::SmallVector<core::GenericValue>>()[target_index_arr[i]];
						}						
					}

					*target = core::GenericValue(value);
				}
			),
			FuncRegisterInfo(
				"PIR.JIT.return_generic_aggregate_f16",
				[](
					core::GenericValue* return_value,
					uint16_t* value,
					uint32_t[] target_index_arr,
					uint32_t target_num_indices
				) -> void {
					core::GenericValue* target = return_value;

					if(target_index_arr != nullptr){
						for(size_t i = 0; i < target_num_indices; i+=1){
							target = &target->as<evo::SmallVector<core::GenericValue>>()[target_index_arr[i]];
						}						
					}

					*target = core::GenericValue(core::GenericFloat::createF16(*value));
				}
			),
			FuncRegisterInfo(
				"PIR.JIT.return_generic_aggregate_bf16",
				[](
					core::GenericValue* return_value,
					uint16_t* value,
					uint32_t[] target_index_arr,
					uint32_t target_num_indices
				) -> void {
					core::GenericValue* target = return_value;

					if(target_index_arr != nullptr){
						for(size_t i = 0; i < target_num_indices; i+=1){
							target = &target->as<evo::SmallVector<core::GenericValue>>()[target_index_arr[i]];
						}						
					}

					*target = core::GenericValue(core::GenericFloat::createBF16(*value));
				}
			),
			FuncRegisterInfo(
				"PIR.JIT.return_generic_aggregate_f32",
				[](
					core::GenericValue* return_value,
					evo::float32_t value,
					uint32_t[] target_index_arr,
					uint32_t target_num_indices
				) -> void {
					core::GenericValue* target = return_value;

					if(target_index_arr != nullptr){
						for(size_t i = 0; i < target_num_indices; i+=1){
							target = &target->as<evo::SmallVector<core::GenericValue>>()[target_index_arr[i]];
						}						
					}

					*target = core::GenericValue(core::GenericFloat::createF32(value));
				}
			),
			FuncRegisterInfo(
				"PIR.JIT.return_generic_aggregate_f64",
				[](
					core::GenericValue* return_value,
					evo::float64_t value,
					uint32_t[] target_index_arr,
					uint32_t target_num_indices
				) -> void {
					core::GenericValue* target = return_value;

					if(target_index_arr != nullptr){
						for(size_t i = 0; i < target_num_indices; i+=1){
							target = &target->as<evo::SmallVector<core::GenericValue>>()[target_index_arr[i]];
						}						
					}

					*target = core::GenericValue(core::GenericFloat::createF64(value));
				}
			),
			FuncRegisterInfo(
				"PIR.JIT.return_generic_aggregate_f80",
				[](
					core::GenericValue* return_value,
					uint64_t* value,
					uint32_t[] target_index_arr,
					uint32_t target_num_indices
				) -> void {
					core::GenericValue* target = return_value;

					if(target_index_arr != nullptr){
						for(size_t i = 0; i < target_num_indices; i+=1){
							target = &target->as<evo::SmallVector<core::GenericValue>>()[target_index_arr[i]];
						}						
					}

					*target = core::GenericValue(
						core::GenericFloat::createF80(core::GenericInt(80, evo::ArrayProxy<uint64_t>(value, 2)))
					);
				}
			),
			FuncRegisterInfo(
				"PIR.JIT.return_generic_aggregate_f128",
				[](
					core::GenericValue* return_value,
					uint64_t* value,
					uint32_t[] target_index_arr,
					uint32_t target_num_indices
				) -> void {
					core::GenericValue* target = return_value;

					if(target_index_arr != nullptr){
						for(size_t i = 0; i < target_num_indices; i+=1){
							target = &target->as<evo::SmallVector<core::GenericValue>>()[target_index_arr[i]];
						}						
					}

					*target = core::GenericValue(
						core::GenericFloat::createF128(core::GenericInt(128, evo::ArrayProxy<uint64_t>(value, 2)))
					);
				}
			),
			FuncRegisterInfo(
				"PIR.JIT.return_generic_char",
				[](core::GenericValue* return_value, uint8_t value) -> void {
					*return_value = core::GenericValue(char(value));
				}
			),
			FuncRegisterInfo(
				"PIR.JIT.get_generic_int",
				[](core::GenericValue* source, void* value, uint64_t num_bytes) -> void {
					const core::GenericInt& generic_int = source->as<core::GenericInt>();
					std::memcpy(value, generic_int.data(), num_bytes);
				}
			),
			FuncRegisterInfo(
				"PIR.JIT.get_generic_bool",
				[](core::GenericValue* source) -> bool {
					return source->as<bool>();
				}
			),
			FuncRegisterInfo(
				"PIR.JIT.get_generic_float",
				[](core::GenericValue* source, void* value) -> void {
					const core::GenericInt generic_int = source->as<core::GenericFloat>().bitCastToGenericInt();
					std::memcpy(value, generic_int.data(), generic_int.getBitWidth() / 8);
				}
			),
		});
	}


	

}