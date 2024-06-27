//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include <iostream>

#include <Evo.h>

#include <Panther.h>
namespace panther = pcit::panther;



auto main(int argc, const char* argv[]) -> int {
	auto args = std::vector<std::string_view>(argv, argv + argc);

	auto printer = pcit::core::Printer();

	printer.printCyan("Panther:\n");
	printer.printGray("--------------\n");


	auto context = panther::Context(panther::createDefaultDiagnosticCallback(printer));



	const panther::Source::ID src_id = context.getSourceManager().addSource(
		std::string("example"), std::string("var foo: Int #pub = 12;")
	);


	context.emitDiagnostic(
		panther::Diagnostic::Level::Warning,
		panther::Diagnostic::Code::SemaUnknownIdentifier,
		panther::Source::Location(context.getSourceManager().getSource(src_id).getID(), 1, 1, 5, 8),
		"Unknown identifier"
	);



	#if !defined(PCIT_BUILD_RELEASE) && defined(EVO_COMPILER_MSVC)
		printer.printGray("Press Enter to close...");
		std::cin.get();
	#endif

	return EXIT_SUCCESS;
}