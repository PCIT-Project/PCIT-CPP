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

	auto _internal_signal_handler(int signal) -> void;

	class ExecutionEngine{
		public:
			using FuncRunError = ExecutionEngineExecutor::FuncRunError;

		public:
			#if defined(EVO_PLATFORM_WINDOWS)
				ExecutionEngine(Module& _module) : module(_module) {}
				~ExecutionEngine() = default;
			#else
				ExecutionEngine(Module& _module);
				~ExecutionEngine();
			#endif

			auto runFunction(Function::ID func_id, std::span<core::GenericValue> arguments)
				-> evo::Expected<core::GenericValue, FuncRunError>;

		
		private:
			using Executor = ExecutionEngineExecutor;
			EVO_NODISCARD auto get_current_executor() -> Executor&;

		private:
			Module& module;

			evo::StepVector<Executor> executors_alloc{};
			std::unordered_map<std::thread::id, Executor&> executors{};
			mutable evo::SpinLock executors_lock{};

			friend class ExecutionEngineExecutor;
			friend void pcit::pir::_internal_signal_handler(int);
	};


}


