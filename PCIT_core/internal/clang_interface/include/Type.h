////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>


namespace pcit::clangint{


	namespace BaseType{
		
		enum class Primitive{
			VOID,
			ISIZE,
			I8,
			I16,
			I32,
			I64,
			I128,
			USIZE,
			UI8,
			UI16,
			UI32,
			UI64,
			UI128,
			F16,
			BF16,
			F32,
			F64,
			F80,
			F128,
			BYTE,
			BOOL,
			CHAR,
			RAWPTR,
			C_SHORT,
			C_USHORT,
			C_INT,
			C_UINT,
			C_LONG,
			C_ULONG,
			C_LONG_LONG,
			C_ULONG_LONG,
			C_LONG_DOUBLE,
			UNKNOWN,
		};

	}






	struct Type{
		enum class Qualifier{
			POINTER,
			CONST_POINTER,
			L_VALUE_REFERENCE,
			R_VALUE_REFERENCE,
		};

		evo::Variant<BaseType::Primitive> baseType{};
		evo::SmallVector<Qualifier> qualifiers{};
		bool isConst;
	};



}