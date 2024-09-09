//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once

#include <Evo.h>

#include "./types.h"
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


	class Alloca{
		public:
			Alloca(llvm::AllocaInst* native_stmt) : _native(native_stmt) {};
			~Alloca() = default;

			EVO_NODISCARD auto getAllocatedType() const -> Type;


			EVO_NODISCARD explicit operator Value() const;
			EVO_NODISCARD auto asValue() const -> Value { return static_cast<Value>(*this); }


			EVO_NODISCARD auto native() const -> llvm::AllocaInst* { return this->_native; }
	
		private:
			llvm::AllocaInst* _native;
	};


	class Constant{
		public:
			Constant(llvm::Constant* native_type) : _native(native_type) {};
			~Constant() = default;

			EVO_NODISCARD explicit operator Value() const;
			EVO_NODISCARD auto asValue() const -> Value { return static_cast<Value>(*this); }


			EVO_NODISCARD auto native() const -> llvm::Constant* { return this->_native; }
	
		private:
			llvm::Constant* _native;
	};


	class ConstantInt{
		public:
			ConstantInt(llvm::ConstantInt* native_type) : _native(native_type) {};
			~ConstantInt() = default;

			EVO_NODISCARD explicit operator Value() const;
			EVO_NODISCARD auto asValue() const -> Value { return static_cast<Value>(*this); }

			EVO_NODISCARD explicit operator Constant() const;
			EVO_NODISCARD auto asConstant() const -> Constant { return static_cast<Constant>(*this); }


			EVO_NODISCARD auto native() const -> llvm::ConstantInt* { return this->_native; }
	
		private:
			llvm::ConstantInt* _native;
	};



	class CallInst{
		public:
			CallInst(llvm::CallInst* native_type) : _native(native_type) {};
			~CallInst() = default;

			EVO_NODISCARD explicit operator Value() const;
			EVO_NODISCARD auto asValue() const -> Value { return static_cast<Value>(*this); }


			EVO_NODISCARD auto native() const -> llvm::CallInst* { return this->_native; }
	
		private:
			llvm::CallInst* _native;
	};


	class LoadInst{
		public:
			LoadInst(llvm::LoadInst* native_stmt) : _native(native_stmt) {};
			~LoadInst() = default;

			EVO_NODISCARD explicit operator Value() const;
			EVO_NODISCARD auto asValue() const -> Value { return static_cast<Value>(*this); }


			EVO_NODISCARD auto native() const -> llvm::LoadInst* { return this->_native; }
	
		private:
			llvm::LoadInst* _native;
	};




	class GlobalVariable{
		public:
			GlobalVariable(llvm::GlobalVariable* native_type) : _native(native_type) {};
			~GlobalVariable() = default;

			EVO_NODISCARD auto getType() const -> Type;

			EVO_NODISCARD explicit operator Value() const;
			EVO_NODISCARD auto asValue() const -> Value { return static_cast<Value>(*this); }
			

			EVO_NODISCARD auto native() const -> llvm::GlobalVariable* { return this->_native; }
	
		private:
			llvm::GlobalVariable* _native;
	};


	
}