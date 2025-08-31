////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>
#include <PCIT_core.h>


#include "./Function.h"
#include "./GlobalVar.h"
#include "./Type.h"


namespace llvm{
	class LLVMContext;
	class Module;
}



namespace pcit::pir{


	class JITEngine{
		public:
			struct InitConfig{
				bool allowDefaultSymbolLinking = false;
			};
		

		public:
			JITEngine() = default;

			~JITEngine(){
				if(this->isInitialized()){
					this->deinit();
				}
			}

			JITEngine(const JITEngine&) = delete;
			auto operator=(const JITEngine&) -> JITEngine& = delete;

			JITEngine(JITEngine&& rhs) : data(std::exchange(rhs.data, nullptr)) {}
			auto operator=(JITEngine&& rhs) -> JITEngine& {
				std::destroy_at(this);
				std::construct_at(this, std::move(rhs));
				return *this;
			}



			// if returns error, not initialized
			// error is list of messages from LLVM
			auto init(const InitConfig& config) -> evo::Expected<void, evo::SmallVector<std::string>>;
			
			auto deinit() -> void;


			EVO_NODISCARD auto addModule(const class Module& module)
				-> evo::Expected<void, evo::SmallVector<std::string>>;

			// deletes the context and the module
			EVO_NODISCARD auto addModule(llvm::LLVMContext* llvm_context, llvm::Module* module)
				-> evo::Expected<void, evo::SmallVector<std::string>>;



			struct ModuleSubsets{
				evo::ArrayProxy<Type> structs;
				evo::ArrayProxy<GlobalVar::ID> globalVars;
				evo::ArrayProxy<GlobalVar::ID> globalVarDecls;
				evo::ArrayProxy<Function::ID> funcs;
				evo::ArrayProxy<Function::ID> funcDecls;
				evo::ArrayProxy<ExternalFunction::ID> externFuncs;
			};

			EVO_NODISCARD auto addModuleSubset(const class Module& module, const ModuleSubsets& module_subsets)
				-> evo::Expected<void, evo::SmallVector<std::string>>;

			EVO_NODISCARD auto addModuleSubsetWithWeakDependencies(
				const class Module& module, const ModuleSubsets& module_subsets
			) -> evo::Expected<void, evo::SmallVector<std::string>>;




			// requires the function to be called to return Void and take the following parameters:
			// 		1) Ptr to an array of Ptr where each element is an argument to the function 
			// 		2) Ptr to the return value 
			// 	Note: calling convension should be C, and linkage should be external
			// 	Note: both arguments are required, even if unused by the target function
			// 	Note: no conversion from core::GenericValue is needed to be done on the JIT side,
			// 		as that is done automatically by JITEngine.
			EVO_NODISCARD auto runFunc(
				const class Module& module, Function::ID func_id, std::span<core::GenericValue> args, Type return_type
			) -> core::GenericValue;


			template<class T>
			EVO_NODISCARD auto getFuncPtr(std::string_view name) -> T {
				static_assert(std::is_pointer<T>(), "Must be function pointer");
				return (T)this->get_func_ptr(name);
			}

			struct FuncRegisterInfo{
				std::string_view name;
				void* funcCallAddress;
			};
			EVO_NODISCARD auto registerFuncs(evo::ArrayProxy<FuncRegisterInfo> func_register_infos)
				-> evo::Expected<void, evo::SmallVector<std::string>>;

			EVO_NODISCARD auto registerFunc(std::string_view name, void* func_call_address)
				-> evo::Expected<void, evo::SmallVector<std::string>>;


			EVO_NODISCARD auto isInitialized() const -> bool { return this->data != nullptr; }


		private:
			auto get_func_ptr(std::string_view name) -> void*;


		private:
			struct Data;
			Data* data = nullptr;
	};


}


