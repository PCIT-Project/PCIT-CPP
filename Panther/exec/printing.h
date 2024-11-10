////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>

#include <Panther.h>
namespace panther = pcit::panther;


namespace pthr{

	auto printTitle(pcit::core::Printer& printer) -> void;

	auto printTokens(pcit::core::Printer& printer, const panther::Source& source) -> void;

	auto printAST(pcit::core::Printer& printer, const panther::Source& source) -> void;


}