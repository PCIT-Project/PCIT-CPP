////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <Evo.h>

#include "./native_ptr_decls.h"

namespace pcit::llvmint{
	
	class Type{
		public:
			Type(llvm::Type* native_type) : _native(native_type) {};
			~Type() = default;

			EVO_NODISCARD auto native() const -> llvm::Type* { return this->_native; }
	
		private:
			llvm::Type* _native;
	};


	class FunctionType{
		public:
			FunctionType(llvm::FunctionType* native_type) : _native(native_type) {};
			~FunctionType() = default;

			EVO_NODISCARD explicit operator Type() const;
			EVO_NODISCARD auto asType() const -> Type { return static_cast<Type>(*this); }

			EVO_NODISCARD auto native() const -> llvm::FunctionType* { return this->_native; }
	
		private:
			llvm::FunctionType* _native;
	};


	class IntegerType{
		public:
			IntegerType(llvm::IntegerType* native_type) : _native(native_type) {};
			~IntegerType() = default;

			EVO_NODISCARD explicit operator Type() const;
			EVO_NODISCARD auto asType() const -> Type { return static_cast<Type>(*this); }

			EVO_NODISCARD auto native() const -> llvm::IntegerType* { return this->_native; }
	
		private:
			llvm::IntegerType* _native;
	};


	class PointerType{
		public:
			PointerType(llvm::PointerType* native_type) : _native(native_type) {};
			~PointerType() = default;

			EVO_NODISCARD explicit operator Type() const;
			EVO_NODISCARD auto asType() const -> Type { return static_cast<Type>(*this); }

			EVO_NODISCARD auto native() const -> llvm::PointerType* { return this->_native; }
	
		private:
			llvm::PointerType* _native;
	};


	class ArrayType{
		public:
			ArrayType(llvm::ArrayType* native_type) : _native(native_type) {};
			~ArrayType() = default;

			EVO_NODISCARD explicit operator Type() const;
			EVO_NODISCARD auto asType() const -> Type { return static_cast<Type>(*this); }

			EVO_NODISCARD auto native() const -> llvm::ArrayType* { return this->_native; }
	
		private:
			llvm::ArrayType* _native;
	};


	class StructType{
		public:
			StructType(llvm::StructType* native_type) : _native(native_type) {};
			~StructType() = default;

			EVO_NODISCARD explicit operator Type() const;
			EVO_NODISCARD auto asType() const -> Type { return static_cast<Type>(*this); }

			EVO_NODISCARD auto native() const -> llvm::StructType* { return this->_native; }
	
		private:
			llvm::StructType* _native;
	};

	
}