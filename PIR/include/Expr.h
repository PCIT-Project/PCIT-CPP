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
				None, // for the optional optimization

				// values
				GlobalValue,
				FunctionPointer,
				Number,
				Boolean,
				ParamExpr,

				// stmts
				Call,
				CallVoid, // is separated from Call to allow for Expr::isValue()
				Breakpoint,

				Ret,
				Branch,
				CondBranch,
				Unreachable,

				Alloca,
				Load,
				Store,
				CalcPtr,
				Memcpy,
				Memset,

				BitCast,
				Trunc,
				FTrunc,
				SExt,
				ZExt,
				FExt,
				IToF,
				UIToF,
				FToI,
				FToUI,

				Add,
				SAddWrap,
				SAddWrapResult,
				SAddWrapWrapped,
				UAddWrap,
				UAddWrapResult,
				UAddWrapWrapped,
				SAddSat,
				UAddSat,
				FAdd,
				Sub,
				SSubWrap,
				SSubWrapResult,
				SSubWrapWrapped,
				USubWrap,
				USubWrapResult,
				USubWrapWrapped,
				SSubSat,
				USubSat,
				FSub,
				Mul,
				SMulWrap,
				SMulWrapResult,
				SMulWrapWrapped,
				UMulWrap,
				UMulWrapResult,
				UMulWrapWrapped,
				SMulSat,
				UMulSat,
				FMul,
				SDiv,
				UDiv,
				FDiv,
				SRem,
				URem,
				FRem,
				FNeg,

				IEq,
				FEq,
				INeq,
				FNeq,
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

				And,
				Or,
				Xor,
				SHL,
				SSHLSat,
				USHLSat,
				SSHR,
				USHR,
			};

		public:
			~Expr() = default;

			EVO_NODISCARD constexpr auto kind() const -> Kind { return this->_kind; }

			EVO_NODISCARD auto isValue() const -> bool {
                switch(this->_kind){
					case Kind::Number:          case Kind::Boolean:         case Kind::GlobalValue:
					case Kind::ParamExpr:       case Kind::Call:            case Kind::Alloca:
					case Kind::Load:            case Kind::CalcPtr:         case Kind::BitCast:
					case Kind::Trunc:           case Kind::FTrunc:          case Kind::SExt:
					case Kind::ZExt:            case Kind::FExt:            case Kind::IToF:
					case Kind::UIToF:           case Kind::FToI:            case Kind::FToUI:
					case Kind::Add:             case Kind::SAddWrap:        case Kind::SAddWrapResult:
					case Kind::SAddWrapWrapped: case Kind::UAddWrap:        case Kind::UAddWrapResult:
					case Kind::UAddWrapWrapped: case Kind::SAddSat:         case Kind::UAddSat:
					case Kind::FAdd:            case Kind::Sub:             case Kind::SSubWrap:
					case Kind::SSubWrapResult:  case Kind::SSubWrapWrapped: case Kind::USubWrap:
					case Kind::USubWrapResult:  case Kind::USubWrapWrapped: case Kind::SSubSat:
					case Kind::USubSat:         case Kind::FSub:            case Kind::Mul:
					case Kind::SMulWrap:        case Kind::SMulWrapResult:  case Kind::SMulWrapWrapped:
					case Kind::UMulWrap:        case Kind::UMulWrapResult:  case Kind::UMulWrapWrapped:
					case Kind::SMulSat:         case Kind::UMulSat:         case Kind::FMul:
					case Kind::SDiv:            case Kind::UDiv:            case Kind::FDiv:
					case Kind::SRem:            case Kind::URem:            case Kind::FRem:
					case Kind::FNeg:            case Kind::IEq:             case Kind::FEq:
					case Kind::INeq:            case Kind::FNeq:            case Kind::SLT:
					case Kind::ULT:             case Kind::FLT:             case Kind::SLTE:
					case Kind::ULTE:            case Kind::FLTE:            case Kind::SGT:
					case Kind::UGT:             case Kind::FGT:             case Kind::SGTE:
					case Kind::UGTE:            case Kind::FGTE:            case Kind::And:
					case Kind::Or:              case Kind::Xor:             case Kind::SHL:
					case Kind::SSHLSat:         case Kind::USHLSat:         case Kind::SSHR:
					case Kind::USHR: {
						return true;
					} break;
					default: return false;
                }
			}

			EVO_NODISCARD auto isConstant() const -> bool {
				switch(this->_kind){
					case Kind::Number: case Kind::Boolean: return true;
					default: return false;
				}
			}

			EVO_NODISCARD auto isStmt() const -> bool {
				switch(this->_kind){
					case Kind::Call:            case Kind::CallVoid:        case Kind::Breakpoint:
					case Kind::Ret:             case Kind::Branch:          case Kind::CondBranch:
					case Kind::Unreachable:     case Kind::Alloca:          case Kind::Load:
					case Kind::Store:           case Kind::CalcPtr:        	case Kind::Memcpy:
					case Kind::Memset:          case Kind::BitCast:         case Kind::Trunc:
					case Kind::FTrunc:          case Kind::SExt:            case Kind::ZExt:
					case Kind::FExt:            case Kind::IToF:            case Kind::UIToF:
					case Kind::FToI:            case Kind::FToUI:           case Kind::Add:
					case Kind::SAddWrap:        case Kind::SAddWrapResult:  case Kind::SAddWrapWrapped:
					case Kind::UAddWrap:        case Kind::UAddWrapResult:  case Kind::UAddWrapWrapped:
					case Kind::SAddSat:         case Kind::UAddSat:         case Kind::FAdd:
					case Kind::Sub:             case Kind::SSubWrap:        case Kind::SSubWrapResult:
					case Kind::SSubWrapWrapped: case Kind::USubWrap:        case Kind::USubWrapResult:
					case Kind::USubWrapWrapped: case Kind::SSubSat:         case Kind::USubSat:
					case Kind::FSub:            case Kind::Mul:             case Kind::SMulWrap:
					case Kind::SMulWrapResult:  case Kind::SMulWrapWrapped: case Kind::UMulWrap:
					case Kind::UMulWrapResult:  case Kind::UMulWrapWrapped: case Kind::SMulSat:
					case Kind::UMulSat:         case Kind::FMul:            case Kind::SDiv:
					case Kind::UDiv:            case Kind::FDiv:            case Kind::SRem:
					case Kind::URem:            case Kind::FRem:            case Kind::FNeg:
					case Kind::IEq:             case Kind::FEq:             case Kind::INeq:
					case Kind::FNeq:            case Kind::SLT:             case Kind::ULT:
					case Kind::FLT:             case Kind::SLTE:            case Kind::ULTE:
					case Kind::FLTE:            case Kind::SGT:             case Kind::UGT:
					case Kind::FGT:             case Kind::SGTE:            case Kind::UGTE:
					case Kind::FGTE:            case Kind::And:             case Kind::Or:
					case Kind::Xor:             case Kind::SHL:             case Kind::SSHLSat:
					case Kind::USHLSat:         case Kind::SSHR:            case Kind::USHR: {
						return true;
					} break;
					default: return false;
				}
			}

			EVO_NODISCARD auto isMultiValueStmt() const -> bool {
				switch(this->_kind){
					case Kind::SAddWrap: case Kind::UAddWrap: case Kind::SSubWrap: case Kind::USubWrap:
					case Kind::SMulWrap: case Kind::UMulWrap: {
						return true;
					} break;
					default: return false;
				}
			}

			EVO_NODISCARD auto isTerminator() const -> bool {
				switch(this->_kind){
					case Kind::Ret: case Kind::Branch: case Kind::CondBranch: case Kind::Unreachable: return true;
					default: return false;
				}
			}


			EVO_NODISCARD auto operator==(const Expr&) const -> bool = default;

			constexpr auto operator=(const Expr& rhs) -> Expr& {
				this->_kind = rhs._kind;
				this->index = rhs.index;
				return *this;
			}	


		private:
			constexpr Expr(Kind expr_kind, uint32_t _index) : _kind(expr_kind), index(_index) {}
			constexpr Expr(Kind expr_kind) : _kind(expr_kind), index(0) {}
	
		private:
			Kind _kind;
			uint32_t index;

			friend struct ExprOptInterface;
			friend class ReaderAgent;
			friend class Agent;
			friend class PassManager;
	};

	static_assert(sizeof(Expr) == 8);


	struct ExprOptInterface{
		static constexpr auto init(Expr* expr) -> void {
			*expr = Expr(Expr::Kind::None);
		}

		static constexpr auto has_value(const Expr& expr) -> bool {
			return expr.kind() != Expr::Kind::None;
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
	class optional<pcit::pir::Expr> 
		: public pcit::core::Optional<pcit::pir::Expr, pcit::pir::ExprOptInterface>{

		public:
			using pcit::core::Optional<pcit::pir::Expr, pcit::pir::ExprOptInterface>::Optional;
			using pcit::core::Optional<pcit::pir::Expr, pcit::pir::ExprOptInterface>::operator=;
	};

	
}



namespace pcit::pir{
	

	// Get through Function
	struct Number{
		Type type;

		EVO_NODISCARD auto getInt() const -> const core::GenericInt& {
			evo::debugAssert(this->type.kind() == Type::Kind::Integer, "This number is not integral");
			return this->value.as<core::GenericInt>();
		}

		EVO_NODISCARD auto getFloat() const -> const core::GenericFloat& {
			evo::debugAssert(this->type.isFloat(), "This number is not float");
			return this->value.as<core::GenericFloat>();
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
		evo::Variant<FunctionID, FunctionDeclID, PtrCall> target;
		evo::SmallVector<Expr> args;
	};

	struct CallVoid{
		evo::Variant<FunctionID, FunctionDeclID, PtrCall> target;
		evo::SmallVector<Expr> args;
	};


	//////////////////////////////////////////////////////////////////////
	// control flow

	struct Ret{
		std::optional<Expr> value;
	};

	struct Branch{
		BasicBlockID target;
	};

	struct CondBranch{
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


}


