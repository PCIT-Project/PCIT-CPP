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
		PrintAST,
		Parse,
	} target;

	bool verbose;
	bool print_color;
	evo::uint max_threads;

	evo::uint max_num_errors = 1;
};



auto main(int argc, const char* argv[]) -> int {
	auto args = evo::SmallVector<std::string_view>(argv, argv + argc);

	auto config = Config{
		.target      = Config::Target::PrintAST,
		.verbose     = true,
		.print_color = pcit::core::Printer::platformSupportsColor() == pcit::core::Printer::DetectResult::Yes,

		// .max_threads = panther::Context::optimalNumThreads(),
		.max_threads = 0,

		.max_num_errors = 2,
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
		printer.printlnCyan("pthr (Panther Compiler)");
		printer.printlnGray("-----------------------");

		#if defined(PCIT_BUILD_DEBUG)
			printer.printlnMagenta(std::format("v{} (debug)", pcit::core::version));
		#elif defined(PCIT_BUILD_DEV)
			printer.printlnMagenta(std::format("v{} (dev)", pcit::core::version));
		#elif defined(PCIT_BUILD_OPTIMIZE)
			printer.printlnMagenta(std::format("v{} (optimize)", pcit::core::version));
		#elif defined(PCIT_BUILD_RELEASE)
			printer.printlnMagenta(std::format("v{} (release)", pcit::core::version));
		#elif defined(PCIT_BUILD_DIST)
			printer.printlnMagenta(std::format("v{}", pcit::core::version));
		#else
			#error Unknown or unsupported build
		#endif

		switch(config.target){
			break; case Config::Target::PrintTokens: printer.printlnMagenta("Target: PrintTokens");
			break; case Config::Target::PrintAST:    printer.printlnMagenta("Target: PrintAST");
			break; case Config::Target::Parse:    printer.printlnMagenta("Target: Parse");
			break; default: evo::debugFatalBreak("Unknown or unsupported config target (cannot print target)");
		};
	}


	const evo::uint num_threads = config.max_threads;

	auto context = panther::Context(panther::createDefaultDiagnosticCallback(printer), panther::Context::Config{
		.numThreads   = num_threads,
		.maxNumErrors = config.max_num_errors,
	});


	auto exit = [&]() noexcept -> void {
		if(context.isMultiThreaded() && context.threadsRunning()){
			context.shutdownThreads();
		}

		#if !defined(PCIT_BUILD_DIST) && defined(EVO_COMPILER_MSVC)
			printer.printGray("Press Enter to close...");
			std::cin.get();
		#endif
	};


	if(context.isMultiThreaded()){
		if(config.verbose){
			if(num_threads > 1){
				printer.printlnMagenta(std::format("Running multi-threaded ({} worker threads)", num_threads));
			}else{
				printer.printlnMagenta("Running multi-threaded (1 worker thread)");				
			}
		}

		context.startupThreads();
	}else{
		if(config.verbose){
			printer.printlnMagenta("Running single-threaded");
		}
	}


	///////////////////////////////////
	// load files

	context.loadFiles({
		"test.pthr",
		// "test2.pthr",
		
		// "./local/big_test.pthr",
		// "./local/big_test_with_params.pthr",
	});

	if(context.isMultiThreaded()){
		context.waitForAllTasks();
	}

	if(context.errored()){
		if(config.verbose){ printer.printlnError("Encountered an error loading files"); }

		exit();
		return EXIT_FAILURE;
	}


	if(config.verbose){ printer.printlnSuccess("Successfully loaded all files"); }



	///////////////////////////////////
	// tokenize

	const auto tokenize_start = evo::time::now();

	context.tokenizeLoadedFiles();

	if(context.isMultiThreaded()){
		context.waitForAllTasks();
	}

	if(context.errored()){
		if(config.verbose){ printer.printlnError("Encountered an error tokenizing files"); }

		exit();
		return EXIT_FAILURE;
	}


	if(config.verbose){ printer.printlnSuccess("Successfully tokenized all files"); }

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
	// parse

	context.parseLoadedFiles();

	if(context.isMultiThreaded()){
		context.waitForAllTasks();
	}

	if(context.errored()){
		if(config.verbose){ printer.printlnError("Encountered an error parsing files"); }

		exit();
		return EXIT_FAILURE;
	}


	if(config.verbose){ printer.printlnSuccess("Successfully parsed all files"); }

	if(config.target == Config::Target::PrintAST){
		const panther::SourceManager& source_manager = context.getSourceManager();

		for(panther::Source::ID source_id : source_manager){
			const panther::Source& source = source_manager.getSource(source_id);

			pthr::printAST(printer, source);
		}

		exit();
		return EXIT_SUCCESS;

	}else if(config.target == Config::Target::Parse){
		printer.printlnInfo(
			"Completed tokenizing and parsing in {}ms",
			static_cast<evo::time::Milliseconds>(evo::time::now() - tokenize_start)
		);

		exit();
		return EXIT_SUCCESS;
	}



	///////////////////////////////////
	// done

	evo::debugFatalBreak("Unknown or unsupported config target");
}