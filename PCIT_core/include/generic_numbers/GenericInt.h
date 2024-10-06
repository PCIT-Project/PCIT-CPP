//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////

#pragma once

#include <Evo.h>

#include "../../internal/llvm_interface/include/AP_numbers/APInt.h"


namespace llvm{
	class APInt;
}


namespace pcit::core{


	class GenericInt{
		public:
			GenericInt(evo::uint bit_width, uint64_t value, bool is_signed = false)
				: ap_int(bit_width, value, is_signed) {}

			GenericInt(const llvm::APInt& native) : ap_int(llvmint::APInt::fromNative(native)) {}

			~GenericInt() = default;


			template<class INTEGRAL>
			EVO_NODISCARD static auto create(INTEGRAL num) -> GenericInt {
				static_assert(std::is_integral_v<INTEGRAL>, "must be an integral");
				return GenericInt(sizeof(num) * 8, uint64_t(num), std::is_signed_v<INTEGRAL>);
			}


			EVO_NODISCARD auto operator==(const GenericInt& rhs) const -> bool { return this->ap_int == rhs.ap_int; }

			EVO_NODISCARD explicit operator uint64_t() const { return *this->ap_int.getRawData(); }

			
		private:
			llvmint::APInt ap_int;

			friend class GenericFloat;			
	};
	
}
