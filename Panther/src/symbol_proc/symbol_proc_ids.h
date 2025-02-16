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



namespace pcit::panther{

	// For lookup in Context::symbol_proc_manager
	struct SymbolProcID : public core::UniqueID<uint32_t, struct SymbolProcID> { 
		using core::UniqueID<uint32_t, SymbolProcID>::UniqueID;
	};


	using SymbolProcNamespace = std::unordered_multimap<std::string_view, SymbolProcID>;


}

