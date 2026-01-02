////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/Agent.h"

#include "../include/misc.h"
#include "../include/ReaderAgent.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

namespace pcit::pir{

	//////////////////////////////////////////////////////////////////////
	// targets

	auto Agent::setTargetFunction(Function::ID id) -> void {
		this->target_func = &this->module.getFunction(id);
		this->target_basic_block = nullptr;
		this->insert_index = std::numeric_limits<size_t>::max();
	}

	auto Agent::setTargetFunction(Function& func) -> void {
		this->target_func = &func;
		this->target_basic_block = nullptr;
		this->insert_index = std::numeric_limits<size_t>::max();
	}

	auto Agent::removeTargetFunction() -> void {
		this->target_func = nullptr;
		this->target_basic_block = nullptr;
		this->insert_index = std::numeric_limits<size_t>::max();
	}

	auto Agent::getTargetFunction() const -> Function& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		return *this->target_func;
	}


	auto Agent::setTargetBasicBlock(BasicBlock::ID id) -> void {
		evo::debugAssert(this->hasTargetFunction(), "No target function is set");
		// TODO(FUTURE): check that block is in function

		this->target_basic_block = &this->getBasicBlock(id);
		this->insert_index = std::numeric_limits<size_t>::max();
	}

	auto Agent::setTargetBasicBlock(BasicBlock& basic_block) -> void {
		evo::debugAssert(this->hasTargetFunction(), "No target function is set");
		// TODO(FUTURE): check that block is in function

		this->target_basic_block = &basic_block;
		this->insert_index = std::numeric_limits<size_t>::max();
	}

	auto Agent::setTargetBasicBlockAtEnd() -> void {
		evo::debugAssert(this->hasTargetFunction(), "No target function is set");

		this->setTargetBasicBlock(this->target_func->basic_blocks.back());
	}


	auto Agent::removeTargetBasicBlock() -> void {
		this->target_basic_block = nullptr;
		this->insert_index = std::numeric_limits<size_t>::max();
	}

	auto Agent::getTargetBasicBlock() const -> BasicBlock& {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		return *this->target_basic_block;
	}


	auto Agent::setInsertIndex(size_t index) -> void {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		// TODO(FUTURE): check that index is in block

		this->insert_index = index;
	}

	auto Agent::setInsertIndexAtEnd() -> void {
		this->insert_index = std::numeric_limits<size_t>::max();
	}


	//////////////////////////////////////////////////////////////////////
	// misc expr stuff

	auto Agent::getExprType(const Expr& expr) const -> Type {
		if(this->hasTargetFunction()){
			return ReaderAgent(this->module, this->getTargetFunction()).getExprType(expr);
		}else{
			return ReaderAgent(this->module).getExprType(expr);
		}
	}



	auto Agent::replaceExpr(Expr original, Expr replacement) const -> void {
		evo::debugAssert(this->hasTargetFunction(), "No target function is set");
		evo::debugAssert(
			!original.isMultiValueStmt(), "Cannot replace multi-value statement (extract values and manually remove)"
		);
		evo::debugAssert(
			!replacement.isMultiValueStmt(), "Replacement cannot be multi-value statement (extract values)"
		);
		evo::debugAssert(
			this->getExprType(original) == this->getExprType(replacement),
			"type of original and replacement do not match"
		);

		struct OriginalLocation{
			BasicBlock::ID basic_block_id;
			size_t index;
		};
		auto original_location = std::optional<OriginalLocation>();

		for(const BasicBlock::ID& basic_block_id : *this->target_func){
			BasicBlock& basic_block = this->getBasicBlock(basic_block_id);
			for(size_t i = 0; Expr& stmt : basic_block){
				EVO_DEFER([&](){ i += 1; });

				if(stmt == original){
					original_location.emplace(basic_block_id, i);
					continue;
				}

				switch(stmt.kind()){
					case Expr::Kind::NONE: evo::debugFatalBreak("Invalid stmt");
					
					case Expr::Kind::GLOBAL_VALUE:     continue;
					case Expr::Kind::FUNCTION_POINTER: continue;
					case Expr::Kind::NUMBER:           continue;
					case Expr::Kind::BOOLEAN:          continue;
					case Expr::Kind::NULLPTR:          continue;
					case Expr::Kind::PARAM_EXPR:       continue;
					
					case Expr::Kind::CALL: {
						Call& call_inst = this->module.calls[stmt.index];

						if(call_inst.target.is<PtrCall>() && call_inst.target.as<PtrCall>().location == original){
							call_inst.target.as<PtrCall>().location = replacement;
						}

						for(Expr& arg : call_inst.args){
							if(arg == original){ arg = replacement; }
						}
					} break;
					
					case Expr::Kind::CALL_VOID: {
						CallVoid& call_void_inst = this->module.call_voids[stmt.index];

						if(
							call_void_inst.target.is<PtrCall>() &&
							call_void_inst.target.as<PtrCall>().location == original
						){
							call_void_inst.target.as<PtrCall>().location = replacement;
						}

						for(Expr& arg : call_void_inst.args){
							if(arg == original){ arg = replacement; }
						}
					} break;

					case Expr::Kind::ABORT: continue;
					case Expr::Kind::BREAKPOINT: continue;
	
					case Expr::Kind::RET: {
						Ret& ret_inst = this->module.rets[stmt.index];

						if(ret_inst.value.has_value() && *ret_inst.value == original){
							ret_inst.value.emplace(replacement);
						}
					} break;
					
					case Expr::Kind::JUMP: continue;

					case Expr::Kind::BRANCH: {
						Branch& branch = this->module.branches[stmt.index];

						if(branch.cond == original){ branch.cond = replacement; }
					} break;

					case Expr::Kind::UNREACHABLE: continue;

					case Expr::Kind::PHI: {
						Phi& phi = this->module.phis[stmt.index];

						for(Phi::Predecessor& predecessor : phi.predecessors){
							if(predecessor.value == original){ predecessor.value = replacement; }
						}
					} break;

					case Expr::Kind::SWITCH: {
						Switch& switch_stmt = this->module.switches[stmt.index];

						if(switch_stmt.cond == original){ switch_stmt.cond = replacement; }

						for(Switch::Case& switch_case : switch_stmt.cases){
							if(switch_case.value == original){ switch_case.value = replacement; }
						}
					} break;

					case Expr::Kind::ALLOCA: continue;

					case Expr::Kind::LOAD: {
						Load& load = this->module.loads[stmt.index];

						if(load.source == original){ load.source = replacement; }
					} break;

					case Expr::Kind::STORE: {
						Store& store = this->module.stores[stmt.index];

						if(store.destination == original){ store.destination = replacement; }
						if(store.value == original){ store.value = replacement; }
					} break;

					case Expr::Kind::CALC_PTR: {
						CalcPtr& calc_ptr = this->module.calc_ptrs[stmt.index];

						if(calc_ptr.basePtr == original){
							calc_ptr.basePtr = replacement;
						}else{
							for(CalcPtr::Index& index : calc_ptr.indices){
								if(index.is<Expr>() && index.as<Expr>() == original){ index.as<Expr>() = replacement; }
							}
						}
					} break;

					case Expr::Kind::MEMCPY: {
						Memcpy& memcpy = this->module.memcpys[stmt.index];

						if(memcpy.dst == original){ memcpy.dst = replacement; }
						if(memcpy.src == original){ memcpy.src = replacement; }
						if(memcpy.numBytes == original){ memcpy.numBytes = replacement; }
					} break;

					case Expr::Kind::MEMSET: {
						Memset& memset = this->module.memsets[stmt.index];

						if(memset.dst == original){ memset.dst = replacement; }
						if(memset.value == original){ memset.value = replacement; }
						if(memset.numBytes == original){ memset.numBytes = replacement; }
					} break;

					case Expr::Kind::BIT_CAST: {
						BitCast& bitcast = this->module.bitcasts[stmt.index];
						if(bitcast.fromValue == original){ bitcast.fromValue = replacement; }
					} break;

					case Expr::Kind::TRUNC: {
						Trunc& trunc = this->module.truncs[stmt.index];
						if(trunc.fromValue == original){ trunc.fromValue = replacement; }
					} break;

					case Expr::Kind::FTRUNC: {
						FTrunc& ftrunc = this->module.ftruncs[stmt.index];
						if(ftrunc.fromValue == original){ ftrunc.fromValue = replacement; }
					} break;

					case Expr::Kind::SEXT: {
						SExt& sext = this->module.sexts[stmt.index];
						if(sext.fromValue == original){ sext.fromValue = replacement; }
					} break;

					case Expr::Kind::ZEXT: {
						ZExt& zext = this->module.zexts[stmt.index];
						if(zext.fromValue == original){ zext.fromValue = replacement; }
					} break;

					case Expr::Kind::FEXT: {
						FExt& fext = this->module.fexts[stmt.index];
						if(fext.fromValue == original){ fext.fromValue = replacement; }
					} break;

					case Expr::Kind::ITOF: {
						IToF& itof = this->module.itofs[stmt.index];
						if(itof.fromValue == original){ itof.fromValue = replacement; }
					} break;

					case Expr::Kind::UITOF: {
						UIToF& uitof = this->module.uitofs[stmt.index];
						if(uitof.fromValue == original){ uitof.fromValue = replacement; }
					} break;

					case Expr::Kind::FTOI: {
						FToI& ftoi = this->module.ftois[stmt.index];
						if(ftoi.fromValue == original){ ftoi.fromValue = replacement; }
					} break;

					case Expr::Kind::FTOUI: {
						FToUI& ftoui = this->module.ftouis[stmt.index];
						if(ftoui.fromValue == original){ ftoui.fromValue = replacement; }
					} break;

					
					case Expr::Kind::ADD: {
						Add& add = this->module.adds[stmt.index];

						if(add.lhs == original){ add.lhs = replacement; }
						if(add.rhs == original){ add.rhs = replacement; }
					} break;

					case Expr::Kind::SADD_WRAP: {
						SAddWrap& sadd_wrap = this->module.sadd_wraps[stmt.index];

						if(sadd_wrap.lhs == original){ sadd_wrap.lhs = replacement; }
						if(sadd_wrap.rhs == original){ sadd_wrap.rhs = replacement; }
					} break;

					case Expr::Kind::SADD_WRAP_RESULT: continue;
					case Expr::Kind::SADD_WRAP_WRAPPED: continue;

					case Expr::Kind::UADD_WRAP: {
						UAddWrap& uadd_wrap = this->module.uadd_wraps[stmt.index];

						if(uadd_wrap.lhs == original){ uadd_wrap.lhs = replacement; }
						if(uadd_wrap.rhs == original){ uadd_wrap.rhs = replacement; }
					} break;

					case Expr::Kind::UADD_WRAP_RESULT: continue;
					case Expr::Kind::UADD_WRAP_WRAPPED: continue;

					case Expr::Kind::SADD_SAT: {
						SAddSat& sadd_wrap = this->module.sadd_sats[stmt.index];

						if(sadd_wrap.lhs == original){ sadd_wrap.lhs = replacement; }
						if(sadd_wrap.rhs == original){ sadd_wrap.rhs = replacement; }
					} break;

					case Expr::Kind::UADD_SAT: {
						UAddSat& uadd_wrap = this->module.uadd_sats[stmt.index];

						if(uadd_wrap.lhs == original){ uadd_wrap.lhs = replacement; }
						if(uadd_wrap.rhs == original){ uadd_wrap.rhs = replacement; }
					} break;

					case Expr::Kind::FADD: {
						FAdd& fadd = this->module.fadds[stmt.index];

						if(fadd.lhs == original){ fadd.lhs = replacement; }
						if(fadd.rhs == original){ fadd.rhs = replacement; }
					} break;


					case Expr::Kind::SUB: {
						Sub& sub = this->module.subs[stmt.index];

						if(sub.lhs == original){ sub.lhs = replacement; }
						if(sub.rhs == original){ sub.rhs = replacement; }
					} break;

					case Expr::Kind::SSUB_WRAP: {
						SSubWrap& ssub_wrap = this->module.ssub_wraps[stmt.index];

						if(ssub_wrap.lhs == original){ ssub_wrap.lhs = replacement; }
						if(ssub_wrap.rhs == original){ ssub_wrap.rhs = replacement; }
					} break;

					case Expr::Kind::SSUB_WRAP_RESULT: continue;
					case Expr::Kind::SSUB_WRAP_WRAPPED: continue;

					case Expr::Kind::USUB_WRAP: {
						USubWrap& usub_wrap = this->module.usub_wraps[stmt.index];

						if(usub_wrap.lhs == original){ usub_wrap.lhs = replacement; }
						if(usub_wrap.rhs == original){ usub_wrap.rhs = replacement; }
					} break;

					case Expr::Kind::USUB_WRAP_RESULT: continue;
					case Expr::Kind::USUB_WRAP_WRAPPED: continue;

					case Expr::Kind::SSUB_SAT: {
						SSubSat& ssub_sat = this->module.ssub_sats[stmt.index];

						if(ssub_sat.lhs == original){ ssub_sat.lhs = replacement; }
						if(ssub_sat.rhs == original){ ssub_sat.rhs = replacement; }
					} break;

					case Expr::Kind::USUB_SAT: {
						USubSat& usub_sat = this->module.usub_sats[stmt.index];

						if(usub_sat.lhs == original){ usub_sat.lhs = replacement; }
						if(usub_sat.rhs == original){ usub_sat.rhs = replacement; }
					} break;

					case Expr::Kind::FSUB: {
						FSub& fsub = this->module.fsubs[stmt.index];

						if(fsub.lhs == original){ fsub.lhs = replacement; }
						if(fsub.rhs == original){ fsub.rhs = replacement; }
					} break;


					case Expr::Kind::MUL: {
						Mul& mul = this->module.muls[stmt.index];

						if(mul.lhs == original){ mul.lhs = replacement; }
						if(mul.rhs == original){ mul.rhs = replacement; }
					} break;

					case Expr::Kind::SMUL_WRAP: {
						SMulWrap& smul_wrap = this->module.smul_wraps[stmt.index];

						if(smul_wrap.lhs == original){ smul_wrap.lhs = replacement; }
						if(smul_wrap.rhs == original){ smul_wrap.rhs = replacement; }
					} break;

					case Expr::Kind::SMUL_WRAP_RESULT: continue;
					case Expr::Kind::SMUL_WRAP_WRAPPED: continue;

					case Expr::Kind::UMUL_WRAP: {
						UMulWrap& umul_wrap = this->module.umul_wraps[stmt.index];

						if(umul_wrap.lhs == original){ umul_wrap.lhs = replacement; }
						if(umul_wrap.rhs == original){ umul_wrap.rhs = replacement; }
					} break;

					case Expr::Kind::UMUL_WRAP_RESULT: continue;
					case Expr::Kind::UMUL_WRAP_WRAPPED: continue;

					case Expr::Kind::SMUL_SAT: {
						SMulSat& smul_sat = this->module.smul_sats[stmt.index];

						if(smul_sat.lhs == original){ smul_sat.lhs = replacement; }
						if(smul_sat.rhs == original){ smul_sat.rhs = replacement; }
					} break;

					case Expr::Kind::UMUL_SAT: {
						UMulSat& umul_sat = this->module.umul_sats[stmt.index];

						if(umul_sat.lhs == original){ umul_sat.lhs = replacement; }
						if(umul_sat.rhs == original){ umul_sat.rhs = replacement; }
					} break;

					case Expr::Kind::FMUL: {
						FMul& fmul = this->module.fmuls[stmt.index];

						if(fmul.lhs == original){ fmul.lhs = replacement; }
						if(fmul.rhs == original){ fmul.rhs = replacement; }
					} break;

					case Expr::Kind::SDIV: {
						SDiv& sdiv = this->module.sdivs[stmt.index];

						if(sdiv.lhs == original){ sdiv.lhs = replacement; }
						if(sdiv.rhs == original){ sdiv.rhs = replacement; }
					} break;

					case Expr::Kind::UDIV: {
						UDiv& udiv = this->module.udivs[stmt.index];

						if(udiv.lhs == original){ udiv.lhs = replacement; }
						if(udiv.rhs == original){ udiv.rhs = replacement; }
					} break;

					case Expr::Kind::FDIV: {
						FDiv& fdiv = this->module.fdivs[stmt.index];

						if(fdiv.lhs == original){ fdiv.lhs = replacement; }
						if(fdiv.rhs == original){ fdiv.rhs = replacement; }
					} break;

					case Expr::Kind::SREM: {
						SRem& srem = this->module.srems[stmt.index];

						if(srem.lhs == original){ srem.lhs = replacement; }
						if(srem.rhs == original){ srem.rhs = replacement; }
					} break;

					case Expr::Kind::UREM: {
						URem& urem = this->module.urems[stmt.index];

						if(urem.lhs == original){ urem.lhs = replacement; }
						if(urem.rhs == original){ urem.rhs = replacement; }
					} break;

					case Expr::Kind::FREM: {
						FRem& frem = this->module.frems[stmt.index];

						if(frem.lhs == original){ frem.lhs = replacement; }
						if(frem.rhs == original){ frem.rhs = replacement; }
					} break;

					case Expr::Kind::FNEG: {
						FNeg& fneg = this->module.fnegs[stmt.index];

						if(fneg.rhs == original){ fneg.rhs = replacement; }
					} break;



					case Expr::Kind::IEQ: {
						IEq& ieq = this->module.ieqs[stmt.index];

						if(ieq.lhs == original){ ieq.lhs = replacement; }
						if(ieq.rhs == original){ ieq.rhs = replacement; }
					} break;

					case Expr::Kind::FEQ: {
						FEq& feq = this->module.feqs[stmt.index];

						if(feq.lhs == original){ feq.lhs = replacement; }
						if(feq.rhs == original){ feq.rhs = replacement; }
					} break;
					
					case Expr::Kind::INEQ: {
						INeq& ineq = this->module.ineqs[stmt.index];

						if(ineq.lhs == original){ ineq.lhs = replacement; }
						if(ineq.rhs == original){ ineq.rhs = replacement; }
					} break;

					case Expr::Kind::FNEQ: {
						FNeq& fneq = this->module.fneqs[stmt.index];

						if(fneq.lhs == original){ fneq.lhs = replacement; }
						if(fneq.rhs == original){ fneq.rhs = replacement; }
					} break;
					
					case Expr::Kind::SLT: {
						SLT& slt = this->module.slts[stmt.index];

						if(slt.lhs == original){ slt.lhs = replacement; }
						if(slt.rhs == original){ slt.rhs = replacement; }
					} break;
					
					case Expr::Kind::ULT: {
						ULT& ult = this->module.ults[stmt.index];

						if(ult.lhs == original){ ult.lhs = replacement; }
						if(ult.rhs == original){ ult.rhs = replacement; }
					} break;

					case Expr::Kind::FLT: {
						FLT& flt = this->module.flts[stmt.index];

						if(flt.lhs == original){ flt.lhs = replacement; }
						if(flt.rhs == original){ flt.rhs = replacement; }
					} break;
					case Expr::Kind::SLTE: {
						SLTE& slte = this->module.sltes[stmt.index];

						if(slte.lhs == original){ slte.lhs = replacement; }
						if(slte.rhs == original){ slte.rhs = replacement; }
					} break;
					
					case Expr::Kind::ULTE: {
						ULTE& ulte = this->module.ultes[stmt.index];

						if(ulte.lhs == original){ ulte.lhs = replacement; }
						if(ulte.rhs == original){ ulte.rhs = replacement; }
					} break;

					case Expr::Kind::FLTE: {
						FLTE& flte = this->module.fltes[stmt.index];

						if(flte.lhs == original){ flte.lhs = replacement; }
						if(flte.rhs == original){ flte.rhs = replacement; }
					} break;
					case Expr::Kind::SGT: {
						SGT& sgt = this->module.sgts[stmt.index];

						if(sgt.lhs == original){ sgt.lhs = replacement; }
						if(sgt.rhs == original){ sgt.rhs = replacement; }
					} break;
					
					case Expr::Kind::UGT: {
						UGT& ugt = this->module.ugts[stmt.index];

						if(ugt.lhs == original){ ugt.lhs = replacement; }
						if(ugt.rhs == original){ ugt.rhs = replacement; }
					} break;

					case Expr::Kind::FGT: {
						FGT& fgt = this->module.fgts[stmt.index];

						if(fgt.lhs == original){ fgt.lhs = replacement; }
						if(fgt.rhs == original){ fgt.rhs = replacement; }
					} break;
					case Expr::Kind::SGTE: {
						SGTE& sgte = this->module.sgtes[stmt.index];

						if(sgte.lhs == original){ sgte.lhs = replacement; }
						if(sgte.rhs == original){ sgte.rhs = replacement; }
					} break;
					
					case Expr::Kind::UGTE: {
						UGTE& ugte = this->module.ugtes[stmt.index];

						if(ugte.lhs == original){ ugte.lhs = replacement; }
						if(ugte.rhs == original){ ugte.rhs = replacement; }
					} break;

					case Expr::Kind::FGTE: {
						FGTE& fgte = this->module.fgtes[stmt.index];

						if(fgte.lhs == original){ fgte.lhs = replacement; }
						if(fgte.rhs == original){ fgte.rhs = replacement; }
					} break;

					case Expr::Kind::AND: {
						And& and_stmt = this->module.ands[stmt.index];

						if(and_stmt.lhs == original){ and_stmt.lhs = replacement; }
						if(and_stmt.rhs == original){ and_stmt.rhs = replacement; }
					} break;

					case Expr::Kind::OR: {
						Or& or_stmt = this->module.ors[stmt.index];

						if(or_stmt.lhs == original){ or_stmt.lhs = replacement; }
						if(or_stmt.rhs == original){ or_stmt.rhs = replacement; }
					} break;

					case Expr::Kind::XOR: {
						Xor& xor_stmt = this->module.xors[stmt.index];

						if(xor_stmt.lhs == original){ xor_stmt.lhs = replacement; }
						if(xor_stmt.rhs == original){ xor_stmt.rhs = replacement; }
					} break;

					case Expr::Kind::SHL: {
						SHL& shl = this->module.shls[stmt.index];

						if(shl.lhs == original){ shl.lhs = replacement; }
						if(shl.rhs == original){ shl.rhs = replacement; }
					} break;

					case Expr::Kind::SSHL_SAT: {
						SSHLSat& sshlsat = this->module.sshlsats[stmt.index];

						if(sshlsat.lhs == original){ sshlsat.lhs = replacement; }
						if(sshlsat.rhs == original){ sshlsat.rhs = replacement; }
					} break;

					case Expr::Kind::USHL_SAT: {
						USHLSat& ushlsat = this->module.ushlsats[stmt.index];

						if(ushlsat.lhs == original){ ushlsat.lhs = replacement; }
						if(ushlsat.rhs == original){ ushlsat.rhs = replacement; }
					} break;

					case Expr::Kind::SSHR: {
						SSHR& sshr = this->module.sshrs[stmt.index];

						if(sshr.lhs == original){ sshr.lhs = replacement; }
						if(sshr.rhs == original){ sshr.rhs = replacement; }
					} break;

					case Expr::Kind::USHR: {
						USHR& ushr = this->module.ushrs[stmt.index];

						if(ushr.lhs == original){ ushr.lhs = replacement; }
						if(ushr.rhs == original){ ushr.rhs = replacement; }
					} break;

					case Expr::Kind::BIT_REVERSE: {
						BitReverse& bit_reverse = this->module.bit_reverses[stmt.index];

						if(bit_reverse.arg == original){ bit_reverse.arg = replacement; }
					} break;

					case Expr::Kind::BSWAP: {
						BSwap& bswap = this->module.bswaps[stmt.index];

						if(bswap.arg == original){ bswap.arg = replacement; }
					} break;

					case Expr::Kind::CTPOP: {
						CtPop& ctpop = this->module.ctpops[stmt.index];

						if(ctpop.arg == original){ ctpop.arg = replacement; }
					} break;

					case Expr::Kind::CTLZ: {
						CTLZ& ctlz = this->module.ctlzs[stmt.index];

						if(ctlz.arg == original){ ctlz.arg = replacement; }
					} break;

					case Expr::Kind::CTTZ: {
						CTTZ& cttz = this->module.cttzs[stmt.index];

						if(cttz.arg == original){ cttz.arg = replacement; }
					} break;

					case Expr::Kind::LIFETIME_START: {
						LifetimeStart& lifetime_start = this->module.lifetime_starts[stmt.index];

						if(lifetime_start.arg == original){ lifetime_start.arg = replacement; }
					} break;

					case Expr::Kind::LIFETIME_END: {
						LifetimeEnd& lifetime_end = this->module.lifetime_ends[stmt.index];

						if(lifetime_end.arg == original){ lifetime_end.arg = replacement; }
					} break;
				}
			}
		}

		if(original_location.has_value()){
			BasicBlock& basic_block = this->getBasicBlock(original_location->basic_block_id);
			basic_block.remove(original_location->index);
		}

		this->delete_expr(original);
	}



	auto Agent::removeStmt(Expr stmt_to_remove) const -> void {
		evo::debugAssert(this->hasTargetFunction(), "No target function is set");
		evo::debugAssert(stmt_to_remove.isStmt(), "not a statement");

		if(stmt_to_remove.kind() != Expr::Kind::ALLOCA){
			for(const BasicBlock::ID& basic_block_id : *this->target_func){
				BasicBlock& basic_block = this->getBasicBlock(basic_block_id);
	
				for(size_t i = 0; const Expr& stmt : basic_block){
					if(stmt == stmt_to_remove){
						basic_block.remove(i);
						this->delete_expr(stmt_to_remove);
						return;
					}

					i += 1;
				}

			}
		}

		this->delete_expr(stmt_to_remove);
	}

	
	//////////////////////////////////////////////////////////////////////
	// basic blocks

	auto Agent::createBasicBlock(Function::ID func, std::string&& name) const -> BasicBlock::ID {
		return this->createBasicBlock(this->module.getFunction(func), std::move(name));
	}

	auto Agent::createBasicBlock(Function& func, std::string&& name) const -> BasicBlock::ID {
		const pcit::pir::BasicBlock::ID new_block_id =
			this->module.basic_blocks.emplace_back(this->get_stmt_name(std::move(name)));
		func.append_basic_block(new_block_id);

		this->getBasicBlock(new_block_id).id = new_block_id;

		return new_block_id;
	}

	auto Agent::createBasicBlock(std::string&& name) const -> BasicBlock::ID {
		evo::debugAssert(this->hasTargetFunction(), "Cannot use this function as there is no target function set");
		return this->createBasicBlock(*this->target_func, std::move(name));
	}

	auto Agent::createBasicBlockInline(std::string&& name) const -> BasicBlock::ID {
		evo::debugAssert(this->hasTargetBasicBlock(), "Cannot use this function as there is no target basic block set");

		const pcit::pir::BasicBlock::ID new_block_id =
			this->module.basic_blocks.emplace_back(this->get_stmt_name(std::move(name)));
		this->target_func->insert_basic_block_after(new_block_id, this->getTargetBasicBlock());

		this->getBasicBlock(new_block_id).id = new_block_id;

		return new_block_id;
	}

	auto Agent::getBasicBlock(BasicBlock::ID id) const -> BasicBlock& {
		return this->module.basic_blocks[id];
	}


	//////////////////////////////////////////////////////////////////////
	// numbers

	auto Agent::createNumber(const Type& type, core::GenericInt&& value) const -> Expr {
		evo::debugAssert(type.isNumeric(), "Number type must be numeric");
		evo::debugAssert(type.kind() == Type::Kind::INTEGER, "Type and value must both be integer or both be floating");

		return Expr(Expr::Kind::NUMBER, this->module.numbers.emplace_back(type, std::move(value)));
	}

	auto Agent::createNumber(const Type& type, const core::GenericInt& value) const -> Expr {
		evo::debugAssert(type.isNumeric(), "Number type must be numeric");
		evo::debugAssert(type.kind() == Type::Kind::INTEGER, "Type and value must both be integer or both be floating");

		return Expr(Expr::Kind::NUMBER, this->module.numbers.emplace_back(type, value));
	}

	auto Agent::createNumber(const Type& type, core::GenericFloat&& value) const -> Expr {
		evo::debugAssert(type.isNumeric(), "Number type must be numeric");
		evo::debugAssert(type.isFloat(), "Type and value must both be integer or both be floating");

		return Expr(Expr::Kind::NUMBER, this->module.numbers.emplace_back(type, std::move(value)));
	}

	auto Agent::createNumber(const Type& type, const core::GenericFloat& value) const -> Expr {
		evo::debugAssert(type.isNumeric(), "Number type must be numeric");
		evo::debugAssert(type.isFloat(), "Type and value must both be integer or both be floating");

		return Expr(Expr::Kind::NUMBER, this->module.numbers.emplace_back(type, value));
	}

	auto Agent::getNumber(const Expr& expr) const -> const Number& {
		return ReaderAgent(this->module).getNumber(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// booleans

	auto Agent::createBoolean(bool value) -> Expr {
		return Expr(Expr::Kind::BOOLEAN, uint32_t(value));
	}

	auto Agent::getBoolean(const Expr& expr) -> bool {
		return ReaderAgent::getBoolean(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// nullptr

	auto Agent::createNullptr() -> Expr {
		return Expr(Expr::Kind::NULLPTR, 0);
	}



	//////////////////////////////////////////////////////////////////////
	// param exprs

	auto Agent::createParamExpr(uint32_t index) -> Expr {
		return Expr(Expr::Kind::PARAM_EXPR, index);
	}

	auto Agent::getParamExpr(const Expr& expr) -> ParamExpr {
		return ReaderAgent::getParamExpr(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// global values (expr)

	auto Agent::createGlobalValue(const GlobalVar::ID& global_id) -> Expr {
		return Expr(Expr::Kind::GLOBAL_VALUE, global_id.get());
	}

	auto Agent::getGlobalValue(const Expr& expr) const -> const GlobalVar& {
		return ReaderAgent(this->module).getGlobalValue(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// global values (expr)

	auto Agent::createFunctionPointer(const Function::ID& func_id) -> Expr {
		return Expr(Expr::Kind::FUNCTION_POINTER, func_id.get());
	}

	auto Agent::getFunctionPointer(const Expr& expr) const -> const Function& {
		return ReaderAgent(this->module).getFunctionPointer(expr);
	}



	//////////////////////////////////////////////////////////////////////
	// calls

	auto Agent::createCall(Function::ID func, evo::SmallVector<Expr>&& args, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunction(func).getReturnType().kind() != Type::Kind::VOID,
			"Call cannot return `Void` (did you mean CallVoid?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::CALL,
			this->module.calls.emplace_back(this->get_stmt_name(std::move(name)), func, std::move(args))
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::createCall(Function::ID func, const evo::SmallVector<Expr>& args, std::string&& name) const
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunction(func).getReturnType().kind() != Type::Kind::VOID,
			"Call cannot return `Void` (did you mean CallVoid?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::CALL,
			this->module.calls.emplace_back(this->get_stmt_name(std::move(name)), func, args)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}


	auto Agent::createCall(ExternalFunction::ID func, evo::SmallVector<Expr>&& args, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getExternalFunction(func).returnType.kind() != Type::Kind::VOID,
			"Call cannot return `Void` (did you mean CallVoid?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::CALL,
			this->module.calls.emplace_back(this->get_stmt_name(std::move(name)), func, std::move(args))
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::createCall(ExternalFunction::ID func, const evo::SmallVector<Expr>& args, std::string&& name) const
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getExternalFunction(func).returnType.kind() != Type::Kind::VOID,
			"Call cannot return `Void` (did you mean CallVoid?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::CALL,
			this->module.calls.emplace_back(this->get_stmt_name(std::move(name)), func, args)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}


	auto Agent::createCall(
		const Expr& func, const Type& func_type, evo::SmallVector<Expr>&& args, std::string&& name
	) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunctionType(func_type).returnType.kind() != Type::Kind::VOID,
			"Call cannot return `Void` (did you mean CallVoid?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func_type, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::CALL,
			this->module.calls.emplace_back(
				this->get_stmt_name(std::move(name)), PtrCall(func, func_type), std::move(args)
			)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::createCall(
		const Expr& func, const Type& func_type, const evo::SmallVector<Expr>& args, std::string&& name
	) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunctionType(func_type).returnType.kind() != Type::Kind::VOID,
			"Call cannot return `Void` (did you mean CallVoid?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func_type, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::CALL,
			this->module.calls.emplace_back(this->get_stmt_name(std::move(name)), PtrCall(func, func_type), args)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}


	auto Agent::getCall(const Expr& expr) const -> const Call& {
		return ReaderAgent(this->module, this->getTargetFunction()).getCall(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// call voids

	auto Agent::createCallVoid(Function::ID func, evo::SmallVector<Expr>&& args) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunction(func).getReturnType().kind() == Type::Kind::VOID,
			"CallVoid must return `Void` (did you mean Call?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::CALL_VOID, this->module.call_voids.emplace_back(func, std::move(args))
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::createCallVoid(Function::ID func, const evo::SmallVector<Expr>& args) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunction(func).getReturnType().kind() == Type::Kind::VOID,
			"CallVoid must return `Void` (did you mean Call?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(Expr::Kind::CALL_VOID, this->module.call_voids.emplace_back(func, args));
		this->insert_stmt(new_expr);
		return new_expr;
	}


	auto Agent::createCallVoid(ExternalFunction::ID func, evo::SmallVector<Expr>&& args) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getExternalFunction(func).returnType.kind() == Type::Kind::VOID,
			"CallVoid must return `Void` (did you mean Call?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::CALL_VOID, this->module.call_voids.emplace_back(func, std::move(args))
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::createCallVoid(ExternalFunction::ID func, const evo::SmallVector<Expr>& args) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getExternalFunction(func).returnType.kind() == Type::Kind::VOID,
			"CallVoid must return `Void` (did you mean Call?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(Expr::Kind::CALL_VOID, this->module.call_voids.emplace_back(func, args));
		this->insert_stmt(new_expr);
		return new_expr;
	}


	auto Agent::createCallVoid(const Expr& func, const Type& func_type, evo::SmallVector<Expr>&& args) const
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunctionType(func_type).returnType.kind() == Type::Kind::VOID,
			"CallVoid must return `Void` (did you mean Call?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func_type, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::CALL_VOID,
			this->module.call_voids.emplace_back(PtrCall(func, func_type), std::move(args))
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::createCallVoid(const Expr& func, const Type& func_type, const evo::SmallVector<Expr>& args) const
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunctionType(func_type).returnType.kind() == Type::Kind::VOID,
			"CallVoid must return `Void` (did you mean Call?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func_type, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::CALL_VOID, this->module.call_voids.emplace_back(PtrCall(func, func_type), args)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}


	auto Agent::getCallVoid(const Expr& expr) const -> const CallVoid& {
		return ReaderAgent(this->module, this->getTargetFunction()).getCallVoid(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// abort / breakpoint

	auto Agent::createAbort() const -> Expr {
		const auto new_expr = Expr(Expr::Kind::ABORT);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::createBreakpoint() const -> Expr {
		const auto new_expr = Expr(Expr::Kind::BREAKPOINT);
		this->insert_stmt(new_expr);
		return new_expr;
	}


	//////////////////////////////////////////////////////////////////////
	// ret instructions

	auto Agent::createRet(const Expr& expr) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(expr.isValue(), "Must return value");
		evo::debugAssert(
			this->getExprType(expr) == this->target_func->getReturnType(), "Return type must match function"
		);

		const auto new_expr = Expr(Expr::Kind::RET, this->module.rets.emplace_back(expr));
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::createRet() const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(this->target_func->getReturnType().kind() == Type::Kind::VOID, "Return type must match");

		const auto new_expr = Expr(Expr::Kind::RET, this->module.rets.emplace_back(std::nullopt));
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getRet(const Expr& expr) const -> const Ret& {
		return ReaderAgent(this->module, this->getTargetFunction()).getRet(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// jump instructions

	auto Agent::createJump(BasicBlock::ID basic_block_id) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");

		const auto new_expr = Expr(Expr::Kind::JUMP, basic_block_id.get());
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getJump(const Expr& expr) -> Jump {
		return ReaderAgent::getJump(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// branch instructions

	auto Agent::createBranch(const Expr& cond, BasicBlock::ID then_block, BasicBlock::ID else_block) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(this->getExprType(cond).kind() == Type::Kind::BOOL, "Cond must be of type Bool");

		const auto new_expr = Expr(
			Expr::Kind::BRANCH, this->module.branches.emplace_back(cond, then_block, else_block)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getBranch(const Expr& expr) const -> Branch {
		return ReaderAgent(this->module, this->getTargetFunction()).getBranch(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// unreachable

	auto Agent::createUnreachable() const -> Expr {
		const auto new_expr = Expr(Expr::Kind::UNREACHABLE);
		this->insert_stmt(new_expr);
		return new_expr;
	}


	//////////////////////////////////////////////////////////////////////
	// phi instructions

	auto Agent::createPhi(evo::SmallVector<Phi::Predecessor>&& predecessors, std::string&& name) const-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(predecessors.size() >= 2, "Phi statement must have at least 2 predecessor blocks");

		#if defined(PCIT_CONFIG_DEBUG)
			const Type first_type = this->getExprType(predecessors[0].value);

			for(size_t i = 1; i < predecessors.size(); i+=1){
				evo::debugAssert(
					this->getExprType(predecessors[i].value) == first_type,
					"All predecessor blocks must return the same type"
				);
			}
		#endif

		const auto new_expr = Expr(
			Expr::Kind::PHI,
			this->module.phis.emplace_back(this->get_stmt_name(std::move(name)), std::move(predecessors))
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getPhi(const Expr& expr) const -> Phi {
		return ReaderAgent(this->module, this->getTargetFunction()).getPhi(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// switches

	auto Agent::createSwitch(Expr cond, evo::SmallVector<Switch::Case>&& cases, BasicBlock::ID default_block) const
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(cases.empty() == false, "Switch statement must have at least 1 case");

		#if defined(PCIT_CONFIG_DEBUG)
			const pir::Type cond_type = this->getExprType(cond);

			evo::debugAssert(cond_type.kind() == pir::Type::Kind::INTEGER, "Cond type must be integral");

			for(const Switch::Case& target_case : cases){
				evo::debugAssert(
					this->getExprType(target_case.value) == cond_type,
					"All switch cases have the same type as the condition"
				);
			}
		#endif

		const auto new_expr = Expr(
			Expr::Kind::SWITCH, this->module.switches.emplace_back(cond, std::move(cases), default_block)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getSwitch(const Expr& expr) const -> Switch {
		return ReaderAgent(this->module, this->getTargetFunction()).getSwitch(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// alloca

	auto Agent::createAlloca(const Type& type, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetFunction(), "No target functions set");

		return Expr(
			Expr::Kind::ALLOCA, this->target_func->allocas.emplace_back(this->get_stmt_name(std::move(name)), type)
		);
	}

	auto Agent::getAlloca(const Expr& expr) const -> const Alloca& {
		return ReaderAgent(this->module, this->getTargetFunction()).getAlloca(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// load

	auto Agent::createLoad(
		const Expr& source, const Type& type, std::string&& name, bool is_volatile, AtomicOrdering atomic_ordering
	) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(atomic_ordering.isValidForLoad(), "This atomic ordering is not valid for a load");
		evo::debugAssert(this->getExprType(source).kind() == Type::Kind::PTR, "Source must be of type Ptr");

		const auto new_stmt = Expr(
			Expr::Kind::LOAD,
			this->module.loads.emplace_back(
				this->get_stmt_name(std::move(name)), source, type, is_volatile, atomic_ordering
			)
		);
		this->insert_stmt(new_stmt);
		return new_stmt;
	}

	auto Agent::getLoad(const Expr& expr) const -> const Load& {
		return ReaderAgent(this->module, this->getTargetFunction()).getLoad(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// store

	auto Agent::createStore(
		const Expr& destination, const Expr& value, bool is_volatile, AtomicOrdering atomic_ordering
	) const -> void {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(atomic_ordering.isValidForStore(), "This atomic ordering is not valid for a store");
		evo::debugAssert(
			this->getExprType(destination).kind() == Type::Kind::PTR, "Destination must be of type Ptr"
		);

		const auto new_stmt = Expr(
			Expr::Kind::STORE, this->module.stores.emplace_back(destination, value, is_volatile, atomic_ordering)
		);
		this->insert_stmt(new_stmt);
	}

	auto Agent::getStore(const Expr& expr) const -> const Store& {
		return ReaderAgent(this->module, this->getTargetFunction()).getStore(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// calc ptr

	EVO_NODISCARD auto Agent::createCalcPtr(
		const Expr& base_ptr, const Type& ptr_type, evo::SmallVector<CalcPtr::Index>&& indices, std::string&& name
	) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(this->getExprType(base_ptr).kind() == Type::Kind::PTR, "Base ptr must be of type Ptr");
		evo::debugAssert(!indices.empty(), "There must be at least one index");
		#if defined(PCIT_CONFIG_DEBUG)
			Type target_type = ptr_type;

			if(target_type.isAggregate()){
				for(size_t i = 1; i < indices.size(); i+=1){
					const CalcPtr::Index& index = indices[i];

					evo::debugAssert(
						index.is<int64_t>() || this->getExprType(index.as<Expr>()).kind() == Type::Kind::INTEGER,
						"@calcPtr indicies must be integers"
					);

					if(target_type.kind() == Type::Kind::ARRAY){
						const ArrayType& array_type = this->module.getArrayType(target_type);

						if(index.is<int64_t>()){
							evo::debugAssert(
								index.as<int64_t>() >= 0 && size_t(index.as<int64_t>()) < array_type.length,
								"indexing into an array must be a valid index"
							);
							
						}else if(index.as<Expr>().kind() == Expr::Kind::NUMBER){
							const int64_t member_index = static_cast<int64_t>(
								this->getNumber(index.as<Expr>()).getInt()
							);
							evo::debugAssert(
								member_index >= 0 && size_t(member_index) < array_type.length,
								"indexing into an array must be a valid index"
							);
						}

						target_type = array_type.elemType;

					}else if(target_type.kind() == Type::Kind::STRUCT){
						evo::debugAssert(index.is<int64_t>(), "Cannot index into a struct with a pcit::pir::Expr");

						const StructType& struct_type = this->module.getStructType(target_type);

						evo::debugAssert(
							index.as<int64_t>() >= 0 && size_t(index.as<int64_t>()) < struct_type.members.size(),
							"indexing into a struct must be a valid member index"
						);
						target_type = struct_type.members[size_t(index.as<int64_t>())];

					}else{
						evo::debugAssert(i+1 == indices.size(), "@calcPtr cannot index a Non-aggregate type");
					}
				}
			}else{
				evo::debugAssert(indices.size() == 1, "Non-aggregate type for @calcPtr must have exactly 1 index");
			}
		#endif

		const auto new_stmt = Expr(
			Expr::Kind::CALC_PTR,
			this->module.calc_ptrs.emplace_back(
				this->get_stmt_name(std::move(name)), ptr_type, base_ptr, std::move(indices)
			)
		);
		this->insert_stmt(new_stmt);
		return new_stmt;
	}

	EVO_NODISCARD auto Agent::getCalcPtr(const Expr& expr) const -> const CalcPtr& {
		return ReaderAgent(this->module, this->getTargetFunction()).getCalcPtr(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// memcpy

	auto Agent::createMemcpy(const Expr& dst, const Expr& src, const Expr& num_bytes, bool is_volatile) const -> Expr {
		evo::debugAssert(this->getExprType(dst).kind() == Type::Kind::PTR, "dst must be pointer");
		evo::debugAssert(this->getExprType(src).kind() == Type::Kind::PTR, "src must be pointer");
		evo::debugAssert(
			this->getExprType(num_bytes).kind() == Type::Kind::INTEGER && this->getExprType(num_bytes).getWidth() <= 64,
			"num bytes must be integral and have a max width of 64"
		);



		const auto new_stmt = Expr(
			Expr::Kind::MEMCPY, this->module.memcpys.emplace_back(dst, src, num_bytes, is_volatile)
		);
		this->insert_stmt(new_stmt);
		return new_stmt;
	}

	auto Agent::getMemcpy(const Expr& expr) const -> const Memcpy& {
		return ReaderAgent(this->module, this->getTargetFunction()).getMemcpy(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// memset

	auto Agent::createMemset(const Expr& dst, const Expr& value, const Expr& num_bytes, bool is_volatile) const 
	-> Expr {
		evo::debugAssert(this->getExprType(dst).kind() == Type::Kind::PTR, "dst must be pointer");
		evo::debugAssert(
			this->getExprType(value).kind() == Type::Kind::INTEGER && this->getExprType(value).getWidth() == 8,
			"value must be of type I8"
		);
		evo::debugAssert(
			this->getExprType(num_bytes).kind() == Type::Kind::INTEGER
			&& this->getExprType(num_bytes).getWidth() <= 64,
			"num bytes must be integral and have a max width of 64"
		);


		const auto new_stmt = Expr(
			Expr::Kind::MEMSET, this->module.memsets.emplace_back(dst, value, num_bytes, is_volatile)
		);
		this->insert_stmt(new_stmt);
		return new_stmt;
	}

	auto Agent::getMemset(const Expr& expr) const -> const Memset& {
		return ReaderAgent(this->module, this->getTargetFunction()).getMemset(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// BitCast
	
	EVO_NODISCARD auto Agent::createBitCast(const Expr& fromValue, const Type& toType, std::string&& name) const
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getSize(this->getExprType(fromValue)) == this->module.getSize(toType),
			"Cannot convert to a type of a different size ({} != {})",
			this->module.getSize(this->getExprType(fromValue)),
			this->module.getSize(toType)
		);

		const auto new_expr = Expr(
			Expr::Kind::BIT_CAST,
			this->module.bitcasts.emplace_back(this->get_stmt_name(std::move(name)), fromValue, toType)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	EVO_NODISCARD auto Agent::getBitCast(const Expr& expr) const -> const BitCast& {
		return ReaderAgent(this->module, this->getTargetFunction()).getBitCast(expr);
	}

	
	//////////////////////////////////////////////////////////////////////
	// Trunc
	
	EVO_NODISCARD auto Agent::createTrunc(const Expr& fromValue, const Type& toType, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getSize(this->getExprType(fromValue)) >= this->module.getSize(toType),
			"Cannot convert to a type of a greater size"
		);
		evo::debugAssert(this->getExprType(fromValue).kind() == Type::Kind::INTEGER, "can only convert integers");
		evo::debugAssert(toType.kind() == Type::Kind::INTEGER, "can only convert to integers");

		const auto new_expr = Expr(
			Expr::Kind::TRUNC,
			this->module.truncs.emplace_back(this->get_stmt_name(std::move(name)), fromValue, toType)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	EVO_NODISCARD auto Agent::getTrunc(const Expr& expr) const -> const Trunc& {
		return ReaderAgent(this->module, this->getTargetFunction()).getTrunc(expr);
	}

	
	//////////////////////////////////////////////////////////////////////
	// FTrunc
	
	EVO_NODISCARD auto Agent::createFTrunc(const Expr& fromValue, const Type& toType, std::string&& name) const
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getSize(this->getExprType(fromValue)) >= this->module.getSize(toType),
			"Cannot convert to a type of a greater size"
		);
		evo::debugAssert(this->getExprType(fromValue).isFloat(), "can only convert floats");
		evo::debugAssert(toType.isFloat(), "can only convert to floats");

		const auto new_expr = Expr(
			Expr::Kind::FTRUNC,
			this->module.ftruncs.emplace_back(this->get_stmt_name(std::move(name)), fromValue, toType)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	EVO_NODISCARD auto Agent::getFTrunc(const Expr& expr) const -> const FTrunc& {
		return ReaderAgent(this->module, this->getTargetFunction()).getFTrunc(expr);
	}

	
	//////////////////////////////////////////////////////////////////////
	// SExt
	
	EVO_NODISCARD auto Agent::createSExt(const Expr& fromValue, const Type& toType, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getSize(this->getExprType(fromValue)) <= this->module.getSize(toType),
			"Cannot convert to a type of a smaller size"
		);
		evo::debugAssert(this->getExprType(fromValue).kind() == Type::Kind::INTEGER, "can only convert integers");
		evo::debugAssert(toType.kind() == Type::Kind::INTEGER, "can only convert to integers");

		const auto new_expr = Expr(
			Expr::Kind::SEXT,
			this->module.sexts.emplace_back(this->get_stmt_name(std::move(name)), fromValue, toType)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	EVO_NODISCARD auto Agent::getSExt(const Expr& expr) const -> const SExt& {
		return ReaderAgent(this->module, this->getTargetFunction()).getSExt(expr);
	}

	
	//////////////////////////////////////////////////////////////////////
	// ZExt
	
	EVO_NODISCARD auto Agent::createZExt(const Expr& fromValue, const Type& toType, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getSize(this->getExprType(fromValue)) <= this->module.getSize(toType),
			"Cannot convert to a type of a smaller size"
		);
		evo::debugAssert(
			this->getExprType(fromValue).kind() == Type::Kind::INTEGER
			|| this->getExprType(fromValue).kind() == Type::Kind::BOOL,
			"can only convert from integers and Bool"
		);
		evo::debugAssert(toType.kind() == Type::Kind::INTEGER, "can only convert to integers");

		const auto new_expr = Expr(
			Expr::Kind::ZEXT,
			this->module.zexts.emplace_back(this->get_stmt_name(std::move(name)), fromValue, toType)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	EVO_NODISCARD auto Agent::getZExt(const Expr& expr) const -> const ZExt& {
		return ReaderAgent(this->module, this->getTargetFunction()).getZExt(expr);
	}

	
	//////////////////////////////////////////////////////////////////////
	// FExt
	
	EVO_NODISCARD auto Agent::createFExt(const Expr& fromValue, const Type& toType, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getSize(this->getExprType(fromValue)) <= this->module.getSize(toType),
			"Cannot convert to a type of a smaller size"
		);
		evo::debugAssert(this->getExprType(fromValue).isFloat(), "can only convert floats");
		evo::debugAssert(toType.isFloat(), "can only convert to floats");

		const auto new_expr = Expr(
			Expr::Kind::FEXT,
			this->module.fexts.emplace_back(this->get_stmt_name(std::move(name)), fromValue, toType)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	EVO_NODISCARD auto Agent::getFExt(const Expr& expr) const -> const FExt& {
		return ReaderAgent(this->module, this->getTargetFunction()).getFExt(expr);
	}

	
	//////////////////////////////////////////////////////////////////////
	// IToF
	
	EVO_NODISCARD auto Agent::createIToF(const Expr& fromValue, const Type& toType, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(this->getExprType(fromValue).kind() == Type::Kind::INTEGER, "can only convert integers");
		evo::debugAssert(toType.isFloat(), "can only convert to floats");

		const auto new_expr = Expr(
			Expr::Kind::ITOF,
			this->module.itofs.emplace_back(this->get_stmt_name(std::move(name)), fromValue, toType)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	EVO_NODISCARD auto Agent::getIToF(const Expr& expr) const -> const IToF& {
		return ReaderAgent(this->module, this->getTargetFunction()).getIToF(expr);
	}

	
	//////////////////////////////////////////////////////////////////////
	// UIToF
	
	EVO_NODISCARD auto Agent::createUIToF(const Expr& fromValue, const Type& toType, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(this->getExprType(fromValue).kind() == Type::Kind::INTEGER, "can only convert integers");
		evo::debugAssert(toType.isFloat(), "can only convert to floats");

		const auto new_expr = Expr(
			Expr::Kind::UITOF,
			this->module.uitofs.emplace_back(this->get_stmt_name(std::move(name)), fromValue, toType)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	EVO_NODISCARD auto Agent::getUIToF(const Expr& expr) const -> const UIToF& {
		return ReaderAgent(this->module, this->getTargetFunction()).getUIToF(expr);
	}

	
	//////////////////////////////////////////////////////////////////////
	// FToI
	
	EVO_NODISCARD auto Agent::createFToI(const Expr& fromValue, const Type& toType, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(this->getExprType(fromValue).isFloat(), "can only convert floats");
		evo::debugAssert(toType.kind() == Type::Kind::INTEGER, "can only convert to integers");

		const auto new_expr = Expr(
			Expr::Kind::FTOI,
			this->module.ftois.emplace_back(this->get_stmt_name(std::move(name)), fromValue, toType)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	EVO_NODISCARD auto Agent::getFToI(const Expr& expr) const -> const FToI& {
		return ReaderAgent(this->module, this->getTargetFunction()).getFToI(expr);
	}

	
	//////////////////////////////////////////////////////////////////////
	// FToUI
	
	EVO_NODISCARD auto Agent::createFToUI(const Expr& fromValue, const Type& toType, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(this->getExprType(fromValue).isFloat(), "can only convert floats");
		evo::debugAssert(
			toType.kind() == Type::Kind::INTEGER || toType.kind() == Type::Kind::BOOL,
			"can only convert to integers and Bool"
		);

		const auto new_expr = Expr(
			Expr::Kind::FTOUI,
			this->module.ftouis.emplace_back(this->get_stmt_name(std::move(name)), fromValue, toType)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	EVO_NODISCARD auto Agent::getFToUI(const Expr& expr) const -> const FToUI& {
		return ReaderAgent(this->module, this->getTargetFunction()).getFToUI(expr);
	}




	//////////////////////////////////////////////////////////////////////
	// add

	auto Agent::createAdd(const Expr& lhs, const Expr& rhs, bool nsw, bool nuw, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER, "The @add instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::ADD,
			this->module.adds.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs, nsw, nuw)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getAdd(const Expr& expr) const -> const Add& {
		return ReaderAgent(this->module, this->getTargetFunction()).getAdd(expr);
	}



	//////////////////////////////////////////////////////////////////////
	// signed add wrap

	auto Agent::createSAddWrap(const Expr& lhs, const Expr& rhs, std::string&& result_name, std::string&& wrapped_name)
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(this->getExprType(lhs).kind() == Type::Kind::INTEGER, "Can only add wrap integers");

		std::string result_stmt_name = this->get_stmt_name(std::move(result_name));
		std::string wrapped_stmt_name = this->get_stmt_name_with_forward_include(
			std::move(wrapped_name), {result_stmt_name}
		);

		const auto new_expr = Expr(
			Expr::Kind::SADD_WRAP,
			this->module.sadd_wraps.emplace_back(std::move(result_stmt_name), std::move(wrapped_stmt_name), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getSAddWrap(const Expr& expr) const -> const SAddWrap& {
		return ReaderAgent(this->module, this->getTargetFunction()).getSAddWrap(expr);
	}


	auto Agent::extractSAddWrapResult(const Expr& expr) -> Expr {
		return ReaderAgent::extractSAddWrapResult(expr);
	}

	auto Agent::extractSAddWrapWrapped(const Expr& expr) -> Expr {
		return ReaderAgent::extractSAddWrapWrapped(expr);
	}



	//////////////////////////////////////////////////////////////////////
	// unsigned add wrap

	auto Agent::createUAddWrap(const Expr& lhs, const Expr& rhs, std::string&& result_name, std::string&& wrapped_name)
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(this->getExprType(lhs).kind() == Type::Kind::INTEGER, "Can only add wrap integers");

		std::string result_stmt_name = this->get_stmt_name(std::move(result_name));
		std::string wrapped_stmt_name = this->get_stmt_name_with_forward_include(
			std::move(wrapped_name), {result_stmt_name}
		);

		const auto new_expr = Expr(
			Expr::Kind::UADD_WRAP,
			this->module.uadd_wraps.emplace_back(std::move(result_stmt_name), std::move(wrapped_stmt_name), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getUAddWrap(const Expr& expr) const -> const UAddWrap& {
		return ReaderAgent(this->module, this->getTargetFunction()).getUAddWrap(expr);
	}


	auto Agent::extractUAddWrapResult(const Expr& expr) -> Expr {
		return ReaderAgent::extractUAddWrapResult(expr);
	}

	auto Agent::extractUAddWrapWrapped(const Expr& expr) -> Expr {
		return ReaderAgent::extractUAddWrapWrapped(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// saddSat

	auto Agent::createSAddSat(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER, "The @saddSat instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::SADD_SAT,
			this->module.sadd_sats.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getSAddSat(const Expr& expr) const -> const SAddSat& {
		return ReaderAgent(this->module, this->getTargetFunction()).getSAddSat(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// uaddSat

	auto Agent::createUAddSat(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER, "The @uaddSat instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::UADD_SAT,
			this->module.uadd_sats.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getUAddSat(const Expr& expr) const -> const UAddSat& {
		return ReaderAgent(this->module, this->getTargetFunction()).getUAddSat(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// fadd

	auto Agent::createFAdd(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(this->getExprType(lhs).isFloat(), "The @fAdd instruction only supports float values");

		const auto new_expr = Expr(
			Expr::Kind::FADD, this->module.fadds.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getFAdd(const Expr& expr) const -> const FAdd& {
		return ReaderAgent(this->module, this->getTargetFunction()).getFAdd(expr);
	}



	//////////////////////////////////////////////////////////////////////
	// sub

	auto Agent::createSub(const Expr& lhs, const Expr& rhs, bool nsw, bool nuw, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER, "The @sub instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::SUB,
			this->module.subs.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs, nsw, nuw)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getSub(const Expr& expr) const -> const Sub& {
		return ReaderAgent(this->module, this->getTargetFunction()).getSub(expr);
	}



	//////////////////////////////////////////////////////////////////////
	// signed sub wrap

	auto Agent::createSSubWrap(const Expr& lhs, const Expr& rhs, std::string&& result_name, std::string&& wrapped_name)
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(this->getExprType(lhs).kind() == Type::Kind::INTEGER, "Can only sub wrap integers");

		std::string result_stmt_name = this->get_stmt_name(std::move(result_name));
		std::string wrapped_stmt_name = this->get_stmt_name_with_forward_include(
			std::move(wrapped_name), {result_stmt_name}
		);

		const auto new_expr = Expr(
			Expr::Kind::SSUB_WRAP,
			this->module.ssub_wraps.emplace_back(std::move(result_stmt_name), std::move(wrapped_stmt_name), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getSSubWrap(const Expr& expr) const -> const SSubWrap& {
		return ReaderAgent(this->module, this->getTargetFunction()).getSSubWrap(expr);
	}


	auto Agent::extractSSubWrapResult(const Expr& expr) -> Expr {
		return ReaderAgent::extractSSubWrapResult(expr);
	}

	auto Agent::extractSSubWrapWrapped(const Expr& expr) -> Expr {
		return ReaderAgent::extractSSubWrapWrapped(expr);
	}



	//////////////////////////////////////////////////////////////////////
	// unsigned sub wrap

	auto Agent::createUSubWrap(const Expr& lhs, const Expr& rhs, std::string&& result_name, std::string&& wrapped_name)
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(this->getExprType(lhs).kind() == Type::Kind::INTEGER, "Can only sub wrap integers");

		std::string result_stmt_name = this->get_stmt_name(std::move(result_name));
		std::string wrapped_stmt_name = this->get_stmt_name_with_forward_include(
			std::move(wrapped_name), {result_stmt_name}
		);

		const auto new_expr = Expr(
			Expr::Kind::USUB_WRAP,
			this->module.usub_wraps.emplace_back(std::move(result_stmt_name), std::move(wrapped_stmt_name), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getUSubWrap(const Expr& expr) const -> const USubWrap& {
		return ReaderAgent(this->module, this->getTargetFunction()).getUSubWrap(expr);
	}


	auto Agent::extractUSubWrapResult(const Expr& expr) -> Expr {
		return ReaderAgent::extractUSubWrapResult(expr);
	}

	auto Agent::extractUSubWrapWrapped(const Expr& expr) -> Expr {
		return ReaderAgent::extractUSubWrapWrapped(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// ssubSat

	auto Agent::createSSubSat(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER, "The @ssubSat instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::SSUB_SAT,
			this->module.ssub_sats.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getSSubSat(const Expr& expr) const -> const SSubSat& {
		return ReaderAgent(this->module, this->getTargetFunction()).getSSubSat(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// usubSat

	auto Agent::createUSubSat(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER, "The @usubSat instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::USUB_SAT,
			this->module.usub_sats.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getUSubSat(const Expr& expr) const -> const USubSat& {
		return ReaderAgent(this->module, this->getTargetFunction()).getUSubSat(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// fsub

	auto Agent::createFSub(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(this->getExprType(lhs).isFloat(), "The @fSub instruction only supports float values");

		const auto new_expr = Expr(
			Expr::Kind::FSUB, this->module.fsubs.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getFSub(const Expr& expr) const -> const FSub& {
		return ReaderAgent(this->module, this->getTargetFunction()).getFSub(expr);
	}



	//////////////////////////////////////////////////////////////////////
	// mul

	auto Agent::createMul(const Expr& lhs, const Expr& rhs, bool nsw, bool nuw, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER, "The @mul instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::MUL,
			this->module.muls.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs, nsw, nuw)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getMul(const Expr& expr) const -> const Mul& {
		return ReaderAgent(this->module, this->getTargetFunction()).getMul(expr);
	}



	//////////////////////////////////////////////////////////////////////
	// signed mul wrap

	auto Agent::createSMulWrap(const Expr& lhs, const Expr& rhs, std::string&& result_name, std::string&& wrapped_name)
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(this->getExprType(lhs).kind() == Type::Kind::INTEGER, "Can only mul wrap integers");

		const std::string result_stmt_name = this->get_stmt_name(std::move(result_name));
		const std::string wrapped_stmt_name = this->get_stmt_name_with_forward_include(
			std::move(wrapped_name), {result_stmt_name}
		);

		const auto new_expr = Expr(
			Expr::Kind::SMUL_WRAP,
			this->module.smul_wraps.emplace_back(std::move(result_stmt_name), std::move(wrapped_stmt_name), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getSMulWrap(const Expr& expr) const -> const SMulWrap& {
		return ReaderAgent(this->module, this->getTargetFunction()).getSMulWrap(expr);
	}


	auto Agent::extractSMulWrapResult(const Expr& expr) -> Expr {
		return ReaderAgent::extractSMulWrapResult(expr);
	}

	auto Agent::extractSMulWrapWrapped(const Expr& expr) -> Expr {
		return ReaderAgent::extractSMulWrapWrapped(expr);
	}



	//////////////////////////////////////////////////////////////////////
	// unsigned mul wrap

	auto Agent::createUMulWrap(const Expr& lhs, const Expr& rhs, std::string&& result_name, std::string&& wrapped_name)
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(this->getExprType(lhs).kind() == Type::Kind::INTEGER, "Can only mul wrap integers");

		std::string result_stmt_name = this->get_stmt_name(std::move(result_name));
		std::string wrapped_stmt_name = this->get_stmt_name_with_forward_include(
			std::move(wrapped_name), {result_stmt_name}
		);

		const auto new_expr = Expr(
			Expr::Kind::UMUL_WRAP,
			this->module.umul_wraps.emplace_back(std::move(result_stmt_name), std::move(wrapped_stmt_name), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getUMulWrap(const Expr& expr) const -> const UMulWrap& {
		return ReaderAgent(this->module, this->getTargetFunction()).getUMulWrap(expr);
	}


	auto Agent::extractUMulWrapResult(const Expr& expr) -> Expr {
		return ReaderAgent::extractUMulWrapResult(expr);
	}

	auto Agent::extractUMulWrapWrapped(const Expr& expr) -> Expr {
		return ReaderAgent::extractUMulWrapWrapped(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// smulSat

	auto Agent::createSMulSat(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER, "The @smulSat instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::SMUL_SAT,
			this->module.smul_sats.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getSMulSat(const Expr& expr) const -> const SMulSat& {
		return ReaderAgent(this->module, this->getTargetFunction()).getSMulSat(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// umulSat

	auto Agent::createUMulSat(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER, "The @umulSat instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::UMUL_SAT,
			this->module.umul_sats.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getUMulSat(const Expr& expr) const -> const UMulSat& {
		return ReaderAgent(this->module, this->getTargetFunction()).getUMulSat(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// fmul

	auto Agent::createFMul(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(this->getExprType(lhs).isFloat(), "The @fmul instruction only supports float values");

		const auto new_expr = Expr(
			Expr::Kind::FMUL, this->module.fmuls.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getFMul(const Expr& expr) const -> const FMul& {
		return ReaderAgent(this->module, this->getTargetFunction()).getFMul(expr);
	}



	//////////////////////////////////////////////////////////////////////
	// sdiv

	auto Agent::createSDiv(const Expr& lhs, const Expr& rhs, bool is_exact, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER, "The @sdiv instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::SDIV,
			this->module.sdivs.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs, is_exact)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getSDiv(const Expr& expr) const -> const SDiv& {
		return ReaderAgent(this->module, this->getTargetFunction()).getSDiv(expr);
	}



	//////////////////////////////////////////////////////////////////////
	// udiv

	auto Agent::createUDiv(const Expr& lhs, const Expr& rhs, bool is_exact, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER, "The @udiv instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::UDIV,
			this->module.udivs.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs, is_exact)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getUDiv(const Expr& expr) const -> const UDiv& {
		return ReaderAgent(this->module, this->getTargetFunction()).getUDiv(expr);
	}



	//////////////////////////////////////////////////////////////////////
	// fdiv

	auto Agent::createFDiv(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(this->getExprType(lhs).isFloat(), "The @fdiv instruction only supports float values");

		const auto new_expr = Expr(
			Expr::Kind::FDIV, this->module.fdivs.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getFDiv(const Expr& expr) const -> const FDiv& {
		return ReaderAgent(this->module, this->getTargetFunction()).getFDiv(expr);
	}



	//////////////////////////////////////////////////////////////////////
	// srem

	auto Agent::createSRem(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER, "The @srem instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::SREM,
			this->module.srems.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getSRem(const Expr& expr) const -> const SRem& {
		return ReaderAgent(this->module, this->getTargetFunction()).getSRem(expr);
	}



	//////////////////////////////////////////////////////////////////////
	// urem

	auto Agent::createURem(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER, "The @urem instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::UREM,
			this->module.urems.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getURem(const Expr& expr) const -> const URem& {
		return ReaderAgent(this->module, this->getTargetFunction()).getURem(expr);
	}



	//////////////////////////////////////////////////////////////////////
	// frem

	auto Agent::createFRem(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(this->getExprType(lhs).isFloat(), "The @frem instruction only supports float values");

		const auto new_expr = Expr(
			Expr::Kind::FREM, this->module.frems.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getFRem(const Expr& expr) const -> const FRem& {
		return ReaderAgent(this->module, this->getTargetFunction()).getFRem(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// fneg


	auto Agent::createFNeg(const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(rhs.isValue(), "Argument must be value");
		evo::debugAssert(this->getExprType(rhs).isFloat(), "The @fneg instruction only supports float values");

		const auto new_expr = Expr(
			Expr::Kind::FNEG, this->module.fnegs.emplace_back(this->get_stmt_name(std::move(name)), rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getFNeg(const Expr& expr) const -> const FNeg& {
		return ReaderAgent(this->module, this->getTargetFunction()).getFNeg(expr);
	}



	//////////////////////////////////////////////////////////////////////
	// equal

	auto Agent::createIEq(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER 
				|| this->getExprType(lhs).kind() == Type::Kind::BOOL
				|| this->getExprType(lhs).kind() == Type::Kind::PTR,
			"The @ieq instruction only supports integers, Bools, and pointers"
		);

		const auto new_expr = Expr(
			Expr::Kind::IEQ,
			this->module.ieqs.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getIEq(const Expr& expr) const -> const IEq& {
		return ReaderAgent(this->module, this->getTargetFunction()).getIEq(expr);
	}


	auto Agent::createFEq(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(this->getExprType(lhs).isFloat(), "The @feq instruction only supports float values");

		const auto new_expr = Expr(
			Expr::Kind::FEQ, this->module.feqs.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getFEq(const Expr& expr) const -> const FEq& {
		return ReaderAgent(this->module, this->getTargetFunction()).getFEq(expr);
	}



	//////////////////////////////////////////////////////////////////////
	// not equal

	auto Agent::createINeq(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER 
				|| this->getExprType(lhs).kind() == Type::Kind::BOOL
				|| this->getExprType(lhs).kind() == Type::Kind::PTR,
			"The @ineq instruction only supports integers, Bools, and pointers"
		);

		const auto new_expr = Expr(
			Expr::Kind::INEQ,
			this->module.ineqs.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getINeq(const Expr& expr) const -> const INeq& {
		return ReaderAgent(this->module, this->getTargetFunction()).getINeq(expr);
	}


	auto Agent::createFNeq(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(this->getExprType(lhs).isFloat(), "The @fneq instruction only supports float values");

		const auto new_expr = Expr(
			Expr::Kind::FNEQ, this->module.fneqs.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getFNeq(const Expr& expr) const -> const FNeq& {
		return ReaderAgent(this->module, this->getTargetFunction()).getFNeq(expr);
	}



	//////////////////////////////////////////////////////////////////////
	// less than

	auto Agent::createSLT(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER, "The @slt instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::SLT,
			this->module.slts.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getSLT(const Expr& expr) const -> const SLT& {
		return ReaderAgent(this->module, this->getTargetFunction()).getSLT(expr);
	}


	auto Agent::createULT(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER, "The @ult instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::ULT,
			this->module.ults.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getULT(const Expr& expr) const -> const ULT& {
		return ReaderAgent(this->module, this->getTargetFunction()).getULT(expr);
	}


	auto Agent::createFLT(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(this->getExprType(lhs).isFloat(), "The @flt instruction only supports float values");

		const auto new_expr = Expr(
			Expr::Kind::FLT, this->module.flts.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getFLT(const Expr& expr) const -> const FLT& {
		return ReaderAgent(this->module, this->getTargetFunction()).getFLT(expr);
	}



	//////////////////////////////////////////////////////////////////////
	// less than or equal to

	auto Agent::createSLTE(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER, "The @slte instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::SLTE,
			this->module.sltes.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getSLTE(const Expr& expr) const -> const SLTE& {
		return ReaderAgent(this->module, this->getTargetFunction()).getSLTE(expr);
	}


	auto Agent::createULTE(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER, "The @ulte instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::ULTE,
			this->module.ultes.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getULTE(const Expr& expr) const -> const ULTE& {
		return ReaderAgent(this->module, this->getTargetFunction()).getULTE(expr);
	}


	auto Agent::createFLTE(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(this->getExprType(lhs).isFloat(), "The @flte instruction only supports float values");

		const auto new_expr = Expr(
			Expr::Kind::FLTE, this->module.fltes.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getFLTE(const Expr& expr) const -> const FLTE& {
		return ReaderAgent(this->module, this->getTargetFunction()).getFLTE(expr);
	}



	//////////////////////////////////////////////////////////////////////
	// greater than

	auto Agent::createSGT(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER, "The @sgt instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::SGT,
			this->module.sgts.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getSGT(const Expr& expr) const -> const SGT& {
		return ReaderAgent(this->module, this->getTargetFunction()).getSGT(expr);
	}


	auto Agent::createUGT(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER, "The @ugt instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::UGT,
			this->module.ugts.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getUGT(const Expr& expr) const -> const UGT& {
		return ReaderAgent(this->module, this->getTargetFunction()).getUGT(expr);
	}


	auto Agent::createFGT(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(this->getExprType(lhs).isFloat(), "The @fgt instruction only supports float values");

		const auto new_expr = Expr(
			Expr::Kind::FGT, this->module.fgts.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getFGT(const Expr& expr) const -> const FGT& {
		return ReaderAgent(this->module, this->getTargetFunction()).getFGT(expr);
	}



	//////////////////////////////////////////////////////////////////////
	// greater than or equal to

	auto Agent::createSGTE(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER, "The @sgte instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::SGTE,
			this->module.sgtes.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getSGTE(const Expr& expr) const -> const SGTE& {
		return ReaderAgent(this->module, this->getTargetFunction()).getSGTE(expr);
	}


	auto Agent::createUGTE(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER, "The @ugte instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::UGTE,
			this->module.ugtes.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getUGTE(const Expr& expr) const -> const UGTE& {
		return ReaderAgent(this->module, this->getTargetFunction()).getUGTE(expr);
	}


	auto Agent::createFGTE(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(this->getExprType(lhs).isFloat(), "The @fgte instruction only supports float values");

		const auto new_expr = Expr(
			Expr::Kind::FGTE, this->module.fgtes.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getFGTE(const Expr& expr) const -> const FGTE& {
		return ReaderAgent(this->module, this->getTargetFunction()).getFGTE(expr);
	}



	//////////////////////////////////////////////////////////////////////
	// bitwise


	auto Agent::createAnd(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER
			|| this->getExprType(lhs).kind() == Type::Kind::BOOL,
			"The @and instruction only supports integers and Bool"
		);

		const auto new_expr = Expr(
			Expr::Kind::AND,
			this->module.ands.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getAnd(const Expr& expr) const -> const And& {
		return ReaderAgent(this->module, this->getTargetFunction()).getAnd(expr);
	}


	auto Agent::createOr(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER
			|| this->getExprType(lhs).kind() == Type::Kind::BOOL,
			"The @or instruction only supports integers and Bool"
		);

		const auto new_expr = Expr(
			Expr::Kind::OR,
			this->module.ors.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getOr(const Expr& expr) const -> const Or& {
		return ReaderAgent(this->module, this->getTargetFunction()).getOr(expr);
	}


	auto Agent::createXor(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER
			|| this->getExprType(lhs).kind() == Type::Kind::BOOL,
			"The @xor instruction only supports integers and Bool"
		);

		const auto new_expr = Expr(
			Expr::Kind::XOR,
			this->module.xors.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getXor(const Expr& expr) const -> const Xor& {
		return ReaderAgent(this->module, this->getTargetFunction()).getXor(expr);
	}


	auto Agent::createSHL(const Expr& lhs, const Expr& rhs, bool nsw, bool nuw, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER, "The @shl instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::SHL,
			this->module.shls.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs, nsw, nuw)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getSHL(const Expr& expr) const -> const SHL& {
		return ReaderAgent(this->module, this->getTargetFunction()).getSHL(expr);
	}


	auto Agent::createSSHLSat(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER, "The @sshlsat instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::SSHL_SAT,
			this->module.sshlsats.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getSSHLSat(const Expr& expr) const -> const SSHLSat& {
		return ReaderAgent(this->module, this->getTargetFunction()).getSSHLSat(expr);
	}


	auto Agent::createUSHLSat(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER, "The @ushlsat instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::USHL_SAT,
			this->module.ushlsats.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getUSHLSat(const Expr& expr) const -> const USHLSat& {
		return ReaderAgent(this->module, this->getTargetFunction()).getUSHLSat(expr);
	}


	auto Agent::createSSHR(const Expr& lhs, const Expr& rhs, bool is_exact, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER, "The @sshr instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::SSHR,
			this->module.sshrs.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs, is_exact)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getSSHR(const Expr& expr) const -> const SSHR& {
		return ReaderAgent(this->module, this->getTargetFunction()).getSSHR(expr);
	}


	auto Agent::createUSHR(const Expr& lhs, const Expr& rhs, bool is_exact, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).kind() == Type::Kind::INTEGER, "The @ushr instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::USHR,
			this->module.ushrs.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs, is_exact)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getUSHR(const Expr& expr) const -> const USHR& {
		return ReaderAgent(this->module, this->getTargetFunction()).getUSHR(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// bit operations

	auto Agent::createBitReverse(const Expr& expr, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(expr.isValue(), "Expr must be value");
		evo::debugAssert(
			this->getExprType(expr).kind() == Type::Kind::INTEGER, "The @bitReverse instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::BIT_REVERSE, this->module.bit_reverses.emplace_back(this->get_stmt_name(std::move(name)), expr)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getBitReverse(const Expr& expr) const -> const BitReverse& {
		return ReaderAgent(this->module, this->getTargetFunction()).getBitReverse(expr);
	}


	auto Agent::createBSwap(const Expr& expr, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(expr.isValue(), "Expr must be value");
		evo::debugAssert(
			this->getExprType(expr).kind() == Type::Kind::INTEGER, "The @bswap instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::BSWAP, this->module.bswaps.emplace_back(this->get_stmt_name(std::move(name)), expr)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getBSwap(const Expr& expr) const -> const BSwap& {
		return ReaderAgent(this->module, this->getTargetFunction()).getBSwap(expr);
	}


	auto Agent::createCtPop(const Expr& expr, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(expr.isValue(), "Expr must be value");
		evo::debugAssert(
			this->getExprType(expr).kind() == Type::Kind::INTEGER, "The @ctPop instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::CTPOP, this->module.ctpops.emplace_back(this->get_stmt_name(std::move(name)), expr)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getCtPop(const Expr& expr) const -> const CtPop& {
		return ReaderAgent(this->module, this->getTargetFunction()).getCtPop(expr);
	}


	auto Agent::createCTLZ(const Expr& expr, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(expr.isValue(), "Expr must be value");
		evo::debugAssert(
			this->getExprType(expr).kind() == Type::Kind::INTEGER, "The @ctlz instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::CTLZ, this->module.ctlzs.emplace_back(this->get_stmt_name(std::move(name)), expr)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getCTLZ(const Expr& expr) const -> const CTLZ& {
		return ReaderAgent(this->module, this->getTargetFunction()).getCTLZ(expr);
	}


	auto Agent::createCTTZ(const Expr& expr, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(expr.isValue(), "Expr must be value");
		evo::debugAssert(
			this->getExprType(expr).kind() == Type::Kind::INTEGER, "The @ctTZ instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::CTTZ, this->module.cttzs.emplace_back(this->get_stmt_name(std::move(name)), expr)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getCTTZ(const Expr& expr) const -> const CTTZ& {
		return ReaderAgent(this->module, this->getTargetFunction()).getCTTZ(expr);
	}



	//////////////////////////////////////////////////////////////////////
	// optimizations

	auto Agent::createLifetimeStart(const Expr& expr, uint64_t size) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			expr.kind() == Expr::Kind::ALLOCA
				|| (expr.kind() == Expr::Kind::PARAM_EXPR && this->getExprType(expr).kind() == Type::Kind::PTR), 
			"Expr must be an alloca or ptr param"
		);

		const auto new_expr = Expr(Expr::Kind::LIFETIME_START, this->module.lifetime_starts.emplace_back(expr, size));
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getLifetimeStart(const Expr& expr) const -> const LifetimeStart& {
		return ReaderAgent(this->module, this->getTargetFunction()).getLifetimeStart(expr);
	}


	auto Agent::createLifetimeEnd(const Expr& expr, uint64_t size) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			expr.kind() == Expr::Kind::ALLOCA
				|| (expr.kind() == Expr::Kind::PARAM_EXPR && this->getExprType(expr).kind() == Type::Kind::PTR), 
			"Expr must be an alloca or ptr param"
		);

		const auto new_expr = Expr(Expr::Kind::LIFETIME_END, this->module.lifetime_ends.emplace_back(expr, size));
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getLifetimeEnd(const Expr& expr) const -> const LifetimeEnd& {
		return ReaderAgent(this->module, this->getTargetFunction()).getLifetimeEnd(expr);
	}






	//////////////////////////////////////////////////////////////////////
	// internal


	auto Agent::insert_stmt(const Expr& stmt) const -> void {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");

		if(this->getInsertIndexAtEnd()){
			this->target_basic_block->append(stmt);
		}else{
			this->target_basic_block->insert(stmt, this->insert_index);
			this->insert_index += 1;
		}
	}


	auto Agent::delete_expr(const Expr& expr) const -> void {
		evo::debugAssert(this->hasTargetFunction(), "Not target function is set");

		switch(expr.kind()){
			break; case Expr::Kind::NONE:              evo::debugFatalBreak("Invalid expr");
			break; case Expr::Kind::GLOBAL_VALUE:      return;
			break; case Expr::Kind::FUNCTION_POINTER:  return;
			break; case Expr::Kind::NUMBER:            this->module.numbers.erase(expr.index);
			break; case Expr::Kind::BOOLEAN:           return;
			break; case Expr::Kind::NULLPTR:           return;
			break; case Expr::Kind::PARAM_EXPR:        return;
			break; case Expr::Kind::CALL:              this->module.calls.erase(expr.index);
			break; case Expr::Kind::CALL_VOID:         this->module.call_voids.erase(expr.index);
			break; case Expr::Kind::ABORT:             return;
			break; case Expr::Kind::BREAKPOINT:        return;
			break; case Expr::Kind::RET:               this->module.rets.erase(expr.index);
			break; case Expr::Kind::JUMP:              return;
			break; case Expr::Kind::BRANCH:            this->module.branches.erase(expr.index);
			break; case Expr::Kind::UNREACHABLE:       return;
			break; case Expr::Kind::PHI:               this->module.phis.erase(expr.index);
			break; case Expr::Kind::SWITCH:            return;
			break; case Expr::Kind::ALLOCA:            this->target_func->allocas.erase(expr.index);
			break; case Expr::Kind::LOAD:              this->module.loads.erase(expr.index);
			break; case Expr::Kind::STORE:             this->module.stores.erase(expr.index);
			break; case Expr::Kind::CALC_PTR:          this->module.calc_ptrs.erase(expr.index);
			break; case Expr::Kind::MEMCPY:            this->module.memcpys.erase(expr.index);
			break; case Expr::Kind::MEMSET:            this->module.memsets.erase(expr.index);
			break; case Expr::Kind::BIT_CAST:          this->module.bitcasts.erase(expr.index);
			break; case Expr::Kind::TRUNC:             this->module.truncs.erase(expr.index);
			break; case Expr::Kind::FTRUNC:            this->module.ftruncs.erase(expr.index);
			break; case Expr::Kind::SEXT:              this->module.sexts.erase(expr.index);
			break; case Expr::Kind::ZEXT:              this->module.zexts.erase(expr.index);
			break; case Expr::Kind::FEXT:              this->module.fexts.erase(expr.index);
			break; case Expr::Kind::ITOF:              this->module.itofs.erase(expr.index);
			break; case Expr::Kind::UITOF:             this->module.uitofs.erase(expr.index);
			break; case Expr::Kind::FTOI:              this->module.ftois.erase(expr.index);
			break; case Expr::Kind::FTOUI:             this->module.ftouis.erase(expr.index);
			break; case Expr::Kind::ADD:               this->module.adds.erase(expr.index);
			break; case Expr::Kind::SADD_WRAP:         this->module.sadd_wraps.erase(expr.index);
			break; case Expr::Kind::SADD_WRAP_RESULT:  return;
			break; case Expr::Kind::SADD_WRAP_WRAPPED: return;
			break; case Expr::Kind::UADD_WRAP:         this->module.uadd_wraps.erase(expr.index);
			break; case Expr::Kind::UADD_WRAP_RESULT:  return;
			break; case Expr::Kind::UADD_WRAP_WRAPPED: return;
			break; case Expr::Kind::SADD_SAT:          this->module.sadd_sats.erase(expr.index);
			break; case Expr::Kind::UADD_SAT:          this->module.uadd_sats.erase(expr.index);
			break; case Expr::Kind::FADD:              this->module.fadds.erase(expr.index);
			break; case Expr::Kind::SUB:               this->module.subs.erase(expr.index);
			break; case Expr::Kind::SSUB_WRAP:         this->module.ssub_wraps.erase(expr.index);
			break; case Expr::Kind::SSUB_WRAP_RESULT:  return;
			break; case Expr::Kind::SSUB_WRAP_WRAPPED: return;
			break; case Expr::Kind::USUB_WRAP:         this->module.usub_wraps.erase(expr.index);
			break; case Expr::Kind::USUB_WRAP_RESULT:  return;
			break; case Expr::Kind::USUB_WRAP_WRAPPED: return;
			break; case Expr::Kind::SSUB_SAT:          this->module.ssub_sats.erase(expr.index);
			break; case Expr::Kind::USUB_SAT:          this->module.usub_sats.erase(expr.index);
			break; case Expr::Kind::FSUB:              this->module.fsubs.erase(expr.index);
			break; case Expr::Kind::MUL:               this->module.muls.erase(expr.index);
			break; case Expr::Kind::SMUL_WRAP:         this->module.smul_wraps.erase(expr.index);
			break; case Expr::Kind::SMUL_WRAP_RESULT:  return;
			break; case Expr::Kind::SMUL_WRAP_WRAPPED: return;
			break; case Expr::Kind::UMUL_WRAP:         this->module.umul_wraps.erase(expr.index);
			break; case Expr::Kind::UMUL_WRAP_RESULT:  return;
			break; case Expr::Kind::UMUL_WRAP_WRAPPED: return;
			break; case Expr::Kind::SMUL_SAT:          this->module.smul_sats.erase(expr.index);
			break; case Expr::Kind::UMUL_SAT:          this->module.umul_sats.erase(expr.index);
			break; case Expr::Kind::FMUL:              this->module.fmuls.erase(expr.index);
			break; case Expr::Kind::SDIV:              this->module.sdivs.erase(expr.index);
			break; case Expr::Kind::UDIV:              this->module.udivs.erase(expr.index);
			break; case Expr::Kind::FDIV:              this->module.fdivs.erase(expr.index);
			break; case Expr::Kind::SREM:              this->module.srems.erase(expr.index);
			break; case Expr::Kind::UREM:              this->module.urems.erase(expr.index);
			break; case Expr::Kind::FREM:              this->module.frems.erase(expr.index);
			break; case Expr::Kind::FNEG:              this->module.fnegs.erase(expr.index);
			break; case Expr::Kind::IEQ:               this->module.ieqs.erase(expr.index);
			break; case Expr::Kind::FEQ:               this->module.feqs.erase(expr.index);
			break; case Expr::Kind::INEQ:              this->module.ineqs.erase(expr.index);
			break; case Expr::Kind::FNEQ:              this->module.fneqs.erase(expr.index);
			break; case Expr::Kind::SLT:               this->module.slts.erase(expr.index);
			break; case Expr::Kind::ULT:               this->module.ults.erase(expr.index);
			break; case Expr::Kind::FLT:               this->module.flts.erase(expr.index);
			break; case Expr::Kind::SLTE:              this->module.sltes.erase(expr.index);
			break; case Expr::Kind::ULTE:              this->module.ultes.erase(expr.index);
			break; case Expr::Kind::FLTE:              this->module.fltes.erase(expr.index);
			break; case Expr::Kind::SGT:               this->module.sgts.erase(expr.index);
			break; case Expr::Kind::UGT:               this->module.ugts.erase(expr.index);
			break; case Expr::Kind::FGT:               this->module.fgts.erase(expr.index);
			break; case Expr::Kind::SGTE:              this->module.sgtes.erase(expr.index);
			break; case Expr::Kind::UGTE:              this->module.ugtes.erase(expr.index);
			break; case Expr::Kind::FGTE:              this->module.fgtes.erase(expr.index);
			break; case Expr::Kind::AND:               this->module.ands.erase(expr.index);
			break; case Expr::Kind::OR:                this->module.ors.erase(expr.index);
			break; case Expr::Kind::XOR:               this->module.xors.erase(expr.index);
			break; case Expr::Kind::SHL:               this->module.shls.erase(expr.index);
			break; case Expr::Kind::SSHL_SAT:          this->module.sshlsats.erase(expr.index);
			break; case Expr::Kind::USHL_SAT:          this->module.ushlsats.erase(expr.index);
			break; case Expr::Kind::SSHR:              this->module.sshrs.erase(expr.index);
			break; case Expr::Kind::USHR:              this->module.ushrs.erase(expr.index);
			break; case Expr::Kind::BIT_REVERSE:       this->module.bit_reverses.erase(expr.index);
			break; case Expr::Kind::BSWAP:             this->module.bswaps.erase(expr.index);
			break; case Expr::Kind::CTPOP:             this->module.ctpops.erase(expr.index);
			break; case Expr::Kind::CTLZ:              this->module.ctlzs.erase(expr.index);
			break; case Expr::Kind::CTTZ:              this->module.cttzs.erase(expr.index);
			break; case Expr::Kind::LIFETIME_START:    this->module.lifetime_starts.erase(expr.index);
			break; case Expr::Kind::LIFETIME_END:      this->module.lifetime_ends.erase(expr.index);
		}

		if(this->getInsertIndexAtEnd() == false){
			this->insert_index -= 1;
		}
	}



	auto Agent::name_exists_in_func(std::string_view name) const -> bool {
		evo::debugAssert(this->hasTargetFunction(), "Not target function is set");

		for(const Parameter& param : this->target_func->getParameters()){
			if(param.getName() == name){ return true; }
		}

		for(const Alloca& alloca : this->target_func->getAllocasRange()){
			if(alloca.name == name){ return true; }
		}

		for(BasicBlock::ID basic_block_id : *this->target_func){
			const BasicBlock& basic_block = this->getBasicBlock(basic_block_id);
			if(basic_block.getName() == name){ return true; }

			for(const Expr& stmt : basic_block){
				switch(stmt.kind()){
					case Expr::Kind::NONE:             evo::debugFatalBreak("Invalid expr");
					case Expr::Kind::GLOBAL_VALUE:     continue;
					case Expr::Kind::FUNCTION_POINTER: continue;
					case Expr::Kind::NUMBER:           continue;
					case Expr::Kind::BOOLEAN:          continue;
					case Expr::Kind::NULLPTR:          continue;
					case Expr::Kind::PARAM_EXPR:       continue;
					case Expr::Kind::CALL:             if(this->getCall(stmt).name == name){ return true; } continue;
					case Expr::Kind::CALL_VOID:        continue;
					case Expr::Kind::ABORT:            continue;
					case Expr::Kind::BREAKPOINT:       continue;
					case Expr::Kind::RET:              continue;
					case Expr::Kind::JUMP:             continue;
					case Expr::Kind::BRANCH:           continue;
					case Expr::Kind::UNREACHABLE:      continue;
					case Expr::Kind::PHI:              if(this->getPhi(stmt).name == name){ return true; } continue;
					case Expr::Kind::SWITCH:           continue;
					case Expr::Kind::ALLOCA:           if(this->getAlloca(stmt).name == name){ return true; } continue;
					case Expr::Kind::LOAD:             if(this->getLoad(stmt).name == name){ return true; } continue;
					case Expr::Kind::STORE:            continue;
					case Expr::Kind::CALC_PTR:         if(this->getCalcPtr(stmt).name == name){ return true; } continue;
					case Expr::Kind::MEMCPY:           continue;
					case Expr::Kind::MEMSET:           continue;
					case Expr::Kind::BIT_CAST:         if(this->getBitCast(stmt).name == name){ return true; } continue;
					case Expr::Kind::TRUNC:            if(this->getTrunc(stmt).name == name){ return true; } continue;
					case Expr::Kind::FTRUNC:           if(this->getFTrunc(stmt).name == name){ return true; } continue;
					case Expr::Kind::SEXT:             if(this->getSExt(stmt).name == name){ return true; } continue;
					case Expr::Kind::ZEXT:             if(this->getZExt(stmt).name == name){ return true; } continue;
					case Expr::Kind::FEXT:             if(this->getFExt(stmt).name == name){ return true; } continue;
					case Expr::Kind::ITOF:             if(this->getIToF(stmt).name == name){ return true; } continue;
					case Expr::Kind::UITOF:            if(this->getUIToF(stmt).name == name){ return true; } continue;
					case Expr::Kind::FTOI:             if(this->getFToI(stmt).name == name){ return true; } continue;
					case Expr::Kind::FTOUI:            if(this->getFToUI(stmt).name == name){ return true; } continue;
					case Expr::Kind::ADD:              if(this->getAdd(stmt).name == name){ return true; } continue;
					case Expr::Kind::SADD_WRAP: {
						const SAddWrap& sadd_wrap = this->getSAddWrap(stmt);
						if(sadd_wrap.resultName == name){ return true; }
						if(sadd_wrap.wrappedName == name){ return true; }
						continue;
					} break;
					case Expr::Kind::SADD_WRAP_RESULT:  continue;
					case Expr::Kind::SADD_WRAP_WRAPPED: continue;
					case Expr::Kind::UADD_WRAP: {
						const UAddWrap& uadd_wrap = this->getUAddWrap(stmt);
						if(uadd_wrap.resultName == name){ return true; }
						if(uadd_wrap.wrappedName == name){ return true; }
						continue;
					} break;
					case Expr::Kind::UADD_WRAP_RESULT:  continue;
					case Expr::Kind::UADD_WRAP_WRAPPED: continue;
					case Expr::Kind::SADD_SAT: if(this->getSAddSat(stmt).name == name){ return true; } continue;
					case Expr::Kind::UADD_SAT: if(this->getUAddSat(stmt).name == name){ return true; } continue;
					case Expr::Kind::FADD:     if(this->getFAdd(stmt).name == name){    return true; } continue;
					case Expr::Kind::SUB:      if(this->getSub(stmt).name == name){     return true; } continue;
					case Expr::Kind::SSUB_WRAP: {
						const SSubWrap& ssub_wrap = this->getSSubWrap(stmt);
						if(ssub_wrap.resultName == name){ return true; }
						if(ssub_wrap.wrappedName == name){ return true; }
						continue;
					} break;
					case Expr::Kind::SSUB_WRAP_RESULT:  continue;
					case Expr::Kind::SSUB_WRAP_WRAPPED: continue;
					case Expr::Kind::USUB_WRAP: {
						const USubWrap& usub_wrap = this->getUSubWrap(stmt);
						if(usub_wrap.resultName == name){ return true; }
						if(usub_wrap.wrappedName == name){ return true; }
						continue;
					} break;
					case Expr::Kind::USUB_WRAP_RESULT:  continue;
					case Expr::Kind::USUB_WRAP_WRAPPED: continue;
					case Expr::Kind::SSUB_SAT: if(this->getSSubSat(stmt).name == name){ return true; } continue;
					case Expr::Kind::USUB_SAT: if(this->getUSubSat(stmt).name == name){ return true; } continue;
					case Expr::Kind::FSUB:     if(this->getFSub(stmt).name == name){    return true; } continue;
					case Expr::Kind::MUL:      if(this->getMul(stmt).name == name){     return true; } continue;
					case Expr::Kind::SMUL_WRAP: {
						const SMulWrap& smul_wrap = this->getSMulWrap(stmt);
						if(smul_wrap.resultName == name){ return true; }
						if(smul_wrap.wrappedName == name){ return true; }
						continue;
					} break;
					case Expr::Kind::SMUL_WRAP_RESULT:  continue;
					case Expr::Kind::SMUL_WRAP_WRAPPED: continue;
					case Expr::Kind::UMUL_WRAP: {
						const UMulWrap& umul_wrap = this->getUMulWrap(stmt);
						if(umul_wrap.resultName == name){ return true; }
						if(umul_wrap.wrappedName == name){ return true; }
						continue;
					} break;
					case Expr::Kind::UMUL_WRAP_RESULT:  continue;
					case Expr::Kind::UMUL_WRAP_WRAPPED: continue;
					case Expr::Kind::SMUL_SAT:    if(this->getSMulSat(stmt).name == name){    return true; } continue;
					case Expr::Kind::UMUL_SAT:    if(this->getUMulSat(stmt).name == name){    return true; } continue;
					case Expr::Kind::FMUL:        if(this->getFMul(stmt).name == name){       return true; } continue;
					case Expr::Kind::SDIV:        if(this->getSDiv(stmt).name == name){       return true; } continue;
					case Expr::Kind::UDIV:        if(this->getUDiv(stmt).name == name){       return true; } continue;
					case Expr::Kind::FDIV:        if(this->getFDiv(stmt).name == name){       return true; } continue;
					case Expr::Kind::SREM:        if(this->getSRem(stmt).name == name){       return true; } continue;
					case Expr::Kind::UREM:        if(this->getURem(stmt).name == name){       return true; } continue;
					case Expr::Kind::FREM:        if(this->getFRem(stmt).name == name){       return true; } continue;
					case Expr::Kind::FNEG:        if(this->getFNeg(stmt).name == name){       return true; } continue;
					case Expr::Kind::IEQ:         if(this->getIEq(stmt).name == name){        return true; } continue;
					case Expr::Kind::FEQ:         if(this->getFEq(stmt).name == name){        return true; } continue;
					case Expr::Kind::INEQ:        if(this->getINeq(stmt).name == name){       return true; } continue;
					case Expr::Kind::FNEQ:        if(this->getFNeq(stmt).name == name){       return true; } continue;
					case Expr::Kind::SLT:         if(this->getSLT(stmt).name == name){        return true; } continue;
					case Expr::Kind::ULT:         if(this->getULT(stmt).name == name){        return true; } continue;
					case Expr::Kind::FLT:         if(this->getFLT(stmt).name == name){        return true; } continue;
					case Expr::Kind::SLTE:        if(this->getSLTE(stmt).name == name){       return true; } continue;
					case Expr::Kind::ULTE:        if(this->getULTE(stmt).name == name){       return true; } continue;
					case Expr::Kind::FLTE:        if(this->getFLTE(stmt).name == name){       return true; } continue;
					case Expr::Kind::SGT:         if(this->getSGT(stmt).name == name){        return true; } continue;
					case Expr::Kind::UGT:         if(this->getUGT(stmt).name == name){        return true; } continue;
					case Expr::Kind::FGT:         if(this->getFGT(stmt).name == name){        return true; } continue;
					case Expr::Kind::SGTE:        if(this->getSGTE(stmt).name == name){       return true; } continue;
					case Expr::Kind::UGTE:        if(this->getUGTE(stmt).name == name){       return true; } continue;
					case Expr::Kind::FGTE:        if(this->getFGTE(stmt).name == name){       return true; } continue;
					case Expr::Kind::AND:         if(this->getAnd(stmt).name == name){        return true; } continue;
					case Expr::Kind::OR:          if(this->getOr(stmt).name == name){         return true; } continue;
					case Expr::Kind::XOR:         if(this->getXor(stmt).name == name){        return true; } continue;
					case Expr::Kind::SHL:         if(this->getSHL(stmt).name == name){        return true; } continue;
					case Expr::Kind::SSHL_SAT:    if(this->getSSHLSat(stmt).name == name){    return true; } continue;
					case Expr::Kind::USHL_SAT:    if(this->getUSHLSat(stmt).name == name){    return true; } continue;
					case Expr::Kind::SSHR:        if(this->getSSHR(stmt).name == name){       return true; } continue;
					case Expr::Kind::USHR:        if(this->getUSHR(stmt).name == name){       return true; } continue;
					case Expr::Kind::BIT_REVERSE: if(this->getBitReverse(stmt).name == name){ return true; } continue;
					case Expr::Kind::BSWAP:       if(this->getBSwap(stmt).name == name){      return true; } continue;
					case Expr::Kind::CTPOP:       if(this->getCtPop(stmt).name == name){      return true; } continue;
					case Expr::Kind::CTLZ:        if(this->getCTLZ(stmt).name == name){       return true; } continue;
					case Expr::Kind::CTTZ:        if(this->getCTTZ(stmt).name == name){       return true; } continue;
					case Expr::Kind::LIFETIME_START: continue;
					case Expr::Kind::LIFETIME_END:   continue;
				}
			}
		}

		return false;
	}


	auto Agent::get_stmt_name(std::string&& name) const -> std::string {
		return this->get_stmt_name_with_forward_include(std::move(name), {});
	}


	auto Agent::get_stmt_name_with_forward_include(
		std::string&& name, evo::ArrayProxy<std::string_view> forward_includes
	) const -> std::string {
		evo::debugAssert(this->hasTargetFunction(), "Not target function is set");
		evo::debugAssert(name.empty() || isStandardName(name), "Not a valid stmt name ({})", name);

		unsigned suffix_num = 1;
		std::string converted_name = [&](){
			if(name.empty()){
				suffix_num += 1;
				return std::format("{}.1", name);
			}else{
				return name;
			}
		}();

		auto name_exists_in_forward_includes = [&](const std::string& name_check) -> bool {
			for(std::string_view forward_include : forward_includes){
				if(name_check == forward_include){ return true; }
			}

			return false;
		};

		while(this->name_exists_in_func(converted_name) || name_exists_in_forward_includes(converted_name)){
			converted_name = std::format("{}.{}", name, suffix_num);
			suffix_num += 1;
		}

		return converted_name;
	}


}