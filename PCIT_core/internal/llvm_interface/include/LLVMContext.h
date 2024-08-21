//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once

#include <Evo.h>

#include "./class_impls/native_ptr_decls.h"

namespace pcit::llvmint{

	class LLVMContext{
		public:
			LLVMContext() = default;
			~LLVMContext() { evo::debugAssert(!this->isInitialized(), "Did not call shutdown() before destructor"); };

			auto init() -> void;
			auto deinit() -> void;


			EVO_NODISCARD auto isInitialized() const -> bool { return this->_native != nullptr; };

			EVO_NODISCARD auto native() const -> const llvm::LLVMContext* { return this->_native; };
			EVO_NODISCARD auto native()       ->       llvm::LLVMContext* { return this->_native; };
	
		private:
			llvm::LLVMContext* _native = nullptr;
	};
	
}