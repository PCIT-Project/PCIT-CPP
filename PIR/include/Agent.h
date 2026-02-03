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
			auto setTargetBasicBlock(BasicBlock& basic_block) -> void;
			auto setTargetBasicBlockAtEnd() -> void;
			auto removeTargetBasicBlock() -> void;

			EVO_NODISCARD auto hasTargetBasicBlock() const -> bool { return this->target_basic_block != nullptr; }
			EVO_NODISCARD auto getTargetBasicBlock() const -> BasicBlock&;


			auto setInsertIndex(size_t index) -> void;
			auto setInsertIndexAtEnd() -> void;
			auto getInsertIndexAtEnd() const -> bool {return this->insert_index == std::numeric_limits<size_t>::max();}


			///////////////////////////////////
			// misc expr stuff

			EVO_NODISCARD auto getExprType(Expr expr) const -> Type;

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

			EVO_NODISCARD auto getNumber(Expr expr) const -> const Number&;


			///////////////////////////////////
			// booleans

			EVO_NODISCARD static auto createBoolean(bool value) -> Expr;

			EVO_NODISCARD static auto getBoolean(Expr expr) -> bool;


			///////////////////////////////////
			// nullptr

			EVO_NODISCARD static auto createNullptr() -> Expr;



			///////////////////////////////////
			// param exprs

			EVO_NODISCARD static auto createParamExpr(uint32_t index) -> Expr;

			EVO_NODISCARD static auto getParamExpr(Expr expr) -> ParamExpr;


			///////////////////////////////////
			// global values (expr)

			EVO_NODISCARD static auto createGlobalValue(const GlobalVar::ID& global_id) -> Expr;

			EVO_NODISCARD static auto getGlobalValue(Expr expr) -> GlobalVar::ID;


			///////////////////////////////////
			// function pointers


			EVO_NODISCARD static auto createFunctionPointer(const Function::ID& global_id) -> Expr;

			EVO_NODISCARD auto getFunctionPointer(Expr expr) const -> const Function&;




			///////////////////////////////////
			// calls

			EVO_NODISCARD auto createCall(
				Function::ID func, evo::SmallVector<Expr>&& args, std::string&& name = ""
			) const -> Expr;
			EVO_NODISCARD auto createCall(
				Function::ID func, const evo::SmallVector<Expr>& args, std::string&& name = ""
			) const -> Expr;

			EVO_NODISCARD auto createCall(
				ExternalFunction::ID func, evo::SmallVector<Expr>&& args, std::string&& name = ""
			) const -> Expr;
			EVO_NODISCARD auto createCall(
				ExternalFunction::ID func, const evo::SmallVector<Expr>& args, std::string&& name = ""
			) const -> Expr;

			EVO_NODISCARD auto createCall(
				Expr func, const Type& func_type, evo::SmallVector<Expr>&& args, std::string&& name = ""
			) const -> Expr;
			EVO_NODISCARD auto createCall(
				Expr func, const Type& func_type, const evo::SmallVector<Expr>& args, std::string&& name = ""
			) const -> Expr;

			EVO_NODISCARD auto getCall(Expr expr) const -> const Call&;


			///////////////////////////////////
			// call voids

			auto createCallVoid(Function::ID func, evo::SmallVector<Expr>&& args) const -> Expr;
			auto createCallVoid(Function::ID func, const evo::SmallVector<Expr>& args) const -> Expr;

			auto createCallVoid(ExternalFunction::ID func, evo::SmallVector<Expr>&& args) const -> Expr;
			auto createCallVoid(ExternalFunction::ID func, const evo::SmallVector<Expr>& args) const -> Expr;

			auto createCallVoid(Expr func, const Type& func_type, evo::SmallVector<Expr>&& args) const
				-> Expr;
			auto createCallVoid(Expr func, const Type& func_type, const evo::SmallVector<Expr>& args) const
				-> Expr;

			EVO_NODISCARD auto getCallVoid(Expr expr) const -> const CallVoid&;


			///////////////////////////////////
			// breakpoint / abort

			auto createAbort() const -> Expr;
			auto createBreakpoint() const -> Expr;


			///////////////////////////////////
			// ret instructions

			auto createRet(Expr expr) const -> Expr;
			auto createRet() const -> Expr;
			EVO_NODISCARD auto getRet(Expr expr) const -> const Ret&;


			///////////////////////////////////
			// branch instructions

			auto createJump(BasicBlock::ID basic_block_id) const -> Expr;
			EVO_NODISCARD static auto getJump(Expr expr) -> Jump;


			///////////////////////////////////
			// condiitonal branch instructions

			auto createBranch(Expr cond, BasicBlock::ID then_block, BasicBlock::ID else_block) const -> Expr;
			EVO_NODISCARD auto getBranch(Expr expr) const -> Branch;


			///////////////////////////////////
			// unreachable

			auto createUnreachable() const -> Expr;


			///////////////////////////////////
			// phi instructions

			auto createPhi(evo::SmallVector<Phi::Predecessor>&& predecessors, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getPhi(Expr expr) const -> Phi;


			///////////////////////////////////
			// switches

			auto createSwitch(Expr cond, evo::SmallVector<Switch::Case>&& cases, BasicBlock::ID default_block) const
				-> Expr;
			EVO_NODISCARD auto getSwitch(Expr expr) const -> Switch;


			///////////////////////////////////
			// alloca

			EVO_NODISCARD auto createAlloca(const Type& type, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getAlloca(Expr expr) const -> const Alloca&;


			///////////////////////////////////
			// load

			EVO_NODISCARD auto createLoad(
				Expr source,
				const Type& type,
				std::string&& name = "",
				bool is_volatile = false,
				AtomicOrdering atomic_ordering = AtomicOrdering::NONE
			) const -> Expr;
			
			EVO_NODISCARD auto getLoad(Expr expr) const -> const Load&;


			///////////////////////////////////
			// store

			EVO_NODISCARD auto createStore(
				Expr destination,
				Expr value,
				bool is_volatile = false,
				AtomicOrdering atomic_ordering = AtomicOrdering::NONE
			) const -> void;

			EVO_NODISCARD auto createStore(const Expr&, const Type&, const char*) const -> Expr = delete;

			EVO_NODISCARD auto getStore(Expr expr) const -> const Store&;


			///////////////////////////////////
			// calc ptr

			EVO_NODISCARD auto createCalcPtr(
				Expr base_ptr,
				const Type& ptr_type,
				evo::SmallVector<CalcPtr::Index>&& indices,
				std::string&& name = ""
			) const -> Expr;
			EVO_NODISCARD auto getCalcPtr(Expr expr) const -> const CalcPtr&;


			///////////////////////////////////
			// memcpy

			auto createMemcpy(Expr dst, Expr src, Expr num_bytes, bool is_volatile = false) const -> Expr;

			auto createMemcpy(Expr dst, Expr src, const Type& src_type, bool is_volatile = false) const
			-> Expr {
				return this->createMemcpy(dst, src, this->module.getSize(src_type, true), is_volatile);
			}

			auto createMemcpy(Expr dst, Expr src, size_t num_bytes, bool is_volatile = false) const
			-> Expr {
				const pir::Expr num_bytes_expr = this->createNumber(
					this->module.createIntegerType(unsigned(this->module.sizeOfPtr() * 8)),
					core::GenericInt(unsigned(this->module.sizeOfPtr() * 8), unsigned(num_bytes))
				);

				return this->createMemcpy(dst, src, num_bytes_expr, is_volatile);
			}

			EVO_NODISCARD auto getMemcpy(Expr expr) const -> const Memcpy&;


			///////////////////////////////////
			// memset

			auto createMemset(Expr dst, Expr value, Expr num_bytes, bool is_volatile = false) const
				-> Expr;

			auto createMemset(Expr dst, Expr value, const Type& dst_type, bool is_volatile = false) const
			-> Expr {
				return this->createMemset(dst, value, this->module.getSize(dst_type, true), is_volatile);
			}

			auto createMemset(Expr dst, Expr value, size_t num_bytes, bool is_volatile = false) const
			-> Expr {
				const pir::Expr num_bytes_expr = this->createNumber(
					this->module.createIntegerType(unsigned(this->module.sizeOfPtr() * 8)),
					core::GenericInt(unsigned(this->module.sizeOfPtr() * 8), unsigned(num_bytes))
				);

				return this->createMemset(dst, value, num_bytes_expr, is_volatile);
			}

			EVO_NODISCARD auto getMemset(Expr expr) const -> const Memset&;


			//////////////////////////////////////////////////////////////////////
			// type conversion

			
			///////////////////////////////////
			// BitCast

			EVO_NODISCARD auto createBitCast(Expr fromValue, const Type& toType, std::string&& name = "") const
				-> Expr;
			EVO_NODISCARD auto getBitCast(Expr expr) const -> const BitCast&;

			
			///////////////////////////////////
			// Trunc

			EVO_NODISCARD auto createTrunc(Expr fromValue, const Type& toType, std::string&& name = "") const
				-> Expr;
			EVO_NODISCARD auto getTrunc(Expr expr) const -> const Trunc&;

			
			///////////////////////////////////
			// FTrunc

			EVO_NODISCARD auto createFTrunc(Expr fromValue, const Type& toType, std::string&& name = "") const
				-> Expr;
			EVO_NODISCARD auto getFTrunc(Expr expr) const -> const FTrunc&;

			
			///////////////////////////////////
			// SExt

			EVO_NODISCARD auto createSExt(Expr fromValue, const Type& toType, std::string&& name = "") const
				-> Expr;
			EVO_NODISCARD auto getSExt(Expr expr) const -> const SExt&;

			
			///////////////////////////////////
			// ZExt

			EVO_NODISCARD auto createZExt(Expr fromValue, const Type& toType, std::string&& name = "") const
				-> Expr;
			EVO_NODISCARD auto getZExt(Expr expr) const -> const ZExt&;

			
			///////////////////////////////////
			// FExt

			EVO_NODISCARD auto createFExt(Expr fromValue, const Type& toType, std::string&& name = "") const
				-> Expr;
			EVO_NODISCARD auto getFExt(Expr expr) const -> const FExt&;

			
			///////////////////////////////////
			// IToF

			EVO_NODISCARD auto createIToF(Expr fromValue, const Type& toType, std::string&& name = "") const
				-> Expr;
			EVO_NODISCARD auto getIToF(Expr expr) const -> const IToF&;

			
			///////////////////////////////////
			// UIToF

			EVO_NODISCARD auto createUIToF(Expr fromValue, const Type& toType, std::string&& name = "") const
				-> Expr;
			EVO_NODISCARD auto getUIToF(Expr expr) const -> const UIToF&;

			
			///////////////////////////////////
			// FToI

			EVO_NODISCARD auto createFToI(Expr fromValue, const Type& toType, std::string&& name = "") const
				-> Expr;
			EVO_NODISCARD auto getFToI(Expr expr) const -> const FToI&;

			
			///////////////////////////////////
			// FToUI

			EVO_NODISCARD auto createFToUI(Expr fromValue, const Type& toType, std::string&& name = "") const
				-> Expr;
			EVO_NODISCARD auto getFToUI(Expr expr) const -> const FToUI&;



			//////////////////////////////////////////////////////////////////////
			// arithmetic

			///////////////////////////////////
			// add

			EVO_NODISCARD auto createAdd(
				Expr lhs, Expr rhs, bool nsw, bool nuw, std::string&& name = ""
			) const -> Expr;
			EVO_NODISCARD auto getAdd(Expr expr) const -> const Add&;

			EVO_NODISCARD auto createSAddWrap(
				Expr lhs, Expr rhs, std::string&& result_name = "", std::string&& wrapped_name = ""
			) -> Expr;
			EVO_NODISCARD auto getSAddWrap(Expr expr) const -> const SAddWrap&;
			EVO_NODISCARD static auto extractSAddWrapResult(Expr expr) -> Expr;
			EVO_NODISCARD static auto extractSAddWrapWrapped(Expr expr) -> Expr;

			EVO_NODISCARD auto createUAddWrap(
				Expr lhs, Expr rhs, std::string&& result_name = "", std::string&& wrapped_name = ""
			) -> Expr;
			EVO_NODISCARD auto getUAddWrap(Expr expr) const -> const UAddWrap&;
			EVO_NODISCARD static auto extractUAddWrapResult(Expr expr) -> Expr;
			EVO_NODISCARD static auto extractUAddWrapWrapped(Expr expr) -> Expr;

			EVO_NODISCARD auto createSAddSat(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getSAddSat(Expr expr) const -> const SAddSat&;

			EVO_NODISCARD auto createUAddSat(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getUAddSat(Expr expr) const -> const UAddSat&;

			EVO_NODISCARD auto createFAdd(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getFAdd(Expr expr) const -> const FAdd&;


			///////////////////////////////////
			// sub

			EVO_NODISCARD auto createSub(
				Expr lhs, Expr rhs, bool nsw, bool nuw, std::string&& name = ""
			) const -> Expr;
			EVO_NODISCARD auto getSub(Expr expr) const -> const Sub&;

			EVO_NODISCARD auto createSSubWrap(
				Expr lhs, Expr rhs, std::string&& result_name = "", std::string&& wrapped_name = ""
			) -> Expr;
			EVO_NODISCARD auto getSSubWrap(Expr expr) const -> const SSubWrap&;
			EVO_NODISCARD static auto extractSSubWrapResult(Expr expr) -> Expr;
			EVO_NODISCARD static auto extractSSubWrapWrapped(Expr expr) -> Expr;

			EVO_NODISCARD auto createUSubWrap(
				Expr lhs, Expr rhs, std::string&& result_name = "", std::string&& wrapped_name = ""
			) -> Expr;
			EVO_NODISCARD auto getUSubWrap(Expr expr) const -> const USubWrap&;
			EVO_NODISCARD static auto extractUSubWrapResult(Expr expr) -> Expr;
			EVO_NODISCARD static auto extractUSubWrapWrapped(Expr expr) -> Expr;

			EVO_NODISCARD auto createSSubSat(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getSSubSat(Expr expr) const -> const SSubSat&;

			EVO_NODISCARD auto createUSubSat(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getUSubSat(Expr expr) const -> const USubSat&;

			EVO_NODISCARD auto createFSub(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getFSub(Expr expr) const -> const FSub&;


			///////////////////////////////////
			// mul

			EVO_NODISCARD auto createMul(
				Expr lhs, Expr rhs, bool nsw, bool nuw, std::string&& name = ""
			) const -> Expr;
			EVO_NODISCARD auto getMul(Expr expr) const -> const Mul&;

			EVO_NODISCARD auto createSMulWrap(
				Expr lhs, Expr rhs, std::string&& result_name = "", std::string&& wrapped_name = ""
			) -> Expr;
			EVO_NODISCARD auto getSMulWrap(Expr expr) const -> const SMulWrap&;
			EVO_NODISCARD static auto extractSMulWrapResult(Expr expr) -> Expr;
			EVO_NODISCARD static auto extractSMulWrapWrapped(Expr expr) -> Expr;

			EVO_NODISCARD auto createUMulWrap(
				Expr lhs, Expr rhs, std::string&& result_name = "", std::string&& wrapped_name = ""
			) -> Expr;
			EVO_NODISCARD auto getUMulWrap(Expr expr) const -> const UMulWrap&;
			EVO_NODISCARD static auto extractUMulWrapResult(Expr expr) -> Expr;
			EVO_NODISCARD static auto extractUMulWrapWrapped(Expr expr) -> Expr;

			EVO_NODISCARD auto createSMulSat(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getSMulSat(Expr expr) const -> const SMulSat&;

			EVO_NODISCARD auto createUMulSat(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getUMulSat(Expr expr) const -> const UMulSat&;

			EVO_NODISCARD auto createFMul(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getFMul(Expr expr) const -> const FMul&;


			///////////////////////////////////
			// div

			EVO_NODISCARD auto createSDiv(
				Expr lhs, Expr rhs, bool is_exact, std::string&& name = ""
			) const -> Expr;
			EVO_NODISCARD auto getSDiv(Expr expr) const -> const SDiv&;

			EVO_NODISCARD auto createUDiv(
				Expr lhs, Expr rhs, bool is_exact, std::string&& name = ""
			) const -> Expr;
			EVO_NODISCARD auto getUDiv(Expr expr) const -> const UDiv&;

			EVO_NODISCARD auto createFDiv(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getFDiv(Expr expr) const -> const FDiv&;


			///////////////////////////////////
			// rem

			EVO_NODISCARD auto createSRem(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getSRem(Expr expr) const -> const SRem&;

			EVO_NODISCARD auto createURem(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getURem(Expr expr) const -> const URem&;

			EVO_NODISCARD auto createFRem(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getFRem(Expr expr) const -> const FRem&;


			///////////////////////////////////
			// neg

			EVO_NODISCARD auto createFNeg(Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getFNeg(Expr expr) const -> const FNeg&;


			//////////////////////////////////////////////////////////////////////
			// comparison

			///////////////////////////////////
			// equal

			EVO_NODISCARD auto createIEq(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getIEq(Expr expr) const -> const IEq&;

			EVO_NODISCARD auto createFEq(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getFEq(Expr expr) const -> const FEq&;


			///////////////////////////////////
			// not equal

			EVO_NODISCARD auto createINeq(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getINeq(Expr expr) const -> const INeq&;

			EVO_NODISCARD auto createFNeq(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getFNeq(Expr expr) const -> const FNeq&;


			///////////////////////////////////
			// less than

			EVO_NODISCARD auto createSLT(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getSLT(Expr expr) const -> const SLT&;

			EVO_NODISCARD auto createULT(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getULT(Expr expr) const -> const ULT&;

			EVO_NODISCARD auto createFLT(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getFLT(Expr expr) const -> const FLT&;


			///////////////////////////////////
			// less than or equal to

			EVO_NODISCARD auto createSLTE(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getSLTE(Expr expr) const -> const SLTE&;

			EVO_NODISCARD auto createULTE(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getULTE(Expr expr) const -> const ULTE&;

			EVO_NODISCARD auto createFLTE(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getFLTE(Expr expr) const -> const FLTE&;


			///////////////////////////////////
			// greater than

			EVO_NODISCARD auto createSGT(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getSGT(Expr expr) const -> const SGT&;

			EVO_NODISCARD auto createUGT(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getUGT(Expr expr) const -> const UGT&;

			EVO_NODISCARD auto createFGT(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getFGT(Expr expr) const -> const FGT&;


			///////////////////////////////////
			// greater than or equal to

			EVO_NODISCARD auto createSGTE(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getSGTE(Expr expr) const -> const SGTE&;

			EVO_NODISCARD auto createUGTE(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getUGTE(Expr expr) const -> const UGTE&;

			EVO_NODISCARD auto createFGTE(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getFGTE(Expr expr) const -> const FGTE&;


			//////////////////////////////////////////////////////////////////////
			// bitwise

			EVO_NODISCARD auto createAnd(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getAnd(Expr expr) const -> const And&;

			EVO_NODISCARD auto createOr(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getOr(Expr expr) const -> const Or&;

			EVO_NODISCARD auto createXor(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getXor(Expr expr) const -> const Xor&;

			EVO_NODISCARD auto createSHL(Expr lhs, Expr rhs, bool nsw, bool nuw, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getSHL(Expr expr) const -> const SHL&;

			EVO_NODISCARD auto createSSHLSat(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getSSHLSat(Expr expr) const -> const SSHLSat&;

			EVO_NODISCARD auto createUSHLSat(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getUSHLSat(Expr expr) const -> const USHLSat&;

			EVO_NODISCARD auto createSSHR(Expr lhs, Expr rhs, bool is_exact, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getSSHR(Expr expr) const -> const SSHR&;

			EVO_NODISCARD auto createUSHR(Expr lhs, Expr rhs, bool is_exact, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getUSHR(Expr expr) const -> const USHR&;



			//////////////////////////////////////////////////////////////////////
			// bit operations
			
			EVO_NODISCARD auto createBitReverse(Expr expr, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getBitReverse(Expr expr) const -> const BitReverse&;

			EVO_NODISCARD auto createByteSwap(Expr expr, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getByteSwap(Expr expr) const -> const ByteSwap&;

			EVO_NODISCARD auto createCtPop(Expr expr, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getCtPop(Expr expr) const -> const CtPop&;

			EVO_NODISCARD auto createCTLZ(Expr expr, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getCTLZ(Expr expr) const -> const CTLZ&;

			EVO_NODISCARD auto createCTTZ(Expr expr, std::string&& name = "") const -> Expr;
			EVO_NODISCARD auto getCTTZ(Expr expr) const -> const CTTZ&;


			//////////////////////////////////////////////////////////////////////
			// atomics

			EVO_NODISCARD auto createCmpXchg(
				Expr target,
				Expr expected,
				Expr desired,
				std::string&& loaded_name = "",
				std::string&& succeeded_name = "",
				bool isWeak = false,
				AtomicOrdering success_ordering = AtomicOrdering::SEQUENTIALLY_CONSISTENT,
				AtomicOrdering failure_ordering = AtomicOrdering::SEQUENTIALLY_CONSISTENT
			) const -> Expr;
			EVO_NODISCARD auto getCmpXchg(Expr expr) const -> const CmpXchg&;

			EVO_NODISCARD static auto extractCmpXchgLoaded(Expr) -> Expr;
			EVO_NODISCARD static auto extractCmpXchgSucceeded(Expr) -> Expr;



			EVO_NODISCARD auto createAtomicRMW(
				AtomicRMW::Op op,
				Expr target,
				Expr value,
				std::string&& name = "",
				AtomicOrdering ordering = AtomicOrdering::SEQUENTIALLY_CONSISTENT
			) const -> Expr;
			EVO_NODISCARD auto getAtomicRMW(Expr expr) const -> const AtomicRMW&;


			//////////////////////////////////////////////////////////////////////
			// optimizations

			auto createLifetimeStart(Expr expr, uint64_t size) const -> Expr;
			EVO_NODISCARD auto getLifetimeStart(Expr expr) const -> const LifetimeStart&;

			auto createLifetimeEnd(Expr expr, uint64_t size) const -> Expr;
			EVO_NODISCARD auto getLifetimeEnd(Expr expr) const -> const LifetimeEnd&;


		private:
			auto insert_stmt(Expr stmt) const -> void;
			auto delete_expr(Expr expr) const -> void;

			EVO_NODISCARD auto name_exists_in_func(std::string_view) const -> bool;
			EVO_NODISCARD auto get_stmt_name(std::string&& name) const -> std::string;
			EVO_NODISCARD auto get_stmt_name_with_forward_include(
				std::string&& name, evo::ArrayProxy<std::string_view> forward_includes
			) const -> std::string;

			template<bool REPLACE_WITH_VALUE>
			EVO_NODISCARD auto replace_stmt_impl(Expr original, Expr replacement) const -> void;

	
		private:
			Module& module;
			Function* target_func;
			BasicBlock* target_basic_block;
			mutable size_t insert_index = std::numeric_limits<size_t>::max();
			// `insert_index` is mutable to allow for it to move when inserting / deleting stmts
	};


}


