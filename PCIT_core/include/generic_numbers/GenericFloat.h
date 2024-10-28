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

			EVO_NODISCARD static auto createF16(GenericInt&& value) -> GenericFloat {
				return GenericFloat(llvmint::APFloatBase::IEEEhalf(), std::move(value));
			}
			EVO_NODISCARD static auto createF16(uint16_t value) -> GenericFloat {
				return createF16(GenericInt(16, value));
			}

			EVO_NODISCARD static auto createBF16(GenericInt&& value) -> GenericFloat {
				return GenericFloat(llvmint::APFloatBase::BFloat(), std::move(value));
			}
			EVO_NODISCARD static auto createBF16(uint16_t value) -> GenericFloat {
				return createBF16(GenericInt(16, value));
			}

			EVO_NODISCARD static auto createF32(GenericInt&& value) -> GenericFloat {
				return GenericFloat(llvmint::APFloatBase::IEEEsingle(), std::move(value));
			}
			EVO_NODISCARD static auto createF32(uint32_t value) -> GenericFloat {
				return GenericFloat(std::bit_cast<float32_t>(value));
			}

			EVO_NODISCARD static auto createF64(GenericInt&& value) -> GenericFloat {
				return GenericFloat(llvmint::APFloatBase::IEEEdouble(), std::move(value));
			}
			EVO_NODISCARD static auto createF64(uint64_t value) -> GenericFloat {
				return GenericFloat(std::bit_cast<float64_t>(value));
			}

			EVO_NODISCARD static auto createF80(GenericInt&& value) -> GenericFloat {
				return GenericFloat(llvmint::APFloatBase::x87DoubleExtended(), std::move(value));
			}

			EVO_NODISCARD static auto createF128(GenericInt&& value) -> GenericFloat {
				return GenericFloat(llvmint::APFloatBase::IEEEquad(), std::move(value));
			}


			~GenericFloat() = default;


			EVO_NODISCARD auto operator==(const GenericFloat& rhs) const -> bool {
				return this->ap_float == rhs.ap_float;
			}


			EVO_NODISCARD explicit operator float64_t() const { return this->ap_float.convertToDouble(); }
			EVO_NODISCARD explicit operator float32_t() const { return this->ap_float.convertToFloat();  }


			//////////////////////////////////////////////////////////////////////
			// Arithmetic operations


			EVO_NODISCARD auto add(const GenericFloat& rhs) const -> GenericFloat {
				llvmint::APFloat ap_float_copy = this->ap_float;
				std::ignore = ap_float_copy.add(rhs.ap_float, llvmint::glue::RoundingMode::TowardZero);
				return GenericFloat(std::move(ap_float_copy));
			}

			EVO_NODISCARD auto sub(const GenericFloat& rhs) const -> GenericFloat {
				llvmint::APFloat ap_float_copy = this->ap_float;
				std::ignore = ap_float_copy.subtract(rhs.ap_float, llvmint::glue::RoundingMode::TowardZero);
				return GenericFloat(std::move(ap_float_copy));
			}

			EVO_NODISCARD auto mul(const GenericFloat& rhs) const -> GenericFloat {
				llvmint::APFloat ap_float_copy = this->ap_float;
				std::ignore = ap_float_copy.multiply(rhs.ap_float, llvmint::glue::RoundingMode::TowardZero);
				return GenericFloat(std::move(ap_float_copy));
			}

			EVO_NODISCARD auto div(const GenericFloat& rhs) const -> GenericFloat {
				llvmint::APFloat ap_float_copy = this->ap_float;
				std::ignore = ap_float_copy.divide(rhs.ap_float, llvmint::glue::RoundingMode::TowardZero);
				return GenericFloat(std::move(ap_float_copy));
			}

			EVO_NODISCARD auto rem(const GenericFloat& rhs) const -> GenericFloat {
				llvmint::APFloat ap_float_copy = this->ap_float;
				std::ignore = ap_float_copy.mod(rhs.ap_float);
				return GenericFloat(std::move(ap_float_copy));
			}


			//////////////////////////////////////////////////////////////////////
			// logical

			EVO_NODISCARD auto eq(const GenericFloat& rhs)  const -> bool { return this->ap_float == rhs.ap_float; }
			EVO_NODISCARD auto neq(const GenericFloat& rhs) const -> bool { return this->ap_float != rhs.ap_float; }
			EVO_NODISCARD auto lt(const GenericFloat& rhs)  const -> bool { return this->ap_float < rhs.ap_float;  }
			EVO_NODISCARD auto le(const GenericFloat& rhs)  const -> bool { return this->ap_float <= rhs.ap_float; }
			EVO_NODISCARD auto gt(const GenericFloat& rhs)  const -> bool { return this->ap_float > rhs.ap_float;  }
			EVO_NODISCARD auto ge(const GenericFloat& rhs)  const -> bool { return this->ap_float >= rhs.ap_float; }



			//////////////////////////////////////////////////////////////////////
			// type conversions

			EVO_NODISCARD auto asF16() const -> GenericFloat {
				llvmint::APFloat ap_float_copy = this->ap_float;
				bool loses_info;
				ap_float_copy.convert(
					llvmint::APFloatBase::IEEEhalf(), llvmint::glue::RoundingMode::TowardZero, &loses_info
				);
				return ap_float_copy;
			}

			EVO_NODISCARD auto asBF16() const -> GenericFloat {
				llvmint::APFloat ap_float_copy = this->ap_float;
				bool loses_info;
				ap_float_copy.convert(
					llvmint::APFloatBase::BFloat(), llvmint::glue::RoundingMode::TowardZero, &loses_info
				);
				return ap_float_copy;
			}

			EVO_NODISCARD auto asF32() const -> GenericFloat {
				llvmint::APFloat ap_float_copy = this->ap_float;
				bool loses_info;
				ap_float_copy.convert(
					llvmint::APFloatBase::IEEEsingle(), llvmint::glue::RoundingMode::TowardZero, &loses_info
				);
				return ap_float_copy;
			}

			EVO_NODISCARD auto asF64() const -> GenericFloat {
				llvmint::APFloat ap_float_copy = this->ap_float;
				bool loses_info;
				ap_float_copy.convert(
					llvmint::APFloatBase::IEEEdouble(), llvmint::glue::RoundingMode::TowardZero, &loses_info
				);
				return ap_float_copy;
			}

			EVO_NODISCARD auto asF80() const -> GenericFloat {
				llvmint::APFloat ap_float_copy = this->ap_float;
				bool loses_info;
				ap_float_copy.convert(
					llvmint::APFloatBase::x87DoubleExtended(), llvmint::glue::RoundingMode::TowardZero, &loses_info
				);
				return ap_float_copy;
			}

			EVO_NODISCARD auto asF128() const -> GenericFloat {
				llvmint::APFloat ap_float_copy = this->ap_float;
				bool loses_info;
				ap_float_copy.convert(
					llvmint::APFloatBase::IEEEquad(), llvmint::glue::RoundingMode::TowardZero, &loses_info
				);
				return ap_float_copy;
			}



		private:
			GenericFloat(const llvmint::fltSemantics& semantics, GenericInt&& value)
				: ap_float(semantics, value.ap_int) {}

			GenericFloat(llvmint::APFloat&& _ap_float) : ap_float(_ap_float) {}

	
		private:
			llvmint::APFloat ap_float;
	};
	
}
