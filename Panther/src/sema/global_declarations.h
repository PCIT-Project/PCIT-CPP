//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once


#include <PCIT_core.h>

#include "../../include/source_data.h"

namespace pcit::panther{
	class Context;
}


namespace pcit::panther::sema{
	
	auto analyze_global_declarations(Context& context, SourceID source_id) -> bool;
	
	
}