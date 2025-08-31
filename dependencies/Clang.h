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
    #pragma warning(disable : 4702) // needed for some reason...
    #pragma warning(disable : 4217) // needed for some reason...
#endif


#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Mangle.h>

#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Basic/TargetInfo.h>

#include <clang/CodeGen/CodeGenAction.h>

#include <clang/ExtractAPI/FrontendActions.h>

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Frontend/FrontendActions.h>

#include <clang/Lex/PreprocessorOptions.h>

#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/Module.h>


#if defined(EVO_COMPILER_MSVC)
    #pragma warning(default : 4217)
    #pragma warning(default : 4702)
    #pragma warning(pop)
#endif