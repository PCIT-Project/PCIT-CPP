//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once


namespace llvm{

	class LLVMContext;
	class Module;
	class TargetMachine;

	class IRBuilderBase;
	class NoFolder;
	class IRBuilderDefaultInserter;

	class ExecutionEngine;
	
	class Function;
	class Argument;

	// types
	class Type;
	class FunctionType;
	class IntegerType;
	class PointerType;

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