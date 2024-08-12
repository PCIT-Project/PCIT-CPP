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
	
	class BasicBlock{
		public:
			BasicBlock(llvm::BasicBlock* native_stmt) : _native(native_stmt) {};
			~BasicBlock() = default;

			EVO_NODISCARD auto native() const -> llvm::BasicBlock* { return this->_native; }
	
		private:
			llvm::BasicBlock* _native;
	};


	
}