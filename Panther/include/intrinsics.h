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

#include "./type_ids.h"

namespace pcit::panther{

	
	namespace IntrinsicFunc{
		enum class Kind {
			ABORT,
			BREAKPOINT,
			BUILD_SET_NUM_THREADS,
			BUILD_SET_OUTPUT,
			BUILD_SET_USE_STD_LIB,

			_max_,
		};

		EVO_NODISCARD auto lookupKind(std::string_view name) -> std::optional<Kind>;
		auto initLookupTableIfNeeded() -> void;
	};


}