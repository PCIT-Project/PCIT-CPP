////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/intrinsics.h"

#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

#include "../include/TypeManager.h"


namespace pcit::panther{


	//////////////////////////////////////////////////////////////////////
	// intrinsics

	std::atomic<bool> intrinsic_lookup_tables_initialized = false;
	static std::unordered_map<std::string_view, IntrinsicFunc::Kind> intrinsic_kinds{};
	static std::optional<std::unordered_map<std::string_view, IntrinsicFunc::Kind>::iterator> intrinsic_kinds_end{};


	auto IntrinsicFunc::lookupKind(std::string_view name) -> std::optional<Kind> {
		evo::debugAssert(intrinsic_lookup_tables_initialized.load(), "IntrinsicFunc lookup tables weren't initialized");

		const auto find = intrinsic_kinds.find(name);
		if(find == intrinsic_kinds_end){ return std::nullopt; }
		return find->second;
	}


	auto IntrinsicFunc::initLookupTableIfNeeded() -> void {
		const bool was_initialized = intrinsic_lookup_tables_initialized.exchange(true);
		if(was_initialized){ return; }

		intrinsic_kinds = std::unordered_map<std::string_view, Kind>{
			{"abort",              Kind::ABORT},
			{"breakpoint",         Kind::BREAKPOINT},
			{"buildSetNumThreads", Kind::BUILD_SET_NUM_THREADS},
			{"buildSetOutput",     Kind::BUILD_SET_OUTPUT},
			{"buildSetUseStdLib",  Kind::BUILD_SET_USE_STD_LIB},
		};

		intrinsic_kinds_end = intrinsic_kinds.end();
	}


	//////////////////////////////////////////////////////////////////////
	// templated intrinsics


	std::atomic<bool> template_intrinsic_lookup_tables_initialized = false;
	static std::unordered_map<std::string_view, TemplateIntrinsicFunc::Kind> template_intrinsic_kinds{};
	static std::optional<
		std::unordered_map<std::string_view, TemplateIntrinsicFunc::Kind>::iterator
	> template_intrinsic_kinds_end{};


	auto TemplateIntrinsicFunc::lookupKind(std::string_view name) -> std::optional<Kind> {
		evo::debugAssert(
			template_intrinsic_lookup_tables_initialized.load(),
			"TemplateIntrinsicFunc lookup tables weren't initialized"
		);

		const auto find = template_intrinsic_kinds.find(name);
		if(find == template_intrinsic_kinds_end){ return std::nullopt; }
		return find->second;
	}


	auto TemplateIntrinsicFunc::initLookupTableIfNeeded() -> void {
		const bool was_initialized = template_intrinsic_lookup_tables_initialized.exchange(true);
		if(was_initialized){ return; }

		template_intrinsic_kinds = std::unordered_map<std::string_view, Kind>{
			{"sizeOf", Kind::SIZE_OF},
		};

		template_intrinsic_kinds_end = template_intrinsic_kinds.end();
	}

}