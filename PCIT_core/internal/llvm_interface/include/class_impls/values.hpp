////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <Evo.hpp>

#include "./types.hpp"
#include "./native_ptr_decls.hpp"
#include "./enums.hpp"
#include "../DIBuilder.hpp"

namespace pcit::llvmint{
	
	class Value{
		public:
			Value(llvm::Value* native_type) : _native(native_type) {};
			~Value() = default;

			[[nodiscard]] auto native() const -> llvm::Value* { return this->_native; }
	
		private:
			llvm::Value* _native;
	};


	class Alloca{
		public:
			Alloca(llvm::AllocaInst* native_stmt) : _native(native_stmt) {};
			~Alloca() = default;

			[[nodiscard]] auto getAllocatedType() const -> Type;


			[[nodiscard]] explicit operator Value() const;
			[[nodiscard]] auto asValue() const -> Value { return static_cast<Value>(*this); }


			[[nodiscard]] auto native() const -> llvm::AllocaInst* { return this->_native; }
	
		private:
			llvm::AllocaInst* _native;
	};


	class Constant{
		public:
			Constant(llvm::Constant* native_type) : _native(native_type) {};
			~Constant() = default;

			[[nodiscard]] auto getType() const -> Type;

			[[nodiscard]] explicit operator Value() const;
			[[nodiscard]] auto asValue() const -> Value { return static_cast<Value>(*this); }


			[[nodiscard]] auto native() const -> llvm::Constant* { return this->_native; }
	
		private:
			llvm::Constant* _native;
	};


	class ConstantInt{
		public:
			ConstantInt(llvm::ConstantInt* native_type) : _native(native_type) {};
			~ConstantInt() = default;

			[[nodiscard]] explicit operator Value() const;
			[[nodiscard]] auto asValue() const -> Value { return static_cast<Value>(*this); }

			[[nodiscard]] explicit operator Constant() const;
			[[nodiscard]] auto asConstant() const -> Constant { return static_cast<Constant>(*this); }


			[[nodiscard]] auto native() const -> llvm::ConstantInt* { return this->_native; }
	
		private:
			llvm::ConstantInt* _native;
	};

	class ReturnInst{
		public:
			ReturnInst(llvm::ReturnInst* native_type) : _native(native_type) {};
			~ReturnInst() = default;

			auto setLocation(DIBuilder::Location location) -> void;


			[[nodiscard]] auto native() const -> llvm::ReturnInst* { return this->_native; }
	
		private:
			llvm::ReturnInst* _native;
	};

	class BranchInst{
		public:
			BranchInst(llvm::BranchInst* native_type) : _native(native_type) {};
			~BranchInst() = default;

			auto setLocation(DIBuilder::Location location) -> void;


			[[nodiscard]] auto native() const -> llvm::BranchInst* { return this->_native; }
	
		private:
			llvm::BranchInst* _native;
	};


	class CallInst{
		public:
			CallInst(llvm::CallInst* native_type) : _native(native_type) {};
			~CallInst() = default;

			auto setCallingConv(CallingConv calling_conv) -> void;
			auto setLocation(DIBuilder::Location location) -> void;

			[[nodiscard]] explicit operator Value() const;
			[[nodiscard]] auto asValue() const -> Value { return static_cast<Value>(*this); }


			[[nodiscard]] auto native() const -> llvm::CallInst* { return this->_native; }
	
		private:
			llvm::CallInst* _native;
	};


	class LoadInst{
		public:
			LoadInst(llvm::LoadInst* native_stmt) : _native(native_stmt) {};
			~LoadInst() = default;

			[[nodiscard]] explicit operator Value() const;
			[[nodiscard]] auto asValue() const -> Value { return static_cast<Value>(*this); }


			[[nodiscard]] auto native() const -> llvm::LoadInst* { return this->_native; }
	
		private:
			llvm::LoadInst* _native;
	};


	class StoreInst{
		public:
			StoreInst(llvm::StoreInst* native_stmt) : _native(native_stmt) {};
			~StoreInst() = default;

			auto setLocation(DIBuilder::Location location) -> void;

			[[nodiscard]] auto native() const -> llvm::StoreInst* { return this->_native; }
	
		private:
			llvm::StoreInst* _native;
	};




	class GlobalVariable{
		public:
			GlobalVariable(llvm::GlobalVariable* native_type) : _native(native_type) {};
			~GlobalVariable() = default;

			auto setAlignment(unsigned alignment) -> void;
			auto setInitializer(Constant value) -> void;

			[[nodiscard]] auto getType() const -> Type;

			[[nodiscard]] explicit operator Value() const;
			[[nodiscard]] auto asValue() const -> Value { return static_cast<Value>(*this); }

			[[nodiscard]] explicit operator Constant() const;
			[[nodiscard]] auto asConstant() const -> Constant { return static_cast<Constant>(*this); }
			

			[[nodiscard]] auto native() const -> llvm::GlobalVariable* { return this->_native; }
	
		private:
			llvm::GlobalVariable* _native;
	};


	
}