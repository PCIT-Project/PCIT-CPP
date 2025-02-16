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

namespace pcit::panther::BaseType{


	struct PrimitiveID : public core::UniqueID<uint32_t, struct PrimitiveID> {
		using core::UniqueID<uint32_t, PrimitiveID>::UniqueID; 
	};

	struct FunctionID : public core::UniqueID<uint32_t, struct FunctionID> {
		using core::UniqueID<uint32_t, FunctionID>::UniqueID; 
	};

	struct ArrayID : public core::UniqueID<uint32_t, struct ArrayID> {
		using core::UniqueID<uint32_t, ArrayID>::UniqueID; 
	};

	struct AliasID : public core::UniqueID<uint32_t, struct AliasID> {
		using core::UniqueID<uint32_t, AliasID>::UniqueID; 
	};

	struct TypedefID : public core::UniqueID<uint32_t, struct TypedefID> {
		using core::UniqueID<uint32_t, TypedefID>::UniqueID; 
	};

	struct StructID : public core::UniqueID<uint32_t, struct StructID> {
		using core::UniqueID<uint32_t, StructID>::UniqueID; 
	};


}