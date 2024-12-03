////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


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


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


struct Config{
	enum class Target{
		Run,
		LLVMIR,      // not guaranteed to remain
		PrintLLVMIR, // not guaranteed to remain
		PrintPIR,
		SemanticAnalysis,
		Parse,
		PrintAST,
		PrintTokens,
	} target;

	bool verbose;
	bool print_color;

	fs::path relative_dir{};
	bool relative_dir_set = false;

	bool add_source_locations = true;
	bool checked_math = true;

	unsigned max_threads    = 0;
	unsigned max_num_errors = std::numeric_limits<unsigned>::max();
	bool may_recover        = true;
};


auto main(int argc, const char* argv[]) -> int {
	auto args = evo::SmallVector<std::string_view>(argv, argv + argc);

	auto config = Config{
		.target      = Config::Target::Run,
		.verbose     = true,
		.print_color = pcit::core::Printer::platformSupportsColor() == pcit::core::Printer::DetectResult::Yes,

		// .add_source_locations = false,
		// .checked_math = false,

		// .max_threads    = panther::Context::optimalNumThreads(),
		.max_num_errors = 10,
		// .may_recover    = false,
	};

	// print UTF-8 characters on windows
	#if defined(EVO_PLATFORM_WINDOWS)
		::SetConsoleOutputCP(CP_UTF8);
	#endif

	#if defined(PCIT_CONFIG_DEBUG)
		evo::log::setDefaultThreadSaferCallback();
	#endif

	auto printer = pcit::core::Printer::createConsole(config.print_color);


	if(config.verbose){
		pthr::printTitle(printer);

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

		switch(config.target){
			break; case Config::Target::PrintTokens:      printer.printlnMagenta("Target: PrintTokens");
			break; case Config::Target::PrintAST:         printer.printlnMagenta("Target: PrintAST");
			break; case Config::Target::Parse:            printer.printlnMagenta("Target: Parse");
			break; case Config::Target::SemanticAnalysis: printer.printlnMagenta("Target: SemanticAnalysis");
			break; case Config::Target::PrintPIR:         printer.printlnMagenta("Target: PrintPIR");
			break; case Config::Target::PrintLLVMIR:      printer.printlnMagenta("Target: PrintLLVMIR");
			break; case Config::Target::LLVMIR:           printer.printlnMagenta("Target: LLVMIR");
			break; case Config::Target::Run:              printer.printlnMagenta("Target: Run");
			break; default: evo::debugFatalBreak("Unknown or unsupported config target (cannot print target)");
		}

	}


	const unsigned num_threads = config.max_threads;

	if(config.relative_dir_set == false){
		config.relative_dir_set = true;

		std::error_code ec;
		config.relative_dir = fs::current_path(ec);
		if(ec){
			printer.printlnError("Failed to get relative directory");
			printer.printlnError("\tcode: \"{}\"", ec.value());
			printer.printlnError("\tmessage: \"{}\"", ec.message());

			return EXIT_FAILURE;
		}
	}


	if(config.verbose){
		printer.printlnMagenta("Relative Directory: \"{}\"", config.relative_dir.string());
	}

	auto context = panther::Context(
		printer,
		panther::createDefaultDiagnosticCallback(printer),
		panther::Context::Config{
			.basePath     = config.relative_dir,

			.addSourceLocations = config.add_source_locations,
			.checkedMath        = config.checked_math,

			.numThreads   = num_threads,
			.maxNumErrors = config.max_num_errors,
			.mayRecover   = config.may_recover,
		}
	);


	const auto exit_defer = evo::Defer([&]() -> void {
		if(context.isMultiThreaded() && context.threadsRunning()){
			context.shutdownThreads();
		}
	});


	#if !defined(PCIT_BUILD_DIST) && defined(EVO_PLATFORM_WINDOWS)
		if(::IsDebuggerPresent()){
			static auto at_exit_call = [&]() -> void {
				// not using printer because it should always go to stdout
				if(config.print_color){
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
		config.relative_dir / "test.pthr",
		// config.relative_dir / "test2.pthr",
		
		// "./local/big_test.pthr",
		// "./local/big_test_with_params.pthr",
	});

	if(context.isMultiThreaded()){
		context.waitForAllTasks();
	}

	if(context.errored()){
		if(config.verbose){ printer.printlnError("Encountered an error loading files"); }

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

		return EXIT_FAILURE;
	}


	if(config.verbose){ printer.printlnSuccess("Successfully tokenized all files"); }

	if(config.target == Config::Target::PrintTokens){
		const panther::SourceManager& source_manager = context.getSourceManager();

		for(panther::Source::ID source_id : source_manager){
			const panther::Source& source = source_manager.getSource(source_id);

			pthr::printTokens(printer, source);
		}

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

		return EXIT_FAILURE;
	}


	if(config.verbose){ printer.printlnSuccess("Successfully parsed all files"); }

	if(config.target == Config::Target::PrintAST){
		const panther::SourceManager& source_manager = context.getSourceManager();

		for(panther::Source::ID source_id : source_manager){
			const panther::Source& source = source_manager.getSource(source_id);

			pthr::printAST(printer, source);
		}

		return EXIT_SUCCESS;

	}else if(config.target == Config::Target::Parse){
		if(config.verbose){
			const evo::time::Nanoseconds time_diff = evo::time::now() - tokenize_start;

			if(time_diff < static_cast<evo::time::Nanoseconds>(evo::time::Milliseconds(10))){
				printer.printlnInfo(
					"Completed tokenizing and parsing in {:.3}ms",
					static_cast<float32_t>(time_diff) / 10e6
				);
			}else{
				printer.printlnInfo(
					"Completed tokenizing and parsing in {}ms",
					static_cast<evo::time::Milliseconds>(time_diff)
				);
			}
		}

		return EXIT_SUCCESS;
	}


	///////////////////////////////////
	// semantic analysis

	context.semanticAnalysisLoadedFiles();

	if(context.isMultiThreaded()){
		context.waitForAllTasks();
	}

	if(context.errored()){
		if(config.verbose){ printer.printlnError("Encountered an error doing semantic analysis"); }

		return EXIT_FAILURE;
	}

	if(config.verbose){ printer.printlnSuccess("Successfully analyzed semantics all files"); }


	if(config.target == Config::Target::SemanticAnalysis){

		return EXIT_SUCCESS;
	}



	///////////////////////////////////
	// print PIR

	if(config.target == Config::Target::PrintPIR){
		if(context.printPIR() == false){ return EXIT_FAILURE; }

		return EXIT_SUCCESS;
	}


	///////////////////////////////////
	// print llvmir

	if(config.target == Config::Target::PrintLLVMIR){
		const evo::Result<std::string> llvm_ir = context.printLLVMIR(false);

		if(llvm_ir.isError()){
			return EXIT_FAILURE;			
		}

		printer.printlnCyan(llvm_ir.value());

		return EXIT_SUCCESS;

	}else if(config.target == Config::Target::LLVMIR){
		const evo::Result<std::string> llvm_ir = context.printLLVMIR(true);

		if(llvm_ir.isError()){
			return EXIT_FAILURE;			
		}

		if(evo::fs::writeFile("a.ll", llvm_ir.value()) == false){
			printer.printlnError("Failed to write file: \"a.ll\"");

		}else if(config.verbose){
			printer.printlnSuccess("Successfully write LLVMIR file \"a.ll\"");
		}

		return EXIT_SUCCESS;

	}else if(config.target == Config::Target::Run){
		if(config.verbose){ printer.printlnGray("------------------------------\nRunning:"); }

		const evo::Result<uint8_t> run_result = context.run();

		if(run_result.isError()){
			return EXIT_FAILURE;			
		}

		if(config.verbose){ printer.printlnInfo("Return Code: {}", run_result.value()); }

		return EXIT_SUCCESS;
	}



	///////////////////////////////////
	// done

	evo::debugFatalBreak("Unknown or unsupported config target");
}