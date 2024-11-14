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
#include "./Type.h"
#include "./Expr.h"

namespace pcit::pir{


	struct GlobalVar{
		struct Zeroinit{};
		struct Uninit{};

		std::string name;
		Type type;
		Linkage linkage;
		evo::Variant<Expr, Zeroinit, Uninit> value;
		bool isConstant;
		bool isExternal;

		// For lookup in Module
		struct ID : public core::UniqueID<uint32_t, struct ID> {
			using core::UniqueID<uint32_t, ID>::UniqueID;
		};

	};


}

