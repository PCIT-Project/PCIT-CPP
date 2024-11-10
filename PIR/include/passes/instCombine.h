//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>

#include <PCIT_core.h>

#include "../PassManager.h"

namespace pcit::pir::passes{

	// 1 + 2 --> 3
	EVO_NODISCARD auto constant_folding_impl(class Expr& stmt, const class Agent& agent) -> bool;

	EVO_NODISCARD inline auto constantFolding() -> PassManager::StmtPass {
		return PassManager::StmtPass(constant_folding_impl);
	}


	// x + 0 --> x
	// x - x --> 0
	EVO_NODISCARD auto inst_simplify_impl(class Expr& stmt, const class Agent& agent) -> bool;

	EVO_NODISCARD inline auto instSimplify() -> PassManager::StmtPass {
		return PassManager::StmtPass(inst_simplify_impl);
	}


	// x * 4 --> x << 2
	EVO_NODISCARD auto inst_combine_impl(class Expr& stmt, const class Agent& agent) -> bool;

	EVO_NODISCARD inline auto instCombine() -> PassManager::StmtPassGroup {
		return PassManager::StmtPassGroup({
			constantFolding(),
			instSimplify(),
			PassManager::StmtPass(inst_combine_impl),
		});
	}

}


