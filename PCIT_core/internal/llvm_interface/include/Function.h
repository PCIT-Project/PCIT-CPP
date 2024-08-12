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
#include "./class_impls/enums.h"
#include "./class_impls/stmts.h"

namespace pcit::llvmint{

	class Function{
		public:
			Function(llvm::Function* native_func) : _native(native_func) {};
			~Function() = default;

			auto front() const -> const BasicBlock;
			auto front()       ->       BasicBlock;
			auto back() const -> const BasicBlock;
			auto back()       ->       BasicBlock;

			auto setNoThrow() -> void;
			auto setCallingConv(CallingConv calling_conv) -> void;


			EVO_NODISCARD auto native() const -> llvm::Function* { return this->_native; }
	
		private:
			llvm::Function* _native;
	};
	
}