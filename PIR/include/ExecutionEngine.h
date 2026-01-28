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
#include "./JITEngine.h"


namespace pcit::pir{

	auto _internal_signal_handler(int signal) -> void;

	class ExecutionEngine{
		public:
			using FuncRunError = ExecutionEngineExecutor::FuncRunError;
			using InitConfig = JITEngine::InitConfig;

		public:
			ExecutionEngine(Module& _module);
			~ExecutionEngine();


			// if returns error, not initialized
			// error is list of messages from LLVM
			EVO_NODISCARD auto init(const InitConfig& config) -> evo::Expected<void, evo::SmallVector<std::string>>;
			EVO_NODISCARD auto isInitialized() const -> bool { return this->jit_engine.isInitialized(); }



			auto runFunction(Function::ID func_id, std::span<core::GenericValue> arguments)
				-> evo::Expected<core::GenericValue, FuncRunError>;

		
		private:
			using Executor = ExecutionEngineExecutor;
			EVO_NODISCARD auto get_current_executor() -> Executor&;


			struct LoweredResult{
				bool needs_to_be_lowered;
				std::atomic<bool>& was_finished_being_lowered;
			};
			EVO_NODISCARD auto check_global_lowered(GlobalVar::ID global_id) -> LoweredResult {
				const auto lock = std::scoped_lock(this->lowered_lock);

				const auto find = this->lowered_globals.find(global_id);
				if(find != this->lowered_globals.end()){ return LoweredResult(false, find->second); }

				std::atomic<bool>& new_lowered_flag = this->finished_lowered_flags.emplace_back();
				this->lowered_globals.emplace(global_id, new_lowered_flag);
				return LoweredResult(true, new_lowered_flag);
			}

		private:
			Module& module;

			evo::StepVector<Executor> executors_alloc{};
			std::unordered_map<std::thread::id, Executor&> executors{};
			mutable evo::SpinLock executors_lock{};

			JITEngine jit_engine{};
			evo::StepVector<std::atomic<bool>> finished_lowered_flags{};
			std::unordered_map<GlobalVar::ID, std::atomic<bool>&> lowered_globals{};
			mutable evo::SpinLock lowered_lock{};

			friend class ExecutionEngineExecutor;
			friend void pcit::pir::_internal_signal_handler(int);
	};


}


