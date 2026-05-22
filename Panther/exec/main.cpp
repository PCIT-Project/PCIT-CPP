////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include <iostream>

#include <Evo.hpp>
#include <Panther.hpp>
namespace core = pcit::core;
namespace panther = pcit::panther;
namespace pir = pcit::pir;
#include <PLNK.hpp>
namespace plnk = pcit::plnk;

#include "./printing.hpp"


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
		"Failed to add Panther standard library",
		panther::Diagnostic::Location::NONE,
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



[[nodiscard]] static auto run_compile(
	const pthr::CmdArgsConfig& cmd_args_config,
	panther::Context::PantherBuildConfig& config,
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

		.includeDebugInfo = config.addDebugInfo,
		.numThreads       = config.numThreads,
	};

	if(cmd_args_config.verbosity == pthr::CmdArgsConfig::Verbosity::FULL){
		printer.printlnMagenta("Compile relative directory: \"{}\"", cmd_args_config.workingDirectory.string());
	}

	auto context = panther::Context(
		panther::createDefaultDiagnosticCallback(printer, cmd_args_config.workingDirectory), context_config
	);


	using PantherBuildConfig = panther::Context::PantherBuildConfig;

	unsigned num_errors = 0;

	for(const panther::Context::PantherBuildConfig::Package& package : config.packages){
		const std::string_view package_path = static_cast<std::string_view>(package.path);

		const std::filesystem::path normalized_package_path = 
			(cmd_args_config.workingDirectory / std::filesystem::path(package_path)).lexically_normal();

		const CreatePantherPackageResult create_package_res = context.getSourceManager().createPackage(
			panther::Source::Package(normalized_package_path, static_cast<std::string>(package.name), package.warns)
		);

		if(create_package_res.has_value() == false){
			switch(create_package_res.error()){
				case panther::SourceManager::CreatePackageFailReason::PATH_NOT_ABSOLUTE: {
					panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
						panther::Diagnostic::Level::ERROR,
						"Invalid path for package",
						panther::Diagnostic::Location::NONE,
						evo::SmallVector<panther::Diagnostic::Info>{
							panther::Diagnostic::Info(std::format("Path: \"{}\"", package_path)),
							panther::Diagnostic::Info(
								std::format("Normalized Path: \"{}\"", normalized_package_path.string())
							),
							panther::Diagnostic::Info("Path isn't absolute")
						}
					));
				} break;

				case panther::SourceManager::CreatePackageFailReason::PATH_DOESNT_EXIST: {
					panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
						panther::Diagnostic::Level::ERROR,
						"Invalid path for package",
						panther::Diagnostic::Location::NONE,
						evo::SmallVector<panther::Diagnostic::Info>{
							panther::Diagnostic::Info(std::format("Path: \"{}\"", package_path)),
							panther::Diagnostic::Info(
								std::format("Normalized Path: \"{}\"", normalized_package_path.string())
							),
							panther::Diagnostic::Info("Path doesn't exist")
						}
					));
				} break;

				case panther::SourceManager::CreatePackageFailReason::PATH_NOT_DIRECTORY: {
					panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
						panther::Diagnostic::Level::ERROR,
						"Invalid path for package",
						panther::Diagnostic::Location::NONE,
						evo::SmallVector<panther::Diagnostic::Info>{
							panther::Diagnostic::Info(std::format("Path: \"{}\"", package_path)),
							panther::Diagnostic::Info(
								std::format("Normalized Path: \"{}\"", normalized_package_path.string())
							),
							panther::Diagnostic::Info("Path isn't directory")
						}
					));
				} break;

				case panther::SourceManager::CreatePackageFailReason::INVALID_NAME: {
					panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
						panther::Diagnostic::Level::ERROR,
						"Invalid name for package",
						panther::Diagnostic::Location::NONE,
						evo::SmallVector<panther::Diagnostic::Info>{
							panther::Diagnostic::Info(
								std::format("Name: \"{}\"", static_cast<std::string_view>(package.name))
							),
						}
					));
				} break;
			}

			num_errors += 1;
		}


		if(static_cast<std::string_view>(package.name) == "std"){
			context.addStdLib(create_package_res.value());
		}


		for(const panther::Context::PantherBuildConfig::StringRef& source_file_path : package.sourceFiles){
			const panther::Context::AddSourceResult result =
				context.addSourceFile(static_cast<std::string_view>(source_file_path), create_package_res.value());

			switch(result){
				case panther::Context::AddSourceResult::SUCCESS: break;

				case panther::Context::AddSourceResult::DOESNT_EXIST: {
					panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
						panther::Diagnostic::Level::ERROR,
						"File doesn't exist",
						panther::Diagnostic::Location::NONE,
						evo::SmallVector<panther::Diagnostic::Info>{
							panther::Diagnostic::Info(
								std::format("Path: \"{}\"", static_cast<std::string_view>(source_file_path))
							),
							panther::Diagnostic::Info(
								std::format("Relative directory: \"{}\"", normalized_package_path.string())
							)
						}
					));

					num_errors += 1;
				} break;

				case panther::Context::AddSourceResult::NOT_FILE: {
					panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
						panther::Diagnostic::Level::ERROR,
						"Path isn't a file",
						panther::Diagnostic::Location::NONE,
						evo::SmallVector<panther::Diagnostic::Info>{
							panther::Diagnostic::Info(
								std::format("Path: \"{}\"", static_cast<std::string_view>(source_file_path))
							),
							panther::Diagnostic::Info(
								std::format("Relative directory: \"{}\"", normalized_package_path.string())
							)
						}
					));

					num_errors += 1;
				} break;

				case panther::Context::AddSourceResult::NOT_DIRECTORY: {
					evo::debugFatalBreak("Shouldn't be possible to get this error code");
				} break;
			}
		}

		using PackageDirectory = panther::Context::PantherBuildConfig::Package::Directory;
		for(const PackageDirectory& source_directory : package.sourceDirectories){
			const panther::Context::AddSourceResult result = [&]() -> panther::Context::AddSourceResult {
				if(source_directory.isRecursive){
					return context.addSourceDirectoryRecursive(
						static_cast<std::string_view>(source_directory.path), create_package_res.value()
					);
				}else{
					return context.addSourceDirectory(
						static_cast<std::string_view>(source_directory.path), create_package_res.value()
					);
				}
			}();

			switch(result){
				case panther::Context::AddSourceResult::SUCCESS: break;

				case panther::Context::AddSourceResult::DOESNT_EXIST: {
					panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
						panther::Diagnostic::Level::ERROR,
						"Directory doesn't exist",
						panther::Diagnostic::Location::NONE,
						evo::SmallVector<panther::Diagnostic::Info>{
							panther::Diagnostic::Info(
								std::format("Path: \"{}\"", static_cast<std::string_view>(source_directory.path))
							),
							panther::Diagnostic::Info(
								std::format("Relative directory: \"{}\"", normalized_package_path.string())
							)
						}
					));

					num_errors += 1;
				} break;

				case panther::Context::AddSourceResult::NOT_FILE: {
					evo::debugFatalBreak("Shouldn't be possible to get this error code");
				} break;

				case panther::Context::AddSourceResult::NOT_DIRECTORY: {
					panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
						panther::Diagnostic::Level::ERROR,
						"Directory is not directory",
						panther::Diagnostic::Location::NONE,
						evo::SmallVector<panther::Diagnostic::Info>{
							panther::Diagnostic::Info(
								std::format("Path: \"{}\"", static_cast<std::string_view>(source_directory.path))
							),
							panther::Diagnostic::Info(
								std::format("Relative directory: \"{}\"", normalized_package_path.string())
							)
						}
					));

					num_errors += 1;
				} break;
			}
		}
	}

	for(const panther::Context::PantherBuildConfig::CFamilyHeader& c_family_header : config.cFamilyHeaders){
		const panther::Context::AddSourceResult result = [&](){
			if(c_family_header.isCPP){
				return context.addCPPHeaderFile(
					static_cast<std::string_view>(c_family_header.path), c_family_header.addIncludesToPubApi
				);
			}else{
				return context.addCHeaderFile(
					static_cast<std::string_view>(c_family_header.path), c_family_header.addIncludesToPubApi
				);
			}
		}();

		switch(result){
			case panther::Context::AddSourceResult::SUCCESS: break;

			case panther::Context::AddSourceResult::DOESNT_EXIST: {
				panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
					panther::Diagnostic::Level::ERROR,
					"File doesn't exist",
					panther::Diagnostic::Location::NONE,
					evo::SmallVector<panther::Diagnostic::Info>{
						panther::Diagnostic::Info(
							std::format("Path: \"{}\"", static_cast<std::string_view>(c_family_header.path))
						)
					}
				));
				num_errors += 1;
			} break;

			case panther::Context::AddSourceResult::NOT_FILE: {
				panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
					panther::Diagnostic::Level::ERROR,
					"Path isn't a file",
					panther::Diagnostic::Location::NONE,
					evo::SmallVector<panther::Diagnostic::Info>{
						panther::Diagnostic::Info(
							std::format("Path: \"{}\"", static_cast<std::string_view>(c_family_header.path))
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


	if(num_errors > 0){
		print_num_errors(num_errors, printer);
		return evo::resultError;
	}



	if(num_errors > 0){
		print_num_errors(num_errors, printer);
		return evo::resultError;
	}


	switch(config.output.getTag()){
		case PantherBuildConfig::Output::Tag::TOKENS: {
			if(context.tokenize().isError()){
				print_num_context_errors(context, printer);
				return evo::resultError;
			}

			for(const panther::Source::ID source_id : context.getSourceManager().getSourceIDRange()){
				pthr::print_tokens(printer, context.getSourceManager()[source_id], cmd_args_config.workingDirectory);
			}

			return evo::Result<>();
		} break;

		case PantherBuildConfig::Output::Tag::AST: {
			if(context.parse().isError()){
				print_num_context_errors(context, printer);
				return evo::resultError;
			}

			for(const panther::Source::ID source_id : context.getSourceManager().getSourceIDRange()){
				pthr::print_AST(
					printer, context.getSourceManager()[source_id], cmd_args_config.workingDirectory, false
				);
			}

			return evo::Result<>();
		} break;

		case PantherBuildConfig::Output::Tag::SEMANTIC_ANALYSIS: {
			if(context.analyzeSemantics().isError()){
				print_num_context_errors(context, printer);
				return evo::resultError;
			}

			return evo::Result<>();
		} break;

		case PantherBuildConfig::Output::Tag::PIR: {
			if(context.analyzeSemantics().isError()){
				print_num_context_errors(context, printer);
				return evo::resultError;
			}


			auto printer_for_pir_module = core::Printer::createString(printer.isPrintingColor());

			if(context.lowerToPIR(panther::Context::EntryKind::NONE).isError()){ return evo::resultError; }
			pir::printModule(context.getPIRModule(), printer_for_pir_module);

			evo::print(printer_for_pir_module.getString());

			return evo::Result<>();
		} break;

		case PantherBuildConfig::Output::Tag::LLVMIR: {
			if(context.analyzeSemantics().isError()){
				print_num_context_errors(context, printer);
				return evo::resultError;
			}

			if(context.lowerToPIR(panther::Context::EntryKind::NONE).isError()){ return evo::resultError; }

			const evo::Result<std::string> llvmir_string = context.lowerToLLVMIR();
			if(llvmir_string.isError()){ return evo::resultError; }

			printer.print(llvmir_string.value());

			return evo::Result<>();
		} break;

		case PantherBuildConfig::Output::Tag::ASSEMBLY: {
			if(context.analyzeSemantics().isError()){
				print_num_context_errors(context, printer);
				return evo::resultError;
			}

			if(context.lowerToPIR(panther::Context::EntryKind::NONE).isError()){ return evo::resultError; }

			const evo::Result<std::string> asm_result = context.lowerToAssembly();
			if(asm_result.isError()){
				panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
					panther::Diagnostic::Level::ERROR,
					"Failed to output assembly code",
					panther::Diagnostic::Location::NONE
				));
				return evo::resultError;
			}

			printer.print(asm_result.value());

			return evo::Result<>();
		} break;

		case PantherBuildConfig::Output::Tag::OBJECT: {
			if(context.analyzeSemantics().isError()){
				print_num_context_errors(context, printer);
				return evo::resultError;
			}

			if(context.lowerToPIR(panther::Context::EntryKind::NONE).isError()){ return evo::resultError; }

			const evo::Result<std::vector<evo::byte>> object_data = context.lowerToObject();
			if(object_data.isError()){
				panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
					panther::Diagnostic::Level::ERROR,
					"Failed to output object file",
					panther::Diagnostic::Location::NONE
				));
				return evo::resultError;
			}

			if(evo::fs::writeBinaryFile("build/output.o", object_data.value()) == false){
				panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
					panther::Diagnostic::Level::ERROR,
					"Failed to output object file",
					panther::Diagnostic::Location::NONE,
					panther::Diagnostic::Info("Failed to write file")
				));
				return evo::resultError;
			}

			return evo::Result<>();
		} break;

		case PantherBuildConfig::Output::Tag::RUN: {
			if(context.analyzeSemantics().isError()){
				print_num_context_errors(context, printer);
				return evo::resultError;
			}

			const evo::Result<uint8_t> entry_res = context.runEntry(true);
			if(entry_res.isError()){ return evo::resultError; }
			printer.printlnSuccess("Value returned from entry: {}", entry_res.value());
			return evo::Result<>();
		} break;

		case PantherBuildConfig::Output::Tag::EXECUTABLE: {
			if(context.analyzeSemantics().isError()){
				print_num_context_errors(context, printer);
				return evo::resultError;
			}

			const panther::Context::EntryKind entry_kind = [&](){
				if(config.output.exectuableData().isConsole){
					return panther::Context::EntryKind::CONSOLE_EXECUTABLE;
				}else{
					return panther::Context::EntryKind::WINDOWED_EXECUTABLE;
				}
			}();

			if(context.lowerToPIR(entry_kind).isError()){
				return evo::resultError;
			}

			const evo::Result<std::vector<evo::byte>> object_data = context.lowerToObject();
			if(object_data.isError()){
				panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
					panther::Diagnostic::Level::ERROR,
					"Failed to output object file",
					panther::Diagnostic::Location::NONE
				));
				return evo::resultError;
			}

			{
				std::error_code ec;
				std::filesystem::create_directories(std::filesystem::path("build"), ec);
				if(ec){
					panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
						panther::Diagnostic::Level::ERROR,
						"Failed to create directory \"build\"",
						panther::Diagnostic::Location::NONE,
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
					"Failed to output obj",
					panther::Diagnostic::Location::NONE,
					panther::Diagnostic::Info("Failed to write file")
				));
				return evo::resultError;
			}

			auto plnk_options = plnk::Options(plnk::Target::WINDOWS);
			plnk_options.outputFilePath = "build/output.exe";
			plnk_options.getWindowsSpecific().subsystem = [&](){
				if(config.output.exectuableData().isConsole){
					return plnk::Options::WindowsSpecific::Subsystem::CONSOLE;
				}else{
					return plnk::Options::WindowsSpecific::Subsystem::WINDOWS;
				}
			}();

			const plnk::LinkResult link_result = plnk::link({std::filesystem::path("build/output.o")}, plnk_options);

			if(link_result.messages.empty() == false){
				printer.printlnCyan("<Info> Linker messages");
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
					printer.printRed("<Error> ");
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
		panther::Diagnostic::createFatalMessage("Unknown compile output recieved"), panther::Diagnostic::Location::NONE
	);
	return evo::resultError;
}



static auto run_build_system(const pthr::CmdArgsConfig& cmd_args_config, core::Printer& printer)
-> evo::Result<uint8_t> {
	using ContextConfig = panther::Context::Config;
	const auto context_config = ContextConfig{
		.mode             = ContextConfig::Mode::BUILD_SYSTEM,
		.title            = "<Panther-Build-System>",
		.target           = core::Target::getNative(),
		.workingDirectory = cmd_args_config.workingDirectory,

		.includeDebugInfo = true,

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
				.name     = "std",
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
			.name     = "build",
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


	return context.runBuildSystem(
		[&](panther::Context::PantherBuildConfig& panther_build_config) -> evo::Result<> {
			return run_compile(cmd_args_config, panther_build_config, printer);
		}
	);
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
				"Failed to get relative directory",
				panther::Diagnostic::Location::NONE,
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
			printer.printlnMagenta("v{} (debug)", pcit::core::VERSION);
		#elif defined(PCIT_BUILD_RELEASE)
			printer.printlnMagenta("v{}", pcit::core::VERSION);
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


	const evo::Result<uint8_t> build_system_result = run_build_system(cmd_args_config, printer);

	if(build_system_result.isError()){ return EXIT_FAILURE; }

	if(build_system_result.value() != 0){
		panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
			panther::Diagnostic::Level::ERROR,
			std::format("Build system errored with code: {}", build_system_result.value()),
			panther::Diagnostic::Location::NONE
		));
		return EXIT_FAILURE;
	}

	if(cmd_args_config.verbosity >= pthr::CmdArgsConfig::Verbosity::SOME){
		printer.printlnSuccess("Successfully completed");
	}

	return EXIT_SUCCESS;
}