////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/Function.h"

#include <LLVM.h>


namespace pcit::llvmint{

	//////////////////////////////////////////////////////////////////////
	// Argument

	auto Argument::setName(std::string_view name) -> void {
		this->native()->setName(name);
	}


	auto Argument::setZeroExt() -> void {
		this->native()->addAttr(llvm::Attribute::AttrKind::ZExt);
	}

	auto Argument::setSignExt() -> void {
		this->native()->addAttr(llvm::Attribute::AttrKind::SExt);
	}

	auto Argument::setNoAlias() -> void {
		this->native()->addAttr(llvm::Attribute::AttrKind::NoAlias);
	}

	auto Argument::setNonNull() -> void {
		this->native()->addAttr(llvm::Attribute::AttrKind::NonNull);
	}

	auto Argument::setDereferencable(uint64_t size) -> void {
		this->native()->addAttr(llvm::Attribute::getWithDereferenceableBytes(this->native()->getContext(), size));
	}

	auto Argument::setReadOnly() -> void {
		this->native()->addAttr(llvm::Attribute::AttrKind::ReadOnly);
	}

	auto Argument::setWriteOnly() -> void {
		this->native()->addAttr(llvm::Attribute::AttrKind::WriteOnly);
	}

	auto Argument::setWritable() -> void {
		this->native()->addAttr(llvm::Attribute::AttrKind::Writable);
	}

	auto Argument::setStructRet(const Type& type) -> void {
		this->native()->addAttr(llvm::Attribute::getWithStructRetType(this->native()->getContext(), type.native()));
	}





	Argument::operator Value() const { return Value(static_cast<llvm::Value*>(this->native())); }



	//////////////////////////////////////////////////////////////////////
	// Function

	auto Function::front() const -> const BasicBlock {
		return BasicBlock(&this->native()->front());
	}

	auto Function::front() -> BasicBlock {
		return BasicBlock(&this->native()->front());
	}

	auto Function::back() const -> const BasicBlock {
		return BasicBlock(&this->native()->back());
	}

	auto Function::back() -> BasicBlock {
		return BasicBlock(&this->native()->back());
	}


	auto Function::getArg(unsigned index) const -> Argument {
		return Argument(this->native()->getArg(index));
	}



	auto Function::setNoThrow() -> void {
		this->native()->setDoesNotThrow();
	}

	auto Function::setCallingConv(CallingConv calling_conv) -> void {
		this->native()->setCallingConv(evo::to_underlying(calling_conv));
	}



	Function::operator Value() const { return Value(static_cast<llvm::Value*>(this->native())); }
	Function::operator Constant() const { return Constant(static_cast<llvm::Constant*>(this->native())); }



	
}