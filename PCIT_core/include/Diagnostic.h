////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


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
			std::vector<Info> sub_infos;

			Info(std::string&& _message) : message(std::move(_message)), location(), sub_infos() {};
			Info(std::string&& _message, std::optional<Location> loc) 
				: message(std::move(_message)), location(loc), sub_infos() {};
			Info(std::string&& _message, std::optional<Location> loc, std::vector<Info>&& _sub_infos)
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