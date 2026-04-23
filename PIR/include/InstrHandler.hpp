////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.hpp>
#include <PCIT_core.hpp>


#include "./Expr.hpp"
#include "./BasicBlock.hpp"
#include "./Function.hpp"
#include "./Module.hpp"



namespace pcit::pir{


	class InstrHandler{
		public:
			InstrHandler(Module& _module) : module(_module), target_func(nullptr), target_basic_block(nullptr) {}
			InstrHandler(Module& _module, Function& func)
				: module(_module), target_func(&func), target_basic_block(nullptr) {}
			InstrHandler(Module& _module, Function& func, BasicBlock& basic_block)
				: module(_module), target_func(&func), target_basic_block(&basic_block) {}
			InstrHandler(Module& _module, Function& func, BasicBlock& basic_block, size_t _insert_index)
				: module(_module), target_func(&func), target_basic_block(&basic_block), insert_index(_insert_index) {}

			~InstrHandler() = default;


			///////////////////////////////////
			// targets

			[[nodiscard]] auto getModule() const -> Module& { return this->module; }

			auto setTargetFunction(Function::ID id) -> void;
			auto setTargetFunction(Function& func) -> void;
			auto removeTargetFunction() -> void;

			[[nodiscard]] auto hasTargetFunction() const -> bool { return this->target_func != nullptr; }
			[[nodiscard]] auto getTargetFunction() const -> Function&;


			auto setTargetBasicBlock(BasicBlock::ID id) -> void;
			auto setTargetBasicBlock(BasicBlock& basic_block) -> void;
			auto setTargetBasicBlockAtEnd() -> void;
			auto removeTargetBasicBlock() -> void;
			auto deleteBodyOfTargetBasicBlock() -> void;

			[[nodiscard]] auto hasTargetBasicBlock() const -> bool { return this->target_basic_block != nullptr; }
			[[nodiscard]] auto getTargetBasicBlock() const -> BasicBlock&;


			auto setInsertIndex(size_t index) -> void;
			auto setInsertIndexAtEnd() -> void;
			auto getInsertIndexAtEnd() const -> bool {return this->insert_index == std::numeric_limits<size_t>::max();}


			//////////////////
			// source location

			auto pushSourceLocation(meta::SourceLocation source_location) -> void;
			auto popSourceLocation() -> void;


			struct DeferPopSourceLocation{
				DeferPopSourceLocation(InstrHandler& handler) : _handler(&handler) {}

				~DeferPopSourceLocation(){ if(this->_handler != nullptr){ this->_handler->popSourceLocation(); } }

				DeferPopSourceLocation(const DeferPopSourceLocation&) = delete;
				DeferPopSourceLocation(DeferPopSourceLocation&& rhs) : _handler(std::exchange(rhs._handler, nullptr)) {}

				private:
					InstrHandler* _handler;
			};

			[[nodiscard]] auto scopedSourceLocation(meta::SourceLocation source_location) -> DeferPopSourceLocation;



			///////////////////////////////////
			// misc expr stuff

			[[nodiscard]] auto getExprType(Expr expr) const -> Type;

			auto replaceExpr(Expr original, Expr replacement) const -> void;
			auto removeStmt(Expr stmt_to_remove) const -> void;


			///////////////////////////////////
			// basic blocks

			auto createBasicBlock(Function::ID func, std::string&& name = "") const -> BasicBlock::ID;
			auto createBasicBlock(Function& func, std::string&& name = "") const -> BasicBlock::ID;
			auto createBasicBlock(std::string&& name = "") const -> BasicBlock::ID;
			auto createBasicBlockInline(std::string&& name = "") const -> BasicBlock::ID;

			[[nodiscard]] auto getBasicBlock(BasicBlock::ID id) const -> BasicBlock&;


			///////////////////////////////////
			// numbers

			[[nodiscard]] auto createNumber(Type type, core::GenericInt&& value) const -> Expr;
			[[nodiscard]] auto createNumber(Type type, const core::GenericInt& value) const -> Expr;
			[[nodiscard]] auto createNumber(Type type, core::GenericFloat&& value) const -> Expr;
			[[nodiscard]] auto createNumber(Type type, const core::GenericFloat& value) const -> Expr;

			[[nodiscard]] auto getNumber(Expr expr) const -> const Number&;


			///////////////////////////////////
			// booleans

			[[nodiscard]] static auto createBoolean(bool value) -> Expr;

			[[nodiscard]] static auto getBoolean(Expr expr) -> bool;


			///////////////////////////////////
			// nullptr

			[[nodiscard]] static auto createNullptr() -> Expr;



			///////////////////////////////////
			// param exprs

			[[nodiscard]] static auto createParamExpr(uint32_t index) -> Expr;

			[[nodiscard]] static auto getParamExpr(Expr expr) -> ParamExpr;


			///////////////////////////////////
			// global values (expr)

			[[nodiscard]] static auto createGlobalValue(GlobalVar::ID global_id) -> Expr;

			[[nodiscard]] static auto getGlobalValue(Expr expr) -> GlobalVar::ID;


			///////////////////////////////////
			// function pointers


			[[nodiscard]] static auto createFunctionPointer(Function::ID function_id) -> Expr;

			[[nodiscard]] static auto getFunctionPointer(Expr expr) -> Function::ID;




			///////////////////////////////////
			// calls

			[[nodiscard]] auto createCall(
				Function::ID func, evo::SmallVector<Expr>&& args, std::string&& name = ""
			) const -> Expr;
			[[nodiscard]] auto createCall(
				Function::ID func, const evo::SmallVector<Expr>& args, std::string&& name = ""
			) const -> Expr;

			[[nodiscard]] auto createCall(
				ExternalFunction::ID func, evo::SmallVector<Expr>&& args, std::string&& name = ""
			) const -> Expr;
			[[nodiscard]] auto createCall(
				ExternalFunction::ID func, const evo::SmallVector<Expr>& args, std::string&& name = ""
			) const -> Expr;

			[[nodiscard]] auto createCall(
				Expr func, Type func_type, evo::SmallVector<Expr>&& args, std::string&& name = ""
			) const -> Expr;
			[[nodiscard]] auto createCall(
				Expr func, Type func_type, const evo::SmallVector<Expr>& args, std::string&& name = ""
			) const -> Expr;

			[[nodiscard]] auto getCall(Expr expr) const -> const Call&;


			///////////////////////////////////
			// call voids

			auto createCallVoid(Function::ID func, evo::SmallVector<Expr>&& args) const -> Expr;
			auto createCallVoid(Function::ID func, const evo::SmallVector<Expr>& args) const -> Expr;

			auto createCallVoid(ExternalFunction::ID func, evo::SmallVector<Expr>&& args) const -> Expr;
			auto createCallVoid(ExternalFunction::ID func, const evo::SmallVector<Expr>& args) const -> Expr;

			auto createCallVoid(Expr func, Type func_type, evo::SmallVector<Expr>&& args) const -> Expr;
			auto createCallVoid(Expr func, Type func_type, const evo::SmallVector<Expr>& args) const -> Expr;

			[[nodiscard]] auto getCallVoid(Expr expr) const -> const CallVoid&;


			///////////////////////////////////
			// call no return

			auto createCallNoReturn(Function::ID func, evo::SmallVector<Expr>&& args) const -> Expr;
			auto createCallNoReturn(Function::ID func, const evo::SmallVector<Expr>& args) const -> Expr;

			auto createCallNoReturn(ExternalFunction::ID func, evo::SmallVector<Expr>&& args) const -> Expr;
			auto createCallNoReturn(ExternalFunction::ID func, const evo::SmallVector<Expr>& args) const -> Expr;

			auto createCallNoReturn(Expr func, Type func_type, evo::SmallVector<Expr>&& args) const -> Expr;
			auto createCallNoReturn(Expr func, Type func_type, const evo::SmallVector<Expr>& args) const -> Expr;

			[[nodiscard]] auto getCallNoReturn(Expr expr) const -> const CallNoReturn&;


			///////////////////////////////////
			// breakpoint / abort

			auto createAbort() const -> Expr;
			[[nodiscard]] auto getAbort(Expr expr) const -> const Abort&;

			auto createBreakpoint() const -> Expr;
			[[nodiscard]] auto getBreakpoint(Expr expr) const -> const Breakpoint&;


			///////////////////////////////////
			// ret instructions

			auto createRet(Expr expr) const -> Expr;
			auto createRet() const -> Expr;

			[[nodiscard]] auto getRet(Expr expr) const -> const Ret&;


			///////////////////////////////////
			// branch instructions

			auto createJump(BasicBlock::ID basic_block_id) const -> Expr;

			[[nodiscard]] auto getJump(Expr expr) const -> const Jump&;


			///////////////////////////////////
			// condiitonal branch instructions

			auto createBranch(Expr cond, BasicBlock::ID then_block, BasicBlock::ID else_block) const -> Expr;
			[[nodiscard]] auto getBranch(Expr expr) const -> const Branch&;


			///////////////////////////////////
			// unreachable

			auto createUnreachable() const -> Expr;


			///////////////////////////////////
			// phi instructions

			auto createPhi(evo::SmallVector<Phi::Predecessor>&& predecessors, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getPhi(Expr expr) const -> const Phi&;


			///////////////////////////////////
			// switches

			auto createSwitch(Expr cond, evo::SmallVector<Switch::Case>&& cases, BasicBlock::ID default_block) const
				-> Expr;
			[[nodiscard]] auto getSwitch(Expr expr) const -> const Switch&;


			///////////////////////////////////
			// alloca

			[[nodiscard]] auto createAlloca(Type type, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getAlloca(Expr expr) const -> const Alloca&;


			///////////////////////////////////
			// load

			[[nodiscard]] auto createLoad(
				Expr source,
				Type type,
				std::string&& name = "",
				bool is_volatile = false,
				AtomicOrdering atomic_ordering = AtomicOrdering::NONE
			) const -> Expr;
			
			[[nodiscard]] auto getLoad(Expr expr) const -> const Load&;


			///////////////////////////////////
			// store

			[[nodiscard]] auto createStore(
				Expr destination,
				Expr value,
				bool is_volatile = false,
				AtomicOrdering atomic_ordering = AtomicOrdering::NONE
			) const -> void;

			[[nodiscard]] auto createStore(const Expr&, Type, const char*) const -> Expr = delete;

			[[nodiscard]] auto getStore(Expr expr) const -> const Store&;


			///////////////////////////////////
			// calc ptr

			[[nodiscard]] auto createCalcPtr(
				Expr base_ptr,
				Type ptr_type,
				evo::SmallVector<CalcPtr::Index>&& indices,
				std::string&& name = ""
			) const -> Expr;
			[[nodiscard]] auto getCalcPtr(Expr expr) const -> const CalcPtr&;


			///////////////////////////////////
			// memcpy

			auto createMemcpy(Expr dst, Expr src, Expr num_bytes, bool is_volatile = false) const -> Expr;

			auto createMemcpy(Expr dst, Expr src, Type src_type, bool is_volatile = false) const
			-> Expr {
				return this->createMemcpy(dst, src, this->module.numBytes(src_type), is_volatile);
			}

			auto createMemcpy(Expr dst, Expr src, size_t num_bytes, bool is_volatile = false) const
			-> Expr {
				const pir::Expr num_bytes_expr = this->createNumber(
					this->module.createUnsignedType(unsigned(this->module.sizeOfPtr() * 8)),
					core::GenericInt(unsigned(this->module.sizeOfPtr() * 8), unsigned(num_bytes))
				);

				return this->createMemcpy(dst, src, num_bytes_expr, is_volatile);
			}

			[[nodiscard]] auto getMemcpy(Expr expr) const -> const Memcpy&;


			///////////////////////////////////
			// memset

			auto createMemset(Expr dst, Expr value, Expr num_bytes, bool is_volatile = false) const
				-> Expr;

			auto createMemset(Expr dst, Expr value, Type dst_type, bool is_volatile = false) const
			-> Expr {
				return this->createMemset(dst, value, this->module.numBytes(dst_type), is_volatile);
			}

			auto createMemset(Expr dst, Expr value, size_t num_bytes, bool is_volatile = false) const
			-> Expr {
				const pir::Expr num_bytes_expr = this->createNumber(
					this->module.createUnsignedType(unsigned(this->module.sizeOfPtr() * 8)),
					core::GenericInt(unsigned(this->module.sizeOfPtr() * 8), unsigned(num_bytes))
				);

				return this->createMemset(dst, value, num_bytes_expr, is_volatile);
			}

			[[nodiscard]] auto getMemset(Expr expr) const -> const Memset&;


			//////////////////////////////////////////////////////////////////////
			// type conversion

			
			///////////////////////////////////
			// BitCast

			[[nodiscard]] auto createBitCast(Expr fromValue, Type toType, std::string&& name = "") const
				-> Expr;
			[[nodiscard]] auto getBitCast(Expr expr) const -> const BitCast&;

			
			///////////////////////////////////
			// Trunc

			[[nodiscard]] auto createTrunc(Expr fromValue, Type toType, std::string&& name = "") const
				-> Expr;
			[[nodiscard]] auto getTrunc(Expr expr) const -> const Trunc&;

			
			///////////////////////////////////
			// FTrunc

			[[nodiscard]] auto createFTrunc(Expr fromValue, Type toType, std::string&& name = "") const
				-> Expr;
			[[nodiscard]] auto getFTrunc(Expr expr) const -> const FTrunc&;

			
			///////////////////////////////////
			// SExt

			[[nodiscard]] auto createSExt(Expr fromValue, Type toType, std::string&& name = "") const
				-> Expr;
			[[nodiscard]] auto getSExt(Expr expr) const -> const SExt&;

			
			///////////////////////////////////
			// ZExt

			[[nodiscard]] auto createZExt(Expr fromValue, Type toType, std::string&& name = "") const
				-> Expr;
			[[nodiscard]] auto getZExt(Expr expr) const -> const ZExt&;


			///////////////////////////////////
			// Ext (select SExt or ZExt)

			[[nodiscard]] auto createExt(Expr fromValue, Type toType, std::string&& name = "") const -> Expr {
				if(toType.kind() == Type::Kind::UNSIGNED){
					return this->createZExt(fromValue, toType, std::move(name));
				}else{
					return this->createSExt(fromValue, toType, std::move(name));
				}
			}

			
			///////////////////////////////////
			// FExt

			[[nodiscard]] auto createFExt(Expr fromValue, Type toType, std::string&& name = "") const
				-> Expr;
			[[nodiscard]] auto getFExt(Expr expr) const -> const FExt&;

			
			///////////////////////////////////
			// IToF

			[[nodiscard]] auto createIToF(Expr fromValue, Type toType, std::string&& name = "") const
				-> Expr;
			[[nodiscard]] auto getIToF(Expr expr) const -> const IToF&;

			
			///////////////////////////////////
			// UIToF

			[[nodiscard]] auto createUIToF(Expr fromValue, Type toType, std::string&& name = "") const
				-> Expr;
			[[nodiscard]] auto getUIToF(Expr expr) const -> const UIToF&;

			
			///////////////////////////////////
			// FToI

			[[nodiscard]] auto createFToI(Expr fromValue, Type toType, std::string&& name = "") const
				-> Expr;
			[[nodiscard]] auto getFToI(Expr expr) const -> const FToI&;

			
			///////////////////////////////////
			// FToUI

			[[nodiscard]] auto createFToUI(Expr fromValue, Type toType, std::string&& name = "") const
				-> Expr;
			[[nodiscard]] auto getFToUI(Expr expr) const -> const FToUI&;



			//////////////////////////////////////////////////////////////////////
			// arithmetic

			///////////////////////////////////
			// add

			[[nodiscard]] auto createAdd(
				Expr lhs, Expr rhs, bool nsw, bool nuw, std::string&& name = ""
			) const -> Expr;
			[[nodiscard]] auto getAdd(Expr expr) const -> const Add&;

			[[nodiscard]] auto createSAddWrap(
				Expr lhs, Expr rhs, std::string&& result_name = "", std::string&& wrapped_name = ""
			) -> Expr;
			[[nodiscard]] auto getSAddWrap(Expr expr) const -> const SAddWrap&;
			[[nodiscard]] static auto extractSAddWrapResult(Expr expr) -> Expr;
			[[nodiscard]] static auto extractSAddWrapWrapped(Expr expr) -> Expr;

			[[nodiscard]] auto createUAddWrap(
				Expr lhs, Expr rhs, std::string&& result_name = "", std::string&& wrapped_name = ""
			) -> Expr;
			[[nodiscard]] auto getUAddWrap(Expr expr) const -> const UAddWrap&;
			[[nodiscard]] static auto extractUAddWrapResult(Expr expr) -> Expr;
			[[nodiscard]] static auto extractUAddWrapWrapped(Expr expr) -> Expr;

			[[nodiscard]] auto createSAddSat(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getSAddSat(Expr expr) const -> const SAddSat&;

			[[nodiscard]] auto createUAddSat(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getUAddSat(Expr expr) const -> const UAddSat&;

			[[nodiscard]] auto createFAdd(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getFAdd(Expr expr) const -> const FAdd&;


			///////////////////////////////////
			// sub

			[[nodiscard]] auto createSub(
				Expr lhs, Expr rhs, bool nsw, bool nuw, std::string&& name = ""
			) const -> Expr;
			[[nodiscard]] auto getSub(Expr expr) const -> const Sub&;

			[[nodiscard]] auto createSSubWrap(
				Expr lhs, Expr rhs, std::string&& result_name = "", std::string&& wrapped_name = ""
			) -> Expr;
			[[nodiscard]] auto getSSubWrap(Expr expr) const -> const SSubWrap&;
			[[nodiscard]] static auto extractSSubWrapResult(Expr expr) -> Expr;
			[[nodiscard]] static auto extractSSubWrapWrapped(Expr expr) -> Expr;

			[[nodiscard]] auto createUSubWrap(
				Expr lhs, Expr rhs, std::string&& result_name = "", std::string&& wrapped_name = ""
			) -> Expr;
			[[nodiscard]] auto getUSubWrap(Expr expr) const -> const USubWrap&;
			[[nodiscard]] static auto extractUSubWrapResult(Expr expr) -> Expr;
			[[nodiscard]] static auto extractUSubWrapWrapped(Expr expr) -> Expr;

			[[nodiscard]] auto createSSubSat(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getSSubSat(Expr expr) const -> const SSubSat&;

			[[nodiscard]] auto createUSubSat(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getUSubSat(Expr expr) const -> const USubSat&;

			[[nodiscard]] auto createFSub(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getFSub(Expr expr) const -> const FSub&;


			///////////////////////////////////
			// mul

			[[nodiscard]] auto createMul(
				Expr lhs, Expr rhs, bool nsw, bool nuw, std::string&& name = ""
			) const -> Expr;
			[[nodiscard]] auto getMul(Expr expr) const -> const Mul&;

			[[nodiscard]] auto createSMulWrap(
				Expr lhs, Expr rhs, std::string&& result_name = "", std::string&& wrapped_name = ""
			) -> Expr;
			[[nodiscard]] auto getSMulWrap(Expr expr) const -> const SMulWrap&;
			[[nodiscard]] static auto extractSMulWrapResult(Expr expr) -> Expr;
			[[nodiscard]] static auto extractSMulWrapWrapped(Expr expr) -> Expr;

			[[nodiscard]] auto createUMulWrap(
				Expr lhs, Expr rhs, std::string&& result_name = "", std::string&& wrapped_name = ""
			) -> Expr;
			[[nodiscard]] auto getUMulWrap(Expr expr) const -> const UMulWrap&;
			[[nodiscard]] static auto extractUMulWrapResult(Expr expr) -> Expr;
			[[nodiscard]] static auto extractUMulWrapWrapped(Expr expr) -> Expr;

			[[nodiscard]] auto createSMulSat(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getSMulSat(Expr expr) const -> const SMulSat&;

			[[nodiscard]] auto createUMulSat(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getUMulSat(Expr expr) const -> const UMulSat&;

			[[nodiscard]] auto createFMul(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getFMul(Expr expr) const -> const FMul&;


			///////////////////////////////////
			// div

			[[nodiscard]] auto createSDiv(
				Expr lhs, Expr rhs, bool is_exact, std::string&& name = ""
			) const -> Expr;
			[[nodiscard]] auto getSDiv(Expr expr) const -> const SDiv&;

			[[nodiscard]] auto createUDiv(
				Expr lhs, Expr rhs, bool is_exact, std::string&& name = ""
			) const -> Expr;
			[[nodiscard]] auto getUDiv(Expr expr) const -> const UDiv&;

			[[nodiscard]] auto createFDiv(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getFDiv(Expr expr) const -> const FDiv&;


			///////////////////////////////////
			// rem

			[[nodiscard]] auto createSRem(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getSRem(Expr expr) const -> const SRem&;

			[[nodiscard]] auto createURem(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getURem(Expr expr) const -> const URem&;

			[[nodiscard]] auto createFRem(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getFRem(Expr expr) const -> const FRem&;


			///////////////////////////////////
			// neg

			[[nodiscard]] auto createFNeg(Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getFNeg(Expr expr) const -> const FNeg&;


			//////////////////////////////////////////////////////////////////////
			// comparison

			///////////////////////////////////
			// equal

			[[nodiscard]] auto createIEq(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getIEq(Expr expr) const -> const IEq&;

			[[nodiscard]] auto createFEq(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getFEq(Expr expr) const -> const FEq&;


			///////////////////////////////////
			// not equal

			[[nodiscard]] auto createINeq(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getINeq(Expr expr) const -> const INeq&;

			[[nodiscard]] auto createFNeq(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getFNeq(Expr expr) const -> const FNeq&;


			///////////////////////////////////
			// less than

			[[nodiscard]] auto createSLT(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getSLT(Expr expr) const -> const SLT&;

			[[nodiscard]] auto createULT(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getULT(Expr expr) const -> const ULT&;

			[[nodiscard]] auto createFLT(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getFLT(Expr expr) const -> const FLT&;


			///////////////////////////////////
			// less than or equal to

			[[nodiscard]] auto createSLTE(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getSLTE(Expr expr) const -> const SLTE&;

			[[nodiscard]] auto createULTE(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getULTE(Expr expr) const -> const ULTE&;

			[[nodiscard]] auto createFLTE(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getFLTE(Expr expr) const -> const FLTE&;


			///////////////////////////////////
			// greater than

			[[nodiscard]] auto createSGT(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getSGT(Expr expr) const -> const SGT&;

			[[nodiscard]] auto createUGT(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getUGT(Expr expr) const -> const UGT&;

			[[nodiscard]] auto createFGT(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getFGT(Expr expr) const -> const FGT&;


			///////////////////////////////////
			// greater than or equal to

			[[nodiscard]] auto createSGTE(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getSGTE(Expr expr) const -> const SGTE&;

			[[nodiscard]] auto createUGTE(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getUGTE(Expr expr) const -> const UGTE&;

			[[nodiscard]] auto createFGTE(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getFGTE(Expr expr) const -> const FGTE&;


			//////////////////////////////////////////////////////////////////////
			// bitwise

			[[nodiscard]] auto createAnd(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getAnd(Expr expr) const -> const And&;

			[[nodiscard]] auto createOr(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getOr(Expr expr) const -> const Or&;

			[[nodiscard]] auto createXor(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getXor(Expr expr) const -> const Xor&;

			[[nodiscard]] auto createSHL(Expr lhs, Expr rhs, bool nsw, bool nuw, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getSHL(Expr expr) const -> const SHL&;

			[[nodiscard]] auto createSSHLSat(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getSSHLSat(Expr expr) const -> const SSHLSat&;

			[[nodiscard]] auto createUSHLSat(Expr lhs, Expr rhs, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getUSHLSat(Expr expr) const -> const USHLSat&;

			[[nodiscard]] auto createSSHR(Expr lhs, Expr rhs, bool is_exact, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getSSHR(Expr expr) const -> const SSHR&;

			[[nodiscard]] auto createUSHR(Expr lhs, Expr rhs, bool is_exact, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getUSHR(Expr expr) const -> const USHR&;



			//////////////////////////////////////////////////////////////////////
			// bit operations
			
			[[nodiscard]] auto createBitReverse(Expr expr, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getBitReverse(Expr expr) const -> const BitReverse&;

			[[nodiscard]] auto createByteSwap(Expr expr, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getByteSwap(Expr expr) const -> const ByteSwap&;

			[[nodiscard]] auto createCtPop(Expr expr, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getCtPop(Expr expr) const -> const CtPop&;

			[[nodiscard]] auto createCTLZ(Expr expr, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getCTLZ(Expr expr) const -> const CTLZ&;

			[[nodiscard]] auto createCTTZ(Expr expr, std::string&& name = "") const -> Expr;
			[[nodiscard]] auto getCTTZ(Expr expr) const -> const CTTZ&;


			//////////////////////////////////////////////////////////////////////
			// atomics

			[[nodiscard]] auto createCmpXchg(
				Expr target,
				Expr expected,
				Expr desired,
				std::string&& loaded_name = "",
				std::string&& succeeded_name = "",
				bool isWeak = false,
				AtomicOrdering success_ordering = AtomicOrdering::SEQUENTIALLY_CONSISTENT,
				AtomicOrdering failure_ordering = AtomicOrdering::SEQUENTIALLY_CONSISTENT
			) const -> Expr;
			[[nodiscard]] auto getCmpXchg(Expr expr) const -> const CmpXchg&;

			[[nodiscard]] static auto extractCmpXchgLoaded(Expr) -> Expr;
			[[nodiscard]] static auto extractCmpXchgSucceeded(Expr) -> Expr;



			[[nodiscard]] auto createAtomicRMW(
				AtomicRMW::Op op,
				Expr target,
				Expr value,
				std::string&& name = "",
				AtomicOrdering ordering = AtomicOrdering::SEQUENTIALLY_CONSISTENT
			) const -> Expr;
			[[nodiscard]] auto getAtomicRMW(Expr expr) const -> const AtomicRMW&;


			//////////////////////////////////////////////////////////////////////
			// meta

			auto createMetaLocalVar(std::string&& name, Expr value, meta::Type type) const -> Expr;
			[[nodiscard]] auto getMetaLocalVar(Expr expr) const -> const MetaLocalVar&;

			auto createMetaParam(std::string&& name, Expr value, uint32_t param_index, meta::Type type) const -> Expr;
			[[nodiscard]] auto getMetaParam(Expr expr) const -> const MetaParam&;


		private:
			auto insert_stmt(Expr stmt) const -> void;
			auto delete_expr(Expr expr) const -> void;

			[[nodiscard]] auto name_exists_in_func(std::string_view) const -> bool;
			[[nodiscard]] auto get_stmt_name(std::string&& name) const -> std::string;
			[[nodiscard]] auto get_stmt_name_with_forward_include(
				std::string&& name, evo::ArrayProxy<std::string_view> forward_includes
			) const -> std::string;

			template<bool REPLACE_WITH_VALUE>
			[[nodiscard]] auto replace_stmt_impl(Expr original, Expr replacement) const -> void;

			[[nodiscard]] auto get_current_source_location() const -> std::optional<meta::SourceLocation>;

	
		private:
			Module& module;
			Function* target_func;
			BasicBlock* target_basic_block;

			std::stack<
				std::optional<meta::SourceLocation>, evo::SmallVector<std::optional<meta::SourceLocation>, 16>
			> source_locations{};

			// `insert_index` is mutable to allow for it to change when inserting / deleting stmts
			mutable size_t insert_index = std::numeric_limits<size_t>::max();


	};


}


