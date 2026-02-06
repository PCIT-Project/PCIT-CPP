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


#include "../include/Module.h"
#include "../include/ReaderAgent.h"


#if !defined(EVO_PLATFORM_WINDOWS)
	#include <csetjmp>
#endif

namespace pcit::pir{

	class ExecutionEngineExecutor{
		public:
			enum class FuncRunError{
				ABORT,
				EXCEEDED_MAX_CALL_DEPTH,
				BREAKPOINT,
				OUT_OF_BOUNDS_ACCESS,
				NULLPTR_ACCESS,
				SEG_FAULT,
				ARITHMETIC_WRAP,
				FLOATING_POINT_EXCEPTION,
				UNKNOWN_EXCEPTION,
			};


		public:
			ExecutionEngineExecutor(class ExecutionEngine& _engine) : engine(_engine) {}
			~ExecutionEngineExecutor() = default;

			auto runFunction(Function::ID func_id, std::span<core::GenericValue> arguments)
				-> evo::Expected<core::GenericValue, FuncRunError>;


			#if !defined(EVO_PLATFORM_WINDOWS)
				auto set_signal_error(FuncRunError error) -> std::jmp_buf&;
			#endif


		private:
			struct StackFrame{
				Function::ID func_id;
				BasicBlock::ID current_basic_block_id;

				ReaderAgent reader_agent;
				const BasicBlock* current_basic_block;

				size_t instruction_index = 0;
				evo::SmallVector<std::byte*> params{};
				std::unique_ptr<std::byte[]> alloca_buffer{};
				std::unordered_map<const Alloca*, size_t> alloca_offsets{};
				std::unordered_map<Expr, core::GenericValue> registers{};
				std::optional<BasicBlock::ID> previous_basic_block_id{};
				std::optional<pir::Expr> ret_value{};
			};

			EVO_NODISCARD auto run_function_impl() -> evo::Expected<core::GenericValue, FuncRunError>;

			EVO_NODISCARD auto get_expr(Expr expr, StackFrame& stack_frame) -> core::GenericValue&;
			EVO_NODISCARD auto get_expr_maybe_ptr(Expr expr, StackFrame& stack_frame) -> core::GenericValue*;
			EVO_NODISCARD auto get_expr_ptr(Expr expr, StackFrame& stack_frame) -> std::byte*;

			auto setup_allocas(StackFrame& stack_frame) -> void;


		private:
			class ExecutionEngine& engine;

			evo::SmallVector<StackFrame> stack_frames{};

			#if !defined(EVO_PLATFORM_WINDOWS)
				std::atomic<std::optional<FuncRunError>> signal_error{};
				std::jmp_buf jump_buf;
			#endif
	};


}


