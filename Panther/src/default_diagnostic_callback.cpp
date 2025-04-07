////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/default_diagnostic_callback.h"

namespace pcit::panther{


	enum class DiagnosticLevel{
		FATAL,
		ERROR,
		WARNING,
		INFO,
	};



	EVO_NODISCARD static auto get_diagnostic_level(Diagnostic::Level level) -> DiagnosticLevel {
		switch(level){
			case Diagnostic::Level::FATAL:   return DiagnosticLevel::FATAL;
			case Diagnostic::Level::ERROR:   return DiagnosticLevel::ERROR;
			case Diagnostic::Level::WARNING: return DiagnosticLevel::WARNING;
		}

		evo::debugFatalBreak("Unknown or unsupported diagnostic level");
	}


	static auto print_location(
		core::Printer& printer,
		const std::filesystem::path& rel_dir,
		const Source& source,
		DiagnosticLevel level,
		const Source::Location& location,
		unsigned depth
	) -> void {

		///////////////////////////////////
		// print file location

		for(size_t i = 0; i < depth; i+=1){
			printer.print("\t");
		}

		std::error_code ec;

		// clickable link in terminal: https://stackoverflow.com/a/66814614
		printer.printGray(
			std::format(
				"\x1B]8;;file://{}\x1B\\{}\x1B]8;;\x1B\\({}:{})\n", 
				source.getPath().string(),
				std::filesystem::relative(source.getPath(), rel_dir, ec).string(),
				location.lineStart,
				location.collumnStart
			)
		);

		evo::debugAssert(ec.value() == 0, "Error getting relative path");

		const std::string line_number_str = std::to_string(location.lineStart);


		///////////////////////////////////
		// find line in the source code

		size_t cursor = 0;
		size_t current_line = 1;
		while(current_line < location.lineStart){
			evo::debugAssert(
				cursor < source.getData().size(), "out of bounds looking for line in source code for error"
			);

			if(source.getData()[cursor] == '\n'){
				current_line += 1;

			}else if(source.getData()[cursor] == '\r'){
				current_line += 1;

				if(source.getData()[cursor + 1] == '\n'){
					cursor += 1;
				}
			}

			cursor += 1;
		}


		///////////////////////////////////
		// get actual line and remove leading whitespace

		auto line_str = std::string{};
		size_t point_collumn = location.collumnStart;
		bool remove_whitespace = true;

		auto line_char_is_tab = evo::SmallVector<bool>();

		while(source.getData()[cursor] != '\n' && source.getData()[cursor] != '\r' && cursor < source.getData().size()){
			if(remove_whitespace && (source.getData()[cursor] == '\t' || source.getData()[cursor] == ' ')){
				// remove leading whitespace
				point_collumn -= 1;

			}else{
				if(source.getData()[cursor] == '\t'){
					line_str += "    ";
					line_char_is_tab.emplace_back(true);
				}else{
					line_str += source.getData()[cursor];
					line_char_is_tab.emplace_back(false);
				}
				remove_whitespace = false;
			}

			cursor += 1;
		}

		line_char_is_tab.emplace_back(false);



		for(size_t i = 0; i < depth; i+=1){
			printer.print("\t");
		}
		printer.printGray(std::format("{} | {}\n", line_number_str, line_str));


		///////////////////////////////////
		// print formatting space for pointer line

		auto line_space_str = std::string();
		for(size_t i = 0; i < depth; i+=1){
			line_space_str += '\t';
		}
		for(size_t i = 0; i < line_number_str.size(); i+=1){
			line_space_str += ' ';
		}
		printer.printGray(std::format("{} | ", line_space_str));


		///////////////////////////////////
		// print pointer str

		auto pointer_str = std::string();

		for(size_t i = 0; i < point_collumn - 1; i+=1){
			if(line_char_is_tab[i]){
				pointer_str += "    ";
			}else{
				pointer_str += ' ';
			}
		}

		if(location.lineStart == location.lineEnd){
			for(uint32_t i = location.collumnStart; i < location.collumnEnd + 1; i+=1){
				if(line_char_is_tab[i - (location.collumnStart - point_collumn)]){
					pointer_str += "^^^^";
				}else{
					pointer_str += '^';
				}
			}
		}else{
			for(size_t i = point_collumn; i < line_char_is_tab.size(); i+=1){
				if(line_char_is_tab[i]){
					pointer_str += "^^^^";
				}else{
					pointer_str += '^';
				}
			}
			pointer_str += "-->";
		}

		pointer_str += '\n';

		switch(level){
			break; case DiagnosticLevel::FATAL:   printer.printError(pointer_str);
			break; case DiagnosticLevel::ERROR:   printer.printError(pointer_str);
			break; case DiagnosticLevel::WARNING: printer.printWarning(pointer_str);
			break; case DiagnosticLevel::INFO:    printer.printInfo(pointer_str);
		}
	}


	static auto print_info(
		core::Printer& printer,
		const std::filesystem::path* rel_dir,
		const Context* context,
		const Diagnostic::Info& info,
		unsigned depth
	) -> void {
		for(size_t i = 0; i < depth; i+=1){
			printer.print("\t");
		}

		printer.printCyan(std::format("<Info> {}\n", info.message));

		if(info.location.is<Source::Location>()){
			// message is this way to make sense when called from `printDiagnosticWithoutLocation`
			evo::debugAssert(context != nullptr, "Cannot print diagnostic info with location with this function");

			const Source& source = context->getSourceManager()[info.location.as<Source::Location>().sourceID];
			print_location(
				printer, *rel_dir, source, DiagnosticLevel::INFO, info.location.as<Source::Location>(), depth + 1
			);
		}

		for(const Diagnostic::Info& sub_info : info.sub_infos){
			print_info(printer, rel_dir, context, sub_info, depth + 1);
		}
	}

	

	auto createDefaultDiagnosticCallback(core::Printer& printer_ref, const std::filesystem::path& relative_dir)
	-> Context::DiagnosticCallback {
		return [&printer = printer_ref, &rel_dir = relative_dir](const Context& context, const Diagnostic& diagnostic) 
		-> void {
			const std::string diagnostic_message = std::format(
				"<{}|{}> {}\n", Diagnostic::printLevel(diagnostic.level), diagnostic.code, diagnostic.message
			);

			switch(diagnostic.level){
				break; case Diagnostic::Level::FATAL:   printer.printFatal(diagnostic_message);
				break; case Diagnostic::Level::ERROR:   printer.printError(diagnostic_message);
				break; case Diagnostic::Level::WARNING: printer.printWarning(diagnostic_message);
			}

			if(diagnostic.location.is<Source::Location>()){
				const Source::Location& location = diagnostic.location.as<Source::Location>();
				const Source& source = context.getSourceManager()[location.sourceID];

				print_location(printer, rel_dir, source, get_diagnostic_level(diagnostic.level), location, 1);
			}

			for(const Diagnostic::Info& info : diagnostic.infos){
				print_info(printer, &rel_dir, &context, info, 1);
			}


			if(diagnostic.level == Diagnostic::Level::FATAL){
				printer.printFatal(
					"\tThis is a bug in the compiler.\n"
					"\tPlease report it on Github: https://github.com/PCIT-Project/PCIT-CPP/issues\n"
					"\tGuidelines for creating issues: "
						"https://github.com/PCIT-Project/PCIT-CPP/blob/main/CONTRIBUTING.md#issues\n"
				);
			}

			#if defined(PCIT_BUILD_DEBUG)
				if(diagnostic.level == Diagnostic::Level::ERROR || diagnostic.level == Diagnostic::Level::FATAL){
					evo::breakpoint();
				}
			#endif
		};
	}




	auto printDiagnosticWithoutLocation(pcit::core::Printer& printer, const Diagnostic& diagnostic) -> void {
		const std::string diagnostic_message = std::format(
			"<{}|{}> {}\n", Diagnostic::printLevel(diagnostic.level), diagnostic.code, diagnostic.message
		);

		switch(diagnostic.level){
			break; case Diagnostic::Level::FATAL:   printer.printFatal(diagnostic_message);
			break; case Diagnostic::Level::ERROR:   printer.printError(diagnostic_message);
			break; case Diagnostic::Level::WARNING: printer.printWarning(diagnostic_message);
		}

		evo::debugAssert(
			diagnostic.location.is<Diagnostic::Location::None>(),
			"Cannot print diagnostic with location with this function"
		);

		for(const Diagnostic::Info& info : diagnostic.infos){
			print_info(printer, nullptr, nullptr, info, 1);
		}

		if(diagnostic.level == Diagnostic::Level::FATAL){
			printer.printFatal(
				"\tThis is a bug in the compiler.\n"
				"\tPlease report it on Github: https://github.com/PCIT-Project/PCIT-CPP/issues\n"
				"\tGuidelines for creating issues: "
					"https://github.com/PCIT-Project/PCIT-CPP/blob/main/CONTRIBUTING.md#issues\n"
			);
		}

		#if defined(PCIT_BUILD_DEBUG)
			if(diagnostic.level == Diagnostic::Level::ERROR || diagnostic.level == Diagnostic::Level::FATAL){
				evo::breakpoint();
			}
		#endif
	}


}