////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/Interpreter.h"



namespace pcit::pir{
	
	
	auto Interpreter::runFunction(Function::ID func_id, evo::SmallVector<core::GenericValue>& args)
	-> evo::Expected<core::GenericValue, ErrorInfo> {
		const Function& func = this->module.getFunction(func_id);
		this->reader.setTargetFunction(func);
		const BasicBlock::ID entry_block_id = *func.begin();

		this->current_basic_block = &this->reader.getBasicBlock(entry_block_id);
		this->current_instruction_index = 0;
		this->stack.emplace(func, std::nullopt);

		return this->run();
	}



	auto Interpreter::run() -> evo::Expected<core::GenericValue, ErrorInfo> {
		while(true){
			const Expr& instr = this->current_basic_block->operator[](this->current_instruction_index);

			switch(instr.kind()){
				case Expr::Kind::NONE: evo::debugFatalBreak("Invalid Expr");

				case Expr::Kind::GLOBAL_VALUE: case Expr::Kind::FUNCTION_POINTER: case Expr::Kind::NUMBER:
				case Expr::Kind::BOOLEAN:      case Expr::Kind::PARAM_EXPR: {
					evo::debugFatalBreak("Invalid instruction");
				} break;

				case Expr::Kind::CALL: {
					const Call& call = this->reader.getCall(instr);

					call.target.visit([&](const auto& target) -> void {
						using TargetT = std::decay_t<decltype(target)>;

						if constexpr(std::is_same<TargetT, Function::ID>()){
							const Function& func = this->module.getFunction(target);
							this->reader.setTargetFunction(func);
							const BasicBlock::ID entry_block_id = *func.begin();

							this->stack.top().basic_block = this->current_basic_block;
							this->stack.top().instruction_index = this->current_instruction_index;
							this->stack.top().return_location = instr;

							this->current_basic_block = &this->reader.getBasicBlock(entry_block_id);
							this->current_instruction_index = ~0ull; // this is needed because it will be incremented
							this->stack.emplace(func, std::nullopt);

						}else if constexpr(std::is_same<TargetT, FunctionDecl::ID>()){
							evo::unimplemented("Func call target FunctionDecl::ID");

						}else if constexpr(std::is_same<TargetT, PtrCall>()){
							evo::unimplemented("Func call target PtrCall");

						}else{
							static_assert(false, "Unsupported func call target");
						}
					});
				} break;

				case Expr::Kind::CALL_VOID: {
					// const CallVoid& callvoid = this->reader.getCallVoid(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::CallVoid");
				} break;

				case Expr::Kind::BREAKPOINT: {
					evo::unimplemented("PIR Interpreter - Expr::Kind::Breakpoint");
				} break;

				case Expr::Kind::RET: {
					const Ret& ret = this->reader.getRet(instr);

					if(ret.value.has_value()){
						this->return_register = this->get_expr(*ret.value);
					}else{
						this->return_register = core::GenericValue();
					}

					this->stack.pop();

					if(this->stack.empty()){
						return this->return_register;
					}else{
						this->reader.setTargetFunction(this->stack.top().func);
						this->current_basic_block = this->stack.top().basic_block;
						this->current_instruction_index = this->stack.top().instruction_index;

						if(this->stack.top().return_location.has_value()){
							this->stack.top().values.emplace(*this->stack.top().return_location, this->return_register);
						}
					}
				} break;

				case Expr::Kind::BRANCH: {
					// const Branch& branch = this->reader.getBranch(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::Branch");
				} break;

				case Expr::Kind::COND_BRANCH: {
					// const CondBranch& condbranch = this->reader.getCondBranch(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::CondBranch");
				} break;

				case Expr::Kind::UNREACHABLE: {
					evo::unimplemented("PIR Interpreter - Expr::Kind::Unreachable");
				} break;

				case Expr::Kind::ALLOCA: {
					evo::debugFatalBreak("Invalid instruction - `@alloca`");
				} break;

				case Expr::Kind::LOAD: {
					// const Load& load = this->reader.getLoad(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::Load");
				} break;

				case Expr::Kind::STORE: {
					// const Store& store = this->reader.getStore(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::Store");
				} break;

				case Expr::Kind::CALC_PTR: {
					// const CalcPtr& calcptr = this->reader.getCalcPtr(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::CalcPtr");
				} break;

				case Expr::Kind::MEMCPY: {
					// const Memcpy& memcpy = this->reader.getMemcpy(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::Memcpy");
				} break;

				case Expr::Kind::MEMSET: {
					// const Memset& memset = this->reader.getMemset(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::Memset");
				} break;

				case Expr::Kind::BIT_CAST: {
					// const BitCast& bitcast = this->reader.getBitCast(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::BitCast");
				} break;

				case Expr::Kind::TRUNC: {
					// const Trunc& trunc = this->reader.getTrunc(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::Trunc");
				} break;

				case Expr::Kind::FTRUNC: {
					// const FTrunc& ftrunc = this->reader.getFTrunc(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::FTrunc");
				} break;

				case Expr::Kind::SEXT: {
					// const SExt& sext = this->reader.getSExt(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::SExt");
				} break;

				case Expr::Kind::ZEXT: {
					// const ZExt& zext = this->reader.getZExt(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::ZExt");
				} break;

				case Expr::Kind::FEXT: {
					// const FExt& fext = this->reader.getFExt(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::FExt");
				} break;

				case Expr::Kind::ITOF: {
					// const IToF& itof = this->reader.getIToF(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::IToF");
				} break;

				case Expr::Kind::UITOF: {
					// const UIToF& uitof = this->reader.getUIToF(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::UIToF");
				} break;

				case Expr::Kind::FTOI: {
					// const FToI& ftoi = this->reader.getFToI(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::FToI");
				} break;

				case Expr::Kind::FTOUI: {
					// const FToUI& ftoui = this->reader.getFToUI(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::FToUI");
				} break;

				case Expr::Kind::ADD: {
					// const Add& add = this->reader.getAdd(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::Add");
				} break;

				case Expr::Kind::SADD_WRAP: {
					// const SAddWrap& saddwrap = this->reader.getSAddWrap(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::SAddWrap");
				} break;

				case Expr::Kind::SADD_WRAP_RESULT: {
					evo::unimplemented("PIR Interpreter - Expr::Kind::SAddWrapResult");
				} break;

				case Expr::Kind::SADD_WRAP_WRAPPED: {
					evo::unimplemented("PIR Interpreter - Expr::Kind::SAddWrapWrapped");
				} break;

				case Expr::Kind::UADD_WRAP: {
					// const UAddWrap& uaddwrap = this->reader.getUAddWrap(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::UAddWrap");
				} break;

				case Expr::Kind::UADD_WRAP_RESULT: {
					evo::unimplemented("PIR Interpreter - Expr::Kind::UAddWrapResult");
				} break;

				case Expr::Kind::UADD_WRAP_WRAPPED: {
					evo::unimplemented("PIR Interpreter - Expr::Kind::UAddWrapWrapped");
				} break;

				case Expr::Kind::SADD_SAT: {
					// const SAddSat& saddsat = this->reader.getSAddSat(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::SAddSat");
				} break;

				case Expr::Kind::UADD_SAT: {
					// const UAddSat& uaddsat = this->reader.getUAddSat(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::UAddSat");
				} break;

				case Expr::Kind::FADD: {
					// const FAdd& fadd = this->reader.getFAdd(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::FAdd");
				} break;

				case Expr::Kind::SUB: {
					// const Sub& sub = this->reader.getSub(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::Sub");
				} break;

				case Expr::Kind::SSUB_WRAP: {
					// const SSubWrap& ssubwrap = this->reader.getSSubWrap(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::SSubWrap");
				} break;

				case Expr::Kind::SSUB_WRAP_RESULT: {
					evo::unimplemented("PIR Interpreter - Expr::Kind::SSubWrapResult");
				} break;

				case Expr::Kind::SSUB_WRAP_WRAPPED: {
					evo::unimplemented("PIR Interpreter - Expr::Kind::SSubWrapWrapped");
				} break;

				case Expr::Kind::USUB_WRAP: {
					// const USubWrap& usubwrap = this->reader.getUSubWrap(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::USubWrap");
				} break;

				case Expr::Kind::USUB_WRAP_RESULT: {
					evo::unimplemented("PIR Interpreter - Expr::Kind::USubWrapResult");
				} break;

				case Expr::Kind::USUB_WRAP_WRAPPED: {
					evo::unimplemented("PIR Interpreter - Expr::Kind::USubWrapWrapped");
				} break;

				case Expr::Kind::SSUB_SAT: {
					// const SSubSat& ssubsat = this->reader.getSSubSat(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::SSubSat");
				} break;

				case Expr::Kind::USUB_SAT: {
					// const USubSat& usubsat = this->reader.getUSubSat(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::USubSat");
				} break;

				case Expr::Kind::FSUB: {
					// const FSub& fsub = this->reader.getFSub(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::FSub");
				} break;

				case Expr::Kind::MUL: {
					// const Mul& mul = this->reader.getMul(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::Mul");
				} break;

				case Expr::Kind::SMUL_WRAP: {
					// const SMulWrap& smulwrap = this->reader.getSMulWrap(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::SMulWrap");
				} break;

				case Expr::Kind::SMUL_WRAP_RESULT: {
					evo::unimplemented("PIR Interpreter - Expr::Kind::SMulWrapResult");
				} break;

				case Expr::Kind::SMUL_WRAP_WRAPPED: {
					evo::unimplemented("PIR Interpreter - Expr::Kind::SMulWrapWrapped");
				} break;

				case Expr::Kind::UMUL_WRAP: {
					// const UMulWrap& umulwrap = this->reader.getUMulWrap(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::UMulWrap");
				} break;

				case Expr::Kind::UMUL_WRAP_RESULT: {
					evo::unimplemented("PIR Interpreter - Expr::Kind::UMulWrapResult");
				} break;

				case Expr::Kind::UMUL_WRAP_WRAPPED: {
					evo::unimplemented("PIR Interpreter - Expr::Kind::UMulWrapWrapped");
				} break;

				case Expr::Kind::SMUL_SAT: {
					// const SMulSat& smulsat = this->reader.getSMulSat(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::SMulSat");
				} break;

				case Expr::Kind::UMUL_SAT: {
					// const UMulSat& umulsat = this->reader.getUMulSat(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::UMulSat");
				} break;

				case Expr::Kind::FMUL: {
					// const FMul& fmul = this->reader.getFMul(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::FMul");
				} break;

				case Expr::Kind::SDIV: {
					// const SDiv& sdiv = this->reader.getSDiv(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::SDiv");
				} break;

				case Expr::Kind::UDIV: {
					// const UDiv& udiv = this->reader.getUDiv(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::UDiv");
				} break;

				case Expr::Kind::FDIV: {
					// const FDiv& fdiv = this->reader.getFDiv(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::FDiv");
				} break;

				case Expr::Kind::SREM: {
					// const SRem& srem = this->reader.getSRem(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::SRem");
				} break;

				case Expr::Kind::UREM: {
					// const URem& urem = this->reader.getURem(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::URem");
				} break;

				case Expr::Kind::FREM: {
					// const FRem& frem = this->reader.getFRem(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::FRem");
				} break;

				case Expr::Kind::FNEG: {
					// const FNeg& fneg = this->reader.getFNeg(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::FNeg");
				} break;

				case Expr::Kind::IEQ: {
					// const IEq& ieq = this->reader.getIEq(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::IEq");
				} break;

				case Expr::Kind::FEQ: {
					// const FEq& feq = this->reader.getFEq(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::FEq");
				} break;

				case Expr::Kind::INEQ: {
					// const INeq& ineq = this->reader.getINeq(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::INeq");
				} break;

				case Expr::Kind::FNEQ: {
					// const FNeq& fneq = this->reader.getFNeq(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::FNeq");
				} break;

				case Expr::Kind::SLT: {
					// const SLT& slt = this->reader.getSLT(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::SLT");
				} break;

				case Expr::Kind::ULT: {
					// const ULT& ult = this->reader.getULT(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::ULT");
				} break;

				case Expr::Kind::FLT: {
					// const FLT& flt = this->reader.getFLT(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::FLT");
				} break;

				case Expr::Kind::SLTE: {
					// const SLTE& slte = this->reader.getSLTE(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::SLTE");
				} break;

				case Expr::Kind::ULTE: {
					// const ULTE& ulte = this->reader.getULTE(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::ULTE");
				} break;

				case Expr::Kind::FLTE: {
					// const FLTE& flte = this->reader.getFLTE(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::FLTE");
				} break;

				case Expr::Kind::SGT: {
					// const SGT& sgt = this->reader.getSGT(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::SGT");
				} break;

				case Expr::Kind::UGT: {
					// const UGT& ugt = this->reader.getUGT(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::UGT");
				} break;

				case Expr::Kind::FGT: {
					// const FGT& fgt = this->reader.getFGT(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::FGT");
				} break;

				case Expr::Kind::SGTE: {
					// const SGTE& sgte = this->reader.getSGTE(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::SGTE");
				} break;

				case Expr::Kind::UGTE: {
					// const UGTE& ugte = this->reader.getUGTE(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::UGTE");
				} break;

				case Expr::Kind::FGTE: {
					// const FGTE& fgte = this->reader.getFGTE(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::FGTE");
				} break;

				case Expr::Kind::AND: {
					// const And& and_res = this->reader.getAnd(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::And");
				} break;

				case Expr::Kind::OR: {
					// const Or& or_res = this->reader.getOr(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::Or");
				} break;

				case Expr::Kind::XOR: {
					// const Xor& xor_res = this->reader.getXor(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::Xor");
				} break;

				case Expr::Kind::SHL: {
					// const SHL& shl = this->reader.getSHL(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::SHL");
				} break;

				case Expr::Kind::SSHL_SAT: {
					// const SSHLSat& sshlsat = this->reader.getSSHLSat(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::SSHLSat");
				} break;

				case Expr::Kind::USHL_SAT: {
					// const USHLSat& ushlsat = this->reader.getUSHLSat(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::USHLSat");
				} break;

				case Expr::Kind::SSHR: {
					// const SSHR& sshr = this->reader.getSSHR(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::SSHR");
				} break;

				case Expr::Kind::USHR: {
					// const USHR& ushr = this->reader.getUSHR(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::USHR");
				} break;
			}


			this->current_instruction_index += 1;
		}
	}



	auto Interpreter::get_expr(Expr expr) -> const core::GenericValue& {
		StackFrame& current_stack_frame = this->stack.top();

		switch(expr.kind()){
			case Expr::Kind::NONE: evo::debugFatalBreak("Invalid Expr");

			case Expr::Kind::GLOBAL_VALUE: {
				evo::unimplemented("Expr::Kind::GlobalValue");
			} break;

			case Expr::Kind::FUNCTION_POINTER: {
				evo::unimplemented("Expr::Kind::FunctionPointer");
			} break;

			case Expr::Kind::NUMBER: {
				auto find = current_stack_frame.values.find(expr);
				if(find != current_stack_frame.values.end()){ return find->second; }

				return current_stack_frame.values.emplace(
					expr, this->reader.getNumber(expr).asGenericValue()
				).first->second;
			} break;

			case Expr::Kind::BOOLEAN: {
				evo::unimplemented("Expr::Kind::Boolean");
			} break;

			case Expr::Kind::PARAM_EXPR: {
				evo::unimplemented("Expr::Kind::ParamExpr");
			} break;

			default: {
				return current_stack_frame.values[expr];
			} break;
		}
	}

	auto Interpreter::set_expr(Expr expr, core::GenericValue&& value) -> void {
		this->stack.top().values[expr] = std::move(value);
	}


}