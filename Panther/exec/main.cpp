//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

#include <Evo.h>

#include <Panther.h>
namespace panther = pcit::panther;

#include "./printing.h"


#if defined(EVO_PLATFORM_WINDOWS)
	#if !defined(WIN32_LEAN_AND_MEAN)
		#define WIN32_LEAN_AND_MEAN
	#endif

	#if !defined(NOCOMM)
		#define NOCOMM
	#endif

	#if !defined(NOMINMAX)
		#define NOMINMAX
	#endif

	#include <Windows.h>
#endif


struct Config{
	enum class Target{
		PrintTokens,
	} target;

	bool verbose;
	bool print_color;
	evo::uint max_threads;
};



auto main(int argc, const char* argv[]) -> int {
	auto args = std::vector<std::string_view>(argv, argv + argc);

	auto config = Config{
		.target      = Config::Target::PrintTokens,
		.verbose     = true,
		.print_color = pcit::core::Printer::platformSupportsColor() == pcit::core::Printer::DetectResult::Yes,

		.max_threads = panther::Context::optimalNumThreads(),
		// .max_threads = 0,
	};


	// print UTF-8 characters on windows
	#if defined(EVO_PLATFORM_WINDOWS)
		::SetConsoleOutputCP(CP_UTF8);
	#endif

	#if defined(PCIT_CONFIG_DEBUG)
		evo::log::setDefaultThreadSaferCallback();
	#endif

	auto printer = pcit::core::Printer(config.print_color);


	if(config.verbose){
		printer.printCyan("pthr (Panther Compiler)\n");
		printer.printGray("-----------------------\n");
		printer.printMagenta(std::format("v{}\n", pcit::core::version));

		switch(config.target){
			break; case Config::Target::PrintTokens: printer.printMagenta("Target: PrintTokens\n");
			break; default: evo::debugFatalBreak("Unknown or unsupported config target");
		};
	}


	const evo::uint num_threads = config.max_threads;

	auto context = panther::Context(panther::createDefaultDiagnosticCallback(printer), panther::Context::Config{
		.numThreads   = num_threads,
		.maxNumErrors = 1,
	});


	auto exit = [&]() noexcept -> void {
		if(context.isMultiThreaded() && context.threadsRunning()){
			context.shutdownThreads();
		}

		#if !defined(PCIT_BUILD_RELEASE_DIST) && defined(EVO_COMPILER_MSVC)
			printer.printGray("Press Enter to close...");
			std::cin.get();
		#endif
	};


	if(context.isMultiThreaded()){
		if(config.verbose){
			if(num_threads > 1){
				printer.printMagenta(std::format("Running multi-threaded ({} worker threads)\n", num_threads));
			}else{
				printer.printMagenta("Running multi-threaded (1 worker thread)\n");				
			}
		}

		context.startupThreads();
	}else{
		if(config.verbose){
			printer.printMagenta("Running single-threaded\n");
		}
	}


	///////////////////////////////////
	// load files

	context.loadFiles({
		"test.pthr",
		"test2.pthr",
	});

	if(context.isMultiThreaded()){
		context.waitForAllTasks();
	}

	if(context.errored()){
		if(config.verbose){ printer.printError("Encountered an error loading files\n"); }

		exit();
		return EXIT_FAILURE;
	}


	if(config.verbose){ printer.printSuccess("Successfully loaded all files\n"); }



	///////////////////////////////////
	// tokenize

	context.tokenizeLoadedFiles();

	if(context.isMultiThreaded()){
		context.waitForAllTasks();
	}

	if(context.errored()){
		if(config.verbose){ printer.printError("Encountered an error tokenizing files\n"); }

		exit();
		return EXIT_FAILURE;
	}


	if(config.verbose){ printer.printSuccess("Successfully tokenized all files\n"); }

	if(config.target == Config::Target::PrintTokens){
		const panther::SourceManager& source_manager = context.getSourceManager();

		for(panther::Source::ID source_id : source_manager){
			const panther::Source& source = source_manager.getSource(source_id);

			pthr::printTokens(printer, source);
		}

		exit();
		return EXIT_SUCCESS;
	}


	///////////////////////////////////
	// done

	evo::debugFatalBreak("Unknown or unsupported config target");
}