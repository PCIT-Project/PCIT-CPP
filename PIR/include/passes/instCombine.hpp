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

#include "../PassManager.hpp"

namespace pcit::pir::passes{

	// 1 + 2 --> 3
	[[nodiscard]] auto constant_folding_impl(Expr stmt, const class InstrHandler& handler)
		-> PassManager::MadeTransformation;

	[[nodiscard]] inline auto constantFolding() -> PassManager::StmtPass {
		return PassManager::StmtPass(constant_folding_impl);
	}


	// x + 0 --> x
	// x - x --> 0
	[[nodiscard]] auto inst_simplify_impl(Expr stmt, const class InstrHandler& handler)
		-> PassManager::MadeTransformation;

	[[nodiscard]] inline auto instSimplify() -> PassManager::StmtPass {
		return PassManager::StmtPass(inst_simplify_impl);
	}


	// x * 4 --> x << 2
	[[nodiscard]] auto inst_combine_impl(Expr stmt, const class InstrHandler& handler)
		-> PassManager::MadeTransformation;

	[[nodiscard]] inline auto instCombine() -> PassManager::StmtPassGroup {
		return PassManager::StmtPassGroup({
			constantFolding(),
			instSimplify(),
			PassManager::StmtPass(inst_combine_impl),
		});
	}

}


