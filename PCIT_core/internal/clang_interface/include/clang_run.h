////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>

#include "./API.h"
#include "./DiagnosticList.h"
#include "../../../include/Target.h"
#include "../../llvm_interface/include/class_impls/native_ptr_decls.h"


namespace pcit::clangint{

	struct COpts{
		enum class Standard{
			LATEST,
			C23,
			C17,
			C11,
			C99,
			C89,
		};

		Standard standard = Standard::LATEST;
	};

	struct CPPOpts{
		enum class Standard{
			LATEST,
			CPP23,
			CPP20,
			CPP17,
			CPP14,
			CPP11,
		};

		Standard standard = Standard::LATEST;
	};


	auto getHeaderAPI(
		const std::string& file_name,
		std::string_view file_data,
		evo::Variant<COpts, CPPOpts> opts,
		core::Target target,
		DiagnosticList& diagnostic_list,
		API& api
	) -> evo::Result<>;



	auto getSourceLLVM(
		const std::string& file_name,
		std::string_view file_data,
		evo::Variant<COpts, CPPOpts> opts,
		core::Target target,
		llvm::LLVMContext* llvm_context,
		DiagnosticList& diagnostic_list
	) -> evo::Result<llvm::Module*>; // consumer must delete the result

	

}