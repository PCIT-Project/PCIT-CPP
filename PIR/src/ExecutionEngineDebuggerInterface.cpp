////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/ExecutionEngineDebuggerInterface.h"

#include "./ExecutionEngineExecutor.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::pir{


	auto ExecutionEngineDebuggerInterface::getStackFrames() const
	-> evo::ArrayProxy<ExecutionEngineExecutor::StackFrame> {
		return this->executor.stack_frames;
	}

	auto ExecutionEngineDebuggerInterface::getLastErrorCode() const
	-> std::optional<ExecutionEngineExecutor::FuncRunError::Code> {
		return this->executor.last_error;	
	}


	auto ExecutionEngineDebuggerInterface::continueExecution() const
	-> evo::Expected<core::GenericValue, ExecutionEngineExecutor::FuncRunError::Code> {
		if(this->may_continue_execution() == false){
			return evo::Unexpected(*this->executor.last_error);
		}

		this->executor.last_error.reset();
		// this->executor.stack_frames.back().instruction_index += 1;
		return this->executor.run_function_impl();
	}

	auto ExecutionEngineDebuggerInterface::stepExecution() const -> std::optional<core::GenericValue> {
		if(this->may_continue_execution() == false){
			return std::nullopt;
		}

		this->executor.last_error.reset();
		// this->executor.stack_frames.back().instruction_index += 1;
		return this->executor.run_step();
	}




	
	auto ExecutionEngineDebuggerInterface::getStackTrace() const -> evo::SmallVector<pir::Function::ID> {
		auto stack_trace = evo::SmallVector<Function::ID>();

		for(const ExecutionEngineExecutor::StackFrame& stack_frame : this->executor.stack_frames){
			stack_trace.emplace_back(stack_frame.func_id);
		}

		return stack_trace;
	}


	auto ExecutionEngineDebuggerInterface::may_continue_execution() const -> bool {
		return this->executor.last_error.has_value() == false
			|| *this->executor.last_error == ExecutionEngineExecutor::FuncRunError::Code::BREAKPOINT;
	}


}