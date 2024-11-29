////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/IRBuilder.h"

#include <LLVM.h>

#include "../include/LLVMContext.h"

#include "../../../include/generic_values/GenericInt.h"
#include "../../../include/generic_values/GenericFloat.h"



#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::llvmint{

	//////////////////////////////////////////////////////////////////////
	// helpers


	template<class LLVM_TYPE, class LLVMINT_TYPE>
	EVO_NODISCARD static auto createArrayRef(evo::ArrayProxy<LLVMINT_TYPE> arr_proxy) -> llvm::ArrayRef<LLVM_TYPE*> {
		return llvm::ArrayRef((LLVM_TYPE**)arr_proxy.data(), arr_proxy.size());
	}


	EVO_NODISCARD static auto convertGenericFloatToAPFloat(const core::GenericFloat& value) -> llvm::APFloat {
		return evo::bitCast<llvm::APFloat>(value.copyToLLVMNative());
	}



	//////////////////////////////////////////////////////////////////////
	// IRBuilder


	IRBuilder::IRBuilder(LLVMContext& context){
		this->folder = new llvm::NoFolder();
		this->inserter = new llvm::IRBuilderDefaultInserter();


		llvm::MDNode* fp_math_tag = nullptr;
		llvm::ArrayRef<llvm::OperandBundleDef> op_bundles = std::nullopt;

		this->builder = new llvm::IRBuilderBase(
			*context.native(), *this->folder, *this->inserter, fp_math_tag, op_bundles
		);
	}
	
	IRBuilder::~IRBuilder(){
		delete this->builder;
		delete this->folder;
		delete this->inserter;
	}




	//////////////////////////////////////////////////////////////////////
	// control flow

	auto IRBuilder::createBasicBlock(const Function& func, evo::CStrProxy name) -> BasicBlock {
		return BasicBlock(llvm::BasicBlock::Create(this->get_native_context(), name.c_str(), func.native()));
	}

	auto IRBuilder::createRet() -> void {
		this->builder->CreateRetVoid();
	}

	auto IRBuilder::createRet(const Value& value) -> void {
		this->builder->CreateRet(value.native());
	}


	auto IRBuilder::createUnreachable() -> void {
		this->builder->CreateUnreachable();
	}


	auto IRBuilder::createBranch(const BasicBlock& block) -> void {
		this->builder->CreateBr(block.native());
	}

	auto IRBuilder::createCondBranch(const Value& cond, const BasicBlock& then_block, const BasicBlock& else_block)
	-> void {
		this->builder->CreateCondBr(cond.native(), then_block.native(), else_block.native());
	}



	auto IRBuilder::createCall(const Function& func, evo::ArrayProxy<Value> params, evo::CStrProxy name) -> CallInst {
		return this->builder->CreateCall(func.native(), createArrayRef<llvm::Value>(params), name.c_str());
	}

	auto IRBuilder::createCall(const Value& value, const FunctionType& type, evo::ArrayProxy<Value> params, evo::CStrProxy name) -> CallInst {
		return this->builder->CreateCall(
			type.native(), value.native(), createArrayRef<llvm::Value>(params), name.c_str()
		);
	}

	auto IRBuilder::createIntrinsicCall(
		IntrinsicID id, const Type& return_type, evo::ArrayProxy<Value> params, evo::CStrProxy name
	)-> CallInst {

		// llvm/IR/IntrinsicEnums.inc
		const llvm::Intrinsic::ID intrinsic_id = [&]() -> llvm::Intrinsic::ID {
			switch(id){
				case IntrinsicID::debugtrap:    return llvm::Intrinsic::IndependentIntrinsics::debugtrap;

				case IntrinsicID::saddSat:      return llvm::Intrinsic::IndependentIntrinsics::sadd_sat;
				case IntrinsicID::saddOverflow: return llvm::Intrinsic::IndependentIntrinsics::sadd_with_overflow;
				case IntrinsicID::smulFixSat:   return llvm::Intrinsic::IndependentIntrinsics::smul_fix_sat;
				case IntrinsicID::smulOverflow: return llvm::Intrinsic::IndependentIntrinsics::smul_with_overflow;
				case IntrinsicID::sshlSat:      return llvm::Intrinsic::IndependentIntrinsics::sshl_sat;
				case IntrinsicID::ssubSat:      return llvm::Intrinsic::IndependentIntrinsics::ssub_sat;
				case IntrinsicID::ssubOverflow: return llvm::Intrinsic::IndependentIntrinsics::ssub_with_overflow;

				case IntrinsicID::uaddSat:      return llvm::Intrinsic::IndependentIntrinsics::uadd_sat;
				case IntrinsicID::uaddOverflow: return llvm::Intrinsic::IndependentIntrinsics::uadd_with_overflow;
				case IntrinsicID::umulFixSat:   return llvm::Intrinsic::IndependentIntrinsics::umul_fix_sat;
				case IntrinsicID::umulOverflow: return llvm::Intrinsic::IndependentIntrinsics::umul_with_overflow;
				case IntrinsicID::ushlSat:      return llvm::Intrinsic::IndependentIntrinsics::ushl_sat;
				case IntrinsicID::usubSat:      return llvm::Intrinsic::IndependentIntrinsics::usub_sat;
				case IntrinsicID::usubOverflow: return llvm::Intrinsic::IndependentIntrinsics::usub_with_overflow;
			}

			evo::debugFatalBreak("Unknown or unsupported intrinsic id");
		}();

		return this->builder->CreateIntrinsic(
			return_type.native(), intrinsic_id, createArrayRef<llvm::Value>(params), nullptr, name.c_str()
		);
	};


	auto IRBuilder::createMemSetInline(const Value& dst, const Value& value, const Value& size, bool is_volatile)
	-> void {
		this->builder->CreateMemSetInline(dst.native(), llvm::MaybeAlign(), value.native(), size.native(), is_volatile);
	}


	auto IRBuilder::createAlloca(const Type& type, evo::CStrProxy name) -> Alloca {
		return Alloca(this->builder->CreateAlloca(type.native(), nullptr, name.c_str()));
	}

	auto IRBuilder::createAlloca(const Type& type, const Value& array_length, evo::CStrProxy name) -> Alloca {
		return Alloca(this->builder->CreateAlloca(type.native(), array_length.native(), name.c_str()));
	}
	


	//////////////////////////////////////////////////////////////////////
	// memory

	auto IRBuilder::createLoad(
		const Value& value, const Type& type, bool is_volatile, AtomicOrdering atomic_ordering, evo::CStrProxy name
	) -> LoadInst {
		llvm::LoadInst* load_inst = this->builder->CreateLoad(type.native(), value.native(), is_volatile, name.c_str());
		load_inst->setAtomic(static_cast<llvm::AtomicOrdering>(atomic_ordering));
		return LoadInst(load_inst);
	}

	auto IRBuilder::createLoad(
		const Alloca& alloca, bool is_volatile, AtomicOrdering atomic_ordering, evo::CStrProxy name
	) -> LoadInst {
		return this->createLoad(alloca.asValue(), alloca.getAllocatedType(), is_volatile, atomic_ordering, name);
	}

	auto IRBuilder::createLoad(
		const GlobalVariable& global_var, bool is_volatile, AtomicOrdering atomic_ordering, evo::CStrProxy name
	) -> LoadInst {
		return this->createLoad(global_var.asValue(), global_var.getType(), is_volatile, atomic_ordering, name);
	}


	auto IRBuilder::createStore(
		const Alloca& dst, const Value& source, bool is_volatile, AtomicOrdering atomic_ordering
	) -> void {
		llvm::StoreInst* store_inst = this->builder->CreateStore(source.native(), dst.native(), is_volatile);
		store_inst->setAtomic(static_cast<llvm::AtomicOrdering>(atomic_ordering));
	}

	auto IRBuilder::createStore(
		const Value& dst, const Value& source, bool is_volatile, AtomicOrdering atomic_ordering
	) -> void {
		llvm::StoreInst* store_inst = this->builder->CreateStore(source.native(), dst.native(), is_volatile);
		store_inst->setAtomic(static_cast<llvm::AtomicOrdering>(atomic_ordering));
	}



	//////////////////////////////////////////////////////////////////////
	// type conversion

	// doesn't use NUW or NSW as for some reason it triggers an assert inside of LLVM
	auto IRBuilder::createTrunc(const Value& value, const Type& dst_type, evo::CStrProxy name)
	-> Value {
		return Value(this->builder->CreateTrunc(value.native(), dst_type.native(), name.c_str()));
	}
	auto IRBuilder::createFPTrunc(const Value& value, const Type& dst_type, evo::CStrProxy name)
	-> Value {
		return Value(this->builder->CreateFPTrunc(value.native(), dst_type.native(), name.c_str()));
	}
	
	auto IRBuilder::createZExt(const Value& value, const Type& dst_type, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateZExt(value.native(), dst_type.native(), name.c_str()));
	}
	auto IRBuilder::createSExt(const Value& value, const Type& dst_type, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateSExt(value.native(), dst_type.native(), name.c_str()));
	}
	auto IRBuilder::createFPExt(const Value& value, const Type& dst_type, evo::CStrProxy name)
	-> Value {
		return Value(this->builder->CreateFPExt(value.native(), dst_type.native(), name.c_str()));
	}


	auto IRBuilder::createSIToFP(const Value& value, const Type& dst_type, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateSIToFP(value.native(), dst_type.native(), name.c_str()));
	}
	auto IRBuilder::createUIToFP(const Value& value, const Type& dst_type, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateUIToFP(value.native(), dst_type.native(), name.c_str()));
	}
	auto IRBuilder::createFPToSI(const Value& value, const Type& dst_type, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateFPToSI(value.native(), dst_type.native(), name.c_str()));
	}
	auto IRBuilder::createFPToUI(const Value& value, const Type& dst_type, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateFPToUI(value.native(), dst_type.native(), name.c_str()));
	}
	
	
	auto IRBuilder::createBitCast(const Value& value, const Type& dst_type, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateBitOrPointerCast(value.native(), dst_type.native(), name.c_str()));
	}

	//////////////////////////////////////////////////////////////////////
	// element operations

	auto IRBuilder::createExtractValue(const Value& value, evo::ArrayProxy<unsigned> indices, evo::CStrProxy name)
	-> Value {
		return Value(
			this->builder->CreateExtractValue(
				value.native(), llvm::ArrayRef(indices.data(), indices.size()), name.c_str()
			)
		);
	}


	//////////////////////////////////////////////////////////////////////
	// operators

	auto IRBuilder::createAdd(const Value& lhs, const Value& rhs, bool nuw, bool nsw, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateAdd(lhs.native(), rhs.native(), name.c_str(), nuw, nsw));
	}

	auto IRBuilder::createFAdd(const Value& lhs, const Value& rhs, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateFAdd(lhs.native(), rhs.native(), name.c_str()));
	}


	auto IRBuilder::createSub(const Value& lhs, const Value& rhs, bool nuw, bool nsw, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateSub(lhs.native(), rhs.native(), name.c_str(), nuw, nsw));
	}

	auto IRBuilder::createFSub(const Value& lhs, const Value& rhs, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateFSub(lhs.native(), rhs.native(), name.c_str()));
	}


	auto IRBuilder::createMul(const Value& lhs, const Value& rhs, bool nuw, bool nsw, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateMul(lhs.native(), rhs.native(), name.c_str(), nuw, nsw));
	}

	auto IRBuilder::createFMul(const Value& lhs, const Value& rhs, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateFMul(lhs.native(), rhs.native(), name.c_str()));
	}


	auto IRBuilder::createUDiv(const Value& lhs, const Value& rhs, bool exact, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateUDiv(lhs.native(), rhs.native(), name.c_str(), exact));
	}

	auto IRBuilder::createSDiv(const Value& lhs, const Value& rhs, bool exact, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateSDiv(lhs.native(), rhs.native(), name.c_str(), exact));
	}

	auto IRBuilder::createFDiv(const Value& lhs, const Value& rhs, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateFDiv(lhs.native(), rhs.native(), name.c_str()));
	}


	auto IRBuilder::createURem(const Value& lhs, const Value& rhs, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateURem(lhs.native(), rhs.native(), name.c_str()));
	}

	auto IRBuilder::createSRem(const Value& lhs, const Value& rhs, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateSRem(lhs.native(), rhs.native(), name.c_str()));
	}

	auto IRBuilder::createFRem(const Value& lhs, const Value& rhs, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateFRem(lhs.native(), rhs.native(), name.c_str()));
	}



	auto IRBuilder::createICmpEQ(const Value& lhs, const Value& rhs, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateICmpEQ(lhs.native(), rhs.native(), name.c_str()));
	}
	auto IRBuilder::createICmpNE(const Value& lhs, const Value& rhs, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateICmpNE(lhs.native(), rhs.native(), name.c_str()));
	}

	auto IRBuilder::createICmpUGT(const Value& lhs, const Value& rhs, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateICmpUGT(lhs.native(), rhs.native(), name.c_str()));
	}
	auto IRBuilder::createICmpUGE(const Value& lhs, const Value& rhs, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateICmpUGE(lhs.native(), rhs.native(), name.c_str()));
	}
	auto IRBuilder::createICmpULT(const Value& lhs, const Value& rhs, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateICmpULT(lhs.native(), rhs.native(), name.c_str()));
	}
	auto IRBuilder::createICmpULE(const Value& lhs, const Value& rhs, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateICmpULE(lhs.native(), rhs.native(), name.c_str()));
	}

	auto IRBuilder::createICmpSGT(const Value& lhs, const Value& rhs, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateICmpSGT(lhs.native(), rhs.native(), name.c_str()));
	}
	auto IRBuilder::createICmpSGE(const Value& lhs, const Value& rhs, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateICmpSGE(lhs.native(), rhs.native(), name.c_str()));
	}
	auto IRBuilder::createICmpSLT(const Value& lhs, const Value& rhs, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateICmpSLT(lhs.native(), rhs.native(), name.c_str()));
	}
	auto IRBuilder::createICmpSLE(const Value& lhs, const Value& rhs, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateICmpSLE(lhs.native(), rhs.native(), name.c_str()));
	}


	auto IRBuilder::createFCmpEQ(const Value& lhs, const Value& rhs, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateFCmpOEQ(lhs.native(), rhs.native(), name.c_str()));
	}
	auto IRBuilder::createFCmpNE(const Value& lhs, const Value& rhs, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateFCmpONE(lhs.native(), rhs.native(), name.c_str()));
	}
	auto IRBuilder::createFCmpGT(const Value& lhs, const Value& rhs, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateFCmpOGT(lhs.native(), rhs.native(), name.c_str()));
	}
	auto IRBuilder::createFCmpGE(const Value& lhs, const Value& rhs, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateFCmpOGE(lhs.native(), rhs.native(), name.c_str()));
	}
	auto IRBuilder::createFCmpLT(const Value& lhs, const Value& rhs, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateFCmpOLT(lhs.native(), rhs.native(), name.c_str()));
	}
	auto IRBuilder::createFCmpLE(const Value& lhs, const Value& rhs, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateFCmpOLE(lhs.native(), rhs.native(), name.c_str()));
	}



	auto IRBuilder::createAnd(const Value& lhs, const Value& rhs, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateAnd(lhs.native(), rhs.native(), name.c_str()));
	}
	auto IRBuilder::createOr(const Value& lhs, const Value& rhs, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateOr(lhs.native(), rhs.native(), name.c_str()));
	}
	auto IRBuilder::createXor(const Value& lhs, const Value& rhs, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateXor(lhs.native(), rhs.native(), name.c_str()));
	}
	auto IRBuilder::createSHL(const Value& lhs, const Value& rhs, bool nuw, bool nsw, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateShl(lhs.native(), rhs.native(), name.c_str(), nuw, nsw));
	}
	auto IRBuilder::createASHR(const Value& lhs, const Value& rhs, bool exact, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateAShr(lhs.native(), rhs.native(), name.c_str(), exact));
	}
	auto IRBuilder::createLSHR(const Value& lhs, const Value& rhs, bool exact, evo::CStrProxy name) -> Value {
		return Value(this->builder->CreateLShr(lhs.native(), rhs.native(), name.c_str(), exact));
	}



	//////////////////////////////////////////////////////////////////////
	// insertion point

	auto IRBuilder::setInsertionPoint(const BasicBlock& block) -> void {
		this->builder->SetInsertPoint(block.native());
	}

	auto IRBuilder::setInsertionPointAtBack(const Function& function) -> void {
		this->setInsertionPoint(function.back());
	}

	auto IRBuilder::getInsertionPoint() -> llvmint::BasicBlock {
		return llvmint::BasicBlock(this->builder->GetInsertBlock());
	}


	//////////////////////////////////////////////////////////////////////
	// values


	auto IRBuilder::getValueBool(bool value) const -> ConstantInt {
		return ConstantInt(this->builder->getInt1(value));
	}


	auto IRBuilder::getValueI8(uint8_t value) const -> ConstantInt {
		return ConstantInt(this->builder->getInt8(value));
	}

	auto IRBuilder::getValueI16(uint16_t value) const -> ConstantInt {
		return ConstantInt(this->builder->getInt16(value));
	}

	auto IRBuilder::getValueI32(uint32_t value) const -> ConstantInt {
		return ConstantInt(this->builder->getInt32(value));
	}

	auto IRBuilder::getValueI64(uint64_t value) const -> ConstantInt {
		return ConstantInt(this->builder->getInt64(value));
	}

	auto IRBuilder::getValueI128(uint64_t value) const -> ConstantInt {
		return this->getValueI_N(128, value);
	}
	

	auto IRBuilder::getValueI_N(unsigned bitwidth, uint64_t value) const -> ConstantInt {
		return ConstantInt(this->builder->getIntN(bitwidth, value));
	}

	auto IRBuilder::getValueI_N(
		unsigned bitwidth, bool is_unsigned, const pcit::core::GenericInt& value
	) const -> ConstantInt {
		return ConstantInt(
			static_cast<llvm::ConstantInt*>(
				llvm::ConstantInt::get(
					this->getTypeI_N(bitwidth).native(),
					evo::unsafeBitCast<llvm::APInt>(value.extOrTrunc(bitwidth, is_unsigned).getNative())
				)
			)
		);
	}

	auto IRBuilder::getValueIntegral(const IntegerType& type, uint64_t value) const -> ConstantInt {
		return ConstantInt(llvm::ConstantInt::get(type.native(), value));
	}



	auto IRBuilder::getValueF16(float64_t value) const -> Constant {
		return this->getValueFloat(this->getTypeF16(), value);
	}

	auto IRBuilder::getValueBF16(float64_t value) const -> Constant {
		return this->getValueFloat(this->getTypeBF16(), value);
	}

	auto IRBuilder::getValueF32(float64_t value) const -> Constant {
		return this->getValueFloat(this->getTypeF32(), value);
	}

	auto IRBuilder::getValueF64(float64_t value) const -> Constant {
		return this->getValueFloat(this->getTypeF64(), value);
	}

	auto IRBuilder::getValueF80(float64_t value) const -> Constant {
		return this->getValueFloat(this->getTypeF80(), value);
	}

	auto IRBuilder::getValueF128(float64_t value) const -> Constant {
		return this->getValueFloat(this->getTypeF128(), value);
	}

	auto IRBuilder::getValueFloat(const Type& type, float64_t value) const -> Constant {
		return Constant(llvm::ConstantFP::get(type.native(), value));
	}

	auto IRBuilder::getValueFloat(const Type& type, const core::GenericFloat& value) const -> Constant {
		return Constant(llvm::ConstantFP::get(type.native(), convertGenericFloatToAPFloat(value)));
	}


	auto IRBuilder::getNaNF16() const -> Constant {
		return Constant(llvm::ConstantFP::getNaN(this->getTypeF16().native()));
	}

	auto IRBuilder::getNaNBF16() const -> Constant {
		return Constant(llvm::ConstantFP::getNaN(this->getTypeBF16().native()));
	}

	auto IRBuilder::getNaNF32() const -> Constant {
		return Constant(llvm::ConstantFP::getNaN(this->getTypeF32().native()));
	}

	auto IRBuilder::getNaNF64() const -> Constant {
		return Constant(llvm::ConstantFP::getNaN(this->getTypeF64().native()));
	}

	auto IRBuilder::getNaNF80() const -> Constant {
		return Constant(llvm::ConstantFP::getNaN(this->getTypeF80().native()));
	}

	auto IRBuilder::getNaNF128() const -> Constant {
		return Constant(llvm::ConstantFP::getNaN(this->getTypeF128().native()));
	}



	auto IRBuilder::getInfinityF16() const -> Constant {
		return Constant(llvm::ConstantFP::getInfinity(this->getTypeF16().native()));
	}

	auto IRBuilder::getInfinityBF16() const -> Constant {
		return Constant(llvm::ConstantFP::getInfinity(this->getTypeBF16().native()));
	}

	auto IRBuilder::getInfinityF32() const -> Constant {
		return Constant(llvm::ConstantFP::getInfinity(this->getTypeF32().native()));
	}

	auto IRBuilder::getInfinityF64() const -> Constant {
		return Constant(llvm::ConstantFP::getInfinity(this->getTypeF64().native()));
	}

	auto IRBuilder::getInfinityF80() const -> Constant {
		return Constant(llvm::ConstantFP::getInfinity(this->getTypeF80().native()));
	}

	auto IRBuilder::getInfinityF128() const -> Constant {
		return Constant(llvm::ConstantFP::getInfinity(this->getTypeF128().native()));
	}


	auto IRBuilder::getValueGlobalStrPtr(std::string_view str, evo::CStrProxy name) const -> Constant {
		return Constant(this->builder->CreateGlobalStringPtr(str, name.c_str()));
	}

	auto IRBuilder::getValueGlobalStr(const std::string& value) const -> Constant {
		return llvm::ConstantDataArray::getString(this->builder->getContext(), value);
	}

	auto IRBuilder::getValueGlobalUndefValue(const Type& type) const -> Constant {
		return llvm::UndefValue::get(type.native());
	}

	auto IRBuilder::getValueGlobalAggregateZero(const Type& type) const -> Constant {
		return llvm::ConstantAggregateZero::get(type.native());
	}

	auto IRBuilder::getValueGlobalArray(evo::ArrayProxy<Constant> values) const -> Constant {
		return llvm::ConstantDataArray::get(this->builder->getContext(), createArrayRef<llvm::Constant>(values));
	}

	auto IRBuilder::getValueGlobalStruct(const StructType& type, evo::ArrayProxy<Constant> values) const -> Constant {
		return llvm::ConstantStruct::get(type.native(), createArrayRef<llvm::Constant>(values));
	}


	

	//////////////////////////////////////////////////////////////////////
	// types

	auto IRBuilder::getFuncProto(
		const Type& return_type, evo::ArrayProxy<Type> params, bool is_var_args
	) const -> FunctionType {
		return FunctionType(
			llvm::FunctionType::get(return_type.native(), createArrayRef<llvm::Type>(params), is_var_args)
		);
	};


	auto IRBuilder::getStructType(evo::ArrayProxy<Type> members) -> StructType {
		return StructType(llvm::StructType::get(this->get_native_context(), createArrayRef<llvm::Type>(members)));
	}

	auto IRBuilder::createStructType(evo::ArrayProxy<Type> members, bool is_packed, evo::CStrProxy name) const
	-> StructType {
		return StructType(llvm::StructType::create(createArrayRef<llvm::Type>(members), name.c_str(), is_packed));
	}


	auto IRBuilder::getTypeBool() const -> IntegerType { return IntegerType(this->builder->getInt1Ty()); }

	auto IRBuilder::getTypeI8() const -> IntegerType { return IntegerType(this->builder->getInt8Ty()); }
	auto IRBuilder::getTypeI16() const -> IntegerType { return IntegerType(this->builder->getInt16Ty()); }
	auto IRBuilder::getTypeI32() const -> IntegerType { return IntegerType(this->builder->getInt32Ty()); }
	auto IRBuilder::getTypeI64() const -> IntegerType { return IntegerType(this->builder->getInt64Ty()); }
	auto IRBuilder::getTypeI128() const -> IntegerType { return IntegerType(this->builder->getInt128Ty()); }

	auto IRBuilder::getTypeI_N(unsigned width) const -> IntegerType {
		return IntegerType(this->builder->getIntNTy(width));
	}

	auto IRBuilder::getTypeF16() const -> Type { return Type(this->builder->getHalfTy()); }
	auto IRBuilder::getTypeBF16() const -> Type { return Type(this->builder->getBFloatTy()); }
	auto IRBuilder::getTypeF32() const -> Type { return Type(this->builder->getFloatTy()); }
	auto IRBuilder::getTypeF64() const -> Type { return Type(this->builder->getDoubleTy()); }
	auto IRBuilder::getTypeF80() const -> Type { return Type(llvm::Type::getX86_FP80Ty(this->builder->getContext())); }
	auto IRBuilder::getTypeF128() const -> Type { return Type(llvm::Type::getFP128Ty(this->builder->getContext())); }

	auto IRBuilder::getTypePtr() const -> PointerType { return PointerType(this->builder->getPtrTy()); }

	auto IRBuilder::getTypeVoid() const -> Type { return Type(this->builder->getVoidTy()); };



	//////////////////////////////////////////////////////////////////////
	// getters


	auto IRBuilder::get_native_context() -> llvm::LLVMContext& {
		return this->builder->getContext();
	}
		
}