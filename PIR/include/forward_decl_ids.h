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


namespace pcit::pir{


	struct BasicBlockID : public core::UniqueID<uint32_t, struct BasicBlockID> {
		using core::UniqueID<uint32_t, BasicBlockID>::UniqueID;
	};


	struct FunctionID : public core::UniqueID<uint32_t, struct FunctionID> {
		using core::UniqueID<uint32_t, FunctionID>::UniqueID;
	};

	struct FunctionDeclID : public core::UniqueID<uint32_t, struct FunctionDeclID> {
		using core::UniqueID<uint32_t, FunctionDeclID>::UniqueID;
	};


}


