//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


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
		.minor   = 40,
		.patch   = 1,
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