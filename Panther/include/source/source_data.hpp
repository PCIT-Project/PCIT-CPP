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


#include <Evo.hpp>
#include <PCIT_core.hpp>


namespace pcit::panther{


	//////////////////////////////////////////////////////////////////////
	// source

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


	//////////////////////////////////////////////////////////////////////
	// c family source

	struct CFamilySourceID : public core::UniqueID<uint32_t, struct CFamilySourceID> { 
		using core::UniqueID<uint32_t, CFamilySourceID>::UniqueID;
		using core::UniqueID<uint32_t, CFamilySourceID>::operator==;
	};


	struct CFamilySourceLocation{
		CFamilySourceID sourceID;
		uint32_t lineStart;
		uint32_t lineEnd;
		uint32_t collumnStart;
		uint32_t collumnEnd;


		CFamilySourceLocation(CFamilySourceID source_id, uint32_t line, uint32_t collumn)
			: sourceID(source_id), lineStart(line), lineEnd(line), collumnStart(collumn), collumnEnd(collumn) {}

		CFamilySourceLocation(
			CFamilySourceID source_id,
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


	struct CFamilySourceDeclInfoID : public core::UniqueID<uint32_t, struct CFamilySourceDeclInfoID> { 
		using core::UniqueID<uint32_t, CFamilySourceDeclInfoID>::UniqueID;
		using core::UniqueID<uint32_t, CFamilySourceDeclInfoID>::operator==;
	};


	struct CFamilySourceDeclInfo{
		CFamilySourceLocation location;
		std::string_view name;
	};



	//////////////////////////////////////////////////////////////////////
	// builtin module

	enum class BuiltinModuleID : uint32_t {
		PTHR,
		BUILD,
		CONFIG,
	};

	struct BuiltinModuleStringID : public core::UniqueID<uint32_t, struct BuiltinModuleStringID> { 
		using core::UniqueID<uint32_t, BuiltinModuleStringID>::UniqueID;
		using core::UniqueID<uint32_t, BuiltinModuleStringID>::operator==;
	};

}