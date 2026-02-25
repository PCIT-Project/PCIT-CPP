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
			ExecutionEngine(Module& _module, uint32_t _max_call_depth = 128);
			~ExecutionEngine();


			// if returns error, not initialized
			// error is list of messages from LLVM
			EVO_NODISCARD auto init(const InitConfig& config) -> evo::Expected<void, evo::SmallVector<std::string>>;
			EVO_NODISCARD auto isInitialized() const -> bool { return this->jit_engine.isInitialized(); }

			EVO_NODISCARD auto maxCallDepth() const -> uint32_t { return this->max_call_depth; }


			EVO_NODISCARD auto runFunction(Function::ID func_id, std::span<core::GenericValue> arguments)
				-> evo::Expected<core::GenericValue, FuncRunError>;


			EVO_NODISCARD auto lookupGlobalVar(const void* ptr_of_lowered_global) -> std::optional<GlobalVar::ID> {
				const auto lock = std::scoped_lock(this->global_ptr_lookup_lock);

				const auto find = this->global_ptr_lookup.find(ptr_of_lowered_global);
				if(find != this->global_ptr_lookup.end()){ return find->second; }
				return std::nullopt;
			}

		
		private:
			using Executor = ExecutionEngineExecutor;
			EVO_NODISCARD auto get_current_executor() -> Executor&;


			struct LoweredResult{
				bool needs_to_be_lowered;
				std::atomic<bool>& was_finished_being_lowered;
			};
			EVO_NODISCARD auto check_global_lowered(GlobalVar::ID global_id) -> LoweredResult {
				const auto lock = std::scoped_lock(this->lowered_globals_lock);

				const auto find = this->lowered_globals.find(global_id);
				if(find != this->lowered_globals.end()){ return LoweredResult(false, find->second); }

				std::atomic<bool>& new_lowered_flag = this->finished_lowered_flags.emplace_back();
				this->lowered_globals.emplace(global_id, new_lowered_flag);
				return LoweredResult(true, new_lowered_flag);
			}

			EVO_NODISCARD auto get_atomic_lock(void* ptr) -> evo::SpinLock& {
				return this->atomic_locks[(std::bit_cast<size_t>(ptr) >> 3) % this->atomic_locks.size()];
			}

			EVO_NODISCARD auto add_global_to_ptr_lookup_map(const void* ptr, GlobalVar::ID id) -> void {
				const auto lock = std::scoped_lock(this->global_ptr_lookup_lock);
				this->global_ptr_lookup.emplace(ptr, id);
			}


		private:
			Module& module;
			const uint32_t max_call_depth;

			evo::StepVector<Executor> executors_alloc{};
			std::unordered_map<std::thread::id, Executor&> executors{};
			mutable evo::SpinLock executors_lock{};

			JITEngine jit_engine{};

			evo::StepVector<std::atomic<bool>> finished_lowered_flags{};
			std::unordered_map<GlobalVar::ID, std::atomic<bool>&> lowered_globals{};
			mutable evo::SpinLock lowered_globals_lock{};

			std::unordered_map<const void*, GlobalVar::ID> global_ptr_lookup{};
			mutable evo::SpinLock global_ptr_lookup_lock{};


			std::array<evo::SpinLock, 64> atomic_locks{};

			friend class ExecutionEngineExecutor;
			friend void ::pcit::pir::_internal_signal_handler(int);
	};


}


