////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>
#include <PCIT_core.h>


#include "./enums.h"
#include "./Type.h"
#include "./Expr.h"

namespace pcit::pir{


	struct GlobalVar{
		struct Zeroinit{};
		struct Uninit{};

		struct String{
			// For lookup in Module
			struct ID : public core::UniqueID<uint32_t, struct ID> {
				using core::UniqueID<uint32_t, ID>::UniqueID;
			};

			std::string value;
			Type type; // TODO: remove this?
		};

		// Forward declaration
		struct ArrayID : public core::UniqueID<uint32_t, struct ArrayID> {
			using core::UniqueID<uint32_t, ArrayID>::UniqueID;
		};

		// Forward declaration
		struct StructID : public core::UniqueID<uint32_t, struct StructID> {
			using core::UniqueID<uint32_t, StructID>::UniqueID;
		};

		using Value = evo::Variant<Expr, Zeroinit, Uninit, String::ID, ArrayID, StructID>;

		struct Array{
			// For lookup in Module
			using ID = ArrayID;

			Type type; // TODO: remove this?
			std::vector<Value> values;
		};

		struct Struct{
			// For lookup in Module
			using ID = StructID;

			Type type; // TODO: remove this?
			std::vector<Value> values;
		};



		std::string name;
		Type type;
		Linkage linkage;
		Value value;
		bool isConstant;

		// For lookup in Module
		struct ID : public core::UniqueID<uint32_t, struct ID> {
			using core::UniqueID<uint32_t, ID>::UniqueID;
		};
	};


}

