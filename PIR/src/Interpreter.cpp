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
				case Expr::Kind::None: evo::debugFatalBreak("Invalid Expr");

				case Expr::Kind::GlobalValue: case Expr::Kind::FunctionPointer:  case Expr::Kind::Number:
				case Expr::Kind::Boolean:     case Expr::Kind::ParamExpr: {
					evo::debugFatalBreak("Invalid instruction");
				} break;

				case Expr::Kind::Call: {
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

				case Expr::Kind::CallVoid: {
					// const CallVoid& callvoid = this->reader.getCallVoid(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::CallVoid");
				} break;

				case Expr::Kind::Breakpoint: {
					evo::unimplemented("PIR Interpreter - Expr::Kind::Breakpoint");
				} break;

				case Expr::Kind::Ret: {
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

				case Expr::Kind::Branch: {
					// const Branch& branch = this->reader.getBranch(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::Branch");
				} break;

				case Expr::Kind::CondBranch: {
					// const CondBranch& condbranch = this->reader.getCondBranch(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::CondBranch");
				} break;

				case Expr::Kind::Unreachable: {
					evo::unimplemented("PIR Interpreter - Expr::Kind::Unreachable");
				} break;

				case Expr::Kind::Alloca: {
					evo::debugFatalBreak("Invalid instruction - `@alloca`");
				} break;

				case Expr::Kind::Load: {
					// const Load& load = this->reader.getLoad(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::Load");
				} break;

				case Expr::Kind::Store: {
					// const Store& store = this->reader.getStore(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::Store");
				} break;

				case Expr::Kind::CalcPtr: {
					// const CalcPtr& calcptr = this->reader.getCalcPtr(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::CalcPtr");
				} break;

				case Expr::Kind::Memcpy: {
					// const Memcpy& memcpy = this->reader.getMemcpy(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::Memcpy");
				} break;

				case Expr::Kind::Memset: {
					// const Memset& memset = this->reader.getMemset(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::Memset");
				} break;

				case Expr::Kind::BitCast: {
					// const BitCast& bitcast = this->reader.getBitCast(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::BitCast");
				} break;

				case Expr::Kind::Trunc: {
					// const Trunc& trunc = this->reader.getTrunc(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::Trunc");
				} break;

				case Expr::Kind::FTrunc: {
					// const FTrunc& ftrunc = this->reader.getFTrunc(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::FTrunc");
				} break;

				case Expr::Kind::SExt: {
					// const SExt& sext = this->reader.getSExt(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::SExt");
				} break;

				case Expr::Kind::ZExt: {
					// const ZExt& zext = this->reader.getZExt(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::ZExt");
				} break;

				case Expr::Kind::FExt: {
					// const FExt& fext = this->reader.getFExt(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::FExt");
				} break;

				case Expr::Kind::IToF: {
					// const IToF& itof = this->reader.getIToF(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::IToF");
				} break;

				case Expr::Kind::UIToF: {
					// const UIToF& uitof = this->reader.getUIToF(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::UIToF");
				} break;

				case Expr::Kind::FToI: {
					// const FToI& ftoi = this->reader.getFToI(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::FToI");
				} break;

				case Expr::Kind::FToUI: {
					// const FToUI& ftoui = this->reader.getFToUI(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::FToUI");
				} break;

				case Expr::Kind::Add: {
					// const Add& add = this->reader.getAdd(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::Add");
				} break;

				case Expr::Kind::SAddWrap: {
					// const SAddWrap& saddwrap = this->reader.getSAddWrap(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::SAddWrap");
				} break;

				case Expr::Kind::SAddWrapResult: {
					evo::unimplemented("PIR Interpreter - Expr::Kind::SAddWrapResult");
				} break;

				case Expr::Kind::SAddWrapWrapped: {
					evo::unimplemented("PIR Interpreter - Expr::Kind::SAddWrapWrapped");
				} break;

				case Expr::Kind::UAddWrap: {
					// const UAddWrap& uaddwrap = this->reader.getUAddWrap(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::UAddWrap");
				} break;

				case Expr::Kind::UAddWrapResult: {
					evo::unimplemented("PIR Interpreter - Expr::Kind::UAddWrapResult");
				} break;

				case Expr::Kind::UAddWrapWrapped: {
					evo::unimplemented("PIR Interpreter - Expr::Kind::UAddWrapWrapped");
				} break;

				case Expr::Kind::SAddSat: {
					// const SAddSat& saddsat = this->reader.getSAddSat(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::SAddSat");
				} break;

				case Expr::Kind::UAddSat: {
					// const UAddSat& uaddsat = this->reader.getUAddSat(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::UAddSat");
				} break;

				case Expr::Kind::FAdd: {
					// const FAdd& fadd = this->reader.getFAdd(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::FAdd");
				} break;

				case Expr::Kind::Sub: {
					// const Sub& sub = this->reader.getSub(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::Sub");
				} break;

				case Expr::Kind::SSubWrap: {
					// const SSubWrap& ssubwrap = this->reader.getSSubWrap(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::SSubWrap");
				} break;

				case Expr::Kind::SSubWrapResult: {
					evo::unimplemented("PIR Interpreter - Expr::Kind::SSubWrapResult");
				} break;

				case Expr::Kind::SSubWrapWrapped: {
					evo::unimplemented("PIR Interpreter - Expr::Kind::SSubWrapWrapped");
				} break;

				case Expr::Kind::USubWrap: {
					// const USubWrap& usubwrap = this->reader.getUSubWrap(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::USubWrap");
				} break;

				case Expr::Kind::USubWrapResult: {
					evo::unimplemented("PIR Interpreter - Expr::Kind::USubWrapResult");
				} break;

				case Expr::Kind::USubWrapWrapped: {
					evo::unimplemented("PIR Interpreter - Expr::Kind::USubWrapWrapped");
				} break;

				case Expr::Kind::SSubSat: {
					// const SSubSat& ssubsat = this->reader.getSSubSat(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::SSubSat");
				} break;

				case Expr::Kind::USubSat: {
					// const USubSat& usubsat = this->reader.getUSubSat(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::USubSat");
				} break;

				case Expr::Kind::FSub: {
					// const FSub& fsub = this->reader.getFSub(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::FSub");
				} break;

				case Expr::Kind::Mul: {
					// const Mul& mul = this->reader.getMul(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::Mul");
				} break;

				case Expr::Kind::SMulWrap: {
					// const SMulWrap& smulwrap = this->reader.getSMulWrap(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::SMulWrap");
				} break;

				case Expr::Kind::SMulWrapResult: {
					evo::unimplemented("PIR Interpreter - Expr::Kind::SMulWrapResult");
				} break;

				case Expr::Kind::SMulWrapWrapped: {
					evo::unimplemented("PIR Interpreter - Expr::Kind::SMulWrapWrapped");
				} break;

				case Expr::Kind::UMulWrap: {
					// const UMulWrap& umulwrap = this->reader.getUMulWrap(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::UMulWrap");
				} break;

				case Expr::Kind::UMulWrapResult: {
					evo::unimplemented("PIR Interpreter - Expr::Kind::UMulWrapResult");
				} break;

				case Expr::Kind::UMulWrapWrapped: {
					evo::unimplemented("PIR Interpreter - Expr::Kind::UMulWrapWrapped");
				} break;

				case Expr::Kind::SMulSat: {
					// const SMulSat& smulsat = this->reader.getSMulSat(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::SMulSat");
				} break;

				case Expr::Kind::UMulSat: {
					// const UMulSat& umulsat = this->reader.getUMulSat(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::UMulSat");
				} break;

				case Expr::Kind::FMul: {
					// const FMul& fmul = this->reader.getFMul(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::FMul");
				} break;

				case Expr::Kind::SDiv: {
					// const SDiv& sdiv = this->reader.getSDiv(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::SDiv");
				} break;

				case Expr::Kind::UDiv: {
					// const UDiv& udiv = this->reader.getUDiv(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::UDiv");
				} break;

				case Expr::Kind::FDiv: {
					// const FDiv& fdiv = this->reader.getFDiv(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::FDiv");
				} break;

				case Expr::Kind::SRem: {
					// const SRem& srem = this->reader.getSRem(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::SRem");
				} break;

				case Expr::Kind::URem: {
					// const URem& urem = this->reader.getURem(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::URem");
				} break;

				case Expr::Kind::FRem: {
					// const FRem& frem = this->reader.getFRem(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::FRem");
				} break;

				case Expr::Kind::FNeg: {
					// const FNeg& fneg = this->reader.getFNeg(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::FNeg");
				} break;

				case Expr::Kind::IEq: {
					// const IEq& ieq = this->reader.getIEq(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::IEq");
				} break;

				case Expr::Kind::FEq: {
					// const FEq& feq = this->reader.getFEq(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::FEq");
				} break;

				case Expr::Kind::INeq: {
					// const INeq& ineq = this->reader.getINeq(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::INeq");
				} break;

				case Expr::Kind::FNeq: {
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

				case Expr::Kind::And: {
					// const And& and_res = this->reader.getAnd(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::And");
				} break;

				case Expr::Kind::Or: {
					// const Or& or_res = this->reader.getOr(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::Or");
				} break;

				case Expr::Kind::Xor: {
					// const Xor& xor_res = this->reader.getXor(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::Xor");
				} break;

				case Expr::Kind::SHL: {
					// const SHL& shl = this->reader.getSHL(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::SHL");
				} break;

				case Expr::Kind::SSHLSat: {
					// const SSHLSat& sshlsat = this->reader.getSSHLSat(instr);
					evo::unimplemented("PIR Interpreter - Expr::Kind::SSHLSat");
				} break;

				case Expr::Kind::USHLSat: {
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
			case Expr::Kind::None: evo::debugFatalBreak("Invalid Expr");

			case Expr::Kind::GlobalValue: {
				evo::unimplemented("Expr::Kind::GlobalValue");
			} break;

			case Expr::Kind::FunctionPointer: {
				evo::unimplemented("Expr::Kind::FunctionPointer");
			} break;

			case Expr::Kind::Number: {
				auto find = current_stack_frame.values.find(expr);
				if(find != current_stack_frame.values.end()){ return find->second; }

				return current_stack_frame.values.emplace(
					expr, this->reader.getNumber(expr).asGenericValue()
				).first->second;
			} break;

			case Expr::Kind::Boolean: {
				evo::unimplemented("Expr::Kind::Boolean");
			} break;

			case Expr::Kind::ParamExpr: {
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