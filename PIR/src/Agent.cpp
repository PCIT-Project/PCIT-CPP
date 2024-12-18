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
		// TODO: check that block is in function

		this->target_basic_block = &this->getBasicBlock(id);
		this->insert_index = std::numeric_limits<size_t>::max();
	}

	auto Agent::setTargetBasicBlock(BasicBlock& basic_block) -> void {
		evo::debugAssert(this->hasTargetFunction(), "No target function is set");
		// TODO: check that block is in function

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
		// TODO: check that index is in block

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

				switch(stmt.getKind()){
					case Expr::Kind::None: evo::debugFatalBreak("Invalid stmt");
					
					case Expr::Kind::GlobalValue: continue;
					case Expr::Kind::Number:      continue;
					case Expr::Kind::Boolean:     continue;
					case Expr::Kind::ParamExpr:   continue;
					
					case Expr::Kind::Call: {
						Call& call_inst = this->module.calls[stmt.index];

						if(call_inst.target.is<PtrCall>() && call_inst.target.as<PtrCall>().location == original){
							call_inst.target.as<PtrCall>().location = replacement;
						}

						for(Expr& arg : call_inst.args){
							if(arg == original){ arg = replacement; }
						}
					} break;
					
					case Expr::Kind::CallVoid: {
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

					case Expr::Kind::Breakpoint: continue;
	
					case Expr::Kind::Ret: {
						Ret& ret_inst = this->module.rets[stmt.index];

						if(ret_inst.value.has_value() && *ret_inst.value == original){
							ret_inst.value.emplace(replacement);
						}
					} break;
					
					case Expr::Kind::Branch: continue;

					case Expr::Kind::CondBranch: {
						CondBranch& cond_branch = this->module.cond_branches[stmt.index];

						if(cond_branch.cond == original){ cond_branch.cond = replacement; }
					} break;

					case Expr::Kind::Unreachable: continue;
					case Expr::Kind::Alloca:      continue;

					case Expr::Kind::Load: {
						Load& load = this->module.loads[stmt.index];

						if(load.source == original){ load.source = replacement; }
					} break;

					case Expr::Kind::Store: {
						Store& store = this->module.stores[stmt.index];

						if(store.destination == original){ store.destination = replacement; }
						if(store.value == original){ store.value = replacement; }
					} break;

					case Expr::Kind::CalcPtr: {
						CalcPtr& calc_ptr = this->module.calc_ptrs[stmt.index];

						if(calc_ptr.basePtr == original){
							calc_ptr.basePtr = replacement;
						}else{
							for(CalcPtr::Index& index : calc_ptr.indices){
								if(index.is<Expr>() && index.as<Expr>() == original){ index.as<Expr>() = replacement; }
							}
						}
					} break;

					case Expr::Kind::BitCast: {
						BitCast& bitcast = this->module.bitcasts[stmt.index];
						if(bitcast.fromValue == original){ bitcast.fromValue = replacement; }
					} break;

					case Expr::Kind::Trunc: {
						Trunc& trunc = this->module.truncs[stmt.index];
						if(trunc.fromValue == original){ trunc.fromValue = replacement; }
					} break;

					case Expr::Kind::FTrunc: {
						FTrunc& ftrunc = this->module.ftruncs[stmt.index];
						if(ftrunc.fromValue == original){ ftrunc.fromValue = replacement; }
					} break;

					case Expr::Kind::SExt: {
						SExt& sext = this->module.sexts[stmt.index];
						if(sext.fromValue == original){ sext.fromValue = replacement; }
					} break;

					case Expr::Kind::ZExt: {
						ZExt& zext = this->module.zexts[stmt.index];
						if(zext.fromValue == original){ zext.fromValue = replacement; }
					} break;

					case Expr::Kind::FExt: {
						FExt& fext = this->module.fexts[stmt.index];
						if(fext.fromValue == original){ fext.fromValue = replacement; }
					} break;

					case Expr::Kind::IToF: {
						IToF& itof = this->module.itofs[stmt.index];
						if(itof.fromValue == original){ itof.fromValue = replacement; }
					} break;

					case Expr::Kind::UIToF: {
						UIToF& uitof = this->module.uitofs[stmt.index];
						if(uitof.fromValue == original){ uitof.fromValue = replacement; }
					} break;

					case Expr::Kind::FToI: {
						FToI& ftoi = this->module.ftois[stmt.index];
						if(ftoi.fromValue == original){ ftoi.fromValue = replacement; }
					} break;

					case Expr::Kind::FToUI: {
						FToUI& ftoui = this->module.ftouis[stmt.index];
						if(ftoui.fromValue == original){ ftoui.fromValue = replacement; }
					} break;

					
					case Expr::Kind::Add: {
						Add& add = this->module.adds[stmt.index];

						if(add.lhs == original){ add.lhs = replacement; }
						if(add.rhs == original){ add.rhs = replacement; }
					} break;

					case Expr::Kind::SAddWrap: {
						SAddWrap& sadd_wrap = this->module.sadd_wraps[stmt.index];

						if(sadd_wrap.lhs == original){ sadd_wrap.lhs = replacement; }
						if(sadd_wrap.rhs == original){ sadd_wrap.rhs = replacement; }
					} break;

					case Expr::Kind::SAddWrapResult: continue;
					case Expr::Kind::SAddWrapWrapped: continue;

					case Expr::Kind::UAddWrap: {
						UAddWrap& uadd_wrap = this->module.uadd_wraps[stmt.index];

						if(uadd_wrap.lhs == original){ uadd_wrap.lhs = replacement; }
						if(uadd_wrap.rhs == original){ uadd_wrap.rhs = replacement; }
					} break;

					case Expr::Kind::UAddWrapResult: continue;
					case Expr::Kind::UAddWrapWrapped: continue;

					case Expr::Kind::SAddSat: {
						SAddSat& sadd_wrap = this->module.sadd_sats[stmt.index];

						if(sadd_wrap.lhs == original){ sadd_wrap.lhs = replacement; }
						if(sadd_wrap.rhs == original){ sadd_wrap.rhs = replacement; }
					} break;

					case Expr::Kind::UAddSat: {
						UAddSat& uadd_wrap = this->module.uadd_sats[stmt.index];

						if(uadd_wrap.lhs == original){ uadd_wrap.lhs = replacement; }
						if(uadd_wrap.rhs == original){ uadd_wrap.rhs = replacement; }
					} break;

					case Expr::Kind::FAdd: {
						FAdd& fadd = this->module.fadds[stmt.index];

						if(fadd.lhs == original){ fadd.lhs = replacement; }
						if(fadd.rhs == original){ fadd.rhs = replacement; }
					} break;


					case Expr::Kind::Sub: {
						Sub& sub = this->module.subs[stmt.index];

						if(sub.lhs == original){ sub.lhs = replacement; }
						if(sub.rhs == original){ sub.rhs = replacement; }
					} break;

					case Expr::Kind::SSubWrap: {
						SSubWrap& ssub_wrap = this->module.ssub_wraps[stmt.index];

						if(ssub_wrap.lhs == original){ ssub_wrap.lhs = replacement; }
						if(ssub_wrap.rhs == original){ ssub_wrap.rhs = replacement; }
					} break;

					case Expr::Kind::SSubWrapResult: continue;
					case Expr::Kind::SSubWrapWrapped: continue;

					case Expr::Kind::USubWrap: {
						USubWrap& usub_wrap = this->module.usub_wraps[stmt.index];

						if(usub_wrap.lhs == original){ usub_wrap.lhs = replacement; }
						if(usub_wrap.rhs == original){ usub_wrap.rhs = replacement; }
					} break;

					case Expr::Kind::USubWrapResult: continue;
					case Expr::Kind::USubWrapWrapped: continue;

					case Expr::Kind::SSubSat: {
						SSubSat& ssub_sat = this->module.ssub_sats[stmt.index];

						if(ssub_sat.lhs == original){ ssub_sat.lhs = replacement; }
						if(ssub_sat.rhs == original){ ssub_sat.rhs = replacement; }
					} break;

					case Expr::Kind::USubSat: {
						USubSat& usub_sat = this->module.usub_sats[stmt.index];

						if(usub_sat.lhs == original){ usub_sat.lhs = replacement; }
						if(usub_sat.rhs == original){ usub_sat.rhs = replacement; }
					} break;

					case Expr::Kind::FSub: {
						FSub& fsub = this->module.fsubs[stmt.index];

						if(fsub.lhs == original){ fsub.lhs = replacement; }
						if(fsub.rhs == original){ fsub.rhs = replacement; }
					} break;


					case Expr::Kind::Mul: {
						Mul& mul = this->module.muls[stmt.index];

						if(mul.lhs == original){ mul.lhs = replacement; }
						if(mul.rhs == original){ mul.rhs = replacement; }
					} break;

					case Expr::Kind::SMulWrap: {
						SMulWrap& smul_wrap = this->module.smul_wraps[stmt.index];

						if(smul_wrap.lhs == original){ smul_wrap.lhs = replacement; }
						if(smul_wrap.rhs == original){ smul_wrap.rhs = replacement; }
					} break;

					case Expr::Kind::SMulWrapResult: continue;
					case Expr::Kind::SMulWrapWrapped: continue;

					case Expr::Kind::UMulWrap: {
						UMulWrap& umul_wrap = this->module.umul_wraps[stmt.index];

						if(umul_wrap.lhs == original){ umul_wrap.lhs = replacement; }
						if(umul_wrap.rhs == original){ umul_wrap.rhs = replacement; }
					} break;

					case Expr::Kind::UMulWrapResult: continue;
					case Expr::Kind::UMulWrapWrapped: continue;

					case Expr::Kind::SMulSat: {
						SMulSat& smul_sat = this->module.smul_sats[stmt.index];

						if(smul_sat.lhs == original){ smul_sat.lhs = replacement; }
						if(smul_sat.rhs == original){ smul_sat.rhs = replacement; }
					} break;

					case Expr::Kind::UMulSat: {
						UMulSat& umul_sat = this->module.umul_sats[stmt.index];

						if(umul_sat.lhs == original){ umul_sat.lhs = replacement; }
						if(umul_sat.rhs == original){ umul_sat.rhs = replacement; }
					} break;

					case Expr::Kind::FMul: {
						FMul& fmul = this->module.fmuls[stmt.index];

						if(fmul.lhs == original){ fmul.lhs = replacement; }
						if(fmul.rhs == original){ fmul.rhs = replacement; }
					} break;

					case Expr::Kind::SDiv: {
						SDiv& sdiv = this->module.sdivs[stmt.index];

						if(sdiv.lhs == original){ sdiv.lhs = replacement; }
						if(sdiv.rhs == original){ sdiv.rhs = replacement; }
					} break;

					case Expr::Kind::UDiv: {
						UDiv& udiv = this->module.udivs[stmt.index];

						if(udiv.lhs == original){ udiv.lhs = replacement; }
						if(udiv.rhs == original){ udiv.rhs = replacement; }
					} break;

					case Expr::Kind::FDiv: {
						FDiv& fdiv = this->module.fdivs[stmt.index];

						if(fdiv.lhs == original){ fdiv.lhs = replacement; }
						if(fdiv.rhs == original){ fdiv.rhs = replacement; }
					} break;

					case Expr::Kind::SRem: {
						SRem& srem = this->module.srems[stmt.index];

						if(srem.lhs == original){ srem.lhs = replacement; }
						if(srem.rhs == original){ srem.rhs = replacement; }
					} break;

					case Expr::Kind::URem: {
						URem& urem = this->module.urems[stmt.index];

						if(urem.lhs == original){ urem.lhs = replacement; }
						if(urem.rhs == original){ urem.rhs = replacement; }
					} break;

					case Expr::Kind::FRem: {
						FRem& frem = this->module.frems[stmt.index];

						if(frem.lhs == original){ frem.lhs = replacement; }
						if(frem.rhs == original){ frem.rhs = replacement; }
					} break;



					case Expr::Kind::IEq: {
						IEq& ieq = this->module.ieqs[stmt.index];

						if(ieq.lhs == original){ ieq.lhs = replacement; }
						if(ieq.rhs == original){ ieq.rhs = replacement; }
					} break;

					case Expr::Kind::FEq: {
						FEq& feq = this->module.feqs[stmt.index];

						if(feq.lhs == original){ feq.lhs = replacement; }
						if(feq.rhs == original){ feq.rhs = replacement; }
					} break;
					
					case Expr::Kind::INeq: {
						INeq& ineq = this->module.ineqs[stmt.index];

						if(ineq.lhs == original){ ineq.lhs = replacement; }
						if(ineq.rhs == original){ ineq.rhs = replacement; }
					} break;

					case Expr::Kind::FNeq: {
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

		if(stmt_to_remove.getKind() != Expr::Kind::Alloca){
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
		return new_block_id;
	}

	auto Agent::getBasicBlock(BasicBlock::ID id) const -> BasicBlock& {
		return this->module.basic_blocks[id];
	}


	//////////////////////////////////////////////////////////////////////
	// numbers

	auto Agent::createNumber(const Type& type, core::GenericInt&& value) const -> Expr {
		evo::debugAssert(type.isNumeric(), "Number type must be numeric");
		evo::debugAssert(
			type.getKind() == Type::Kind::Integer, "Type and value must both be integer or both be floating"
		);

		return Expr(Expr::Kind::Number, this->module.numbers.emplace_back(type, std::move(value)));
	}

	auto Agent::createNumber(const Type& type, const core::GenericInt& value) const -> Expr {
		evo::debugAssert(type.isNumeric(), "Number type must be numeric");
		evo::debugAssert(
			type.getKind() == Type::Kind::Integer, "Type and value must both be integer or both be floating"
		);

		return Expr(Expr::Kind::Number, this->module.numbers.emplace_back(type, value));
	}

	auto Agent::createNumber(const Type& type, core::GenericFloat&& value) const -> Expr {
		evo::debugAssert(type.isNumeric(), "Number type must be numeric");
		evo::debugAssert(type.isFloat(), "Type and value must both be integer or both be floating");

		return Expr(Expr::Kind::Number, this->module.numbers.emplace_back(type, std::move(value)));
	}

	auto Agent::createNumber(const Type& type, const core::GenericFloat& value) const -> Expr {
		evo::debugAssert(type.isNumeric(), "Number type must be numeric");
		evo::debugAssert(type.isFloat(), "Type and value must both be integer or both be floating");

		return Expr(Expr::Kind::Number, this->module.numbers.emplace_back(type, value));
	}

	auto Agent::getNumber(const Expr& expr) const -> const Number& {
		return ReaderAgent(this->module).getNumber(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// booleans

	auto Agent::createBoolean(bool value) -> Expr {
		return Expr(Expr::Kind::Boolean, uint32_t(value));
	}

	auto Agent::getBoolean(const Expr& expr) -> bool {
		return ReaderAgent::getBoolean(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// param exprs

	auto Agent::createParamExpr(uint32_t index) -> Expr {
		return Expr(Expr::Kind::ParamExpr, index);
	}

	auto Agent::getParamExpr(const Expr& expr) -> ParamExpr {
		return ReaderAgent::getParamExpr(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// global values (expr)

	auto Agent::createGlobalValue(const GlobalVar::ID& global_id) -> Expr {
		return Expr(Expr::Kind::GlobalValue, global_id.get());
	}

	auto Agent::getGlobalValue(const Expr& expr) const -> const GlobalVar& {
		return ReaderAgent(this->module).getGlobalValue(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// calls

	auto Agent::createCall(Function::ID func, evo::SmallVector<Expr>&& args, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunction(func).getReturnType().getKind() != Type::Kind::Void,
			"Call cannot return `Void` (did you mean CallVoid?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::Call,
			this->module.calls.emplace_back(this->get_stmt_name(std::move(name)), func, std::move(args))
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::createCall(Function::ID func, const evo::SmallVector<Expr>& args, std::string&& name) const
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunction(func).getReturnType().getKind() != Type::Kind::Void,
			"Call cannot return `Void` (did you mean CallVoid?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::Call,
			this->module.calls.emplace_back(this->get_stmt_name(std::move(name)), func, args)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}


	auto Agent::createCall(FunctionDecl::ID func, evo::SmallVector<Expr>&& args, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunctionDecl(func).returnType.getKind() != Type::Kind::Void,
			"Call cannot return `Void` (did you mean CallVoid?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::Call,
			this->module.calls.emplace_back(this->get_stmt_name(std::move(name)), func, std::move(args))
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::createCall(FunctionDecl::ID func, const evo::SmallVector<Expr>& args, std::string&& name) const
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunctionDecl(func).returnType.getKind() != Type::Kind::Void,
			"Call cannot return `Void` (did you mean CallVoid?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::Call,
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
			this->module.getFunctionType(func_type).returnType.getKind() != Type::Kind::Void,
			"Call cannot return `Void` (did you mean CallVoid?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func_type, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::Call,
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
			this->module.getFunctionType(func_type).returnType.getKind() != Type::Kind::Void,
			"Call cannot return `Void` (did you mean CallVoid?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func_type, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::Call,
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
			this->module.getFunction(func).getReturnType().getKind() == Type::Kind::Void,
			"CallVoid must return `Void` (did you mean Call?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::CallVoid, this->module.call_voids.emplace_back(func, std::move(args))
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::createCallVoid(Function::ID func, const evo::SmallVector<Expr>& args) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunction(func).getReturnType().getKind() == Type::Kind::Void,
			"CallVoid must return `Void` (did you mean Call?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(Expr::Kind::CallVoid, this->module.call_voids.emplace_back(func, args));
		this->insert_stmt(new_expr);
		return new_expr;
	}


	auto Agent::createCallVoid(FunctionDecl::ID func, evo::SmallVector<Expr>&& args) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunctionDecl(func).returnType.getKind() == Type::Kind::Void,
			"CallVoid must return `Void` (did you mean Call?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::CallVoid, this->module.call_voids.emplace_back(func, std::move(args))
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::createCallVoid(FunctionDecl::ID func, const evo::SmallVector<Expr>& args) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunctionDecl(func).returnType.getKind() == Type::Kind::Void,
			"CallVoid must return `Void` (did you mean Call?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(Expr::Kind::CallVoid, this->module.call_voids.emplace_back(func, args));
		this->insert_stmt(new_expr);
		return new_expr;
	}


	auto Agent::createCallVoid(const Expr& func, const Type& func_type, evo::SmallVector<Expr>&& args) const
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunctionType(func_type).returnType.getKind() == Type::Kind::Void,
			"CallVoid must return `Void` (did you mean Call?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func_type, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::CallVoid,
			this->module.call_voids.emplace_back(PtrCall(func, func_type), std::move(args))
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::createCallVoid(const Expr& func, const Type& func_type, const evo::SmallVector<Expr>& args) const
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunctionType(func_type).returnType.getKind() == Type::Kind::Void,
			"CallVoid must return `Void` (did you mean Call?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func_type, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::CallVoid, this->module.call_voids.emplace_back(PtrCall(func, func_type), args)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}


	auto Agent::getCallVoid(const Expr& expr) const -> const CallVoid& {
		return ReaderAgent(this->module, this->getTargetFunction()).getCallVoid(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// breakpoint

	auto Agent::createBreakpoint() const -> Expr {
		const auto new_expr = Expr(Expr::Kind::Breakpoint);
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

		const auto new_expr = Expr(Expr::Kind::Ret, this->module.rets.emplace_back(expr));
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::createRet() const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->target_func->getReturnType().getKind() == Type::Kind::Void, "Return type must match"
		);

		const auto new_expr = Expr(Expr::Kind::Ret, this->module.rets.emplace_back(std::nullopt));
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getRet(const Expr& expr) const -> const Ret& {
		return ReaderAgent(this->module, this->getTargetFunction()).getRet(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// branch instructions

	auto Agent::createBranch(BasicBlock::ID basic_block_id) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");

		const auto new_expr = Expr(Expr::Kind::Branch, basic_block_id.get());
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getBranch(const Expr& expr) -> Branch {
		return ReaderAgent::getBranch(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// conditional branch instructions

	auto Agent::createCondBranch(const Expr& cond, BasicBlock::ID then_block, BasicBlock::ID else_block) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(this->getExprType(cond).getKind() == Type::Kind::Bool, "Cond must be of type Bool");

		const auto new_expr = Expr(
			Expr::Kind::CondBranch, this->module.cond_branches.emplace_back(cond, then_block, else_block)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getCondBranch(const Expr& expr) const -> CondBranch {
		return ReaderAgent(this->module, this->getTargetFunction()).getCondBranch(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// unreachable

	auto Agent::createUnreachable() const -> Expr {
		const auto new_expr = Expr(Expr::Kind::Unreachable);
		this->insert_stmt(new_expr);
		return new_expr;
	}


	//////////////////////////////////////////////////////////////////////
	// alloca

	auto Agent::createAlloca(const Type& type, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetFunction(), "No target functions set");

		return Expr(
			Expr::Kind::Alloca, this->target_func->allocas.emplace_back(this->get_stmt_name(std::move(name)), type)
		);
	}

	auto Agent::getAlloca(const Expr& expr) const -> const Alloca& {
		return ReaderAgent(this->module, this->getTargetFunction()).getAlloca(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// load

	auto Agent::createLoad(
		const Expr& source, const Type& type, bool is_volatile, AtomicOrdering atomic_ordering, std::string&& name
	) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(atomic_ordering.isValidForLoad(), "This atomic ordering is not valid for a load");
		evo::debugAssert(this->getExprType(source).getKind() == Type::Kind::Ptr, "Source must be of type Ptr");

		const auto new_stmt = Expr(
			Expr::Kind::Load,
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
			this->getExprType(destination).getKind() == Type::Kind::Ptr, "Destination must be of type Ptr"
		);

		const auto new_stmt = Expr(
			Expr::Kind::Store, this->module.stores.emplace_back(destination, value, is_volatile, atomic_ordering)
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
		evo::debugAssert(this->getExprType(base_ptr).getKind() == Type::Kind::Ptr, "Base ptr must be of type Ptr");
		evo::debugAssert(!indices.empty(), "There must be at least one index");
		#if defined(PCIT_CONFIG_DEBUG)
			Type target_type = ptr_type;
			
			for(size_t i = 1; i < indices.size(); i+=1){
				const CalcPtr::Index& index = indices[i];

				evo::debugAssert(target_type.isAggregate(), "ptr type must be an aggregate type");
				evo::debugAssert(
					index.is<int64_t>() || this->getExprType(index.as<Expr>()).getKind() == Type::Kind::Integer,
					"Index must be integer"
				);

				if(target_type.getKind() == Type::Kind::Array){
					const ArrayType& array_type = this->module.getArrayType(target_type);

					if(index.is<int64_t>()){
						evo::debugAssert(
							index.as<int64_t>() >= 0 && size_t(index.as<int64_t>()) < array_type.length,
							"indexing into an array must be a valid index"
						);
						
					}else if(index.as<Expr>().getKind() == Expr::Kind::Number){
						const int64_t member_index = static_cast<int64_t>(this->getNumber(index.as<Expr>()).getInt());
						evo::debugAssert(
							member_index >= 0 && size_t(member_index) < array_type.length,
							"indexing into an array must be a valid index"
						);
					}

					target_type = array_type.elemType;

				}else{
					evo::debugAssert(index.is<int64_t>(), "Cannot index into a struct with a pcit::pir::Expr");

					const StructType& struct_type = this->module.getStructType(target_type);

					evo::debugAssert(
						index.as<int64_t>() >= 0 && size_t(index.as<int64_t>()) < struct_type.members.size(),
						"indexing into a struct must be a valid member index"
					);
					target_type = struct_type.members[size_t(index.as<int64_t>())];
				}
			}
		#endif

		const auto new_stmt = Expr(
			Expr::Kind::CalcPtr,
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
	// BitCast
	
	EVO_NODISCARD auto Agent::createBitCast(const Expr& fromValue, const Type& toType, std::string&& name) const
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getSize(this->getExprType(fromValue)) == this->module.getSize(toType),
			"Cannot convert to a type of a different size"
		);

		const auto new_expr = Expr(
			Expr::Kind::BitCast,
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
		evo::debugAssert(this->getExprType(fromValue).getKind() == Type::Kind::Integer, "can only convert integers");
		evo::debugAssert(toType.getKind() == Type::Kind::Integer, "can only convert to integers");

		const auto new_expr = Expr(
			Expr::Kind::Trunc,
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
			Expr::Kind::FTrunc,
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
		evo::debugAssert(this->getExprType(fromValue).getKind() == Type::Kind::Integer, "can only convert integers");
		evo::debugAssert(toType.getKind() == Type::Kind::Integer, "can only convert to integers");

		const auto new_expr = Expr(
			Expr::Kind::SExt,
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
		evo::debugAssert(this->getExprType(fromValue).getKind() == Type::Kind::Integer, "can only convert integers");
		evo::debugAssert(toType.getKind() == Type::Kind::Integer, "can only convert to integers");

		const auto new_expr = Expr(
			Expr::Kind::ZExt,
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
			Expr::Kind::FExt,
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
		evo::debugAssert(this->getExprType(fromValue).getKind() == Type::Kind::Integer, "can only convert integers");
		evo::debugAssert(toType.isFloat(), "can only convert to floats");

		const auto new_expr = Expr(
			Expr::Kind::IToF,
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
		evo::debugAssert(this->getExprType(fromValue).getKind() == Type::Kind::Integer, "can only convert integers");
		evo::debugAssert(toType.isFloat(), "can only convert to floats");

		const auto new_expr = Expr(
			Expr::Kind::UIToF,
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
		evo::debugAssert(toType.getKind() == Type::Kind::Integer, "can only convert to integers");

		const auto new_expr = Expr(
			Expr::Kind::FToI,
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
		evo::debugAssert(toType.getKind() == Type::Kind::Integer, "can only convert to integers");

		const auto new_expr = Expr(
			Expr::Kind::FToUI,
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

	auto Agent::createAdd(const Expr& lhs, const Expr& rhs, bool may_wrap, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).getKind() == Type::Kind::Integer, "The @add instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::Add,
			this->module.adds.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs, may_wrap)
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
		evo::debugAssert(this->getExprType(lhs).getKind() == Type::Kind::Integer, "Can only add wrap integers");

		const std::string result_stmt_name = this->get_stmt_name(std::move(result_name));
		const std::string wrapped_stmt_name = this->get_stmt_name_with_forward_include(
			std::move(wrapped_name), {result_stmt_name}
		);

		const auto new_expr = Expr(
			Expr::Kind::SAddWrap,
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
		evo::debugAssert(this->getExprType(lhs).getKind() == Type::Kind::Integer, "Can only add wrap integers");

		const std::string result_stmt_name = this->get_stmt_name(std::move(result_name));
		const std::string wrapped_stmt_name = this->get_stmt_name_with_forward_include(
			std::move(wrapped_name), {result_stmt_name}
		);

		const auto new_expr = Expr(
			Expr::Kind::UAddWrap,
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
			this->getExprType(lhs).getKind() == Type::Kind::Integer, "The @saddSat instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::SAddSat,
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
			this->getExprType(lhs).getKind() == Type::Kind::Integer, "The @uaddSat instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::UAddSat,
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
			Expr::Kind::FAdd, this->module.fadds.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getFAdd(const Expr& expr) const -> const FAdd& {
		return ReaderAgent(this->module, this->getTargetFunction()).getFAdd(expr);
	}



	//////////////////////////////////////////////////////////////////////
	// sub

	auto Agent::createSub(const Expr& lhs, const Expr& rhs, bool may_wrap, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).getKind() == Type::Kind::Integer, "The @sub instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::Sub,
			this->module.subs.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs, may_wrap)
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
		evo::debugAssert(this->getExprType(lhs).getKind() == Type::Kind::Integer, "Can only sub wrap integers");

		const std::string result_stmt_name = this->get_stmt_name(std::move(result_name));
		const std::string wrapped_stmt_name = this->get_stmt_name_with_forward_include(
			std::move(wrapped_name), {result_stmt_name}
		);

		const auto new_expr = Expr(
			Expr::Kind::SSubWrap,
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
		evo::debugAssert(this->getExprType(lhs).getKind() == Type::Kind::Integer, "Can only sub wrap integers");

		const std::string result_stmt_name = this->get_stmt_name(std::move(result_name));
		const std::string wrapped_stmt_name = this->get_stmt_name_with_forward_include(
			std::move(wrapped_name), {result_stmt_name}
		);

		const auto new_expr = Expr(
			Expr::Kind::USubWrap,
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
			this->getExprType(lhs).getKind() == Type::Kind::Integer, "The @ssubSat instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::SSubSat,
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
			this->getExprType(lhs).getKind() == Type::Kind::Integer, "The @usubSat instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::USubSat,
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
			Expr::Kind::FSub, this->module.fsubs.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getFSub(const Expr& expr) const -> const FSub& {
		return ReaderAgent(this->module, this->getTargetFunction()).getFSub(expr);
	}



	//////////////////////////////////////////////////////////////////////
	// mul

	auto Agent::createMul(const Expr& lhs, const Expr& rhs, bool may_wrap, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).getKind() == Type::Kind::Integer, "The @mul instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::Mul,
			this->module.muls.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs, may_wrap)
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
		evo::debugAssert(this->getExprType(lhs).getKind() == Type::Kind::Integer, "Can only mul wrap integers");

		const std::string result_stmt_name = this->get_stmt_name(std::move(result_name));
		const std::string wrapped_stmt_name = this->get_stmt_name_with_forward_include(
			std::move(wrapped_name), {result_stmt_name}
		);

		const auto new_expr = Expr(
			Expr::Kind::SMulWrap,
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
		evo::debugAssert(this->getExprType(lhs).getKind() == Type::Kind::Integer, "Can only mul wrap integers");

		const std::string result_stmt_name = this->get_stmt_name(std::move(result_name));
		const std::string wrapped_stmt_name = this->get_stmt_name_with_forward_include(
			std::move(wrapped_name), {result_stmt_name}
		);

		const auto new_expr = Expr(
			Expr::Kind::UMulWrap,
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
			this->getExprType(lhs).getKind() == Type::Kind::Integer, "The @smulSat instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::SMulSat,
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
			this->getExprType(lhs).getKind() == Type::Kind::Integer, "The @umulSat instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::UMulSat,
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
		evo::debugAssert(this->getExprType(lhs).isFloat(), "The @fMul instruction only supports float values");

		const auto new_expr = Expr(
			Expr::Kind::FMul, this->module.fmuls.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
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
			this->getExprType(lhs).getKind() == Type::Kind::Integer, "The @sdiv instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::SDiv,
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
			this->getExprType(lhs).getKind() == Type::Kind::Integer, "The @udiv instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::UDiv,
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
		evo::debugAssert(this->getExprType(lhs).isFloat(), "The @fMul instruction only supports float values");

		const auto new_expr = Expr(
			Expr::Kind::FDiv, this->module.fdivs.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
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
			this->getExprType(lhs).getKind() == Type::Kind::Integer, "The @srem instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::SRem,
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
			this->getExprType(lhs).getKind() == Type::Kind::Integer, "The @urem instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::URem,
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
		evo::debugAssert(this->getExprType(lhs).isFloat(), "The @fMul instruction only supports float values");

		const auto new_expr = Expr(
			Expr::Kind::FRem, this->module.frems.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getFRem(const Expr& expr) const -> const FRem& {
		return ReaderAgent(this->module, this->getTargetFunction()).getFRem(expr);
	}



	//////////////////////////////////////////////////////////////////////
	// equal

	auto Agent::createIEq(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).getKind() == Type::Kind::Integer, "The @ieq instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::IEq,
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
			Expr::Kind::FEq, this->module.feqs.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
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
			this->getExprType(lhs).getKind() == Type::Kind::Integer, "The @ineq instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::INeq,
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
			Expr::Kind::FNeq, this->module.fneqs.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
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
			this->getExprType(lhs).getKind() == Type::Kind::Integer, "The @slt instruction only supports integers"
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
			this->getExprType(lhs).getKind() == Type::Kind::Integer, "The @ult instruction only supports integers"
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
			this->getExprType(lhs).getKind() == Type::Kind::Integer, "The @slte instruction only supports integers"
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
			this->getExprType(lhs).getKind() == Type::Kind::Integer, "The @ulte instruction only supports integers"
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
			this->getExprType(lhs).getKind() == Type::Kind::Integer, "The @sgt instruction only supports integers"
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
			this->getExprType(lhs).getKind() == Type::Kind::Integer, "The @ugt instruction only supports integers"
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
			this->getExprType(lhs).getKind() == Type::Kind::Integer, "The @sgte instruction only supports integers"
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
			this->getExprType(lhs).getKind() == Type::Kind::Integer, "The @ugte instruction only supports integers"
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

		switch(expr.getKind()){
			break; case Expr::Kind::None:            evo::debugFatalBreak("Invalid expr");
			break; case Expr::Kind::GlobalValue:     return;
			break; case Expr::Kind::Number:          this->module.numbers.erase(expr.index);
			break; case Expr::Kind::Boolean:         return;
			break; case Expr::Kind::ParamExpr:       return;
			break; case Expr::Kind::Call:            this->module.calls.erase(expr.index);
			break; case Expr::Kind::CallVoid:        this->module.call_voids.erase(expr.index);
			break; case Expr::Kind::Breakpoint:      return;
			break; case Expr::Kind::Ret:             this->module.rets.erase(expr.index);
			break; case Expr::Kind::Branch:          return;
			break; case Expr::Kind::CondBranch:      this->module.cond_branches.erase(expr.index);
			break; case Expr::Kind::Unreachable:     return;
			break; case Expr::Kind::Alloca:          this->target_func->allocas.erase(expr.index);
			break; case Expr::Kind::Load:            this->module.loads.erase(expr.index);
			break; case Expr::Kind::Store:           this->module.stores.erase(expr.index);
			break; case Expr::Kind::CalcPtr:         this->module.calc_ptrs.erase(expr.index);
			break; case Expr::Kind::BitCast:         this->module.bitcasts.erase(expr.index);
			break; case Expr::Kind::Trunc:           this->module.truncs.erase(expr.index);
			break; case Expr::Kind::FTrunc:          this->module.ftruncs.erase(expr.index);
			break; case Expr::Kind::SExt:            this->module.sexts.erase(expr.index);
			break; case Expr::Kind::ZExt:            this->module.zexts.erase(expr.index);
			break; case Expr::Kind::FExt:            this->module.fexts.erase(expr.index);
			break; case Expr::Kind::IToF:            this->module.itofs.erase(expr.index);
			break; case Expr::Kind::UIToF:           this->module.uitofs.erase(expr.index);
			break; case Expr::Kind::FToI:            this->module.ftois.erase(expr.index);
			break; case Expr::Kind::FToUI:           this->module.ftouis.erase(expr.index);
			break; case Expr::Kind::Add:             this->module.adds.erase(expr.index);
			break; case Expr::Kind::SAddWrap:        this->module.sadd_wraps.erase(expr.index);
			break; case Expr::Kind::SAddWrapResult:  return;
			break; case Expr::Kind::SAddWrapWrapped: return;
			break; case Expr::Kind::UAddWrap:        this->module.uadd_wraps.erase(expr.index);
			break; case Expr::Kind::UAddWrapResult:  return;
			break; case Expr::Kind::UAddWrapWrapped: return;
			break; case Expr::Kind::SAddSat:         this->module.sadd_sats.erase(expr.index);
			break; case Expr::Kind::UAddSat:         this->module.uadd_sats.erase(expr.index);
			break; case Expr::Kind::FAdd:            this->module.fadds.erase(expr.index);
			break; case Expr::Kind::Sub:             this->module.subs.erase(expr.index);
			break; case Expr::Kind::SSubWrap:        this->module.ssub_wraps.erase(expr.index);
			break; case Expr::Kind::SSubWrapResult:  return;
			break; case Expr::Kind::SSubWrapWrapped: return;
			break; case Expr::Kind::USubWrap:        this->module.usub_wraps.erase(expr.index);
			break; case Expr::Kind::USubWrapResult:  return;
			break; case Expr::Kind::USubWrapWrapped: return;
			break; case Expr::Kind::SSubSat:         this->module.ssub_sats.erase(expr.index);
			break; case Expr::Kind::USubSat:         this->module.usub_sats.erase(expr.index);
			break; case Expr::Kind::FSub:            this->module.fsubs.erase(expr.index);
			break; case Expr::Kind::Mul:             this->module.muls.erase(expr.index);
			break; case Expr::Kind::SMulWrap:        this->module.smul_wraps.erase(expr.index);
			break; case Expr::Kind::SMulWrapResult:  return;
			break; case Expr::Kind::SMulWrapWrapped: return;
			break; case Expr::Kind::UMulWrap:        this->module.umul_wraps.erase(expr.index);
			break; case Expr::Kind::UMulWrapResult:  return;
			break; case Expr::Kind::UMulWrapWrapped: return;
			break; case Expr::Kind::SMulSat:         this->module.smul_sats.erase(expr.index);
			break; case Expr::Kind::UMulSat:         this->module.umul_sats.erase(expr.index);
			break; case Expr::Kind::FMul:            this->module.fmuls.erase(expr.index);
			break; case Expr::Kind::SDiv:            this->module.sdivs.erase(expr.index);
			break; case Expr::Kind::UDiv:            this->module.udivs.erase(expr.index);
			break; case Expr::Kind::FDiv:            this->module.fdivs.erase(expr.index);
			break; case Expr::Kind::SRem:            this->module.srems.erase(expr.index);
			break; case Expr::Kind::URem:            this->module.urems.erase(expr.index);
			break; case Expr::Kind::FRem:            this->module.frems.erase(expr.index);
			break; case Expr::Kind::IEq:             this->module.ieqs.erase(expr.index);
			break; case Expr::Kind::FEq:             this->module.feqs.erase(expr.index);
			break; case Expr::Kind::INeq:            this->module.ineqs.erase(expr.index);
			break; case Expr::Kind::FNeq:            this->module.fneqs.erase(expr.index);
			break; case Expr::Kind::SLT:             this->module.slts.erase(expr.index);
			break; case Expr::Kind::ULT:             this->module.ults.erase(expr.index);
			break; case Expr::Kind::FLT:             this->module.flts.erase(expr.index);
			break; case Expr::Kind::SLTE:            this->module.sltes.erase(expr.index);
			break; case Expr::Kind::ULTE:            this->module.ultes.erase(expr.index);
			break; case Expr::Kind::FLTE:            this->module.fltes.erase(expr.index);
			break; case Expr::Kind::SGT:             this->module.sgts.erase(expr.index);
			break; case Expr::Kind::UGT:             this->module.ugts.erase(expr.index);
			break; case Expr::Kind::FGT:             this->module.fgts.erase(expr.index);
			break; case Expr::Kind::SGTE:            this->module.sgtes.erase(expr.index);
			break; case Expr::Kind::UGTE:            this->module.ugtes.erase(expr.index);
			break; case Expr::Kind::FGTE:            this->module.fgtes.erase(expr.index);
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

		for(BasicBlock::ID basic_block_id : *this->target_func){
			const BasicBlock& basic_block = this->getBasicBlock(basic_block_id);
			if(basic_block.getName() == name){ return true; }

			for(const Expr& stmt : basic_block){
				switch(stmt.getKind()){
					case Expr::Kind::None:        evo::debugFatalBreak("Invalid expr");
					case Expr::Kind::GlobalValue: continue;
					case Expr::Kind::Number:      continue;
					case Expr::Kind::Boolean:     continue;
					case Expr::Kind::ParamExpr:   continue;
					case Expr::Kind::Call:        if(this->getCall(stmt).name == name){ return true; } continue;
					case Expr::Kind::CallVoid:    continue;
					case Expr::Kind::Breakpoint:  continue;
					case Expr::Kind::Ret:         continue;
					case Expr::Kind::Branch:      continue;
					case Expr::Kind::CondBranch:  continue;
					case Expr::Kind::Unreachable: continue;
					case Expr::Kind::Alloca:      if(this->getAlloca(stmt).name == name){ return true; } continue;
					case Expr::Kind::Load:        if(this->getLoad(stmt).name == name){ return true; } continue;
					case Expr::Kind::Store:       continue;
					case Expr::Kind::CalcPtr:     if(this->getCalcPtr(stmt).name == name){ return true; } continue;
					case Expr::Kind::BitCast:     if(this->getBitCast(stmt).name == name){ return true; } continue;
					case Expr::Kind::Trunc:       if(this->getTrunc(stmt).name == name){ return true; } continue;
					case Expr::Kind::FTrunc:      if(this->getFTrunc(stmt).name == name){ return true; } continue;
					case Expr::Kind::SExt:        if(this->getSExt(stmt).name == name){ return true; } continue;
					case Expr::Kind::ZExt:        if(this->getZExt(stmt).name == name){ return true; } continue;
					case Expr::Kind::FExt:        if(this->getFExt(stmt).name == name){ return true; } continue;
					case Expr::Kind::IToF:        if(this->getIToF(stmt).name == name){ return true; } continue;
					case Expr::Kind::UIToF:       if(this->getUIToF(stmt).name == name){ return true; } continue;
					case Expr::Kind::FToI:        if(this->getFToI(stmt).name == name){ return true; } continue;
					case Expr::Kind::FToUI:       if(this->getFToUI(stmt).name == name){ return true; } continue;
					case Expr::Kind::Add:         if(this->getAdd(stmt).name == name){ return true; } continue;
					case Expr::Kind::SAddWrap: {
						const SAddWrap& sadd_wrap = this->getSAddWrap(stmt);
						if(sadd_wrap.resultName == name){ return true; }
						if(sadd_wrap.wrappedName == name){ return true; }
						continue;
					} break;
					case Expr::Kind::SAddWrapResult:  continue;
					case Expr::Kind::SAddWrapWrapped: continue;
					case Expr::Kind::UAddWrap: {
						const UAddWrap& uadd_wrap = this->getUAddWrap(stmt);
						if(uadd_wrap.resultName == name){ return true; }
						if(uadd_wrap.wrappedName == name){ return true; }
						continue;
					} break;
					case Expr::Kind::UAddWrapResult:  continue;
					case Expr::Kind::UAddWrapWrapped: continue;
					case Expr::Kind::SAddSat:         if(this->getSAddSat(stmt).name == name){ return true; } continue;
					case Expr::Kind::UAddSat:         if(this->getUAddSat(stmt).name == name){ return true; } continue;
					case Expr::Kind::FAdd:            if(this->getFAdd(stmt).name == name){ return true; } continue;
					case Expr::Kind::Sub:             if(this->getSub(stmt).name == name){ return true; } continue;
					case Expr::Kind::SSubWrap: {
						const SSubWrap& ssub_wrap = this->getSSubWrap(stmt);
						if(ssub_wrap.resultName == name){ return true; }
						if(ssub_wrap.wrappedName == name){ return true; }
						continue;
					} break;
					case Expr::Kind::SSubWrapResult:  continue;
					case Expr::Kind::SSubWrapWrapped: continue;
					case Expr::Kind::USubWrap: {
						const USubWrap& usub_wrap = this->getUSubWrap(stmt);
						if(usub_wrap.resultName == name){ return true; }
						if(usub_wrap.wrappedName == name){ return true; }
						continue;
					} break;
					case Expr::Kind::USubWrapResult:  continue;
					case Expr::Kind::USubWrapWrapped: continue;
					case Expr::Kind::SSubSat:         if(this->getSSubSat(stmt).name == name){ return true; } continue;
					case Expr::Kind::USubSat:         if(this->getUSubSat(stmt).name == name){ return true; } continue;
					case Expr::Kind::FSub:            if(this->getFSub(stmt).name == name){ return true; } continue;
					case Expr::Kind::Mul:             if(this->getMul(stmt).name == name){ return true; } continue;
					case Expr::Kind::SMulWrap: {
						const SMulWrap& smul_wrap = this->getSMulWrap(stmt);
						if(smul_wrap.resultName == name){ return true; }
						if(smul_wrap.wrappedName == name){ return true; }
						continue;
					} break;
					case Expr::Kind::SMulWrapResult:  continue;
					case Expr::Kind::SMulWrapWrapped: continue;
					case Expr::Kind::UMulWrap: {
						const UMulWrap& umul_wrap = this->getUMulWrap(stmt);
						if(umul_wrap.resultName == name){ return true; }
						if(umul_wrap.wrappedName == name){ return true; }
						continue;
					} break;
					case Expr::Kind::UMulWrapResult:  continue;
					case Expr::Kind::UMulWrapWrapped: continue;
					case Expr::Kind::SMulSat:         if(this->getSMulSat(stmt).name == name){ return true; } continue;
					case Expr::Kind::UMulSat:         if(this->getUMulSat(stmt).name == name){ return true; } continue;
					case Expr::Kind::FMul:            if(this->getFMul(stmt).name == name){ return true; } continue;
					case Expr::Kind::SDiv:            if(this->getSDiv(stmt).name == name){ return true; } continue;
					case Expr::Kind::UDiv:            if(this->getUDiv(stmt).name == name){ return true; } continue;
					case Expr::Kind::FDiv:            if(this->getFDiv(stmt).name == name){ return true; } continue;
					case Expr::Kind::SRem:            if(this->getSRem(stmt).name == name){ return true; } continue;
					case Expr::Kind::URem:            if(this->getURem(stmt).name == name){ return true; } continue;
					case Expr::Kind::FRem:            if(this->getFRem(stmt).name == name){ return true; } continue;
					case Expr::Kind::IEq:             if(this->getIEq(stmt).name == name){ return true; } continue;
					case Expr::Kind::FEq:             if(this->getFEq(stmt).name == name){ return true; } continue;
					case Expr::Kind::INeq:            if(this->getINeq(stmt).name == name){ return true; } continue;
					case Expr::Kind::FNeq:            if(this->getFNeq(stmt).name == name){ return true; } continue;
					case Expr::Kind::SLT:             if(this->getSLT(stmt).name == name){ return true; } continue;
					case Expr::Kind::ULT:             if(this->getULT(stmt).name == name){ return true; } continue;
					case Expr::Kind::FLT:             if(this->getFLT(stmt).name == name){ return true; } continue;
					case Expr::Kind::SLTE:            if(this->getSLTE(stmt).name == name){ return true; } continue;
					case Expr::Kind::ULTE:            if(this->getULTE(stmt).name == name){ return true; } continue;
					case Expr::Kind::FLTE:            if(this->getFLTE(stmt).name == name){ return true; } continue;
					case Expr::Kind::SGT:             if(this->getSGT(stmt).name == name){ return true; } continue;
					case Expr::Kind::UGT:             if(this->getUGT(stmt).name == name){ return true; } continue;
					case Expr::Kind::FGT:             if(this->getFGT(stmt).name == name){ return true; } continue;
					case Expr::Kind::SGTE:            if(this->getSGTE(stmt).name == name){ return true; } continue;
					case Expr::Kind::UGTE:            if(this->getUGTE(stmt).name == name){ return true; } continue;
					case Expr::Kind::FGTE:            if(this->getFGTE(stmt).name == name){ return true; } continue;
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