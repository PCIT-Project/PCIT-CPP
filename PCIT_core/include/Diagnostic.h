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

	enum class DiagnosticLevel{
		Fatal,
		Error,
		Warning,
		Info,
	};


	template<class CodeEnum, class Location>
	struct DiagnosticImpl{
		static_assert(std::is_enum_v<CodeEnum>, "Diagnostic CodeEnum must be an enum");
		using Level = DiagnosticLevel;
		using Code = CodeEnum;

		struct Info{
			std::string message;
			std::optional<Location> location;
		};

		DiagnosticLevel level;
		CodeEnum code;
		std::optional<Location> location;
		std::string message;
		std::vector<Info> infos;
	};


	EVO_NODISCARD inline auto printDiagnosticLevel(DiagnosticLevel level) noexcept -> std::string_view {
		switch(level){
			break; case DiagnosticLevel::Fatal:   return "Fatal";
			break; case DiagnosticLevel::Error:   return "Error";
			break; case DiagnosticLevel::Warning: return "Warning";
		};

		evo::debugFatalBreak("Unknown or unsupported pcit::core::DiagnosticLevel");
	};

};


template<>
struct std::formatter<pcit::core::DiagnosticLevel> : std::formatter<std::string_view> {
    auto format(const pcit::core::DiagnosticLevel& level, std::format_context& ctx) {
        return std::formatter<std::string_view>::format(printDiagnosticLevel(level), ctx);
    };
};