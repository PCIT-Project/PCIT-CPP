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


namespace pcit::pir{


	class Expr{
		public:
			enum class Kind : uint32_t {
				None, // for the optional optimization

				// values
				GlobalValue,
				Number,
				Boolean,
				ParamExpr,

				// stmts
				Call,
				CallVoid, // is separated from Call to allow for Expr::isValue()
				Ret,
				Branch,
				Alloca,

				Add,
				AddWrap,
				AddWrapResult,
				AddWrapWrapped,
			};

		public:
			~Expr() = default;

			EVO_NODISCARD constexpr auto getKind() const -> Kind { return this->kind; }

			EVO_NODISCARD auto isValue() const -> bool {
				return this->isConstant()                || this->kind == Kind::GlobalValue    ||
				       this->kind == Kind::ParamExpr     || this->kind == Kind::Call           ||
				       this->kind == Kind::Alloca        || this->kind == Kind::Add            ||
				       this->kind == Kind::AddWrapResult || this->kind == Kind::AddWrapWrapped;
			}

			EVO_NODISCARD auto isConstant() const -> bool {
				return this->kind == Kind::Number || this->kind == Kind::Boolean;
			}

			EVO_NODISCARD auto isStmt() const -> bool {
				return this->kind == Kind::Call    || this->kind == Kind::CallVoid || 
					   this->kind == Kind::Ret     || this->kind == Kind::Branch   ||
					   this->kind == Kind::Alloca  || this->kind == Kind::Add      ||
					   this->kind == Kind::AddWrap;
			}

			EVO_NODISCARD auto isMultiValueStmt() const -> bool {
				return this->kind == Kind::AddWrap;
			}

			EVO_NODISCARD auto isTerminator() const -> bool {
				return this->kind == Kind::Ret || this->kind == Kind::Branch;
			}


			EVO_NODISCARD auto operator==(const Expr&) const -> bool = default;

			constexpr auto operator=(const Expr& rhs) -> Expr& {
				this->kind = rhs.kind;
				this->index = rhs.index;
				return *this;
			}	


		private:
			constexpr Expr(Kind _kind, uint32_t _index) : kind(_kind), index(_index) {}
			constexpr Expr(Kind _kind) : kind(_kind), index(0) {}
	
		private:
			Kind kind;
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
			return expr.getKind() != Expr::Kind::None;
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
			evo::debugAssert(this->type.isIntegral(), "This number is not integral");
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

	struct Ret{
		std::optional<Expr> value;
	};

	struct Branch{
		BasicBlockID target;
	};

	struct Alloca{
		std::string name;
		Type type;
	};

	struct Add{
		std::string name;
		Expr lhs;
		Expr rhs;
		bool mayWrap;
	};


	struct AddWrap{
		std::string resultName;
		std::string wrappedName;
		Expr lhs;
		Expr rhs;
	};


}


