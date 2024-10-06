//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
//                                                                  //
//  	Glue code for the adapted LLVM files. All functions are     //
// attributed with the location they came from in the LLVM          //
// codebase. There may also be some modifications                   //
//                                                                  //
//////////////////////////////////////////////////////////////////////



#pragma once

#include <Evo.h>



namespace pcit::llvmint::glue{


	#define LLVM_READONLY
	#define LLVM_READNONE
	#define LLVM_UNLIKELY(x) x


	///////////////////////////////////
	// llvm/ADT/FloatingPointMode.h

	enum class RoundingMode : int8_t {
		// Rounding mode defined in IEEE-754.
		TowardZero        = 0,    ///< roundTowardZero.
		NearestTiesToEven = 1,    ///< roundTiesToEven.
		TowardPositive    = 2,    ///< roundTowardPositive.
		TowardNegative    = 3,    ///< roundTowardNegative.
		NearestTiesToAway = 4,    ///< roundTiesToAway.

		// Special values.
		Dynamic = 7,    ///< Denotes mode unknown at compile time.
		Invalid = -1    ///< Denotes invalid value.
	};


	///////////////////////////////////
	// llvm/Support/MathExtras.h

	EVO_NODISCARD constexpr auto isMask_64(uint64_t Value) -> bool {
		return Value && ((Value + 1) & Value) == 0;
	}

	EVO_NODISCARD constexpr auto isShiftedMask_64(uint64_t Value) -> bool {
		return Value && isMask_64((Value - 1) | Value);
	}

	EVO_NODISCARD constexpr auto isPowerOf2_64(uint64_t Value) -> bool {
		return std::has_single_bit(Value);
	}

	EVO_NODISCARD inline auto isShiftedMask_64(uint64_t Value, unsigned &MaskIdx, unsigned &MaskLen) -> bool {
	  if (!isShiftedMask_64(Value))
	    return false;
	  MaskIdx = std::countr_zero(Value);
	  MaskLen = std::popcount(Value);
	  return true;
	}

	EVO_NODISCARD inline auto SignExtend64(uint64_t X, unsigned B) -> int64_t {
		evo::debugAssert(B > 0, "Bit width can't be 0.");
		evo::debugAssert(B <= 64, "Bit width out of range.");
		return int64_t(X << (64 - B)) >> (64 - B);
	}


	
}
