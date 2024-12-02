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

		for(const StructType& struct_type : this->get_module().getStructTypeIter()){
			this->print_struct_type(struct_type);
		}

		this->printer.println();

		for(const GlobalVar& global_var : this->get_module().getGlobalVarIter()){
			this->print_global_var(global_var);
		}

		this->printer.println();

		for(const FunctionDecl& function_decl : this->get_module().getFunctionDeclIter()){
			this->print_function_decl(function_decl);
		}

		this->printer.println();

		for(const Function& function : this->get_module().getFunctionIter()){
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
			this->printer.print("{}${}", tabs(1), alloca.name);
			this->printer.printRed(" = @alloca ");
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
		this->printer.printGreen(" &{}", struct_type.name);
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
		switch(type.getKind()){
			case Type::Kind::Void:     { this->printer.printCyan("Void");                  } break;
			case Type::Kind::Signed:   { this->printer.printCyan("I{}", type.getWidth());  } break;
			case Type::Kind::Unsigned: { this->printer.printCyan("UI{}", type.getWidth()); } break;
			case Type::Kind::Bool:     { this->printer.printCyan("Bool");                  } break;
			case Type::Kind::Float:    { this->printer.printCyan("F{}", type.getWidth());  } break;
			case Type::Kind::BFloat:   { this->printer.printCyan("BF16");                  } break;
			case Type::Kind::Ptr:      { this->printer.printCyan("Ptr");                   } break;

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
		switch(expr.getKind()){
			case Expr::Kind::None: evo::debugFatalBreak("Not valid expr");

			case Expr::Kind::Number: {
				const Number& number = this->reader.getNumber(expr);
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

			case Expr::Kind::Boolean: {
				this->printer.printMagenta(evo::boolStr(this->reader.getBoolean(expr)));
			} break;

			case Expr::Kind::GlobalValue: {
				const GlobalVar& global_var = this->reader.getGlobalValue(expr);
				this->printer.print("${}", global_var.name);
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

			case Expr::Kind::Add: {
				const Add& add = this->reader.getAdd(expr);
				this->printer.print("${}", add.name);
			} break;

			case Expr::Kind::AddWrap: evo::debugFatalBreak("Expr::Kind::AddWrap is not a valid expression");

			case Expr::Kind::AddWrapResult: {
				const AddWrap& add_wrap = this->reader.getAddWrap(expr);
				this->printer.print("${}", add_wrap.resultName);
			} break;

			case Expr::Kind::AddWrapWrapped: {
				const AddWrap& add_wrap = this->reader.getAddWrap(expr);
				this->printer.print("${}", add_wrap.wrappedName);
			} break;
		}
	}


	auto ModulePrinter::print_expr_stmt(const Expr& stmt) -> void {
		switch(stmt.getKind()){
			case Expr::Kind::None: evo::debugFatalBreak("Not valid expr");

			case Expr::Kind::Number:      evo::debugFatalBreak("Expr::Kind::Number is not a valid statement");
			case Expr::Kind::Boolean:     evo::debugFatalBreak("Expr::Kind::Boolean is not a valid statement");
			case Expr::Kind::GlobalValue: evo::debugFatalBreak("Expr::Kind::GlobalValue is not a valid statement");
			case Expr::Kind::ParamExpr:   evo::debugFatalBreak("Expr::Kind::ParamExpr is not a valid statement");

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
						this->printer.printCyan("UI64");
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

			case Expr::Kind::Add: {
				const Add& add = this->reader.getAdd(stmt);

				this->printer.print("{}${} ", tabs(2), add.name);
				this->printer.printRed("= @add ");
				this->print_expr(add.lhs);
				this->printer.print(", ");
				this->print_expr(add.rhs);
				if(add.mayWrap){ this->printer.printRed(" #mayWrap"); }
				this->printer.println();
			} break;


			case Expr::Kind::AddWrap: {
				const AddWrap& add_wrap = this->reader.getAddWrap(stmt);

				this->printer.print("{}${}, ${} ", tabs(2), add_wrap.resultName, add_wrap.wrappedName);
				this->printer.printRed("= @addWrap ");

				this->print_expr(add_wrap.lhs);
				this->printer.print(", ");
				this->print_expr(add_wrap.rhs);
				this->printer.println();
			} break;

			case Expr::Kind::AddWrapResult: evo::debugFatalBreak("Expr::Kind::AddWrapResult is not a valid statement");
			case Expr::Kind::AddWrapWrapped:
				evo::debugFatalBreak("Expr::Kind::AddWrapWrapped is not a valid statement");

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