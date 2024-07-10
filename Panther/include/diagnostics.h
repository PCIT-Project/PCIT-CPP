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
		TokUnrecognizedCharacter,           // T1
		TokUnterminatedMultilineComment,    // T2
		TokUnterminatedTextLiteral,         // T3
		TokUnterminatedTextEscapeSequence,  // T4
		TokLiteralLeadingZero,              // T5
		TokLiteralNumMultipleDecimalPoints, // T6
		TokInvalidFPBase,                   // T7
		TokInvalidNumDigit,                 // T8
		TokLiteralNumTooBig,                // T9
		TokInvalidIntegerWidth,				// T10
		TokUnknownFailureToTokenizeNum,     // T11
		TokFileTooLarge,                    // T12

		ParserUnknownStmtStart,          // P1
		ParserIncorrectStmtContinuation, // P2
		ParserUnexpectedEOF,             // P3
		ParserInvalidKindForAThisParam,  // P4
		ParserDereferenceOrUnwrapOnType, // P5
		ParserAssumedTokenNotPreset,     // P6

		MiscFileDoesNotExist, // M1
		MiscLoadFileFailed,   // M2
	};

	using Diagnostic = core::DiagnosticImpl<DiagnosticCode, Source::Location>;


	EVO_NODISCARD inline auto printDiagnosticCode(DiagnosticCode code) noexcept -> std::string_view {
		switch(code){
			break; case DiagnosticCode::TokUnrecognizedCharacter:           return "T1";
			break; case DiagnosticCode::TokUnterminatedMultilineComment:    return "T2";
			break; case DiagnosticCode::TokUnterminatedTextLiteral:         return "T3";
			break; case DiagnosticCode::TokUnterminatedTextEscapeSequence:  return "T4";
			break; case DiagnosticCode::TokLiteralLeadingZero:              return "T5";
			break; case DiagnosticCode::TokLiteralNumMultipleDecimalPoints: return "T6";
			break; case DiagnosticCode::TokInvalidFPBase:                   return "T7";
			break; case DiagnosticCode::TokInvalidNumDigit:                 return "T8";
			break; case DiagnosticCode::TokLiteralNumTooBig:                return "T9";
			break; case DiagnosticCode::TokInvalidIntegerWidth:             return "T10";
			break; case DiagnosticCode::TokUnknownFailureToTokenizeNum:     return "T11";
			break; case DiagnosticCode::TokFileTooLarge:                    return "T12";

			break; case DiagnosticCode::ParserUnknownStmtStart:          return "P1";
			break; case DiagnosticCode::ParserIncorrectStmtContinuation: return "P2";
			break; case DiagnosticCode::ParserUnexpectedEOF:             return "P3";
			break; case DiagnosticCode::ParserInvalidKindForAThisParam:  return "P4";
			break; case DiagnosticCode::ParserDereferenceOrUnwrapOnType: return "P5";
			break; case DiagnosticCode::ParserAssumedTokenNotPreset:     return "P6";

			break; case DiagnosticCode::MiscFileDoesNotExist: return "M1";
			break; case DiagnosticCode::MiscLoadFileFailed:   return "M2";
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