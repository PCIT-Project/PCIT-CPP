////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <Evo.h>

// #include "./class_impls/native_ptr_decls.h"
// #include "./class_impls/types.h"
// #include "./class_impls/enums.h"
// #include "./Function.h"

// #include "../../../include/platform.h"

namespace pcit::llvmint{


	class OrcJIT{
		public:
			struct InitConfig{
				bool allowDefaultSymbolLinking = false;
			};

		public:
			OrcJIT() = default;

			#if defined(PCIT_CONFIG_DEBUG)
				~OrcJIT(){
					evo::debugAssert(
						this->isInitialized() == false, "`OrcJIT::deinit()` must be called before destructor`"
					);
				}
			#else
				~OrcJIT() = default;
			#endif

			EVO_NODISCARD auto init(const InitConfig& config) -> evo::Result<>; // if returns error, not initialized
			EVO_NODISCARD auto deinit() -> void;


			EVO_NODISCARD auto addModule(class LLVMContext&& context, class Module&& module) -> evo::Result<>;

			EVO_NODISCARD auto lookupFunc(std::string_view name) -> void*;

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