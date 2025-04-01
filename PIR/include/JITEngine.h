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

			auto init(const InitConfig& config) -> evo::Result<>;
			auto deinit() -> void;


			EVO_NODISCARD auto addModule(const class Module& module) -> evo::Result<>;

			auto lookupFunc(std::string_view name) -> void*;

			struct FuncRegisterInfo{
				std::string_view name;
				void* funcCallAddress;
			};
			EVO_NODISCARD auto registerFuncs(evo::ArrayProxy<FuncRegisterInfo> func_register_infos) -> evo::Result<>;
			EVO_NODISCARD auto registerFunc(std::string_view name, void* func_call_address) -> evo::Result<>;


			EVO_NODISCARD auto isInitialized() const -> bool { return this->data != nullptr; }


		private:
			struct Data;
			Data* data = nullptr;
	};


}


