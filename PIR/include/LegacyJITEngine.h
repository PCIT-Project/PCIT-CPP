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


	class LegacyJITEngine{
		public:
			LegacyJITEngine() = default;

			#if defined(PCIT_CONFIG_DEBUG)
				~LegacyJITEngine(){
					evo::debugAssert(
						this->isInitialized() == false, "Didn't deinit LegacyJITEngine before destructor"
					);
				}
			#else
				~LegacyJITEngine() = default;
			#endif

			auto init(const class Module& _module) -> void;
			auto deinit() -> void;

			EVO_NODISCARD auto isInitialized() const -> bool { return this->data != nullptr; }

			EVO_NODISCARD auto registerFunction(std::string_view extern_func_name, void* func) -> void;
			EVO_NODISCARD auto registerFunction(const ExternalFunction::ID extern_func_id, void* func) -> void;
			EVO_NODISCARD auto registerFunction(const ExternalFunction& extern_func, void* func) -> void;

			EVO_NODISCARD auto runFunc(std::string_view func_name) const -> evo::Result<core::GenericValue>;
			EVO_NODISCARD auto runFunc(Function::ID func_id) const -> evo::Result<core::GenericValue>;
			EVO_NODISCARD auto runFunc(const Function& func) const -> evo::Result<core::GenericValue>;

			// only for use by registered functions
			static auto panicJump() -> void;

		private:
			const class Module* module = nullptr;

			struct Data;
			Data* data = nullptr;
	};


}


