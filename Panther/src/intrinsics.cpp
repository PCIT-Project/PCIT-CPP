////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/intrinsics.hpp"

#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

#include "../include/TypeManager.hpp"


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
			{"abort",                        Kind::ABORT},
			{"breakpoint",                   Kind::BREAKPOINT},
			{"panic",                        Kind::PANIC},
			{"_entry",                       Kind::ENTRY},

			{"isComptime",                   Kind::IS_COMPTIME},
			{"_ctPrint",                     Kind::CT_PRINT},
			{"_ctPrintln",                   Kind::CT_PRINTLN},
			{"_ctAlloc",                     Kind::CT_ALLOC},
			{"_ctDealloc",                   Kind::CT_DEALLOC},

			// type traits
			{"_ctGetIntegerTypeID",          Kind::CT_GET_INTEGER_TYPE_ID},

			// build system
			{"_createPantherBuild",          Kind::CREATE_PANTHER_BUILD},

			// misc
			{"compilerExecutableDirectory", Kind::COMPILER_EXECUTABLE_DIRECTORY},
			{"compileWorkingDirectory",     Kind::COMPILE_WORKING_DIRECTORY},
			{"setModuleName",                Kind::SET_MODULE_NAME},
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
			{"_getTypeID",                       Kind::GET_TYPE_ID},
			{"_arrayElementTypeID",              Kind::ARRAY_ELEMENT_TYPE_ID},
			{"_arrayRefElementTypeID",           Kind::ARRAY_REF_ELEMENT_TYPE_ID},
			{"_numBytes",                        Kind::NUM_BYTES},
			{"_numBits",                         Kind::NUM_BITS},
			{"_numAlignBytes",                   Kind::NUM_ALIGN_BYTES},
			{"_getIntegerTypeID",                Kind::GET_INTEGER_TYPE_ID},
			{"_isDefaultInitializable",          Kind::IS_DEFAULT_INITIALIZABLE},
			{"_isTriviallyDefaultInitializable", Kind::IS_TRIVIALLY_DEFAULT_INITIALIZABLE},
			{"_isComptimeDefaultInitializable",  Kind::IS_COMPTIME_DEFAULT_INITIALIZABLE},
			{"_isRuntimeDefaultInitializable",   Kind::IS_RUNTIME_DEFAULT_INITIALIZABLE},
			{"_isNoErrorDefaultInitializable",   Kind::IS_NO_ERROR_DEFAULT_INITIALIZABLE},
			{"_isSafeDefaultInitializable",      Kind::IS_SAFE_DEFAULT_INITIALIZABLE},
			{"_isTriviallyDeletable",            Kind::IS_TRIVIALLY_DELETABLE},
			{"_isComptimeDeletable",             Kind::IS_COMPTIME_DELETABLE},
			{"_isRuntimeDeletable",              Kind::IS_RUNTIME_DELETABLE},
			{"_isCopyable",                      Kind::IS_COPYABLE},
			{"_isTriviallyCopyable",             Kind::IS_TRIVIALLY_COPYABLE},
			{"_isComptimeCopyable",              Kind::IS_COMPTIME_COPYABLE},
			{"_isRuntimeCopyable",               Kind::IS_RUNTIME_COPYABLE},
			{"_isNoErrorCopyable",               Kind::IS_NO_ERROR_COPYABLE},
			{"_isSafeCopyable",                  Kind::IS_SAFE_COPYABLE},
			{"_isMovable",                       Kind::IS_MOVABLE},
			{"_isTriviallyMovable",              Kind::IS_TRIVIALLY_MOVABLE},
			{"_isComptimeMovable",               Kind::IS_COMPTIME_MOVABLE},
			{"_isRuntimeMovable",                Kind::IS_RUNTIME_MOVABLE},
			{"_isNoErrorMovable",                Kind::IS_NO_ERROR_MOVABLE},
			{"_isSafeMovable",                   Kind::IS_SAFE_MOVABLE},
			{"_isComparable",                    Kind::IS_COMPARABLE},
			{"_isTriviallyComparable",           Kind::IS_TRIVIALLY_COMPARABLE},
			{"_isComptimeComparable",            Kind::IS_COMPTIME_COMPARABLE},
			{"_isRuntimeComparable",             Kind::IS_RUNTIME_COMPARABLE},
			{"_isNoErrorComparable",             Kind::IS_NO_ERROR_COMPARABLE},
			{"_isSafeComparable",                Kind::IS_SAFE_COMPARABLE},
			{"_isIntegral",                      Kind::IS_INTEGRAL},
			{"_isSignedIntegral",                Kind::IS_SIGNED_INTEGRAL},
			{"_isUnsignedIntegral",              Kind::IS_UNSIGNED_INTEGRAL},
			{"_isFloatingPoint",                 Kind::IS_FLOATING_POINT},
			{"_isPointer",                       Kind::IS_POINTER},

			{"_bitCast",                         Kind::BIT_CAST},
			{"_trunc",                           Kind::TRUNC},
			{"_ftrunc",                          Kind::FTRUNC},
			{"_sext",                            Kind::SEXT},
			{"_zext",                            Kind::ZEXT},
			{"_fext",                            Kind::FEXT},
			{"_iToF",                            Kind::I_TO_F},
			{"_fToI",                            Kind::F_TO_I},

			{"_add",                             Kind::ADD},
			{"_addWrap",                         Kind::ADD_WRAP},
			{"_addSat",                          Kind::ADD_SAT},
			{"_fadd",                            Kind::FADD},
			{"_sub",                             Kind::SUB},
			{"_subWrap",                         Kind::SUB_WRAP},
			{"_subSat",                          Kind::SUB_SAT},
			{"_fsub",                            Kind::FSUB},
			{"_mul",                             Kind::MUL},
			{"_mulWrap",                         Kind::MUL_WRAP},
			{"_mulSat",                          Kind::MUL_SAT},
			{"_fmul",                            Kind::FMUL},
			{"_div",                             Kind::DIV},
			{"_fdiv",                            Kind::FDIV},
			{"_rem",                             Kind::REM},
			{"_fneg",                            Kind::FNEG},

			{"_eq",                              Kind::EQ},
			{"_neq",                             Kind::NEQ},
			{"_lt",                              Kind::LT},
			{"_lte",                             Kind::LTE},
			{"_gt",                              Kind::GT},
			{"_gte",                             Kind::GTE},

			{"_and",                             Kind::AND},
			{"_or",                              Kind::OR},
			{"_xor",                             Kind::XOR},
			{"_shl",                             Kind::SHL},
			{"_shlSat",                          Kind::SHL_SAT},
			{"_shr",                             Kind::SHR},
			{"_bitReverse",                      Kind::BIT_REVERSE},
			{"_byteSwap",                        Kind::BYTE_SWAP},
			{"_ctPop",                           Kind::CTPOP},
			{"_ctlz",                            Kind::CTLZ},
			{"_cttz",                            Kind::CTTZ},

			{"_atomicLoad",                      Kind::ATOMIC_LOAD},
			{"_atomicStore",                     Kind::ATOMIC_STORE},
			{"_cmpxchg",                         Kind::CMPXCHG},
			{"_atomicRMW",                       Kind::ATOMIC_RMW},

			{"_wasmMemoryGrow",                  Kind::WASM_MEMORY_GROW},
			{"_wasmMemorySize",                  Kind::WASM_MEMORY_SIZE},

			{"makeComptimeBuffer",               Kind::MAKE_COMPTIME_BUFFER},
		};

		template_intrinsic_kinds_end = template_intrinsic_kinds.end();
	}

}