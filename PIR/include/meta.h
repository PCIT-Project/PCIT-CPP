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


#include "./meta_ids.h"
#include "./Type.h"



namespace pcit::pir::meta{


	enum class Language{
		PANTHER,
		C,
		CPP, 
	};

	
	struct File{
		using ID = FileID;

		ItemID itemID;
		std::string path;
		Language language;
		std::string producerName;
	};


	struct BasicType{
		using ID = BasicTypeID;

		ItemID itemID;
		std::string name;
		pir::Type underlyingType;
	};


	struct QualifiedType{
		using ID = QualifiedTypeID;

		enum class Qualifier{
			POINTER,
			MUT_POINTER,
		};

		ItemID itemID;
		std::string name;
		meta::Type qualeeType;
		Qualifier qualifier;
	};


	struct StructType{
		using ID = StructTypeID;

		struct Member{
			meta::Type type;
			std::string name;
		};

		ItemID itemID;
		pir::Type structType;
		std::string name;
		meta::FileID fileID;
		meta::Scope scopeWhereDefined;
		uint32_t lineNumber;
		evo::SmallVector<Member> members;
	};


	struct SourceLocation{
		LocalScope scope;
		uint32_t line;
		uint32_t collumn;
	};

}