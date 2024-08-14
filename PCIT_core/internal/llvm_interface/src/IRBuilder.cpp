//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "../include/IRBuilder.h"

#include <LLVM.h>

#include "../include/LLVMContext.h"


namespace pcit::llvmint{

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
	// create

	auto IRBuilder::createBasicBlock(const Function& func, evo::CStrProxy name) -> BasicBlock {
		return BasicBlock(llvm::BasicBlock::Create(this->get_native_context(), name.c_str(), func.native()));
	}


	auto IRBuilder::createAlloca(const Type& type, evo::CStrProxy name) -> Alloca {
		return Alloca(this->builder->CreateAlloca(type.native(), nullptr, name.c_str()));
	};


	auto IRBuilder::createStore(const Alloca& dst, const Value& source, bool is_volatile) -> void {
		this->builder->CreateStore(source.native(), dst.native(), is_volatile);
	}

	auto IRBuilder::createStore(const Value& dst, const Value& source, bool is_volatile) -> void {
		this->builder->CreateStore(source.native(), dst.native(), is_volatile);
	}



	auto IRBuilder::createRet() -> void {
		this->builder->CreateRetVoid();
	}

	

	//////////////////////////////////////////////////////////////////////
	// set

	auto IRBuilder::setInsertionPoint(const BasicBlock& block) -> void {
		this->builder->SetInsertPoint(block.native());
	};

	auto IRBuilder::setInsertionPointAtBack(const Function& function) -> void {
		this->setInsertionPoint(function.back());
	};


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
	

	auto IRBuilder::getValueI_N(evo::uint bitwidth, uint64_t value) const -> ConstantInt {
		return ConstantInt(this->builder->getIntN(bitwidth, value));
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

	auto IRBuilder::getValueF128(float64_t value) const -> Constant {
		return this->getValueFloat(this->getTypeF128(), value);
	}

	auto IRBuilder::getValueFloat(const Type& type, float64_t value) const -> Constant {
		return Constant(llvm::ConstantFP::get(type.native(), value));
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

	auto IRBuilder::getInfinityF128() const -> Constant {
		return Constant(llvm::ConstantFP::getInfinity(this->getTypeF128().native()));
	}



	

	//////////////////////////////////////////////////////////////////////
	// types

	auto IRBuilder::getFuncProto(
		const Type& return_type, evo::ArrayProxy<Type> params, bool is_var_args
	) -> FunctionType {
		return FunctionType(
			llvm::FunctionType::get(
				return_type.native(),
				llvm::ArrayRef((llvm::Type**)params.data(), params.size()),
				is_var_args
			)
		);
	};

	auto IRBuilder::getTypeBool() const -> IntegerType { return IntegerType(this->builder->getInt1Ty()); }

	auto IRBuilder::getTypeI8() const -> IntegerType { return IntegerType(this->builder->getInt8Ty()); }
	auto IRBuilder::getTypeI16() const -> IntegerType { return IntegerType(this->builder->getInt16Ty()); }
	auto IRBuilder::getTypeI32() const -> IntegerType { return IntegerType(this->builder->getInt32Ty()); }
	auto IRBuilder::getTypeI64() const -> IntegerType { return IntegerType(this->builder->getInt64Ty()); }
	auto IRBuilder::getTypeI128() const -> IntegerType { return IntegerType(this->builder->getInt128Ty()); }

	auto IRBuilder::getTypeI_N(evo::uint width) const -> IntegerType {
		return IntegerType(this->builder->getIntNTy(width));
	}

	auto IRBuilder::getTypeF16() const -> Type { return Type(this->builder->getHalfTy()); }
	auto IRBuilder::getTypeBF16() const -> Type { return Type(this->builder->getBFloatTy()); }
	auto IRBuilder::getTypeF32() const -> Type { return Type(this->builder->getFloatTy()); }
	auto IRBuilder::getTypeF64() const -> Type { return Type(this->builder->getDoubleTy()); }
	auto IRBuilder::getTypeF128() const -> Type { return Type(llvm::Type::getFP128Ty(this->builder->getContext())); }

	auto IRBuilder::getTypePtr() const -> PointerType { return PointerType(this->builder->getPtrTy()); }

	auto IRBuilder::getTypeVoid() const -> Type { return Type(this->builder->getVoidTy()); };



	//////////////////////////////////////////////////////////////////////
	// getters


	auto IRBuilder::get_native_context() -> llvm::LLVMContext& {
		return this->builder->getContext();
	}
		
}