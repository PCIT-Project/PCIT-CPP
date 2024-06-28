//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>
#include <PCIT_core.h>

#include "./Source.h"

namespace pcit::panther{


	enum class DiagnosticCode{
		SemaUnknownIdentifier, // S1

		MiscFileDoesNotExist, // M1
		MiscLoadFileFailed, // M2
	};

	using Diagnostic = core::DiagnosticImpl<DiagnosticCode, std::optional<Source::Location>>;


	EVO_NODISCARD inline auto printDiagnosticCode(DiagnosticCode code) noexcept -> std::string_view {
		switch(code){
			break; case DiagnosticCode::SemaUnknownIdentifier: return "S1";

			break; case DiagnosticCode::MiscFileDoesNotExist: return "M1";
			break; case DiagnosticCode::MiscLoadFileFailed: return "M2";
		};
		
		evo::debugFatalBreak("Unknown or unsupported pcit::panther::DiagnosticCode");
	};

};



template<>
struct std::formatter<pcit::panther::DiagnosticCode> : std::formatter<std::string_view> {
    auto format(const pcit::panther::DiagnosticCode& code, std::format_context& ctx) {
        return std::formatter<std::string_view>::format(printDiagnosticCode(code), ctx);
    };
};
