////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include "./enums.h"
#include "./Type.h"
#include "./Expr.h"
#include "./BasicBlock.h"
#include "./GlobalVar.h"
#include "./Function.h"
#include "./Module.h"

#include "./ReaderAgent.h"
#include "./Agent.h"

#include "./ModulePrinter.h"

#include "./PassManager.h"
#include "./passes/instCombine.h"
