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
		Fatal,
		Error,
		Warning,
		Info,
	};

	EVO_NODISCARD static auto print_diagnostic_level(Diagnostic::Level level) -> std::string_view {
		switch(level){
			case Diagnostic::Level::Fatal:   return "Fatal";
			case Diagnostic::Level::Error:   return "Error";
			case Diagnostic::Level::Warning: return "Warning";
		}

		evo::debugFatalBreak("Unknown or unsupported diagnostic level");
	}

	EVO_NODISCARD static auto get_diagnostic_level(Diagnostic::Level level) -> DiagnosticLevel {
		switch(level){
			case Diagnostic::Level::Fatal:   return DiagnosticLevel::Fatal;
			case Diagnostic::Level::Error:   return DiagnosticLevel::Error;
			case Diagnostic::Level::Warning: return DiagnosticLevel::Warning;
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

		while(source.getData()[cursor] != '\n' && source.getData()[cursor] != '\r' && cursor < source.getData().size()){
			if(remove_whitespace && (source.getData()[cursor] == '\t' || source.getData()[cursor] == ' ')){
				// remove leading whitespace
				point_collumn -= 1;

			}else{
				line_str += source.getData()[cursor];
				remove_whitespace = false;
			}

			cursor += 1;
		}

		if(level == DiagnosticLevel::Info){
			printer.printGray(std::format("\t\t{} | {}\n", line_number_str, line_str));
		}else{
			printer.printGray(std::format("\t{} | {}\n", line_number_str, line_str));
		}


		///////////////////////////////////
		// print formatting space for pointer line

		auto line_space_str = std::string();
		for(size_t i = 0; i < line_number_str.size(); i+=1){
			line_space_str += ' ';
		}

		if(level == DiagnosticLevel::Info){
			printer.printGray(std::format("\t\t{} | ", line_space_str));
		}else{
			printer.printGray(std::format("\t{} | ", line_space_str));
		}


		///////////////////////////////////
		// print pointer str

		auto pointer_str = std::string();

		for(size_t i = 0; i < point_collumn - 1; i+=1){
			pointer_str += ' ';
		}

		if(location.lineStart == location.lineEnd){
			for(uint32_t i = location.collumnStart; i < location.collumnEnd + 1; i+=1){
				pointer_str += '^';
			}
		}else{
			for(size_t i = point_collumn; i < line_str.size() + 1; i+=1){
				if(i == point_collumn){
					pointer_str += '^';
				}else{
					pointer_str += '~';
				}
			}
		}

		pointer_str += '\n';

		switch(level){
			break; case DiagnosticLevel::Fatal:   printer.printError(pointer_str);
			break; case DiagnosticLevel::Error:   printer.printError(pointer_str);
			break; case DiagnosticLevel::Warning: printer.printWarning(pointer_str);
			break; case DiagnosticLevel::Info:    printer.printInfo(pointer_str);
		}
	}


	static auto print_info(
		core::Printer& printer,
		const std::filesystem::path& rel_dir,
		const Context& context,
		const Diagnostic::Info& info,
		unsigned depth
	) -> void {
		for(size_t i = 0; i < depth; i+=1){
			printer.print("\t");
		}

		printer.printCyan(std::format("<Info> {}\n", info.message));

		if(info.location.is<Source::Location>()){
			const Source& source = context.getSourceManager()[info.location.as<Source::Location>().sourceID];
			print_location(
				printer, rel_dir, source, DiagnosticLevel::Info, info.location.as<Source::Location>(), depth + 1
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
				"<{}|{}> {}\n", print_diagnostic_level(diagnostic.level), diagnostic.code, diagnostic.message
			);

			switch(diagnostic.level){
				break; case Diagnostic::Level::Fatal:   printer.printFatal(diagnostic_message);
				break; case Diagnostic::Level::Error:   printer.printError(diagnostic_message);
				break; case Diagnostic::Level::Warning: printer.printWarning(diagnostic_message);
			}

			if(diagnostic.location.is<Source::Location>()){
				const Source::Location& location = diagnostic.location.as<Source::Location>();
				const Source& source = context.getSourceManager()[location.sourceID];

				print_location(printer, rel_dir, source, get_diagnostic_level(diagnostic.level), location, 1);
			}

			for(const Diagnostic::Info& info : diagnostic.infos){
				print_info(printer, rel_dir, context, info, 1);
			}


			if(diagnostic.level == Diagnostic::Level::Fatal){
				printer.printFatal(
					"\tThis is a bug in the compiler.\n"
					"\tPlease report it on Github: https://github.com/PCIT-Project/PCIT-CPP/issues\n"
					"\tGuidelines for creating issues: "
						"https://github.com/PCIT-Project/PCIT-CPP/blob/main/CONTRIBUTING.md#issues\n"
				);
			}

			#if defined(PCIT_BUILD_DEBUG)
				if(diagnostic.level == Diagnostic::Level::Error || diagnostic.level == Diagnostic::Level::Fatal){
					evo::breakpoint();
				}
			#endif
		};
	}


}