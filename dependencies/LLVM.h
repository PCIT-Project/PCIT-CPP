//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////

#pragma once

#if defined(EVO_COMPILER_MSVC)
    #pragma warning(push, 0)
    #pragma warning(disable : 4244) // needed for some reason...
    #pragma warning(disable : 4996) // needed for some reason...
#endif

#define _SILENCE_CXX20_CISO646_REMOVED_WARNING

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>

#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/Core.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
// #include <llvm/ExecutionEngine/Orc/MapperJITLinkMemoryManager.h>
#include <llvm/ExecutionEngine/Orc/Mangling.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/AbsoluteSymbols.h>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/NoFolder.h>

#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Analysis/CGSCCPassManager.h>

#include <llvm/Passes/PassBuilder.h>

#include <llvm/Transforms/Utils/Cloning.h>

#include <llvm/Support/TargetSelect.h>


#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/PassManager.h>

#include <llvm/MC/TargetRegistry.h>

#include <llvm/Support/FileSystem.h>

#include <llvm/TargetParser/Host.h>



#undef _SILENCE_CXX20_CISO646_REMOVED_WARNING

#if defined(EVO_COMPILER_MSVC)
    #pragma warning(default : 4996)
    #pragma warning(default : 4244)
    #pragma warning(pop)
#endif