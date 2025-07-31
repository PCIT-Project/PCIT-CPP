////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
//                                                                  //
// This file is just for forward declaration to prevent circular    //
// includes. These types are not intended to be used directly by    //
// user, rather use their respective aliases from `Source`          //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>
#include <PCIT_core.h>


namespace pcit::panther{


	struct SourceID : public core::UniqueID<uint32_t, struct SourceID> { 
		using core::UniqueID<uint32_t, SourceID>::UniqueID;
		using core::UniqueID<uint32_t, SourceID>::operator==;
	};


	struct SourceLocation{
		SourceID sourceID;
		uint32_t lineStart;
		uint32_t lineEnd;
		uint32_t collumnStart;
		uint32_t collumnEnd;


		SourceLocation(SourceID source_id, uint32_t line, uint32_t collumn)
			: sourceID(source_id), lineStart(line), lineEnd(line), collumnStart(collumn), collumnEnd(collumn) {}

		SourceLocation(
			SourceID source_id, uint32_t line_start, uint32_t line_end, uint32_t collumn_start, uint32_t collumn_end
		) : 
			sourceID(source_id),
			lineStart(line_start),
			lineEnd(line_end),
			collumnStart(collumn_start),
			collumnEnd(collumn_end)
		{}
	};



	struct ClangSourceID : public core::UniqueID<uint32_t, struct ClangSourceID> { 
		using core::UniqueID<uint32_t, ClangSourceID>::UniqueID;
		using core::UniqueID<uint32_t, ClangSourceID>::operator==;
	};


	struct ClangSourceLocation{
		ClangSourceID sourceID;
		uint32_t lineStart;
		uint32_t lineEnd;
		uint32_t collumnStart;
		uint32_t collumnEnd;


		ClangSourceLocation(ClangSourceID source_id, uint32_t line, uint32_t collumn)
			: sourceID(source_id), lineStart(line), lineEnd(line), collumnStart(collumn), collumnEnd(collumn) {}

		ClangSourceLocation(
			ClangSourceID source_id,
			uint32_t line_start,
			uint32_t line_end,
			uint32_t collumn_start,
			uint32_t collumn_end
		) : 
			sourceID(source_id),
			lineStart(line_start),
			lineEnd(line_end),
			collumnStart(collumn_start),
			collumnEnd(collumn_end)
		{}
	};


	struct ClangSourceDeclInfoID : public core::UniqueID<uint32_t, struct ClangSourceDeclInfoID> { 
		using core::UniqueID<uint32_t, ClangSourceDeclInfoID>::UniqueID;
		using core::UniqueID<uint32_t, ClangSourceDeclInfoID>::operator==;
	};


	struct ClangSourceDeclInfo{
		ClangSourceLocation location;
		std::string_view name;
	};

}