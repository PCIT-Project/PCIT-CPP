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
#include "./class_impls/types.h"
#include "./class_impls/stmts.h"
#include "./Function.h"

namespace pcit::llvmint{

	
	class IRBuilder{
		public:
			enum class IntrinsicID{
				debugtrap,
			};

		public:
			IRBuilder(class LLVMContext& context);
			~IRBuilder();


			//////////////////////////////////////////////////////////////////////
			// create

			auto createBasicBlock(const Function& func, evo::CStrProxy name = '\0') -> BasicBlock;

			// createAlloca
			// createLoad
			// createStore

			auto createRet() -> void;

			// createGEP
			// createUnreachable
			// createBranch
			// createCondBranch
			// createCall
			// createIntrinsicCall
			// createPhi


			///////////////////////////////////
			// type conversion

			// createTrunc
			// createZExt
			// createSExt


			///////////////////////////////////
			// operators

			// createAdd
			// createSub
			// createMul
			// createUDiv
			// createSDiv


			// createICmpEQ
			// createICmpNE

			// createICmpUGT
			// createICmpUGE
			// createICmpULT
			// createICmpULE
			
			// createICmpSGT
			// createICmpSGE
			// createICmpSLT
			// createICmpSLE

			// createNot


			//////////////////////////////////////////////////////////////////////
			// set

			auto setInsertionPoint(const BasicBlock& block) -> void;
			auto setInsertionPointAtBack(const Function& func) -> void;



			//////////////////////////////////////////////////////////////////////
			// values

			// valueUI32
			// valueUI64
			// valueUI_N

			// valueCInt

			// valueBool

			// valueString

			// valueGlobal


			//////////////////////////////////////////////////////////////////////
			// types

			EVO_NODISCARD auto getFuncProto(
				const llvmint::Type& return_type, evo::ArrayProxy<llvmint::Type> params, bool is_var_args
			) -> llvmint::FunctionType;


			// getTypeBool


			// getTypeI8
			// getTypeI16
			// getTypeI32
			// getTypeI64
			// getTypeI128

			// getTypeI_N

			// getTypeInt
			// getTypeUInt
			// getTypeISize
			// getTypeUSize


			// getTypeCInt


			// getTypePtr

			EVO_NODISCARD auto getTypeVoid() const -> llvmint::Type;



		private:
			EVO_NODISCARD auto get_native_context() -> llvm::LLVMContext&;

	
		private:
			llvm::IRBuilderBase* builder = nullptr;

			llvm::NoFolder* folder = nullptr;
			llvm::IRBuilderDefaultInserter* inserter = nullptr;
	};

	
}