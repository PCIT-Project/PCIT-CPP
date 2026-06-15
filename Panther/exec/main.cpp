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
#include "./args.hpp"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pthr{


	static auto setup_env() -> void {
		core::windows::setConsoleToUTF8Mode();

		#if defined(PCIT_CONFIG_DEBUG)
			evo::log::setDefaultThreadSaferCallback();
		#endif
	}


	static auto setup_debug_close(bool print_color) -> void {
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


[[nodiscard]] static auto create_absolute_path(const fs::path& path, const fs::path& base_path) -> fs::path {
	return (base_path / path).lexically_normal();
}


[[nodiscard]] static auto create_directories(const std::filesystem::path& path, core::Printer& printer)
-> evo::Result<> {
	std::error_code ec;
	std::filesystem::create_directories(path, ec);

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

	return evo::Result<>();
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

		case panther::SourceManager::CreatePackageFailReason::INVALID_NAME: {
			infos.emplace_back("Invaid name");
		} break;

		case panther::SourceManager::CreatePackageFailReason::INVALID_OPTION_NAME: {
			infos.emplace_back("Invaid option name");
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

	if(cmd_args_config.verbosity == pthr::CmdArgsConfig::Verbosity::SOME){
		printer.printlnMagenta("Running compile");

	}else if(cmd_args_config.verbosity == pthr::CmdArgsConfig::Verbosity::FULL){
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
		.title                  = config.title,
		.target                 = core::Target(config.architecture, config.platform),
		.mode                   = ContextConfig::Mode::COMPILE,
		.optMode                = config.optMode,
		.compilerExecutablePath = cmd_args_config.executablePath,
		.workingDirectory       = cmd_args_config.workingDirectory,

		.includeDebugInfo       = config.addDebugInfo,
		.numThreads             = config.numThreads,
	};

	if(cmd_args_config.verbosity == pthr::CmdArgsConfig::Verbosity::FULL){
		printer.printlnGray("Compile relative directory: \"{}\"", cmd_args_config.workingDirectory.string());
	}

	if(cmd_args_config.verbosity >= pthr::CmdArgsConfig::Verbosity::SOME){
		printer.printlnMagenta("Architecture: {}", config.architecture);
		printer.printlnMagenta("Platform: {}", config.platform);

		switch(config.optMode){
			break; case pir::OptMode::NONE:            printer.printlnMagenta("Optimization Mode: NONE");
			break; case pir::OptMode::SPEED_MINOR:     printer.printlnMagenta("Optimization Mode: SPEED_MINOR");
			break; case pir::OptMode::SPEED:           printer.printlnMagenta("Optimization Mode: SPEED");
			break; case pir::OptMode::SPEED_AGRESSIVE: printer.printlnMagenta("Optimization Mode: SPEED_AGRESSIVE");
			break; case pir::OptMode::SIZE:            printer.printlnMagenta("Optimization Mode: SIZE");
			break; case pir::OptMode::SIZE_AGRESSIVE:  printer.printlnMagenta("Optimization Mode: SIZE_AGRESSIVE");
		}

		printer.printlnMagenta("Include Debug Info: {}", config.addDebugInfo);
	}



	auto context = panther::Context(
		panther::createDefaultDiagnosticCallback(printer, cmd_args_config.workingDirectory), context_config
	);


	using PantherBuildConfig = panther::Context::PantherBuildConfig;

	unsigned num_errors = 0;

	for(const panther::Context::PantherBuildConfig::Package& package : config.packages){
		const std::string_view package_path = static_cast<std::string_view>(package.path);

		const std::filesystem::path normalized_package_path = create_absolute_path(
			std::filesystem::path(package_path), cmd_args_config.workingDirectory
		);

		bool errored_in_options = false;
		auto options = std::unordered_map<std::string_view, panther::Source::Package::Option>();
		for(const panther::Context::PantherBuildConfig::Package::Option& option : package.options){
			const std::string_view option_name = static_cast<std::string_view>(option.name);

			if(option_name.empty()){
				panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
					panther::Diagnostic::Level::ERROR,
					"Package option name \"\" is invalid",
					panther::Diagnostic::Location::NONE,
					evo::SmallVector<panther::Diagnostic::Info>{
						panther::Diagnostic::Info("Name cannot be empty"),
					}
				));
				errored_in_options = true;
				break;
			}

			if(evo::isLetter(option_name[0]) == false && option_name[0] != '_'){
				panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
					panther::Diagnostic::Level::ERROR,
					std::format("Package option name \"{}\" is invalid", option_name),
					panther::Diagnostic::Location::NONE
				));
				errored_in_options = true;
				break;
			}

			for(size_t i = 1; i < option_name.size(); i+=1){
				if(evo::isAlphaNumeric(option_name[i]) == false && option_name[i] != '_'){
					panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
						panther::Diagnostic::Level::ERROR,
						std::format("Package option name \"{}\" is invalid", option_name),
						panther::Diagnostic::Location::NONE
					));
					errored_in_options = true;
					break;
				}
			}
			if(errored_in_options){ break; }


			switch(option.value.getTag()){
				case panther::Context::PantherBuildConfig::Package::Option::Value::Tag::BOOLEAN: {
					options.emplace(option_name, option.value.boolValue());
				} break;

				case panther::Context::PantherBuildConfig::Package::Option::Value::Tag::UI8: {
					options.emplace(option_name, option.value.ui8Value());
				} break;

				case panther::Context::PantherBuildConfig::Package::Option::Value::Tag::UI16: {
					options.emplace(option_name, option.value.ui16Value());
				} break;

				case panther::Context::PantherBuildConfig::Package::Option::Value::Tag::UI32: {
					options.emplace(option_name, option.value.ui32Value());
				} break;

				case panther::Context::PantherBuildConfig::Package::Option::Value::Tag::UI64: {
					options.emplace(option_name, option.value.ui64Value());
				} break;

				case panther::Context::PantherBuildConfig::Package::Option::Value::Tag::I8: {
					options.emplace(option_name, option.value.i8Value());
				} break;

				case panther::Context::PantherBuildConfig::Package::Option::Value::Tag::I16: {
					options.emplace(option_name, option.value.i16Value());
				} break;

				case panther::Context::PantherBuildConfig::Package::Option::Value::Tag::I32: {
					options.emplace(option_name, option.value.i32Value());
				} break;

				case panther::Context::PantherBuildConfig::Package::Option::Value::Tag::I64: {
					options.emplace(option_name, option.value.i64Value());
				} break;

				case panther::Context::PantherBuildConfig::Package::Option::Value::Tag::F32: {
					options.emplace(option_name, option.value.f32Value());
				} break;

				case panther::Context::PantherBuildConfig::Package::Option::Value::Tag::F64: {
					options.emplace(option_name, option.value.f64Value());
				} break;

				case panther::Context::PantherBuildConfig::Package::Option::Value::Tag::STRING: {
					options.emplace(option_name, option.value.stringValue());
				} break;
			}
		}

		if(errored_in_options){
			num_errors += 1;
			continue;
		}


		const CreatePantherPackageResult create_package_res = context.getSourceManager().createPackage(
			panther::Source::Package(
				normalized_package_path,
				static_cast<std::string>(package.name),
				package.warns,
				std::move(options)
			)
		);

		if(create_package_res.has_value() == false){
			switch(create_package_res.error()){
				case panther::SourceManager::CreatePackageFailReason::PATH_NOT_ABSOLUTE: {
					panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
						panther::Diagnostic::Level::ERROR,
						"Invalid path for package",
						panther::Diagnostic::Location::NONE,
						evo::SmallVector<panther::Diagnostic::Info>{
							panther::Diagnostic::Info("Path isn't absolute"),
							panther::Diagnostic::Info(std::format("Path: \"{}\"", package_path)),
							panther::Diagnostic::Info(
								std::format("Normalized Path: \"{}\"", normalized_package_path.string())
							),
						}
					));
				} break;

				case panther::SourceManager::CreatePackageFailReason::PATH_DOESNT_EXIST: {
					panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
						panther::Diagnostic::Level::ERROR,
						"Invalid path for package",
						panther::Diagnostic::Location::NONE,
						evo::SmallVector<panther::Diagnostic::Info>{
							panther::Diagnostic::Info("Path doesn't exist"),
							panther::Diagnostic::Info(std::format("Path: \"{}\"", package_path)),
							panther::Diagnostic::Info(
								std::format("Normalized Path: \"{}\"", normalized_package_path.string())
							),
						}
					));
				} break;

				case panther::SourceManager::CreatePackageFailReason::PATH_NOT_DIRECTORY: {
					panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
						panther::Diagnostic::Level::ERROR,
						"Invalid path for package",
						panther::Diagnostic::Location::NONE,
						evo::SmallVector<panther::Diagnostic::Info>{
							panther::Diagnostic::Info("Path isn't directory"),
							panther::Diagnostic::Info(std::format("Path: \"{}\"", package_path)),
							panther::Diagnostic::Info(
								std::format("Normalized Path: \"{}\"", normalized_package_path.string())
							),
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

				case panther::SourceManager::CreatePackageFailReason::INVALID_OPTION_NAME: {
					evo::debugFatalBreak("Should have already caught");
				} break;
			}

			num_errors += 1;

			continue;

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
		const std::string_view header_path = static_cast<std::string_view>(c_family_header.path);

		const std::filesystem::path normalized_header_path = create_absolute_path(
			std::filesystem::path(header_path), cmd_args_config.workingDirectory
		);

		auto system_include_directories = evo::SmallVector<std::string>();
		for(
			panther::Context::PantherBuildConfig::StringRef system_include_directory
			: c_family_header.systemIncludeDirectories
		){
			system_include_directories.emplace_back(static_cast<std::string>(system_include_directory));
		}

		auto include_directories = evo::SmallVector<std::string>();
		for(panther::Context::PantherBuildConfig::StringRef include_directory : c_family_header.includeDirectories){
			include_directories.emplace_back(static_cast<std::string>(include_directory));
		}


		const panther::Context::AddSourceResult result = [&](){
			if(c_family_header.isCPP){
				return context.addCPPHeaderFile(
					normalized_header_path,
					std::move(system_include_directories),
					std::move(include_directories),
					c_family_header.addIncludesToPubApi
				);
			}else{
				return context.addCHeaderFile(
					normalized_header_path,
					std::move(system_include_directories),
					std::move(include_directories),
					c_family_header.addIncludesToPubApi
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

			auto printer_for_tokens = core::Printer::createString(
				printer.isPrintingColor() && config.output.tokensData().path.has_value() == false
			);

			for(const panther::Source::ID source_id : context.getSourceManager().getSourceIDRange()){
				pthr::print_tokens(
					printer_for_tokens, context.getSourceManager()[source_id], cmd_args_config.workingDirectory
				);
			}

			if(config.output.tokensData().path.has_value()){
				const std::filesystem::path tokens_path = create_absolute_path(
					std::filesystem::path(static_cast<std::string_view>(*config.output.tokensData().path)),
					cmd_args_config.workingDirectory
				);

				if(create_directories(tokens_path.parent_path(), printer).isError()){
					return evo::resultError;
				}

				if(evo::fs::writeFile(tokens_path.string(), printer_for_tokens.getString()) == false){
					panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
						panther::Diagnostic::Level::ERROR,
						"Failed to write tokens file",
						panther::Diagnostic::Location::NONE
					));
					return evo::resultError;
				}

				if(cmd_args_config.verbosity >= pthr::CmdArgsConfig::Verbosity::SOME){
					printer.printlnMagenta("Created tokens file: \"{}\"", tokens_path.string());
				}

			}else{
				printer.print(printer_for_tokens.getString());
			}

			return evo::Result<>();
		} break;

		case PantherBuildConfig::Output::Tag::AST: {
			if(context.parse().isError()){
				print_num_context_errors(context, printer);
				return evo::resultError;
			}

			auto printer_for_ast = core::Printer::createString(
				printer.isPrintingColor() && config.output.astData().path.has_value() == false
			);

			for(const panther::Source::ID source_id : context.getSourceManager().getSourceIDRange()){
				pthr::print_ast(
					printer_for_ast,
					context.getSourceManager()[source_id],
					cmd_args_config.workingDirectory,
					false
				);
			}


			if(config.output.astData().path.has_value()){
				const std::filesystem::path ast_path = create_absolute_path(
					std::filesystem::path(static_cast<std::string_view>(*config.output.astData().path)),
					cmd_args_config.workingDirectory
				);

				if(create_directories(ast_path.parent_path(), printer).isError()){
					return evo::resultError;
				}

				if(evo::fs::writeFile(ast_path.string(), printer_for_ast.getString()) == false){
					panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
						panther::Diagnostic::Level::ERROR,
						"Failed to write AST file",
						panther::Diagnostic::Location::NONE
					));
					return evo::resultError;
				}

				if(cmd_args_config.verbosity >= pthr::CmdArgsConfig::Verbosity::SOME){
					printer.printlnMagenta("Created AST file: \"{}\"", ast_path.string());
				}

			}else{
				printer.print(printer_for_ast.getString());
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


			auto printer_for_pir_module = core::Printer::createString(
				printer.isPrintingColor() && config.output.pirData().path.has_value() == false
			);

			if(context.lowerToPIR(panther::Context::EntryKind::NONE).isError()){ return evo::resultError; }
			pir::printModule(context.getPIRModule(), printer_for_pir_module);


			if(config.output.pirData().path.has_value()){
				const std::filesystem::path pir_path = create_absolute_path(
					std::filesystem::path(static_cast<std::string_view>(*config.output.pirData().path)),
					cmd_args_config.workingDirectory
				);

				if(create_directories(pir_path.parent_path(), printer).isError()){
					return evo::resultError;
				}

				if(evo::fs::writeFile(pir_path.string(), printer_for_pir_module.getString()) == false){
					panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
						panther::Diagnostic::Level::ERROR,
						"Failed to write PIR code file",
						panther::Diagnostic::Location::NONE
					));
					return evo::resultError;
				}

				if(cmd_args_config.verbosity >= pthr::CmdArgsConfig::Verbosity::SOME){
					printer.printlnMagenta("Created PIR file: \"{}\"", pir_path.string());
				}

			}else{
				printer.print(printer_for_pir_module.getString());
			}

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


			if(config.output.llvmirData().path.has_value()){
				const std::filesystem::path llvm_ir_path = create_absolute_path(
					std::filesystem::path(static_cast<std::string_view>(*config.output.llvmirData().path)),
					cmd_args_config.workingDirectory
				);

				if(create_directories(llvm_ir_path.parent_path(), printer).isError()){
					return evo::resultError;
				}

				if(evo::fs::writeFile(llvm_ir_path.string(), llvmir_string.value()) == false){
					panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
						panther::Diagnostic::Level::ERROR,
						"Failed to write LLVM IR code file",
						panther::Diagnostic::Location::NONE
					));
					return evo::resultError;
				}

				if(cmd_args_config.verbosity >= pthr::CmdArgsConfig::Verbosity::SOME){
					printer.printlnMagenta("Created LLVM IR file: \"{}\"", llvm_ir_path.string());
				}

			}else{
				printer.print(llvmir_string.value());
			}

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
					"Failed to create assembly code data",
					panther::Diagnostic::Location::NONE
				));
				return evo::resultError;
			}

			if(config.output.assemblyData().path.has_value()){
				const std::filesystem::path assembly_path = create_absolute_path(
					std::filesystem::path(static_cast<std::string_view>(*config.output.assemblyData().path)),
					cmd_args_config.workingDirectory
				);

				if(create_directories(assembly_path.parent_path(), printer).isError()){
					return evo::resultError;
				}


				if(evo::fs::writeFile(assembly_path.string(), asm_result.value()) == false){
					panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
						panther::Diagnostic::Level::ERROR,
						"Failed to write assembly code file",
						panther::Diagnostic::Location::NONE
					));
					return evo::resultError;
				}

				if(cmd_args_config.verbosity >= pthr::CmdArgsConfig::Verbosity::SOME){
					printer.printlnMagenta("Created assembly file: \"{}\"", assembly_path.string());
				}

			}else{
				printer.print(asm_result.value());
			}

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
					"Failed to create object file data",
					panther::Diagnostic::Location::NONE
				));
				return evo::resultError;
			}


			const std::filesystem::path object_path = create_absolute_path(
				std::filesystem::path(static_cast<std::string_view>(config.output.objectData().path)),
				cmd_args_config.workingDirectory
			);

			if(create_directories(object_path.parent_path(), printer).isError()){
				return evo::resultError;
			}


			if(evo::fs::writeBinaryFile(
				static_cast<std::string>(config.output.objectData().path), object_data.value()
			) == false){
				panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
					panther::Diagnostic::Level::ERROR,
					"Failed to write object file",
					panther::Diagnostic::Location::NONE
				));
				return evo::resultError;
			}

			if(cmd_args_config.verbosity >= pthr::CmdArgsConfig::Verbosity::SOME){
				printer.printlnMagenta("Created object file: \"{}\"", object_path.string());
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
				if(config.output.executableData().isConsole){
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



			const std::filesystem::path object_path = create_absolute_path(
				std::filesystem::path(static_cast<std::string_view>(config.output.executableData().objectPath)),
				cmd_args_config.workingDirectory
			);


			if(create_directories(object_path.parent_path(), printer).isError()){
				return evo::resultError;
			}


			if(evo::fs::writeBinaryFile(object_path.string(), object_data.value()) == false){
				panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
					panther::Diagnostic::Level::ERROR,
					"Failed to output obj",
					panther::Diagnostic::Location::NONE,
					panther::Diagnostic::Info("Failed to write file")
				));
				return evo::resultError;
			}

			if(cmd_args_config.verbosity >= pthr::CmdArgsConfig::Verbosity::SOME){
				printer.printlnMagenta("Created object file: \"{}\"", object_path.string());
			}


			const std::filesystem::path exec_path = create_absolute_path(
				std::filesystem::path(static_cast<std::string_view>(config.output.executableData().path)),
				cmd_args_config.workingDirectory
			);


			if(create_directories(exec_path.parent_path(), printer).isError()){
				return evo::resultError;
			}


			auto plnk_options = plnk::Options(plnk::Target::WINDOWS);
			plnk_options.outputFilePath = exec_path.string();
			plnk_options.getWindowsSpecific().subsystem = [&](){
				if(config.output.executableData().isConsole){
					return plnk::Options::WindowsSpecific::Subsystem::CONSOLE;
				}else{
					return plnk::Options::WindowsSpecific::Subsystem::WINDOWS;
				}
			}();

			const plnk::LinkResult link_result = plnk::link({object_path}, plnk_options);


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

			if(cmd_args_config.verbosity >= pthr::CmdArgsConfig::Verbosity::SOME){
				printer.printlnMagenta("Created executable: \"{}\"", exec_path.string());
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
		.title                  = "<Panther-Build-System>",
		.target                 = core::Target::getNative(),
		.mode                   = ContextConfig::Mode::BUILD_SYSTEM,
		.optMode                = pir::OptMode::SPEED,
		.compilerExecutablePath = cmd_args_config.executablePath,
		.workingDirectory       = cmd_args_config.workingDirectory,

		.includeDebugInfo       = true,

		.numThreads             = cmd_args_config.numBuildThreads,
	};

	if(cmd_args_config.verbosity == pthr::CmdArgsConfig::Verbosity::FULL){
		printer.printlnGray("Build system relative directory: \"{}\"", cmd_args_config.workingDirectory.string());
	}

	auto context = panther::Context(
		panther::createDefaultDiagnosticCallback(printer, cmd_args_config.workingDirectory), context_config
	);


	if(cmd_args_config.use_std_lib){
		const CreatePantherPackageResult std_package_id = context.getSourceManager().createPackage(
			panther::Source::Package{
				#if defined(PCIT_BUILD_DIST)
					.basePath = cmd_args_config.executablePath / "Panther-std/std",
				#else
					.basePath = cmd_args_config.executablePath / "../../../../extern/Panther-std/std",
				#endif
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
		switch(package_res.error()){
			case panther::SourceManager::CreatePackageFailReason::PATH_NOT_ABSOLUTE: {
				evo::debugFatalBreak("Should have already been made absolute");
			} break;

			case panther::SourceManager::CreatePackageFailReason::PATH_DOESNT_EXIST: {
				panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
					panther::Diagnostic::Level::ERROR,
					"Working directory path doesn't exist",
					panther::Diagnostic::Location::NONE,
					panther::Diagnostic::Info(std::format("Path: \"{}\"", cmd_args_config.workingDirectory.string()))
				));
				return evo::resultError;
			} break;

			case panther::SourceManager::CreatePackageFailReason::PATH_NOT_DIRECTORY: {
				panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
					panther::Diagnostic::Level::ERROR,
					"Working directory path isn't a directory",
					panther::Diagnostic::Location::NONE,
					panther::Diagnostic::Info(std::format("Path: \"{}\"", cmd_args_config.workingDirectory.string()))
				));
				return evo::resultError;
			} break;

			case panther::SourceManager::CreatePackageFailReason::INVALID_NAME: {
				evo::debugFatalBreak("Should never have this result");
			} break;

			case panther::SourceManager::CreatePackageFailReason::INVALID_OPTION_NAME: {
				evo::debugFatalBreak("Should never have this result");
			} break;
		}
	}


	const std::filesystem::path build_file_path = [&]() -> std::filesystem::path {
		if(cmd_args_config.file.has_value()){
			return *cmd_args_config.file;
		}else{
			return "build.pthr";
		}
	}();

	switch(context.addSourceFile(build_file_path, *package_res)){
		case panther::Context::AddSourceResult::SUCCESS: {
			// do nothing
		} break;

		case panther::Context::AddSourceResult::DOESNT_EXIST: {
			panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
				panther::Diagnostic::Level::ERROR,
				"Build system root file doesn't exist",
				panther::Diagnostic::Location::NONE,
				evo::SmallVector<panther::Diagnostic::Info>{
					panther::Diagnostic::Info(std::format("Given Path: \"{}\"", build_file_path.string())),
					panther::Diagnostic::Info(
						std::format(
							"Target Path: \"{}\"", (cmd_args_config.workingDirectory / build_file_path).string()
						)
					)
				}
			));
			return evo::resultError;
		} break;

		case panther::Context::AddSourceResult::NOT_FILE: {
			panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
				panther::Diagnostic::Level::ERROR,
				"Build system root file path isn't a file",
				panther::Diagnostic::Location::NONE,
				evo::SmallVector<panther::Diagnostic::Info>{
					panther::Diagnostic::Info(std::format("Given Path: \"{}\"", build_file_path.string())),
					panther::Diagnostic::Info(
						std::format(
							"Target Path: \"{}\"", (cmd_args_config.workingDirectory / build_file_path).string()
						)
					)
				}
			));
			return evo::resultError;
		} break;

		case panther::Context::AddSourceResult::NOT_DIRECTORY: {
			evo::debugFatalBreak("Invalid result");
		} break;
	}

	if(context.analyzeSemantics().isError()){
		print_num_context_errors(context, printer);
		return evo::resultError;
	}

	if(cmd_args_config.verbosity >= pthr::CmdArgsConfig::Verbosity::FULL){
		printer.printlnGray("Executing build system");
	}

	return context.runBuildSystem(
		[&](panther::Context::PantherBuildConfig& panther_build_config) -> evo::Result<> {
			return run_compile(cmd_args_config, panther_build_config, printer);
		},
		true
	);
}




static auto run_scripting(const pthr::CmdArgsConfig& cmd_args_config, core::Printer& printer)
-> evo::Result<uint8_t> {
	using ContextConfig = panther::Context::Config;
	const auto context_config = ContextConfig{
		.title                  = "<Panther-Script>",
		.target                 = core::Target::getNative(),
		.mode                   = ContextConfig::Mode::SCRIPTING,
		.optMode                = pir::OptMode::SPEED,
		.compilerExecutablePath = cmd_args_config.executablePath,
		.workingDirectory       = cmd_args_config.workingDirectory,

		.includeDebugInfo       = true,

		.numThreads             = cmd_args_config.numBuildThreads,
	};

	if(cmd_args_config.verbosity == pthr::CmdArgsConfig::Verbosity::FULL){
		printer.printlnGray("Script relative directory: \"{}\"", cmd_args_config.workingDirectory.string());
	}

	auto context = panther::Context(
		panther::createDefaultDiagnosticCallback(printer, cmd_args_config.workingDirectory), context_config
	);


	if(cmd_args_config.use_std_lib){
		const CreatePantherPackageResult std_package_id = context.getSourceManager().createPackage(
			panther::Source::Package{
				#if defined(PCIT_BUILD_DIST)
					.basePath = cmd_args_config.executablePath / "Panther-std/std",
				#else
					.basePath = cmd_args_config.executablePath / "../../../../extern/Panther-std/std",
				#endif
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
			.name     = "script",
			.warn     = panther::Source::Package::Warns::all(),
		}
	);

	if(package_res.has_value() == false){
		switch(package_res.error()){
			case panther::SourceManager::CreatePackageFailReason::PATH_NOT_ABSOLUTE: {
				evo::debugFatalBreak("Should have already been made absolute");
			} break;

			case panther::SourceManager::CreatePackageFailReason::PATH_DOESNT_EXIST: {
				panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
					panther::Diagnostic::Level::ERROR,
					"Working directory path doesn't exist",
					panther::Diagnostic::Location::NONE,
					panther::Diagnostic::Info(std::format("Path: \"{}\"", cmd_args_config.workingDirectory.string()))
				));
				return evo::resultError;
			} break;

			case panther::SourceManager::CreatePackageFailReason::PATH_NOT_DIRECTORY: {
				panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
					panther::Diagnostic::Level::ERROR,
					"Working directory path isn't a directory",
					panther::Diagnostic::Location::NONE,
					panther::Diagnostic::Info(std::format("Path: \"{}\"", cmd_args_config.workingDirectory.string()))
				));
				return evo::resultError;
			} break;

			case panther::SourceManager::CreatePackageFailReason::INVALID_NAME: {
				evo::debugFatalBreak("Should never have this result");
			} break;

			case panther::SourceManager::CreatePackageFailReason::INVALID_OPTION_NAME: {
				evo::debugFatalBreak("Should never have this result");
			} break;
		}
	}


	const std::filesystem::path build_file_path = [&]() -> std::filesystem::path {
		if(cmd_args_config.file.has_value()){
			return *cmd_args_config.file;
		}else{
			return "run.pthr";
		}
	}();

	switch(context.addSourceFile(build_file_path, *package_res)){
		case panther::Context::AddSourceResult::SUCCESS: {
			// do nothing
		} break;

		case panther::Context::AddSourceResult::DOESNT_EXIST: {
			panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
				panther::Diagnostic::Level::ERROR,
				"Script root file doesn't exist",
				panther::Diagnostic::Location::NONE,
				evo::SmallVector<panther::Diagnostic::Info>{
					panther::Diagnostic::Info(std::format("Given Path: \"{}\"", build_file_path.string())),
					panther::Diagnostic::Info(
						std::format(
							"Target Path: \"{}\"", (cmd_args_config.workingDirectory / build_file_path).string()
						)
					)
				}
			));
			return evo::resultError;
		} break;

		case panther::Context::AddSourceResult::NOT_FILE: {
			panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
				panther::Diagnostic::Level::ERROR,
				"Script root file path isn't a file",
				panther::Diagnostic::Location::NONE,
				evo::SmallVector<panther::Diagnostic::Info>{
					panther::Diagnostic::Info(std::format("Given Path: \"{}\"", build_file_path.string())),
					panther::Diagnostic::Info(
						std::format(
							"Target Path: \"{}\"", (cmd_args_config.workingDirectory / build_file_path).string()
						)
					)
				}
			));
			return evo::resultError;
		} break;

		case panther::Context::AddSourceResult::NOT_DIRECTORY: {
			evo::debugFatalBreak("Invalid result");
		} break;
	}

	if(context.analyzeSemantics().isError()){
		print_num_context_errors(context, printer);
		return evo::resultError;
	}

	if(cmd_args_config.verbosity == pthr::CmdArgsConfig::Verbosity::FULL){
		printer.printlnGray("Executing run script");
	}

	return context.runEntry(true);
}












auto main(int argc, const char* argv[]) -> int {
	auto args = std::vector<std::string_view>(argv, argv + argc);

	pthr::setup_env();

	if(args.size() == 1){
		const bool print_in_color = core::Printer::platformSupportsColor() == core::Printer::DetectResult::YES;
		pthr::setup_debug_close(print_in_color);
		auto printer = core::Printer::createConsole(print_in_color);
		pthr::print_help(printer);
		return EXIT_SUCCESS;
	}

	evo::Result<pthr::CmdArgsConfig> cmd_args_config =
		pthr::parse_args(args[1], evo::ArrayProxy<std::string_view>(args.data() + 2, args.size() - 2));

	if(cmd_args_config.isError()){
		pthr::setup_debug_close(core::Printer::platformSupportsColor() == core::Printer::DetectResult::YES);
		return EXIT_FAILURE;
	}

	pthr::setup_debug_close(cmd_args_config.value().print_color);

	auto printer = core::Printer::createConsole(cmd_args_config.value().print_color);

	{
		std::error_code ec;
		std::filesystem::path actual_working_path = std::filesystem::current_path(ec);
		if(ec){
			panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
				panther::Diagnostic::Level::ERROR,
				"Failed to get actual working directory",
				panther::Diagnostic::Location::NONE,
				evo::SmallVector<panther::Diagnostic::Info>{
					panther::Diagnostic::Info(std::format("\tcode: \"{}\"", ec.value())),
					panther::Diagnostic::Info(std::format("\tmessage: \"{}\"", ec.message())),
				}
			));

			return EXIT_FAILURE;
		}

		cmd_args_config.value().workingDirectory = create_absolute_path(
			cmd_args_config.value().workingDirectory, actual_working_path
		);
	}


	if(cmd_args_config.value().verbosity >= pthr::CmdArgsConfig::Verbosity::SOME){
		pthr::print_logo(printer);
		pthr::print_version(printer);
	}


	switch(cmd_args_config.value().action){
		case pthr::CmdArgsConfig::Action::BUILD: {
			if(cmd_args_config.value().verbosity == pthr::CmdArgsConfig::Verbosity::FULL){
				printer.printlnGray(
					"pthr executable directory: \"{}\"", cmd_args_config.value().executablePath.string()
				);

				if(cmd_args_config.value().numBuildThreads.isSingle()){
					printer.printlnGray("Building build system single-threaded");
				}else if(cmd_args_config.value().numBuildThreads.getNum() == 1){
					printer.printlnGray("Building build system multi-threaded (1 worker thread)");
				}else{
					printer.printlnGray(
						"Building build system multi-threaded ({} worker threads)",
						cmd_args_config.value().numBuildThreads.getNum()
					);
				}
			}


			const evo::Result<uint8_t> build_system_result = run_build_system(cmd_args_config.value(), printer);

			if(build_system_result.isError()){ return EXIT_FAILURE; }

			if(build_system_result.value() != 0){
				panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
					panther::Diagnostic::Level::ERROR,
					std::format("Build system errored with code: {}", build_system_result.value()),
					panther::Diagnostic::Location::NONE
				));
				return EXIT_FAILURE;
			}

			if(cmd_args_config.value().verbosity >= pthr::CmdArgsConfig::Verbosity::SOME){
				printer.printlnSuccess("Successfully completed");
			}
		} break;

		case pthr::CmdArgsConfig::Action::RUN: {
			if(cmd_args_config.value().verbosity == pthr::CmdArgsConfig::Verbosity::FULL){
				printer.printlnGray(
					"pthr executable directory: \"{}\"", cmd_args_config.value().executablePath.string()
				);

				if(cmd_args_config.value().numBuildThreads.isSingle()){
					printer.printlnGray("Building run script single-threaded");
				}else if(cmd_args_config.value().numBuildThreads.getNum() == 1){
					printer.printlnGray("Building run script multi-threaded (1 worker thread)");
				}else{
					printer.printlnGray(
						"Building run script multi-threaded ({} worker threads)",
						cmd_args_config.value().numBuildThreads.getNum()
					);
				}
			}

			const evo::Result<uint8_t> run_result = run_scripting(cmd_args_config.value(), printer);

			if(run_result.isError()){ return EXIT_FAILURE; }

			if(run_result.value() != 0){
				panther::printDiagnosticWithoutLocation(printer, panther::Diagnostic(
					panther::Diagnostic::Level::ERROR,
					std::format("Run script errored with code: {}", run_result.value()),
					panther::Diagnostic::Location::NONE
				));
				return EXIT_FAILURE;
			}

			if(cmd_args_config.value().verbosity >= pthr::CmdArgsConfig::Verbosity::SOME){
				printer.printlnSuccess("Successfully completed");
			}
		} break;

		case pthr::CmdArgsConfig::Action::HELP: {
			pthr::print_help(printer);
		} break;

		case pthr::CmdArgsConfig::Action::VERSION: {
			pthr::print_logo(printer);
			pthr::print_version(printer);
		} break;
	}

	return EXIT_SUCCESS;
}