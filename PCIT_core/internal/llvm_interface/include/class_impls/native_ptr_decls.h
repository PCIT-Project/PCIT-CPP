//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
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

	class Function;

	// types
	class Type;
	class FunctionType;

	// stmts
	class BasicBlock;
	
}