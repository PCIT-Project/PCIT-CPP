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


#include <lld/Common/LLVM.h>
#include <lld/Common/Driver.h>
#include <lld/Common/CommonLinkerContext.h>
#include <llvm/Support/CrashRecoveryContext.h>


#undef _SILENCE_CXX20_CISO646_REMOVED_WARNING

#if defined(EVO_COMPILER_MSVC)
    #pragma warning(default : 4996)
    #pragma warning(default : 4244)
    #pragma warning(pop)
#endif