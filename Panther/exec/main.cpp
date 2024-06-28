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


struct Config{
	bool verbose;
	bool print_color;
	evo::uint max_threads;
};



auto main(int argc, const char* argv[]) -> int {
	auto args = std::vector<std::string_view>(argv, argv + argc);

	auto config = Config{
		.verbose     = true,
		.print_color = pcit::core::Printer::platformSupportsColor() == pcit::core::Printer::DetectResult::Yes,

		.max_threads = panther::Context::optimalNumThreads(),
		// .max_threads = 0,
	};


	#if defined(PCIT_CONFIG_DEBUG)
		evo::log::setDefaultThreadSaferCallback();
	#endif

	auto printer = pcit::core::Printer(config.print_color);


	if(config.verbose){
		printer.printCyan("pthr (Panther Compiler)\n");
		printer.printGray("-----------------------\n");
		printer.printMagenta(std::format("v{}\n", pcit::core::version));
	}


	const evo::uint num_threads = config.max_threads;

	auto context = panther::Context(panther::createDefaultDiagnosticCallback(printer), panther::Context::Config{
		.numThreads   = num_threads,
		.maxNumErrors = 0,
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

	context.loadFiles({
		"test.pthr",
		"test2.pthr",
		// "asdf",
	});

	if(context.isMultiThreaded()){
		context.waitForAllTasks();
	}

	if(context.errored()){
		if(config.verbose){ printer.printError("Encountered an error\n"); }

		exit();
		return EXIT_FAILURE;
	}


	if(config.verbose){ printer.printSuccess("Success\n"); }


	
	exit();
	return EXIT_SUCCESS;
}