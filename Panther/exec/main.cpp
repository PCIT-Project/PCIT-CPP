//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the Panthera Project, under the Apache License v2.0      //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include <iostream>

#include <Evo.h>

#include <Panther.h>
namespace panther = panthera::panther;


auto main(int argc, const char* argv[]) -> int {
	auto args = std::vector<std::string_view>(argv, argv + argc);

	panther::test();

	#if defined(PANTHERA_CONFIG_DEBUG) && defined(EVO_COMPILER_MSVC)
		evo::printlnGray("Press Enter to close...");
		std::cin.get();
	#endif

	return EXIT_SUCCESS;
}