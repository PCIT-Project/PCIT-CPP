////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/getDefaultDebugger.hpp"

#include "../include/ExecutionEngine.hpp"
#include "../include/ExecutionEngineDebuggerInterface.hpp"
#include "../include/ModulePrinter.hpp"
#include "../include/ReaderAgent.hpp"

#include <ranges>
#include <iostream>


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::pir{


	struct DefaultDebugger{
		ExecutionEngineDebuggerInterface& debugger;
		Module& module;


		std::unordered_map<std::string_view, std::function<void(evo::ArrayProxy<std::string_view>)>> runners{};

		std::unordered_map<std::string_view, const Function&> func_lookup{};
		bool func_lookup_created = false;

		struct FuncFrameInfo{
			std::unordered_map<std::string_view, Expr> register_lookup{};
			std::unordered_map<std::string_view, const Alloca&> alloca_lookup{};
			std::unordered_map<std::string_view, size_t> param_lookup{};
		};
		std::unordered_map<Function::ID, FuncFrameInfo> func_frame_infos{};




		[[nodiscard]] auto run() -> evo::Expected<core::GenericValue, ExecutionEngineExecutor::FuncRunError::Code> {
			// evo::print("\033[?1049h"); // switch to alternate buffer
			// EVO_DEFER([&](){ evo::print("\033[?1049l"); }); // switch back to normal buffer

			evo::println("Debugger:");
			evo::printlnGray("Type \"help\" for help");

			if(this->debugger.getLastErrorCode().has_value()){
				switch(*this->debugger.getLastErrorCode()){
					break; case ExecutionEngineExecutor::FuncRunError::Code::ABORT:
						evo::printlnBlue("Process stopped with code: ABORT");

					break; case ExecutionEngineExecutor::FuncRunError::Code::BREAKPOINT:
						evo::printlnBlue("Process stopped with code: BREAKPOINT");

					break; case ExecutionEngineExecutor::FuncRunError::Code::EXCEEDED_MAX_CALL_DEPTH:
						evo::printlnBlue("Process stopped with code: EXCEEDED_MAX_CALL_DEPTH");

					break; case ExecutionEngineExecutor::FuncRunError::Code::OUT_OF_BOUNDS_ACCESS:
						evo::printlnBlue("Process stopped with code: OUT_OF_BOUNDS_ACCESS");

					break; case ExecutionEngineExecutor::FuncRunError::Code::NULLPTR_ACCESS:
						evo::printlnBlue("Process stopped with code: NULLPTR_ACCESS");

					break; case ExecutionEngineExecutor::FuncRunError::Code::SEG_FAULT:
						evo::printlnBlue("Process stopped with code: SEG_FAULT");

					break; case ExecutionEngineExecutor::FuncRunError::Code::ARITHMETIC_WRAP:
						evo::printlnBlue("Process stopped with code: ARITHMETIC_WRAP");

					break; case ExecutionEngineExecutor::FuncRunError::Code::FLOATING_POINT_EXCEPTION:
						evo::printlnBlue("Process stopped with code: FLOATING_POINT_EXCEPTION");

					break; case ExecutionEngineExecutor::FuncRunError::Code::UNKNOWN_EXCEPTION:
						evo::printlnBlue("Process stopped with code: UNKNOWN_EXCEPTION");
				}
			}

			this->command_stack_trace(evo::ArrayProxy<std::string_view>());

			using Args = evo::ArrayProxy<std::string_view>;

			runners.emplace("st",          [&](Args args){ this->command_stack_trace(args); });
			runners.emplace("stack-trace", [&](Args args){ this->command_stack_trace(args); });
			runners.emplace("p",           [&](Args args){ this->command_print_value(args); });
			runners.emplace("print",       [&](Args args){ this->command_print_value(args); });
			runners.emplace("pf",          [&](Args args){ this->command_print_func(args); });
			runners.emplace("print-func",  [&](Args args){ this->command_print_func(args); });
			runners.emplace("help",        [&](Args args){ this->command_help(args); });



			while(true){
				evo::printGray(" > ");
				evo::print("");

				std::string command;
				std::getline(std::cin, command);

				auto args = evo::SmallVector<std::string_view>();
				{
					size_t arg_start = 0;
					size_t arg_size = 0;

					bool in_quote = false;

					for(size_t i = 0; char character : command){
						switch(character){
							case ' ': {
								if(in_quote){
									arg_size += 1;
									break;
								}

								args.emplace_back(std::string_view(&command[arg_start], arg_size));
								arg_start = i + 1;
								arg_size = 0;
							} break;

							case '"': {
								if(in_quote == false){
									if(arg_size == 0){
										in_quote = true;
										arg_start += 1;
									}

								}else{
									in_quote = true;

									args.emplace_back(std::string_view(&command[arg_start], arg_size));
									arg_start = i + 1;
									arg_size = 0;
								}
							} break;

							default: {
								arg_size += 1;
							} break;
						}

						i += 1;
					}

					if(arg_size != 0){
						args.emplace_back(std::string_view(&command[arg_start], arg_size));
					}
				}

				if(args.empty()){ continue; }

				if(args[0] == "q" || args[0] == "quit"){
					evo::printlnGray("Quitting Debugger");
					break;
				}

				if(args[0] == "c" || args[0] == "continue"){
					evo::Expected<core::GenericValue, ExecutionEngineExecutor::FuncRunError::Code>
						continue_execution = this->debugger.continueExecution();

					if(continue_execution.has_value()){ return std::move(continue_execution.value()); }

					switch(continue_execution.error()){
						break; case ExecutionEngineExecutor::FuncRunError::Code::ABORT:
							evo::printBlue("Process stopped with code: ABORT\n\n");

						break; case ExecutionEngineExecutor::FuncRunError::Code::BREAKPOINT:
							evo::printBlue("Process stopped with code: BREAKPOINT\n\n");

						break; case ExecutionEngineExecutor::FuncRunError::Code::EXCEEDED_MAX_CALL_DEPTH:
							evo::printBlue("Process stopped with code: EXCEEDED_MAX_CALL_DEPTH\n\n");

						break; case ExecutionEngineExecutor::FuncRunError::Code::OUT_OF_BOUNDS_ACCESS:
							evo::printBlue("Process stopped with code: OUT_OF_BOUNDS_ACCESS\n\n");

						break; case ExecutionEngineExecutor::FuncRunError::Code::NULLPTR_ACCESS:
							evo::printBlue("Process stopped with code: NULLPTR_ACCESS\n\n");

						break; case ExecutionEngineExecutor::FuncRunError::Code::SEG_FAULT:
							evo::printBlue("Process stopped with code: SEG_FAULT\n\n");

						break; case ExecutionEngineExecutor::FuncRunError::Code::ARITHMETIC_WRAP:
							evo::printBlue("Process stopped with code: ARITHMETIC_WRAP\n\n");

						break; case ExecutionEngineExecutor::FuncRunError::Code::FLOATING_POINT_EXCEPTION:
							evo::printBlue("Process stopped with code: FLOATING_POINT_EXCEPTION\n\n");

						break; case ExecutionEngineExecutor::FuncRunError::Code::UNKNOWN_EXCEPTION:
							evo::printBlue("Process stopped with code: UNKNOWN_EXCEPTION\n\n");
					}

					continue;
				}


				if(args[0] == "s" || args[0] == "step"){
					std::optional<core::GenericValue> step_execution = this->debugger.stepExecution();

					if(step_execution.has_value()){ return step_execution.value(); }

					if(this->debugger.getLastErrorCode().has_value()){
						switch(*this->debugger.getLastErrorCode()){
							break; case ExecutionEngineExecutor::FuncRunError::Code::ABORT:
								evo::printBlue("Process stopped with code: ABORT\n\n");

							break; case ExecutionEngineExecutor::FuncRunError::Code::BREAKPOINT:
								evo::printBlue("Process stopped with code: BREAKPOINT\n\n");

							break; case ExecutionEngineExecutor::FuncRunError::Code::EXCEEDED_MAX_CALL_DEPTH:
								evo::printBlue("Process stopped with code: EXCEEDED_MAX_CALL_DEPTH\n\n");

							break; case ExecutionEngineExecutor::FuncRunError::Code::OUT_OF_BOUNDS_ACCESS:
								evo::printBlue("Process stopped with code: OUT_OF_BOUNDS_ACCESS\n\n");

							break; case ExecutionEngineExecutor::FuncRunError::Code::NULLPTR_ACCESS:
								evo::printBlue("Process stopped with code: NULLPTR_ACCESS\n\n");

							break; case ExecutionEngineExecutor::FuncRunError::Code::SEG_FAULT:
								evo::printBlue("Process stopped with code: SEG_FAULT\n\n");

							break; case ExecutionEngineExecutor::FuncRunError::Code::ARITHMETIC_WRAP:
								evo::printBlue("Process stopped with code: ARITHMETIC_WRAP\n\n");

							break; case ExecutionEngineExecutor::FuncRunError::Code::FLOATING_POINT_EXCEPTION:
								evo::printBlue("Process stopped with code: FLOATING_POINT_EXCEPTION\n\n");

							break; case ExecutionEngineExecutor::FuncRunError::Code::UNKNOWN_EXCEPTION:
								evo::printBlue("Process stopped with code: UNKNOWN_EXCEPTION\n\n");
						}
					}

					continue;
				}



				const auto command_find = this->runners.find(args[0]);

				if(command_find == this->runners.end()){
					evo::printlnRed("Unknown command: \"{}\"", args[0]);
					evo::println();
					continue;
				}

				command_find->second(args);
				evo::println();
			}

			return evo::Unexpected(ExecutionEngineExecutor::FuncRunError::Code::ABORT);
		}


		auto command_help(evo::ArrayProxy<std::string_view> args) -> void {
			std::ignore = args;
			evo::println(R"(Help:
	Commands:
		q  | quit          quit the debugger        
		st | stack-trace   print stack trace        
		s  | step          step execution           
		c  | continue      continue execution       
		p  | print         print value              pf {expr} | pf {stack-frame} {expr}
		pf | print-func    print function by name   pf | pf {func} | pf {stack-frame}
		help               print the help           

	Use `@x` to access stack frame number `x`
	Use `@@` to access the current stack frame)");
		}


		auto command_stack_trace(evo::ArrayProxy<std::string_view> args) -> void {
			if(args.size() > 1){
				evo::printlnRed("Unknown stack-trace argument \"{}\"", args[1]);
				return;
			}

			evo::SmallVector<Function::ID> stack_trace = this->debugger.getStackTrace();

			evo::println("Stack Trace:");

			for(size_t i = stack_trace.size() - 1; const Function::ID func_id : stack_trace | std::views::reverse){
				const Function& pir_func = this->module.getFunction(func_id);

				evo::printGray("	({}) ", i);
				evo::println(pir_func.getName());

				i -= 1;
			}
		}


		auto command_print_value(evo::ArrayProxy<std::string_view> args) -> void {
			auto expr_name = std::string_view();
			const ExecutionEngineExecutor::StackFrame* stack_frame = nullptr;

			switch(args.size()){
				case 1: {
					evo::printlnRed("Requires an expression name");
					return;
				} break;

				case 2: {
					expr_name = args[1];
					stack_frame = &this->get_current_stack_frame();
				} break;

				case 3: {
					expr_name = args[2];

					const evo::Result<const ExecutionEngineExecutor::StackFrame*> stack_frame_res =
						this->get_stack_frame_from_arg(args[1]);

					if(stack_frame_res.isError()){ return; }

					stack_frame = stack_frame_res.value();
				} break;

				default: {
					evo::printlnRed("Too many arguments");
					return;
				} break;
			}


			const FuncFrameInfo& func_frame_info = this->get_func_frame_info(stack_frame->func_id);

			const auto register_lookup_find = func_frame_info.register_lookup.find(expr_name);
			if(register_lookup_find != func_frame_info.register_lookup.end()){
				const Expr expr = register_lookup_find->second;

				const auto register_find = stack_frame->registers.find(expr);
				if(register_find == stack_frame->registers.end()){
					evo::printlnRed("Register \"{}\" isn't set yet", expr_name);
					return;
				}

				const core::GenericValue& register_value = register_find->second;

				auto printer = core::Printer::createConsole(true);
				this->print_expr_value(register_value, stack_frame->reader_agent.getExprType(expr), printer, 0);
				printer.println();
				return;
			}

			const auto alloca_lookup_find = func_frame_info.alloca_lookup.find(expr_name);
			if(alloca_lookup_find != func_frame_info.alloca_lookup.end()){
				const Alloca& alloca_expr = alloca_lookup_find->second;

				const size_t alloca_offset = stack_frame->alloca_offsets.at(&alloca_expr);

				const core::GenericValue alloca_value = core::GenericValue::fromData(
					evo::ArrayProxy<std::byte>(
						&stack_frame->alloca_buffer[alloca_offset], this->module.numBytes(alloca_expr.type)
					)
				);

				auto printer = core::Printer::createConsole(true);
				this->print_expr_value(alloca_value, alloca_expr.type, printer, 0);
				printer.println();
				return;
			}

			const auto param_lookup_find = func_frame_info.param_lookup.find(expr_name);
			if(param_lookup_find != func_frame_info.param_lookup.end()){
				const size_t param_index = param_lookup_find->second;

				const Parameter& param = stack_frame->reader_agent.getTargetFunction().getParameters()[param_index];

				const core::GenericValue param_value = [&]() -> core::GenericValue {
					if(param.getType().kind() == Type::Kind::PTR){
						return core::GenericValue::createPtr(stack_frame->params[param_index]);

					}else{
						return core::GenericValue::fromData(
							evo::ArrayProxy<std::byte>(
								stack_frame->params[param_index], this->module.numBytes(param.getType())
							)
						);
					}
				}();



				auto printer = core::Printer::createConsole(true);
				this->print_expr_value(param_value, param.getType(), printer, 0);
				printer.println();
				return;
			}
			
			evo::printlnRed("Expression \"{}\" doesn't exist", expr_name);
		}


		auto command_print_func(evo::ArrayProxy<std::string_view> args) -> void {
			if(args.size() > 2){
				evo::printlnRed("Too many arguments");
				return;
			}


			this->setup_func_lookup_if_needed();

			const Function* target_function = nullptr;
			const ExecutionEngineExecutor::StackFrame* stack_frame = nullptr;

			if(args.size() == 1){
				target_function = &this->get_current_stack_frame().reader_agent.getTargetFunction();

			}else if(args[1][0] == '@'){
				const evo::Result<const ExecutionEngineExecutor::StackFrame*> stack_frame_res = 
					this->get_stack_frame_from_arg(args[1]);

				if(stack_frame_res.isError()){ return; }

				stack_frame = stack_frame_res.value();
				target_function = &stack_frame_res.value()->reader_agent.getTargetFunction();
				
			}else{
				const auto find = this->func_lookup.find(args[1]);

				if(find == this->func_lookup.end()){
					evo::printlnRed("Unkonwn function \"{}\"", args[1]);
					return;
				}
				
				target_function = &find->second;
			}


			auto printer = core::Printer::createString(true);

			auto module_printer = ModulePrinter(this->module, printer);
			module_printer.printFunction(*target_function);

			if(stack_frame != nullptr){
				const std::string_view target_basic_block_name = stack_frame->current_basic_block->getName();

				enum class SearchState{
					LOOKING_FOR_BASIC_BLOCK,
					LOOKING_FOR_EXPR,
					LOOKING_FOR_BEGIN_OF_EXPR,
					LOOKING_FOR_END_OF_EXPR,
					DONE,
				};

				SearchState search_state = SearchState::LOOKING_FOR_BASIC_BLOCK;

				size_t found_expressions = 0;

				for(size_t i = 0; i < printer.getString().size(); i+=1){
					switch(search_state){
						case SearchState::LOOKING_FOR_BASIC_BLOCK: {
							if(printer.getString()[i] == ':' && printer.getString()[i + 1] == '\n'){
								const auto basic_block_name_cmp_target = std::string_view(
									&printer.getString()[i - target_basic_block_name.size()],
									target_basic_block_name.size()
								);

								if(basic_block_name_cmp_target == target_basic_block_name){
									search_state = SearchState::LOOKING_FOR_EXPR;
								}
							}
						} break;

						case SearchState::LOOKING_FOR_EXPR: {
							if(printer.getString()[i] == '\n'){
								found_expressions += 1;
							}

							if(found_expressions == stack_frame->instruction_index){
								search_state = SearchState::LOOKING_FOR_BEGIN_OF_EXPR;
							}	
						} break;

						case SearchState::LOOKING_FOR_BEGIN_OF_EXPR: {
							if(printer.getString()[i] == '@' || printer.getString()[i] == '$'){
								printer.getString().insert(i, "\033[100m");
								search_state = SearchState::LOOKING_FOR_END_OF_EXPR;
								i += 5;
							}
						} break;

						case SearchState::LOOKING_FOR_END_OF_EXPR: {
							if(std::string_view(&printer.getString()[i], 4) == "\033[0m"){
								printer.getString().insert(i + 4, "\033[100m");
								i += 7;

							}else if(printer.getString()[i] == '\n'){
								printer.getString().insert(i, "\033[40m");
								search_state = SearchState::DONE;
							}
						} break;

						case SearchState::DONE: evo::unimplemented("Should have broken out of loop already");
					}

					if(search_state == SearchState::DONE){ break; }
				}
			}

			evo::print(printer.getString());
		}



		auto print_expr_value(
			const core::GenericValue& value, pir::Type type, core::Printer& printer, size_t indentation
		) -> void {
			const auto indentation_str = std::string(indentation, '\t');

			printer.print(indentation_str);

			switch(type.kind()){
				case Type::Kind::VOID: {
					printer.printCyan("Void");
				} break;

				case Type::Kind::UNSIGNED: {
					printer.printCyan("UI{}", type.getWidth());
					printer.print("(");
					printer.printMagenta(value.getInt(type.getWidth()).toString(false));
					printer.print(")");
				} break;

				case Type::Kind::SIGNED: {
					printer.printCyan("I{}", type.getWidth());
					printer.print("(");
					printer.printMagenta(value.getInt(type.getWidth()).toString(true));
					printer.print(")");
				} break;

				case Type::Kind::BOOL: {
					if(value.getBool()){
						printer.printMagenta("true");
					}else{
						printer.printMagenta("false");
					}
				} break;

				case Type::Kind::FLOAT: {
					switch(type.getWidth()){
						case 16: {
							printer.printCyan("F16");
							printer.print("(");
							printer.printMagenta(value.getF16().toString());
							printer.print(")");
						} break;

						case 32: {
							printer.printCyan("F32");
							printer.print("(");
							printer.printMagenta(value.getF32().toString());
							printer.print(")");
						} break;

						case 64: {
							printer.printCyan("F64");
							printer.print("(");
							printer.printMagenta(value.getF64().toString());
							printer.print(")");
						} break;

						case 80: {
							printer.printCyan("F80");
							printer.print("(");
							printer.printMagenta(value.getF80().toString());
							printer.print(")");
						} break;

						case 128: {
							printer.printCyan("F128");
							printer.print("(");
							printer.printMagenta(value.getF128().toString());
							printer.print(")");
						} break;
					}
				} break;

				case Type::Kind::PTR: {
					printer.printCyan("PTR");
					printer.print("(");
					printer.printMagenta("{}", value.getPtr<void*>());
					printer.print(")");
				} break;

				case Type::Kind::ARRAY: {
					const ArrayType& array_type = this->module.getArrayType(type);

					const size_t elem_size = this->module.numBytes(array_type.elemType);


					ModulePrinter(this->module, printer).printType(type);
					printer.println("[");

					for(size_t i = 0; i < array_type.length; i+=1){
						const core::GenericValue generic_value = core::GenericValue::fromData(
							value.dataRange().subarr(elem_size * i, elem_size)
						);

						this->print_expr_value(generic_value, array_type.elemType, printer, indentation + 1);

						if(i + 1 < array_type.length){
							printer.println(",");
						}else{
							printer.println();
						}
					}

					printer.print(indentation_str);
					printer.print("]");
				} break;

				case Type::Kind::STRUCT: {
					const StructType& struct_type = this->module.getStructType(type);

					ModulePrinter(this->module, printer).printType(type);
					printer.println("{");

					size_t offset = 0;

					for(size_t i = 0; Type member : struct_type.members){
						const size_t member_type_size = this->module.numBytes(member);

						const core::GenericValue generic_value = core::GenericValue::fromData(
							value.dataRange().subarr(offset, member_type_size)
						);

						this->print_expr_value(generic_value, member, printer, indentation + 1);

						if(i + 1 < struct_type.members.size()){
							printer.println(",");
						}else{
							printer.println();
						}
						
						offset += member_type_size;
						i += 1;
					}

					printer.print(indentation_str);
					printer.print("}");
				} break;

				case Type::Kind::FUNCTION: {
					printer.printlnGray("{{FUNCTION}}");
				} break;
			}
		}



		auto setup_func_lookup_if_needed() -> void {
			if(this->func_lookup_created){ return; }

			this->func_lookup.reserve(this->module.getFunctionConstIter().size());

			for(const Function& func : this->module.getFunctionConstIter()){
				this->func_lookup.emplace(func.getName(), func);
			}

			this->func_lookup_created = true;
		}


		auto get_func_frame_info(Function::ID func_id) -> FuncFrameInfo& {
			const auto find = this->func_frame_infos.find(func_id);
			if(find != this->func_frame_infos.end()){ return find->second; }

			FuncFrameInfo& func_frame_info = this->func_frame_infos.emplace(func_id, FuncFrameInfo()).first->second;

			Function& func = this->module.getFunction(func_id);

			auto reader = ReaderAgent(this->module, func);

			for(BasicBlock::ID basic_block_id : func){
				const BasicBlock& basic_block = reader.getBasicBlock(basic_block_id);

				for(Expr expr : basic_block){
					const evo::Result<std::string_view> expr_name = this->get_expr_name(expr, reader);
					if(expr_name.isError()){ continue; }

					func_frame_info.register_lookup.emplace(expr_name.value(), expr);
				}
			}

			for(const Alloca& func_alloca : func.getAllocasRange()){
				func_frame_info.alloca_lookup.emplace(func_alloca.name, func_alloca);
			}

			for(size_t i = 0; const Parameter& param : func.getParameters()){
				func_frame_info.param_lookup.emplace(param.getName(), i);
				i += 1;
			}

			return func_frame_info;
		}


		auto get_current_stack_frame() -> const ExecutionEngineExecutor::StackFrame& {
			return this->debugger.getStackFrames().back();
		}

		auto get_stack_frame_from_arg(std::string_view arg) -> evo::Result<const ExecutionEngineExecutor::StackFrame*> {
			if(arg.size() < 2 || arg[0] != '@'){
				evo::printlnRed("Invalid stack frame target \"{}\"", arg);
				return evo::resultError;
			}

			if(arg[1] == '@'){
				if(arg.size() != 2){
					evo::printlnRed("Invalid stack frame target \"{}\"", arg);
					return evo::resultError;
				}
				
				return &this->get_current_stack_frame();

			}else{
				size_t stack_frame_index = 0;

				for(size_t i = 1; i < arg.size(); i+=1){
					stack_frame_index *= 10;
					stack_frame_index += arg[i] - '0';

					if(stack_frame_index > this->debugger.getStackFrames().size()){
						evo::printlnRed("Invalid stack frame target \"{}\"", arg);
						evo::printlnRed("	Stack frame index is greater than number of stack frames");
						return evo::resultError;
					}
				}

				return &this->debugger.getStackFrames()[stack_frame_index];
			}
		}



		auto get_expr_name(Expr expr, const ReaderAgent& reader) -> evo::Result<std::string_view> {
			switch(expr.kind()){
				case Expr::Kind::NONE: evo::debugFatalBreak("Not valid expr");

				case Expr::Kind::GLOBAL_VALUE: 
					return std::string_view(reader.getModule().getGlobalVar(reader.getGlobalValue(expr)).name);

				case Expr::Kind::FUNCTION_POINTER:
					return std::string_view(reader.getModule().getFunction(reader.getFunctionPointer(expr)).getName());

				case Expr::Kind::NUMBER:  return evo::resultError;
				case Expr::Kind::BOOLEAN: return evo::resultError;
				case Expr::Kind::NULLPTR: return evo::resultError;

				case Expr::Kind::PARAM_EXPR:
					return std::string_view(
						reader.getTargetFunction().getParameters()[reader.getParamExpr(expr).index].getName()
					);

				case Expr::Kind::CALL: return std::string_view(reader.getCall(expr).name);

				case Expr::Kind::CALL_VOID:         return evo::resultError;
				case Expr::Kind::CALL_NO_RETURN:    return evo::resultError;
				case Expr::Kind::ABORT:             return evo::resultError;
				case Expr::Kind::BREAKPOINT:        return evo::resultError;
				case Expr::Kind::RET:               return evo::resultError;
				case Expr::Kind::JUMP:              return evo::resultError;
				case Expr::Kind::BRANCH:            return evo::resultError;
				case Expr::Kind::UNREACHABLE:       return evo::resultError;

				case Expr::Kind::PHI:               return std::string_view(reader.getPhi(expr).name);
				case Expr::Kind::SWITCH:            return evo::resultError;

				case Expr::Kind::ALLOCA:            return std::string_view(reader.getAlloca(expr).name);
				case Expr::Kind::LOAD:              return std::string_view(reader.getLoad(expr).name);
				case Expr::Kind::STORE:             return evo::resultError;

				case Expr::Kind::CALC_PTR:          return std::string_view(reader.getCalcPtr(expr).name);
				case Expr::Kind::MEMCPY:            return evo::resultError;
				case Expr::Kind::MEMSET:            return evo::resultError;

				case Expr::Kind::BIT_CAST:          return std::string_view(reader.getBitCast(expr).name);
				case Expr::Kind::TRUNC:             return std::string_view(reader.getTrunc(expr).name);
				case Expr::Kind::FTRUNC:            return std::string_view(reader.getFTrunc(expr).name);
				case Expr::Kind::SEXT:              return std::string_view(reader.getSExt(expr).name);
				case Expr::Kind::ZEXT:              return std::string_view(reader.getZExt(expr).name);
				case Expr::Kind::FEXT:              return std::string_view(reader.getFExt(expr).name);
				case Expr::Kind::ITOF:              return std::string_view(reader.getIToF(expr).name);
				case Expr::Kind::UITOF:             return std::string_view(reader.getUIToF(expr).name);
				case Expr::Kind::FTOI:              return std::string_view(reader.getFToI(expr).name);
				case Expr::Kind::FTOUI:             return std::string_view(reader.getFToUI(expr).name);

				case Expr::Kind::ADD:               return std::string_view(reader.getAdd(expr).name);
				case Expr::Kind::SADD_WRAP:         return evo::resultError;

				case Expr::Kind::SADD_WRAP_RESULT:  return std::string_view(reader.getSAddWrap(expr).resultName);
				case Expr::Kind::SADD_WRAP_WRAPPED: return std::string_view(reader.getSAddWrap(expr).wrappedName);
				case Expr::Kind::UADD_WRAP:         return evo::resultError;

				case Expr::Kind::UADD_WRAP_RESULT:  return std::string_view(reader.getUAddWrap(expr).resultName);
				case Expr::Kind::UADD_WRAP_WRAPPED: return std::string_view(reader.getUAddWrap(expr).wrappedName);
				case Expr::Kind::SADD_SAT:          return std::string_view(reader.getSAddSat(expr).name);
				case Expr::Kind::UADD_SAT:          return std::string_view(reader.getUAddSat(expr).name);
				case Expr::Kind::FADD:              return std::string_view(reader.getFAdd(expr).name);
				case Expr::Kind::SUB:               return std::string_view(reader.getSub(expr).name);
				case Expr::Kind::SSUB_WRAP:         return evo::resultError;

				case Expr::Kind::SSUB_WRAP_RESULT:  return std::string_view(reader.getSSubWrap(expr).resultName);
				case Expr::Kind::SSUB_WRAP_WRAPPED: return std::string_view(reader.getSSubWrap(expr).wrappedName);
				case Expr::Kind::USUB_WRAP:         return evo::resultError;

				case Expr::Kind::USUB_WRAP_RESULT:  return std::string_view(reader.getUSubWrap(expr).resultName);
				case Expr::Kind::USUB_WRAP_WRAPPED: return std::string_view(reader.getUSubWrap(expr).wrappedName);
				case Expr::Kind::SSUB_SAT:          return std::string_view(reader.getSSubSat(expr).name);
				case Expr::Kind::USUB_SAT:          return std::string_view(reader.getUSubSat(expr).name);
				case Expr::Kind::FSUB:              return std::string_view(reader.getFSub(expr).name);
				case Expr::Kind::MUL:               return std::string_view(reader.getMul(expr).name);
				case Expr::Kind::SMUL_WRAP:         return evo::resultError;

				case Expr::Kind::SMUL_WRAP_RESULT:  return std::string_view(reader.getSMulWrap(expr).resultName);
				case Expr::Kind::SMUL_WRAP_WRAPPED: return std::string_view(reader.getSMulWrap(expr).wrappedName);
				case Expr::Kind::UMUL_WRAP:         return evo::resultError;

				case Expr::Kind::UMUL_WRAP_RESULT:  return std::string_view(reader.getUMulWrap(expr).resultName);
				case Expr::Kind::UMUL_WRAP_WRAPPED: return std::string_view(reader.getUMulWrap(expr).wrappedName);
				case Expr::Kind::SMUL_SAT:          return std::string_view(reader.getSMulSat(expr).name);
				case Expr::Kind::UMUL_SAT:          return std::string_view(reader.getUMulSat(expr).name);
				case Expr::Kind::FMUL:              return std::string_view(reader.getFMul(expr).name);
				case Expr::Kind::SDIV:              return std::string_view(reader.getSDiv(expr).name);
				case Expr::Kind::UDIV:              return std::string_view(reader.getUDiv(expr).name);
				case Expr::Kind::FDIV:              return std::string_view(reader.getFDiv(expr).name);
				case Expr::Kind::SREM:              return std::string_view(reader.getSRem(expr).name);
				case Expr::Kind::UREM:              return std::string_view(reader.getURem(expr).name);
				case Expr::Kind::FREM:              return std::string_view(reader.getFRem(expr).name);
				case Expr::Kind::FNEG:              return std::string_view(reader.getFNeg(expr).name);
				case Expr::Kind::IEQ:               return std::string_view(reader.getIEq(expr).name);
				case Expr::Kind::FEQ:               return std::string_view(reader.getFEq(expr).name);
				case Expr::Kind::INEQ:              return std::string_view(reader.getINeq(expr).name);
				case Expr::Kind::FNEQ:              return std::string_view(reader.getFNeq(expr).name);
				case Expr::Kind::SLT:               return std::string_view(reader.getSLT(expr).name);
				case Expr::Kind::ULT:               return std::string_view(reader.getULT(expr).name);
				case Expr::Kind::FLT:               return std::string_view(reader.getFLT(expr).name);
				case Expr::Kind::SLTE:              return std::string_view(reader.getSLTE(expr).name);
				case Expr::Kind::ULTE:              return std::string_view(reader.getULTE(expr).name);
				case Expr::Kind::FLTE:              return std::string_view(reader.getFLTE(expr).name);
				case Expr::Kind::SGT:               return std::string_view(reader.getSGT(expr).name);
				case Expr::Kind::UGT:               return std::string_view(reader.getUGT(expr).name);
				case Expr::Kind::FGT:               return std::string_view(reader.getFGT(expr).name);
				case Expr::Kind::SGTE:              return std::string_view(reader.getSGTE(expr).name);
				case Expr::Kind::UGTE:              return std::string_view(reader.getUGTE(expr).name);
				case Expr::Kind::FGTE:              return std::string_view(reader.getFGTE(expr).name);
				case Expr::Kind::AND:               return std::string_view(reader.getAnd(expr).name);
				case Expr::Kind::OR:                return std::string_view(reader.getOr(expr).name);
				case Expr::Kind::XOR:               return std::string_view(reader.getXor(expr).name);
				case Expr::Kind::SHL:               return std::string_view(reader.getSHL(expr).name);
				case Expr::Kind::SSHL_SAT:          return std::string_view(reader.getSSHLSat(expr).name);
				case Expr::Kind::USHL_SAT:          return std::string_view(reader.getUSHLSat(expr).name);
				case Expr::Kind::SSHR:              return std::string_view(reader.getSSHR(expr).name);
				case Expr::Kind::USHR:              return std::string_view(reader.getUSHR(expr).name);
				case Expr::Kind::BIT_REVERSE:       return std::string_view(reader.getBitReverse(expr).name);
				case Expr::Kind::BYTE_SWAP:         return std::string_view(reader.getByteSwap(expr).name);
				case Expr::Kind::CTPOP:             return std::string_view(reader.getCtPop(expr).name);
				case Expr::Kind::CTLZ:              return std::string_view(reader.getCTLZ(expr).name);
				case Expr::Kind::CTTZ:              return std::string_view(reader.getCTTZ(expr).name);
				case Expr::Kind::CMPXCHG:           return evo::resultError;

				case Expr::Kind::CMPXCHG_LOADED:    return std::string_view(reader.getCmpXchg(expr).loadedName);
				case Expr::Kind::CMPXCHG_SUCCEEDED: return std::string_view(reader.getCmpXchg(expr).succeededName);
				case Expr::Kind::ATOMIC_RMW:        return std::string_view(reader.getAtomicRMW(expr).name);

				case Expr::Kind::META_LOCAL_VAR:    return evo::resultError;
			}

			evo::debugFatalBreak("Unknown expr kind");
		}

	};




	
	auto getDefaultDebugger() -> ExecutionEngine::DebuggerFunc {
		return [](ExecutionEngineDebuggerInterface& debugger, Module& module)
		-> evo::Expected<core::GenericValue, ExecutionEngineExecutor::FuncRunError::Code> {
			return DefaultDebugger(debugger, module).run();
		};
	}


}