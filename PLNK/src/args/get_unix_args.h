////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>

#include "./Args.h"
#include "../../include/Options.h"

#include <filesystem>


namespace pcit::plnk{

	
	auto get_unix_args(evo::ArrayProxy<std::filesystem::path> object_file_paths, const Options& options) -> Args;


}


