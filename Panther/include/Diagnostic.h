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
			FATAL,
			ERROR,
			WARNING,
		};

		enum class Code{
			//////////////////
			// tokens

			TOK_UNRECOGNIZED_CHARACTER,
			TOK_UNTERMINATED_MULTILINE_COMMENT,
			TOK_UNTERMINATED_TEXT_LITERAL,
			TOK_UNTERMINATED_TEXT_ESCAPE_SEQUENCE,
			TOK_LITERAL_LEADING_ZERO,
			TOK_LITERAL_NUM_MULTIPLE_DECIMAL_POINTS,
			TOK_INVALID_FPBASE,
			TOK_FLOAT_LITERAL_ENDING_IN_PERIOD,
			TOK_INVALID_NUM_DIGIT,
			TOK_LITERAL_NUM_TOO_BIG,
			TOK_INVALID_INTEGER_WIDTH,
			TOK_UNKNOWN_FAILURE_TO_TOKENIZE_NUM,
			TOK_INVALID_CHAR,
			TOK_FILE_TOO_LARGE,
			TOK_FILE_LOCATION_LIMIT_OOB,
			// TOK_DOUBLE_UNDERSCORE_NOT_ALLOWED,


			//////////////////
			// parser

			PARSER_UNKNOWN_STMT_START,
			PARSER_INCORRECT_STMT_CONTINUATION,
			PARSER_UNEXPECTED_EOF,
			PARSER_INVALID_KIND_FOR_A_THIS_PARAM,
			PARSER_OOO_DEFAULT_VALUE_PARAM,
			PARSER_DEREFERENCE_OR_UNWRAP_ON_TYPE,
			PARSER_ASSUMED_TOKEN_NOT_PRESET,
			PARSER_EMPTY_MULTI_ASSIGN,
			PARSER_EMPTY_FUNC_RETURN_BLOCK,
			PARSER_INVALID_NEW_EXPR,
			PARSER_ATTRIBUTES_IN_WRONG_PLACE,
			PARSER_TOO_MANY_ATTRIBUTE_ARGS,
			PARSER_EMPTY_ERROR_RETURN_PARAMS,
			PARSER_TEMPLATE_PARAMETER_BLOCK_EMPTY,
			PARSER_TYPE_DEDUCER_INVALID_IN_THIS_CONTEXT,
			PARSER_BLOCK_EXPR_EMPTY_OUTPUTS_BLOCK,


			//////////////////
			// symbol proc

			SYMBOL_PROC_INVALID_GLOBAL_STMT,
			SYMBOL_PROC_INVALID_BASE_TYPE,
			SYMBOL_PROC_INVALID_EXPR_KIND,
			SYMBOL_PROC_IMPORT_REQUIRES_ONE_ARG,
			SYMBOL_PROC_CIRCULAR_DEP,
			SYMBOL_PROC_TYPE_USED_AS_EXPR,
			SYMBOL_PROC_VAR_WITH_NO_VALUE,
			SYMBOL_PROC_LABELED_VOID_RETURN,


			//////////////////
			// sema

			// types
			SEMA_VOID_WITH_QUALIFIERS,
			SEMA_INVALID_TYPE_QUALIFIERS,
			SEMA_GENERIC_TYPE_NOT_IN_TEMPLATE_PACK_DECL,
			SEMA_NOT_TEMPLATED_TYPE_WITH_TEMPLATE_ARGS,

			// idents
			SEMA_IDENT_NOT_IN_SCOPE,
			SEMA_IDENT_ALREADY_IN_SCOPE,
			SEMA_INTRINSIC_DOESNT_EXIST,

			// vars
			SEMA_VAR_TYPE_VOID,
			SEMA_VAR_DEF_NOT_EPHEMERAL,
			SEMA_VAR_INITIALIZER_WITHOUT_EXPLICIT_TYPE,
			SEMA_VAR_INITIALIZER_ON_NON_VAR,

			// exprs
			SEMA_TYPE_USED_AS_EXPR,
			SEMA_INVALID_ACCESSOR_RHS,
			SEMA_EXPR_NOT_CONSTEXPR,
			SEMA_EXPR_NOT_COMPTIME,
			SEMA_COPY_ARG_NOT_CONCRETE,
			SEMA_MOVE_ARG_NOT_CONCRETE,
			SEMA_MOVE_ARG_NOT_MUTABLE,

			// type checking
			SEMA_MULTI_RETURN_INTO_SINGLE_VALUE,
			SEMA_CANNOT_CONVERT_FLUID_VALUE,
			SEMA_TYPE_MISMATCH,
			SEMA_ALIAS_CANNOT_BE_VOID,
			SEMA_CANNOT_INFER_TYPE,
			SEMA_TYPE_DEDUCER_IN_GLOBAL_VAR,

			// imports
			SEMA_NO_SYMBOL_IN_SCOPE_WITH_THAT_IDENT,
			SEMA_SYMBOL_NOT_PUB,
			SEMA_FAILED_TO_IMPORT_MODULE,
			SEMA_MODULE_VAR_MUST_BE_DEF,

			// attributes
			SEMA_ATTRIBUTE_ALREADY_SET,
			SEMA_UNKNOWN_ATTRIBUTE,
			SEMA_ATTRIBUTE_IMPLICT_SET,
			SEMA_TOO_MANY_ATTRIBUTE_ARGS,

			// templates
			SEMA_TEMPLATE_PARAM_CANNOT_BE_TYPE_VOID,
			SEMA_TEMPLATE_PARAM_TYPE_DEFAULT_MUST_BE_TYPE,
			SEMA_TEMPLATE_PARAM_EXPR_DEFAULT_MUST_BE_EXPR,
			SEMA_TEMPLATE_TOO_FEW_ARGS,
			SEMA_TEMPLATE_TOO_MANY_ARGS,
			SEMA_TEMPLATE_INVALID_ARG,

			// functions
			SEMA_PARAM_TYPE_VOID,
			SEMA_INVALID_SCOPE_FOR_THIS,
			SEMA_NAMED_VOID_RETURN,
			SEMA_NOT_FIRST_RETURN_VOID,
			SEMA_FUNC_ISNT_TERMINATED,
			SEMA_INVALID_ENTRY,

			// function calls
			SEMA_CANNOT_CALL_LIKE_FUNCTION,
			SEMA_MULTIPLE_MATCHING_FUNCTION_OVERLOADS,
			SEMA_NO_MATCHING_FUNCTION,
			SEMA_ERROR_RETURNED_FROM_CONSTEXPR_FUNC_RUN,
			SEMA_FUNC_ISNT_CONSTEXPR,
			SEMA_FUNC_ISNT_COMPTIME,
			SEMA_FUNC_ISNT_RUNTIME,
			SEMA_DISCARDING_RETURNS,
			SEMA_INVALID_MODE_FOR_INTRINSIC,
			SEMA_FUNC_ERRORS,
			SEMA_FUNC_DOESNT_ERROR,
			SEMA_IMPORT_DOESNT_ERROR,

			// Assignment
			SEMA_ASSIGN_LHS_NOT_CONCRETE,
			SEMA_ASSIGN_LHS_NOT_MUTABLE,
			SEMA_ASSIGN_RHS_NOT_EPHEMERAL,
			SEMA_MULTI_ASSIGN_RHS_NOT_MULTI,
			SEMA_MULTI_ASSIGN_RHS_WRONG_NUM,

			// Try
			SEMA_TRY_EXCEPT_PARAMS_WRONG_NUM,
			SEMA_TRY_ELSE_ATTEMPT_NOT_FUNC_CALL,
			SEMA_TRY_ELSE_ATTEMPT_NOT_SINGLE,
			SEMA_TRY_ELSE_EXCEPT_NOT_EPHEMERAL,

			// terminators
			SEMA_INCORRECT_RETURN_STMT_KIND,
			SEMA_RETURN_NOT_EPHEMERAL,
			SEMA_ERROR_IN_FUNC_WITHOUT_ERRORS,
			SEMA_SCOPE_IS_ALREADY_TERMINATED,
			SEMA_NOT_ERRORING_FUNC_CALL,
			SEMA_RETURN_LABEL_NOT_FOUND,

			// misc
			SEMA_BLOCK_EXPR_OUTPUT_PARAM_VOID,
			SEMA_BLOCK_EXPR_NOT_TERMINATED,


			//////////////////
			// misc

			MISC_UNIMPLEMENTED_FEATURE, // M0
			MISC_UNKNOWN_ERROR,         // M1
			MISC_FILE_DOES_NOT_EXIST,   // M2
			MISC_LOAD_FILE_FAILED,      // M3
			MISC_LLVM_ERROR,            // M4
			MISC_STALL_DETECTED,        // M5
			MISC_NO_ENTRY,              // M6


			//////////////////
			// frontend

			FRONTEND_FAILED_TO_GET_REL_DIR,       // F1
			FRONTEND_BUILD_SYSTEM_RETURNED_ERROR, // F2
			FRONTEND_FAILED_TO_ADD_STD_LIB,       // F3
			FRONTEND_FAILED_TO_OUTPUT_ASM,        // F4
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
				EVO_NODISCARD static auto get(const AST::FuncDecl::Param& param, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::FuncDecl::Return& ret, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::AliasDecl& alias_decl, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::TypedefDecl& typedef_decl, const class Source& src)
					-> Location;
				EVO_NODISCARD static auto get(const AST::StructDecl& struct_decl, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::Return& return_stmt, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::Error& error_stmt, const class Source& src) -> Location;
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
				EVO_NODISCARD static auto get(const AST::TryElse& try_expr, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::Type& type, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::TypeIDConverter& type, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::AttributeBlock::Attribute& attr, const class Source& src)
					-> Location;

				// sema / types
				EVO_NODISCARD static auto get(
					const sema::GlobalVar::ID& sema_var_id, const class Source& src, const class Context& context
				) -> Location;

				EVO_NODISCARD static auto get(
					const sema::Func::ID& func_id, const class Source& src, const class Context& context
				) -> Location;

				EVO_NODISCARD static auto get(
					const sema::TemplatedFunc::ID& templated_func_id,
					const class Source& src,
					const class Context& context
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
			evo::SmallVector<Info> sub_infos;

			Info(std::string&& _message) : message(std::move(_message)), location(), sub_infos() {
				// _debug_analyze_message(this->message);
			}
			Info(std::string&& _message, Location loc) 
				: message(std::move(_message)), location(loc), sub_infos() {
				// _debug_analyze_message(this->message);
			}
			Info(std::string&& _message, Location loc, evo::SmallVector<Info>&& _sub_infos)
				: message(std::move(_message)), location(loc), sub_infos(std::move(_sub_infos)) {
				// _debug_analyze_message(this->message);
			}

			// Info(const Info& rhs) : message(rhs.message), location(rhs.location), sub_infos(rhs.sub_infos) {}

			// auto operator=(const Info& rhs) -> Info& {
			// 	this->message = rhs.message;
			// 	this->location = rhs.location;
			// 	this->sub_infos = rhs.sub_infos;
				
			// 	return *this;
			// }

			// Info(Info&& rhs)
			// 	: message(std::move(rhs.message)), location(rhs.location), sub_infos(std::move(rhs.sub_infos)) {}
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
				case Code::TOK_UNRECOGNIZED_CHARACTER:              return "T1";
				case Code::TOK_UNTERMINATED_MULTILINE_COMMENT:      return "T2";
				case Code::TOK_UNTERMINATED_TEXT_LITERAL:           return "T3";
				case Code::TOK_UNTERMINATED_TEXT_ESCAPE_SEQUENCE:   return "T4";
				case Code::TOK_LITERAL_LEADING_ZERO:                return "T5";
				case Code::TOK_LITERAL_NUM_MULTIPLE_DECIMAL_POINTS: return "T6";
				case Code::TOK_INVALID_FPBASE:                      return "T7";
				case Code::TOK_FLOAT_LITERAL_ENDING_IN_PERIOD:      return "T8";
				case Code::TOK_INVALID_NUM_DIGIT:                   return "T9";
				case Code::TOK_LITERAL_NUM_TOO_BIG:                 return "T10";
				case Code::TOK_INVALID_INTEGER_WIDTH:               return "T11";
				case Code::TOK_UNKNOWN_FAILURE_TO_TOKENIZE_NUM:     return "T12";
				case Code::TOK_INVALID_CHAR:                        return "T13";
				case Code::TOK_FILE_TOO_LARGE:                      return "T14";
				case Code::TOK_FILE_LOCATION_LIMIT_OOB:             return "T15";
				// case Code::TOK_DOUBLE_UNDERSCORE_NOT_ALLOWED:       return "T16";

				case Code::PARSER_UNKNOWN_STMT_START:                   return "P1";
				case Code::PARSER_INCORRECT_STMT_CONTINUATION:          return "P2";
				case Code::PARSER_UNEXPECTED_EOF:                       return "P3";
				case Code::PARSER_INVALID_KIND_FOR_A_THIS_PARAM:        return "P4";
				case Code::PARSER_OOO_DEFAULT_VALUE_PARAM:              return "P5";
				case Code::PARSER_DEREFERENCE_OR_UNWRAP_ON_TYPE:        return "P6";
				case Code::PARSER_ASSUMED_TOKEN_NOT_PRESET:             return "P7";
				case Code::PARSER_EMPTY_MULTI_ASSIGN:                   return "P8";
				case Code::PARSER_EMPTY_FUNC_RETURN_BLOCK:              return "P9";
				case Code::PARSER_INVALID_NEW_EXPR:                     return "P10";
				case Code::PARSER_ATTRIBUTES_IN_WRONG_PLACE:            return "P11";
				case Code::PARSER_TOO_MANY_ATTRIBUTE_ARGS:              return "P12";
				case Code::PARSER_EMPTY_ERROR_RETURN_PARAMS:            return "P13";
				case Code::PARSER_TEMPLATE_PARAMETER_BLOCK_EMPTY:       return "P14";
				case Code::PARSER_TYPE_DEDUCER_INVALID_IN_THIS_CONTEXT: return "P15";
				case Code::PARSER_BLOCK_EXPR_EMPTY_OUTPUTS_BLOCK:       return "P16";

				// TODO(FUTURE): give individual codes and put in correct order
				case Code::SYMBOL_PROC_INVALID_GLOBAL_STMT:
				case Code::SYMBOL_PROC_INVALID_BASE_TYPE:
				case Code::SYMBOL_PROC_INVALID_EXPR_KIND:
				case Code::SYMBOL_PROC_IMPORT_REQUIRES_ONE_ARG:
				case Code::SYMBOL_PROC_CIRCULAR_DEP:
				case Code::SYMBOL_PROC_TYPE_USED_AS_EXPR:
				case Code::SYMBOL_PROC_VAR_WITH_NO_VALUE:
				case Code::SYMBOL_PROC_LABELED_VOID_RETURN:
					return "SP";

				// TODO(FUTURE): give individual codes and put in correct order
				case Code::SEMA_VOID_WITH_QUALIFIERS:
				case Code::SEMA_INVALID_TYPE_QUALIFIERS:
				case Code::SEMA_GENERIC_TYPE_NOT_IN_TEMPLATE_PACK_DECL:
				case Code::SEMA_NOT_TEMPLATED_TYPE_WITH_TEMPLATE_ARGS:
				case Code::SEMA_IDENT_NOT_IN_SCOPE:
				case Code::SEMA_IDENT_ALREADY_IN_SCOPE:
				case Code::SEMA_INTRINSIC_DOESNT_EXIST:
				case Code::SEMA_VAR_TYPE_VOID:
				case Code::SEMA_VAR_DEF_NOT_EPHEMERAL:
				case Code::SEMA_VAR_INITIALIZER_WITHOUT_EXPLICIT_TYPE:
				case Code::SEMA_VAR_INITIALIZER_ON_NON_VAR:
				case Code::SEMA_TYPE_USED_AS_EXPR:
				case Code::SEMA_INVALID_ACCESSOR_RHS:
				case Code::SEMA_EXPR_NOT_CONSTEXPR:
				case Code::SEMA_EXPR_NOT_COMPTIME:
				case Code::SEMA_COPY_ARG_NOT_CONCRETE:
				case Code::SEMA_MOVE_ARG_NOT_CONCRETE:
				case Code::SEMA_MOVE_ARG_NOT_MUTABLE:
				case Code::SEMA_MULTI_RETURN_INTO_SINGLE_VALUE:
				case Code::SEMA_CANNOT_CONVERT_FLUID_VALUE:
				case Code::SEMA_TYPE_MISMATCH:
				case Code::SEMA_ALIAS_CANNOT_BE_VOID:
				case Code::SEMA_CANNOT_INFER_TYPE:
				case Code::SEMA_TYPE_DEDUCER_IN_GLOBAL_VAR:
				case Code::SEMA_NO_SYMBOL_IN_SCOPE_WITH_THAT_IDENT:
				case Code::SEMA_SYMBOL_NOT_PUB:
				case Code::SEMA_FAILED_TO_IMPORT_MODULE:
				case Code::SEMA_MODULE_VAR_MUST_BE_DEF:
				case Code::SEMA_ATTRIBUTE_ALREADY_SET:
				case Code::SEMA_UNKNOWN_ATTRIBUTE:
				case Code::SEMA_ATTRIBUTE_IMPLICT_SET:
				case Code::SEMA_TOO_MANY_ATTRIBUTE_ARGS:
				case Code::SEMA_TEMPLATE_PARAM_CANNOT_BE_TYPE_VOID:
				case Code::SEMA_TEMPLATE_PARAM_TYPE_DEFAULT_MUST_BE_TYPE:
				case Code::SEMA_TEMPLATE_PARAM_EXPR_DEFAULT_MUST_BE_EXPR:
				case Code::SEMA_TEMPLATE_TOO_FEW_ARGS:
				case Code::SEMA_TEMPLATE_TOO_MANY_ARGS:
				case Code::SEMA_TEMPLATE_INVALID_ARG:
				case Code::SEMA_PARAM_TYPE_VOID:
				case Code::SEMA_INVALID_SCOPE_FOR_THIS:
				case Code::SEMA_NAMED_VOID_RETURN:
				case Code::SEMA_NOT_FIRST_RETURN_VOID:
				case Code::SEMA_FUNC_ISNT_TERMINATED:
				case Code::SEMA_INVALID_ENTRY:
				case Code::SEMA_CANNOT_CALL_LIKE_FUNCTION:
				case Code::SEMA_MULTIPLE_MATCHING_FUNCTION_OVERLOADS:
				case Code::SEMA_NO_MATCHING_FUNCTION:
				case Code::SEMA_ERROR_RETURNED_FROM_CONSTEXPR_FUNC_RUN:
				case Code::SEMA_FUNC_ISNT_CONSTEXPR:
				case Code::SEMA_FUNC_ISNT_COMPTIME:
				case Code::SEMA_FUNC_ISNT_RUNTIME:
				case Code::SEMA_DISCARDING_RETURNS:
				case Code::SEMA_INVALID_MODE_FOR_INTRINSIC:
				case Code::SEMA_FUNC_ERRORS:
				case Code::SEMA_FUNC_DOESNT_ERROR:
				case Code::SEMA_IMPORT_DOESNT_ERROR:
				case Code::SEMA_ASSIGN_LHS_NOT_CONCRETE:
				case Code::SEMA_ASSIGN_LHS_NOT_MUTABLE:
				case Code::SEMA_ASSIGN_RHS_NOT_EPHEMERAL:
				case Code::SEMA_MULTI_ASSIGN_RHS_NOT_MULTI:
				case Code::SEMA_MULTI_ASSIGN_RHS_WRONG_NUM:
				case Code::SEMA_TRY_EXCEPT_PARAMS_WRONG_NUM:
				case Code::SEMA_TRY_ELSE_ATTEMPT_NOT_FUNC_CALL:
				case Code::SEMA_TRY_ELSE_ATTEMPT_NOT_SINGLE:
				case Code::SEMA_TRY_ELSE_EXCEPT_NOT_EPHEMERAL:
				case Code::SEMA_INCORRECT_RETURN_STMT_KIND:
				case Code::SEMA_ERROR_IN_FUNC_WITHOUT_ERRORS:
				case Code::SEMA_RETURN_NOT_EPHEMERAL:
				case Code::SEMA_SCOPE_IS_ALREADY_TERMINATED:
				case Code::SEMA_NOT_ERRORING_FUNC_CALL:
				case Code::SEMA_RETURN_LABEL_NOT_FOUND:
				case Code::SEMA_BLOCK_EXPR_OUTPUT_PARAM_VOID:
				case Code::SEMA_BLOCK_EXPR_NOT_TERMINATED:
					return "S";

				case Code::MISC_UNIMPLEMENTED_FEATURE: return "M0";
				case Code::MISC_UNKNOWN_ERROR:         return "M1";
				case Code::MISC_FILE_DOES_NOT_EXIST:   return "M2";
				case Code::MISC_LOAD_FILE_FAILED:      return "M3";
				case Code::MISC_LLVM_ERROR:            return "M4";
				case Code::MISC_STALL_DETECTED:        return "M5";
				case Code::MISC_NO_ENTRY:              return "M6";

				case Code::FRONTEND_FAILED_TO_GET_REL_DIR:       return "F1";
				case Code::FRONTEND_BUILD_SYSTEM_RETURNED_ERROR: return "F2";
				case Code::FRONTEND_FAILED_TO_ADD_STD_LIB:       return "F3";
				case Code::FRONTEND_FAILED_TO_OUTPUT_ASM:        return "F4";
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
				break; case Level::FATAL:   return "Fatal";
				break; case Level::ERROR:   return "Error";
				break; case Level::WARNING: return "Warning";
			}

			evo::debugFatalBreak("Unknown or unsupported pcit::core::Diagnostic::Level");
		}



		EVO_NODISCARD static auto _debug_analyze_message(std::string_view message) -> void {
			evo::debugAssert(message.empty() == false, "Diagnostic message cannot be empty");
			evo::debugAssert(std::isupper(int(message[0])), "Diagnostic message must start with an upper letter");
		}


		private:
			EVO_NODISCARD static auto get_ast_node_from_symbol_proc(const SymbolProc& symbol_proc) -> const AST::Node&;

			friend class Location;
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