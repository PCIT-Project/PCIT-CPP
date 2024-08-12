//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>

#include <source_location>

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

			Info(std::string&& _message) : message(std::move(_message)), location() {};
			Info(std::string&& _message, Location loc) : message(std::move(_message)), location(loc) {};

			Info(const Info& rhs) : message(rhs.message), location(rhs.location) {};

			auto operator=(const Info& rhs) -> Info& {
				this->message = rhs.message;
				this->location = rhs.location;
				
				return *this;
			}

			Info(Info&& rhs) : message(std::move(rhs.message)), location(rhs.location) {};
		};

		DiagnosticLevel level;
		CodeEnum code;
		std::optional<Location> location;
		std::string message;
		evo::SmallVector<Info> infos;


		// TODO: create the rest of the overloads
		DiagnosticImpl(
			DiagnosticLevel _level,
			CodeEnum _code,
			const std::optional<Location>& _location,
			const std::string& _message,
			const evo::SmallVector<Info>& _infos = {}
		) : level(_level), code(_code), location(_location), message(_message), infos(_infos) {}

		DiagnosticImpl(
			DiagnosticLevel _level,
			CodeEnum _code,
			std::optional<Location>&& _location,
			std::string&& _message,
			evo::SmallVector<Info>&& _infos = {}
		) :
			level(_level),
			code(_code),
			location(std::move(_location)),
			message(std::move(_message)),
			infos(std::move(_infos)) 
		{}


		DiagnosticImpl(const DiagnosticImpl&) = default;
		DiagnosticImpl(DiagnosticImpl&&) = default;



		static auto createFatalMessage(
			std::string_view msg, std::source_location source_location = std::source_location::current()
		) -> std::string {
			return std::format(
				"{} (error location: {} | {})", msg, source_location.function_name(), source_location.line()
			);
		}
	};


	EVO_NODISCARD inline auto printDiagnosticLevel(DiagnosticLevel level) -> std::string_view {
		switch(level){
			break; case DiagnosticLevel::Fatal:   return "Fatal";
			break; case DiagnosticLevel::Error:   return "Error";
			break; case DiagnosticLevel::Warning: return "Warning";
		}

		evo::debugFatalBreak("Unknown or unsupported pcit::core::DiagnosticLevel");
	}

}


template<>
struct std::formatter<pcit::core::DiagnosticLevel> : std::formatter<std::string_view> {
    auto format(const pcit::core::DiagnosticLevel& level, std::format_context& ctx) const
    -> std::format_context::iterator {
        return std::formatter<std::string_view>::format(pcit::core::printDiagnosticLevel(level), ctx);
    }
};