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
			{"abort",                   Kind::ABORT},
			{"breakpoint",              Kind::BREAKPOINT},
			{"buildSetNumThreads",      Kind::BUILD_SET_NUM_THREADS},
			{"buildSetOutput",          Kind::BUILD_SET_OUTPUT},
			{"buildSetStdLibPackage",   Kind::BUILD_SET_STD_LIB_PACKAGE},
			{"buildCreatePackage",      Kind::BUILD_CREATE_PACKAGE},
			{"buildAddSourceFile",      Kind::BUILD_ADD_SOURCE_FILE},
			{"buildAddSourceDirectory", Kind::BUILD_ADD_SOURCE_DIRECTORY},
			{"buildAddCHeaderFile",     Kind::BUILD_ADD_C_HEADER_FILE},
			{"buildAddCPPHeaderFile",   Kind::BUILD_ADD_CPP_HEADER_FILE},
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
			{"getTypeID",                       Kind::GET_TYPE_ID},
			{"arrayElementTypeID",              Kind::ARRAY_ELEMENT_TYPE_ID},
			{"arrayRefElementTypeID",           Kind::ARRAY_REF_ELEMENT_TYPE_ID},
			{"numBytes",                        Kind::NUM_BYTES},
			{"numBits",                         Kind::NUM_BITS},
			{"isDefaultInitializable",          Kind::IS_DEFAULT_INITIALIZABLE},
			{"isTriviallyDefaultInitializable", Kind::IS_TRIVIALLY_DEFAULT_INITIALIZABLE},
			{"isComptimeDefaultInitializable",  Kind::IS_COMPTIME_DEFAULT_INITIALIZABLE},
			{"isNoErrorDefaultInitializable",   Kind::IS_NO_ERROR_DEFAULT_INITIALIZABLE},
			{"isSafeDefaultInitializable",      Kind::IS_SAFE_DEFAULT_INITIALIZABLE},
			{"isTriviallyDeletable",            Kind::IS_TRIVIALLY_DELETABLE},
			{"isComptimeDeletable",             Kind::IS_COMPTIME_DELETABLE},
			{"isCopyable",                      Kind::IS_COPYABLE},
			{"isTriviallyCopyable",             Kind::IS_TRIVIALLY_COPYABLE},
			{"isComptimeCopyable",              Kind::IS_COMPTIME_COPYABLE},
			{"isNoErrorCopyable",               Kind::IS_NO_ERROR_COPYABLE},
			{"isSafeCopyable",                  Kind::IS_SAFE_COPYABLE},
			{"isMovable",                       Kind::IS_MOVABLE},
			{"isTriviallyMovable",              Kind::IS_TRIVIALLY_MOVABLE},
			{"isComptimeMovable",               Kind::IS_COMPTIME_MOVABLE},
			{"isNoErrorMovable",                Kind::IS_NO_ERROR_MOVABLE},
			{"isSafeMovable",                   Kind::IS_SAFE_MOVABLE},
			{"isComparable",                    Kind::IS_COMPARABLE},
			{"isTriviallyComparable",           Kind::IS_TRIVIALLY_COMPARABLE},
			{"isComptimeComparable",            Kind::IS_COMPTIME_COMPARABLE},
			{"isNoErrorComparable",             Kind::IS_NO_ERROR_COMPARABLE},
			{"isSafeComparable",                Kind::IS_SAFE_COMPARABLE},

			{"bitCast",                         Kind::BIT_CAST},
			{"trunc",                           Kind::TRUNC},
			{"ftrunc",                          Kind::FTRUNC},
			{"sext",                            Kind::SEXT},
			{"zext",                            Kind::ZEXT},
			{"fext",                            Kind::FEXT},
			{"iToF",                            Kind::I_TO_F},
			{"fToI",                            Kind::F_TO_I},

			{"add",                             Kind::ADD},
			{"addWrap",                         Kind::ADD_WRAP},
			{"addSat",                          Kind::ADD_SAT},
			{"fadd",                            Kind::FADD},
			{"sub",                             Kind::SUB},
			{"subWrap",                         Kind::SUB_WRAP},
			{"subSat",                          Kind::SUB_SAT},
			{"fsub",                            Kind::FSUB},
			{"mul",                             Kind::MUL},
			{"mulWrap",                         Kind::MUL_WRAP},
			{"mulSat",                          Kind::MUL_SAT},
			{"fmul",                            Kind::FMUL},
			{"div",                             Kind::DIV},
			{"fdiv",                            Kind::FDIV},
			{"rem",                             Kind::REM},
			{"fneg",                            Kind::FNEG},

			{"eq",                              Kind::EQ},
			{"neq",                             Kind::NEQ},
			{"lt",                              Kind::LT},
			{"lte",                             Kind::LTE},
			{"gt",                              Kind::GT},
			{"gte",                             Kind::GTE},

			{"and",                             Kind::AND},
			{"or",                              Kind::OR},
			{"xor",                             Kind::XOR},
			{"shl",                             Kind::SHL},
			{"shlSat",                          Kind::SHL_SAT},
			{"shr",                             Kind::SHR},
			{"bitReverse",                      Kind::BIT_REVERSE},
			{"byteSwap",                        Kind::BYTE_SWAP},
			{"ctPop",                           Kind::CTPOP},
			{"ctlz",                            Kind::CTLZ},
			{"cttz",                            Kind::CTTZ},

			{"atomicLoad",                      Kind::ATOMIC_LOAD},
			{"atomicStore",                     Kind::ATOMIC_STORE},
			{"cmpxchg",                         Kind::CMPXCHG},
			{"atomicRMW",                       Kind::ATOMIC_RMW},
		};

		template_intrinsic_kinds_end = template_intrinsic_kinds.end();
	}

}