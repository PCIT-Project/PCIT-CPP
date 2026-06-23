////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <filesystem>

#include <Evo.hpp>
#include <PCIT_core.hpp>
#include <Panther.hpp>
namespace core = pcit::core;
namespace panther = pcit::panther;


namespace pthr{

	struct CmdArgsConfig{
		enum class Action{
			BUILD,
			SCRIPT,
			HELP,
			VERSION,
		};

		enum class Verbosity{
			NONE = 0,
			SOME = 1,
			FULL = 2,
		};

		std::filesystem::path executablePath;

		Action action                          = Action::BUILD;
		std::optional<std::string_view> file   = std::nullopt;
		Verbosity verbosity                    = Verbosity::NONE;
		std::filesystem::path workingDirectory = {};
		bool print_color = core::Printer::platformSupportsColor() == core::Printer::DetectResult::YES;

		panther::Context::NumThreads numBuildThreads = panther::Context::NumThreads::single();
		bool use_std_lib                             = true;

		auto printError(std::string_view message) -> void {
			auto printer = core::Printer::createConsole(this->print_color);
			panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
				panther::Diagnostic::Level::ERROR,
				std::string(message),
				panther::Diagnostic::Location::NONE,
				panther::Diagnostic::Info("`pthr help` for help")
			));
		}

		template<class... Args>
		auto printError(std::format_string<Args...> fmt, Args&&... args) noexcept -> void {
			this->printError(std::format(fmt, std::forward<decltype(args)>(args)...));
		};
	};


	auto print_help(core::Printer& printer) -> void;

	[[nodiscard]] auto parse_args(std::string_view action, evo::ArrayProxy<std::string_view> args)
		-> evo::Result<pthr::CmdArgsConfig>;
	
}