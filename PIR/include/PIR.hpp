////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include "./misc.hpp"

#include "./enums.hpp"
#include "./Type.hpp"
#include "./Expr.hpp"
#include "./BasicBlock.hpp"
#include "./GlobalVar.hpp"
#include "./Function.hpp"
#include "./Module.hpp"

#include "./InstrReader.hpp"
#include "./InstrHandler.hpp"

#include "./ModulePrinter.hpp"

#include "./PassManager.hpp"
#include "./passes/instCombine.hpp"
#include "./passes/removeUnusedStmts.hpp"

#include "./llvmir.hpp"
#include "./JITEngine.hpp"
#include "./ExecutionEngine.hpp"