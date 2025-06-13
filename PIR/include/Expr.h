////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>

#include "./forward_decl_ids.h"
#include "./Type.h"
#include "./enums.h"

namespace pcit::pir{


	class Expr{
		public:
			enum class Kind : uint32_t {
				NONE, // for the optional optimization

				// values
				GLOBAL_VALUE,
				FUNCTION_POINTER,
				NUMBER,
				BOOLEAN,
				PARAM_EXPR,
				NULLPTR,

				// stmts
				CALL,
				CALL_VOID, // is separated from Call to allow for Expr::isValue()
				ABORT,
				BREAKPOINT,

				RET,
				JUMP,
				BRANCH,
				UNREACHABLE,

				ALLOCA,
				LOAD,
				STORE,
				CALC_PTR,
				MEMCPY,
				MEMSET,

				BIT_CAST,
				TRUNC,
				FTRUNC,
				SEXT,
				ZEXT,
				FEXT,
				ITOF,
				UITOF,
				FTOI,
				FTOUI,

				ADD,
				SADD_WRAP,
				SADD_WRAP_RESULT,
				SADD_WRAP_WRAPPED,
				UADD_WRAP,
				UADD_WRAP_RESULT,
				UADD_WRAP_WRAPPED,
				SADD_SAT,
				UADD_SAT,
				FADD,
				SUB,
				SSUB_WRAP,
				SSUB_WRAP_RESULT,
				SSUB_WRAP_WRAPPED,
				USUB_WRAP,
				USUB_WRAP_RESULT,
				USUB_WRAP_WRAPPED,
				SSUB_SAT,
				USUB_SAT,
				FSUB,
				MUL,
				SMUL_WRAP,
				SMUL_WRAP_RESULT,
				SMUL_WRAP_WRAPPED,
				UMUL_WRAP,
				UMUL_WRAP_RESULT,
				UMUL_WRAP_WRAPPED,
				SMUL_SAT,
				UMUL_SAT,
				FMUL,
				SDIV,
				UDIV,
				FDIV,
				SREM,
				UREM,
				FREM,
				FNEG,

				IEQ,
				FEQ,
				INEQ,
				FNEQ,
				SLT,
				ULT,
				FLT,
				SLTE,
				ULTE,
				FLTE,
				SGT,
				UGT,
				FGT,
				SGTE,
				UGTE,
				FGTE,

				AND,
				OR,
				XOR,
				SHL,
				SSHL_SAT,
				USHL_SAT,
				SSHR,
				USHR,

				BIT_REVERSE,
				BSWAP,
				CTPOP,
				CTLZ,
				CTTZ,
			};

		public:
			~Expr() = default;

			EVO_NODISCARD constexpr auto kind() const -> Kind { return this->_kind; }

			EVO_NODISCARD auto isValue() const -> bool {
                switch(this->_kind){
					case Kind::NUMBER:            case Kind::BOOLEAN:           case Kind::GLOBAL_VALUE:
					case Kind::PARAM_EXPR:        case Kind::NULLPTR:           case Kind::CALL:
					case Kind::ALLOCA:            case Kind::LOAD:              case Kind::CALC_PTR:
					case Kind::BIT_CAST:          case Kind::TRUNC:             case Kind::FTRUNC:
					case Kind::SEXT:              case Kind::ZEXT:              case Kind::FEXT:
					case Kind::ITOF:              case Kind::UITOF:             case Kind::FTOI:
					case Kind::FTOUI:             case Kind::ADD:               case Kind::SADD_WRAP:
					case Kind::SADD_WRAP_RESULT:  case Kind::SADD_WRAP_WRAPPED: case Kind::UADD_WRAP:
					case Kind::UADD_WRAP_RESULT:  case Kind::UADD_WRAP_WRAPPED: case Kind::SADD_SAT:
					case Kind::UADD_SAT:          case Kind::FADD:              case Kind::SUB:
					case Kind::SSUB_WRAP:         case Kind::SSUB_WRAP_RESULT:  case Kind::SSUB_WRAP_WRAPPED:
					case Kind::USUB_WRAP:         case Kind::USUB_WRAP_RESULT:  case Kind::USUB_WRAP_WRAPPED:
					case Kind::SSUB_SAT:          case Kind::USUB_SAT:          case Kind::FSUB:
					case Kind::MUL:               case Kind::SMUL_WRAP:         case Kind::SMUL_WRAP_RESULT:
					case Kind::SMUL_WRAP_WRAPPED: case Kind::UMUL_WRAP:         case Kind::UMUL_WRAP_RESULT:
					case Kind::UMUL_WRAP_WRAPPED: case Kind::SMUL_SAT:          case Kind::UMUL_SAT:
					case Kind::FMUL:              case Kind::SDIV:              case Kind::UDIV:
					case Kind::FDIV:              case Kind::SREM:              case Kind::UREM:
					case Kind::FREM:              case Kind::FNEG:              case Kind::IEQ:
					case Kind::FEQ:               case Kind::INEQ:              case Kind::FNEQ:
					case Kind::SLT:               case Kind::ULT:               case Kind::FLT:
					case Kind::SLTE:              case Kind::ULTE:              case Kind::FLTE:
					case Kind::SGT:               case Kind::UGT:               case Kind::FGT:
					case Kind::SGTE:              case Kind::UGTE:              case Kind::FGTE:
					case Kind::AND:               case Kind::OR:                case Kind::XOR:
					case Kind::SHL:               case Kind::SSHL_SAT:          case Kind::USHL_SAT:
					case Kind::SSHR:              case Kind::USHR:              case Kind::BIT_REVERSE:
					case Kind::BSWAP:             case Kind::CTPOP:             case Kind::CTLZ:
					case Kind::CTTZ: {
						return true;
					} break;
					default: return false;
                }
			}

			EVO_NODISCARD auto isConstant() const -> bool {
				switch(this->_kind){
					case Kind::NUMBER: case Kind::BOOLEAN: return true;
					default: return false;
				}
			}

			EVO_NODISCARD auto isStmt() const -> bool {
				switch(this->_kind){
					case Kind::CALL:              case Kind::CALL_VOID:         case Kind::ABORT:
					case Kind::BREAKPOINT:        case Kind::RET:               case Kind::JUMP:
					case Kind::BRANCH:            case Kind::UNREACHABLE:       case Kind::ALLOCA:
					case Kind::LOAD:              case Kind::STORE:             case Kind::CALC_PTR:
					case Kind::MEMCPY:            case Kind::MEMSET:            case Kind::BIT_CAST:
					case Kind::TRUNC:             case Kind::FTRUNC:            case Kind::SEXT:
					case Kind::ZEXT:              case Kind::FEXT:              case Kind::ITOF:
					case Kind::UITOF:             case Kind::FTOI:              case Kind::FTOUI:
					case Kind::ADD:               case Kind::SADD_WRAP:         case Kind::SADD_WRAP_RESULT:
					case Kind::SADD_WRAP_WRAPPED: case Kind::UADD_WRAP:         case Kind::UADD_WRAP_RESULT:
					case Kind::UADD_WRAP_WRAPPED: case Kind::SADD_SAT:          case Kind::UADD_SAT:
					case Kind::FADD:              case Kind::SUB:               case Kind::SSUB_WRAP:
					case Kind::SSUB_WRAP_RESULT:  case Kind::SSUB_WRAP_WRAPPED: case Kind::USUB_WRAP:
					case Kind::USUB_WRAP_RESULT:  case Kind::USUB_WRAP_WRAPPED: case Kind::SSUB_SAT:
					case Kind::USUB_SAT:          case Kind::FSUB:              case Kind::MUL:
					case Kind::SMUL_WRAP:         case Kind::SMUL_WRAP_RESULT:  case Kind::SMUL_WRAP_WRAPPED:
					case Kind::UMUL_WRAP:         case Kind::UMUL_WRAP_RESULT:  case Kind::UMUL_WRAP_WRAPPED:
					case Kind::SMUL_SAT:          case Kind::UMUL_SAT:          case Kind::FMUL:
					case Kind::SDIV:              case Kind::UDIV:              case Kind::FDIV:
					case Kind::SREM:              case Kind::UREM:              case Kind::FREM:
					case Kind::FNEG:              case Kind::IEQ:               case Kind::FEQ:
					case Kind::INEQ:              case Kind::FNEQ:              case Kind::SLT:
					case Kind::ULT:               case Kind::FLT:               case Kind::SLTE:
					case Kind::ULTE:              case Kind::FLTE:              case Kind::SGT:
					case Kind::UGT:               case Kind::FGT:               case Kind::SGTE:
					case Kind::UGTE:              case Kind::FGTE:              case Kind::AND:
					case Kind::OR:                case Kind::XOR:               case Kind::SHL:
					case Kind::SSHL_SAT:          case Kind::USHL_SAT:          case Kind::SSHR:
					case Kind::USHR:              case Kind::BIT_REVERSE:       case Kind::BSWAP:
					case Kind::CTPOP:             case Kind::CTLZ:              case Kind::CTTZ: {
						return true;
					} break;
					default: return false;
				}
			}

			EVO_NODISCARD auto isMultiValueStmt() const -> bool {
				switch(this->_kind){
					case Kind::SADD_WRAP: case Kind::UADD_WRAP: case Kind::SSUB_WRAP: case Kind::USUB_WRAP:
					case Kind::SMUL_WRAP: case Kind::UMUL_WRAP: {
						return true;
					} break;
					default: return false;
				}
			}

			EVO_NODISCARD auto isTerminator() const -> bool {
				switch(this->_kind){
					case Kind::RET: case Kind::JUMP: case Kind::BRANCH: case Kind::UNREACHABLE: return true;
					default: return false;
				}
			}


			EVO_NODISCARD auto operator==(const Expr&) const -> bool = default;

			// constexpr auto operator=(const Expr& rhs) -> Expr& {
			// 	this->_kind = rhs._kind;
			// 	this->index = rhs.index;
			// 	return *this;
			// }	


		private:
			constexpr Expr(Kind expr_kind, uint32_t _index) : _kind(expr_kind), index(_index) {}
			constexpr Expr(Kind expr_kind) : _kind(expr_kind), index(0) {}
	
		private:
			Kind _kind;
			uint32_t index;

			friend struct core::OptionalInterface<Expr>;
			friend class ReaderAgent;
			friend class Agent;
			friend class PassManager;
	};

	static_assert(sizeof(Expr) == 8);
	static_assert(std::is_trivially_copyable<Expr>());


}



namespace pcit::core{
	
	template<>
	struct OptionalInterface<pir::Expr>{
		static constexpr auto init(pir::Expr* expr) -> void {
			*expr = pir::Expr(pir::Expr::Kind::NONE);
		}

		static constexpr auto has_value(const pir::Expr& expr) -> bool {
			return expr.kind() != pir::Expr::Kind::NONE;
		}
	};

}


namespace std{

	template<>
	struct hash<pcit::pir::Expr>{
		auto operator()(const pcit::pir::Expr& expr) const noexcept -> size_t {
			return hash<uint64_t>{}(evo::bitCast<uint64_t>(expr));
		}
	};


	template<>
	class optional<pcit::pir::Expr> : public pcit::core::Optional<pcit::pir::Expr>{
		public:
			using pcit::core::Optional<pcit::pir::Expr>::Optional;
			using pcit::core::Optional<pcit::pir::Expr>::operator=;
	};

	
}



namespace pcit::pir{

	static_assert(std::atomic<std::optional<Expr>>::is_always_lock_free);
	
	struct Number{
		// TODO(PERF): need to have `.type`, or just have a function to get it based on `.value`?
		const Type type;

		EVO_NODISCARD auto getInt() const -> const core::GenericInt& {
			evo::debugAssert(this->type.kind() == Type::Kind::INTEGER, "This number is not integral");
			return this->value.as<core::GenericInt>();
		}

		EVO_NODISCARD auto getFloat() const -> const core::GenericFloat& {
			evo::debugAssert(this->type.isFloat(), "This number is not float");
			return this->value.as<core::GenericFloat>();
		}

		EVO_NODISCARD auto asGenericValue() const -> core::GenericValue {
			return value.visit([](const auto& val){ return core::GenericValue(evo::copy(val)); });
		}

		Number(const Type& _type, core::GenericInt&& val) : type(_type), value(std::move(val)) {}
		Number(const Type& _type, const core::GenericInt& val) : type(_type), value(val) {}
		Number(const Type& _type, core::GenericFloat&& val) : type(_type), value(std::move(val)) {}
		Number(const Type& _type, const core::GenericFloat& val) : type(_type), value(val) {}

		private:
			evo::Variant<core::GenericInt, core::GenericFloat> value;
	};

	struct ParamExpr{
		uint32_t index;
	};


	struct PtrCall{
		Expr location;
		Type funcType;
	};

	struct Call{
		std::string name;
		evo::Variant<FunctionID, ExternalFunctionID, PtrCall> target;
		evo::SmallVector<Expr> args;
	};

	struct CallVoid{
		evo::Variant<FunctionID, ExternalFunctionID, PtrCall> target;
		evo::SmallVector<Expr> args;
	};


	//////////////////////////////////////////////////////////////////////
	// control flow

	struct Ret{
		std::optional<Expr> value;
	};

	struct Jump{
		BasicBlockID target;
	};

	struct Branch{
		Expr cond;
		BasicBlockID thenBlock;
		BasicBlockID elseBlock;
	};



	//////////////////////////////////////////////////////////////////////
	// memory

	struct Alloca{
		std::string name;
		Type type;
	};

	struct Load{
		std::string name;
		Expr source;
		Type type;
		bool isVolatile;
		AtomicOrdering atomicOrdering;
	};

	struct Store{
		Expr destination;
		Expr value;
		bool isVolatile;
		AtomicOrdering atomicOrdering;
	};

	struct CalcPtr{
		using Index = evo::Variant<Expr, int64_t>;

		std::string name;
		Type ptrType;
		Expr basePtr;
		evo::SmallVector<Index> indices;
	};

	struct Memcpy{
		Expr dst;
		Expr src;
		Expr numBytes;
		bool isVolatile;
	};

	struct Memset{
		Expr dst;
		Expr value;
		Expr numBytes;
		bool isVolatile;
	};



	//////////////////////////////////////////////////////////////////////
	// type conversion

	struct BitCast{
		std::string name;
		Expr fromValue;
		Type toType;
	};

	struct Trunc{
		std::string name;
		Expr fromValue;
		Type toType;
	};

	struct FTrunc{
		std::string name;
		Expr fromValue;
		Type toType;
	};

	struct SExt{
		std::string name;
		Expr fromValue;
		Type toType;
	};

	struct ZExt{
		std::string name;
		Expr fromValue;
		Type toType;
	};

	struct FExt{
		std::string name;
		Expr fromValue;
		Type toType;
	};

	struct IToF{
		std::string name;
		Expr fromValue;
		Type toType;
	};

	struct UIToF{
		std::string name;
		Expr fromValue;
		Type toType;
	};

	struct FToI{
		std::string name;
		Expr fromValue;
		Type toType;
	};

	struct FToUI{
		std::string name;
		Expr fromValue;
		Type toType;
	};


	//////////////////////////////////////////////////////////////////////
	// arithmetic

	///////////////////////////////////
	// add

	struct Add{
		std::string name;
		Expr lhs;
		Expr rhs;
		bool nsw;
		bool nuw;
	};

	struct SAddWrap{
		std::string resultName;
		std::string wrappedName;
		Expr lhs;
		Expr rhs;
	};

	struct UAddWrap{
		std::string resultName;
		std::string wrappedName;
		Expr lhs;
		Expr rhs;
	};

	struct UAddSat{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct SAddSat{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct FAdd{
		std::string name;
		Expr lhs;
		Expr rhs;
	};


	///////////////////////////////////
	// sub

	struct Sub{
		std::string name;
		Expr lhs;
		Expr rhs;
		bool nsw;
		bool nuw;
	};

	struct SSubWrap{
		std::string resultName;
		std::string wrappedName;
		Expr lhs;
		Expr rhs;
	};

	struct USubWrap{
		std::string resultName;
		std::string wrappedName;
		Expr lhs;
		Expr rhs;
	};

	struct USubSat{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct SSubSat{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct FSub{
		std::string name;
		Expr lhs;
		Expr rhs;
	};


	///////////////////////////////////
	// mul

	struct Mul{
		std::string name;
		Expr lhs;
		Expr rhs;
		bool nsw;
		bool nuw;
	};

	struct SMulWrap{
		std::string resultName;
		std::string wrappedName;
		Expr lhs;
		Expr rhs;
	};

	struct UMulWrap{
		std::string resultName;
		std::string wrappedName;
		Expr lhs;
		Expr rhs;
	};

	struct UMulSat{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct SMulSat{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct FMul{
		std::string name;
		Expr lhs;
		Expr rhs;
	};


	///////////////////////////////////
	// div / rem

	struct SDiv{
		std::string name;
		Expr lhs;
		Expr rhs;
		bool isExact;
	};

	struct UDiv{
		std::string name;
		Expr lhs;
		Expr rhs;
		bool isExact;
	};

	struct FDiv{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct SRem{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct URem{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct FRem{
		std::string name;
		Expr lhs;
		Expr rhs;
	};


	struct FNeg{
		std::string name;
		Expr rhs;
	};


	//////////////////////////////////////////////////////////////////////
	// logical

	struct IEq{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct FEq{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct INeq{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct FNeq{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct SLT{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct ULT{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct FLT{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct SLTE{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct ULTE{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct FLTE{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct SGT{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct UGT{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct FGT{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct SGTE{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct UGTE{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct FGTE{
		std::string name;
		Expr lhs;
		Expr rhs;
	};



	//////////////////////////////////////////////////////////////////////
	// bitwise

	struct And{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct Or{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct Xor{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct SHL{
		std::string name;
		Expr lhs;
		Expr rhs;
		bool nsw;
		bool nuw;
	};

	struct SSHLSat{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct USHLSat{
		std::string name;
		Expr lhs;
		Expr rhs;
	};

	struct SSHR{
		std::string name;
		Expr lhs;
		Expr rhs;
		bool isExact;
	};

	struct USHR{
		std::string name;
		Expr lhs;
		Expr rhs;
		bool isExact;
	};


	//////////////////////////////////////////////////////////////////////
	// bit operations

	struct BitReverse{
		std::string name;
		Expr arg;
	};

	struct BSwap{
		std::string name;
		Expr arg;
	};

	struct CtPop{
		std::string name;
		Expr arg;
	};

	struct CTLZ{
		std::string name;
		Expr arg;
	};

	struct CTTZ{
		std::string name;
		Expr arg;
	};



}


