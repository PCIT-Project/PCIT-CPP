////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <filesystem>

#include <Evo.h>


namespace pcit::clangint{


	struct DiagnosticList{
		struct Diagnostic{
			enum class Level{
				FATAL,
				ERROR,
				WARNING,
				REMARK,
				NOTE,
				IGNORED,
			};


			struct Location{
				std::filesystem::path filePath;
				uint32_t line;
				uint32_t collumn;
			};


			std::string message;
			Level level;
			std::optional<Location> location;
		};

		evo::SmallVector<Diagnostic> diagnostics{};
	};

}