//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once

#include <Evo.h>

#include "./class_impls/native_ptr_decls.h"
#include "./class_impls/types.h"
#include "./class_impls/stmts.h"
#include "./class_impls/values.h"
#include "./Function.h"

namespace pcit::llvmint{


	
	class IRBuilder{
		public:
			enum class IntrinsicID{
				debugtrap,

				saddSat,
				saddOverflow,
				smulFixSat,
				smulOverflow,
				ssubSat,
				ssubOverflow,

				uaddSat,
				uaddOverflow,
				umulFixSat,
				umulOverflow,
				usubSat,
				usubOverflow,
			};

		public:
			IRBuilder(class LLVMContext& context);
			~IRBuilder();


			//////////////////////////////////////////////////////////////////////
			// create

			auto createBasicBlock(const Function& func, evo::CStrProxy name = '\0') -> BasicBlock;

			EVO_NODISCARD auto createAlloca(const Type& type, evo::CStrProxy name = '\0') -> Alloca;
			EVO_NODISCARD auto createAlloca(const Type& type, const Value& array_length, evo::CStrProxy name = '\0') 
			-> Alloca;
			
			EVO_NODISCARD auto createLoad(
				const Value& value, const Type& type, bool is_volatile, evo::CStrProxy name = '\0'
			) -> LoadInst;
			EVO_NODISCARD auto createLoad(
				const Alloca& alloca, bool is_volatile, evo::CStrProxy name = '\0'
			) -> LoadInst;
			EVO_NODISCARD auto createLoad(
				const GlobalVariable& global_var, bool is_volatile, evo::CStrProxy name = '\0'
			) -> LoadInst;

			auto createStore(const Alloca& dst, const Value& source, bool is_volatile) -> void;
			auto createStore(const Value& dst, const Value& source, bool is_volatile) -> void;

			auto createRet() -> void;
			auto createRet(const Value& value) -> void;

			auto createUnreachable() -> void;
			
			auto createBranch(const BasicBlock& block) -> void;

			auto createCondBranch(const Value& cond, const BasicBlock& then_block, const BasicBlock& else_block)
				-> void;

			// createPhi

			auto createCall(const Function& func, evo::ArrayProxy<Value> params, evo::CStrProxy name = '\0') 
				-> CallInst;
			auto createIntrinsicCall(
				IntrinsicID id, const Type& return_type, evo::ArrayProxy<Value> params, evo::CStrProxy name = '\0'
			) -> CallInst;

			auto createMemSetInline(const Value& dst, const Value& value, const Value& size, bool is_volatile) -> void;


			///////////////////////////////////
			// type conversion

			EVO_NODISCARD auto createTrunc(const Value& value, const Type& dst_type, evo::CStrProxy name = '\0')
				-> Value;
			EVO_NODISCARD auto createFPTrunc(const Value& value, const Type& dst_type, evo::CStrProxy name = '\0')
				-> Value;

			EVO_NODISCARD auto createZExt(const Value& value, const Type& dst_type, evo::CStrProxy name = '\0')
				-> Value;
			EVO_NODISCARD auto createSExt(const Value& value, const Type& dst_type, evo::CStrProxy name = '\0')
				-> Value;
			EVO_NODISCARD auto createFPExt(const Value& value, const Type& dst_type, evo::CStrProxy name = '\0')
				-> Value;

			EVO_NODISCARD auto createSIToFP(const Value& value, const Type& dst_type, evo::CStrProxy name = '\0')
				-> Value;
			EVO_NODISCARD auto createUIToFP(const Value& value, const Type& dst_type, evo::CStrProxy name = '\0')
				-> Value;
			EVO_NODISCARD auto createFPToSI(const Value& value, const Type& dst_type, evo::CStrProxy name = '\0')
				-> Value;
			EVO_NODISCARD auto createFPToUI(const Value& value, const Type& dst_type, evo::CStrProxy name = '\0')
				-> Value;

			EVO_NODISCARD auto createBitCast(const Value& value, const Type& dst_type, evo::CStrProxy name = '\0')
				-> Value;

			
			///////////////////////////////////
			// element operations

			EVO_NODISCARD auto createExtractValue(
				const Value& value, evo::ArrayProxy<unsigned> indices, evo::CStrProxy name = '\0'
			) -> Value;


			///////////////////////////////////
			// operators

			EVO_NODISCARD auto createAdd(
				const Value& lhs, const Value& rhs, bool nuw, bool nsw, evo::CStrProxy name = '\0'
			) -> Value;
			EVO_NODISCARD auto createFAdd(const Value& lhs, const Value& rhs, evo::CStrProxy name = '\0') -> Value;

			EVO_NODISCARD auto createSub(
				const Value& lhs, const Value& rhs, bool nuw, bool nsw, evo::CStrProxy name = '\0'
			) -> Value;
			EVO_NODISCARD auto createFSub(const Value& lhs, const Value& rhs, evo::CStrProxy name = '\0') -> Value;

			EVO_NODISCARD auto createMul(
				const Value& lhs, const Value& rhs, bool nuw, bool nsw, evo::CStrProxy name = '\0'
			) -> Value;
			EVO_NODISCARD auto createFMul(const Value& lhs, const Value& rhs, evo::CStrProxy name = '\0') -> Value;

			EVO_NODISCARD auto createUDiv(const Value& lhs, const Value& rhs, bool exact, evo::CStrProxy name = '\0')
				-> Value;
			EVO_NODISCARD auto createSDiv(const Value& lhs, const Value& rhs, bool exact, evo::CStrProxy name = '\0')
				-> Value;
			EVO_NODISCARD auto createFDiv(const Value& lhs, const Value& rhs, evo::CStrProxy name = '\0') -> Value;

			EVO_NODISCARD auto createURem(const Value& lhs, const Value& rhs, evo::CStrProxy name = '\0') -> Value;
			EVO_NODISCARD auto createSRem(const Value& lhs, const Value& rhs, evo::CStrProxy name = '\0') -> Value;
			EVO_NODISCARD auto createFRem(const Value& lhs, const Value& rhs, evo::CStrProxy name = '\0') -> Value;


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
			// insertion point

			auto setInsertionPoint(const BasicBlock& block) -> void;
			auto setInsertionPointAtBack(const Function& func) -> void;

			EVO_NODISCARD auto getInsertionPoint() -> llvmint::BasicBlock;



			//////////////////////////////////////////////////////////////////////
			// values
			
			EVO_NODISCARD auto getValueBool(bool value) const -> ConstantInt;

			EVO_NODISCARD auto getValueI8(uint8_t value) const -> ConstantInt;
			EVO_NODISCARD auto getValueI16(uint16_t value) const -> ConstantInt;
			EVO_NODISCARD auto getValueI32(uint32_t value) const -> ConstantInt;
			EVO_NODISCARD auto getValueI64(uint64_t value) const -> ConstantInt;
			EVO_NODISCARD auto getValueI128(uint64_t value) const -> ConstantInt;
			
			EVO_NODISCARD auto getValueI_N(unsigned bitwidth, uint64_t value) const -> ConstantInt;

			EVO_NODISCARD auto getValueIntegral(const IntegerType& type, uint64_t value) const -> ConstantInt;

			EVO_NODISCARD auto getValueF16(float64_t value) const -> Constant;
			EVO_NODISCARD auto getValueBF16(float64_t value) const -> Constant;
			EVO_NODISCARD auto getValueF32(float64_t value) const -> Constant;
			EVO_NODISCARD auto getValueF64(float64_t value) const -> Constant;
			EVO_NODISCARD auto getValueF80(float64_t value) const -> Constant;
			EVO_NODISCARD auto getValueF128(float64_t value) const -> Constant;

			EVO_NODISCARD auto getValueFloat(const Type& type, float64_t value) const -> Constant;

			EVO_NODISCARD auto getNaNF16() const -> Constant;
			EVO_NODISCARD auto getNaNBF16() const -> Constant;
			EVO_NODISCARD auto getNaNF32() const -> Constant;
			EVO_NODISCARD auto getNaNF64() const -> Constant;
			EVO_NODISCARD auto getNaNF80() const -> Constant;
			EVO_NODISCARD auto getNaNF128() const -> Constant;

			EVO_NODISCARD auto getInfinityF16() const -> Constant;
			EVO_NODISCARD auto getInfinityBF16() const -> Constant;
			EVO_NODISCARD auto getInfinityF32() const -> Constant;
			EVO_NODISCARD auto getInfinityF64() const -> Constant;
			EVO_NODISCARD auto getInfinityF80() const -> Constant;
			EVO_NODISCARD auto getInfinityF128() const -> Constant;


			EVO_NODISCARD auto getValueGlobalStrPtr(std::string_view str, evo::CStrProxy name = '\0') const -> Constant;


			//////////////////////////////////////////////////////////////////////
			// types

			EVO_NODISCARD auto getFuncProto(
				const Type& return_type, evo::ArrayProxy<Type> params, bool is_var_args
			) const -> FunctionType;

			EVO_NODISCARD auto getStructType(evo::ArrayProxy<Type> members) -> StructType;
			EVO_NODISCARD auto createStructType(
				evo::ArrayProxy<Type> members, bool is_packed, evo::CStrProxy name = '\0'
			) const -> StructType;
			EVO_NODISCARD auto createStructType(evo::ArrayProxy<Type>, const char*) = delete;

			EVO_NODISCARD auto getTypeBool() const -> IntegerType;

			EVO_NODISCARD auto getTypeI8() const -> IntegerType;
			EVO_NODISCARD auto getTypeI16() const -> IntegerType;
			EVO_NODISCARD auto getTypeI32() const -> IntegerType;
			EVO_NODISCARD auto getTypeI64() const -> IntegerType;
			EVO_NODISCARD auto getTypeI128() const -> IntegerType;

			EVO_NODISCARD auto getTypeI_N(unsigned width) const -> IntegerType;

			EVO_NODISCARD auto getTypeF16() const -> Type;
			EVO_NODISCARD auto getTypeBF16() const -> Type;
			EVO_NODISCARD auto getTypeF32() const -> Type;
			EVO_NODISCARD auto getTypeF64() const -> Type;
			EVO_NODISCARD auto getTypeF80() const -> Type;
			EVO_NODISCARD auto getTypeF128() const -> Type;

			EVO_NODISCARD auto getTypePtr() const -> PointerType;

			EVO_NODISCARD auto getTypeVoid() const -> Type;



		private:
			EVO_NODISCARD auto get_native_context() -> llvm::LLVMContext&;

	
		private:
			llvm::IRBuilderBase* builder = nullptr;

			llvm::NoFolder* folder = nullptr;
			llvm::IRBuilderDefaultInserter* inserter = nullptr;
	};

	
}