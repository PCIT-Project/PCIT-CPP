////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>

namespace pcit::core::windows{

	// no-op when not on windows
	auto setConsoleToUTF8Mode() -> void;

	#if defined(EVO_PLATFORM_WINDOWS)
		EVO_NODISCARD auto isDebuggerPresent() -> bool;
	#endif


}