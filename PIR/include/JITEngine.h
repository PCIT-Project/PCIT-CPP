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


namespace pcit::pir{


	class JITEngine{
		public:
			struct InitConfig{
				bool allowDefaultSymbolLinking = false;
			};
		

		public:
			JITEngine() = default;

			#if defined(PCIT_CONFIG_DEBUG)
				~JITEngine(){
					evo::debugAssert(
						this->isInitialized() == false, "Didn't deinit JITEngine before destructor"
					);
				}
			#else
				~JITEngine() = default;
			#endif

			// if returns error, not initialized
			// error is list of messages from LLVM
			auto init(const InitConfig& config) -> evo::Expected<void, evo::SmallVector<std::string>>;
			
			auto deinit() -> void;


			EVO_NODISCARD auto addModule(const class Module& module)
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


			EVO_NODISCARD auto runFunc(
				const class Module& module, Function::ID func_id, std::span<core::GenericValue> args
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


			// Needed if you want to use `runFunc`
			EVO_NODISCARD auto registerJITInterfaceFuncs() -> evo::Expected<void, evo::SmallVector<std::string>>;


			EVO_NODISCARD auto isInitialized() const -> bool { return this->data != nullptr; }


		private:
			auto get_func_ptr(std::string_view name) -> void*;


		private:
			struct Data;
			Data* data = nullptr;
	};


}


