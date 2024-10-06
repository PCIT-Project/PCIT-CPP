//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////

#pragma once

#include <Evo.h>

#include "../../internal/llvm_interface/include/AP_numbers/APFloat.h"

namespace pcit::core{


	class GenericFloat{
		public:
			explicit GenericFloat(float32_t val) : ap_float(val) {}
			explicit GenericFloat(float64_t val) : ap_float(val) {}

			// EVO_NODISCARD static auto createF16(GenericInt&& value) -> GenericFloat {
			// 	return GenericFloat(llvmint::APFloatBase::IEEEhalf(), std::move(value));
			// }

			// EVO_NODISCARD static auto createBF16(GenericInt&& value) -> GenericFloat {
			// 	return GenericFloat(llvmint::APFloatBase::BFloat(), std::move(value));
			// }

			// EVO_NODISCARD static auto createF32(GenericInt&& value) -> GenericFloat {
			// 	return GenericFloat(llvmint::APFloatBase::IEEEsingle(), std::move(value));
			// }

			// EVO_NODISCARD static auto createF64(GenericInt&& value) -> GenericFloat {
			// 	return GenericFloat(llvmint::APFloatBase::IEEEdouble(), std::move(value));
			// }

			// EVO_NODISCARD static auto createF80(GenericInt&& value) -> GenericFloat {
			// 	return GenericFloat(llvmint::APFloatBase::IEEEquad(), std::move(value));
			// }

			// EVO_NODISCARD static auto createF128(GenericInt&& value) -> GenericFloat {
			// 	return GenericFloat(llvmint::APFloatBase::x87DoubleExtended(), std::move(value));
			// }


			~GenericFloat() = default;


			EVO_NODISCARD auto operator==(const GenericFloat& rhs) const -> bool {
				return this->ap_float == rhs.ap_float;
			}


			EVO_NODISCARD explicit operator float64_t() const { return this->ap_float.convertToDouble(); }
			EVO_NODISCARD explicit operator float32_t() const { return this->ap_float.convertToFloat();  }



		private:
			// GenericFloat(const llvmint::fltSemantics& semantics, GenericInt&& value)
			// 	: ap_float(semantics, value.ap_int) {}

	
		private:
			llvmint::APFloat ap_float;
	};
	
}
