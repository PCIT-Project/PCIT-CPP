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
	
	class Value{
		public:
			Value(llvm::Value* native_type) : _native(native_type) {};
			~Value() = default;

			EVO_NODISCARD auto native() const -> llvm::Value* { return this->_native; }
	
		private:
			llvm::Value* _native;
	};


	class Constant{
		public:
			Constant(llvm::Constant* native_type) : _native(native_type) {};
			~Constant() = default;

			EVO_NODISCARD operator Value() const;

			EVO_NODISCARD auto native() const -> llvm::Constant* { return this->_native; }
	
		private:
			llvm::Constant* _native;
	};


	class ConstantInt{
		public:
			ConstantInt(llvm::ConstantInt* native_type) : _native(native_type) {};
			~ConstantInt() = default;

			EVO_NODISCARD operator Value() const;
			EVO_NODISCARD operator Constant() const;

			EVO_NODISCARD auto native() const -> llvm::ConstantInt* { return this->_native; }
	
		private:
			llvm::ConstantInt* _native;
	};



	
}