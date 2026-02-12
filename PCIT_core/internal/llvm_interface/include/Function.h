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
#include "./class_impls/enums.h"
#include "./class_impls/stmts.h"
#include "./class_impls/values.h"
#include "./class_impls/types.h"

namespace pcit::llvmint{

	class Argument{
		public:
			Argument(llvm::Argument* native_argument) : _native(native_argument) {};
			~Argument() = default;

			auto setName(std::string_view name) -> void;

			auto setZeroExt() -> void;
			auto setSignExt() -> void;
			auto setNoAlias() -> void;
			auto setNonNull() -> void;
			auto setDereferencable(uint64_t size) -> void;
			auto setReadOnly() -> void;
			auto setWriteOnly() -> void;
			auto setWritable() -> void;
			auto setStructRet(const Type& type) -> void;


			EVO_NODISCARD explicit operator Value() const;
			EVO_NODISCARD auto asValue() const -> Value { return static_cast<Value>(*this); }


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

			EVO_NODISCARD auto getArg(unsigned index) const -> Argument;

			auto setNoThrow() -> void;
			auto setNoReturn() -> void;
			auto setCallingConv(CallingConv calling_conv) -> void;

			EVO_NODISCARD explicit operator Value() const;
			EVO_NODISCARD auto asValue() const -> Value { return static_cast<Value>(*this); }

			EVO_NODISCARD explicit operator Constant() const;
			EVO_NODISCARD auto asConstant() const -> Constant { return static_cast<Constant>(*this); }

			EVO_NODISCARD auto native() const -> llvm::Function* { return this->_native; }
	
		private:
			llvm::Function* _native;
	};
	
}