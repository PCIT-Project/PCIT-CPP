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
#include "./tokens/Token.h"
#include "./AST/AST.h"
#include "./sema/sema.h"

namespace pcit::panther{



	struct Diagnostic{
		enum class Level{
			Fatal,
			Error,
			Warning,
		};

		enum class Code{
			//////////////////
			// tokens

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


			//////////////////
			// parser

			ParserUnknownStmtStart,
			ParserIncorrectStmtContinuation,
			ParserUnexpectedEOF,
			ParserInvalidKindForAThisParam,
			ParserDereferenceOrUnwrapOnType,
			ParserAssumedTokenNotPreset,
			ParserEmptyMultiAssign,
			ParserEmptyFuncReturnBlock,
			ParserInvalidNewExpr,
			ParserAttributesInWrongPlace,
			ParserTooManyAttributeArgs,


			//////////////////
			// symbol proc

			SymbolProcInvalidGlobalStmt,
			SymbolProcInvalidBaseType,
			SymbolProcInvalidExprKind,
			SymbolProcImportRequiresOneArg,
			SymbolProcCircularDep,


			//////////////////
			// sema

			// types
			SemaVoidWithQualifiers,
			SemaInvalidTypeQualifiers,
			SemaGenericTypeNotInTemplatePackDecl,

			// idents
			SemaIdentNotInScope,
			SemaIdentAlreadyInScope,

			// vars
			SemaVarTypeVoid,
			SemaVarWithNoValue,
			SemaVarDefNotEphemeral,
			SemaVarInitializerWithoutExplicitType,
			SemaVarInitializerOnNonVar,

			// exprs
			SemaTypeUsedAsExpr,
			SemaInvalidAccessorRHS,

			// type checking
			SemaMultiReturnIntoSingleValue,
			SemaCannotConvertFluidValue,
			SemaTypeMismatch,
			SemaAliasCannotBeVoid,

			// imports
			SemaNoSymbolInModuleWithThatIdent,
			SemaSymbolNotPub,
			SemaFailedToImportModule,
			SemaModuleVarMustBeDef,

			// attributes
			SemaAttributeAlreadySet,
			SemaUnknownAttribute,
			SemaAttributeImplictSet,
			SemaTooManyAttributeArgs,


			//////////////////
			// misc

			MiscUnimplementedFeature,
			MiscFileDoesNotExist,
			MiscLoadFileFailed,
			MiscStallDetected,
		};


		class Location{
			public:
				using None = std::monostate;
				static constexpr None NONE = std::monostate();

			public:
				Location() : variant() {}
				Location(None) : variant() {}
				Location(SourceLocation src_location) : variant(src_location) {}

				~Location() = default;


				template<class T>
				EVO_NODISCARD auto is() const -> bool { return this->variant.is<T>(); }

				template<class T>
				EVO_NODISCARD auto as() -> T& { return this->variant.as<T>(); }

				template<class T>
				EVO_NODISCARD auto as() const -> const T& { return this->variant.as<T>(); }

				auto visit(auto callable)       { return this->variant.visit(callable); }
				auto visit(auto callable) const { return this->variant.visit(callable); }


				// These are convinence functions to get the locations of a specific item.
				// It is not necesarily the fastest way of getting the source location, 
				// 		so don't use in performance-critical code.

				// Tokens
				EVO_NODISCARD static auto get(Token::ID token_id, const class Source& src) -> Location;

				// AST
				EVO_NODISCARD static auto get(const AST::Node& node, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::VarDecl& var_decl, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::FuncDecl& func_decl, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::AliasDecl& alias_decl, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::TypedefDecl& typedef_decl, const class Source& src)
					-> Location;
				EVO_NODISCARD static auto get(const AST::StructDecl& struct_decl, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::Return& return_stmt, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::Conditional& conditional, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::WhenConditional& when_cond, const class Source& src)
					-> Location;
				EVO_NODISCARD static auto get(const AST::While& while_loop, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::Block& block, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::FuncCall& func_call, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::TemplatedExpr& templated_expr, const class Source& src)
					-> Location;
				EVO_NODISCARD static auto get(const AST::Prefix& prefix, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::Infix& infix, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::Postfix& postfix, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::MultiAssign& multi_assign, const class Source& src)
					-> Location;
				EVO_NODISCARD static auto get(const AST::New& new_expr, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::Type& type, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::TypeIDConverter& type, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::AttributeBlock::Attribute& attr, const class Source& src)
					-> Location;

				// sema / types
				EVO_NODISCARD static auto get(
					const sema::Var::ID& sema_var_id, const class Source& src, const class Context& context
				) -> Location;

				EVO_NODISCARD static auto get(
					const BaseType::Alias::ID& alias_id, const class Source& src, const class Context& context
				) -> Location;

				EVO_NODISCARD static auto get(
					const BaseType::Struct::ID& struct_id, const class Source& src, const class Context& context
				) -> Location;
		
			private:
				evo::Variant<None, SourceLocation> variant;
		};

		

		struct Info{
			std::string message;
			Location location;
			std::vector<Info> sub_infos;

			Info(std::string&& _message) : message(std::move(_message)), location(), sub_infos() {
				// _debug_analyze_message(this->message);
			}
			Info(std::string&& _message, Location loc) 
				: message(std::move(_message)), location(loc), sub_infos() {
				// _debug_analyze_message(this->message);
			}
			Info(std::string&& _message, Location loc, std::vector<Info>&& _sub_infos)
				: message(std::move(_message)), location(loc), sub_infos(std::move(_sub_infos)) {
				// _debug_analyze_message(this->message);
			}

			Info(const Info& rhs) : message(rhs.message), location(rhs.location), sub_infos(rhs.sub_infos) {}

			auto operator=(const Info& rhs) -> Info& {
				this->message = rhs.message;
				this->location = rhs.location;
				this->sub_infos = rhs.sub_infos;
				
				return *this;
			}

			Info(Info&& rhs)
				: message(std::move(rhs.message)), location(rhs.location), sub_infos(std::move(rhs.sub_infos)) {}
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
		) : level(_level), code(_code), location(_location), message(_message), infos(_infos) {
			_debug_analyze_message(this->message);
		}

		Diagnostic(
			Level _level,
			Code _code,
			const Location& _location,
			std::string&& _message,
			const evo::SmallVector<Info>& _infos
		) : level(_level), code(_code), location(_location), message(std::move(_message)), infos(_infos) {
			_debug_analyze_message(this->message);
		}

		Diagnostic(
			Level _level,
			Code _code,
			const Location& _location,
			const std::string& _message,
			evo::SmallVector<Info>&& _infos = {}
		) : level(_level), code(_code), location(_location), message(_message), infos(std::move(_infos)) {
			_debug_analyze_message(this->message);
		}

		Diagnostic(
			Level _level,
			Code _code,
			const Location& _location,
			std::string&& _message,
			evo::SmallVector<Info>&& _infos = {}
		) : level(_level), code(_code), location(_location), message(std::move(_message)), infos(std::move(_infos)) {
			_debug_analyze_message(this->message);
		}


		Diagnostic(
			Level _level,
			Code _code,
			const Location& _location,
			const std::string& _message,
			const Info& info
		) : level(_level), code(_code), location(_location), message(_message), infos{info} {
			_debug_analyze_message(this->message);
		}

		Diagnostic(
			Level _level,
			Code _code,
			const Location& _location,
			std::string&& _message,
			const Info& info
		) : level(_level), code(_code), location(_location), message(std::move(_message)), infos{info} {
			_debug_analyze_message(this->message);
		}

		Diagnostic(
			Level _level,
			Code _code,
			const Location& _location,
			const std::string& _message,
			Info&& info
		) : level(_level), code(_code), location(_location), message(_message), infos{std::move(info)} {
			_debug_analyze_message(this->message);
		}

		Diagnostic(
			Level _level,
			Code _code,
			const Location& _location,
			std::string&& _message,
			Info&& info
		) : level(_level), code(_code), location(_location), message(std::move(_message)), infos{std::move(info)} {
			_debug_analyze_message(this->message);
		}



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
				case Code::ParserAttributesInWrongPlace:       return "P10";
				case Code::ParserTooManyAttributeArgs:         return "P11";

				// TODO: give individual codes and put in correct order
				case Code::SymbolProcInvalidGlobalStmt:
				case Code::SymbolProcInvalidBaseType:
				case Code::SymbolProcInvalidExprKind:
				case Code::SymbolProcImportRequiresOneArg:
				case Code::SymbolProcCircularDep:
					return "SP";

				// TODO: give individual codes and put in correct order
				case Code::SemaVoidWithQualifiers:
				case Code::SemaInvalidTypeQualifiers:
				case Code::SemaGenericTypeNotInTemplatePackDecl:
				case Code::SemaIdentNotInScope:
				case Code::SemaIdentAlreadyInScope:
				case Code::SemaVarTypeVoid:
				case Code::SemaVarWithNoValue:
				case Code::SemaVarDefNotEphemeral:
				case Code::SemaVarInitializerWithoutExplicitType:
				case Code::SemaVarInitializerOnNonVar:
				case Code::SemaTypeUsedAsExpr:
				case Code::SemaInvalidAccessorRHS:
				case Code::SemaMultiReturnIntoSingleValue:
				case Code::SemaCannotConvertFluidValue:
				case Code::SemaTypeMismatch:
				case Code::SemaAliasCannotBeVoid:
				case Code::SemaNoSymbolInModuleWithThatIdent:
				case Code::SemaSymbolNotPub:
				case Code::SemaFailedToImportModule:
				case Code::SemaModuleVarMustBeDef:
				case Code::SemaAttributeAlreadySet:
				case Code::SemaUnknownAttribute:
				case Code::SemaAttributeImplictSet:
				case Code::SemaTooManyAttributeArgs:
					return "S";

				case Code::MiscUnimplementedFeature:           return "M0";
				case Code::MiscFileDoesNotExist:               return "M1";
				case Code::MiscLoadFileFailed:                 return "M2";
				case Code::MiscStallDetected:                  return "M3";
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



		EVO_NODISCARD static auto _debug_analyze_message(std::string_view message) -> void {
			evo::debugAssert(message.empty() == false, "Diagnostic message cannot be empty");
			evo::debugAssert(std::isupper(int(message[0])), "Diagnostic message must start with an upper letter");
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