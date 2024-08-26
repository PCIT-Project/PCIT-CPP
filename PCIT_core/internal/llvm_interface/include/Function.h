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
#include "./class_impls/values.h"

namespace pcit::llvmint{

	class Argument{
		public:
			Argument(llvm::Argument* native_argument) : _native(native_argument) {};
			~Argument() = default;

			auto setName(std::string_view name) -> void;

			EVO_NODISCARD operator Value() const;


			EVO_NODISCARD auto native() const -> llvm::Argument* { return this->_native; }
	
		private:
			llvm::Argument* _native;
	};


	class Function{
		public:
			Function(llvm::Function* native_func) : _native(native_func) {};
			~Function() = default;

			EVO_NODISCARD auto front() const -> const BasicBlock;
			EVO_NODISCARD auto front()       ->       BasicBlock;
			EVO_NODISCARD auto back() const -> const BasicBlock;
			EVO_NODISCARD auto back()       ->       BasicBlock;

			EVO_NODISCARD auto getArg(evo::uint index) const -> Argument;

			auto setNoThrow() -> void;
			auto setCallingConv(CallingConv calling_conv) -> void;


			EVO_NODISCARD auto native() const -> llvm::Function* { return this->_native; }
	
		private:
			llvm::Function* _native;
	};
	
}