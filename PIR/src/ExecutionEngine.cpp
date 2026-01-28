////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/ExecutionEngine.h"

#include <csignal>


namespace pcit::pir{

	#if defined(EVO_PLATFORM_WINDOWS)

		ExecutionEngine::ExecutionEngine(Module& _module) : module(_module) {}

		ExecutionEngine::~ExecutionEngine(){
			if(this->jit_engine.isInitialized()){ this->jit_engine.deinit(); }
		}

	#else

		static std::atomic<ExecutionEngine*> execution_engine_signal_ptr = nullptr;
		static std::atomic<bool> installed_signal_handlers = false;

		auto _internal_signal_handler(int signal) -> void {
			ExecutionEngine* engine = execution_engine_signal_ptr.load();
			if(engine == nullptr){ return; }

			const auto func_run_error = [&]() -> ExecutionEngineExecutor::FuncRunError {
				switch(signal){
					break; case SIGTERM: return ExecutionEngineExecutor::FuncRunError::UNKNOWN_EXCEPTION;
					break; case SIGSEGV: return ExecutionEngineExecutor::FuncRunError::SEG_FAULT;
					break; case SIGINT:  return ExecutionEngineExecutor::FuncRunError::UNKNOWN_EXCEPTION;
					break; case SIGILL:  return ExecutionEngineExecutor::FuncRunError::UNKNOWN_EXCEPTION;
					break; case SIGABRT: return ExecutionEngineExecutor::FuncRunError::ABORT;
					break; case SIGFPE:  return ExecutionEngineExecutor::FuncRunError::FLOATING_POINT_EXCEPTION;
					break; default:      return ExecutionEngineExecutor::FuncRunError::UNKNOWN_EXCEPTION;
				}
			}();

			const auto lock = std::scoped_lock(engine->executors_lock);
			std::longjmp(engine->executors.at(std::this_thread::get_id()).set_signal_error(func_run_error), true);
		}


		ExecutionEngine::ExecutionEngine(Module& _module) : module(_module)	{
			ExecutionEngine* expected = nullptr;
			if(execution_engine_signal_ptr.compare_exchange_strong(expected, this) == false){
				evo::debugFatalBreak("Execution engine with signal handler already exists");
			}

			
			if(installed_signal_handlers.exchange(true) == false){
				std::signal(SIGTERM, _internal_signal_handler);
				std::signal(SIGSEGV, _internal_signal_handler);
				std::signal(SIGINT, _internal_signal_handler);
				std::signal(SIGILL, _internal_signal_handler);
				std::signal(SIGABRT, _internal_signal_handler);
				std::signal(SIGFPE, _internal_signal_handler);
			}
		}


		ExecutionEngine::~ExecutionEngine(){
			execution_engine_signal_ptr = nullptr;

			if(this->jit_engine.isInitialized()){ this->jit_engine.deinit(); }
		}

	#endif



	auto ExecutionEngine::init(const InitConfig& config) -> evo::Expected<void, evo::SmallVector<std::string>> {
		return this->jit_engine.init(config);
	}



	auto ExecutionEngine::runFunction(Function::ID func_id, std::span<core::GenericValue> arguments)
	-> evo::Expected<core::GenericValue, FuncRunError> {
		Executor& current_executor = this->get_current_executor();
		return current_executor.runFunction(func_id, arguments);
	}


	auto ExecutionEngine::get_current_executor() -> Executor& {
		const std::thread::id this_id = std::this_thread::get_id();

		const auto lock = std::scoped_lock(this->executors_lock);

		const auto find = this->executors.find(this_id);
		if(find != this->executors.end()){ return find->second; }

		Executor& created_executor = this->executors_alloc.emplace_back(*this);
		this->executors.emplace(this_id, created_executor);
		return created_executor;
	}


}