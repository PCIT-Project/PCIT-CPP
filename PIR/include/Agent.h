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


#include "./Expr.h"
#include "./BasicBlock.h"
#include "./Function.h"
#include "./Module.h"



namespace pcit::pir{


	// A unified way to interact with things like exprs and basic blocks
	// 	Called "Agent" as it's sort of a go-between and manages menial stuff for yous

	class Agent{
		public:
			Agent(Module& _module) : module(_module), target_func(nullptr), target_basic_block(nullptr) {}
			Agent(Module& _module, Function& func) : module(_module), target_func(&func), target_basic_block(nullptr) {}
			Agent(Module& _module, Function& func, BasicBlock& basic_block)
				: module(_module), target_func(&func), target_basic_block(&basic_block) {}

			~Agent() = default;


			///////////////////////////////////
			// targets

			EVO_NODISCARD auto getModule() -> Module& { return this->module; }

			auto setTargetFunction(Function::ID id) -> void;
			auto setTargetFunction(Function& func) -> void;
			auto removeTargetFunction() -> void;

			EVO_NODISCARD auto hasTargetFunction() const -> bool { return this->target_func != nullptr; }
			EVO_NODISCARD auto getTargetFunction() const -> Function&;


			auto setTargetBasicBlock(BasicBlock::ID id) -> void;
			auto setTargetBasicBlock(BasicBlock& func) -> void;
			auto removeTargetBasicBlock() -> void;

			EVO_NODISCARD auto hasTargetBasicBlock() const -> bool { return this->target_basic_block != nullptr; }
			EVO_NODISCARD auto getTargetBasicBlock() const -> BasicBlock&;


			///////////////////////////////////
			// misc expr stuff

			EVO_NODISCARD auto getExprType(const Expr& expr) const -> Type;

			auto replaceExpr(Expr original, const Expr& replacement) const -> void;


			///////////////////////////////////
			// basic blocks

			auto createBasicBlock(std::string&& name, Function::ID func) const -> BasicBlock::ID;
			auto createBasicBlock(std::string&& name, Function& func) const -> BasicBlock::ID;
			auto createBasicBlock(std::string&& name) const -> BasicBlock::ID;

			EVO_NODISCARD auto getBasicBlock(BasicBlock::ID id) const -> BasicBlock&;


			///////////////////////////////////
			// numbers

			auto createNumber(const Type& type, core::GenericInt&& value) const -> Expr;
			auto createNumber(const Type& type, const core::GenericInt& value) const -> Expr;
			auto createNumber(const Type& type, core::GenericFloat&& value) const -> Expr;
			auto createNumber(const Type& type, const core::GenericFloat& value) const -> Expr;

			EVO_NODISCARD auto getNumber(const Expr& expr) const -> const Number&;


			///////////////////////////////////
			// param exprs

			EVO_NODISCARD static auto createParamExpr(uint32_t index) -> Expr;

			EVO_NODISCARD static auto getParamExpr(const Expr& expr) -> ParamExpr;


			///////////////////////////////////
			// calls

			EVO_NODISCARD auto createCallInst(
				std::string&& name, Function::ID func, evo::SmallVector<Expr>&& args
			) const -> Expr;
			EVO_NODISCARD auto createCallInst(
				std::string&& name, Function::ID func, const evo::SmallVector<Expr>& args
			) const -> Expr;

			EVO_NODISCARD auto createCallInst(
				std::string&& name, FunctionDecl::ID func, evo::SmallVector<Expr>&& args
			) const -> Expr;
			EVO_NODISCARD auto createCallInst(
				std::string&& name, FunctionDecl::ID func, const evo::SmallVector<Expr>& args
			) const -> Expr;

			EVO_NODISCARD auto createCallInst(
				std::string&& name, const Expr& func, const Type& func_type, evo::SmallVector<Expr>&& args
			) const -> Expr;
			EVO_NODISCARD auto createCallInst(
				std::string&& name, const Expr& func, const Type& func_type, const evo::SmallVector<Expr>& args
			) const -> Expr;



			///////////////////////////////////
			// call voids

			auto createCallVoidInst(Function::ID func, evo::SmallVector<Expr>&& args) const -> Expr;
			auto createCallVoidInst(Function::ID func, const evo::SmallVector<Expr>& args) const -> Expr;

			auto createCallVoidInst(FunctionDecl::ID func, evo::SmallVector<Expr>&& args) const -> Expr;
			auto createCallVoidInst(FunctionDecl::ID func, const evo::SmallVector<Expr>& args) const -> Expr;

			auto createCallVoidInst(const Expr& func, const Type& func_type, evo::SmallVector<Expr>&& args) const
				-> Expr;
			auto createCallVoidInst(const Expr& func, const Type& func_type, const evo::SmallVector<Expr>& args) const
				-> Expr;


			///////////////////////////////////
			// ret instructions

			auto createRetInst(const Expr& expr) const -> Expr;
			auto createRetInst() const -> Expr;
			EVO_NODISCARD auto getRetInst(const Expr& expr) const -> const RetInst&;


			///////////////////////////////////
			// br instructions

			auto createBrInst(BasicBlock::ID basic_block_id) const -> Expr;
			EVO_NODISCARD static auto getBrInst(const Expr& expr) -> BrInst;


			///////////////////////////////////
			// add

			EVO_NODISCARD auto createAdd(std::string&& name, const Expr& lhs, const Expr& rhs, bool may_wrap) const 
				-> Expr;
			EVO_NODISCARD auto getAdd(const Expr& expr) const -> const Add&;


		private:
			auto delete_expr(const Expr& expr) const -> void;

	
		private:
			Module& module;
			Function* target_func;
			BasicBlock* target_basic_block;
	};


}


