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



namespace pcit::pir::meta{

	struct ID : public core::UniqueID<uint32_t, struct ID> {
		using core::UniqueID<uint32_t, ID>::UniqueID;
	};


	enum class Language{
		PANTHER,
		C,
		CPP, 
	};

	
	struct File{
		struct ID : public core::UniqueID<uint32_t, struct ID> {
			using core::UniqueID<uint32_t, ID>::UniqueID;
		};


		meta::ID metaID;
		std::string path;
		Language language;
		std::string producerName;
	};


}


