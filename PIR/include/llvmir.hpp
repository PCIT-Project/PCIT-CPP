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

#include "./enums.hpp"


namespace llvm{
	class LLVMContext;
	class Module;
}


namespace pcit::pir{

	auto lowerToLLVMIR(
		const class Module& module,
		OptMode opt_mode = OptMode::NONE,
		bool add_debug_info = true,
		llvm::LLVMContext* llvm_context = nullptr,
		evo::SmallVector<llvm::Module*>&& modules = {},
		core::TimerNS* lower_to_llvmir_timer = nullptr,
		core::TimerNS* optimize_timer = nullptr,
		core::TimerNS* link_timer = nullptr
	) -> std::string;

	auto lowerToAssembly(
		const class Module& module,
		OptMode opt_mode = OptMode::NONE,
		bool add_debug_info = true,
		llvm::LLVMContext* llvm_context = nullptr,
		evo::SmallVector<llvm::Module*>&& modules = {},
		core::TimerNS* lower_to_llvmir_timer = nullptr,
		core::TimerNS* optimize_timer = nullptr,
		core::TimerNS* link_timer = nullptr,
		core::TimerNS* lower_to_assembly_timer = nullptr
	) -> evo::Result<std::string>;

	auto lowerToObject(
		const class Module& module,
		OptMode opt_mode = OptMode::NONE,
		bool add_debug_info = true,
		llvm::LLVMContext* llvm_context = nullptr,
		evo::SmallVector<llvm::Module*>&& modules = {},
		core::TimerNS* lower_to_llvmir_timer = nullptr,
		core::TimerNS* optimize_timer = nullptr,
		core::TimerNS* link_timer = nullptr,
		core::TimerNS* lower_to_object_timer = nullptr
	) -> evo::Result<std::vector<evo::byte>>;

}

