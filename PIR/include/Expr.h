//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>

#include "./forward_decl_ids.h"
#include "./Type.h"


namespace pcit::pir{


	class Expr{
		public:
			enum class Kind{
				None, // for the optional optimization

				// values
				GlobalValue,
				Number,
				ParamExpr,

				// stmts
				CallInst,
				CallVoidInst, // is separated from CallInst to allow for Expr::isValue()
				RetInst,
				BrInst,

				Add,
			};

		public:
			~Expr() = default;

			EVO_NODISCARD constexpr auto getKind() const -> Kind { return this->kind; }

			EVO_NODISCARD auto isValue() const -> bool {
				return this->isConstant() || this->kind == Kind::ParamExpr || this->kind == Kind::CallInst ||
					   this->kind == Kind::Add;
			}

			EVO_NODISCARD auto isConstant() const -> bool {
				return this->kind == Kind::Number || this->kind == Kind::GlobalValue;
			}

			EVO_NODISCARD auto isStmt() const -> bool {
				return this->kind == Kind::CallInst || this->kind == Kind::CallVoidInst || 
					   this->kind == Kind::RetInst  || this->kind == Kind::BrInst       ||
					   this->kind == Kind::Add;
			}

			EVO_NODISCARD auto isTerminator() const -> bool {
				return this->kind == Kind::RetInst || this->kind == Kind::BrInst;
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

			friend class Function;
			friend class Module;
			friend struct ExprOptInterface;
			friend class ReaderAgent;
			friend class Agent;
	};


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

	struct CallInst{
		std::string name;
		evo::Variant<FunctionID, FunctionDeclID, PtrCall> target;
		evo::SmallVector<Expr> args;
	};

	struct CallVoidInst{
		evo::Variant<FunctionID, FunctionDeclID, PtrCall> target;
		evo::SmallVector<Expr> args;
	};

	struct RetInst{
		std::optional<Expr> value;
	};

	struct BrInst{
		BasicBlockID target;
	};

	struct Add{
		std::string name;
		Expr lhs;
		Expr rhs;
		bool mayWrap;
	};


}


