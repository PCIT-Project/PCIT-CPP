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


#include "./Expr.h"
#include "./BasicBlock.h"
#include "./Function.h"
#include "./Module.h"



namespace pcit::pir{


	// A unified way to interact with things like exprs and basic blocks
	// 	Called "Agent" as it's sort of a go-between and manages menial stuff for you
	// 	I would have picked "Interface" but I didn't want to overload the term

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

			auto replaceStmt(Expr original, const Expr& replacement) const -> void;
			auto replaceStmtWithValue(Expr original, const Expr& replacement) const -> void;
			auto removeStmt(Expr stmt_to_remove) const -> void;


			///////////////////////////////////
			// basic blocks

			auto createBasicBlock(Function::ID func, std::string&& name = "") const -> BasicBlock::ID;
			auto createBasicBlock(Function& func, std::string&& name = "") const -> BasicBlock::ID;
			auto createBasicBlock(std::string&& name = "") const -> BasicBlock::ID;

			EVO_NODISCARD auto getBasicBlock(BasicBlock::ID id) const -> BasicBlock&;


			///////////////////////////////////
			// numbers

			auto createNumber(const Type& type, core::GenericInt&& value) const -> Expr;
			auto createNumber(const Type& type, const core::GenericInt& value) const -> Expr;
			auto createNumber(const Type& type, core::GenericFloat&& value) const -> Expr;
			auto createNumber(const Type& type, const core::GenericFloat& value) const -> Expr;

			EVO_NODISCARD auto getNumber(const Expr& expr) const -> const Number&;


			///////////////////////////////////
			// booleans

			EVO_NODISCARD static auto createBoolean(bool value) -> Expr;

			EVO_NODISCARD static auto getBoolean(const Expr& expr) -> bool;



			///////////////////////////////////
			// param exprs

			EVO_NODISCARD static auto createParamExpr(uint32_t index) -> Expr;

			EVO_NODISCARD static auto getParamExpr(const Expr& expr) -> ParamExpr;


			///////////////////////////////////
			// global values (expr)

			EVO_NODISCARD static auto createGlobalValue(const GlobalVar::ID& global_id) -> Expr;

			EVO_NODISCARD auto getGlobalValue(const Expr& expr) const -> const GlobalVar&;


			///////////////////////////////////
			// calls

			EVO_NODISCARD auto createCall(
				Function::ID func, evo::SmallVector<Expr>&& args, std::string&& name = ""
			) const -> Expr;
			EVO_NODISCARD auto createCall(
				Function::ID func, const evo::SmallVector<Expr>& args, std::string&& name = ""
			) const -> Expr;

			EVO_NODISCARD auto createCall(
				FunctionDecl::ID func, evo::SmallVector<Expr>&& args, std::string&& name = ""
			) const -> Expr;
			EVO_NODISCARD auto createCall(
				FunctionDecl::ID func, const evo::SmallVector<Expr>& args, std::string&& name = ""
			) const -> Expr;

			EVO_NODISCARD auto createCall(
				const Expr& func, const Type& func_type, evo::SmallVector<Expr>&& args, std::string&& name = ""
			) const -> Expr;
			EVO_NODISCARD auto createCall(
				const Expr& func, const Type& func_type, const evo::SmallVector<Expr>& args, std::string&& name = ""
			) const -> Expr;

			EVO_NODISCARD auto getCall(const Expr& expr) const -> const Call&;


			///////////////////////////////////
			// call voids

			auto createCallVoid(Function::ID func, evo::SmallVector<Expr>&& args) const -> Expr;
			auto createCallVoid(Function::ID func, const evo::SmallVector<Expr>& args) const -> Expr;

			auto createCallVoid(FunctionDecl::ID func, evo::SmallVector<Expr>&& args) const -> Expr;
			auto createCallVoid(FunctionDecl::ID func, const evo::SmallVector<Expr>& args) const -> Expr;

			auto createCallVoid(const Expr& func, const Type& func_type, evo::SmallVector<Expr>&& args) const
				-> Expr;
			auto createCallVoid(const Expr& func, const Type& func_type, const evo::SmallVector<Expr>& args) const
				-> Expr;

			EVO_NODISCARD auto getCallVoid(const Expr& expr) const -> const CallVoid&;


			///////////////////////////////////
			// ret instructions

			auto createRet(const Expr& expr) const -> Expr;
			auto createRet() const -> Expr;
			EVO_NODISCARD auto getRet(const Expr& expr) const -> const Ret&;


			///////////////////////////////////
			// branch instructions

			auto createBranch(BasicBlock::ID basic_block_id) const -> Expr;
			EVO_NODISCARD static auto getBranch(const Expr& expr) -> Branch;


			///////////////////////////////////
			// alloca

			EVO_NODISCARD auto createAlloca(const Type& type, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getAlloca(const Expr& expr) const -> const Alloca&;


			///////////////////////////////////
			// add

			EVO_NODISCARD auto createAdd(const Expr& lhs, const Expr& rhs, bool may_wrap, std::string&& name = "") const 
				-> Expr;
			auto createAdd(const Expr&, const Expr&, const char*) = delete;
			EVO_NODISCARD auto getAdd(const Expr& expr) const -> const Add&;


			///////////////////////////////////
			// add wrap

			EVO_NODISCARD auto createAddWrap(
				const Expr& lhs, const Expr& rhs, std::string&& result_name = "", std::string&& wrapped_name = ""
			) -> Expr;

			EVO_NODISCARD auto getAddWrap(const Expr& expr) const -> const AddWrap&;

			EVO_NODISCARD static auto extractAddWrapResult(const Expr& expr) -> Expr;
			EVO_NODISCARD static auto extractAddWrapWrapped(const Expr& expr) -> Expr;


		private:
			auto delete_expr(const Expr& expr) const -> void;

			EVO_NODISCARD auto name_exists_in_func(std::string_view) const -> bool;
			EVO_NODISCARD auto get_stmt_name(std::string&& name) const -> std::string;

			template<bool REPLACE_WITH_VALUE>
			EVO_NODISCARD auto replace_stmt_impl(Expr original, const Expr& replacement) const -> void;

	
		private:
			Module& module;
			Function* target_func;
			BasicBlock* target_basic_block;
	};


}


