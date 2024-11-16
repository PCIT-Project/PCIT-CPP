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

			auto init(const class Module& _module) -> void;
			auto deinit() -> void;

			EVO_NODISCARD auto isInitialized() const -> bool { return this->data != nullptr; }

			EVO_NODISCARD auto registerFunction(std::string_view func_decl_name, void* func) -> void;
			EVO_NODISCARD auto registerFunction(const FunctionDecl::ID func_decl_id, void* func) -> void;
			EVO_NODISCARD auto registerFunction(const FunctionDecl& func_decl, void* func) -> void;

			EVO_NODISCARD auto runFunc(std::string_view func_name) const -> evo::Result<core::GenericValue>;
			EVO_NODISCARD auto runFunc(Function::ID func_id) const -> evo::Result<core::GenericValue>;
			EVO_NODISCARD auto runFunc(const Function& func) const -> evo::Result<core::GenericValue>;


		private:
			const class Module* module = nullptr;

			struct Data;
			Data* data = nullptr;
	};


}


