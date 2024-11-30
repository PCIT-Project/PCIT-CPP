////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>
#include <PCIT_core.h>

#include "./enums.h"


namespace pcit::pir{

	auto lowerToLLVMIR(const class Module& module, OptMode opt_mode = OptMode::None) -> std::string;

	auto lowerToAssembly(const class Module& module, OptMode opt_mode = OptMode::None) -> evo::Result<std::string>;

	auto lowerToObject(const class Module& module, OptMode opt_mode = OptMode::None)
		-> evo::Result<std::vector<evo::byte>>;

}

