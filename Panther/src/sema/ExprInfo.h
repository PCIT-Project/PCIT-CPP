////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>
#include <PCIT_core.h>

#include "../../include/source/source_data.h"
#include "../../include/TypeManager.h"
#include "../../include/sema/sema.h"


namespace pcit::panther{

	
	struct ExprInfo{
		enum class ValueCategory{
			Ephemeral,
			EphemeralFluid,
			ConcreteConst,
			ConcreteMut,
			ConcreteConstForwardable,
			ConcreteConstDestrMovable,

			Initializer, // uninit / zeroinit
			Module,
			Function, // function, not func pointer
			Intrinsic,
			TemplateIntrinsic, // uninstantiated
			Template, // uninstantiated
			Type,
		};

		enum class ValueStage{
			Comptime,
			Constexpr,
			Runtime,
		};

		struct InitializerType{};
		
		struct FluidType{};

		using TypeID = evo::Variant<
			InitializerType,                // Initializer
			FluidType,                      // EphemeralFluid
			TypeInfo::ID,                   // ConcreteConst|ConcreateMut|ConcreteConstForwardable
						                    //   |ConcreteConstDestrMovable|Ephemeral|Function|Intrinsic|Type
			evo::SmallVector<TypeInfo::ID>, // Ephemeral
			SourceID                        // Module
			// TODO: Template
			// TODO: TemplateIntrinsic
		>;

		ValueCategory value_category;
		ValueStage value_stage;
		TypeID type_id;


		///////////////////////////////////
		// constructors

		ExprInfo(
			ValueCategory vc,
			ValueStage vs,
			evo::SmallVector<TypeInfo::ID>&& type_ids,
			evo::SmallVector<sema::Expr>&& expr_list
		)
			: value_category(vc), value_stage(vs), type_id(InitializerType{}), exprs(std::move(expr_list)) {
			// TODO: remove this and move directly into `type_id` when the MSVC bug is fixed
			this->type_id.emplace<evo::SmallVector<TypeInfo::ID>>(std::move(type_ids));

			evo::debugAssert(
				this->value_category == ValueCategory::Ephemeral || this->value_category == ValueCategory::Function,
				"multi-expr must be multi-return or function overload set"
			);
		}
		

		ExprInfo(ValueCategory vc, ValueStage vs, auto&& _type_id, const sema::Expr& expr_single)
			: value_category(vc), value_stage(vs), type_id(std::move(_type_id)), exprs{expr_single} {}

		ExprInfo(ValueCategory vc, ValueStage vs, const auto& _type_id, const sema::Expr& expr_single)
			: value_category(vc), value_stage(vs), type_id(_type_id), exprs{expr_single} {}


		ExprInfo(ValueCategory vc, ValueStage vs, auto&& _type_id, std::nullopt_t)
			: value_category(vc), value_stage(vs), type_id(std::move(_type_id)), exprs() {
			evo::debugAssert(
				this->value_category == ValueCategory::Module || this->value_category == ValueCategory::Type
			);
			evo::debugAssert(this->value_stage == ValueStage::Comptime);
		}



		///////////////////////////////////
		// value type checking

		EVO_NODISCARD constexpr auto is_ephemeral() const -> bool {
			return this->value_category == ValueCategory::Ephemeral
				|| this->value_category == ValueCategory::EphemeralFluid;
		}

		EVO_NODISCARD constexpr auto is_concrete() const -> bool {
			return this->value_category == ValueCategory::ConcreteConst
				|| this->value_category == ValueCategory::ConcreteMut
				|| this->value_category == ValueCategory::ConcreteConstForwardable
				|| this->value_category == ValueCategory::ConcreteConstDestrMovable;
		}

		EVO_NODISCARD constexpr auto is_const() const -> bool {
			return this->value_category == ValueCategory::ConcreteConst 
				|| this->value_category == ValueCategory::Function;
		}


		///////////////////////////////////
		// single value

		EVO_NODISCARD auto isSingleValue() const -> bool {
			return this->type_id.is<TypeInfo::ID>() || this->type_id.is<FluidType>();
		}

		EVO_NODISCARD auto getExpr() const& -> const sema::Expr& {
			evo::debugAssert(this->isSingleValue(), "does not hold single value");
			return this->exprs.front();
		}

		EVO_NODISCARD auto getExpr() & -> sema::Expr& {
			evo::debugAssert(this->isSingleValue(), "does not hold single value");
			return this->exprs.front();
		}

		EVO_NODISCARD auto getExpr() const&& -> const sema::Expr&& {
			evo::debugAssert(this->isSingleValue(), "does not hold single value");
			return std::move(this->exprs.front());
		}

		EVO_NODISCARD auto getExpr() && -> sema::Expr&& {
			evo::debugAssert(this->isSingleValue(), "does not hold single value");
			return std::move(this->exprs.front());
		}


		//////////////////
		// module expr

		EVO_NODISCARD auto getModuleExpr() const -> const sema::Expr& {
			evo::debugAssert(this->type_id.is<SourceID>(), "does not hold module");
			return this->exprs.front();
		}


		///////////////////////////////////
		// multiple values

		EVO_NODISCARD auto isMultiValue() const -> bool { return this->type_id.is<evo::SmallVector<TypeInfo::ID>>(); }

		EVO_NODISCARD auto getExprList() const -> evo::ArrayProxy<sema::Expr> {
			evo::debugAssert(this->isMultiValue(), "does not hold multi-value");
			return this->exprs;
		}


		private:
			evo::SmallVector<sema::Expr> exprs; // empty if from ValueCategory::Module
	};
	


}