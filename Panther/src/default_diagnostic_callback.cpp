//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "../include/default_diagnostic_callback.h"

namespace pcit::panther{
	

	auto createDefaultDiagnosticCallback(const core::Printer& printer_ref) noexcept -> Context::DiagnosticCallback {
		return [&printer = printer_ref](const Context& context, const Diagnostic& diagnostic) noexcept -> void {

			const std::string diagnostic_message = std::format(
				"<{}|{}> {}\n", diagnostic.level, diagnostic.code, diagnostic.message
			);

			switch(diagnostic.level){
				break; case Diagnostic::Level::Fatal:   printer.printFatal(diagnostic_message);
				break; case Diagnostic::Level::Error:   printer.printError(diagnostic_message);
				break; case Diagnostic::Level::Warning: printer.printWarning(diagnostic_message);
			};


			if(diagnostic.location.has_value()){
				const Source::Location& location = *diagnostic.location;
				const Source& source = context.getSourceManager().getSource(location.sourceID);

				printer.printGray(
					std::format(
						"\t{}:{}:{}\n",
						source.getLocationAsString(),
						location.lineStart,
						location.collumnStart
					)
				);
			}

		};
	};


};