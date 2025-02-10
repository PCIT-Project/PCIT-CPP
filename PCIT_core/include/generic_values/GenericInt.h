////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////

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

			EVO_NODISCARD explicit operator uint64_t() const { return *this->ap_int.getRawData();           }
			EVO_NODISCARD explicit operator int64_t()  const { return int64_t(*this->ap_int.getRawData());  }
			EVO_NODISCARD explicit operator uint32_t() const { return uint32_t(*this->ap_int.getRawData()); }
			EVO_NODISCARD explicit operator int32_t()  const { return int32_t(*this->ap_int.getRawData());  }
			EVO_NODISCARD explicit operator uint16_t() const { return uint16_t(*this->ap_int.getRawData()); }
			EVO_NODISCARD explicit operator int16_t()  const { return int16_t(*this->ap_int.getRawData());  }
			EVO_NODISCARD explicit operator uint8_t()  const { return uint8_t(*this->ap_int.getRawData());  }
			EVO_NODISCARD explicit operator int8_t()   const { return int8_t(*this->ap_int.getRawData());   }


			EVO_NODISCARD auto getBitWidth() const -> unsigned { return this->ap_int.getBitWidth(); }


			EVO_NODISCARD auto toString(bool is_signed, unsigned base = 10) const -> std::string {
				auto output = std::string();
				this->ap_int.toString(output, base, is_signed);
				return output;
			}

			EVO_NODISCARD auto getNative() const -> const llvmint::APInt& { return this->ap_int; }
			


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


			///////////////////////////////////
			// bitsize

			EVO_NODISCARD auto bitwiseAnd(const GenericInt& rhs) const -> GenericInt {
				return GenericInt(this->ap_int & rhs.ap_int);
			}

			EVO_NODISCARD auto bitwiseOr(const GenericInt& rhs) const -> GenericInt {
				return GenericInt(this->ap_int | rhs.ap_int);
			}

			EVO_NODISCARD auto bitwiseXor(const GenericInt& rhs) const -> GenericInt {
				return GenericInt(this->ap_int ^ rhs.ap_int);
			}

			EVO_NODISCARD auto bitwiseNot() const -> GenericInt {
				return GenericInt(~this->ap_int);
			}

			EVO_NODISCARD auto sshl(const GenericInt& rhs) const -> WrapResult {
				bool wrapped;
				llvmint::APInt result = this->ap_int.sshl_ov(rhs.ap_int, wrapped);
				return WrapResult(GenericInt(std::move(result)), wrapped);
			}

			EVO_NODISCARD auto ushl(const GenericInt& rhs) const -> WrapResult {
				bool wrapped;
				llvmint::APInt result = this->ap_int.ushl_ov(rhs.ap_int, wrapped);
				return WrapResult(GenericInt(std::move(result)), wrapped);
			}

			EVO_NODISCARD auto sshlSat(const GenericInt& rhs) const -> GenericInt {
				return GenericInt(this->ap_int.sshl_sat(rhs.ap_int));
			}

			EVO_NODISCARD auto ushlSat(const GenericInt& rhs) const -> GenericInt {
				return GenericInt(this->ap_int.ushl_sat(rhs.ap_int));
			}

			EVO_NODISCARD auto sshr(const GenericInt& rhs) const -> WrapResult {
				llvmint::APInt result = this->ap_int.ashr(rhs.ap_int);
				const bool wrapped = result.ne(this->ap_int);
				return WrapResult(GenericInt(std::move(result)), wrapped);
			}

			EVO_NODISCARD auto ushr(const GenericInt& rhs) const -> WrapResult {
				llvmint::APInt result = this->ap_int.lshr(rhs.ap_int);
				const bool wrapped = result.ne(this->ap_int);
				return WrapResult(GenericInt(std::move(result)), wrapped);
			}



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

			EVO_NODISCARD auto ext(unsigned width, bool is_unsigned) const -> GenericInt {
				if(is_unsigned){
					return this->zext(width);
				}else{
					return this->sext(width);
				}
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


namespace std{

	template<>
	struct formatter<pcit::core::GenericInt>{
	    constexpr auto parse(format_parse_context& ctx) const -> auto {
	        return ctx.begin();
	    }

	    auto format(const pcit::core::GenericInt& generic_int, format_context& ctx) const -> auto {
	        return format_to(ctx.out(), "{}", generic_int.toString(false));
	    }
	};
	
}
