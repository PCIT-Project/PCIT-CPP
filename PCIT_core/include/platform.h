////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>

namespace pcit::core{

	enum class OS{
		Unknown,

		Windows,
		Linux,
	};

	EVO_NODISCARD constexpr auto getCurrentOS() -> OS {
		#if defined(EVO_PLATFORM_WINDOWS)
			return OS::Windows;
		#elif defined(EVO_PLATFORM_LINUX)
			return OS::Linux;
		#else
			return OS::Unknown;
		#endif
	}



	enum class Architecture{
		Unknown,

		X86_64,
	};

	EVO_NODISCARD constexpr auto getCurrentArchitecture() -> Architecture {
		#if defined(EVO_ARCH_X86_64)
			return Architecture::X86_64;
		#else
			return Architecture::Unknown;
		#endif
	}

}


template<>
struct std::formatter<pcit::core::OS> : std::formatter<std::string_view> {
    auto format(const pcit::core::OS& os, std::format_context& ctx) const -> std::format_context::iterator {
        switch(os){
        	case pcit::core::OS::Windows: return std::formatter<std::string_view>::format("Windows", ctx);
        	case pcit::core::OS::Linux:   return std::formatter<std::string_view>::format("Linux", ctx);
        	default: evo::debugFatalBreak("Unknown or unsupported OS");
        }
    }
};


template<>
struct std::formatter<pcit::core::Architecture> : std::formatter<std::string_view> {
    auto format(const pcit::core::Architecture& arch, std::format_context& ctx) const
    -> std::format_context::iterator {
        switch(arch){
        	case pcit::core::Architecture::X86_64: return std::formatter<std::string_view>::format("x86_64", ctx);
        	default: evo::debugFatalBreak("Unknown or unsupported architecture");
        }
    }
};