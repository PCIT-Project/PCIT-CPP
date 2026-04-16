////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.hpp>
#include <PCIT_core.hpp>


#include "./Module.hpp"
#include "../src/ExecutionEngineExecutor.hpp"


namespace pcit::pir{

	class ExecutionEngineDebuggerInterface{
		public:
			ExecutionEngineDebuggerInterface(class ExecutionEngineExecutor& _executor) : executor(_executor) {}
			~ExecutionEngineDebuggerInterface() = default;


			[[nodiscard]] auto getStackFrames() const -> evo::ArrayProxy<ExecutionEngineExecutor::StackFrame>;

			[[nodiscard]] auto getLastErrorCode() const -> std::optional<ExecutionEngineExecutor::FuncRunError::Code>;

			[[nodiscard]] auto continueExecution() const
				-> evo::Expected<core::GenericValue, ExecutionEngineExecutor::FuncRunError::Code>;

			[[nodiscard]] auto stepExecution() const -> std::optional<core::GenericValue>;

			[[nodiscard]] auto getStackTrace() const -> evo::SmallVector<pir::Function::ID>;


		private:
			[[nodiscard]] auto may_continue_execution() const -> bool;

	
		private:
			class ExecutionEngineExecutor& executor;
			
	};



}


