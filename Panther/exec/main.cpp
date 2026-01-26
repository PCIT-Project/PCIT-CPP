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
		std::filesystem::path workingDirectory{};
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



using CreatePantherPackageResult =
	evo::Expected<panther::Source::Package::ID, panther::SourceManager::CreatePackageFailReason>;

static auto error_failed_to_add_std_lib(
	panther::SourceManager::CreatePackageFailReason fail_reason, core::Printer& printer
) -> void {
	auto infos = evo::SmallVector<panther::Diagnostic::Info>();
	switch(fail_reason){
		case panther::SourceManager::CreatePackageFailReason::PATH_NOT_ABSOLUTE: {
			infos.emplace_back("Path isn't absolute");
		} break;

		case panther::SourceManager::CreatePackageFailReason::PATH_DOESNT_EXIST: {
			infos.emplace_back("Path doesn't exist");
		} break;

		case panther::SourceManager::CreatePackageFailReason::PATH_NOT_DIRECTORY: {
			infos.emplace_back("Path isn't directory");
		} break;
	}

	panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
		panther::Diagnostic::Level::ERROR,
		panther::Diagnostic::Code::FRONTEND_FAILED_TO_ADD_STD_LIB,
		panther::Diagnostic::Location::NONE,
		"Failed to add Panther standard library",
		std::move(infos)
	));
}


static auto print_num_errors(unsigned num_errors, core::Printer& printer) -> void {
	evo::debugAssert(num_errors > 0, "Should only call this if no errors occured");

	if(num_errors == 1){
		printer.printlnError("Failed with 1 error");
	}else{
		printer.printlnError("Failed with {} errors", num_errors);
	}
}

static auto print_num_context_errors(const panther::Context& context, core::Printer& printer) -> void {
	print_num_errors(context.getNumErrors(), printer);
}



static auto run_build_system(const pthr::CmdArgsConfig& cmd_args_config, core::Printer& printer)
-> evo::Result<panther::Context::BuildSystemConfig> {
	using ContextConfig = panther::Context::Config;
	const auto context_config = ContextConfig{
		.mode             = ContextConfig::Mode::BUILD_SYSTEM,
		.title            = "<Panther-Build-System>",
		.target           = core::Target::getNative(),
		.workingDirectory = cmd_args_config.workingDirectory,

		.numThreads = cmd_args_config.numBuildThreads,
	};

	if(cmd_args_config.verbosity == pthr::CmdArgsConfig::Verbosity::FULL){
		printer.printlnMagenta("Build system relative directory: \"{}\"", cmd_args_config.workingDirectory.string());
	}

	auto context = panther::Context(
		panther::createDefaultDiagnosticCallback(printer, cmd_args_config.workingDirectory), context_config
	);


	if(cmd_args_config.use_std_lib){
		const CreatePantherPackageResult std_package_id = context.getSourceManager().createPackage(
			panther::Source::Package{
				.basePath = cmd_args_config.workingDirectory / "../extern/Panther-std/std",
				.warn     = panther::Source::Package::Warns::all(),
			}
		);

		if(std_package_id.has_value() == false){
			error_failed_to_add_std_lib(std_package_id.error(), printer);
			return evo::resultError;
		}

		const panther::Context::AddSourceResult add_dir_result = 
			context.addSourceDirectoryRecursive("./", *std_package_id);

		evo::debugAssert(add_dir_result == panther::Context::AddSourceResult::SUCCESS, "This should never fail");

		context.addStdLib(*std_package_id);
	}


	const CreatePantherPackageResult package_res = context.getSourceManager().createPackage(
		panther::Source::Package{
			.basePath = cmd_args_config.workingDirectory,
			.warn     = panther::Source::Package::Warns::all(),
		}
	);

	if(package_res.has_value() == false){
		// TODO(FUTURE): handle fail
		evo::debugFatalBreak("Failed to create build package");
	}

	std::ignore = context.addSourceFile("build.pthr", *package_res);


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
	panther::Context::BuildSystemConfig& config,
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
		.mode             = ContextConfig::Mode::COMPILE,
		.title            = "Panther Testing",
		.target           = core::Target::getNative(),
		.workingDirectory = cmd_args_config.workingDirectory,

		.numThreads = config.numThreads,
	};

	if(cmd_args_config.verbosity == pthr::CmdArgsConfig::Verbosity::FULL){
		printer.printlnMagenta("Compile relative directory: \"{}\"", cmd_args_config.workingDirectory.string());
	}

	auto context = panther::Context(
		panther::createDefaultDiagnosticCallback(printer, cmd_args_config.workingDirectory), context_config
	);


	using BuildSystemConfig = panther::Context::BuildSystemConfig;

	unsigned num_errors = 0;

	for(const panther::Source::Package& package : config.packages){
		const fs::path package_path = package.basePath;
		const std::string package_name = package.name;

		const CreatePantherPackageResult res = context.getSourceManager().createPackage(std::move(package));

		if(res.has_value() == false){
			switch(res.error()){
				case panther::SourceManager::CreatePackageFailReason::PATH_NOT_ABSOLUTE: {
					panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
						panther::Diagnostic::Level::ERROR,
						panther::Diagnostic::Code::FRONTEND_DIRECTORY_DOESNT_EXIST,
						panther::Diagnostic::Location::NONE,
						"Invalid path for package",
						evo::SmallVector<panther::Diagnostic::Info>{
							panther::Diagnostic::Info(std::format("Path: \"{}\"", package_path.string())),
							panther::Diagnostic::Info("Path isn't absolute")
						}
					));
				} break;

				case panther::SourceManager::CreatePackageFailReason::PATH_DOESNT_EXIST: {
					panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
						panther::Diagnostic::Level::ERROR,
						panther::Diagnostic::Code::FRONTEND_DIRECTORY_DOESNT_EXIST,
						panther::Diagnostic::Location::NONE,
						"Invalid path for package",
						evo::SmallVector<panther::Diagnostic::Info>{
							panther::Diagnostic::Info(std::format("Path: \"{}\"", package_path.string())),
							panther::Diagnostic::Info("Path doesn't exist")
						}
					));
				} break;

				case panther::SourceManager::CreatePackageFailReason::PATH_NOT_DIRECTORY: {
					panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
						panther::Diagnostic::Level::ERROR,
						panther::Diagnostic::Code::FRONTEND_DIRECTORY_DOESNT_EXIST,
						panther::Diagnostic::Location::NONE,
						"Invalid path for package",
						evo::SmallVector<panther::Diagnostic::Info>{
							panther::Diagnostic::Info(std::format("Path: \"{}\"", package_path.string())),
							panther::Diagnostic::Info("Path isn't directory")
						}
					));
				} break;

				case panther::SourceManager::CreatePackageFailReason::INVALID_NAME: {
					panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
						panther::Diagnostic::Level::ERROR,
						panther::Diagnostic::Code::FRONTEND_DIRECTORY_DOESNT_EXIST,
						panther::Diagnostic::Location::NONE,
						"Invalid name for package",
						evo::SmallVector<panther::Diagnostic::Info>{
							panther::Diagnostic::Info(std::format("Name: \"{}\"", package_name)),
						}
					));
				} break;
			}

			num_errors += 1;
		}
	}


	if(config.stdLibPackageID.has_value()){
		context.addStdLib(*config.stdLibPackageID);	
	}


	if(num_errors > 0){
		print_num_errors(num_errors, printer);
		return evo::resultError;
	}


	for(const panther::Context::BuildSystemConfig::PantherFile& source_file : config.sourceFiles){
		const panther::Context::AddSourceResult result = context.addSourceFile(source_file.path, source_file.packageID);

		switch(result){
			case panther::Context::AddSourceResult::SUCCESS: break;

			case panther::Context::AddSourceResult::DOESNT_EXIST: {
				const panther::Source::Package& package = context.getSourceManager().getPackage(source_file.packageID);

				panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
					panther::Diagnostic::Level::ERROR,
					panther::Diagnostic::Code::FRONTEND_FILE_DOESNT_EXIST,
					panther::Diagnostic::Location::NONE,
					"File doesn't exist",
					evo::SmallVector<panther::Diagnostic::Info>{
						panther::Diagnostic::Info(std::format("Path: \"{}\"", source_file.path)),
						panther::Diagnostic::Info(
							std::format("Relative directory: \"{}\"", package.basePath.string())
						)
					}
				));

				num_errors += 1;
			} break;

			case panther::Context::AddSourceResult::NOT_DIRECTORY: {
				evo::debugFatalBreak("Shouldn't be possible to get this code");
			} break;
		}
	}


	for(const panther::Context::BuildSystemConfig::PantherDirectory& source_directory : config.sourceDirectories){
		const panther::Context::AddSourceResult result = [&]() -> panther::Context::AddSourceResult {
			if(source_directory.isRecursive){
				return context.addSourceDirectoryRecursive(source_directory.path, source_directory.packageID);
			}else{
				return context.addSourceDirectory(source_directory.path, source_directory.packageID);
			}
		}();

		switch(result){
			case panther::Context::AddSourceResult::SUCCESS: break;

			case panther::Context::AddSourceResult::DOESNT_EXIST: {
				const panther::Source::Package& package = 
					context.getSourceManager().getPackage(source_directory.packageID);

				panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
					panther::Diagnostic::Level::ERROR,
					panther::Diagnostic::Code::FRONTEND_DIRECTORY_DOESNT_EXIST,
					panther::Diagnostic::Location::NONE,
					"Directory doesn't exist",
					evo::SmallVector<panther::Diagnostic::Info>{
						panther::Diagnostic::Info(std::format("Path: \"{}\"", source_directory.path)),
						panther::Diagnostic::Info(
							std::format("Relative directory: \"{}\"", package.basePath.string())
						)
					}
				));

				num_errors += 1;
			} break;

			case panther::Context::AddSourceResult::NOT_DIRECTORY: {
				const panther::Source::Package& package = 
					context.getSourceManager().getPackage(source_directory.packageID);

				panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
					panther::Diagnostic::Level::ERROR,
					panther::Diagnostic::Code::FRONTEND_DIRECTORY_NOT_DIRECTORY,
					panther::Diagnostic::Location::NONE,
					"Directory is not directory",
					evo::SmallVector<panther::Diagnostic::Info>{
						panther::Diagnostic::Info(std::format("Path: \"{}\"", source_directory.path)),
						panther::Diagnostic::Info(
							std::format("Relative directory: \"{}\"", package.basePath.string())
						)
					}
				));

				num_errors += 1;
			} break;
		}
	}




	for(const panther::Context::BuildSystemConfig::CLangFile& c_lang_header_file : config.cLangFiles){
		const panther::Context::AddSourceResult result = [&](){
			if(c_lang_header_file.isCPP){
				return context.addCPPHeaderFile(c_lang_header_file.path, c_lang_header_file.addIncludesToPubApi);
			}else{
				return context.addCHeaderFile(c_lang_header_file.path, c_lang_header_file.addIncludesToPubApi);
			}
		}();

		switch(result){
			case panther::Context::AddSourceResult::SUCCESS: break;

			case panther::Context::AddSourceResult::DOESNT_EXIST: {
				panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
					panther::Diagnostic::Level::ERROR,
					panther::Diagnostic::Code::FRONTEND_FILE_DOESNT_EXIST,
					panther::Diagnostic::Location::NONE,
					"File doesn't exist",
					evo::SmallVector<panther::Diagnostic::Info>{
						panther::Diagnostic::Info(std::format("Path: \"{}\"", c_lang_header_file.path))
					}
				));
				num_errors += 1;
			} break;

			case panther::Context::AddSourceResult::NOT_DIRECTORY: {
				evo::debugFatalBreak("Shouldn't be possible to get this code");
			} break;
		}
	}


	if(num_errors > 0){
		print_num_errors(num_errors, printer);
		return evo::resultError;
	}


	switch(config.output){
		case BuildSystemConfig::Output::TOKENS: {
			if(context.tokenize().isError()){
				print_num_context_errors(context, printer);
				return evo::resultError;
			}

			for(const panther::Source::ID source_id : context.getSourceManager().getSourceIDRange()){
				pthr::print_tokens(printer, context.getSourceManager()[source_id], cmd_args_config.workingDirectory);
			}

			return evo::Result<>();
		} break;

		case BuildSystemConfig::Output::AST: {
			if(context.parse().isError()){
				print_num_context_errors(context, printer);
				return evo::resultError;
			}

			for(const panther::Source::ID source_id : context.getSourceManager().getSourceIDRange()){
				pthr::print_AST(printer, context.getSourceManager()[source_id], cmd_args_config.workingDirectory);
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


	{
		std::error_code ec;
		cmd_args_config.workingDirectory = std::filesystem::current_path(ec);
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

			return EXIT_FAILURE;
		}
	}


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

	evo::Result<panther::Context::BuildSystemConfig> build_system_run = 
		run_build_system(cmd_args_config, printer);
	if(build_system_run.isError()){ return EXIT_FAILURE; }


	if(run_compile(cmd_args_config, build_system_run.value(), printer).isError()){ return EXIT_FAILURE; }


	if(cmd_args_config.verbosity >= pthr::CmdArgsConfig::Verbosity::SOME){
		printer.printlnSuccess("Successfully completed");
	}

	return EXIT_SUCCESS;
}