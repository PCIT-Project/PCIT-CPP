////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <filesystem>
namespace fs = std::filesystem;

#include <Evo.hpp>
#include <PCIT_core.hpp>
#include <Panther.hpp>
namespace core = pcit::core;
namespace panther = pcit::panther;


namespace pthr{


	auto print_logo(core::Printer& printer) -> void;
	auto print_version(core::Printer& printer) -> void;


	auto print_tokens(core::Printer& printer, const panther::Source& source, const fs::path& relative_dir) -> void;

	auto print_ast(
		core::Printer& printer, const panther::Source& source, const fs::path& relative_dir, bool legacy_mode
	) -> void;

	
	struct TimerOptions{
		bool tokenization = false;
		bool parsing = false;
		bool semantic_analysis = false;
		bool lower_to_pir_comptime = false;
		bool lower_to_pir_runtime = false;
		bool pir_optimize = false;
		bool lower_to_llvmir = false;
		bool llvmir_optimize = false;
		bool lower_to_aseembly = false;
		bool lower_to_object = false;
		bool link = false;
		bool execution = false;
	};

	auto print_timers(
		core::Printer& printer,
		const panther::Context& context,
		const core::TimerNS& total_timer,
		const TimerOptions& timer_options
	) -> void;

}