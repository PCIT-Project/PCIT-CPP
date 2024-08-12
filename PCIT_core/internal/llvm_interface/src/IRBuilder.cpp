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

	

	//////////////////////////////////////////////////////////////////////
	// types

	auto IRBuilder::getFuncProto(
		const llvmint::Type& return_type, evo::ArrayProxy<llvmint::Type> params, bool is_var_args
	) -> llvmint::FunctionType {
		return llvmint::FunctionType(
			llvm::FunctionType::get(
				return_type.native(),
				llvm::ArrayRef((llvm::Type**)params.data(), params.size()),
				is_var_args
			)
		);
	};


	auto IRBuilder::getTypeVoid() const -> llvmint::Type { return llvmint::Type(this->builder->getVoidTy()); };



	//////////////////////////////////////////////////////////////////////
	// getters


	auto IRBuilder::get_native_context() -> llvm::LLVMContext& {
		return this->builder->getContext();
	}
		
}