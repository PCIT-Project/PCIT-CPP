//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once

#include <Evo.h>

#include "./class_impls/native_ptr_decls.h"
#include "./class_impls/enums.h"
#include "./class_impls/stmts.h"


namespace pcit::llvmint{


	class ExecutionEngine{
		public:
			ExecutionEngine() = default;
			~ExecutionEngine(){
				evo::debugAssert(
					this->hasCreatedEngine() == false,
					"ExecutionEngine destructor run without shutting down"
				);
			};

			// creates copy of the module (module.getClone())
			auto createEngine(const class Module& module) -> void;

			auto shutdownEngine() -> void;


			auto registerFunction(const class Function& func, void* func_call) -> void;
			auto registerFunction(std::string_view func, void* func_call_address) -> void;

			// EVO_NODISCARD auto runFunction(std::string_view func_name, evo::ArrayProxy<GenericValue> params)
			// 	-> GenericValue;


			template<typename T>
			EVO_NODISCARD auto runFunctionDirectly(std::string_view func_name) -> T {
				const uint64_t func_addr = this->get_func_address(func_name);
				
				using FuncType = T(*)(void);
				const FuncType func = (FuncType)func_addr;
				if constexpr(std::is_same_v<T, void>){
					func();
				}else{
					return func();
				}
			};



			auto setupLinkedFuncs() -> void;

			EVO_NODISCARD auto hasCreatedEngine() const -> bool { return this->engine != nullptr; };




		private:
			EVO_NODISCARD auto get_func_address(std::string_view func_name) const -> uint64_t;
	
		private:
			llvm::ExecutionEngine* engine = nullptr;
	};
}