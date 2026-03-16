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
#include "../src/ExecutionEngineExecutor.h"


namespace pcit::pir{

	class ExecutionEngineDebuggerInterface{
		public:
			ExecutionEngineDebuggerInterface(class ExecutionEngineExecutor& _executor) : executor(_executor) {}
			~ExecutionEngineDebuggerInterface() = default;


			EVO_NODISCARD auto getStackFrames() const -> evo::ArrayProxy<ExecutionEngineExecutor::StackFrame>;

			EVO_NODISCARD auto getLastErrorCode() const -> std::optional<ExecutionEngineExecutor::FuncRunError::Code>;

			EVO_NODISCARD auto continueExecution() const
				-> evo::Expected<core::GenericValue, ExecutionEngineExecutor::FuncRunError::Code>;

			EVO_NODISCARD auto stepExecution() const -> std::optional<core::GenericValue>;

			EVO_NODISCARD auto getStackTrace() const -> evo::SmallVector<pir::Function::ID>;


		private:
			EVO_NODISCARD auto may_continue_execution() const -> bool;

	
		private:
			class ExecutionEngineExecutor& executor;
			
	};



}


