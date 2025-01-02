////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/windows.h"


#if defined(EVO_PLATFORM_WINDOWS)
	#if !defined(WIN32_LEAN_AND_MEAN)
		#define WIN32_LEAN_AND_MEAN
	#endif

	#if !defined(NOCOMM)
		#define NOCOMM
	#endif

	#if !defined(NOMINMAX)
		#define NOMINMAX
	#endif

	#include <Windows.h>
#endif


namespace pcit::core::windows{


	auto setConsoleToUTF8Mode() -> void {
		#if defined(EVO_PLATFORM_WINDOWS)
			::SetConsoleOutputCP(CP_UTF8);
		#endif
	}


	#if defined(EVO_PLATFORM_WINDOWS)

		auto isDebuggerPresent() -> bool {
			return ::IsDebuggerPresent();
		}
		
	#endif

	
}
