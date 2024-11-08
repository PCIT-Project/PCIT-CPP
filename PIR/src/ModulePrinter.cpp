//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


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
		this->printer.printlnGray("// module: {}", this->module.getName());

		for(const StructType& struct_type : this->module.getStructTypeIter()){
			this->print_struct_type(struct_type);
		}

		for(const GlobalVar& global_var : this->module.getGlobalVarIter()){
			this->print_global_var(global_var);
		}

		for(const FunctionDecl& function_decl : this->module.getFunctionDeclIter()){
			this->print_function_decl(function_decl);
		}

		for(const Function& function : this->module.getFunctionIter()){
			this->print_function(function);
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
		this->printer.print("\n");
		this->printer.printCyan("func ");
		this->printer.printGreen("${} ", func_decl.name);
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
		this->func = &function;

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

		for(const BasicBlock::ID& basic_block_id : function){
			this->print_basic_block(this->module.getBasicBlock(basic_block_id));
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
		this->printer.println(";");
	}


	auto ModulePrinter::print_struct_type(const StructType& struct_type) -> void {
		this->printer.print("\n");
		this->printer.printCyan("type");
		this->printer.printGreen(" ${}", struct_type.name);
		this->printer.printRed(" = ");
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
		this->printer.println("};");
	}


	auto ModulePrinter::print_global_var(const GlobalVar& global_var) -> void {
		this->printer.print("\n");

		if(global_var.isConstant){
			this->printer.printCyan("const ");
		}else{
			this->printer.printCyan("var ");
		}

		this->printer.print("${}", global_var.name);
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

		if(global_var.isExternal){
			this->printer.printRed("#external ");
		}

		if(global_var.value.has_value()){
			this->printer.printRed("= ");
			this->print_expr(*global_var.value);
		}

		this->printer.println(";");
	}
		

	auto ModulePrinter::print_basic_block(const BasicBlock& basic_block) -> void {
		this->printer.println("{}${}:", tabs(1), basic_block.getName());

		for(const Expr& expr : basic_block){
			this->print_expr_stmt(expr);
		}
	}



	auto ModulePrinter::print_type(const Type& type) -> void {
		switch(type.getKind()){
			case Type::Kind::Void:     { this->printer.printCyan("Void");                  } break;
			case Type::Kind::Signed:   { this->printer.printCyan("I{}", type.getWidth());  } break;
			case Type::Kind::Unsigned: { this->printer.printCyan("UI{}", type.getWidth()); } break;
			case Type::Kind::Float:    { this->printer.printCyan("F{}", type.getWidth());  } break;
			case Type::Kind::BFloat:   { this->printer.printCyan("BF16");                  } break;
			case Type::Kind::Ptr:      { this->printer.printCyan("Ptr");                   } break;

			case Type::Kind::Array: {
				const ArrayType& array_type = this->module.getTypeArray(type);

				printer.print("[");
				this->print_type(array_type.elemType);
				printer.printRed(":");
				printer.printMagenta("{}", array_type.length);
				printer.print("]");
			} break;

			case Type::Kind::Struct: {
				const StructType& struct_type = this->module.getTypeStruct(type);
				
				printer.print("${}", struct_type.name);
			} break;

			case Type::Kind::Function: evo::debugFatalBreak("Cannot print function type");
		}

	}



	auto ModulePrinter::print_expr(const Expr& expr) -> void {
		switch(expr.getKind()){
			case Expr::Kind::None: evo::debugFatalBreak("Not valid expr");

			case Expr::Kind::Number: {
				const Number& number = this->module.getNumber(expr);
				this->print_type(number.type);
				this->printer.print("(");
				if(number.type.isIntegral()){
					const bool is_signed = number.type.getKind() == Type::Kind::Signed;
					this->printer.printMagenta(number.getInt().toString(is_signed));
				}else{
					this->printer.printMagenta(number.getFloat().toString());
				}
				this->printer.print(")");
			} break;

			case Expr::Kind::GlobalValue: {
				const GlobalVar& global_var = this->module.getGlobalValue(expr);
				this->printer.print("${}", global_var.name);
			} break;

			case Expr::Kind::ParamExpr: {
				const ParamExpr param_expr = this->func->getParamExpr(expr);
				this->printer.print("${}", this->func->getParameters()[param_expr.index].getName());
			} break;

			case Expr::Kind::CallInst: {
				const CallInst& call_inst = this->func->getCallInst(expr);
				this->printer.print("${}", call_inst.name);
			} break;

			case Expr::Kind::CallVoidInst: evo::debugFatalBreak("Expr::Kind::CallVoidInst is not a valid expression");
			case Expr::Kind::RetInst: evo::debugFatalBreak("Expr::Kind::RetInst is not a valid expression");
			case Expr::Kind::BrInst: evo::debugFatalBreak("Expr::Kind::BrInst is not a valid expression");

			case Expr::Kind::Add: {
				const Add& add = this->func->getAdd(expr);
				this->printer.print("${}", add.name);
			} break;
		}
	}


	auto ModulePrinter::print_expr_stmt(const Expr& expr) -> void {
		switch(expr.getKind()){
			case Expr::Kind::None: evo::debugFatalBreak("Not valid expr");

			case Expr::Kind::Number:      evo::debugFatalBreak("Expr::Kind::Number is not a valid statement");
			case Expr::Kind::GlobalValue: evo::debugFatalBreak("Expr::Kind::GlobalValue is not a valid statement");
			case Expr::Kind::ParamExpr:   evo::debugFatalBreak("Expr::Kind::ParamExpr is not a valid statement");

			case Expr::Kind::CallInst: {
				const CallInst& call_inst = this->func->getCallInst(expr);

				this->printer.print("{}${} ", tabs(2), call_inst.name);
				this->printer.printRed("= ");

				this->print_function_call_impl(call_inst.target, call_inst.args);
			} break;

			case Expr::Kind::CallVoidInst: {
				const CallVoidInst& call_void_inst = this->func->getCallVoidInst(expr);

				this->printer.print(tabs(2));

				this->print_function_call_impl(call_void_inst.target, call_void_inst.args);
			} break;

			case Expr::Kind::RetInst: {
				const RetInst& ret_inst = this->func->getRetInst(expr);
				if(ret_inst.value.has_value()){
					this->printer.printRed("{}@ret ", tabs(2));
					this->print_expr(*ret_inst.value);
				}else{
					this->printer.printRed("{}@ret", tabs(2));
				}
				this->printer.println(";");
			} break;


			case Expr::Kind::BrInst: {
				this->printer.printRed("{}@br ", tabs(2));
				this->printer.println("${};", this->module.getBasicBlock(this->func->getBrInst(expr).target).getName());
			} break;

			case Expr::Kind::Add: {
				const Add& add = this->func->getAdd(expr);
				this->printer.print("{}${} ", tabs(2), add.name);
				this->printer.printRed("= @add");
				this->printer.print("(");
				this->print_expr(add.lhs);
				this->printer.print(", ");
				this->print_expr(add.rhs);
				this->printer.println(");");
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
				this->printer.print("${}", this->module.getFunction(target).getName());

			}else if constexpr(std::is_same_v<ValueT, FunctionDecl::ID>){
				this->printer.print("${}", this->module.getFunctionDecl(target).name);
				
			}else if constexpr(std::is_same_v<ValueT, PtrCall>){
				// TODO: 
				evo::debugFatalBreak("UNIMPLEMENTED");

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

		this->printer.println(");");
	}


}