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
namespace pir = pcit::pir;
#include <PLNK.h>
namespace plnk = pcit::plnk;

#include "./printing.h"


namespace pthr{

	struct CmdArgsConfig{
		enum class Verbosity{
			NONE = 0,
			SOME = 1,
			FULL = 2,
		};

		Verbosity verbosity  = Verbosity::FULL;
		panther::Context::NumThreads numBuildThreads = panther::Context::NumThreads::single();
		bool print_color     = core::Printer::platformSupportsColor() == core::Printer::DetectResult::YES;
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
		panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
			panther::Diagnostic::Level::ERROR,
			panther::Diagnostic::Code::FRONTEND_FAILED_TO_GET_REL_DIR,
			panther::Diagnostic::Location::NONE,
			"Failed to get relative directory",
			evo::SmallVector<panther::Diagnostic::Info>{
				panther::Diagnostic::Info(std::format("\tcode: \"{}\"", ec.value())),
				panther::Diagnostic::Info(std::format("\tmessage: \"{}\"", ec.message())),
			}
		));

		return evo::resultError;
	}

	return current_path;
}


static auto error_failed_to_add_std_lib(panther::Context::AddSourceResult add_std_lib_res, core::Printer& printer)
-> void {
	auto infos = evo::SmallVector<panther::Diagnostic::Info>();
	if(add_std_lib_res == panther::Context::AddSourceResult::DOESNT_EXIST){
		infos.emplace_back("Path doesn't exist");
	}else{
		infos.emplace_back("Path is not a directory");
	}

	panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
		panther::Diagnostic::Level::ERROR,
		panther::Diagnostic::Code::FRONTEND_FAILED_TO_ADD_STD_LIB,
		panther::Diagnostic::Location::NONE,
		"Failed to add Panther standard library",
		std::move(infos)
	));
}


static auto print_num_context_errors(const panther::Context& context, core::Printer& printer) -> void {
	const unsigned num_errors = context.getNumErrors();
	if(num_errors == 1){
		printer.printlnError("Failed with 1 error");
	}else{
		printer.printlnError("Failed with {} errors", context.getNumErrors());
	}
}


static auto run_build_system(const pthr::CmdArgsConfig& cmd_args_config, core::Printer& printer)
-> evo::Result<panther::Context::BuildSystemConfig> {
	using ContextConfig = panther::Context::Config;
	const auto context_config = ContextConfig{
		.mode   = ContextConfig::Mode::BUILD_SYSTEM,
		.title  = "<Panther-Build-System>",
		.target = core::Target::getCurrent(),

		.numThreads = cmd_args_config.numBuildThreads,
	};

	const evo::Result<std::filesystem::path> current_path = get_current_path(printer);
	if(current_path.isError()){ return evo::resultError; }

	if(cmd_args_config.verbosity == pthr::CmdArgsConfig::Verbosity::FULL){
		printer.printlnMagenta("Build system relative directory: \"{}\"", current_path.value().string());
	}

	auto context = panther::Context(
		panther::createDefaultDiagnosticCallback(printer, current_path.value()), context_config
	);


	if(cmd_args_config.use_std_lib){
		const panther::Context::AddSourceResult add_std_lib_res = 
			context.addStdLib(current_path.value() / "../extern/Panther-std/std");

		if(add_std_lib_res != panther::Context::AddSourceResult::SUCCESS){
			error_failed_to_add_std_lib(add_std_lib_res, printer);
			return evo::resultError;
		}
	}


	const panther::Source::CompilationConfig::ID comp_config = context.getSourceManager().createSourceCompilationConfig(
		panther::Source::CompilationConfig{
			.basePath = current_path.value(),
			.warn = panther::Source::CompilationConfig::Warns{
				.methodCallOnNonMethod = true,
			},
		}
	);

	std::ignore = context.addSourceFile("build.pthr", comp_config);


	if(context.analyzeSemantics().isError()){
		print_num_context_errors(context, printer);
		return evo::resultError;
	}

	const evo::Result<uint8_t> entry_res = context.runEntry();
	if(entry_res.isError()){ return evo::resultError; }

	if(entry_res.value() != 0){
		panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
			panther::Diagnostic::Level::ERROR,
			panther::Diagnostic::Code::FRONTEND_BUILD_SYSTEM_RETURNED_ERROR,
			panther::Diagnostic::Location::NONE,
			std::format("Build system errored with code: {}", entry_res.value())
		));
		return evo::resultError;
	}

	return context.getBuildSystemConfig();
}






EVO_NODISCARD static auto run_compile(
	const pthr::CmdArgsConfig& cmd_args_config,
	const panther::Context::BuildSystemConfig& config,
	core::Printer& printer
) -> evo::Result<> {
	if(cmd_args_config.verbosity == pthr::CmdArgsConfig::Verbosity::FULL){
		if(config.numThreads.isSingle()){
			printer.printlnMagenta("Running compile single-threaded");
		}else if(config.numThreads.getNum() == 1){
			printer.printlnMagenta("Running compile multi-threaded (1 worker thread)");
		}else{
			printer.printlnMagenta(
				"Running compile multi-threaded ({} worker threads)", config.numThreads.getNum()
			);
		}
	}



	using ContextConfig = panther::Context::Config;
	const auto context_config = ContextConfig{
		.mode       = ContextConfig::Mode::COMPILE,
		.title      = "Panther Testing",
		.target     = core::Target::getCurrent(),
		.numThreads = config.numThreads,
	};

	const evo::Result<std::filesystem::path> current_path = get_current_path(printer);
	if(current_path.isError()){ return evo::resultError; }

	if(cmd_args_config.verbosity == pthr::CmdArgsConfig::Verbosity::FULL){
		printer.printlnMagenta("Compile relative directory: \"{}\"", current_path.value().string());
	}

	auto context = panther::Context(
		panther::createDefaultDiagnosticCallback(printer, current_path.value()), context_config
	);


	using BuildSystemConfig = panther::Context::BuildSystemConfig;


	if(
		config.useStdLib 
		&& config.output != BuildSystemConfig::Output::TOKENS
		&& config.output != BuildSystemConfig::Output::AST
	){
		const panther::Context::AddSourceResult add_std_lib_res = 
			context.addStdLib(current_path.value() / "../extern/Panther-std/std");

		if(add_std_lib_res != panther::Context::AddSourceResult::SUCCESS){
			error_failed_to_add_std_lib(add_std_lib_res, printer);
			return evo::resultError;
		}
	}


	const panther::Source::CompilationConfig::ID comp_config = context.getSourceManager().createSourceCompilationConfig(
		panther::Source::CompilationConfig{
			.basePath = current_path.value(),
			.warn = panther::Source::CompilationConfig::Warns{
				.methodCallOnNonMethod = true,
			},
		}
	);

	std::ignore = context.addSourceFile("test.pthr", comp_config);
	// std::ignore = context.addCPPHeaderFile("test.h", true);


	switch(config.output){
		case BuildSystemConfig::Output::TOKENS: {
			if(context.tokenize().isError()){
				print_num_context_errors(context, printer);
				return evo::resultError;
			}

			for(const panther::Source::ID source_id : context.getSourceManager().getSourceIDRange()){
				pthr::print_tokens(printer, context.getSourceManager()[source_id], current_path.value());
			}

			return evo::Result<>();
		} break;

		case BuildSystemConfig::Output::AST: {
			if(context.parse().isError()){
				print_num_context_errors(context, printer);
				return evo::resultError;
			}

			for(const panther::Source::ID source_id : context.getSourceManager().getSourceIDRange()){
				pthr::print_AST(printer, context.getSourceManager()[source_id], current_path.value());
			}

			return evo::Result<>();
		} break;

		case BuildSystemConfig::Output::BUILD_SYMBOL_PROCS: {
			if(context.buildSymbolProcs().isError()){
				print_num_context_errors(context, printer);
				return evo::resultError;
			}

			return evo::Result<>();
		} break;

		case BuildSystemConfig::Output::SEMANTIC_ANALYSIS: {
			if(context.analyzeSemantics().isError()){
				print_num_context_errors(context, printer);
				return evo::resultError;
			}

			return evo::Result<>();
		} break;

		case BuildSystemConfig::Output::PIR: {
			if(context.analyzeSemantics().isError()){
				print_num_context_errors(context, printer);
				return evo::resultError;
			}

			auto module = pir::Module(evo::copy(context.getConfig().title), context.getConfig().target);
			if(context.lowerToPIR(panther::Context::EntryKind::NONE, module).isError()){ return evo::resultError; }
			pir::printModule(module, printer);

			return evo::Result<>();
		} break;

		case BuildSystemConfig::Output::LLVMIR: {
			if(context.analyzeSemantics().isError()){
				print_num_context_errors(context, printer);
				return evo::resultError;
			}

			auto module = pir::Module(evo::copy(context.getConfig().title), context.getConfig().target);
			if(context.lowerToPIR(panther::Context::EntryKind::NONE, module).isError()){ return evo::resultError; }

			const evo::Result<std::string> llvmir_string = context.lowerToLLVMIR(module);
			if(llvmir_string.isError()){ return evo::resultError; }

			printer.print(llvmir_string.value());

			return evo::Result<>();
		} break;

		case BuildSystemConfig::Output::ASSEMBLY: {
			if(context.analyzeSemantics().isError()){
				print_num_context_errors(context, printer);
				return evo::resultError;
			}

			auto module = pir::Module(evo::copy(context.getConfig().title), context.getConfig().target);
			if(context.lowerToPIR(panther::Context::EntryKind::NONE, module).isError()){ return evo::resultError; }

			const evo::Result<std::string> asm_result = context.lowerToAssembly(module);
			if(asm_result.isError()){
				panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
					panther::Diagnostic::Level::ERROR,
					panther::Diagnostic::Code::FRONTEND_FAILED_TO_OUTPUT_ASM,
					panther::Diagnostic::Location::NONE,
					"Failed to output assembly code"
				));
				return evo::resultError;
			}

			printer.print(asm_result.value());

			return evo::Result<>();
		} break;

		case BuildSystemConfig::Output::OBJECT: {
			if(context.analyzeSemantics().isError()){
				print_num_context_errors(context, printer);
				return evo::resultError;
			}

			auto module = pir::Module(evo::copy(context.getConfig().title), context.getConfig().target);
			if(context.lowerToPIR(panther::Context::EntryKind::NONE, module).isError()){ return evo::resultError; }

			const evo::Result<std::vector<evo::byte>> object_data = context.lowerToObject(module);
			if(object_data.isError()){
				panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
					panther::Diagnostic::Level::ERROR,
					panther::Diagnostic::Code::FRONTEND_FAILED_TO_OUTPUT_OBJ,
					panther::Diagnostic::Location::NONE,
					"Failed to output object file"
				));
				return evo::resultError;
			}

			if(evo::fs::writeBinaryFile("build/output.o", object_data.value()) == false){
				panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
					panther::Diagnostic::Level::ERROR,
					panther::Diagnostic::Code::FRONTEND_FAILED_TO_OUTPUT_OBJ,
					panther::Diagnostic::Location::NONE,
					"Failed to output object file",
					panther::Diagnostic::Info("Failed to write file")
				));
				return evo::resultError;
			}

			return evo::Result<>();
		} break;

		case BuildSystemConfig::Output::RUN: {
			if(context.analyzeSemantics().isError()){
				print_num_context_errors(context, printer);
				return evo::resultError;
			}

			const evo::Result<uint8_t> entry_res = context.runEntry(true);
			if(entry_res.isError()){ return evo::resultError; }
			printer.printlnSuccess("Value returned from entry: {}", entry_res.value());
			return evo::Result<>();
		} break;

		case BuildSystemConfig::Output::CONSOLE_EXECUTABLE: case BuildSystemConfig::Output::WINDOWED_EXECUTABLE: {
			if(context.analyzeSemantics().isError()){
				print_num_context_errors(context, printer);
				return evo::resultError;
			}

			auto module = pir::Module(evo::copy(context.getConfig().title), context.getConfig().target);

			const panther::Context::EntryKind entry_kind = [&](){
				if(config.output == BuildSystemConfig::Output::CONSOLE_EXECUTABLE){
					return panther::Context::EntryKind::CONSOLE_EXECUTABLE;
				}else{
					return panther::Context::EntryKind::WINDOWED_EXECUTABLE;
				}
			}();

			if(context.lowerToPIR(entry_kind, module).isError()){
				return evo::resultError;
			}

			const evo::Result<std::vector<evo::byte>> object_data = context.lowerToObject(module);
			if(object_data.isError()){
				panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
					panther::Diagnostic::Level::ERROR,
					panther::Diagnostic::Code::FRONTEND_FAILED_TO_OUTPUT_OBJ,
					panther::Diagnostic::Location::NONE,
					"Failed to output object file"
				));
				return evo::resultError;
			}

			{
				std::error_code ec;
				std::filesystem::create_directories(std::filesystem::path("build"), ec);
				if(ec){
					panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
						panther::Diagnostic::Level::ERROR,
						panther::Diagnostic::Code::FRONTEND_FAILED_TO_GET_REL_DIR,
						panther::Diagnostic::Location::NONE,
						"Failed to create directory \"build\"",
						evo::SmallVector<panther::Diagnostic::Info>{
							panther::Diagnostic::Info(std::format("\tcode: \"{}\"", ec.value())),
							panther::Diagnostic::Info(std::format("\tmessage: \"{}\"", ec.message())),
						}
					));

					return evo::resultError;
				}
			}

			if(evo::fs::writeBinaryFile("build/output.o", object_data.value()) == false){
				panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
					panther::Diagnostic::Level::ERROR,
					panther::Diagnostic::Code::FRONTEND_FAILED_TO_OUTPUT_OBJ,
					panther::Diagnostic::Location::NONE,
					"Failed to output obj",
					panther::Diagnostic::Info("Failed to write file")
				));
				return evo::resultError;
			}

			auto plnk_options = plnk::Options(plnk::Target::WINDOWS);
			plnk_options.outputFilePath = "build/output.exe";
			plnk_options.getWindowsSpecific().subsystem = [&](){
				if(config.output == BuildSystemConfig::Output::CONSOLE_EXECUTABLE){
					return plnk::Options::WindowsSpecific::Subsystem::CONSOLE;
				}else{
					return plnk::Options::WindowsSpecific::Subsystem::WINDOWS;
				}
			}();

			const plnk::LinkResult link_result = plnk::link({std::filesystem::path("build/output.o")}, plnk_options);

			if(link_result.messages.empty() == false){
				printer.printlnCyan("<Info:L> Linker messages");
				for(const std::string& message : link_result.messages){
					printer.printCyan(message);
				}
			}

			if(link_result.errMessages.empty() == false){
				// OLD PRINTING VERSION:
				// if(link_result.errMessages.size() == 1){
				// 	printer.printlnRed("<Error:L> Linking failed with 1 error:");
				// }else{
				// 	printer.printlnRed("<Error:L> Linking failed with {} errors:", link_result.errMessages.size());
				// }
				// for(const std::string& err_message : link_result.errMessages){
				// 	printer.printCyan(err_message);
				// }

				for(const std::string& err_message : link_result.errMessages){
					printer.printRed("<Error:L> ");
					auto output_buffer = std::string();

					bool printed_error = false;

					for(size_t i = 0; char character : err_message){
						if(character != '\n'){
							output_buffer += character;

						}else if(i == 0){
							// skip if the first character is a newline

						}else{
							if(printed_error == false){
								printed_error = true;
								printer.printlnRed(output_buffer);

							}else{
								printer.printlnCyan(output_buffer);
							}

							output_buffer.clear();

							if(i + 1 < err_message.size()){
								printer.print("\t");
							}
						}

						i += 1;
					}

					if(output_buffer.empty() == false){
						if(printed_error == false){
							printer.printlnRed(output_buffer);

						}else{
							printer.printlnCyan(output_buffer);
						}
					}
				}

				return evo::resultError;
			}

			return evo::Result<>();
		} break;
	}

	context.emitFatal(
		panther::Diagnostic::Code::MISC_UNKNOWN_ERROR,
		panther::Diagnostic::Location::NONE,
		"Unknown compile output recieved"
	);
	return evo::resultError;
}







auto main(int argc, const char* argv[]) -> int {
	auto args = std::vector<std::string_view>(argv, argv + argc);

	auto cmd_args_config = pthr::CmdArgsConfig();
	pthr::setup_env(cmd_args_config.print_color);
	auto printer = core::Printer::createConsole(cmd_args_config.print_color);


	if(cmd_args_config.verbosity >= pthr::CmdArgsConfig::Verbosity::SOME){
		pthr::print_logo(printer);

		#if defined(PCIT_BUILD_DEBUG)
			printer.printlnMagenta("v{} (debug)", pcit::core::version);
		#elif defined(PCIT_BUILD_OPTIMIZE)
			printer.printlnMagenta("v{} (optimize)", pcit::core::version);
		#elif defined(PCIT_BUILD_RELEASE)
			printer.printlnMagenta("v{} (release)", pcit::core::version);
		#elif defined(PCIT_BUILD_DIST)
			printer.printlnMagenta("v{}", pcit::core::version);
		#else
			#error Unknown or unsupported build
		#endif
	}

	if(cmd_args_config.verbosity == pthr::CmdArgsConfig::Verbosity::FULL){
		if(cmd_args_config.numBuildThreads.isSingle()){
			printer.printlnMagenta("Running build system single-threaded");
		}else if(cmd_args_config.numBuildThreads.getNum() == 1){
			printer.printlnMagenta("Running build system multi-threaded (1 worker thread)");
		}else{
			printer.printlnMagenta(
				"Running build system multi-threaded ({} worker threads)", cmd_args_config.numBuildThreads.getNum()
			);
		}
	}

	const evo::Result<panther::Context::BuildSystemConfig> build_system_run = 
		run_build_system(cmd_args_config, printer);
	if(build_system_run.isError()){ return EXIT_FAILURE; }


	if(run_compile(cmd_args_config, build_system_run.value(), printer).isError()){ return EXIT_FAILURE; }


	if(cmd_args_config.verbosity >= pthr::CmdArgsConfig::Verbosity::SOME){
		printer.printlnSuccess("Successfully completed");
	}

	return EXIT_SUCCESS;
}