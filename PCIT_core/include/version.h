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

	struct Version{
		uint16_t major;
		uint16_t release;
		uint16_t minor;
		uint16_t patch;
	};

	constexpr auto version = Version{
		.major   = 0,
		.release = 0,
		.minor   = 232,
		.patch   = 0,
	};

}
 	

template<>
struct std::formatter<pcit::core::Version> : std::formatter<std::string> {
    auto format(const pcit::core::Version& version, std::format_context& ctx) const -> std::format_context::iterator {
        return std::formatter<std::string>::format(
        	std::format("{}.{}.{}.{}", version.major, version.release, version.minor, version.patch),
        	ctx
        );
    }
};