////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <filesystem>
namespace fs = std::filesystem;

#include <Evo.h>
#include <PCIT_core.h>
#include <Panther.h>
namespace core = pcit::core;
namespace panther = pcit::panther;


namespace pthr{


	auto print_logo(core::Printer& printer) -> void;

	auto print_tokens(core::Printer& printer, const panther::Source& source, const fs::path& relative_dir) -> void;

	auto print_AST(pcit::core::Printer& printer, const panther::Source& source, const fs::path& relative_dir) -> void;

	
}