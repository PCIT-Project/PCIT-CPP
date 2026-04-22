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
#include "./class_impls/types.hpp"
#include "./class_impls/stmts.hpp"
#include "./class_impls/values.hpp"
#include "./Function.hpp"


namespace pcit::core{
	class GenericInt;
	class GenericFloat;
}

namespace pcit::llvmint{

	
	class IRBuilder{
		public:
			enum class IntrinsicID{
				TRAP,
				DEBUG_TRAP,

				LIFETIME_START,
				LIFETIME_END,

				SADD_SAT,
				SADD_OVERFLOW,
				SMUL_FIX_SAT,
				SMUL_OVERFLOW,
				SSHL_SAT,
				SSUB_SAT,
				SSUB_OVERFLOW,

				UADD_SAT,
				UADD_OVERFLOW,
				UMUL_FIX_SAT,
				UMUL_OVERFLOW,
				USHL_SAT,
				USUB_SAT,
				USUB_OVERFLOW,

				BIT_REVERSE,
				BSWAP,
				CTPOP,
				CTLZ,
				CTTZ,
			};

		public:
			IRBuilder(class LLVMContext& context);
			~IRBuilder();


			//////////////////////////////////////////////////////////////////////
			// create

			///////////////////////////////////
			// control flow

			auto createBasicBlock(const Function& func, std::string_view name = {}) -> BasicBlock;

			auto createRet() -> ReturnInst;
			auto createRet(const Value& value) -> ReturnInst;

			auto createUnreachable() -> void;
			
			auto createBranch(const BasicBlock& block) -> BranchInst;

			auto createCondBranch(const Value& cond, const BasicBlock& then_block, const BasicBlock& else_block)
				-> void;

			struct Incoming{
				Value value;
				BasicBlock block;
			};
			auto createPhi(const Type& type, evo::ArrayProxy<Incoming> incoming, std::string_view name = {}) -> Value;

			struct Case{
				ConstantInt value;
				BasicBlock block;
			};
			auto createSwitch(const Value& cond, BasicBlock default_block, evo::ArrayProxy<Case> cases) -> void;


			auto createCall(const Function& func, evo::ArrayProxy<Value> params, std::string_view name = {}) 
				-> CallInst;
			auto createCall(
				const Value& value, const FunctionType& type, evo::ArrayProxy<Value> params, std::string_view name = {}
			)  -> CallInst;
			auto createIntrinsicCall(
				IntrinsicID id, const Type& return_type, evo::ArrayProxy<Value> params, std::string_view name = {}
			) -> CallInst;

			auto createMemCpyInline(const Value& dst, const Value& src, const Value& size, bool is_volatile) -> void;
			auto createMemSetInline(const Value& dst, const Value& value, const Value& size, bool is_volatile) -> void;


			///////////////////////////////////
			// memory

			[[nodiscard]] auto createAlloca(const Type& type, std::string_view name = {}) -> Alloca;

			[[nodiscard]] auto createAlloca(const Type& type, const Value& array_length, std::string_view name = {}) 
			-> Alloca;
			
			[[nodiscard]] auto createLoad(
				const Value& value,
				const Type& type,
				bool is_volatile,
				AtomicOrdering atomic_ordering = AtomicOrdering::NotAtomic,
				std::string_view name = {}
			) -> LoadInst;

			[[nodiscard]] auto createLoad(
				const Alloca& alloca,
				bool is_volatile,
				AtomicOrdering atomic_ordering = AtomicOrdering::NotAtomic,
				std::string_view name = {}
			) -> LoadInst;

			[[nodiscard]] auto createLoad(
				const GlobalVariable& global_var,
				bool is_volatile,
				AtomicOrdering atomic_ordering = AtomicOrdering::NotAtomic,
				std::string_view name = {}
			) -> LoadInst;


			auto createStore(
				const Alloca& dst,
				const Value& source,
				bool is_volatile,
				AtomicOrdering atomic_ordering = AtomicOrdering::NotAtomic
			) -> StoreInst;

			auto createStore(
				const Value& dst,
				const Value& source,
				bool is_volatile,
				AtomicOrdering atomic_ordering = AtomicOrdering::NotAtomic
			) -> StoreInst;


			///////////////////////////////////
			// type conversion

			[[nodiscard]] auto createTrunc(const Value& value, const Type& dst_type, std::string_view name = {})
				-> Value;
			[[nodiscard]] auto createFTrunc(const Value& value, const Type& dst_type, std::string_view name = {})
				-> Value;

			[[nodiscard]] auto createZExt(const Value& value, const Type& dst_type, std::string_view name = {})
				-> Value;
			[[nodiscard]] auto createSExt(const Value& value, const Type& dst_type, std::string_view name = {})
				-> Value;
			[[nodiscard]] auto createFExt(const Value& value, const Type& dst_type, std::string_view name = {})
				-> Value;

			[[nodiscard]] auto createIToF(const Value& value, const Type& dst_type, std::string_view name = {})
				-> Value;
			[[nodiscard]] auto createUIToF(const Value& value, const Type& dst_type, std::string_view name = {})
				-> Value;
			[[nodiscard]] auto createFToI(const Value& value, const Type& dst_type, std::string_view name = {})
				-> Value;
			[[nodiscard]] auto createFToUI(const Value& value, const Type& dst_type, std::string_view name = {})
				-> Value;

			[[nodiscard]] auto createBitCast(const Value& value, const Type& dst_type, std::string_view name = {})
				-> Value;

			
			///////////////////////////////////
			// element operations

			[[nodiscard]] auto createExtractValue(
				const Value& value, evo::ArrayProxy<unsigned> indices, std::string_view name = {}
			) -> Value;


			[[nodiscard]] auto createGetElementPtr(
				const Type& type, const Value& value, evo::ArrayProxy<Value> indices, std::string_view name = {}
			) -> Value;


			///////////////////////////////////
			// operators

			[[nodiscard]] auto createAdd(
				const Value& lhs, const Value& rhs, bool nuw, bool nsw, std::string_view name = {}
			) -> Value;
			[[nodiscard]] auto createFAdd(const Value& lhs, const Value& rhs, std::string_view name = {}) -> Value;

			[[nodiscard]] auto createSub(
				const Value& lhs, const Value& rhs, bool nuw, bool nsw, std::string_view name = {}
			) -> Value;
			[[nodiscard]] auto createFSub(const Value& lhs, const Value& rhs, std::string_view name = {}) -> Value;

			[[nodiscard]] auto createMul(
				const Value& lhs, const Value& rhs, bool nuw, bool nsw, std::string_view name = {}
			) -> Value;
			[[nodiscard]] auto createFMul(const Value& lhs, const Value& rhs, std::string_view name = {}) -> Value;

			[[nodiscard]] auto createUDiv(const Value& lhs, const Value& rhs, bool exact, std::string_view name = {})
				-> Value;
			[[nodiscard]] auto createSDiv(const Value& lhs, const Value& rhs, bool exact, std::string_view name = {})
				-> Value;
			[[nodiscard]] auto createFDiv(const Value& lhs, const Value& rhs, std::string_view name = {}) -> Value;

			[[nodiscard]] auto createURem(const Value& lhs, const Value& rhs, std::string_view name = {}) -> Value;
			[[nodiscard]] auto createSRem(const Value& lhs, const Value& rhs, std::string_view name = {}) -> Value;
			[[nodiscard]] auto createFRem(const Value& lhs, const Value& rhs, std::string_view name = {}) -> Value;

			[[nodiscard]] auto createFNeg(const Value& rhs, std::string_view name = {}) -> Value;



			[[nodiscard]] auto createICmpEQ(const Value& lhs, const Value& rhs, std::string_view name = {}) -> Value;
			[[nodiscard]] auto createICmpNE(const Value& lhs, const Value& rhs, std::string_view name = {}) -> Value;

			[[nodiscard]] auto createICmpUGT(const Value& lhs, const Value& rhs, std::string_view name = {}) -> Value;
			[[nodiscard]] auto createICmpUGE(const Value& lhs, const Value& rhs, std::string_view name = {}) -> Value;
			[[nodiscard]] auto createICmpULT(const Value& lhs, const Value& rhs, std::string_view name = {}) -> Value;
			[[nodiscard]] auto createICmpULE(const Value& lhs, const Value& rhs, std::string_view name = {}) -> Value;
			
			[[nodiscard]] auto createICmpSGT(const Value& lhs, const Value& rhs, std::string_view name = {}) -> Value;
			[[nodiscard]] auto createICmpSGE(const Value& lhs, const Value& rhs, std::string_view name = {}) -> Value;
			[[nodiscard]] auto createICmpSLT(const Value& lhs, const Value& rhs, std::string_view name = {}) -> Value;
			[[nodiscard]] auto createICmpSLE(const Value& lhs, const Value& rhs, std::string_view name = {}) -> Value;


			[[nodiscard]] auto createFCmpEQ(const Value& lhs, const Value& rhs, std::string_view name = {}) -> Value;
			[[nodiscard]] auto createFCmpNE(const Value& lhs, const Value& rhs, std::string_view name = {}) -> Value;

			[[nodiscard]] auto createFCmpGT(const Value& lhs, const Value& rhs, std::string_view name = {}) -> Value;
			[[nodiscard]] auto createFCmpGE(const Value& lhs, const Value& rhs, std::string_view name = {}) -> Value;
			[[nodiscard]] auto createFCmpLT(const Value& lhs, const Value& rhs, std::string_view name = {}) -> Value;
			[[nodiscard]] auto createFCmpLE(const Value& lhs, const Value& rhs, std::string_view name = {}) -> Value;

			

			[[nodiscard]] auto createAnd(const Value& lhs, const Value& rhs, std::string_view name = {}) -> Value;
			[[nodiscard]] auto createOr(const Value& lhs, const Value& rhs, std::string_view name = {}) -> Value;
			[[nodiscard]] auto createXor(const Value& lhs, const Value& rhs, std::string_view name = {}) -> Value;
			[[nodiscard]] auto createSHL(
				const Value& lhs, const Value& rhs, bool nuw, bool nsw, std::string_view name = {}
			) -> Value;
			[[nodiscard]] auto createASHR(const Value& lhs, const Value& rhs, bool exact, std::string_view name = {})
				-> Value;
			[[nodiscard]] auto createLSHR(const Value& lhs, const Value& rhs, bool exact, std::string_view name = {})
				-> Value;


			//////////////////////////////////////////////////////////////////////
			// atomics

			[[nodiscard]] auto createCmpXchg(
				const Value& target,
				const Value& expected,
				const Value& desired,
				bool isWeak = false,
				AtomicOrdering success_ordering = AtomicOrdering::SequentiallyConsistent,
				AtomicOrdering failure_ordering = AtomicOrdering::SequentiallyConsistent,
				std::string_view name = {}
			) -> Value;


			[[nodiscard]] auto createAtomicRMW(
				AtomicRMWOp op,
				const Value& target,
				const Value& value,
				AtomicOrdering ordering,
				std::string_view name = {}
			) -> Value;


			//////////////////////////////////////////////////////////////////////
			// insertion point

			auto setInsertionPoint(const BasicBlock& block) -> void;
			auto setInsertionPointAtBack(const Function& func) -> void;

			[[nodiscard]] auto getInsertionPoint() -> llvmint::BasicBlock;



			//////////////////////////////////////////////////////////////////////
			// values
			
			[[nodiscard]] auto getValueNull() const -> Constant;


			[[nodiscard]] auto getValueBool(bool value) const -> ConstantInt;

			[[nodiscard]] auto getValueI8(uint8_t value) const -> ConstantInt;
			[[nodiscard]] auto getValueI16(uint16_t value) const -> ConstantInt;
			[[nodiscard]] auto getValueI32(uint32_t value) const -> ConstantInt;
			[[nodiscard]] auto getValueI64(uint64_t value) const -> ConstantInt;
			[[nodiscard]] auto getValueI128(uint64_t value) const -> ConstantInt;
			
			[[nodiscard]] auto getValueI_N(unsigned bitwidth, uint64_t value) const -> ConstantInt;
			[[nodiscard]] auto getValueI_N(
				unsigned bitwidth, bool is_unsigned, const class core::GenericInt& value
			) const -> ConstantInt;

			[[nodiscard]] auto getValueIntegral(const IntegerType& type, uint64_t value) const -> ConstantInt;

			[[nodiscard]] auto getValueF16(evo::float64_t value) const -> Constant;
			[[nodiscard]] auto getValueF32(evo::float64_t value) const -> Constant;
			[[nodiscard]] auto getValueF64(evo::float64_t value) const -> Constant;
			[[nodiscard]] auto getValueF80(evo::float64_t value) const -> Constant;
			[[nodiscard]] auto getValueF128(evo::float64_t value) const -> Constant;

			[[nodiscard]] auto getValueFloat(const Type& type, evo::float64_t value) const -> Constant;
			[[nodiscard]] auto getValueFloat(const Type& type, const class core::GenericFloat& value) const -> Constant;

			[[nodiscard]] auto getNaNF16() const -> Constant;
			[[nodiscard]] auto getNaNF32() const -> Constant;
			[[nodiscard]] auto getNaNF64() const -> Constant;
			[[nodiscard]] auto getNaNF80() const -> Constant;
			[[nodiscard]] auto getNaNF128() const -> Constant;

			[[nodiscard]] auto getInfinityF16() const -> Constant;
			[[nodiscard]] auto getInfinityF32() const -> Constant;
			[[nodiscard]] auto getInfinityF64() const -> Constant;
			[[nodiscard]] auto getInfinityF80() const -> Constant;
			[[nodiscard]] auto getInfinityF128() const -> Constant;


			[[nodiscard]] auto getValueGlobalStr(std::string_view str, std::string_view name = {}) const -> Constant;

			[[nodiscard]] auto getValueGlobalStr(const std::string& value) const -> Constant;
			[[nodiscard]] auto getValueGlobalUndefValue(const Type& type) const -> Constant;
			[[nodiscard]] auto getValueGlobalAggregateZero(const Type& type) const -> Constant;
			[[nodiscard]] auto getValueGlobalByteArray(evo::ArrayProxy<std::byte> bytes) const -> Constant;
			[[nodiscard]] auto getValueGlobalArray(const Type& elem_type, evo::ArrayProxy<Constant> values) const
				-> Constant;
			[[nodiscard]] auto getValueGlobalStruct(const StructType& type, evo::ArrayProxy<Constant> values) const
				-> Constant;


			//////////////////////////////////////////////////////////////////////
			// types

			[[nodiscard]] auto getFuncProto(
				const Type& return_type, evo::ArrayProxy<Type> params, bool is_var_args
			) const -> FunctionType;

			[[nodiscard]] auto getArrayType(const Type& elem_type, uint64_t length) const -> ArrayType;

			[[nodiscard]] auto getStructType(evo::ArrayProxy<Type> members) -> StructType;
			[[nodiscard]] auto createStructType(
				evo::ArrayProxy<Type> members, bool is_packed, std::string_view name = {}
			) const -> StructType;
			[[nodiscard]] auto createStructType(evo::ArrayProxy<Type>, const char*) = delete;

			[[nodiscard]] auto getTypeBool() const -> IntegerType;

			[[nodiscard]] auto getTypeI8() const -> IntegerType;
			[[nodiscard]] auto getTypeI16() const -> IntegerType;
			[[nodiscard]] auto getTypeI32() const -> IntegerType;
			[[nodiscard]] auto getTypeI64() const -> IntegerType;
			[[nodiscard]] auto getTypeI128() const -> IntegerType;

			[[nodiscard]] auto getTypeI_N(unsigned width) const -> IntegerType;

			[[nodiscard]] auto getTypeF16() const -> Type;
			[[nodiscard]] auto getTypeF32() const -> Type;
			[[nodiscard]] auto getTypeF64() const -> Type;
			[[nodiscard]] auto getTypeF80() const -> Type;
			[[nodiscard]] auto getTypeF128() const -> Type;

			[[nodiscard]] auto getTypePtr() const -> PointerType;

			[[nodiscard]] auto getTypeVoid() const -> Type;



		private:
			[[nodiscard]] auto get_native_context() const -> llvm::LLVMContext&;

	
		private:
			llvm::IRBuilderBase* builder = nullptr;

			llvm::NoFolder* folder = nullptr;
			llvm::IRBuilderDefaultInserter* inserter = nullptr;
	};

	
}