////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once



namespace pcit::core{

	[[nodiscard]] constexpr auto ceilToPowOf2Multiple(size_t num, size_t multiple) -> size_t {
		return (num + (multiple - 1)) & ~(multiple - 1);
	}

}