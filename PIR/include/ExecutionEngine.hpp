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
#include "./JITEngine.hpp"


namespace pcit::pir{

	auto _internal_signal_handler(int signal) -> void;

	class ExecutionEngine{
		public:
			using FuncRunError = ExecutionEngineExecutor::FuncRunError;
			using InitConfig = JITEngine::InitConfig;

			using DebuggerFunc = std::function<
				evo::Expected<core::GenericValue, FuncRunError::Code>(class ExecutionEngineDebuggerInterface&, Module&)
			>;

		public:
			ExecutionEngine(Module& _module, uint32_t _max_call_depth = 128);
			~ExecutionEngine();


			// if returns error, not initialized
			// error is list of messages from LLVM
			[[nodiscard]] auto init(const InitConfig& config) -> evo::Expected<void, evo::SmallVector<std::string>>;
			[[nodiscard]] auto isInitialized() const -> bool { return this->jit_engine.isInitialized(); }

			auto setDefaultDebugger() -> void;
			auto setDebugger(DebuggerFunc&& _debugger_func) -> void { this->debugger_func = std::move(_debugger_func); }


			[[nodiscard]] auto maxCallDepth() const -> uint32_t { return this->max_call_depth; }


			[[nodiscard]] auto runFunction(Function::ID func_id, std::span<core::GenericValue> arguments)
				-> evo::Expected<core::GenericValue, FuncRunError>;


			[[nodiscard]] auto lookupGlobalVar(const void* ptr_of_lowered_global) const
			-> std::optional<GlobalVar::ID> {
				const auto lock = std::scoped_lock(this->global_ptr_lookup_lock);

				const auto find = this->global_ptr_lookup.find(ptr_of_lowered_global);
				if(find != this->global_ptr_lookup.end()){ return find->second; }
				return std::nullopt;
			}

			[[nodiscard]] auto lookupFunction(const void* ptr_of_lowered_function) const
			-> std::optional<Function::ID> {
				const auto lock = std::scoped_lock(this->function_ptr_lookup_lock);

				const auto find = this->function_ptr_lookup.find(ptr_of_lowered_function);
				if(find != this->function_ptr_lookup.end()){ return find->second; }
				return std::nullopt;
			}


			[[nodiscard]] auto getGlobalVarValue(GlobalVar::ID id) const -> core::GenericValue {
				const auto lock = std::scoped_lock(this->lowered_globals_lock);

				const LoweredGlobal& lowered_global = this->lowered_globals_map.at(id);
				evo::debugAssert(lowered_global.was_lowered, "Global wasn't lowered yet");

				return lowered_global.value;
			}

		
		private:
			using Executor = ExecutionEngineExecutor;
			[[nodiscard]] auto get_current_executor() -> Executor&;

			struct LoweredGlobal{
				core::GenericValue value;
				std::atomic<bool> was_lowered;
			};
			struct LoweredResult{
				bool needs_to_be_lowered;
				LoweredGlobal& lowered_global;
			};
			[[nodiscard]] auto check_global_lowered(GlobalVar::ID global_id) -> LoweredResult {
				const auto lock = std::scoped_lock(this->lowered_globals_lock);

				const auto find = this->lowered_globals_map.find(global_id);
				if(find != this->lowered_globals_map.end()){ return LoweredResult(false, find->second); }

				const GlobalVar& global_var = this->module.getGlobalVar(global_id);
				const size_t global_size = this->module.numBytes(global_var.type);

				LoweredGlobal& new_global = this->lowered_globals.emplace_back(
					core::GenericValue::createUninit(global_size), false
				);

				this->lowered_globals_map.emplace(global_id, new_global);
				return LoweredResult(true, new_global);
			}

			[[nodiscard]] auto get_atomic_lock(void* ptr) -> evo::SpinLock& {
				return this->atomic_locks[(std::bit_cast<size_t>(ptr) >> 3) % this->atomic_locks.size()];
			}

			[[nodiscard]] auto add_global_to_ptr_lookup_map(const void* ptr, GlobalVar::ID id) -> void {
				const auto lock = std::scoped_lock(this->global_ptr_lookup_lock);
				this->global_ptr_lookup.emplace(ptr, id);
			}

			[[nodiscard]] auto add_function_to_ptr_lookup_map(const void* ptr, Function::ID id) -> void {
				const auto lock = std::scoped_lock(this->function_ptr_lookup_lock);
				this->function_ptr_lookup.emplace(ptr, id);
			}


			auto run_debugger(Executor& executor)
				-> std::optional<evo::Expected<core::GenericValue, FuncRunError::Code>>;


		private:
			Module& module;
			const uint32_t max_call_depth;

			evo::StepVector<Executor> executors_alloc{};
			std::unordered_map<std::thread::id, Executor&> executors{};
			mutable evo::SpinLock executors_lock{};

			JITEngine jit_engine{};

			evo::StepVector<LoweredGlobal> lowered_globals{};
			std::unordered_map<GlobalVar::ID, LoweredGlobal&> lowered_globals_map{};
			mutable evo::SpinLock lowered_globals_lock{};

			std::unordered_map<const void*, GlobalVar::ID> global_ptr_lookup{};
			mutable evo::SpinLock global_ptr_lookup_lock{};

			std::unordered_map<const void*, Function::ID> function_ptr_lookup{};
			mutable evo::SpinLock function_ptr_lookup_lock{};

			DebuggerFunc debugger_func{};
			mutable evo::SpinLock debugger_lock;


			std::array<evo::SpinLock, 64> atomic_locks{};

			friend class ExecutionEngineExecutor;
			friend void ::pcit::pir::_internal_signal_handler(int);
	};


}


