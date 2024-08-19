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

	// TODO: reorganize/reorder codes
	enum class DiagnosticCode{
		TokUnrecognizedCharacter,             // T1
		TokUnterminatedMultilineComment,      // T2
		TokUnterminatedTextLiteral,           // T3
		TokUnterminatedTextEscapeSequence,    // T4
		TokLiteralLeadingZero,                // T5
		TokLiteralNumMultipleDecimalPoints,   // T6
		TokInvalidFPBase,                     // T7
		TokInvalidNumDigit,                   // T8
		TokLiteralNumTooBig,                  // T9
		TokInvalidIntegerWidth,               // T10
		TokUnknownFailureToTokenizeNum,       // T11
		TokInvalidChar,                       // T12
		TokFileTooLarge,                      // T13
		TokFileLocationLimitOOB,              // T14
		TokDoubleUnderscoreNotAllowed,        // T15

		ParserUnknownStmtStart,               // P1
		ParserIncorrectStmtContinuation,      // P2
		ParserUnexpectedEOF,                  // P3
		ParserInvalidKindForAThisParam,       // P4
		ParserDereferenceOrUnwrapOnType,      // P5
		ParserAssumedTokenNotPreset,          // P6
		ParserEmptyMultiAssign,               // P7
		ParserEmptyFuncReturnBlock,           // P8

		SemaEncounteredASTKindNone,           // S1
		SemaInvalidGlobalStmtKind,            // S2
		SemaInvalidStmtKind,                  // S3
		SemaInvalidExprKind,                  // S4
		SemaAlreadyDefined,                   // S5
		SemaImproperUseOfTypeVoid,            // S6
		SemaVoidWithQualifiers,               // S7
		SemaGenericTypeWithQualifiers,        // S8
		SemaGenericTypeNotInTemplatePackDecl, // S9
		SemaVarWithNoValue,                   // S10
		SemaCannotInferType,                  // S11
		SemaIncorrectExprValueType,           // S12
		SemaTypeMismatch,                     // S13
		SemaIdentNotInScope,                  // S14
		SemaCannotCallLikeFunction,           // S15
		SemaEmptyTemplatePackDeclaration,     // S16
		SemaExpectedTemplateArgs,             // S17
		SemaUnexpectedTemplateArgs,           // S18
		SemaIncorrectTemplateArgValueType,    // S19
		SemaIncorrectTemplateInstantiation,   // S20
		SemaExpectedConstEvalValue,           // S21
		SemaInvalidBaseType,                  // S22
		SemaUnknownAttribute,                 // S23
		SemaFailedToImportFile,               // S24
		SemaIncorrectImportDecl,              // S25
		SemaUnsupportedOperator,              // S26
		SemaImportMemberDoesntExist,          // S27
		SemaImportMemberIsntPub,              // S28

		LLLVMDataLayoutError,                 // LLVM1

		MiscFileDoesNotExist,                 // M1
		MiscLoadFileFailed,                   // M2
		MiscUnimplementedFeature,             // M3
		MiscUnknownFatal,                     // M4
	};

	using Diagnostic = core::DiagnosticImpl<DiagnosticCode, Source::Location>;




	EVO_NODISCARD inline auto printDiagnosticCode(DiagnosticCode code) -> std::string_view {
		#if defined(EVO_COMPILER_MSVC)
			#pragma warning(default : 4062)
		#endif

		switch(code){
			break; case DiagnosticCode::TokUnrecognizedCharacter:             return "T1";
			break; case DiagnosticCode::TokUnterminatedMultilineComment:      return "T2";
			break; case DiagnosticCode::TokUnterminatedTextLiteral:           return "T3";
			break; case DiagnosticCode::TokUnterminatedTextEscapeSequence:    return "T4";
			break; case DiagnosticCode::TokLiteralLeadingZero:                return "T5";
			break; case DiagnosticCode::TokLiteralNumMultipleDecimalPoints:   return "T6";
			break; case DiagnosticCode::TokInvalidFPBase:                     return "T7";
			break; case DiagnosticCode::TokInvalidNumDigit:                   return "T8";
			break; case DiagnosticCode::TokLiteralNumTooBig:                  return "T9";
			break; case DiagnosticCode::TokInvalidIntegerWidth:               return "T10";
			break; case DiagnosticCode::TokUnknownFailureToTokenizeNum:       return "T11";
			break; case DiagnosticCode::TokInvalidChar:                       return "T12";
			break; case DiagnosticCode::TokFileTooLarge:                      return "T13";
			break; case DiagnosticCode::TokFileLocationLimitOOB:              return "T14";
			break; case DiagnosticCode::TokDoubleUnderscoreNotAllowed:        return "T15";

			break; case DiagnosticCode::ParserUnknownStmtStart:               return "P1";
			break; case DiagnosticCode::ParserIncorrectStmtContinuation:      return "P2";
			break; case DiagnosticCode::ParserUnexpectedEOF:                  return "P3";
			break; case DiagnosticCode::ParserInvalidKindForAThisParam:       return "P4";
			break; case DiagnosticCode::ParserDereferenceOrUnwrapOnType:      return "P5";
			break; case DiagnosticCode::ParserAssumedTokenNotPreset:          return "P6";
			break; case DiagnosticCode::ParserEmptyMultiAssign:               return "P7";
			break; case DiagnosticCode::ParserEmptyFuncReturnBlock:           return "P8";

			break; case DiagnosticCode::SemaEncounteredASTKindNone:           return "S1";
			break; case DiagnosticCode::SemaInvalidGlobalStmtKind:            return "S2";
			break; case DiagnosticCode::SemaInvalidStmtKind:                  return "S3";
			break; case DiagnosticCode::SemaInvalidExprKind:                  return "S4";
			break; case DiagnosticCode::SemaAlreadyDefined:                   return "S5";
			break; case DiagnosticCode::SemaImproperUseOfTypeVoid:            return "S6";
			break; case DiagnosticCode::SemaVoidWithQualifiers:               return "S7";
			break; case DiagnosticCode::SemaGenericTypeWithQualifiers:        return "S8";
			break; case DiagnosticCode::SemaGenericTypeNotInTemplatePackDecl: return "S9";
			break; case DiagnosticCode::SemaVarWithNoValue:                   return "S10";
			break; case DiagnosticCode::SemaCannotInferType:                  return "S11";
			break; case DiagnosticCode::SemaIncorrectExprValueType:           return "S12";
			break; case DiagnosticCode::SemaTypeMismatch:                     return "S13";
			break; case DiagnosticCode::SemaIdentNotInScope:                  return "S14";
			break; case DiagnosticCode::SemaCannotCallLikeFunction:           return "S15";
			break; case DiagnosticCode::SemaEmptyTemplatePackDeclaration:     return "S16";
			break; case DiagnosticCode::SemaExpectedTemplateArgs:             return "S17";
			break; case DiagnosticCode::SemaUnexpectedTemplateArgs:           return "S18";
			break; case DiagnosticCode::SemaIncorrectTemplateArgValueType:    return "S19";
			break; case DiagnosticCode::SemaIncorrectTemplateInstantiation:   return "S20";
			break; case DiagnosticCode::SemaExpectedConstEvalValue:           return "S21";
			break; case DiagnosticCode::SemaInvalidBaseType:                  return "S22";
			break; case DiagnosticCode::SemaUnknownAttribute:                 return "S23";
			break; case DiagnosticCode::SemaFailedToImportFile:               return "S24";
			break; case DiagnosticCode::SemaIncorrectImportDecl:              return "S25";
			break; case DiagnosticCode::SemaUnsupportedOperator:              return "S26";
			break; case DiagnosticCode::SemaImportMemberDoesntExist:          return "S27";
			break; case DiagnosticCode::SemaImportMemberIsntPub:              return "S28";

			break; case DiagnosticCode::LLLVMDataLayoutError:                 return "LLVM1";

			break; case DiagnosticCode::MiscFileDoesNotExist:                 return "M1";
			break; case DiagnosticCode::MiscLoadFileFailed:                   return "M2";
			break; case DiagnosticCode::MiscUnimplementedFeature:             return "M3";
			break; case DiagnosticCode::MiscUnknownFatal:                     return "M4";
		}
		
		evo::debugFatalBreak("Unknown or unsupported pcit::panther::DiagnosticCode");

		#if defined(EVO_COMPILER_MSVC)
			#pragma warning(disable : 4062)
		#endif
	}

}



template<>
struct std::formatter<pcit::panther::DiagnosticCode> : std::formatter<std::string_view> {
    auto format(const pcit::panther::DiagnosticCode& code, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(pcit::panther::printDiagnosticCode(code), ctx);
    }
};
