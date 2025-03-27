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


#include "./Module.h"
#include "./ReaderAgent.h"


namespace pcit::pir{


	class Interpreter{
		public:
			Interpreter(const Module& _module) : module(_module), reader(_module) {}
			~Interpreter() = default;

			struct ErrorInfo{
				
			};

			EVO_NODISCARD auto runFunction(Function::ID func_id, evo::SmallVector<core::GenericValue>& args)
				-> evo::Expected<core::GenericValue, ErrorInfo>;


		private:
			EVO_NODISCARD auto run() -> evo::Expected<core::GenericValue, ErrorInfo>;

			EVO_NODISCARD auto get_expr(Expr expr) -> const core::GenericValue&;
			auto set_expr(Expr expr, core::GenericValue&& value) -> void;


	
		private:
			const Module& module;
			ReaderAgent reader;

			const BasicBlock* current_basic_block = nullptr;
			size_t current_instruction_index = 0;
			core::GenericValue return_register{};

			struct StackFrame{
				const Function& func;
				std::optional<Expr> return_location;
				std::unordered_map<Expr, core::GenericValue> values{};

				const BasicBlock* basic_block = nullptr;
				size_t instruction_index = 0;

				StackFrame(const Function& _func, std::optional<Expr> _return_location)
					: func(_func), return_location(_return_location) {}
			};
			std::stack<StackFrame> stack{};

	};

}


