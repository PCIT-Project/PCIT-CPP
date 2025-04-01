////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <Evo.h>

#include "./class_impls/native_ptr_decls.h"

namespace pcit::llvmint{

	class LLVMContext{
		public:
			LLVMContext() = default;
			~LLVMContext() { evo::debugAssert(!this->isInitialized(), "must be uninitialized before destructor"); };

			LLVMContext(LLVMContext&& rhs){
				llvm::LLVMContext* holder = this->_native;
				this->_native = rhs._native;
				rhs._native = holder;
			}

			auto init() -> void;
			auto deinit() -> void;

			// uninitiailizes
			auto steal() -> llvm::LLVMContext* {
				llvm::LLVMContext* holder = this->_native;
				this->_native = nullptr;
				return holder;
			}


			EVO_NODISCARD auto isInitialized() const -> bool { return this->_native != nullptr; };

			EVO_NODISCARD auto native() const -> const llvm::LLVMContext* { return this->_native; };
			EVO_NODISCARD auto native()       ->       llvm::LLVMContext* { return this->_native; };
	
		private:
			llvm::LLVMContext* _native = nullptr;
	};
	
}