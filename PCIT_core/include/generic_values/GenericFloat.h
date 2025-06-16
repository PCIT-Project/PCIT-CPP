////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Evo.h>

#include "../../internal/llvm_interface/include/AP_numbers/APFloat.h"
#include "./GenericInt.h"

namespace pcit::core{


	class GenericFloat{
		public:
			explicit GenericFloat(evo::float32_t val) : ap_float(val) {}
			explicit GenericFloat(evo::float64_t val) : ap_float(val) {}

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
			EVO_NODISCARD static auto createF32(evo::float32_t value) -> GenericFloat {
				return GenericFloat(value);
			}

			EVO_NODISCARD static auto createF64(GenericInt&& value) -> GenericFloat {
				return GenericFloat(llvmint::APFloatBase::IEEEdouble(), std::move(value));
			}
			EVO_NODISCARD static auto createF64(evo::float64_t value) -> GenericFloat {
				return GenericFloat(value);
			}

			EVO_NODISCARD static auto createF80(GenericInt&& value) -> GenericFloat {
				return GenericFloat(llvmint::APFloatBase::x87DoubleExtended(), std::move(value));
			}
			EVO_NODISCARD static auto createF80(evo::float64_t value) -> GenericFloat {
				return GenericFloat::createF64(value).asF80();
			}

			EVO_NODISCARD static auto createF128(GenericInt&& value) -> GenericFloat {
				return GenericFloat(llvmint::APFloatBase::IEEEquad(), std::move(value));
			}
			EVO_NODISCARD static auto createF128(evo::float64_t value) -> GenericFloat {
				return GenericFloat::createF64(value).asF128();
			}



			EVO_NODISCARD static auto createBF16FromInt(const GenericInt& value, bool is_signed) -> GenericFloat {
				auto output = createBF16(0);
				output.ap_float.convertFromAPInt(
					value.getNative(), is_signed, llvmint::glue::RoundingMode::TowardZero
				);
				return GenericFloat(std::move(output));
			}

			EVO_NODISCARD static auto createF16FromInt(const GenericInt& value, bool is_signed) -> GenericFloat {
				auto output = createF16(0);
				output.ap_float.convertFromAPInt(
					value.getNative(), is_signed, llvmint::glue::RoundingMode::TowardZero
				);
				return GenericFloat(std::move(output));
			}

			EVO_NODISCARD static auto createF32FromInt(const GenericInt& value, bool is_signed) -> GenericFloat {
				auto output = createF32(0);
				output.ap_float.convertFromAPInt(
					value.getNative(), is_signed, llvmint::glue::RoundingMode::TowardZero
				);
				return GenericFloat(std::move(output));
			}

			EVO_NODISCARD static auto createF64FromInt(const GenericInt& value, bool is_signed) -> GenericFloat {
				auto output = createF64(0);
				output.ap_float.convertFromAPInt(
					value.getNative(), is_signed, llvmint::glue::RoundingMode::TowardZero
				);
				return GenericFloat(std::move(output));
			}

			EVO_NODISCARD static auto createF80FromInt(const GenericInt& value, bool is_signed) -> GenericFloat {
				auto output = createF80(0);
				output.ap_float.convertFromAPInt(
					value.getNative(), is_signed, llvmint::glue::RoundingMode::TowardZero
				);
				return GenericFloat(std::move(output));
			}

			EVO_NODISCARD static auto createF128FromInt(const GenericInt& value, bool is_signed) -> GenericFloat {
				auto output = createF128(0);
				output.ap_float.convertFromAPInt(
					value.getNative(), is_signed, llvmint::glue::RoundingMode::TowardZero
				);
				return GenericFloat(std::move(output));
			}


			~GenericFloat() = default;


			EVO_NODISCARD auto operator==(const GenericFloat& rhs) const -> bool {
				return this->ap_float == rhs.ap_float;
			}


			EVO_NODISCARD explicit operator evo::float64_t() const {
				return this->asF64().ap_float.convertToDouble();
			}
			EVO_NODISCARD explicit operator evo::float32_t() const {
				return this->asF32().ap_float.convertToFloat(); 
			}


			EVO_NODISCARD auto toString() const -> std::string {
				auto output = std::string();
				this->ap_float.toString(output);
				return output;
			}


			EVO_NODISCARD auto getNative() const -> const llvmint::APFloat& { return this->ap_float; }



			EVO_NODISCARD auto nextUp() const -> GenericFloat {
				llvmint::APFloat ap_float_copy = this->ap_float;
				std::ignore = ap_float_copy.next(false);
				return GenericFloat(std::move(ap_float_copy));
			}

			EVO_NODISCARD auto nextDown() const -> GenericFloat {
				llvmint::APFloat ap_float_copy = this->ap_float;
				std::ignore = ap_float_copy.next(true);
				return GenericFloat(std::move(ap_float_copy));
			}



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

			EVO_NODISCARD auto neg() const -> GenericFloat {
				llvmint::APFloat ap_float_copy = this->ap_float;
				ap_float_copy.changeSign();
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



			EVO_NODISCARD auto bitCastToGenericInt() const -> GenericInt {
				return this->ap_float.bitcastToAPInt();
			}

			EVO_NODISCARD auto toGenericInt(unsigned width, bool is_signed) const -> GenericInt {
				auto output = core::GenericInt(width, 0, is_signed);
				bool is_exact;
				this->ap_float.convertToInteger(
					output.data_span(), width, is_signed, llvmint::glue::RoundingMode::TowardZero, &is_exact
				);
				return output;
			}


			// Since LLVM native declaration is not accessable (purposely), you'll neet o bitcast it yourself
			// It is safe to do so.
			EVO_NODISCARD auto copyToLLVMNative() const -> std::array<std::byte, sizeof(llvmint::APFloat)> {
				return this->ap_float.copyToLLVMNative();
			}



		private:
			GenericFloat(const llvmint::fltSemantics& semantics, GenericInt&& value)
				: ap_float(semantics, value.ap_int) {}

			GenericFloat(llvmint::APFloat&& _ap_float) : ap_float(_ap_float) {}

	
		private:
			llvmint::APFloat ap_float;
	};
	
}
