////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


namespace llvm{

	class LLVMContext;
	class Module;
	class TargetMachine;

	class IRBuilderBase;
	class NoFolder;
	class IRBuilderDefaultInserter;

	
	class Function;
	class Argument;

	// types
	class Type;
	class FunctionType;
	class IntegerType;
	class PointerType;
	class ArrayType;
	class StructType;

	// stmts
	class BasicBlock;
	class AllocaInst;

	// value
	class Value;
	class Constant;
	class ConstantInt;
	class CallInst;
	class LoadInst;
	class GlobalVariable;

}