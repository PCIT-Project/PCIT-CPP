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
			GenericInt(unsigned bit_width, uint64_t value, bool is_signed = false)
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
			EVO_NODISCARD explicit operator uint32_t() const { return uint32_t(*this->ap_int.getRawData()); }


			EVO_NODISCARD auto getBitWidth() const -> unsigned { return this->ap_int.getBitWidth(); }


			//////////////////////////////////////////////////////////////////////
			// arithmetic operations

			template<class RESULT>
			struct WrapResultImpl{
				RESULT result;
				bool wrapped;
			};
			using WrapResult = WrapResultImpl<GenericInt>; // This is to get around time of definition


			///////////////////////////////////
			// add

			EVO_NODISCARD auto uadd(const GenericInt& rhs) const -> WrapResult {
				bool wrapped;
				llvmint::APInt result = this->ap_int.uadd_ov(rhs.ap_int, wrapped);
				return WrapResult(GenericInt(std::move(result)), wrapped);
			}

			EVO_NODISCARD auto uaddSat(const GenericInt& rhs) const -> GenericInt {
				return GenericInt(this->ap_int.uadd_sat(rhs.ap_int));
			}


			EVO_NODISCARD auto sadd(const GenericInt& rhs) const -> WrapResult {
				bool wrapped;
				llvmint::APInt result = this->ap_int.sadd_ov(rhs.ap_int, wrapped);
				return WrapResult(GenericInt(std::move(result)), wrapped);
			}

			EVO_NODISCARD auto saddSat(const GenericInt& rhs) const -> GenericInt {
				return GenericInt(this->ap_int.sadd_sat(rhs.ap_int));
			}


			///////////////////////////////////
			// sub

			EVO_NODISCARD auto usub(const GenericInt& rhs) const -> WrapResult {
				bool wrapped;
				llvmint::APInt result = this->ap_int.usub_ov(rhs.ap_int, wrapped);
				return WrapResult(GenericInt(std::move(result)), wrapped);
			}

			EVO_NODISCARD auto usubSat(const GenericInt& rhs) const -> GenericInt {
				return GenericInt(this->ap_int.usub_sat(rhs.ap_int));
			}


			EVO_NODISCARD auto ssub(const GenericInt& rhs) const -> WrapResult {
				bool wrapped;
				llvmint::APInt result = this->ap_int.ssub_ov(rhs.ap_int, wrapped);
				return WrapResult(GenericInt(std::move(result)), wrapped);
			}

			EVO_NODISCARD auto ssubSat(const GenericInt& rhs) const -> GenericInt {
				return GenericInt(this->ap_int.ssub_sat(rhs.ap_int));
			}


			///////////////////////////////////
			// mul

			EVO_NODISCARD auto umul(const GenericInt& rhs) const -> WrapResult {
				bool wrapped;
				llvmint::APInt result = this->ap_int.umul_ov(rhs.ap_int, wrapped);
				return WrapResult(GenericInt(std::move(result)), wrapped);
			}

			EVO_NODISCARD auto umulSat(const GenericInt& rhs) const -> GenericInt {
				return GenericInt(this->ap_int.umul_sat(rhs.ap_int));
			}


			EVO_NODISCARD auto smul(const GenericInt& rhs) const -> WrapResult {
				bool wrapped;
				llvmint::APInt result = this->ap_int.smul_ov(rhs.ap_int, wrapped);
				return WrapResult(GenericInt(std::move(result)), wrapped);
			}

			EVO_NODISCARD auto smulSat(const GenericInt& rhs) const -> GenericInt {
				return GenericInt(this->ap_int.smul_sat(rhs.ap_int));
			}


			///////////////////////////////////
			// div / rem

			EVO_NODISCARD auto udiv(const GenericInt& rhs) const -> GenericInt {
				return GenericInt(this->ap_int.udiv(rhs.ap_int));
			}

			EVO_NODISCARD auto sdiv(const GenericInt& rhs) const -> GenericInt {
				return GenericInt(this->ap_int.sdiv(rhs.ap_int));
			}

			EVO_NODISCARD auto urem(const GenericInt& rhs) const -> GenericInt {
				return GenericInt(this->ap_int.urem(rhs.ap_int));
			}

			EVO_NODISCARD auto srem(const GenericInt& rhs) const -> GenericInt {
				return GenericInt(this->ap_int.srem(rhs.ap_int));
			}


			///////////////////////////////////
			// logical

			EVO_NODISCARD auto eq(const GenericInt& rhs)  const -> bool { return this->ap_int.eq(rhs.ap_int); }
			EVO_NODISCARD auto neq(const GenericInt& rhs) const -> bool { return this->ap_int.ne(rhs.ap_int); }

			EVO_NODISCARD auto ult(const GenericInt& rhs) const -> bool { return this->ap_int.ult(rhs.ap_int); }
			EVO_NODISCARD auto ule(const GenericInt& rhs) const -> bool { return this->ap_int.ule(rhs.ap_int); }
			EVO_NODISCARD auto ugt(const GenericInt& rhs) const -> bool { return this->ap_int.ugt(rhs.ap_int); }
			EVO_NODISCARD auto uge(const GenericInt& rhs) const -> bool { return this->ap_int.uge(rhs.ap_int); }

			EVO_NODISCARD auto slt(const GenericInt& rhs) const -> bool { return this->ap_int.slt(rhs.ap_int); }
			EVO_NODISCARD auto sle(const GenericInt& rhs) const -> bool { return this->ap_int.sle(rhs.ap_int); }
			EVO_NODISCARD auto sgt(const GenericInt& rhs) const -> bool { return this->ap_int.sgt(rhs.ap_int); }
			EVO_NODISCARD auto sge(const GenericInt& rhs) const -> bool { return this->ap_int.sge(rhs.ap_int); }



			//////////////////////////////////////////////////////////////////////
			// type conversion

			EVO_NODISCARD auto trunc(unsigned width) const -> GenericInt {
				return GenericInt(this->ap_int.trunc(width));
			}

			EVO_NODISCARD auto sext(unsigned width) const -> GenericInt {
				return GenericInt(this->ap_int.sext(width));
			}

			EVO_NODISCARD auto zext(unsigned width) const -> GenericInt {
				return GenericInt(this->ap_int.zext(width));
			}

			EVO_NODISCARD auto sextOrTrunc(unsigned width) const -> GenericInt {
				return GenericInt(this->ap_int.sextOrTrunc(width));
			}

			EVO_NODISCARD auto zextOrTrunc(unsigned width) const -> GenericInt {
				return GenericInt(this->ap_int.zextOrTrunc(width));
			}

			EVO_NODISCARD auto extOrTrunc(unsigned width, bool is_unsigned) const -> GenericInt {
				if(is_unsigned){
					return this->zextOrTrunc(width);
				}else{
					return this->sextOrTrunc(width);
				}
			}

		private:
			GenericInt(llvmint::APInt&& _ap_int) : ap_int(std::move(_ap_int)) {}

			
		private:
			llvmint::APInt ap_int;

			friend class GenericFloat;			
	};
	
}
