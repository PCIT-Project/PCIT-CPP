//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>
#include <PCIT_core.h>

#include "./AST.h"
#include "./TypeManager.h"


namespace pcit::panther::ASG{

	struct Func{
		class ID : public core::UniqueID<uint32_t, class ID> {
			public:
				using core::UniqueID<uint32_t, ID>::UniqueID;
		};


		AST::Node name;
		BaseType::ID baseTypeID;
	};

}

