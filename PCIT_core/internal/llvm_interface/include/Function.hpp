////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <Evo.hpp>

#include "./class_impls/native_ptr_decls.hpp"
#include "./class_impls/enums.hpp"
#include "./class_impls/stmts.hpp"
#include "./class_impls/values.hpp"
#include "./class_impls/types.hpp"
#include "./DIBuilder.hpp"


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


			[[nodiscard]] explicit operator Value() const;
			[[nodiscard]] auto asValue() const -> Value { return static_cast<Value>(*this); }


			[[nodiscard]] auto native() const -> llvm::Argument* { return this->_native; }
	
		private:
			llvm::Argument* _native;
	};


	class Function{
		public:
			// llvm/Support/CodeGen.h
			enum class UWTableKind{
				NONE    = 0,
				SYNC    = 1,
				ASYNC   = 2,
				DEFAULT = ASYNC,
			};


		public:
			Function(llvm::Function* native_func) : _native(native_func) {};
			~Function() = default;

			[[nodiscard]] auto front() const -> const BasicBlock;
			[[nodiscard]] auto front()       ->       BasicBlock;
			[[nodiscard]] auto back() const -> const BasicBlock;
			[[nodiscard]] auto back()       ->       BasicBlock;

			[[nodiscard]] auto getArg(unsigned index) const -> Argument;

			auto setNoThrow() -> void;
			auto setUWTableKind(UWTableKind kind = UWTableKind::DEFAULT) -> void;
			auto setNoReturn() -> void;
			auto setCallingConv(CallingConv calling_conv) -> void;

			auto setSubprogram(DIBuilder::Subprogram subprogram) -> void;

			[[nodiscard]] explicit operator Value() const;
			[[nodiscard]] auto asValue() const -> Value { return static_cast<Value>(*this); }

			[[nodiscard]] explicit operator Constant() const;
			[[nodiscard]] auto asConstant() const -> Constant { return static_cast<Constant>(*this); }

			[[nodiscard]] auto native() const -> llvm::Function* { return this->_native; }
	
		private:
			llvm::Function* _native;
	};
	
}