//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


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


	class SourceID : public core::UniqueComparableID<uint32_t, class ID> {
		public:
			using core::UniqueComparableID<uint32_t, class ID>::UniqueComparableID;
			using Iterator = IteratorImpl<SourceID>;
	};


	struct SourceLocation{
		SourceID sourceID;
		uint32_t lineStart;
		uint32_t lineEnd;
		uint16_t collumnStart;
		uint16_t collumnEnd;


		SourceLocation(SourceID source_id, uint32_t line, uint16_t collumn) noexcept
			: sourceID(source_id), lineStart(line), lineEnd(line), collumnStart(collumn), collumnEnd(collumn) {};

		SourceLocation(
			SourceID source_id, uint32_t line_start, uint32_t line_end, uint16_t collumn_start, uint16_t collumn_end
		) noexcept 
			: sourceID(source_id),
			  lineStart(line_start),
			  lineEnd(line_end),
			  collumnStart(collumn_start),
			  collumnEnd(collumn_end)
			{};
	};

};