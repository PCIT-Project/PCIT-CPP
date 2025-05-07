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


	struct TemplateIntrinsicFuncInstantiationID 
		: public core::UniqueID<uint32_t, struct TemplateIntrinsicFuncInstantiationID> {
		using core::UniqueID<uint32_t, TemplateIntrinsicFuncInstantiationID>::UniqueID;
	};


	struct CopyID : public core::UniqueID<uint32_t, struct CopyID> {
		using core::UniqueID<uint32_t, CopyID>::UniqueID;
	};

	struct MoveID : public core::UniqueID<uint32_t, struct MoveID> {
		using core::UniqueID<uint32_t, MoveID>::UniqueID;
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

	struct TryElseID : public core::UniqueID<uint32_t, struct TryElseID> {
		using core::UniqueID<uint32_t, TryElseID>::UniqueID;
	};

	struct BlockExprID : public core::UniqueID<uint32_t, struct BlockExprID> {
		using core::UniqueID<uint32_t, BlockExprID>::UniqueID;
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

	struct ErrorID : public core::UniqueID<uint32_t, struct ErrorID> {
		using core::UniqueID<uint32_t, ErrorID>::UniqueID;
	};

	struct ConditionalID : public core::UniqueID<uint32_t, struct ConditionalID> {
		using core::UniqueID<uint32_t, ConditionalID>::UniqueID;
	};

	struct WhileID : public core::UniqueID<uint32_t, struct WhileID> {
		using core::UniqueID<uint32_t, WhileID>::UniqueID;
	};

	struct DeferID : public core::UniqueID<uint32_t, struct DeferID> {
		using core::UniqueID<uint32_t, DeferID>::UniqueID;
	};
	
	struct FuncID : public core::UniqueID<uint32_t, struct FuncID> {
		using core::UniqueID<uint32_t, FuncID>::UniqueID;
	};

	using Parent = evo::Variant<std::monostate, FuncID>;


	struct TemplatedFuncID : public core::UniqueID<uint32_t, struct TemplatedFuncID> {
		using core::UniqueID<uint32_t, TemplatedFuncID>::UniqueID;
	};

	struct TemplatedStructID : public core::UniqueID<uint32_t, struct TemplatedStructID> {
		using core::UniqueID<uint32_t, TemplatedStructID>::UniqueID;
	};


	struct VarID : public core::UniqueID<uint32_t, struct VarID> {
		using core::UniqueID<uint32_t, VarID>::UniqueID;
	};

	struct GlobalVarID : public core::UniqueID<uint32_t, struct GlobalVarID> {
		using core::UniqueID<uint32_t, GlobalVarID>::UniqueID;
	};

	struct ParamID : public core::UniqueID<uint32_t, struct ParamID> {
		using core::UniqueID<uint32_t, ParamID>::UniqueID;
	};

	struct ReturnParamID : public core::UniqueID<uint32_t, struct ReturnParamID> {
		using core::UniqueID<uint32_t, ReturnParamID>::UniqueID;
	};

	struct ErrorReturnParamID : public core::UniqueID<uint32_t, struct ErrorReturnParamID> {
		using core::UniqueID<uint32_t, ErrorReturnParamID>::UniqueID;
	};

	struct BlockExprOutputID : public core::UniqueID<uint32_t, struct BlockExprOutputID> {
		using core::UniqueID<uint32_t, BlockExprOutputID>::UniqueID;
	};

	struct ExceptParamID : public core::UniqueID<uint32_t, struct ExceptParamID> {
		using core::UniqueID<uint32_t, ExceptParamID>::UniqueID;
	};
	
}



namespace std{


	template<>
	struct hash<pcit::panther::sema::FuncID>{
		auto operator()(pcit::panther::sema::FuncID func_id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(func_id.get());
		};
	};

	template<>
	struct hash<pcit::panther::sema::GlobalVarID>{
		auto operator()(pcit::panther::sema::GlobalVarID global_id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(global_id.get());
		};
	};
	
}



