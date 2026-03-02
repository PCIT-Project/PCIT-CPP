////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////



#pragma once


#include <source_location>
#include <filesystem>

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


		class Location{
			public:
				using None = std::monostate;
				static constexpr None NONE = std::monostate{};

				struct Builtin{};
				static constexpr Builtin BUILTIN = Builtin{};

			public:
				Location() : variant() {}
				Location(None) : variant() {}
				Location(Builtin builtin) : variant(builtin) {}
				Location(SourceLocation src_location) : variant(src_location) {}
				Location(ClangSourceLocation clang_location) : variant(clang_location) {}

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
				EVO_NODISCARD static auto get(const AST::VarDef& var_def, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::FuncDef& func_def, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::FuncDef::Param& param, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::FuncDef::Return& ret, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(
					const AST::DeletedSpecialMethod& deleted_special_method, const class Source& src
				) -> Location;
				EVO_NODISCARD static auto get(const AST::FuncAliasDef& alias_def, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::AliasDef& alias_def, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::StructDef& struct_def, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::UnionDef& union_def, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::EnumDef& enum_def, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::InterfaceDef& interface_def, const class Source& src)
					-> Location;
				EVO_NODISCARD static auto get(const AST::InterfaceImpl& interface_impl, const class Source& src)
					-> Location;
				EVO_NODISCARD static auto get(const AST::Return& return_stmt, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::Error& error_stmt, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::Break& break_stmt, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::Continue& continue_stmt, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::Delete& delete_stmt, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::Conditional& conditional, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::WhenConditional& when_cond, const class Source& src)
					-> Location;
				EVO_NODISCARD static auto get(const AST::While& while_loop, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::For& for_loop, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::Switch& switch_stmt, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::Defer& defer, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::Block& block, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::FuncCall& func_call, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::Indexer& indexer, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::TemplatedExpr& templated_expr, const class Source& src)
					-> Location;
				EVO_NODISCARD static auto get(const AST::Prefix& prefix, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::Infix& infix, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::Postfix& postfix, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::MultiAssign& multi_assign, const class Source& src)
					-> Location;
				EVO_NODISCARD static auto get(const AST::New& new_expr, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::ArrayInitNew& new_expr, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::DesignatedInitNew& new_expr, const class Source& src)
					-> Location;
				EVO_NODISCARD static auto get(const AST::TryElse& try_expr, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::Unsafe& unsafe, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::ArrayType& type, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::InterfaceMap& type, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::Type& type, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::TypeIDConverter& type, const class Source& src) -> Location;
				EVO_NODISCARD static auto get(const AST::AttributeBlock::Attribute& attr, const class Source& src)
					-> Location;

				// sema / types
				EVO_NODISCARD static auto get(
					const sema::Var::ID& sema_var_id, const class Source& src, const class Context& context
				) -> Location;


				EVO_NODISCARD static auto get(sema::GlobalVar::ID global_var_id, const class Context& context)
					-> Location;
				EVO_NODISCARD static auto get(const sema::GlobalVar& global_var, const class Context& context)
					-> Location;

				EVO_NODISCARD static auto get(sema::Func::ID func_id, const class Context& context) -> Location;
				EVO_NODISCARD static auto get(const sema::Func& func, const class Context& context) -> Location;

				EVO_NODISCARD static auto get(sema::FuncAlias::ID func_alias_id, const class Context& context)
					-> Location;
				EVO_NODISCARD static auto get(const sema::FuncAlias& func_alias, const class Context& context)
					-> Location;

				EVO_NODISCARD static auto get(sema::TemplatedFunc::ID templated_func_id, const class Context& context)
					-> Location;
				EVO_NODISCARD static auto get(const sema::TemplatedFunc& templated_func, const class Context& context)
					-> Location;

				EVO_NODISCARD static auto get(BaseType::Alias::ID alias_id, const class Context& context)
					-> Location;
				EVO_NODISCARD static auto get(const BaseType::Alias& alias_type, const class Context& context)
					-> Location;

				EVO_NODISCARD static auto get(
					BaseType::DistinctAlias::ID distinct_alias_id, const class Context& context
				) -> Location;
				EVO_NODISCARD static auto get(
					const BaseType::DistinctAlias& distinct_alias_type, const class Context& context
				) -> Location;

				EVO_NODISCARD static auto get(BaseType::Struct::ID struct_id, const class Context& context)
					-> Location;
				EVO_NODISCARD static auto get(const BaseType::Struct& struct_type, const class Context& context)
					-> Location;

				EVO_NODISCARD static auto get(BaseType::Union::ID union_id, const class Context& context)
					-> Location;
				EVO_NODISCARD static auto get(const BaseType::Union& union_type, const class Context& context)
					-> Location;

				EVO_NODISCARD static auto get(BaseType::Enum::ID enum_id, const class Context& context)
					-> Location;
				EVO_NODISCARD static auto get(const BaseType::Enum& enum_type, const class Context& context)
					-> Location;

				EVO_NODISCARD static auto get(BaseType::Interface::ID interface_id, const class Context& context)
					-> Location;
				EVO_NODISCARD static auto get(const BaseType::Interface& interface_type, const class Context& context)
					-> Location;
		
			private:
				evo::Variant<None, Builtin, SourceLocation, ClangSourceLocation> variant;
		};

		

		struct Info{
			std::string message;
			Location location;
			evo::SmallVector<Info> subInfos;

			Info(std::string&& _message) : message(std::move(_message)), location(), subInfos() {
				// _debug_analyze_message(this->message);
			}
			Info(std::string&& _message, Location loc) 
				: message(std::move(_message)), location(loc), subInfos() {
				// _debug_analyze_message(this->message);
			}
			Info(std::string&& _message, Location loc, evo::SmallVector<Info>&& _subInfos)
				: message(std::move(_message)), location(loc), subInfos(std::move(_subInfos)) {
				// _debug_analyze_message(this->message);
			}
		};

		Level level;
		std::string message;
		Location location;
		evo::SmallVector<Info> infos;


		Diagnostic(
			Level _level,
			const std::string& _message,
			const Location& _location,
			const evo::SmallVector<Info>& _infos
		) : level(_level), message(_message), location(_location), infos(_infos) {
			_debug_analyze_message(this->message);
		}

		Diagnostic(
			Level _level,
			std::string&& _message,
			const Location& _location,
			const evo::SmallVector<Info>& _infos
		) : level(_level), message(std::move(_message)), location(_location), infos(_infos) {
			_debug_analyze_message(this->message);
		}

		Diagnostic(
			Level _level,
			const std::string& _message,
			const Location& _location,
			evo::SmallVector<Info>&& _infos = {}
		) : level(_level), message(_message), location(_location),infos(std::move(_infos)) {
			_debug_analyze_message(this->message);
		}

		Diagnostic(
			Level _level,
			std::string&& _message,
			const Location& _location,
			evo::SmallVector<Info>&& _infos = {}
		) : level(_level), message(std::move(_message)), location(_location), infos(std::move(_infos)) {
			_debug_analyze_message(this->message);
		}


		Diagnostic(
			Level _level,
			const std::string& _message,
			const Location& _location,
			const Info& info
		) : level(_level), message(_message), location(_location), infos{info} {
			_debug_analyze_message(this->message);
		}

		Diagnostic(
			Level _level,
			std::string&& _message,
			const Location& _location,
			const Info& info
		) : level(_level), message(std::move(_message)), location(_location), infos{info} {
			_debug_analyze_message(this->message);
		}

		Diagnostic(
			Level _level,
			const std::string& _message,
			const Location& _location,
			Info&& info
		) : level(_level), message(_message), location(_location), infos{std::move(info)} {
			_debug_analyze_message(this->message);
		}

		Diagnostic(
			Level _level,
			std::string&& _message,
			const Location& _location,
			Info&& info
		) : level(_level), message(std::move(_message)), location(_location), infos{std::move(info)} {
			_debug_analyze_message(this->message);
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
			evo::debugAssert(std::islower(int(message[0])) == false, "Diagnostic message cannot be lower-case");
		}


		private:
			friend class Location;
	};

}

template<>
struct std::formatter<pcit::panther::Diagnostic::Level> : std::formatter<std::string_view> {
    auto format(const pcit::panther::Diagnostic::Level& level, std::format_context& ctx) const
    -> std::format_context::iterator {
        return std::formatter<std::string_view>::format(pcit::panther::Diagnostic::printLevel(level), ctx);
    }
};