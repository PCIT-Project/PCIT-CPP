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


#include "../include/Module.hpp"
#include "../include/InstrReader.hpp"


#if !defined(EVO_PLATFORM_WINDOWS)
	#include <csetjmp>
#endif

namespace pcit::pir{

	class ExecutionEngineExecutor{
		public:
			struct FuncRunError{
				enum class Code{
					ABORT,
					BREAKPOINT,
					EXCEEDED_MAX_CALL_DEPTH,
					OUT_OF_BOUNDS_ACCESS,
					NULLPTR_ACCESS,
					SEG_FAULT,
					ARITHMETIC_WRAP,
					FLOATING_POINT_EXCEPTION,
					UNKNOWN_EXCEPTION,
				};
				
				Code code;
				evo::SmallVector<Function::ID> stackTrace;
			};

			struct StackFrame{
				Function::ID func_id;
				BasicBlock::ID current_basic_block_id;

				InstrReader reader_agent;
				const BasicBlock* current_basic_block;

				size_t instruction_index = 0;
				evo::SmallVector<std::byte*> params{};
				std::unique_ptr<std::byte[]> alloca_buffer{};
				std::unordered_map<const Alloca*, size_t> alloca_offsets{};
				std::unordered_map<Expr, core::GenericValue> registers{};
				std::optional<BasicBlock::ID> previous_basic_block_id{};
				std::optional<pir::Expr> ret_value{};
			};


		public:
			ExecutionEngineExecutor(class ExecutionEngine& _engine) : engine(_engine) {}
			~ExecutionEngineExecutor() = default;

			auto runFunction(Function::ID func_id, std::span<core::GenericValue> arguments)
				-> evo::Expected<core::GenericValue, FuncRunError>;


			#if !defined(EVO_PLATFORM_WINDOWS)
				auto set_signal_error(FuncRunError::Code error) -> std::jmp_buf&;
			#endif


		private:
			[[nodiscard]] auto run_function_setup_and_run(Function::ID func_id, std::span<core::GenericValue> arguments)
				-> evo::Expected<core::GenericValue, FuncRunError::Code>;

			[[nodiscard]] auto run_function_impl() -> evo::Expected<core::GenericValue, FuncRunError::Code>;
			[[nodiscard]] auto run_step() -> std::optional<core::GenericValue>;

			[[nodiscard]] auto get_expr(Expr expr, StackFrame& stack_frame) -> core::GenericValue&;
			[[nodiscard]] auto get_expr_maybe_ptr(Expr expr, StackFrame& stack_frame) -> core::GenericValue*;
			[[nodiscard]] auto get_expr_ptr(Expr expr, StackFrame& stack_frame) -> std::byte*;

			[[nodiscard]] auto get_or_create_lowered_global_ptr(GlobalVar::ID id) -> std::byte*;
			auto lower_global_value(const GlobalVar::Value& value, std::span<std::byte> dst) -> void;

			auto setup_allocas(StackFrame& stack_frame) -> void;


		private:
			class ExecutionEngine& engine;

			evo::SmallVector<StackFrame> stack_frames{};
			std::optional<FuncRunError::Code> last_error{};

			#if !defined(EVO_PLATFORM_WINDOWS)
				std::jmp_buf jump_buf;
			#endif


			friend class ExecutionEngineDebuggerInterface;
	};


}


