//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


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

			EVO_NODISCARD auto native() const -> llvm::FunctionType* { return this->_native; }
	
		private:
			llvm::FunctionType* _native;
	};


	class IntegerType{
		public:
			IntegerType(llvm::IntegerType* native_type) : _native(native_type) {};
			~IntegerType() = default;

			EVO_NODISCARD operator Type() const;

			EVO_NODISCARD auto native() const -> llvm::IntegerType* { return this->_native; }
	
		private:
			llvm::IntegerType* _native;
	};


	class PointerType{
		public:
			PointerType(llvm::PointerType* native_type) : _native(native_type) {};
			~PointerType() = default;

			EVO_NODISCARD operator Type() const;

			EVO_NODISCARD auto native() const -> llvm::PointerType* { return this->_native; }
	
		private:
			llvm::PointerType* _native;
	};

	
}