//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>

namespace pcit::core{

	enum class Platform{
		Windows,
		Linux,
	};

	enum class Architecture{
		x86,
	};

}


template<>
struct std::formatter<pcit::core::Platform> : std::formatter<std::string_view> {
    auto format(const pcit::core::Platform& platform, std::format_context& ctx) const -> std::format_context::iterator {
        switch(platform){
        	case pcit::core::Platform::Windows: return std::formatter<std::string_view>::format("Windows", ctx);
        	case pcit::core::Platform::Linux:   return std::formatter<std::string_view>::format("Linux", ctx);
        	default: evo::debugFatalBreak("Unknown or unsupported platform");
        }
    }
};


template<>
struct std::formatter<pcit::core::Architecture> : std::formatter<std::string_view> {
    auto format(const pcit::core::Architecture& architecture, std::format_context& ctx) const
    -> std::format_context::iterator {
        switch(architecture){
        	case pcit::core::Architecture::x86: return std::formatter<std::string_view>::format("x86", ctx);
        	default: evo::debugFatalBreak("Unknown or unsupported architecture");
        }
    }
};