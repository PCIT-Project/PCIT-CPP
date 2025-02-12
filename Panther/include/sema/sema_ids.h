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


namespace pcit::panther::sema{

	// All IDs defined here are used as indexes into SemaBuffer (found in Context)

	//////////////////////////////////////////////////////////////////////
	// expressions

	struct UninitID : public core::UniqueID<uint32_t, struct UninitID> {
		using core::UniqueID<uint32_t, UninitID>::UniqueID;
	};

	struct ZeroinitID : public core::UniqueID<uint32_t, struct ZeroinitID> {
		using core::UniqueID<uint32_t, ZeroinitID>::UniqueID;
	};


	struct IntValueID : public core::UniqueID<uint32_t, struct IntValueID> {
		using core::UniqueID<uint32_t, IntValueID>::UniqueID;
	};

	struct FloatValueID : public core::UniqueID<uint32_t, struct FloatValueID> {
		using core::UniqueID<uint32_t, FloatValueID>::UniqueID;
	};

	struct BoolValueID : public core::UniqueID<uint32_t, struct BoolValueID> {
		using core::UniqueID<uint32_t, BoolValueID>::UniqueID;
	};

	struct StringValueID : public core::UniqueID<uint32_t, struct StringValueID> {
		using core::UniqueID<uint32_t, StringValueID>::UniqueID;
	};

	struct CharValueID : public core::UniqueID<uint32_t, struct CharValueID> {
		using core::UniqueID<uint32_t, CharValueID>::UniqueID;
	};


	struct TemplatedIntrinsicInstantiationID 
		: public core::UniqueID<uint32_t, struct TemplatedIntrinsicInstantiationID> {
		using core::UniqueID<uint32_t, TemplatedIntrinsicInstantiationID>::UniqueID;
	};


	struct CopyID : public core::UniqueID<uint32_t, struct CopyID> {
		using core::UniqueID<uint32_t, CopyID>::UniqueID;
	};

	struct MoveID : public core::UniqueID<uint32_t, struct MoveID> {
		using core::UniqueID<uint32_t, MoveID>::UniqueID;
	};

	struct DestructiveMoveID : public core::UniqueID<uint32_t, struct DestructiveMoveID> {
		using core::UniqueID<uint32_t, DestructiveMoveID>::UniqueID;
	};

	struct ForwardID : public core::UniqueID<uint32_t, struct ForwardID> {
		using core::UniqueID<uint32_t, ForwardID>::UniqueID;
	};

	struct AddrOfID : public core::UniqueID<uint32_t, struct AddrOfID> {
		using core::UniqueID<uint32_t, AddrOfID>::UniqueID;
	};

	struct DerefID : public core::UniqueID<uint32_t, struct DerefID> {
		using core::UniqueID<uint32_t, DerefID>::UniqueID;
	};


	//////////////////////////////////////////////////////////////////////
	// statements


	struct FuncCallID : public core::UniqueID<uint32_t, struct FuncCallID> {
		using core::UniqueID<uint32_t, FuncCallID>::UniqueID;
	};

	struct AssignID : public core::UniqueID<uint32_t, struct AssignID> {
		using core::UniqueID<uint32_t, AssignID>::UniqueID;
	};

	struct MultiAssignID : public core::UniqueID<uint32_t, struct MultiAssignID> {
		using core::UniqueID<uint32_t, MultiAssignID>::UniqueID;
	};

	struct ReturnID : public core::UniqueID<uint32_t, struct ReturnID> {
		using core::UniqueID<uint32_t, ReturnID>::UniqueID;
	};

	struct ConditionalID : public core::UniqueID<uint32_t, struct ConditionalID> {
		using core::UniqueID<uint32_t, ConditionalID>::UniqueID;
	};

	struct WhileID : public core::UniqueID<uint32_t, struct WhileID> {
		using core::UniqueID<uint32_t, WhileID>::UniqueID;
	};

	struct FuncID : public core::UniqueID<uint32_t, struct FuncID> {using core::UniqueID<uint32_t, FuncID>::UniqueID;};

	using Parent = evo::Variant<std::monostate, FuncID>;


	struct TemplatedFuncID : public core::UniqueID<uint32_t, struct TemplatedFuncID> {
		using core::UniqueID<uint32_t, TemplatedFuncID>::UniqueID;
	};

	struct VarID : public core::UniqueID<uint32_t, struct VarID> {
		using core::UniqueID<uint32_t, VarID>::UniqueID;
	};

	struct ParamID : public core::UniqueID<uint32_t, struct ParamID> {
		using core::UniqueID<uint32_t, ParamID>::UniqueID;
	};

	struct ReturnParamID : public core::UniqueID<uint32_t, struct ReturnParamID> {
		using core::UniqueID<uint32_t, ReturnParamID>::UniqueID;
	};
	
}

