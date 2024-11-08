//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>

#include <PCIT_core.h>

#include "./Type.h"
#include "./BasicBlock.h"
#include "./Expr.h"

namespace pcit::pir{

	class Parameter{
		public:
			Parameter(std::string&& _name, Type _type) : name(std::move(_name)), type(_type) {}
			~Parameter() = default;

			EVO_NODISCARD auto getName() const -> std::string_view { return this->name; }
			EVO_NODISCARD auto getType() const -> const Type& { return this->type; }
	
		private:
			std::string name;
			Type type;
	};




	// Create through Module
	struct FunctionDecl{
		std::string name;
		evo::SmallVector<Parameter> parameters;
		CallingConvention callingConvention;
		Linkage linkage;
		Type returnType;


		// for lookup in Module
		using ID = FunctionDeclID;
	};




	// Create through Module
	class Function{
		public:
			// for lookup in Module
			using ID = FunctionID;

		public:
			// Don't call this directly, go through Module
			Function(class Module& module, const FunctionDecl& declaration)
				: parent_module(module), func_decl(declaration) {}

			Function(class Module& module, FunctionDecl&& declaration)
				: parent_module(module), func_decl(std::move(declaration)) {}

			~Function() = default;


			EVO_NODISCARD auto getName() const -> std::string_view { return this->func_decl.name; }
			EVO_NODISCARD auto getParameters() const -> evo::ArrayProxy<Parameter> {
				return this->func_decl.parameters;
			}
			EVO_NODISCARD auto getCallingConvention() const -> const CallingConvention& {
				return this->func_decl.callingConvention;
			}
			EVO_NODISCARD auto getLinkage() const -> const Linkage& { return this->func_decl.linkage; }
			EVO_NODISCARD auto getReturnType() const -> const Type& { return this->func_decl.returnType; }


			auto appendBasicBlock(BasicBlock::ID id) -> void;
			auto insertBasicBlockBefore(BasicBlock::ID id, BasicBlock::ID before) -> void;
			auto insertBasicBlockAfter(BasicBlock::ID id, BasicBlock::ID after) -> void;


			//////////////////////////////////////////////////////////////////////
			// exprs

			EVO_NODISCARD auto getExprType(const Expr& expr) const -> Type;

			EVO_NODISCARD auto replaceExpr(const Expr& original, const Expr& replacement) -> void;


			///////////////////////////////////
			// ParamExpr

			EVO_NODISCARD static auto createParamExpr(uint32_t index) -> Expr {
				return Expr(Expr::Kind::ParamExpr, index);
			}

			EVO_NODISCARD static auto getParamExpr(const Expr& expr) -> ParamExpr {
				evo::debugAssert(expr.getKind() == Expr::Kind::ParamExpr, "not a param expr");
				return ParamExpr(expr.index);
			}


			///////////////////////////////////
			// calls

			EVO_NODISCARD auto createCallInst(
				std::string&& name, Function::ID func, evo::SmallVector<Expr>&& args
			) -> Expr;
			EVO_NODISCARD auto createCallInst(
				std::string&& name, Function::ID func, const evo::SmallVector<Expr>& args
			) -> Expr;

			EVO_NODISCARD auto createCallInst(
				std::string&& name, FunctionDecl::ID func, evo::SmallVector<Expr>&& args
			) -> Expr;
			EVO_NODISCARD auto createCallInst(
				std::string&& name, FunctionDecl::ID func, const evo::SmallVector<Expr>& args
			) -> Expr;

			EVO_NODISCARD auto createCallInst(
				std::string&& name, const Expr& func, const Type& func_type, evo::SmallVector<Expr>&& args
			) -> Expr;
			EVO_NODISCARD auto createCallInst(
				std::string&& name, const Expr& func, const Type& func_type, const evo::SmallVector<Expr>& args
			) -> Expr;


			EVO_NODISCARD auto getCallInst(const Expr& expr) const -> const CallInst& {
				evo::debugAssert(expr.getKind() == Expr::Kind::CallInst, "not a call inst");
				return this->calls[expr.index];
			}


			///////////////////////////////////
			// call voids

			EVO_NODISCARD auto createCallVoidInst(Function::ID func, evo::SmallVector<Expr>&& args) -> Expr;
			EVO_NODISCARD auto createCallVoidInst(Function::ID func, const evo::SmallVector<Expr>& args) -> Expr;

			EVO_NODISCARD auto createCallVoidInst(FunctionDecl::ID func, evo::SmallVector<Expr>&& args) -> Expr;
			EVO_NODISCARD auto createCallVoidInst(FunctionDecl::ID func, const evo::SmallVector<Expr>& args) -> Expr;

			EVO_NODISCARD auto createCallVoidInst(
				const Expr& func, const Type& func_type, evo::SmallVector<Expr>&& args
			) -> Expr;
			EVO_NODISCARD auto createCallVoidInst(
				const Expr& func, const Type& func_type, const evo::SmallVector<Expr>& args
			) -> Expr;

			EVO_NODISCARD auto getCallVoidInst(const Expr& expr) const -> const CallVoidInst& {
				evo::debugAssert(expr.getKind() == Expr::Kind::CallVoidInst, "not a call void inst");
				return this->call_voids[expr.index];
			}


			///////////////////////////////////
			// rets

			EVO_NODISCARD auto createRetInst(const Expr& expr) -> Expr {
				evo::debugAssert(expr.isValue(), "Must return value");
				evo::debugAssert(this->getExprType(expr) == this->getReturnType(), "Return type must match");
				return Expr(Expr::Kind::RetInst, this->rets.emplace_back(expr));
			}

			EVO_NODISCARD auto createRetInst() -> Expr {
				evo::debugAssert(this->getReturnType().getKind() == Type::Kind::Void, "Return type must match");
				return Expr(Expr::Kind::RetInst, this->rets.emplace_back(std::nullopt));
			}

			EVO_NODISCARD auto getRetInst(const Expr& expr) const -> const RetInst& {
				evo::debugAssert(expr.getKind() == Expr::Kind::RetInst, "Not a ret");
				return this->rets[expr.index];
			}


			///////////////////////////////////
			// br

			EVO_NODISCARD auto createBrInst(BasicBlock::ID basic_block_id) -> Expr {
				return Expr(Expr::Kind::BrInst, basic_block_id.get());
			}

			EVO_NODISCARD static auto getBrInst(const Expr& expr) -> BrInst {
				evo::debugAssert(expr.getKind() == Expr::Kind::BrInst, "Not a br");
				return BrInst(BasicBlock::ID(expr.index));
			}


			///////////////////////////////////
			// add

			EVO_NODISCARD auto createAdd(std::string&& name, const Expr& lhs, const Expr& rhs, bool may_wrap) -> Expr {
				evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
				evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
				return Expr(Expr::Kind::Add, this->adds.emplace_back(std::move(name), lhs, rhs, may_wrap));
			}

			EVO_NODISCARD auto getAdd(const Expr& expr) const -> const Add& {
				evo::debugAssert(expr.getKind() == Expr::Kind::Add, "Not an add");
				return this->adds[expr.index];
			}



			//////////////////////////////////////////////////////////////////////
			// iterators

			using Iter                 = evo::SmallVector<BasicBlock::ID>::iterator;
			using ConstIter            = evo::SmallVector<BasicBlock::ID>::const_iterator;
			using ReverseIter          = evo::SmallVector<BasicBlock::ID>::reverse_iterator;
			using ConstReverseIterator = evo::SmallVector<BasicBlock::ID>::const_reverse_iterator;

			EVO_NODISCARD auto begin()        -> Iter      { return this->basic_blocks.begin();  };
			EVO_NODISCARD auto begin()  const -> ConstIter { return this->basic_blocks.begin();  };
			EVO_NODISCARD auto cbegin() const -> ConstIter { return this->basic_blocks.cbegin(); };

			EVO_NODISCARD auto end()        -> Iter      { return this->basic_blocks.end();  };
			EVO_NODISCARD auto end()  const -> ConstIter { return this->basic_blocks.end();  };
			EVO_NODISCARD auto cend() const -> ConstIter { return this->basic_blocks.cend(); };


			EVO_NODISCARD auto rbegin()        -> ReverseIter          { return this->basic_blocks.rbegin();  };
			EVO_NODISCARD auto rbegin()  const -> ConstReverseIterator { return this->basic_blocks.rbegin();  };
			EVO_NODISCARD auto crbegin() const -> ConstReverseIterator { return this->basic_blocks.crbegin(); };

			EVO_NODISCARD auto rend()        -> ReverseIter          { return this->basic_blocks.rend();  };
			EVO_NODISCARD auto rend()  const -> ConstReverseIterator { return this->basic_blocks.rend();  };
			EVO_NODISCARD auto crend() const -> ConstReverseIterator { return this->basic_blocks.crend(); };


		private:
			EVO_NODISCARD auto basic_block_is_already_in(BasicBlock::ID id) const -> bool;



			EVO_NODISCARD auto check_func_call_args(Function::ID id, evo::ArrayProxy<Expr> args) const -> bool;
			EVO_NODISCARD auto check_func_call_args(FunctionDecl::ID id, evo::ArrayProxy<Expr> args) const -> bool;
			EVO_NODISCARD auto check_func_call_args(Type func_type, evo::ArrayProxy<Expr> args) const -> bool;
			EVO_NODISCARD auto check_func_call_args(evo::ArrayProxy<Type> param_types, evo::ArrayProxy<Expr> args) const
				-> bool;

	
		private:
			FunctionDecl func_decl;
			Module& parent_module;

			evo::SmallVector<BasicBlock::ID> basic_blocks{};

			core::StepAlloc<CallInst, uint32_t> calls{};
			core::StepAlloc<CallVoidInst, uint32_t> call_voids{};
			core::StepAlloc<RetInst, uint32_t> rets{};
			core::StepAlloc<Add, uint32_t> adds{};
	};


}


