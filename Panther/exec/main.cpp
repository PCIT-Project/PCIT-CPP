////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include <iostream>

#include <Evo.h>
#include <Panther.h>
namespace core = pcit::core;
namespace panther = pcit::panther;

#include "./printing.h"


namespace pthr{

	struct Config{
		enum class Verbosity{
			None = 0,
			Some = 1,
			Full = 2,
		};

		Verbosity verbosity  = Verbosity::Full;
		bool print_color     = core::Printer::platformSupportsColor() == core::Printer::DetectResult::Yes;
		
		unsigned num_threads = panther::Context::optimalNumThreads();
		// unsigned num_threads = 0;

		bool use_std_lib     = true;
	};


	static auto setup_env(bool print_color) -> void {
		core::windows::setConsoleToUTF8Mode();

		#if defined(PCIT_CONFIG_DEBUG)
			evo::log::setDefaultThreadSaferCallback();
		#endif

		#if !defined(PCIT_BUILD_DIST) && defined(EVO_PLATFORM_WINDOWS)
			if(core::windows::isDebuggerPresent()){
				static auto at_exit_call = [print_color]() -> void {
					// not using printer because it should always go to stdout
					if(print_color){
						evo::printGray("Press [Enter] to close...");
					}else{
						evo::print("Press [Enter] to close...");
					}

					std::cin.get();
					evo::println();
				};
				std::atexit([]() -> void {
					at_exit_call();
				});
			}
		#endif
	}

}



static auto get_current_path(core::Printer& printer) -> evo::Result<std::filesystem::path> {
	std::error_code ec;
	const std::filesystem::path current_path = std::filesystem::current_path(ec);
	if(ec){
		printer.printlnError("Failed to get relative directory");
		printer.printlnError("\tcode: \"{}\"", ec.value());
		printer.printlnError("\tmessage: \"{}\"", ec.message());

		return evo::resultError;
	}

	return current_path;
}


auto main(int argc, const char* argv[]) -> int {
	auto args = std::vector<std::string_view>(argv, argv + argc);

	auto config = pthr::Config();
	pthr::setup_env(config.print_color);
	auto printer = core::Printer::createConsole(config.print_color);


	if(config.verbosity == pthr::Config::Verbosity::Full){
		pthr::print_logo(printer);

		#if defined(PCIT_BUILD_DEBUG)
			printer.printlnMagenta("v{} (debug)", pcit::core::version);
		#elif defined(PCIT_BUILD_DEV)
			printer.printlnMagenta("v{} (dev)", pcit::core::version);
		#elif defined(PCIT_BUILD_OPTIMIZE)
			printer.printlnMagenta("v{} (optimize)", pcit::core::version);
		#elif defined(PCIT_BUILD_RELEASE)
			printer.printlnMagenta("v{} (release)", pcit::core::version);
		#elif defined(PCIT_BUILD_DIST)
			printer.printlnMagenta("v{}", pcit::core::version);
		#else
			#error Unknown or unsupported build
		#endif

		if(config.num_threads == 0){
			printer.printlnMagenta("Running build single-threaded");
		}else if(config.num_threads == 1){
			printer.printlnMagenta("Running build multi-threaded (1 worker thread)");
		}else{
			printer.printlnMagenta("Running build multi-threaded ({} worker threads)", config.num_threads);
		}
	}

	using ContextConfig = panther::Context::Config;
	const auto context_config = ContextConfig{
		.mode         = ContextConfig::Mode::BuildSystem,
		.os           = core::getCurrentOS(),
		.architecture = core::getCurrentArchitecture(),

		.numThreads = config.num_threads,
	};

	const evo::Result<std::filesystem::path> current_path = get_current_path(printer);
	if(current_path.isError()){ return EXIT_FAILURE; }

	auto context = panther::Context(
		panther::createDefaultDiagnosticCallback(printer, current_path.value()), context_config
	);


	if(config.use_std_lib){
		std::ignore = context.addStdLib(current_path.value() / "../lib/std");
	}


	const panther::Source::CompilationConfig::ID comp_config = context.getSourceManager().createSourceCompilationConfig(
		panther::Source::CompilationConfig{
			.basePath = current_path.value(),
		}
	);

	std::ignore = context.addSourceFile("build.pthr", comp_config);


	if(context.analyzeSemantics() == false){
		const unsigned num_errors = context.getNumErrors();
		if(num_errors == 1){
			printer.printlnError("Failed with 1 error");
		}else{
			printer.printlnError("Failed with {} errors", context.getNumErrors());
		}
		return EXIT_FAILURE;
	}

	// for(const panther::Source::ID& source_id : context.getSourceManager()){
	// 	pthr::print_AST(printer, context.getSourceManager()[source_id], current_path.value());
	// }


	if(config.verbosity >= pthr::Config::Verbosity::Some){
		printer.printlnSuccess("Successfully completed");
	}


	return EXIT_SUCCESS;
}