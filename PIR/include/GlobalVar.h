//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>
#include <PCIT_core.h>


#include "./enums.h"
#include "./Type.h"
#include "./Expr.h"

namespace pcit::pir{


	struct GlobalVar{
		std::string name;
		Type type;
		Linkage linkage;
		std::optional<Expr> value;
		bool isConstant;
		bool isExternal;

		// For lookup in Module
		struct ID : public core::UniqueID<uint32_t, struct ID> {
			using core::UniqueID<uint32_t, ID>::UniqueID;
		};

	};


}

