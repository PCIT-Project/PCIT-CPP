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

#include "./Context.h"

#include <filesystem>

namespace pcit::panther{


	EVO_NODISCARD auto createDefaultDiagnosticCallback(
		pcit::core::Printer& printer_ref, const std::filesystem::path& relative_dir
	) -> Context::DiagnosticCallback;




	auto printDiagnosticWithoutLocation(pcit::core::Printer& printer, const Diagnostic& diagnostic) -> void;



}