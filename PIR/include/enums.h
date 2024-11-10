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



namespace pcit::pir{

	enum class CallingConvention{
		Default, // C

		C,
		Fast,
		Cold,
	};


	enum class Linkage{
		Default, // Internal

		Private,
		Internal, // Like private, but shows up as a local symbol in the object file
		External,
	};

}

