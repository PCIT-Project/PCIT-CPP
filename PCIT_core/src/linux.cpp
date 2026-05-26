////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/windows.hpp"


#if defined(EVO_PLATFORM_LINUX)

	#include <unistd.h>

#endif


namespace pcit::core::linux{


	#if defined(EVO_PLATFORM_LINUX)

		auto getExecutablePath() -> std::filesystem::path {
			auto path_buffer = std::array<char, 512>();
			ssize_t count = readlink("/proc/self/exe", path_buffer.data(), path_buffer.size());
			return std::filesystem::path(std::string(path_buffer.data(), (count > 0) ? count : 0));
		}
		
	#endif

	
}
