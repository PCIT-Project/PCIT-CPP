////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once



namespace pcit::panther{

	
	struct WarningContext{
		bool methodCallOnNonMethod        = true;
		bool memberTypeByValueAccessor    = true;
		bool deleteMovedFromExpr          = true;
		bool deleteTriviallyDeletableType = true;
		bool comptimeIfCond               = true;
		bool comptimeSwitchCond           = true;
		bool alreadyUnsafe                = true;
		bool experimentalF80              = true;

		static auto all() -> WarningContext { return WarningContext(); };
	};


	enum class Warning{
		METHOD_CALL_ON_NON_METHOD,
		MEMBER_TYPE_BY_VALUE_ACCESSOR,
		DELETE_MOVED_FROM_EXPR,
		DELETE_TRIVIALLY_DELETABLE_TYPE,
		COMPTIME_IF_COND,
		COMPTIME_SWITCH_COND,
		ALREADY_UNSAFE,
		EXPERIMENTAL_F80,
	};

	
}
