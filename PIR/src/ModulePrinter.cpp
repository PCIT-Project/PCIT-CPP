////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/ModulePrinter.h"

#include "../include/GlobalVar.h"
#include "../include/BasicBlock.h"
#include "../include/Function.h"
#include "../include/Module.h"

#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

namespace pcit::pir{

	static constexpr auto tabs(unsigned indent) -> std::string_view {
		switch(indent){
			case 0: return "";
			case 1: return "    ";
			case 2: return "        ";
			case 3: return "            ";
		}

		evo::debugFatalBreak("Unsupported indent level");
	}
	

	auto ModulePrinter::print() -> void {
		this->printer.printlnGray("// module: {}", this->get_module().getName());

		if(this->get_module().getStructTypeIter().empty() == false){
			this->printer.println();

			for(const StructType& struct_type : this->get_module().getStructTypeIter()){
				this->print_struct_type(struct_type);
			}
		}

		if(this->get_module().getGlobalVarIter().empty() == false){
			this->printer.println();

			for(const GlobalVar& global_var : this->get_module().getGlobalVarIter()){
				this->print_global_var(global_var);
			}
		}

		if(this->get_module().getFunctionDeclIter().empty() == false){
			this->printer.println();

			for(const FunctionDecl& function_decl : this->get_module().getFunctionDeclIter()){
				this->print_function_decl(function_decl);
			}
		}

		if(this->get_module().getFunctionIter().empty() == false){
			this->printer.println();

			for(size_t i = 0; const Function& function : this->get_module().getFunctionIter()){
				this->print_function(function);

				if(i + 1 < this->get_module().getFunctionIter().size()){
					this->printer.println();
				}
				i += 1;
			}
		}
	}



	struct FuncDeclRef{
		std::string_view name;
		evo::ArrayProxy<Parameter> parameters;
		CallingConvention callingConvention;
		Linkage linkage;
		Type returnType;
	};

	auto ModulePrinter::print_function_decl_impl(const FuncDeclRef& func_decl) -> void {
		this->printer.printCyan("func ");
		if(isStandardName(func_decl.name)){
			this->printer.printGreen("&{} ", func_decl.name);
		}else{
			this->printer.print("&");
			this->print_non_standard_name(func_decl.name);
			this->printer.print(" ");
		}
		this->printer.printRed("= ");
		this->printer.print("(");
		for(size_t i = 0; const Parameter& param : func_decl.parameters){
			EVO_DEFER([&](){ i += 1; });

			this->printer.print("${}", param.getName());
			this->printer.printRed(": ");
			this->print_type(param.getType());

			if(i < func_decl.parameters.size() - 1){
				this->printer.print(", ");
			}
		}
		this->printer.print(") ");

		switch(func_decl.callingConvention){
			case CallingConvention::Default: {
				// do nothing
			} break;

			case CallingConvention::C: {
				this->printer.printRed("#callConv");
				this->printer.print("(c) ");
			} break;

			case CallingConvention::Fast: {
				this->printer.printRed("#callConv");
				this->printer.print("(fast) ");
			} break;

			case CallingConvention::Cold: {
				this->printer.printRed("#callConv");
				this->printer.print("(cold) ");
			} break;
		}

		switch(func_decl.linkage){
			case Linkage::Default: {
				// do nothing
			} break;

			case Linkage::Private: {
				this->printer.printRed("#linkage");
				this->printer.print("(private) ");
			} break;

			case Linkage::Internal: {
				this->printer.printRed("#linkage");
				this->printer.print("(internal) ");
			} break;

			case Linkage::External: {
				this->printer.printRed("#linkage");
				this->printer.print("(external) ");
			} break;
		}

		this->printer.printRed("-> ");
		this->print_type(func_decl.returnType);
	}


	auto ModulePrinter::print_function(const Function& function) -> void {
		this->reader.setTargetFunction(function);

		this->print_function_decl_impl(
			FuncDeclRef(
				function.getName(),
				function.getParameters(),
				function.getCallingConvention(),
				function.getLinkage(),
				function.getReturnType()
			)
		);
		

		this->printer.println(" {");

		for(const Alloca& alloca : function.getAllocasRange()){
			this->printer.print("{}${} ", tabs(1), alloca.name);
			this->printer.printRed("= @alloca ");
			this->print_type(alloca.type);
			this->printer.println();
		}

		for(const BasicBlock::ID& basic_block_id : function){
			this->print_basic_block(this->reader.getBasicBlock(basic_block_id));
		}

		this->printer.println("}");
	}


	auto ModulePrinter::print_function_decl(const FunctionDecl& function_decl) -> void {
		this->print_function_decl_impl(
			FuncDeclRef(
				function_decl.name,
				function_decl.parameters,
				function_decl.callingConvention,
				function_decl.linkage,
				function_decl.returnType
			)
		);
		this->printer.println();
	}


	auto ModulePrinter::print_struct_type(const StructType& struct_type) -> void {
		this->printer.printCyan("type");
		this->printer.printGreen(" &{} ", struct_type.name);
		this->printer.printRed("= ");
		this->printer.printCyan("struct ");

		if(struct_type.isPacked){
			this->printer.printRed("#packed ");
		}

		this->printer.print("{");
		for(size_t i = 0; const Type& member : struct_type.members){
			this->print_type(member);

			if(i < struct_type.members.size() - 1){
				this->printer.print(", ");
			}

			i += 1;
		}
		this->printer.println("}");
	}


	auto ModulePrinter::print_global_var(const GlobalVar& global_var) -> void {
		if(global_var.isConstant){
			this->printer.printCyan("const ");
		}else{
			this->printer.printCyan("var ");
		}

		if(isStandardName(global_var.name)){
			this->printer.print("&{}", global_var.name);
		}else{
			this->print_non_standard_name(global_var.name);
		}	
		this->printer.printRed(": ");
		this->print_type(global_var.type);
		this->printer.print(" ");

		switch(global_var.linkage){
			case Linkage::Default: {
				// do nothing
			} break;

			case Linkage::Private: {
				this->printer.printRed("#linkage");
				this->printer.print("(private) ");
			} break;

			case Linkage::Internal: {
				this->printer.printRed("#linkage");
				this->printer.print("(internal) ");
			} break;

			case Linkage::External: {
				this->printer.printRed("#linkage");
				this->printer.print("(external) ");
			} break;
		}

		this->printer.printRed("= ");

		this->print_global_var_value(global_var.value);

		this->printer.println();
	}


	auto ModulePrinter::print_global_var_value(const GlobalVar::Value& global_var_value) -> void {
		global_var_value.visit([&](const auto& value) -> void {
			using ValueT = std::decay_t<decltype(value)>;

			if constexpr(std::is_same<ValueT, Expr>()){
				this->print_expr(value);

			}else if constexpr(std::is_same<ValueT, GlobalVar::Zeroinit>()){
				this->printer.printRed("zeroinit");

			}else if constexpr(std::is_same<ValueT, GlobalVar::Uninit>()){
				this->printer.printRed("uninit");

			}else if constexpr(std::is_same<ValueT, GlobalVar::String::ID>()){
				const GlobalVar::String& string_value = this->get_module().getGlobalString(value);

				this->printer.printYellow("\"");

				auto char_str = evo::StaticString<2>();
				char_str.resize(2);

				auto hex_str = evo::StaticString<3>();
				hex_str.resize(3);
				hex_str[0] = '\\';

				for(char c : string_value.value){
					if(c > 31 && c < 127){
						char_str[0] = c;
						this->printer.printYellow(char_str);
					}else{
						const uint8_t char_num = uint8_t(c);
						uint8_t char_ones_place = char_num % 16;

						const auto num_to_hex_char = [](uint8_t num) -> char {
							switch(num){
								case 0: return '0';
								case 1: return '1';
								case 2: return '2';
								case 3: return '3';
								case 4: return '4';
								case 5: return '5';
								case 6: return '6';
								case 7: return '7';
								case 8: return '8';
								case 9: return '9';
								case 10: return 'A';
								case 11: return 'B';
								case 12: return 'C';
								case 13: return 'D';
								case 14: return 'E';
								case 15: return 'F';
							}
							evo::debugFatalBreak("Not a valid hex decimal ({})", num);
						};

						hex_str[1] = num_to_hex_char((char_num - char_ones_place) / 16);
						hex_str[2] = num_to_hex_char(char_ones_place);
						this->printer.printMagenta(hex_str);
					}
				}

				this->printer.printMagenta("\\00");
				this->printer.printYellow("\"");

			}else if constexpr(std::is_same<ValueT, GlobalVar::Array::ID>()){
				const GlobalVar::Array& array = this->get_module().getGlobalArray(value);

				this->printer.print("[");
				for(size_t i = 0; const GlobalVar::Value& array_elem : array.values){
					this->print_global_var_value(array_elem);
					
					if(i + 1 < array.values.size()){
						this->printer.print(", ");
					}

					i += 1;
				}
				this->printer.print("]");

			}else if constexpr(std::is_same<ValueT, GlobalVar::Struct::ID>()){
				const GlobalVar::Struct& struct_value = this->get_module().getGlobalStruct(value);

				this->printer.print("{");
				for(size_t i = 0; const GlobalVar::Value& struct_elem : struct_value.values){
					this->print_global_var_value(struct_elem);
					
					if(i + 1 < struct_value.values.size()){
						this->printer.print(", ");
					}

					i += 1;
				}
				this->printer.print("}");

			}else{
				static_assert(false, "Unsupported global var kind");
			}
		});
	}
		

	auto ModulePrinter::print_basic_block(const BasicBlock& basic_block) -> void {
		this->printer.println("{}${}:", tabs(1), basic_block.getName());

		for(const Expr& expr : basic_block){
			this->print_expr_stmt(expr);
		}
	}



	auto ModulePrinter::print_type(const Type& type) -> void {
		switch(type.kind()){
			case Type::Kind::Void:     { this->printer.printCyan("Void");                 } break;
			case Type::Kind::Integer:  { this->printer.printCyan("I{}", type.getWidth()); } break;
			case Type::Kind::Bool:     { this->printer.printCyan("Bool");                 } break;
			case Type::Kind::Float:    { this->printer.printCyan("F{}", type.getWidth()); } break;
			case Type::Kind::BFloat:   { this->printer.printCyan("BF16");                 } break;
			case Type::Kind::Ptr:      { this->printer.printCyan("Ptr");                  } break;

			case Type::Kind::Array: {
				const ArrayType& array_type = this->get_module().getArrayType(type);

				printer.print("[");
				this->print_type(array_type.elemType);
				printer.printRed(":");
				printer.printMagenta("{}", array_type.length);
				printer.print("]");
			} break;

			case Type::Kind::Struct: {
				const StructType& struct_type = this->get_module().getStructType(type);
				
				printer.print("&{}", struct_type.name);
			} break;

			case Type::Kind::Function: evo::debugFatalBreak("Cannot print function type");
		}

	}



	auto ModulePrinter::print_expr(const Expr& expr) -> void {
		switch(expr.kind()){
			case Expr::Kind::None: evo::debugFatalBreak("Not valid expr");

			case Expr::Kind::Number: {
				const Number& number = this->reader.getNumber(expr);
				this->print_type(number.type);
				this->printer.print("(");
				if(number.type.kind() == Type::Kind::Integer){
					this->printer.printMagenta(number.getInt().toString(true));
				}else{
					this->printer.printMagenta(number.getFloat().toString());
				}
				this->printer.print(")");
			} break;

			case Expr::Kind::Boolean: {
				this->printer.printMagenta(evo::boolStr(this->reader.getBoolean(expr)));
			} break;

			case Expr::Kind::GlobalValue: {
				const GlobalVar& global_var = this->reader.getGlobalValue(expr);
				this->printer.print("@{}", global_var.name);
			} break;

			case Expr::Kind::FunctionPointer: {
				const Function& function = this->reader.getFunctionPointer(expr);
				this->printer.print("@{}", function.getName());
			} break;

			case Expr::Kind::ParamExpr: {
				const ParamExpr param_expr = this->reader.getParamExpr(expr);
				this->printer.print(
					"${}", this->get_current_func().getParameters()[param_expr.index].getName()
				);
			} break;

			case Expr::Kind::Call: {
				const Call& call_inst = this->reader.getCall(expr);
				this->printer.print("${}", call_inst.name);
			} break;

			case Expr::Kind::CallVoid:    evo::debugFatalBreak("Expr::Kind::CallVoid is not a valid expression");
			case Expr::Kind::Breakpoint:  evo::debugFatalBreak("Expr::Kind::Breakpoint is not a valid expression");
			case Expr::Kind::Ret:         evo::debugFatalBreak("Expr::Kind::Ret is not a valid expression");
			case Expr::Kind::Branch:      evo::debugFatalBreak("Expr::Kind::Branch is not a valid expression");
			case Expr::Kind::CondBranch:  evo::debugFatalBreak("Expr::Kind::CondBranch is not a valid expression");
			case Expr::Kind::Unreachable: evo::debugFatalBreak("Expr::Kind::Unreachable is not a valid expression");

			case Expr::Kind::Alloca: {
				const Alloca& alloca = this->reader.getAlloca(expr);
				this->printer.print("${}", alloca.name);
			} break;

			case Expr::Kind::Load: {
				const Load& load = this->reader.getLoad(expr);
				this->printer.print("${}", load.name);
			} break;

			case Expr::Kind::Store: evo::debugFatalBreak("Expr::Kind::Store is not a valid expression");

			case Expr::Kind::CalcPtr: {
				const CalcPtr& calc_ptr = this->reader.getCalcPtr(expr);
				this->printer.print("${}", calc_ptr.name);
			} break;

			case Expr::Kind::Memcpy: evo::debugFatalBreak("Expr::Kind::Memcpy is not a valid expression");
			case Expr::Kind::Memset: evo::debugFatalBreak("Expr::Kind::Memset is not a valid expression");

			case Expr::Kind::BitCast: {
				const BitCast& bitcast = this->reader.getBitCast(expr);
				this->printer.print("${}", bitcast.name);
			} break;

			case Expr::Kind::Trunc: {
				const Trunc& trunc = this->reader.getTrunc(expr);
				this->printer.print("${}", trunc.name);
			} break;

			case Expr::Kind::FTrunc: {
				const FTrunc& ftrunc = this->reader.getFTrunc(expr);
				this->printer.print("${}", ftrunc.name);
			} break;

			case Expr::Kind::SExt: {
				const SExt& sext = this->reader.getSExt(expr);
				this->printer.print("${}", sext.name);
			} break;

			case Expr::Kind::ZExt: {
				const ZExt& zext = this->reader.getZExt(expr);
				this->printer.print("${}", zext.name);
			} break;

			case Expr::Kind::FExt: {
				const FExt& fext = this->reader.getFExt(expr);
				this->printer.print("${}", fext.name);
			} break;

			case Expr::Kind::IToF: {
				const IToF& itof = this->reader.getIToF(expr);
				this->printer.print("${}", itof.name);
			} break;

			case Expr::Kind::UIToF: {
				const UIToF& uitof = this->reader.getUIToF(expr);
				this->printer.print("${}", uitof.name);
			} break;

			case Expr::Kind::FToI: {
				const FToI& ftoi = this->reader.getFToI(expr);
				this->printer.print("${}", ftoi.name);
			} break;

			case Expr::Kind::FToUI: {
				const FToUI& ftoui = this->reader.getFToUI(expr);
				this->printer.print("${}", ftoui.name);
			} break;


			case Expr::Kind::Add: {
				const Add& add = this->reader.getAdd(expr);
				this->printer.print("${}", add.name);
			} break;

			case Expr::Kind::SAddWrap: evo::debugFatalBreak("Expr::Kind::SAddWrap is not a valid expression");

			case Expr::Kind::SAddWrapResult: {
				const SAddWrap& sadd_wrap = this->reader.getSAddWrap(expr);
				this->printer.print("${}", sadd_wrap.resultName);
			} break;

			case Expr::Kind::SAddWrapWrapped: {
				const SAddWrap& sadd_wrap = this->reader.getSAddWrap(expr);
				this->printer.print("${}", sadd_wrap.wrappedName);
			} break;

			case Expr::Kind::UAddWrap: evo::debugFatalBreak("Expr::Kind::UAddWrap is not a valid expression");

			case Expr::Kind::UAddWrapResult: {
				const UAddWrap& uadd_wrap = this->reader.getUAddWrap(expr);
				this->printer.print("${}", uadd_wrap.resultName);
			} break;

			case Expr::Kind::UAddWrapWrapped: {
				const UAddWrap& uadd_wrap = this->reader.getUAddWrap(expr);
				this->printer.print("${}", uadd_wrap.wrappedName);
			} break;

			case Expr::Kind::SAddSat: {
				const SAddSat& sadd_sat = this->reader.getSAddSat(expr);
				this->printer.print("${}", sadd_sat.name);
			} break;

			case Expr::Kind::UAddSat: {
				const UAddSat& uadd_sat = this->reader.getUAddSat(expr);
				this->printer.print("${}", uadd_sat.name);
			} break;

			case Expr::Kind::FAdd: {
				const FAdd& fadd = this->reader.getFAdd(expr);
				this->printer.print("${}", fadd.name);
			} break;

			case Expr::Kind::Sub: {
				const Sub& sub = this->reader.getSub(expr);
				this->printer.print("${}", sub.name);
			} break;

			case Expr::Kind::SSubWrap: evo::debugFatalBreak("Expr::Kind::SSubWrap is not a valid expression");

			case Expr::Kind::SSubWrapResult: {
				const SSubWrap& ssub_wrap = this->reader.getSSubWrap(expr);
				this->printer.print("${}", ssub_wrap.resultName);
			} break;

			case Expr::Kind::SSubWrapWrapped: {
				const SSubWrap& ssub_wrap = this->reader.getSSubWrap(expr);
				this->printer.print("${}", ssub_wrap.wrappedName);
			} break;

			case Expr::Kind::USubWrap: evo::debugFatalBreak("Expr::Kind::USubWrap is not a valid expression");

			case Expr::Kind::USubWrapResult: {
				const USubWrap& usub_wrap = this->reader.getUSubWrap(expr);
				this->printer.print("${}", usub_wrap.resultName);
			} break;

			case Expr::Kind::USubWrapWrapped: {
				const USubWrap& usub_wrap = this->reader.getUSubWrap(expr);
				this->printer.print("${}", usub_wrap.wrappedName);
			} break;

			case Expr::Kind::SSubSat: {
				const SSubSat& ssub_sat = this->reader.getSSubSat(expr);
				this->printer.print("${}", ssub_sat.name);
			} break;

			case Expr::Kind::USubSat: {
				const USubSat& usub_sat = this->reader.getUSubSat(expr);
				this->printer.print("${}", usub_sat.name);
			} break;

			case Expr::Kind::FSub: {
				const FSub& fsub = this->reader.getFSub(expr);
				this->printer.print("${}", fsub.name);
			} break;

			case Expr::Kind::Mul: {
				const Mul& mul = this->reader.getMul(expr);
				this->printer.print("${}", mul.name);
			} break;

			case Expr::Kind::SMulWrap: evo::debugFatalBreak("Expr::Kind::SMulWrap is not a valid expression");

			case Expr::Kind::SMulWrapResult: {
				const SMulWrap& smul_wrap = this->reader.getSMulWrap(expr);
				this->printer.print("${}", smul_wrap.resultName);
			} break;

			case Expr::Kind::SMulWrapWrapped: {
				const SMulWrap& smul_wrap = this->reader.getSMulWrap(expr);
				this->printer.print("${}", smul_wrap.wrappedName);
			} break;

			case Expr::Kind::UMulWrap: evo::debugFatalBreak("Expr::Kind::UMulWrap is not a valid expression");

			case Expr::Kind::UMulWrapResult: {
				const UMulWrap& umul_wrap = this->reader.getUMulWrap(expr);
				this->printer.print("${}", umul_wrap.resultName);
			} break;

			case Expr::Kind::UMulWrapWrapped: {
				const UMulWrap& umul_wrap = this->reader.getUMulWrap(expr);
				this->printer.print("${}", umul_wrap.wrappedName);
			} break;

			case Expr::Kind::SMulSat: {
				const SMulSat& smul_sat = this->reader.getSMulSat(expr);
				this->printer.print("${}", smul_sat.name);
			} break;

			case Expr::Kind::UMulSat: {
				const UMulSat& umul_sat = this->reader.getUMulSat(expr);
				this->printer.print("${}", umul_sat.name);
			} break;

			case Expr::Kind::FMul: {
				const FMul& fmul = this->reader.getFMul(expr);
				this->printer.print("${}", fmul.name);
			} break;

			case Expr::Kind::SDiv: {
				const SDiv& sdiv = this->reader.getSDiv(expr);
				this->printer.print("${}", sdiv.name);
			} break;

			case Expr::Kind::UDiv: {
				const UDiv& udiv = this->reader.getUDiv(expr);
				this->printer.print("${}", udiv.name);
			} break;

			case Expr::Kind::FDiv: {
				const FDiv& fdiv = this->reader.getFDiv(expr);
				this->printer.print("${}", fdiv.name);
			} break;

			case Expr::Kind::SRem: {
				const SRem& srem = this->reader.getSRem(expr);
				this->printer.print("${}", srem.name);
			} break;

			case Expr::Kind::URem: {
				const URem& urem = this->reader.getURem(expr);
				this->printer.print("${}", urem.name);
			} break;

			case Expr::Kind::FRem: {
				const FRem& frem = this->reader.getFRem(expr);
				this->printer.print("${}", frem.name);
			} break;

			case Expr::Kind::FNeg: {
				const FNeg& fneg = this->reader.getFNeg(expr);
				this->printer.print("${}", fneg.name);
			} break;

			case Expr::Kind::IEq: {
				const IEq& ieq = this->reader.getIEq(expr);
				this->printer.print("${}", ieq.name);
			} break;

			case Expr::Kind::FEq: {
				const FEq& feq = this->reader.getFEq(expr);
				this->printer.print("${}", feq.name);
			} break;

			case Expr::Kind::INeq: {
				const INeq& ineq = this->reader.getINeq(expr);
				this->printer.print("${}", ineq.name);
			} break;

			case Expr::Kind::FNeq: {
				const FNeq& fneq = this->reader.getFNeq(expr);
				this->printer.print("${}", fneq.name);
			} break;

			case Expr::Kind::SLT: {
				const SLT& slt = this->reader.getSLT(expr);
				this->printer.print("${}", slt.name);
			} break;

			case Expr::Kind::ULT: {
				const ULT& ult = this->reader.getULT(expr);
				this->printer.print("${}", ult.name);
			} break;

			case Expr::Kind::FLT: {
				const FLT& flt = this->reader.getFLT(expr);
				this->printer.print("${}", flt.name);
			} break;

			case Expr::Kind::SLTE: {
				const SLTE& slte = this->reader.getSLTE(expr);
				this->printer.print("${}", slte.name);
			} break;

			case Expr::Kind::ULTE: {
				const ULTE& ulte = this->reader.getULTE(expr);
				this->printer.print("${}", ulte.name);
			} break;

			case Expr::Kind::FLTE: {
				const FLTE& flte = this->reader.getFLTE(expr);
				this->printer.print("${}", flte.name);
			} break;

			case Expr::Kind::SGT: {
				const SGT& sgt = this->reader.getSGT(expr);
				this->printer.print("${}", sgt.name);
			} break;

			case Expr::Kind::UGT: {
				const UGT& ugt = this->reader.getUGT(expr);
				this->printer.print("${}", ugt.name);
			} break;

			case Expr::Kind::FGT: {
				const FGT& fgt = this->reader.getFGT(expr);
				this->printer.print("${}", fgt.name);
			} break;

			case Expr::Kind::SGTE: {
				const SGTE& sgte = this->reader.getSGTE(expr);
				this->printer.print("${}", sgte.name);
			} break;

			case Expr::Kind::UGTE: {
				const UGTE& ugte = this->reader.getUGTE(expr);
				this->printer.print("${}", ugte.name);
			} break;

			case Expr::Kind::FGTE: {
				const FGTE& fgte = this->reader.getFGTE(expr);
				this->printer.print("${}", fgte.name);
			} break;

			case Expr::Kind::And: {
				const And& and_stmt = this->reader.getAnd(expr);
				this->printer.print("${}", and_stmt.name);
			} break;

			case Expr::Kind::Or: {
				const Or& or_stmt = this->reader.getOr(expr);
				this->printer.print("${}", or_stmt.name);
			} break;

			case Expr::Kind::Xor: {
				const Xor& xor_stmt = this->reader.getXor(expr);
				this->printer.print("${}", xor_stmt.name);
			} break;

			case Expr::Kind::SHL: {
				const SHL& shl = this->reader.getSHL(expr);
				this->printer.print("${}", shl.name);
			} break;

			case Expr::Kind::SSHLSat: {
				const SSHLSat& sshlsat = this->reader.getSSHLSat(expr);
				this->printer.print("${}", sshlsat.name);
			} break;

			case Expr::Kind::USHLSat: {
				const USHLSat& ushlsat = this->reader.getUSHLSat(expr);
				this->printer.print("${}", ushlsat.name);
			} break;

			case Expr::Kind::SSHR: {
				const SSHR& sshr = this->reader.getSSHR(expr);
				this->printer.print("${}", sshr.name);
			} break;

			case Expr::Kind::USHR: {
				const USHR& ushr = this->reader.getUSHR(expr);
				this->printer.print("${}", ushr.name);
			} break;
		}
	}


	auto ModulePrinter::print_expr_stmt(const Expr& stmt) -> void {
		switch(stmt.kind()){
			case Expr::Kind::None: evo::debugFatalBreak("Not valid expr");

			case Expr::Kind::Number:      evo::debugFatalBreak("Expr::Kind::Number is not a valid statement");
			case Expr::Kind::Boolean:     evo::debugFatalBreak("Expr::Kind::Boolean is not a valid statement");
			case Expr::Kind::GlobalValue: evo::debugFatalBreak("Expr::Kind::GlobalValue is not a valid statement");
			case Expr::Kind::FunctionPointer: 
				evo::debugFatalBreak("Expr::Kind::FunctionPointer is not a valid statement");
			case Expr::Kind::ParamExpr: evo::debugFatalBreak("Expr::Kind::ParamExpr is not a valid statement");

			case Expr::Kind::Call: {
				const Call& call_inst = this->reader.getCall(stmt);

				this->printer.print("{}${} ", tabs(2), call_inst.name);
				this->printer.printRed("= ");

				this->print_function_call_impl(call_inst.target, call_inst.args);
			} break;

			case Expr::Kind::CallVoid: {
				const CallVoid& call_void_inst = this->reader.getCallVoid(stmt);

				this->printer.print(tabs(2));

				this->print_function_call_impl(call_void_inst.target, call_void_inst.args);
			} break;

			case Expr::Kind::Breakpoint: {
				this->printer.printlnRed("{}@breakpoint", tabs(2));
			} break;

			case Expr::Kind::Ret: {
				const Ret& ret_inst = this->reader.getRet(stmt);

				if(ret_inst.value.has_value()){
					this->printer.printRed("{}@ret ", tabs(2));
					this->print_expr(*ret_inst.value);
				}else{
					this->printer.printRed("{}@ret", tabs(2));
				}
				this->printer.println();
			} break;


			case Expr::Kind::Branch: {
				this->printer.printRed("{}@branch ", tabs(2));
				const BasicBlock::ID basic_block_id = reader.getBranch(stmt).target;
				this->printer.println("${}", reader.getBasicBlock(basic_block_id).getName());
			} break;

			case Expr::Kind::CondBranch: {
				const CondBranch& cond_branch = this->reader.getCondBranch(stmt);

				this->printer.printRed("{}@condBranch ", tabs(2));
				this->print_expr(cond_branch.cond);
				this->printer.println(
					", ${}, ${}", 
					reader.getBasicBlock(cond_branch.thenBlock).getName(),
					reader.getBasicBlock(cond_branch.elseBlock).getName()
				);
			} break;

			case Expr::Kind::Unreachable: {
				this->printer.printlnRed("{}@unreachable", tabs(2));
			} break;

			case Expr::Kind::Alloca: evo::debugFatalBreak("Expr::Kind::Alloca should not be printed through this func");

			case Expr::Kind::Load: {
				const Load& load = this->reader.getLoad(stmt);

				this->printer.print("{}${} ", tabs(2), load.name);
				this->printer.printRed("= @load ");
				this->print_type(load.type);
				this->printer.print(" ");
				this->print_expr(load.source);
				this->print_atomic_ordering(load.atomicOrdering);
				if(load.isVolatile){ this->printer.printRed(" #volatile"); }
				this->printer.println();
			} break;

			case Expr::Kind::Store: {
				const Store& store = this->reader.getStore(stmt);

				this->printer.printRed("{}@store ", tabs(2));
				this->print_expr(store.destination);
				this->printer.print(", ");
				this->print_expr(store.value);
				this->print_atomic_ordering(store.atomicOrdering);
				if(store.isVolatile){ this->printer.printRed(" #volatile"); }
				this->printer.println();
			} break;

			case Expr::Kind::CalcPtr: {
				const CalcPtr& calc_ptr = this->reader.getCalcPtr(stmt);

				this->printer.print("{}${} ", tabs(2), calc_ptr.name);
				this->printer.printRed("= @calcPtr ");
				this->print_type(calc_ptr.ptrType);
				this->printer.print(" ");
				for(size_t i = 0; const CalcPtr::Index& index : calc_ptr.indices){
					if(index.is<int64_t>()){
						this->printer.printCyan("I64");
						this->printer.print("(");
						this->printer.printMagenta("{}", index.as<int64_t>());
						this->printer.print(")");
					}else{
						this->print_expr(index.as<Expr>());
					}

					if(i + 1 < calc_ptr.indices.size()){
						this->printer.print(", ");
					}
					i += 1;
				}
				this->printer.println();
			} break;

			case Expr::Kind::Memcpy: {
				const Memcpy& memcpy = this->reader.getMemcpy(stmt);

				this->printer.printRed("{}@memcpy ", tabs(2));
				this->print_expr(memcpy.dst);
				this->printer.print(", ");
				this->print_expr(memcpy.src);
				this->printer.print(", ");
				this->print_expr(memcpy.numBytes);
				if(memcpy.isVolatile){ this->printer.printRed(" #volatile"); }
				this->printer.println();
			} break;

			case Expr::Kind::Memset: {
				const Memset& memset = this->reader.getMemset(stmt);

				this->printer.printRed("{}@memset ", tabs(2));
				this->print_expr(memset.dst);
				this->printer.print(", ");
				this->print_expr(memset.value);
				this->printer.print(", ");
				this->print_expr(memset.numBytes);
				if(memset.isVolatile){ this->printer.printRed(" #volatile"); }
				this->printer.println();
			} break;

			case Expr::Kind::BitCast: {
				const BitCast& bitcast = this->reader.getBitCast(stmt);

				this->printer.print("{}${} ", tabs(2), bitcast.name);
				this->printer.printRed("= @bitCast ");
				this->print_type(bitcast.toType);
				this->printer.print(" ");
				this->print_expr(bitcast.fromValue);
				this->printer.println();
			} break;

			case Expr::Kind::Trunc: {
				const Trunc& trunc = this->reader.getTrunc(stmt);

				this->printer.print("{}${} ", tabs(2), trunc.name);
				this->printer.printRed("= @trunc ");
				this->print_type(trunc.toType);
				this->printer.print(" ");
				this->print_expr(trunc.fromValue);
				this->printer.println();
			} break;

			case Expr::Kind::FTrunc: {
				const FTrunc& ftrunc = this->reader.getFTrunc(stmt);

				this->printer.print("{}${} ", tabs(2), ftrunc.name);
				this->printer.printRed("= @ftrunc ");
				this->print_type(ftrunc.toType);
				this->printer.print(" ");
				this->print_expr(ftrunc.fromValue);
				this->printer.println();
			} break;

			case Expr::Kind::SExt: {
				const SExt& sext = this->reader.getSExt(stmt);

				this->printer.print("{}${} ", tabs(2), sext.name);
				this->printer.printRed("= @sext ");
				this->print_type(sext.toType);
				this->printer.print(" ");
				this->print_expr(sext.fromValue);
				this->printer.println();
			} break;

			case Expr::Kind::ZExt: {
				const ZExt& zext = this->reader.getZExt(stmt);

				this->printer.print("{}${} ", tabs(2), zext.name);
				this->printer.printRed("= @zext ");
				this->print_type(zext.toType);
				this->printer.print(" ");
				this->print_expr(zext.fromValue);
				this->printer.println();
			} break;

			case Expr::Kind::FExt: {
				const FExt& fext = this->reader.getFExt(stmt);

				this->printer.print("{}${} ", tabs(2), fext.name);
				this->printer.printRed("= @fext ");
				this->print_type(fext.toType);
				this->printer.print(" ");
				this->print_expr(fext.fromValue);
				this->printer.println();
			} break;

			case Expr::Kind::IToF: {
				const IToF& itof = this->reader.getIToF(stmt);

				this->printer.print("{}${} ", tabs(2), itof.name);
				this->printer.printRed("= @itof ");
				this->print_type(itof.toType);
				this->printer.print(" ");
				this->print_expr(itof.fromValue);
				this->printer.println();
			} break;

			case Expr::Kind::UIToF: {
				const UIToF& uitof = this->reader.getUIToF(stmt);

				this->printer.print("{}${} ", tabs(2), uitof.name);
				this->printer.printRed("= @uitof ");
				this->print_type(uitof.toType);
				this->printer.print(" ");
				this->print_expr(uitof.fromValue);
				this->printer.println();
			} break;

			case Expr::Kind::FToI: {
				const FToI& ftoi = this->reader.getFToI(stmt);

				this->printer.print("{}${} ", tabs(2), ftoi.name);
				this->printer.printRed("= @ftoi ");
				this->print_type(ftoi.toType);
				this->printer.print(" ");
				this->print_expr(ftoi.fromValue);
				this->printer.println();
			} break;

			case Expr::Kind::FToUI: {
				const FToUI& ftoui = this->reader.getFToUI(stmt);

				this->printer.print("{}${} ", tabs(2), ftoui.name);
				this->printer.printRed("= @ftoui ");
				this->print_type(ftoui.toType);
				this->printer.print(" ");
				this->print_expr(ftoui.fromValue);
				this->printer.println();
			} break;


			case Expr::Kind::Add: {
				const Add& add = this->reader.getAdd(stmt);

				this->printer.print("{}${} ", tabs(2), add.name);
				this->printer.printRed("= @add ");
				this->print_expr(add.lhs);
				this->printer.print(", ");
				this->print_expr(add.rhs);
				if(add.nsw){ this->printer.printRed(" #nsw"); }
				if(add.nuw){ this->printer.printRed(" #nuw"); }
				this->printer.println();
			} break;

			case Expr::Kind::SAddWrap: {
				const SAddWrap& sadd_wrap = this->reader.getSAddWrap(stmt);

				this->printer.print("{}${}, ${} ", tabs(2), sadd_wrap.resultName, sadd_wrap.wrappedName);
				this->printer.printRed("= @sAddWrap ");

				this->print_expr(sadd_wrap.lhs);
				this->printer.print(", ");
				this->print_expr(sadd_wrap.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::SAddWrapResult:
				evo::debugFatalBreak("Expr::Kind::SAddWrapResult is not a valid statement");

			case Expr::Kind::SAddWrapWrapped:
				evo::debugFatalBreak("Expr::Kind::SAddWrapWrapped is not a valid statement");


			case Expr::Kind::UAddWrap: {
				const UAddWrap& uadd_wrap = this->reader.getUAddWrap(stmt);

				this->printer.print("{}${}, ${} ", tabs(2), uadd_wrap.resultName, uadd_wrap.wrappedName);
				this->printer.printRed("= @uAddWrap ");

				this->print_expr(uadd_wrap.lhs);
				this->printer.print(", ");
				this->print_expr(uadd_wrap.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::UAddWrapResult:
				evo::debugFatalBreak("Expr::Kind::UAddWrapResult is not a valid statement");

			case Expr::Kind::UAddWrapWrapped:
				evo::debugFatalBreak("Expr::Kind::UAddWrapWrapped is not a valid statement");

			case Expr::Kind::SAddSat: {
				const SAddSat& sadd_sat = this->reader.getSAddSat(stmt);

				this->printer.print("{}${} ", tabs(2), sadd_sat.name);
				this->printer.printRed("= @saddSat ");
				this->print_expr(sadd_sat.lhs);
				this->printer.print(", ");
				this->print_expr(sadd_sat.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::UAddSat: {
				const UAddSat& uadd_sat = this->reader.getUAddSat(stmt);

				this->printer.print("{}${} ", tabs(2), uadd_sat.name);
				this->printer.printRed("= @uaddSat ");
				this->print_expr(uadd_sat.lhs);
				this->printer.print(", ");
				this->print_expr(uadd_sat.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::FAdd: {
				const FAdd& fadd = this->reader.getFAdd(stmt);

				this->printer.print("{}${} ", tabs(2), fadd.name);
				this->printer.printRed("= @fadd ");
				this->print_expr(fadd.lhs);
				this->printer.print(", ");
				this->print_expr(fadd.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::Sub: {
				const Sub& sub = this->reader.getSub(stmt);

				this->printer.print("{}${} ", tabs(2), sub.name);
				this->printer.printRed("= @sub ");
				this->print_expr(sub.lhs);
				this->printer.print(", ");
				this->print_expr(sub.rhs);
				if(sub.nsw){ this->printer.printRed(" #nsw"); }
				if(sub.nuw){ this->printer.printRed(" #nuw"); }
				this->printer.println();
			} break;

			case Expr::Kind::SSubWrap: {
				const SSubWrap& ssub_wrap = this->reader.getSSubWrap(stmt);

				this->printer.print("{}${}, ${} ", tabs(2), ssub_wrap.resultName, ssub_wrap.wrappedName);
				this->printer.printRed("= @sSubWrap ");

				this->print_expr(ssub_wrap.lhs);
				this->printer.print(", ");
				this->print_expr(ssub_wrap.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::SSubWrapResult:
				evo::debugFatalBreak("Expr::Kind::SSubWrapResult is not a valid statement");

			case Expr::Kind::SSubWrapWrapped:
				evo::debugFatalBreak("Expr::Kind::SSubWrapWrapped is not a valid statement");


			case Expr::Kind::USubWrap: {
				const USubWrap& usub_wrap = this->reader.getUSubWrap(stmt);

				this->printer.print("{}${}, ${} ", tabs(2), usub_wrap.resultName, usub_wrap.wrappedName);
				this->printer.printRed("= @uSubWrap ");

				this->print_expr(usub_wrap.lhs);
				this->printer.print(", ");
				this->print_expr(usub_wrap.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::USubWrapResult:
				evo::debugFatalBreak("Expr::Kind::USubWrapResult is not a valid statement");

			case Expr::Kind::USubWrapWrapped:
				evo::debugFatalBreak("Expr::Kind::USubWrapWrapped is not a valid statement");

			case Expr::Kind::SSubSat: {
				const SSubSat& ssub_sat = this->reader.getSSubSat(stmt);

				this->printer.print("{}${} ", tabs(2), ssub_sat.name);
				this->printer.printRed("= @ssubSat ");
				this->print_expr(ssub_sat.lhs);
				this->printer.print(", ");
				this->print_expr(ssub_sat.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::USubSat: {
				const USubSat& usub_sat = this->reader.getUSubSat(stmt);

				this->printer.print("{}${} ", tabs(2), usub_sat.name);
				this->printer.printRed("= @usubSat ");
				this->print_expr(usub_sat.lhs);
				this->printer.print(", ");
				this->print_expr(usub_sat.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::FSub: {
				const FSub& fsub = this->reader.getFSub(stmt);

				this->printer.print("{}${} ", tabs(2), fsub.name);
				this->printer.printRed("= @fsub ");
				this->print_expr(fsub.lhs);
				this->printer.print(", ");
				this->print_expr(fsub.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::Mul: {
				const Mul& mul = this->reader.getMul(stmt);

				this->printer.print("{}${} ", tabs(2), mul.name);
				this->printer.printRed("= @mul ");
				this->print_expr(mul.lhs);
				this->printer.print(", ");
				this->print_expr(mul.rhs);
				if(mul.nsw){ this->printer.printRed(" #nsw"); }
				if(mul.nuw){ this->printer.printRed(" #nuw"); }
				this->printer.println();
			} break;

			case Expr::Kind::SMulWrap: {
				const SMulWrap& smul_wrap = this->reader.getSMulWrap(stmt);

				this->printer.print("{}${}, ${} ", tabs(2), smul_wrap.resultName, smul_wrap.wrappedName);
				this->printer.printRed("= @sMulWrap ");

				this->print_expr(smul_wrap.lhs);
				this->printer.print(", ");
				this->print_expr(smul_wrap.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::SMulWrapResult:
				evo::debugFatalBreak("Expr::Kind::SMulWrapResult is not a valid statement");

			case Expr::Kind::SMulWrapWrapped:
				evo::debugFatalBreak("Expr::Kind::SMulWrapWrapped is not a valid statement");


			case Expr::Kind::UMulWrap: {
				const UMulWrap& umul_wrap = this->reader.getUMulWrap(stmt);

				this->printer.print("{}${}, ${} ", tabs(2), umul_wrap.resultName, umul_wrap.wrappedName);
				this->printer.printRed("= @uMulWrap ");

				this->print_expr(umul_wrap.lhs);
				this->printer.print(", ");
				this->print_expr(umul_wrap.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::UMulWrapResult:
				evo::debugFatalBreak("Expr::Kind::UMulWrapResult is not a valid statement");

			case Expr::Kind::UMulWrapWrapped:
				evo::debugFatalBreak("Expr::Kind::UMulWrapWrapped is not a valid statement");

			case Expr::Kind::SMulSat: {
				const SMulSat& smul_sat = this->reader.getSMulSat(stmt);

				this->printer.print("{}${} ", tabs(2), smul_sat.name);
				this->printer.printRed("= @smulSat ");
				this->print_expr(smul_sat.lhs);
				this->printer.print(", ");
				this->print_expr(smul_sat.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::UMulSat: {
				const UMulSat& umul_sat = this->reader.getUMulSat(stmt);

				this->printer.print("{}${} ", tabs(2), umul_sat.name);
				this->printer.printRed("= @umulSat ");
				this->print_expr(umul_sat.lhs);
				this->printer.print(", ");
				this->print_expr(umul_sat.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::FMul: {
				const FMul& fmul = this->reader.getFMul(stmt);

				this->printer.print("{}${} ", tabs(2), fmul.name);
				this->printer.printRed("= @fmul ");
				this->print_expr(fmul.lhs);
				this->printer.print(", ");
				this->print_expr(fmul.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::SDiv: {
				const SDiv& sdiv = this->reader.getSDiv(stmt);

				this->printer.print("{}${} ", tabs(2), sdiv.name);
				this->printer.printRed("= @sdiv ");
				this->print_expr(sdiv.lhs);
				this->printer.print(", ");
				this->print_expr(sdiv.rhs);
				if(sdiv.isExact){ this->printer.printRed(" #exact"); }
				this->printer.println();
			} break;

			case Expr::Kind::UDiv: {
				const UDiv& udiv = this->reader.getUDiv(stmt);

				this->printer.print("{}${} ", tabs(2), udiv.name);
				this->printer.printRed("= @udiv ");
				this->print_expr(udiv.lhs);
				this->printer.print(", ");
				this->print_expr(udiv.rhs);
				if(udiv.isExact){ this->printer.printRed(" #exact"); }
				this->printer.println();
			} break;

			case Expr::Kind::FDiv: {
				const FDiv& fdiv = this->reader.getFDiv(stmt);

				this->printer.print("{}${} ", tabs(2), fdiv.name);
				this->printer.printRed("= @fdiv ");
				this->print_expr(fdiv.lhs);
				this->printer.print(", ");
				this->print_expr(fdiv.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::SRem: {
				const SRem& srem = this->reader.getSRem(stmt);

				this->printer.print("{}${} ", tabs(2), srem.name);
				this->printer.printRed("= @srem ");
				this->print_expr(srem.lhs);
				this->printer.print(", ");
				this->print_expr(srem.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::URem: {
				const URem& urem = this->reader.getURem(stmt);

				this->printer.print("{}${} ", tabs(2), urem.name);
				this->printer.printRed("= @urem ");
				this->print_expr(urem.lhs);
				this->printer.print(", ");
				this->print_expr(urem.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::FRem: {
				const FRem& frem = this->reader.getFRem(stmt);

				this->printer.print("{}${} ", tabs(2), frem.name);
				this->printer.printRed("= @frem ");
				this->print_expr(frem.lhs);
				this->printer.print(", ");
				this->print_expr(frem.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::FNeg: {
				const FNeg& fneg = this->reader.getFNeg(stmt);

				this->printer.print("{}${} ", tabs(2), fneg.name);
				this->printer.printRed("= @fneg ");
				this->print_expr(fneg.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::IEq: {
				const IEq& ieq = this->reader.getIEq(stmt);

				this->printer.print("{}${} ", tabs(2), ieq.name);
				this->printer.printRed("= @ieq ");
				this->print_expr(ieq.lhs);
				this->printer.print(", ");
				this->print_expr(ieq.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::FEq: {
				const FEq& feq = this->reader.getFEq(stmt);

				this->printer.print("{}${} ", tabs(2), feq.name);
				this->printer.printRed("= @feq ");
				this->print_expr(feq.lhs);
				this->printer.print(", ");
				this->print_expr(feq.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::INeq: {
				const INeq& ineq = this->reader.getINeq(stmt);

				this->printer.print("{}${} ", tabs(2), ineq.name);
				this->printer.printRed("= @ineq ");
				this->print_expr(ineq.lhs);
				this->printer.print(", ");
				this->print_expr(ineq.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::FNeq: {
				const FNeq& fneq = this->reader.getFNeq(stmt);

				this->printer.print("{}${} ", tabs(2), fneq.name);
				this->printer.printRed("= @fneq ");
				this->print_expr(fneq.lhs);
				this->printer.print(", ");
				this->print_expr(fneq.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::SLT: {
				const SLT& slt = this->reader.getSLT(stmt);

				this->printer.print("{}${} ", tabs(2), slt.name);
				this->printer.printRed("= @slt ");
				this->print_expr(slt.lhs);
				this->printer.print(", ");
				this->print_expr(slt.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::ULT: {
				const ULT& ult = this->reader.getULT(stmt);

				this->printer.print("{}${} ", tabs(2), ult.name);
				this->printer.printRed("= @ult ");
				this->print_expr(ult.lhs);
				this->printer.print(", ");
				this->print_expr(ult.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::FLT: {
				const FLT& flt = this->reader.getFLT(stmt);

				this->printer.print("{}${} ", tabs(2), flt.name);
				this->printer.printRed("= @flt ");
				this->print_expr(flt.lhs);
				this->printer.print(", ");
				this->print_expr(flt.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::SLTE: {
				const SLTE& slte = this->reader.getSLTE(stmt);

				this->printer.print("{}${} ", tabs(2), slte.name);
				this->printer.printRed("= @slte ");
				this->print_expr(slte.lhs);
				this->printer.print(", ");
				this->print_expr(slte.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::ULTE: {
				const ULTE& ulte = this->reader.getULTE(stmt);

				this->printer.print("{}${} ", tabs(2), ulte.name);
				this->printer.printRed("= @ulte ");
				this->print_expr(ulte.lhs);
				this->printer.print(", ");
				this->print_expr(ulte.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::FLTE: {
				const FLTE& flte = this->reader.getFLTE(stmt);

				this->printer.print("{}${} ", tabs(2), flte.name);
				this->printer.printRed("= @flte ");
				this->print_expr(flte.lhs);
				this->printer.print(", ");
				this->print_expr(flte.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::SGT: {
				const SGT& sgt = this->reader.getSGT(stmt);

				this->printer.print("{}${} ", tabs(2), sgt.name);
				this->printer.printRed("= @sgt ");
				this->print_expr(sgt.lhs);
				this->printer.print(", ");
				this->print_expr(sgt.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::UGT: {
				const UGT& ugt = this->reader.getUGT(stmt);

				this->printer.print("{}${} ", tabs(2), ugt.name);
				this->printer.printRed("= @ugt ");
				this->print_expr(ugt.lhs);
				this->printer.print(", ");
				this->print_expr(ugt.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::FGT: {
				const FGT& fgt = this->reader.getFGT(stmt);

				this->printer.print("{}${} ", tabs(2), fgt.name);
				this->printer.printRed("= @fgt ");
				this->print_expr(fgt.lhs);
				this->printer.print(", ");
				this->print_expr(fgt.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::SGTE: {
				const SGTE& sgte = this->reader.getSGTE(stmt);

				this->printer.print("{}${} ", tabs(2), sgte.name);
				this->printer.printRed("= @sgte ");
				this->print_expr(sgte.lhs);
				this->printer.print(", ");
				this->print_expr(sgte.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::UGTE: {
				const UGTE& ugte = this->reader.getUGTE(stmt);

				this->printer.print("{}${} ", tabs(2), ugte.name);
				this->printer.printRed("= @ugte ");
				this->print_expr(ugte.lhs);
				this->printer.print(", ");
				this->print_expr(ugte.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::FGTE: {
				const FGTE& fgte = this->reader.getFGTE(stmt);

				this->printer.print("{}${} ", tabs(2), fgte.name);
				this->printer.printRed("= @fgte ");
				this->print_expr(fgte.lhs);
				this->printer.print(", ");
				this->print_expr(fgte.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::And: {
				const And& and_stmt = this->reader.getAnd(stmt);

				this->printer.print("{}${} ", tabs(2), and_stmt.name);
				this->printer.printRed("= @and ");
				this->print_expr(and_stmt.lhs);
				this->printer.print(", ");
				this->print_expr(and_stmt.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::Or: {
				const Or& or_stmt = this->reader.getOr(stmt);

				this->printer.print("{}${} ", tabs(2), or_stmt.name);
				this->printer.printRed("= @or ");
				this->print_expr(or_stmt.lhs);
				this->printer.print(", ");
				this->print_expr(or_stmt.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::Xor: {
				const Xor& xor_stmt = this->reader.getXor(stmt);

				this->printer.print("{}${} ", tabs(2), xor_stmt.name);
				this->printer.printRed("= @xor ");
				this->print_expr(xor_stmt.lhs);
				this->printer.print(", ");
				this->print_expr(xor_stmt.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::SHL: {
				const SHL& shl = this->reader.getSHL(stmt);

				this->printer.print("{}${} ", tabs(2), shl.name);
				this->printer.printRed("= @shl ");
				this->print_expr(shl.lhs);
				this->printer.print(", ");
				this->print_expr(shl.rhs);
				if(shl.nsw){ this->printer.printRed(" #nsw"); }
				if(shl.nuw){ this->printer.printRed(" #nuw"); }
				this->printer.println();
			} break;

			case Expr::Kind::SSHLSat: {
				const SSHLSat& sshlsat = this->reader.getSSHLSat(stmt);

				this->printer.print("{}${} ", tabs(2), sshlsat.name);
				this->printer.printRed("= @sshlSat ");
				this->print_expr(sshlsat.lhs);
				this->printer.print(", ");
				this->print_expr(sshlsat.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::USHLSat: {
				const USHLSat& ushlsat = this->reader.getUSHLSat(stmt);

				this->printer.print("{}${} ", tabs(2), ushlsat.name);
				this->printer.printRed("= @ushlSat ");
				this->print_expr(ushlsat.lhs);
				this->printer.print(", ");
				this->print_expr(ushlsat.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::SSHR: {
				const SSHR& sshr = this->reader.getSSHR(stmt);

				this->printer.print("{}${} ", tabs(2), sshr.name);
				this->printer.printRed("= @sshr ");
				this->print_expr(sshr.lhs);
				this->printer.print(", ");
				this->print_expr(sshr.rhs);
				if(sshr.isExact){ this->printer.printRed(" #exact"); }
				this->printer.println();
			} break;

			case Expr::Kind::USHR: {
				const USHR& ushr = this->reader.getUSHR(stmt);

				this->printer.print("{}${} ", tabs(2), ushr.name);
				this->printer.printRed("= @ushr ");
				this->print_expr(ushr.lhs);
				this->printer.print(", ");
				this->print_expr(ushr.rhs);
				if(ushr.isExact){ this->printer.printRed(" #exact"); }
				this->printer.println();
			} break;

		}
	}



	auto ModulePrinter::print_function_call_impl(
		const evo::Variant<FunctionID, FunctionDeclID, PtrCall>& call_target, evo::ArrayProxy<Expr> args
	) -> void {
		this->printer.printRed("@call ");

		call_target.visit([&](const auto& target) -> void {
			using ValueT = std::decay_t<decltype(target)>;

			if constexpr(std::is_same_v<ValueT, Function::ID>){
				const std::string_view name = this->get_module().getFunction(target).getName();
				if(isStandardName(name)){
					this->printer.print("&{}", name);
				}else{
					this->printer.print("&");
					this->print_non_standard_name(name);
				}

			}else if constexpr(std::is_same_v<ValueT, FunctionDecl::ID>){
				const std::string_view name = this->get_module().getFunctionDecl(target).name;

				if(isStandardName(name)){
					this->printer.print("&{}", name);
				}else{
					this->printer.print("&");
					this->print_non_standard_name(name);
				}

				
			}else if constexpr(std::is_same_v<ValueT, PtrCall>){
				// TODO: 
				evo::debugFatalBreak("UNIMPLEMENTED (printing of pointer call)");

			}else{
				static_assert(false, "Unsupported call inst target");
			}
		});

		this->printer.print("(");

		for(size_t i = 0; const Expr& arg : args){
			this->print_expr(arg);

			if(i < args.size() - 1){
				this->printer.print(", ");
			}
		
			i += 1;
		}

		this->printer.println(")");
	}


	auto ModulePrinter::print_non_standard_name(std::string_view name) -> void {
		auto converted_name = std::string();

		for(char c : name){
			switch(c){
				case '\0': converted_name += "\0";
				// case '\a': converted_name += "\a";
				// case '\b': converted_name += "\b";
				// case '\t': converted_name += "\t";
				// case '\n': converted_name += "\n";
				// case '\v': converted_name += "\v";
				// case '\f': converted_name += "\f";
				// case '\r': converted_name += "\r";
				case '\"': converted_name += "\"";
				default: converted_name += c;
			}
		}

		this->printer.printYellow("\"{}\"", converted_name);
	}


	auto ModulePrinter::print_atomic_ordering(AtomicOrdering ordering) -> void {
		switch(ordering){
			case AtomicOrdering::None: {
				// none
			} break;

			case AtomicOrdering::Monotonic: {
				this->printer.printRed(" #atomic");
				this->printer.print("(monotonic)");
			} break;

			case AtomicOrdering::Acquire: {
				this->printer.printRed(" #atomic");
				this->printer.print("(acquire)");
			} break;

			case AtomicOrdering::Release: {
				this->printer.printRed(" #atomic");
				this->printer.print("(release)");
			} break;

			case AtomicOrdering::AcquireRelease: {
				this->printer.printRed(" #atomic");
				this->printer.print("(acquireRelease)");
			} break;

			case AtomicOrdering::SequentiallyConsistent: {
				this->printer.printRed(" #atomic");
				this->printer.print("(sequentiallyConsistent)");
			} break;

		}
	}


}