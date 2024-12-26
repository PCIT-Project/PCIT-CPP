////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


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
		ParserInvalidNewExpr,                 // P9

		SemaEncounteredASTKindNone,
		SemaInvalidGlobalStmtKind,
		SemaInvalidStmtKind,
		SemaInvalidExprKind,
		SemaAlreadyDefined,
		SemaOverloadAlreadyDefined,
		SemaImproperUseOfTypeVoid,
		SemaVoidWithQualifiers,
		SemaGenericTypeWithQualifiers,
		SemaGenericTypeNotInTemplatePackDecl,
		SemaVarWithNoValue,
		SemaCannotInferType,
		SemaIncorrectExprValueType,
		SemaTypeMismatch,
		SemaIdentNotInScope,
		SemaTypeNotInScope,
		SemaIntrinsicDoesntExist,
		SemaCannotCallLikeFunction,
		SemaEmptyTemplatePackDeclaration,
		SemaExpectedTemplateArgs,
		SemaUnexpectedTemplateArgs,
		SemaIncorrectTemplateArgValueType,
		SemaIncorrectTemplateInstantiation,
		SemaInvalidIntrinsicTemplateArg,
		SemaExpectedConstEvalValue,
		SemaInvalidBaseType,
		SemaUnknownAttribute,
		SemaFailedToImportFile,
		SemaIncorrectImportDecl,
		SemaUnsupportedOperator,
		SemaImportMemberDoesntExist,
		SemaImportMemberIsntPub,
		SemaConstEvalVarNotDef,
		SemaAssignmentDstNotConcreteMutable,
		SemaAssignmentDstGlobalInRuntimeFunc,
		SemaAssignmentValueNotEphemeral,
		SemaCopyExprNotConcrete,
		SemaMoveInvalidValueType,
		SemaReturnNotEphemeral,
		SemaStmtAfterScopeTerminated,
		SemaIncorrectReturnStmtKind,
		SemaInvalidEntrySignature,
		SemaMultipleEntriesDeclared,
		SemaDiscardingFuncReturn,
		SemaFuncDoesntReturnValue,
		SemaInvalidDiscardStmtRHS,
		SemaInvalidAddrOfRHS,
		SemaInvalidDerefRHS,
		SemaInvalidInfixLHS,
		SemaInvalidInfixRHS,
		SemaInvalidNegateRHS,
		SemaInvalidInfixArgTypes,
		SemaInvalidTypeQualifiers,
		SemaParamTypeVoid,
		SemaNoMatchingFunction,
		SemaMultipleMatchingFunctions,
		SemaParamsCannotBeConstEval,
		SemaArgIncorrectLabel,
		SemaArgMustBeLabeled,
		SemaInvalidUseOfInitializerValueExpr,
		SemaIncorrectNumberOfAssignTargets,
		SemaMultipleValuesIntoOne,
		SemaFuncIsntTerminated,
		SemaAttributeAlreadySet,
		SemaInvalidAttributeArgument,
		SemaCantCallRuntimeFuncInComptimeContext,
		SemaComptimeCircularDependency,
		SemaTypeCannotBeUsedAsExpr,
		SemaExprCannotBeUsedAsType,
		SemaNotValidExprForTypeIDConversion,
		SemaErrorInRunningOfFuncAtComptime,
		SemaErrorInRunningOfIntrinsicAtComptime,
		SemaCannotConvertFluidValue,
		SemaCannotTypedefVoid,
		SemaCannotNewVoid,
		SemaIncorrectNumArgsForPrimitiveOpNew,
		SemaArgHasLabelInrimitiveOpNew,

		SemaWarnSingleValInMultiAssign,
		SemaWarnEntryIsImplicitRuntime,

		LLLVMDataLayoutError,                 // LLVM1

		MiscFileDoesNotExist,                 // M1
		MiscLoadFileFailed,                   // M2
		MiscUnimplementedFeature,             // M3
		MiscUnknownFatal,                     // M4
		MiscNoEntrySet,                       // M5
		MiscInvalidKind,                      // M6
		MiscRuntimePanic,                     // M7
	};

	using Diagnostic = core::DiagnosticImpl<DiagnosticCode, Source::Location>;




	EVO_NODISCARD inline auto printDiagnosticCode(DiagnosticCode code) -> std::string_view {
		#if defined(EVO_COMPILER_MSVC)
			#pragma warning(default : 4062)
		#endif

		// TODO: give Sema and SemaWarn codes unique numbers
		switch(code){
			break; case DiagnosticCode::TokUnrecognizedCharacter:                 return "T1";
			break; case DiagnosticCode::TokUnterminatedMultilineComment:          return "T2";
			break; case DiagnosticCode::TokUnterminatedTextLiteral:               return "T3";
			break; case DiagnosticCode::TokUnterminatedTextEscapeSequence:        return "T4";
			break; case DiagnosticCode::TokLiteralLeadingZero:                    return "T5";
			break; case DiagnosticCode::TokLiteralNumMultipleDecimalPoints:       return "T6";
			break; case DiagnosticCode::TokInvalidFPBase:                         return "T7";
			break; case DiagnosticCode::TokInvalidNumDigit:                       return "T8";
			break; case DiagnosticCode::TokLiteralNumTooBig:                      return "T9";
			break; case DiagnosticCode::TokInvalidIntegerWidth:                   return "T10";
			break; case DiagnosticCode::TokUnknownFailureToTokenizeNum:           return "T11";
			break; case DiagnosticCode::TokInvalidChar:                           return "T12";
			break; case DiagnosticCode::TokFileTooLarge:                          return "T13";
			break; case DiagnosticCode::TokFileLocationLimitOOB:                  return "T14";
			break; case DiagnosticCode::TokDoubleUnderscoreNotAllowed:            return "T15";

			break; case DiagnosticCode::ParserUnknownStmtStart:                   return "P1";
			break; case DiagnosticCode::ParserIncorrectStmtContinuation:          return "P2";
			break; case DiagnosticCode::ParserUnexpectedEOF:                      return "P3";
			break; case DiagnosticCode::ParserInvalidKindForAThisParam:           return "P4";
			break; case DiagnosticCode::ParserDereferenceOrUnwrapOnType:          return "P5";
			break; case DiagnosticCode::ParserAssumedTokenNotPreset:              return "P6";
			break; case DiagnosticCode::ParserEmptyMultiAssign:                   return "P7";
			break; case DiagnosticCode::ParserEmptyFuncReturnBlock:               return "P8";
			break; case DiagnosticCode::ParserInvalidNewExpr:                     return "P9";

			break; case DiagnosticCode::SemaEncounteredASTKindNone:               return "S";
			break; case DiagnosticCode::SemaInvalidGlobalStmtKind:                return "S";
			break; case DiagnosticCode::SemaInvalidStmtKind:                      return "S";
			break; case DiagnosticCode::SemaInvalidExprKind:                      return "S";
			break; case DiagnosticCode::SemaAlreadyDefined:                       return "S";
			break; case DiagnosticCode::SemaOverloadAlreadyDefined:               return "S";
			break; case DiagnosticCode::SemaImproperUseOfTypeVoid:                return "S";
			break; case DiagnosticCode::SemaVoidWithQualifiers:                   return "S";
			break; case DiagnosticCode::SemaGenericTypeWithQualifiers:            return "S";
			break; case DiagnosticCode::SemaGenericTypeNotInTemplatePackDecl:     return "S";
			break; case DiagnosticCode::SemaVarWithNoValue:                       return "S";
			break; case DiagnosticCode::SemaCannotInferType:                      return "S";
			break; case DiagnosticCode::SemaIncorrectExprValueType:               return "S";
			break; case DiagnosticCode::SemaTypeMismatch:                         return "S";
			break; case DiagnosticCode::SemaIdentNotInScope:                      return "S";
			break; case DiagnosticCode::SemaTypeNotInScope:                       return "S";
			break; case DiagnosticCode::SemaIntrinsicDoesntExist:                 return "S";
			break; case DiagnosticCode::SemaCannotCallLikeFunction:               return "S";
			break; case DiagnosticCode::SemaEmptyTemplatePackDeclaration:         return "S";
			break; case DiagnosticCode::SemaExpectedTemplateArgs:                 return "S";
			break; case DiagnosticCode::SemaUnexpectedTemplateArgs:               return "S";
			break; case DiagnosticCode::SemaIncorrectTemplateArgValueType:        return "S";
			break; case DiagnosticCode::SemaIncorrectTemplateInstantiation:       return "S";
			break; case DiagnosticCode::SemaInvalidIntrinsicTemplateArg:          return "S";
			break; case DiagnosticCode::SemaExpectedConstEvalValue:               return "S";
			break; case DiagnosticCode::SemaInvalidBaseType:                      return "S";
			break; case DiagnosticCode::SemaUnknownAttribute:                     return "S";
			break; case DiagnosticCode::SemaFailedToImportFile:                   return "S";
			break; case DiagnosticCode::SemaIncorrectImportDecl:                  return "S";
			break; case DiagnosticCode::SemaUnsupportedOperator:                  return "S";
			break; case DiagnosticCode::SemaImportMemberDoesntExist:              return "S";
			break; case DiagnosticCode::SemaImportMemberIsntPub:                  return "S";
			break; case DiagnosticCode::SemaConstEvalVarNotDef:                   return "S";
			break; case DiagnosticCode::SemaAssignmentDstNotConcreteMutable:      return "S";
			break; case DiagnosticCode::SemaAssignmentDstGlobalInRuntimeFunc:     return "S";
			break; case DiagnosticCode::SemaAssignmentValueNotEphemeral:          return "S";
			break; case DiagnosticCode::SemaCopyExprNotConcrete:                  return "S";
			break; case DiagnosticCode::SemaMoveInvalidValueType:                 return "S";
			break; case DiagnosticCode::SemaReturnNotEphemeral:                   return "S";
			break; case DiagnosticCode::SemaStmtAfterScopeTerminated:             return "S";
			break; case DiagnosticCode::SemaIncorrectReturnStmtKind:              return "S";
			break; case DiagnosticCode::SemaInvalidEntrySignature:                return "S";
			break; case DiagnosticCode::SemaMultipleEntriesDeclared:              return "S";
			break; case DiagnosticCode::SemaDiscardingFuncReturn:                 return "S";
			break; case DiagnosticCode::SemaFuncDoesntReturnValue:                return "S";
			break; case DiagnosticCode::SemaInvalidDiscardStmtRHS:                return "S";
			break; case DiagnosticCode::SemaInvalidAddrOfRHS:                     return "S";
			break; case DiagnosticCode::SemaInvalidDerefRHS:                      return "S";
			break; case DiagnosticCode::SemaInvalidInfixLHS:                      return "S";
			break; case DiagnosticCode::SemaInvalidInfixRHS:                      return "S";
			break; case DiagnosticCode::SemaInvalidNegateRHS:                     return "S";
			break; case DiagnosticCode::SemaInvalidInfixArgTypes:                 return "S";
			break; case DiagnosticCode::SemaInvalidTypeQualifiers:                return "S";
			break; case DiagnosticCode::SemaParamTypeVoid:                        return "S";
			break; case DiagnosticCode::SemaNoMatchingFunction:                   return "S";
			break; case DiagnosticCode::SemaMultipleMatchingFunctions:            return "S";
			break; case DiagnosticCode::SemaParamsCannotBeConstEval:              return "S";
			break; case DiagnosticCode::SemaArgIncorrectLabel:                    return "S";
			break; case DiagnosticCode::SemaArgMustBeLabeled:                     return "S";
			break; case DiagnosticCode::SemaInvalidUseOfInitializerValueExpr:     return "S";
			break; case DiagnosticCode::SemaIncorrectNumberOfAssignTargets:       return "S";
			break; case DiagnosticCode::SemaMultipleValuesIntoOne:                return "S";
			break; case DiagnosticCode::SemaFuncIsntTerminated:                   return "S";
			break; case DiagnosticCode::SemaAttributeAlreadySet:                  return "S";
			break; case DiagnosticCode::SemaInvalidAttributeArgument:             return "S";
			break; case DiagnosticCode::SemaCantCallRuntimeFuncInComptimeContext: return "S";
			break; case DiagnosticCode::SemaComptimeCircularDependency:           return "S";
			break; case DiagnosticCode::SemaTypeCannotBeUsedAsExpr:               return "S";
			break; case DiagnosticCode::SemaExprCannotBeUsedAsType:               return "S";
			break; case DiagnosticCode::SemaNotValidExprForTypeIDConversion:      return "S";
			break; case DiagnosticCode::SemaErrorInRunningOfIntrinsicAtComptime:  return "S";
			break; case DiagnosticCode::SemaErrorInRunningOfFuncAtComptime:       return "S";
			break; case DiagnosticCode::SemaCannotConvertFluidValue:              return "S";
			break; case DiagnosticCode::SemaCannotTypedefVoid:                    return "S";
			break; case DiagnosticCode::SemaCannotNewVoid:                        return "S";
			break; case DiagnosticCode::SemaIncorrectNumArgsForPrimitiveOpNew:    return "S";
			break; case DiagnosticCode::SemaArgHasLabelInrimitiveOpNew:           return "S";

			break; case DiagnosticCode::SemaWarnSingleValInMultiAssign:           return "SW";
			break; case DiagnosticCode::SemaWarnEntryIsImplicitRuntime:           return "SW";

			break; case DiagnosticCode::LLLVMDataLayoutError:                     return "LLVM1";

			break; case DiagnosticCode::MiscFileDoesNotExist:                     return "M1";
			break; case DiagnosticCode::MiscLoadFileFailed:                       return "M2";
			break; case DiagnosticCode::MiscUnimplementedFeature:                 return "M3";
			break; case DiagnosticCode::MiscUnknownFatal:                         return "M4";
			break; case DiagnosticCode::MiscNoEntrySet:                           return "M5";
			break; case DiagnosticCode::MiscInvalidKind:                          return "M6";
			break; case DiagnosticCode::MiscRuntimePanic:                         return "M7";
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
