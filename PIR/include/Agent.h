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
	// Note: a const Agent has all the power to get and create expressions / statements, but the insert target cannot be 
	// 	modified. If you want an Agent that can only get but not create, use a ReaderAgent instead

	class Agent{
		public:
			Agent(Module& _module) : module(_module), target_func(nullptr), target_basic_block(nullptr) {}
			Agent(Module& _module, Function& func) : module(_module), target_func(&func), target_basic_block(nullptr) {}
			Agent(Module& _module, Function& func, BasicBlock& basic_block)
				: module(_module), target_func(&func), target_basic_block(&basic_block) {}
			Agent(Module& _module, Function& func, BasicBlock& basic_block, size_t _insert_index)
				: module(_module), target_func(&func), target_basic_block(&basic_block), insert_index(_insert_index) {}

			~Agent() = default;


			///////////////////////////////////
			// targets

			EVO_NODISCARD auto getModule() const -> Module& { return this->module; }

			auto setTargetFunction(Function::ID id) -> void;
			auto setTargetFunction(Function& func) -> void;
			auto removeTargetFunction() -> void;

			EVO_NODISCARD auto hasTargetFunction() const -> bool { return this->target_func != nullptr; }
			EVO_NODISCARD auto getTargetFunction() const -> Function&;


			auto setTargetBasicBlock(BasicBlock::ID id) -> void;
			auto setTargetBasicBlock(BasicBlock& func) -> void;
			auto setTargetBasicBlockAtEnd() -> void;
			auto removeTargetBasicBlock() -> void;

			EVO_NODISCARD auto hasTargetBasicBlock() const -> bool { return this->target_basic_block != nullptr; }
			EVO_NODISCARD auto getTargetBasicBlock() const -> BasicBlock&;


			auto setInsertIndex(size_t index) -> void;
			auto setInsertIndexAtEnd() -> void;
			auto getInsertIndexAtEnd() const -> bool { return this->insert_index == std::numeric_limits<size_t>::max();}


			///////////////////////////////////
			// misc expr stuff

			EVO_NODISCARD auto getExprType(const Expr& expr) const -> Type;

			auto replaceExpr(Expr original, Expr replacement) const -> void;
			auto removeStmt(Expr stmt_to_remove) const -> void;


			///////////////////////////////////
			// basic blocks

			auto createBasicBlock(Function::ID func, std::string&& name = "") const -> BasicBlock::ID;
			auto createBasicBlock(Function& func, std::string&& name = "") const -> BasicBlock::ID;
			auto createBasicBlock(std::string&& name = "") const -> BasicBlock::ID;
			auto createBasicBlockInline(std::string&& name = "") const -> BasicBlock::ID;

			EVO_NODISCARD auto getBasicBlock(BasicBlock::ID id) const -> BasicBlock&;


			///////////////////////////////////
			// numbers

			EVO_NODISCARD auto createNumber(const Type& type, core::GenericInt&& value) const -> Expr;
			EVO_NODISCARD auto createNumber(const Type& type, const core::GenericInt& value) const -> Expr;
			EVO_NODISCARD auto createNumber(const Type& type, core::GenericFloat&& value) const -> Expr;
			EVO_NODISCARD auto createNumber(const Type& type, const core::GenericFloat& value) const -> Expr;

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
			// function pointers


			EVO_NODISCARD static auto createFunctionPointer(const Function::ID& global_id) -> Expr;

			EVO_NODISCARD auto getFunctionPointer(const Expr& expr) const -> const Function&;




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
			// breakpoint

			auto createBreakpoint() const -> Expr;


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
			// condiitonal branch instructions

			auto createCondBranch(const Expr& cond, BasicBlock::ID then_block, BasicBlock::ID else_block) const -> Expr;
			EVO_NODISCARD auto getCondBranch(const Expr& expr) const -> CondBranch;


			///////////////////////////////////
			// unreachable

			auto createUnreachable() const -> Expr;


			///////////////////////////////////
			// alloca

			EVO_NODISCARD auto createAlloca(const Type& type, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getAlloca(const Expr& expr) const -> const Alloca&;


			///////////////////////////////////
			// load

			EVO_NODISCARD auto createLoad(
				const Expr& source,
				const Type& type,
				bool is_volatile,
				AtomicOrdering atomic_ordering = AtomicOrdering::None,
				std::string&& name = ""
			) const -> Expr;
			EVO_NODISCARD auto getLoad(const Expr& expr) const -> const Load&;


			///////////////////////////////////
			// store

			EVO_NODISCARD auto createStore(
				const Expr& destination,
				const Expr& value,
				bool is_volatile,
				AtomicOrdering atomic_ordering = AtomicOrdering::None
			) const -> void;
			EVO_NODISCARD auto getStore(const Expr& expr) const -> const Store&;


			///////////////////////////////////
			// calc ptr

			EVO_NODISCARD auto createCalcPtr(
				const Expr& base_ptr,
				const Type& ptr_type,
				evo::SmallVector<CalcPtr::Index>&& indices,
				std::string&& name = ""
			) const -> Expr;
			EVO_NODISCARD auto getCalcPtr(const Expr& expr) const -> const CalcPtr&;


			///////////////////////////////////
			// memcpy

			auto createMemcpy(const Expr& dst, const Expr& src, const Expr& num_bytes, bool is_volatile) const -> Expr;

			EVO_NODISCARD auto getMemcpy(const Expr&) const -> const Memcpy&;


			///////////////////////////////////
			// memset

			auto createMemset(const Expr& dst, const Expr& value, const Expr& num_bytes, bool is_volatile) const
				-> Expr;

			EVO_NODISCARD auto getMemset(const Expr&) const -> const Memset&;


			//////////////////////////////////////////////////////////////////////
			// type conversion

			
			///////////////////////////////////
			// BitCast

			EVO_NODISCARD auto createBitCast(const Expr& fromValue, const Type& toType, std::string&& name = "") const
				-> Expr;
			EVO_NODISCARD auto getBitCast(const Expr& expr) const -> const BitCast&;

			
			///////////////////////////////////
			// Trunc

			EVO_NODISCARD auto createTrunc(const Expr& fromValue, const Type& toType, std::string&& name = "") const
				-> Expr;
			EVO_NODISCARD auto getTrunc(const Expr& expr) const -> const Trunc&;

			
			///////////////////////////////////
			// FTrunc

			EVO_NODISCARD auto createFTrunc(const Expr& fromValue, const Type& toType, std::string&& name = "") const
				-> Expr;
			EVO_NODISCARD auto getFTrunc(const Expr& expr) const -> const FTrunc&;

			
			///////////////////////////////////
			// SExt

			EVO_NODISCARD auto createSExt(const Expr& fromValue, const Type& toType, std::string&& name = "") const
				-> Expr;
			EVO_NODISCARD auto getSExt(const Expr& expr) const -> const SExt&;

			
			///////////////////////////////////
			// ZExt

			EVO_NODISCARD auto createZExt(const Expr& fromValue, const Type& toType, std::string&& name = "") const
				-> Expr;
			EVO_NODISCARD auto getZExt(const Expr& expr) const -> const ZExt&;

			
			///////////////////////////////////
			// FExt

			EVO_NODISCARD auto createFExt(const Expr& fromValue, const Type& toType, std::string&& name = "") const
				-> Expr;
			EVO_NODISCARD auto getFExt(const Expr& expr) const -> const FExt&;

			
			///////////////////////////////////
			// IToF

			EVO_NODISCARD auto createIToF(const Expr& fromValue, const Type& toType, std::string&& name = "") const
				-> Expr;
			EVO_NODISCARD auto getIToF(const Expr& expr) const -> const IToF&;

			
			///////////////////////////////////
			// UIToF

			EVO_NODISCARD auto createUIToF(const Expr& fromValue, const Type& toType, std::string&& name = "") const
				-> Expr;
			EVO_NODISCARD auto getUIToF(const Expr& expr) const -> const UIToF&;

			
			///////////////////////////////////
			// FToI

			EVO_NODISCARD auto createFToI(const Expr& fromValue, const Type& toType, std::string&& name = "") const
				-> Expr;
			EVO_NODISCARD auto getFToI(const Expr& expr) const -> const FToI&;

			
			///////////////////////////////////
			// FToUI

			EVO_NODISCARD auto createFToUI(const Expr& fromValue, const Type& toType, std::string&& name = "") const
				-> Expr;
			EVO_NODISCARD auto getFToUI(const Expr& expr) const -> const FToUI&;



			//////////////////////////////////////////////////////////////////////
			// arithmetic

			///////////////////////////////////
			// add

			EVO_NODISCARD auto createAdd(
				const Expr& lhs, const Expr& rhs, bool nsw, bool nuw, std::string&& name = ""
			) const -> Expr;
			EVO_NODISCARD auto getAdd(const Expr& expr) const -> const Add&;

			EVO_NODISCARD auto createSAddWrap(
				const Expr& lhs, const Expr& rhs, std::string&& result_name = "", std::string&& wrapped_name = ""
			) -> Expr;
			EVO_NODISCARD auto getSAddWrap(const Expr& expr) const -> const SAddWrap&;
			EVO_NODISCARD static auto extractSAddWrapResult(const Expr& expr) -> Expr;
			EVO_NODISCARD static auto extractSAddWrapWrapped(const Expr& expr) -> Expr;

			EVO_NODISCARD auto createUAddWrap(
				const Expr& lhs, const Expr& rhs, std::string&& result_name = "", std::string&& wrapped_name = ""
			) -> Expr;
			EVO_NODISCARD auto getUAddWrap(const Expr& expr) const -> const UAddWrap&;
			EVO_NODISCARD static auto extractUAddWrapResult(const Expr& expr) -> Expr;
			EVO_NODISCARD static auto extractUAddWrapWrapped(const Expr& expr) -> Expr;

			EVO_NODISCARD auto createSAddSat(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getSAddSat(const Expr& expr) const -> const SAddSat&;

			EVO_NODISCARD auto createUAddSat(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getUAddSat(const Expr& expr) const -> const UAddSat&;

			EVO_NODISCARD auto createFAdd(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getFAdd(const Expr& expr) const -> const FAdd&;


			///////////////////////////////////
			// sub

			EVO_NODISCARD auto createSub(
				const Expr& lhs, const Expr& rhs, bool nsw, bool nuw, std::string&& name = ""
			) const -> Expr;
			EVO_NODISCARD auto getSub(const Expr& expr) const -> const Sub&;

			EVO_NODISCARD auto createSSubWrap(
				const Expr& lhs, const Expr& rhs, std::string&& result_name = "", std::string&& wrapped_name = ""
			) -> Expr;
			EVO_NODISCARD auto getSSubWrap(const Expr& expr) const -> const SSubWrap&;
			EVO_NODISCARD static auto extractSSubWrapResult(const Expr& expr) -> Expr;
			EVO_NODISCARD static auto extractSSubWrapWrapped(const Expr& expr) -> Expr;

			EVO_NODISCARD auto createUSubWrap(
				const Expr& lhs, const Expr& rhs, std::string&& result_name = "", std::string&& wrapped_name = ""
			) -> Expr;
			EVO_NODISCARD auto getUSubWrap(const Expr& expr) const -> const USubWrap&;
			EVO_NODISCARD static auto extractUSubWrapResult(const Expr& expr) -> Expr;
			EVO_NODISCARD static auto extractUSubWrapWrapped(const Expr& expr) -> Expr;

			EVO_NODISCARD auto createSSubSat(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getSSubSat(const Expr& expr) const -> const SSubSat&;

			EVO_NODISCARD auto createUSubSat(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getUSubSat(const Expr& expr) const -> const USubSat&;

			EVO_NODISCARD auto createFSub(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getFSub(const Expr& expr) const -> const FSub&;


			///////////////////////////////////
			// mul

			EVO_NODISCARD auto createMul(
				const Expr& lhs, const Expr& rhs, bool nsw, bool nuw, std::string&& name = ""
			) const -> Expr;
			EVO_NODISCARD auto getMul(const Expr& expr) const -> const Mul&;

			EVO_NODISCARD auto createSMulWrap(
				const Expr& lhs, const Expr& rhs, std::string&& result_name = "", std::string&& wrapped_name = ""
			) -> Expr;
			EVO_NODISCARD auto getSMulWrap(const Expr& expr) const -> const SMulWrap&;
			EVO_NODISCARD static auto extractSMulWrapResult(const Expr& expr) -> Expr;
			EVO_NODISCARD static auto extractSMulWrapWrapped(const Expr& expr) -> Expr;

			EVO_NODISCARD auto createUMulWrap(
				const Expr& lhs, const Expr& rhs, std::string&& result_name = "", std::string&& wrapped_name = ""
			) -> Expr;
			EVO_NODISCARD auto getUMulWrap(const Expr& expr) const -> const UMulWrap&;
			EVO_NODISCARD static auto extractUMulWrapResult(const Expr& expr) -> Expr;
			EVO_NODISCARD static auto extractUMulWrapWrapped(const Expr& expr) -> Expr;

			EVO_NODISCARD auto createSMulSat(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getSMulSat(const Expr& expr) const -> const SMulSat&;

			EVO_NODISCARD auto createUMulSat(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getUMulSat(const Expr& expr) const -> const UMulSat&;

			EVO_NODISCARD auto createFMul(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getFMul(const Expr& expr) const -> const FMul&;


			///////////////////////////////////
			// div

			EVO_NODISCARD auto createSDiv(
				const Expr& lhs, const Expr& rhs, bool is_exact, std::string&& name = ""
			) const -> Expr;
			EVO_NODISCARD auto getSDiv(const Expr& expr) const -> const SDiv&;

			EVO_NODISCARD auto createUDiv(
				const Expr& lhs, const Expr& rhs, bool is_exact, std::string&& name = ""
			) const -> Expr;
			EVO_NODISCARD auto getUDiv(const Expr& expr) const -> const UDiv&;

			EVO_NODISCARD auto createFDiv(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getFDiv(const Expr& expr) const -> const FDiv&;


			///////////////////////////////////
			// rem

			EVO_NODISCARD auto createSRem(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getSRem(const Expr& expr) const -> const SRem&;

			EVO_NODISCARD auto createURem(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getURem(const Expr& expr) const -> const URem&;

			EVO_NODISCARD auto createFRem(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getFRem(const Expr& expr) const -> const FRem&;


			///////////////////////////////////
			// neg

			EVO_NODISCARD auto createFNeg(const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getFNeg(const Expr& expr) const -> const FNeg&;


			//////////////////////////////////////////////////////////////////////
			// comparison

			///////////////////////////////////
			// equal

			EVO_NODISCARD auto createIEq(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getIEq(const Expr& expr) const -> const IEq&;

			EVO_NODISCARD auto createFEq(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getFEq(const Expr& expr) const -> const FEq&;


			///////////////////////////////////
			// not equal

			EVO_NODISCARD auto createINeq(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getINeq(const Expr& expr) const -> const INeq&;

			EVO_NODISCARD auto createFNeq(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getFNeq(const Expr& expr) const -> const FNeq&;


			///////////////////////////////////
			// less than

			EVO_NODISCARD auto createSLT(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getSLT(const Expr& expr) const -> const SLT&;

			EVO_NODISCARD auto createULT(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getULT(const Expr& expr) const -> const ULT&;

			EVO_NODISCARD auto createFLT(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getFLT(const Expr& expr) const -> const FLT&;


			///////////////////////////////////
			// less than or equal to

			EVO_NODISCARD auto createSLTE(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getSLTE(const Expr& expr) const -> const SLTE&;

			EVO_NODISCARD auto createULTE(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getULTE(const Expr& expr) const -> const ULTE&;

			EVO_NODISCARD auto createFLTE(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getFLTE(const Expr& expr) const -> const FLTE&;


			///////////////////////////////////
			// greater than

			EVO_NODISCARD auto createSGT(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getSGT(const Expr& expr) const -> const SGT&;

			EVO_NODISCARD auto createUGT(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getUGT(const Expr& expr) const -> const UGT&;

			EVO_NODISCARD auto createFGT(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getFGT(const Expr& expr) const -> const FGT&;


			///////////////////////////////////
			// greater than or equal to

			EVO_NODISCARD auto createSGTE(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getSGTE(const Expr& expr) const -> const SGTE&;

			EVO_NODISCARD auto createUGTE(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getUGTE(const Expr& expr) const -> const UGTE&;

			EVO_NODISCARD auto createFGTE(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getFGTE(const Expr& expr) const -> const FGTE&;


			//////////////////////////////////////////////////////////////////////
			// bitwise

			EVO_NODISCARD auto createAnd(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getAnd(const Expr& expr) const -> const And&;

			EVO_NODISCARD auto createOr(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getOr(const Expr& expr) const -> const Or&;

			EVO_NODISCARD auto createXor(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getXor(const Expr& expr) const -> const Xor&;

			EVO_NODISCARD auto createSHL(
				const Expr& lhs, const Expr& rhs, bool nsw, bool nuw, std::string&& name = ""
			) const -> Expr;
			EVO_NODISCARD auto getSHL(const Expr& expr) const -> const SHL&;

			EVO_NODISCARD auto createSSHLSat(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getSSHLSat(const Expr& expr) const -> const SSHLSat&;

			EVO_NODISCARD auto createUSHLSat(const Expr& lhs, const Expr& rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getUSHLSat(const Expr& expr) const -> const USHLSat&;

			EVO_NODISCARD auto createSSHR(
				const Expr& lhs, const Expr& rhs, bool is_exact, std::string&& name = ""
			) const -> Expr;
			EVO_NODISCARD auto getSSHR(const Expr& expr) const -> const SSHR&;

			EVO_NODISCARD auto createUSHR(
				const Expr& lhs, const Expr& rhs, bool is_exact, std::string&& name = ""
			) const -> Expr;
			EVO_NODISCARD auto getUSHR(const Expr& expr) const -> const USHR&;




		private:
			auto insert_stmt(const Expr& stmt) const -> void;
			auto delete_expr(const Expr& expr) const -> void;

			EVO_NODISCARD auto name_exists_in_func(std::string_view) const -> bool;
			EVO_NODISCARD auto get_stmt_name(std::string&& name) const -> std::string;
			EVO_NODISCARD auto get_stmt_name_with_forward_include(
				std::string&& name, evo::ArrayProxy<std::string_view> forward_includes
			) const -> std::string;

			template<bool REPLACE_WITH_VALUE>
			EVO_NODISCARD auto replace_stmt_impl(Expr original, const Expr& replacement) const -> void;

	
		private:
			Module& module;
			Function* target_func;
			BasicBlock* target_basic_block;
			mutable size_t insert_index = std::numeric_limits<size_t>::max();
			// `insert_index` is mutable to allow for it to move when inserting / deleting stmts
	};


}


