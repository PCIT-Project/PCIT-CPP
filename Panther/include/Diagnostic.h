////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////



#pragma once


#include <source_location>

#include <Evo.h>
#include <PCIT_core.h>

#include "./source/source_data.h"


namespace pcit::panther{


	struct Diagnostic{
		enum class Level{
			Fatal,
			Error,
			Warning,
		};

		enum class Code{
			TokUnrecognizedCharacter,
			TokUnterminatedMultilineComment,
			TokUnterminatedTextLiteral,
			TokUnterminatedTextEscapeSequence,
			TokLiteralLeadingZero,
			TokLiteralNumMultipleDecimalPoints,
			TokInvalidFPBase,
			TokInvalidNumDigit,
			TokLiteralNumTooBig,
			TokInvalidIntegerWidth,
			TokUnknownFailureToTokenizeNum,
			TokInvalidChar,
			TokFileTooLarge,
			TokFileLocationLimitOOB,
			TokDoubleUnderscoreNotAllowed,

			ParserUnknownStmtStart,
			ParserIncorrectStmtContinuation,
			ParserUnexpectedEOF,
			ParserInvalidKindForAThisParam,
			ParserDereferenceOrUnwrapOnType,
			ParserAssumedTokenNotPreset,
			ParserEmptyMultiAssign,
			ParserEmptyFuncReturnBlock,
			ParserInvalidNewExpr,
			ParserDiagnosticsInWrongPlace,

			MiscFileDoesNotExist,
			MiscLoadFileFailed,
			MiscUnimplementedFeature,
		};

		using NoLocation = std::monostate;
		static constexpr NoLocation noLocation = std::monostate();
		using Location = evo::Variant<NoLocation, SourceLocation>;

		struct Info{
			std::string message;
			Location location;
			std::vector<Info> sub_infos;

			Info(std::string&& _message) : message(std::move(_message)), location(), sub_infos() {};
			Info(std::string&& _message, Location loc) 
				: message(std::move(_message)), location(loc), sub_infos() {};
			Info(std::string&& _message, Location loc, std::vector<Info>&& _sub_infos)
				: message(std::move(_message)), location(loc), sub_infos(std::move(_sub_infos)) {};

			Info(const Info& rhs) : message(rhs.message), location(rhs.location), sub_infos(rhs.sub_infos) {};

			auto operator=(const Info& rhs) -> Info& {
				this->message = rhs.message;
				this->location = rhs.location;
				this->sub_infos = rhs.sub_infos;
				
				return *this;
			}

			Info(Info&& rhs)
				: message(std::move(rhs.message)), location(rhs.location), sub_infos(std::move(rhs.sub_infos)) {};
		};

		Level level;
		Code code;
		Location location;
		std::string message;
		evo::SmallVector<Info> infos;


		Diagnostic(
			Level _level,
			Code _code,
			const Location& _location,
			const std::string& _message,
			const evo::SmallVector<Info>& _infos = {}
		) : level(_level), code(_code), location(_location), message(_message), infos(_infos) {}

		Diagnostic(
			Level _level,
			Code _code,
			const Location& _location,
			std::string&& _message,
			const evo::SmallVector<Info>& _infos
		) : level(_level), code(_code), location(_location), message(std::move(_message)), infos(_infos) {}

		Diagnostic(
			Level _level,
			Code _code,
			const Location& _location,
			const std::string& _message,
			evo::SmallVector<Info>&& _infos = {}
		) : level(_level), code(_code), location(_location), message(_message), infos(std::move(_infos)) {}

		Diagnostic(
			Level _level,
			Code _code,
			const Location& _location,
			std::string&& _message,
			evo::SmallVector<Info>&& _infos = {}
		) : level(_level), code(_code), location(_location), message(std::move(_message)), infos(std::move(_infos)) {}


		Diagnostic(
			Level _level,
			Code _code,
			const Location& _location,
			const std::string& _message,
			const Info& info
		) : level(_level), code(_code), location(_location), message(_message), infos{info} {}

		Diagnostic(
			Level _level,
			Code _code,
			const Location& _location,
			std::string&& _message,
			const Info& info
		) : level(_level), code(_code), location(_location), message(std::move(_message)), infos{info} {}

		Diagnostic(
			Level _level,
			Code _code,
			const Location& _location,
			const std::string& _message,
			Info&& info
		) : level(_level), code(_code), location(_location), message(_message), infos{std::move(info)} {}

		Diagnostic(
			Level _level,
			Code _code,
			const Location& _location,
			std::string&& _message,
			Info&& info
		) : level(_level), code(_code), location(_location), message(std::move(_message)), infos{std::move(info)} {}



		EVO_NODISCARD static auto printCode(Code code) -> std::string_view {
			switch(code){
				case Code::TokUnrecognizedCharacter:           return "T1";
				case Code::TokUnterminatedMultilineComment:    return "T2";
				case Code::TokUnterminatedTextLiteral:         return "T3";
				case Code::TokUnterminatedTextEscapeSequence:  return "T4";
				case Code::TokLiteralLeadingZero:              return "T5";
				case Code::TokLiteralNumMultipleDecimalPoints: return "T6";
				case Code::TokInvalidFPBase:                   return "T7";
				case Code::TokInvalidNumDigit:                 return "T8";
				case Code::TokLiteralNumTooBig:                return "T9";
				case Code::TokInvalidIntegerWidth:             return "T10";
				case Code::TokUnknownFailureToTokenizeNum:     return "T11";
				case Code::TokInvalidChar:                     return "T12";
				case Code::TokFileTooLarge:                    return "T13";
				case Code::TokFileLocationLimitOOB:            return "T14";
				case Code::TokDoubleUnderscoreNotAllowed:      return "T15";

				case Code::ParserUnknownStmtStart:             return "P1";
				case Code::ParserIncorrectStmtContinuation:    return "P2";
				case Code::ParserUnexpectedEOF:                return "P3";
				case Code::ParserInvalidKindForAThisParam:     return "P4";
				case Code::ParserDereferenceOrUnwrapOnType:    return "P5";
				case Code::ParserAssumedTokenNotPreset:        return "P6";
				case Code::ParserEmptyMultiAssign:             return "P7";
				case Code::ParserEmptyFuncReturnBlock:         return "P8";
				case Code::ParserInvalidNewExpr:               return "P9";
				case Code::ParserDiagnosticsInWrongPlace:      return "P10";

				case Code::MiscFileDoesNotExist:               return "M1";
				case Code::MiscLoadFileFailed:                 return "M2";
				case Code::MiscUnimplementedFeature:           return "M3";
			}

			evo::debugFatalBreak("Unknown or unsupported pcit::panther::Diagnostic::Code");
		}

		EVO_NODISCARD static auto createFatalMessage(
			std::string_view msg, std::source_location source_location = std::source_location::current()
		) -> std::string {
			return std::format(
				"{} (error location: {} | {})", msg, source_location.function_name(), source_location.line()
			);
		}


		EVO_NODISCARD static auto printLevel(Level level) -> std::string_view {
			switch(level){
				break; case Level::Fatal:   return "Fatal";
				break; case Level::Error:   return "Error";
				break; case Level::Warning: return "Warning";
			}

			evo::debugFatalBreak("Unknown or unsupported pcit::core::Diagnostic::Level");
		}
	};

}


template<>
struct std::formatter<pcit::panther::Diagnostic::Code> : std::formatter<std::string_view> {
    auto format(const pcit::panther::Diagnostic::Code& code, std::format_context& ctx) const
    -> std::format_context::iterator {
        return std::formatter<std::string_view>::format(pcit::panther::Diagnostic::printCode(code), ctx);
    }
};

template<>
struct std::formatter<pcit::panther::Diagnostic::Level> : std::formatter<std::string_view> {
    auto format(const pcit::panther::Diagnostic::Level& level, std::format_context& ctx) const
    -> std::format_context::iterator {
        return std::formatter<std::string_view>::format(pcit::panther::Diagnostic::printLevel(level), ctx);
    }
};