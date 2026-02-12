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

	struct NullID : public core::UniqueID<uint32_t, struct NullID> {
		using core::UniqueID<uint32_t, NullID>::UniqueID;
	};

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

	struct AggregateValueID : public core::UniqueID<uint32_t, struct AggregateValueID> {
		using core::UniqueID<uint32_t, AggregateValueID>::UniqueID;
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

	struct ConversionToOptionalID : public core::UniqueID<uint32_t, struct ConversionToOptionalID> {
		using core::UniqueID<uint32_t, ConversionToOptionalID>::UniqueID;
	};

	struct OptionalNullCheckID : public core::UniqueID<uint32_t, struct OptionalNullCheckID> {
		using core::UniqueID<uint32_t, OptionalNullCheckID>::UniqueID;
	};

	struct OptionalExtractID : public core::UniqueID<uint32_t, struct OptionalExtractID> {
		using core::UniqueID<uint32_t, OptionalExtractID>::UniqueID;
	};

	struct DerefID : public core::UniqueID<uint32_t, struct DerefID> {
		using core::UniqueID<uint32_t, DerefID>::UniqueID;
	};

	struct UnwrapID : public core::UniqueID<uint32_t, struct UnwrapID> {
		using core::UniqueID<uint32_t, UnwrapID>::UniqueID;
	};

	struct AccessorID : public core::UniqueID<uint32_t, struct AccessorID> {
		using core::UniqueID<uint32_t, AccessorID>::UniqueID;
	};

	struct UnionAccessorID : public core::UniqueID<uint32_t, struct UnionAccessorID> {
		using core::UniqueID<uint32_t, UnionAccessorID>::UniqueID;
	};

	struct LogicalAndID : public core::UniqueID<uint32_t, struct LogicalAndID> {
		using core::UniqueID<uint32_t, LogicalAndID>::UniqueID;
	};

	struct LogicalOrID : public core::UniqueID<uint32_t, struct LogicalOrID> {
		using core::UniqueID<uint32_t, LogicalOrID>::UniqueID;
	};

	struct TryElseExprID : public core::UniqueID<uint32_t, struct TryElseExprID> {
		using core::UniqueID<uint32_t, TryElseExprID>::UniqueID;
	};

	struct TryElseInterfaceExprID : public core::UniqueID<uint32_t, struct TryElseInterfaceExprID> {
		using core::UniqueID<uint32_t, TryElseInterfaceExprID>::UniqueID;
	};

	struct BlockExprID : public core::UniqueID<uint32_t, struct BlockExprID> {
		using core::UniqueID<uint32_t, BlockExprID>::UniqueID;
	};

	struct FakeTermInfoID : public core::UniqueID<uint32_t, struct FakeTermInfoID> {
		using core::UniqueID<uint32_t, FakeTermInfoID>::UniqueID;
	};

	struct MakeInterfacePtrID : public core::UniqueID<uint32_t, struct MakeInterfacePtrID> {
		using core::UniqueID<uint32_t, MakeInterfacePtrID>::UniqueID;
	};

	struct InterfacePtrExtractThisID : public core::UniqueID<uint32_t, struct InterfacePtrExtractThisID> {
		using core::UniqueID<uint32_t, InterfacePtrExtractThisID>::UniqueID;
	};

	struct InterfaceCallID : public core::UniqueID<uint32_t, struct InterfaceCallID> {
		using core::UniqueID<uint32_t, InterfaceCallID>::UniqueID;
	};

	struct IndexerID : public core::UniqueID<uint32_t, struct IndexerID> {
		using core::UniqueID<uint32_t, IndexerID>::UniqueID;
	};

	struct DefaultNewID : public core::UniqueID<uint32_t, struct DefaultNewID> {
		using core::UniqueID<uint32_t, DefaultNewID>::UniqueID;
	};

	struct InitArrayRefID : public core::UniqueID<uint32_t, struct InitArrayRefID> {
		using core::UniqueID<uint32_t, InitArrayRefID>::UniqueID;
	};

	struct ArrayRefIndexerID : public core::UniqueID<uint32_t, struct ArrayRefIndexerID> {
		using core::UniqueID<uint32_t, ArrayRefIndexerID>::UniqueID;
	};

	struct ArrayRefSizeID : public core::UniqueID<uint32_t, struct ArrayRefSizeID> {
		using core::UniqueID<uint32_t, ArrayRefSizeID>::UniqueID;
	};

	struct ArrayRefDimensionsID : public core::UniqueID<uint32_t, struct ArrayRefDimensionsID> {
		using core::UniqueID<uint32_t, ArrayRefDimensionsID>::UniqueID;
	};

	struct ArrayRefDataID : public core::UniqueID<uint32_t, struct ArrayRefDataID> {
		using core::UniqueID<uint32_t, ArrayRefDataID>::UniqueID;
	};

	struct UnionDesignatedInitNewID : public core::UniqueID<uint32_t, struct UnionDesignatedInitNewID> {
		using core::UniqueID<uint32_t, UnionDesignatedInitNewID>::UniqueID;
	};

	struct UnionTagCmpID : public core::UniqueID<uint32_t, struct UnionTagCmpID> {
		using core::UniqueID<uint32_t, UnionTagCmpID>::UniqueID;
	};

	struct SameTypeCmpID : public core::UniqueID<uint32_t, struct SameTypeCmpID> {
		using core::UniqueID<uint32_t, SameTypeCmpID>::UniqueID;
	};


	//////////////////////////////////////////////////////////////////////
	// statements


	struct FuncCallID : public core::UniqueID<uint32_t, struct FuncCallID> {
		using core::UniqueID<uint32_t, FuncCallID>::UniqueID;
	};

	struct TryElseID : public core::UniqueID<uint32_t, struct TryElseID> {
		using core::UniqueID<uint32_t, TryElseID>::UniqueID;
	};

	struct TryElseInterfaceID : public core::UniqueID<uint32_t, struct TryElseInterfaceID> {
		using core::UniqueID<uint32_t, TryElseInterfaceID>::UniqueID;
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

	struct BreakID : public core::UniqueID<uint32_t, struct BreakID> {
		using core::UniqueID<uint32_t, BreakID>::UniqueID;
	};

	struct ContinueID : public core::UniqueID<uint32_t, struct ContinueID> {
		using core::UniqueID<uint32_t, ContinueID>::UniqueID;
	};

	struct DeleteID : public core::UniqueID<uint32_t, struct DeleteID> {
		using core::UniqueID<uint32_t, DeleteID>::UniqueID;
	};

	struct BlockScopeID : public core::UniqueID<uint32_t, struct BlockScopeID> {
		using core::UniqueID<uint32_t, BlockScopeID>::UniqueID;
	};

	struct ConditionalID : public core::UniqueID<uint32_t, struct ConditionalID> {
		using core::UniqueID<uint32_t, ConditionalID>::UniqueID;
	};

	struct WhileID : public core::UniqueID<uint32_t, struct WhileID> {
		using core::UniqueID<uint32_t, WhileID>::UniqueID;
	};

	struct ForID : public core::UniqueID<uint32_t, struct ForID> {
		using core::UniqueID<uint32_t, ForID>::UniqueID;
	};

	struct ForUnrollID : public core::UniqueID<uint32_t, struct ForUnrollID> {
		using core::UniqueID<uint32_t, ForUnrollID>::UniqueID;
	};

	struct SwitchID : public core::UniqueID<uint32_t, struct SwitchID> {
		using core::UniqueID<uint32_t, SwitchID>::UniqueID;
	};

	struct DeferID : public core::UniqueID<uint32_t, struct DeferID> {
		using core::UniqueID<uint32_t, DeferID>::UniqueID;
	};

	struct LifetimeStartID : public core::UniqueID<uint32_t, struct LifetimeStartID> {
		using core::UniqueID<uint32_t, LifetimeStartID>::UniqueID;
	};

	struct LifetimeEndID : public core::UniqueID<uint32_t, struct LifetimeEndID> {
		using core::UniqueID<uint32_t, LifetimeEndID>::UniqueID;
	};

	struct UnusedExprID : public core::UniqueID<uint32_t, struct UnusedExprID> {
		using core::UniqueID<uint32_t, UnusedExprID>::UniqueID;
	};
	
	struct FuncID : public core::UniqueID<uint32_t, struct FuncID> {
		using core::UniqueID<uint32_t, FuncID>::UniqueID;
	};

	struct FuncAliasID : public core::UniqueID<uint32_t, struct FuncAliasID> {
		using core::UniqueID<uint32_t, FuncAliasID>::UniqueID;
	};



	struct TemplatedFuncID : public core::UniqueID<uint32_t, struct TemplatedFuncID> {
		using core::UniqueID<uint32_t, TemplatedFuncID>::UniqueID;
	};

	struct TemplatedStructID : public core::UniqueID<uint32_t, struct TemplatedStructID> {
		using core::UniqueID<uint32_t, TemplatedStructID>::UniqueID;
	};

	struct StructTemplateAliasID : public core::UniqueID<uint32_t, struct StructTemplateAliasID> {
		using core::UniqueID<uint32_t, StructTemplateAliasID>::UniqueID;
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

	struct VariadicParamID : public core::UniqueID<uint32_t, struct VariadicParamID> {
		using core::UniqueID<uint32_t, VariadicParamID>::UniqueID;
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

	struct ForParamID : public core::UniqueID<uint32_t, struct ForParamID> {
		using core::UniqueID<uint32_t, ForParamID>::UniqueID;
	};
	
}



namespace pcit::core{

	template<>
	struct OptionalInterface<panther::sema::NullID>{
		static constexpr auto init(panther::sema::NullID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::NullID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};
		
	template<>
	struct OptionalInterface<panther::sema::UninitID>{
		static constexpr auto init(panther::sema::UninitID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::UninitID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::ZeroinitID>{
		static constexpr auto init(panther::sema::ZeroinitID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::ZeroinitID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::IntValueID>{
		static constexpr auto init(panther::sema::IntValueID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::IntValueID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::FloatValueID>{
		static constexpr auto init(panther::sema::FloatValueID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::FloatValueID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::BoolValueID>{
		static constexpr auto init(panther::sema::BoolValueID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::BoolValueID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::StringValueID>{
		static constexpr auto init(panther::sema::StringValueID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::StringValueID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::AggregateValueID>{
		static constexpr auto init(panther::sema::AggregateValueID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::AggregateValueID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::CharValueID>{
		static constexpr auto init(panther::sema::CharValueID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::CharValueID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::TemplateIntrinsicFuncInstantiationID>{
		static constexpr auto init(panther::sema::TemplateIntrinsicFuncInstantiationID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::TemplateIntrinsicFuncInstantiationID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::CopyID>{
		static constexpr auto init(panther::sema::CopyID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::CopyID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::MoveID>{
		static constexpr auto init(panther::sema::MoveID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::MoveID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::ForwardID>{
		static constexpr auto init(panther::sema::ForwardID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::ForwardID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::AddrOfID>{
		static constexpr auto init(panther::sema::AddrOfID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::AddrOfID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::ConversionToOptionalID>{
		static constexpr auto init(panther::sema::ConversionToOptionalID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::ConversionToOptionalID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::OptionalNullCheckID>{
		static constexpr auto init(panther::sema::OptionalNullCheckID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::OptionalNullCheckID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::OptionalExtractID>{
		static constexpr auto init(panther::sema::OptionalExtractID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::OptionalExtractID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::DerefID>{
		static constexpr auto init(panther::sema::DerefID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::DerefID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::AccessorID>{
		static constexpr auto init(panther::sema::AccessorID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::AccessorID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::UnionAccessorID>{
		static constexpr auto init(panther::sema::UnionAccessorID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::UnionAccessorID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::LogicalAndID>{
		static constexpr auto init(panther::sema::LogicalAndID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::LogicalAndID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::LogicalOrID>{
		static constexpr auto init(panther::sema::LogicalOrID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::LogicalOrID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::TryElseExprID>{
		static constexpr auto init(panther::sema::TryElseExprID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::TryElseExprID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::TryElseInterfaceExprID>{
		static constexpr auto init(panther::sema::TryElseInterfaceExprID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::TryElseInterfaceExprID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::BlockExprID>{
		static constexpr auto init(panther::sema::BlockExprID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::BlockExprID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::FakeTermInfoID>{
		static constexpr auto init(panther::sema::FakeTermInfoID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::FakeTermInfoID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::MakeInterfacePtrID>{
		static constexpr auto init(panther::sema::MakeInterfacePtrID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::MakeInterfacePtrID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::InterfacePtrExtractThisID>{
		static constexpr auto init(panther::sema::InterfacePtrExtractThisID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::InterfacePtrExtractThisID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::InterfaceCallID>{
		static constexpr auto init(panther::sema::InterfaceCallID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::InterfaceCallID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::FuncCallID>{
		static constexpr auto init(panther::sema::FuncCallID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::FuncCallID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::TryElseID>{
		static constexpr auto init(panther::sema::TryElseID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::TryElseID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::TryElseInterfaceID>{
		static constexpr auto init(panther::sema::TryElseInterfaceID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::TryElseInterfaceID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::AssignID>{
		static constexpr auto init(panther::sema::AssignID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::AssignID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::MultiAssignID>{
		static constexpr auto init(panther::sema::MultiAssignID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::MultiAssignID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::ReturnID>{
		static constexpr auto init(panther::sema::ReturnID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::ReturnID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::ErrorID>{
		static constexpr auto init(panther::sema::ErrorID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::ErrorID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::BreakID>{
		static constexpr auto init(panther::sema::BreakID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::BreakID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::ContinueID>{
		static constexpr auto init(panther::sema::ContinueID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::ContinueID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::DeleteID>{
		static constexpr auto init(panther::sema::DeleteID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::DeleteID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::BlockScopeID>{
		static constexpr auto init(panther::sema::BlockScopeID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::BlockScopeID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::WhileID>{
		static constexpr auto init(panther::sema::WhileID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::WhileID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::ForID>{
		static constexpr auto init(panther::sema::ForID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::ForID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::ForUnrollID>{
		static constexpr auto init(panther::sema::ForUnrollID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::ForUnrollID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::SwitchID>{
		static constexpr auto init(panther::sema::SwitchID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::SwitchID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::DeferID>{
		static constexpr auto init(panther::sema::DeferID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::DeferID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::LifetimeStartID>{
		static constexpr auto init(panther::sema::LifetimeStartID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::LifetimeStartID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::LifetimeEndID>{
		static constexpr auto init(panther::sema::LifetimeEndID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::LifetimeEndID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::UnusedExprID>{
		static constexpr auto init(panther::sema::UnusedExprID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::UnusedExprID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::FuncID>{
		static constexpr auto init(panther::sema::FuncID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::FuncID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::FuncAliasID>{
		static constexpr auto init(panther::sema::FuncAliasID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::FuncAliasID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::TemplatedFuncID>{
		static constexpr auto init(panther::sema::TemplatedFuncID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::TemplatedFuncID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::TemplatedStructID>{
		static constexpr auto init(panther::sema::TemplatedStructID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::TemplatedStructID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::StructTemplateAliasID>{
		static constexpr auto init(panther::sema::StructTemplateAliasID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::StructTemplateAliasID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::VarID>{
		static constexpr auto init(panther::sema::VarID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::VarID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::GlobalVarID>{
		static constexpr auto init(panther::sema::GlobalVarID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::GlobalVarID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::ParamID>{
		static constexpr auto init(panther::sema::ParamID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::ParamID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::VariadicParamID>{
		static constexpr auto init(panther::sema::VariadicParamID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::VariadicParamID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::ReturnParamID>{
		static constexpr auto init(panther::sema::ReturnParamID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::ReturnParamID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::ErrorReturnParamID>{
		static constexpr auto init(panther::sema::ErrorReturnParamID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::ErrorReturnParamID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::BlockExprOutputID>{
		static constexpr auto init(panther::sema::BlockExprOutputID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::BlockExprOutputID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::ExceptParamID>{
		static constexpr auto init(panther::sema::ExceptParamID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::ExceptParamID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::ForParamID>{
		static constexpr auto init(panther::sema::ForParamID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::ForParamID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::IndexerID>{
		static constexpr auto init(panther::sema::IndexerID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::IndexerID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};


	template<>
	struct OptionalInterface<panther::sema::DefaultNewID>{
		static constexpr auto init(panther::sema::DefaultNewID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::DefaultNewID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};


	template<>
	struct OptionalInterface<panther::sema::InitArrayRefID>{
		static constexpr auto init(panther::sema::InitArrayRefID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::InitArrayRefID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::ArrayRefIndexerID>{
		static constexpr auto init(panther::sema::ArrayRefIndexerID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::ArrayRefIndexerID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::ArrayRefSizeID>{
		static constexpr auto init(panther::sema::ArrayRefSizeID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::ArrayRefSizeID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::ArrayRefDimensionsID>{
		static constexpr auto init(panther::sema::ArrayRefDimensionsID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::ArrayRefDimensionsID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::ArrayRefDataID>{
		static constexpr auto init(panther::sema::ArrayRefDataID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::ArrayRefDataID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::UnionDesignatedInitNewID>{
		static constexpr auto init(panther::sema::UnionDesignatedInitNewID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::UnionDesignatedInitNewID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::UnionTagCmpID>{
		static constexpr auto init(panther::sema::UnionTagCmpID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::UnionTagCmpID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct OptionalInterface<panther::sema::SameTypeCmpID>{
		static constexpr auto init(panther::sema::SameTypeCmpID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::SameTypeCmpID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};


}








namespace std{


	template<>
	struct hash<pcit::panther::sema::NullID>{
		auto operator()(pcit::panther::sema::NullID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::NullID> : public pcit::core::Optional<pcit::panther::sema::NullID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::NullID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::NullID>::operator=;
	};

	
	
	template<>
	struct hash<pcit::panther::sema::UninitID>{
		auto operator()(pcit::panther::sema::UninitID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::UninitID> : public pcit::core::Optional<pcit::panther::sema::UninitID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::UninitID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::UninitID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::ZeroinitID>{
		auto operator()(pcit::panther::sema::ZeroinitID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::ZeroinitID> : public pcit::core::Optional<pcit::panther::sema::ZeroinitID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::ZeroinitID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::ZeroinitID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::IntValueID>{
		auto operator()(pcit::panther::sema::IntValueID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::IntValueID> : public pcit::core::Optional<pcit::panther::sema::IntValueID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::IntValueID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::IntValueID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::FloatValueID>{
		auto operator()(pcit::panther::sema::FloatValueID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::FloatValueID> : public pcit::core::Optional<pcit::panther::sema::FloatValueID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::FloatValueID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::FloatValueID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::BoolValueID>{
		auto operator()(pcit::panther::sema::BoolValueID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::BoolValueID> : public pcit::core::Optional<pcit::panther::sema::BoolValueID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::BoolValueID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::BoolValueID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::StringValueID>{
		auto operator()(pcit::panther::sema::StringValueID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::StringValueID> 
		: public pcit::core::Optional<pcit::panther::sema::StringValueID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::StringValueID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::StringValueID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::AggregateValueID>{
		auto operator()(pcit::panther::sema::AggregateValueID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::AggregateValueID>
		: public pcit::core::Optional<pcit::panther::sema::AggregateValueID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::AggregateValueID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::AggregateValueID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::CharValueID>{
		auto operator()(pcit::panther::sema::CharValueID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::CharValueID> : public pcit::core::Optional<pcit::panther::sema::CharValueID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::CharValueID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::CharValueID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::TemplateIntrinsicFuncInstantiationID>{
		auto operator()(pcit::panther::sema::TemplateIntrinsicFuncInstantiationID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::TemplateIntrinsicFuncInstantiationID>
		: public pcit::core::Optional<pcit::panther::sema::TemplateIntrinsicFuncInstantiationID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::TemplateIntrinsicFuncInstantiationID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::TemplateIntrinsicFuncInstantiationID>::operator=;
	};




	template<>
	struct hash<pcit::panther::sema::CopyID>{
		auto operator()(pcit::panther::sema::CopyID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::CopyID> : public pcit::core::Optional<pcit::panther::sema::CopyID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::CopyID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::CopyID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::MoveID>{
		auto operator()(pcit::panther::sema::MoveID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::MoveID>
		: public pcit::core::Optional<pcit::panther::sema::MoveID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::MoveID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::MoveID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::ForwardID>{
		auto operator()(pcit::panther::sema::ForwardID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::ForwardID> : public pcit::core::Optional<pcit::panther::sema::ForwardID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::ForwardID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::ForwardID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::AddrOfID>{
		auto operator()(pcit::panther::sema::AddrOfID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::AddrOfID> : public pcit::core::Optional<pcit::panther::sema::AddrOfID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::AddrOfID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::AddrOfID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::ConversionToOptionalID>{
		auto operator()(pcit::panther::sema::ConversionToOptionalID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::ConversionToOptionalID> 
		: public pcit::core::Optional<pcit::panther::sema::ConversionToOptionalID>{

		public:
			using pcit::core::Optional<pcit::panther::sema::ConversionToOptionalID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::ConversionToOptionalID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::OptionalNullCheckID>{
		auto operator()(pcit::panther::sema::OptionalNullCheckID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::OptionalNullCheckID>
		: public pcit::core::Optional<pcit::panther::sema::OptionalNullCheckID>{

		public:
			using pcit::core::Optional<pcit::panther::sema::OptionalNullCheckID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::OptionalNullCheckID>::operator=;
	};


	template<>
	struct hash<pcit::panther::sema::OptionalExtractID>{
		auto operator()(pcit::panther::sema::OptionalExtractID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::OptionalExtractID>
		: public pcit::core::Optional<pcit::panther::sema::OptionalExtractID>{

		public:
			using pcit::core::Optional<pcit::panther::sema::OptionalExtractID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::OptionalExtractID>::operator=;
	};


	template<>
	struct hash<pcit::panther::sema::DerefID>{
		auto operator()(pcit::panther::sema::DerefID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::DerefID> : public pcit::core::Optional<pcit::panther::sema::DerefID>{

		public:
			using pcit::core::Optional<pcit::panther::sema::DerefID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::DerefID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::AccessorID>{
		auto operator()(pcit::panther::sema::AccessorID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::AccessorID> : public pcit::core::Optional<pcit::panther::sema::AccessorID>{

		public:
			using pcit::core::Optional<pcit::panther::sema::AccessorID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::AccessorID>::operator=;
	};




	template<>
	struct hash<pcit::panther::sema::UnionAccessorID>{
		auto operator()(pcit::panther::sema::UnionAccessorID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::UnionAccessorID> 
		: public pcit::core::Optional<pcit::panther::sema::UnionAccessorID>{

		public:
			using pcit::core::Optional<pcit::panther::sema::UnionAccessorID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::UnionAccessorID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::LogicalAndID>{
		auto operator()(pcit::panther::sema::LogicalAndID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::LogicalAndID> 
		: public pcit::core::Optional<pcit::panther::sema::LogicalAndID>{

		public:
			using pcit::core::Optional<pcit::panther::sema::LogicalAndID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::LogicalAndID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::LogicalOrID>{
		auto operator()(pcit::panther::sema::LogicalOrID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::LogicalOrID> 
		: public pcit::core::Optional<pcit::panther::sema::LogicalOrID>{

		public:
			using pcit::core::Optional<pcit::panther::sema::LogicalOrID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::LogicalOrID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::TryElseExprID>{
		auto operator()(pcit::panther::sema::TryElseExprID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::TryElseExprID>
		: public pcit::core::Optional<pcit::panther::sema::TryElseExprID>{
			
		public:
			using pcit::core::Optional<pcit::panther::sema::TryElseExprID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::TryElseExprID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::TryElseInterfaceExprID>{
		auto operator()(pcit::panther::sema::TryElseInterfaceExprID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::TryElseInterfaceExprID>
		: public pcit::core::Optional<pcit::panther::sema::TryElseInterfaceExprID>{
			
		public:
			using pcit::core::Optional<pcit::panther::sema::TryElseInterfaceExprID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::TryElseInterfaceExprID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::BlockExprID>{
		auto operator()(pcit::panther::sema::BlockExprID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::BlockExprID> : public pcit::core::Optional<pcit::panther::sema::BlockExprID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::BlockExprID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::BlockExprID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::FakeTermInfoID>{
		auto operator()(pcit::panther::sema::FakeTermInfoID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::FakeTermInfoID>
		: public pcit::core::Optional<pcit::panther::sema::FakeTermInfoID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::FakeTermInfoID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::FakeTermInfoID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::MakeInterfacePtrID>{
		auto operator()(pcit::panther::sema::MakeInterfacePtrID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::MakeInterfacePtrID>
		: public pcit::core::Optional<pcit::panther::sema::MakeInterfacePtrID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::MakeInterfacePtrID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::MakeInterfacePtrID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::InterfacePtrExtractThisID>{
		auto operator()(pcit::panther::sema::InterfacePtrExtractThisID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::InterfacePtrExtractThisID>
		: public pcit::core::Optional<pcit::panther::sema::InterfacePtrExtractThisID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::InterfacePtrExtractThisID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::InterfacePtrExtractThisID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::InterfaceCallID>{
		auto operator()(pcit::panther::sema::InterfaceCallID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::InterfaceCallID>
		: public pcit::core::Optional<pcit::panther::sema::InterfaceCallID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::InterfaceCallID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::InterfaceCallID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::FuncCallID>{
		auto operator()(pcit::panther::sema::FuncCallID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::FuncCallID> : public pcit::core::Optional<pcit::panther::sema::FuncCallID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::FuncCallID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::FuncCallID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::TryElseID>{
		auto operator()(pcit::panther::sema::TryElseID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::TryElseID> : public pcit::core::Optional<pcit::panther::sema::TryElseID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::TryElseID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::TryElseID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::TryElseInterfaceID>{
		auto operator()(pcit::panther::sema::TryElseInterfaceID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::TryElseInterfaceID>
		: public pcit::core::Optional<pcit::panther::sema::TryElseInterfaceID>{
			
		public:
			using pcit::core::Optional<pcit::panther::sema::TryElseInterfaceID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::TryElseInterfaceID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::AssignID>{
		auto operator()(pcit::panther::sema::AssignID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::AssignID> : public pcit::core::Optional<pcit::panther::sema::AssignID>{

		public:
			using pcit::core::Optional<pcit::panther::sema::AssignID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::AssignID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::MultiAssignID>{
		auto operator()(pcit::panther::sema::MultiAssignID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::MultiAssignID>
		: public pcit::core::Optional<pcit::panther::sema::MultiAssignID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::MultiAssignID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::MultiAssignID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::ReturnID>{
		auto operator()(pcit::panther::sema::ReturnID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::ReturnID> : public pcit::core::Optional<pcit::panther::sema::ReturnID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::ReturnID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::ReturnID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::ErrorID>{
		auto operator()(pcit::panther::sema::ErrorID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::ErrorID> : public pcit::core::Optional<pcit::panther::sema::ErrorID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::ErrorID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::ErrorID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::BreakID>{
		auto operator()(pcit::panther::sema::BreakID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::BreakID> : public pcit::core::Optional<pcit::panther::sema::BreakID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::BreakID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::BreakID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::ContinueID>{
		auto operator()(pcit::panther::sema::ContinueID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::ContinueID> : public pcit::core::Optional<pcit::panther::sema::ContinueID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::ContinueID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::ContinueID>::operator=;
	};




	template<>
	struct hash<pcit::panther::sema::DeleteID>{
		auto operator()(pcit::panther::sema::DeleteID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::DeleteID> : public pcit::core::Optional<pcit::panther::sema::DeleteID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::DeleteID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::DeleteID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::BlockScopeID>{
		auto operator()(pcit::panther::sema::BlockScopeID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::BlockScopeID> : public pcit::core::Optional<pcit::panther::sema::BlockScopeID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::BlockScopeID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::BlockScopeID>::operator=;
	};
	


	template<>
	struct hash<pcit::panther::sema::ConditionalID>{
		auto operator()(pcit::panther::sema::ConditionalID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::ConditionalID>
		: public pcit::core::Optional<pcit::panther::sema::ConditionalID>{

		public:
			using pcit::core::Optional<pcit::panther::sema::ConditionalID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::ConditionalID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::WhileID>{
		auto operator()(pcit::panther::sema::WhileID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::WhileID> : public pcit::core::Optional<pcit::panther::sema::WhileID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::WhileID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::WhileID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::ForID>{
		auto operator()(pcit::panther::sema::ForID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::ForID> : public pcit::core::Optional<pcit::panther::sema::ForID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::ForID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::ForID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::ForUnrollID>{
		auto operator()(pcit::panther::sema::ForUnrollID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::ForUnrollID> : public pcit::core::Optional<pcit::panther::sema::ForUnrollID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::ForUnrollID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::ForUnrollID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::SwitchID>{
		auto operator()(pcit::panther::sema::SwitchID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::SwitchID> : public pcit::core::Optional<pcit::panther::sema::SwitchID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::SwitchID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::SwitchID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::DeferID>{
		auto operator()(pcit::panther::sema::DeferID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::DeferID> : public pcit::core::Optional<pcit::panther::sema::DeferID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::DeferID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::DeferID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::LifetimeStartID>{
		auto operator()(pcit::panther::sema::LifetimeStartID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::LifetimeStartID>
		: public pcit::core::Optional<pcit::panther::sema::LifetimeStartID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::LifetimeStartID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::LifetimeStartID>::operator=;
	};




	template<>
	struct hash<pcit::panther::sema::LifetimeEndID>{
		auto operator()(pcit::panther::sema::LifetimeEndID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::LifetimeEndID>
		: public pcit::core::Optional<pcit::panther::sema::LifetimeEndID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::LifetimeEndID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::LifetimeEndID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::UnusedExprID>{
		auto operator()(pcit::panther::sema::UnusedExprID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::UnusedExprID>
		: public pcit::core::Optional<pcit::panther::sema::UnusedExprID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::UnusedExprID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::UnusedExprID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::FuncID>{
		auto operator()(pcit::panther::sema::FuncID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::FuncID> : public pcit::core::Optional<pcit::panther::sema::FuncID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::FuncID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::FuncID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::FuncAliasID>{
		auto operator()(pcit::panther::sema::FuncAliasID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::FuncAliasID> : public pcit::core::Optional<pcit::panther::sema::FuncAliasID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::FuncAliasID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::FuncAliasID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::TemplatedFuncID>{
		auto operator()(pcit::panther::sema::TemplatedFuncID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::TemplatedFuncID>
		: public pcit::core::Optional<pcit::panther::sema::TemplatedFuncID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::TemplatedFuncID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::TemplatedFuncID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::TemplatedStructID>{
		auto operator()(pcit::panther::sema::TemplatedStructID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::TemplatedStructID>
		: public pcit::core::Optional<pcit::panther::sema::TemplatedStructID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::TemplatedStructID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::TemplatedStructID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::StructTemplateAliasID>{
		auto operator()(pcit::panther::sema::StructTemplateAliasID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::StructTemplateAliasID>
		: public pcit::core::Optional<pcit::panther::sema::StructTemplateAliasID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::StructTemplateAliasID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::StructTemplateAliasID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::VarID>{
		auto operator()(pcit::panther::sema::VarID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::VarID>
		: public pcit::core::Optional<pcit::panther::sema::VarID>{

		public:
			using pcit::core::Optional<pcit::panther::sema::VarID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::VarID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::GlobalVarID>{
		auto operator()(pcit::panther::sema::GlobalVarID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::GlobalVarID>
		: public pcit::core::Optional<pcit::panther::sema::GlobalVarID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::GlobalVarID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::GlobalVarID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::ParamID>{
		auto operator()(pcit::panther::sema::ParamID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::ParamID>
		: public pcit::core::Optional<pcit::panther::sema::ParamID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::ParamID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::ParamID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::VariadicParamID>{
		auto operator()(pcit::panther::sema::VariadicParamID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::VariadicParamID>
		: public pcit::core::Optional<pcit::panther::sema::VariadicParamID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::VariadicParamID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::VariadicParamID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::ReturnParamID>{
		auto operator()(pcit::panther::sema::ReturnParamID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::ReturnParamID>
		: public pcit::core::Optional<pcit::panther::sema::ReturnParamID>{

		public:
			using pcit::core::Optional<pcit::panther::sema::ReturnParamID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::ReturnParamID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::ErrorReturnParamID>{
		auto operator()(pcit::panther::sema::ErrorReturnParamID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::ErrorReturnParamID>
		: public pcit::core::Optional<pcit::panther::sema::ErrorReturnParamID>{

		public:
			using pcit::core::Optional<pcit::panther::sema::ErrorReturnParamID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::ErrorReturnParamID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::BlockExprOutputID>{
		auto operator()(pcit::panther::sema::BlockExprOutputID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::BlockExprOutputID>
		: public pcit::core::Optional<pcit::panther::sema::BlockExprOutputID>{

		public:
			using pcit::core::Optional<pcit::panther::sema::BlockExprOutputID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::BlockExprOutputID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::ExceptParamID>{
		auto operator()(pcit::panther::sema::ExceptParamID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::ExceptParamID>
		: public pcit::core::Optional<pcit::panther::sema::ExceptParamID>{

		public:
			using pcit::core::Optional<pcit::panther::sema::ExceptParamID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::ExceptParamID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::ForParamID>{
		auto operator()(pcit::panther::sema::ForParamID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::ForParamID>
		: public pcit::core::Optional<pcit::panther::sema::ForParamID>{

		public:
			using pcit::core::Optional<pcit::panther::sema::ForParamID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::ForParamID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::IndexerID>{
		auto operator()(pcit::panther::sema::IndexerID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::IndexerID> : public pcit::core::Optional<pcit::panther::sema::IndexerID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::IndexerID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::IndexerID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::DefaultNewID>{
		auto operator()(pcit::panther::sema::DefaultNewID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::DefaultNewID>
		: public pcit::core::Optional<pcit::panther::sema::DefaultNewID>{
		
		public:
			using pcit::core::Optional<pcit::panther::sema::DefaultNewID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::DefaultNewID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::InitArrayRefID>{
		auto operator()(pcit::panther::sema::InitArrayRefID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::InitArrayRefID> 
		: public pcit::core::Optional<pcit::panther::sema::InitArrayRefID>{
			
		public:
			using pcit::core::Optional<pcit::panther::sema::InitArrayRefID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::InitArrayRefID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::ArrayRefIndexerID>{
		auto operator()(pcit::panther::sema::ArrayRefIndexerID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::ArrayRefIndexerID>
		: public pcit::core::Optional<pcit::panther::sema::ArrayRefIndexerID>{
			
		public:
			using pcit::core::Optional<pcit::panther::sema::ArrayRefIndexerID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::ArrayRefIndexerID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::ArrayRefSizeID>{
		auto operator()(pcit::panther::sema::ArrayRefSizeID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::ArrayRefSizeID>
		: public pcit::core::Optional<pcit::panther::sema::ArrayRefSizeID>{
			
		public:
			using pcit::core::Optional<pcit::panther::sema::ArrayRefSizeID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::ArrayRefSizeID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::ArrayRefDimensionsID>{
		auto operator()(pcit::panther::sema::ArrayRefDimensionsID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::ArrayRefDimensionsID>
		: public pcit::core::Optional<pcit::panther::sema::ArrayRefDimensionsID>{
			
		public:
			using pcit::core::Optional<pcit::panther::sema::ArrayRefDimensionsID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::ArrayRefDimensionsID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::ArrayRefDataID>{
		auto operator()(pcit::panther::sema::ArrayRefDataID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::ArrayRefDataID>
		: public pcit::core::Optional<pcit::panther::sema::ArrayRefDataID>{
			
		public:
			using pcit::core::Optional<pcit::panther::sema::ArrayRefDataID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::ArrayRefDataID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::UnionDesignatedInitNewID>{
		auto operator()(pcit::panther::sema::UnionDesignatedInitNewID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::UnionDesignatedInitNewID>
		: public pcit::core::Optional<pcit::panther::sema::UnionDesignatedInitNewID>{
			
		public:
			using pcit::core::Optional<pcit::panther::sema::UnionDesignatedInitNewID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::UnionDesignatedInitNewID>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::UnionTagCmpID>{
		auto operator()(pcit::panther::sema::UnionTagCmpID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::UnionTagCmpID>
		: public pcit::core::Optional<pcit::panther::sema::UnionTagCmpID>{
			
		public:
			using pcit::core::Optional<pcit::panther::sema::UnionTagCmpID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::UnionTagCmpID>::operator=;
	};


	template<>
	struct hash<pcit::panther::sema::SameTypeCmpID>{
		auto operator()(pcit::panther::sema::SameTypeCmpID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::SameTypeCmpID>
		: public pcit::core::Optional<pcit::panther::sema::SameTypeCmpID>{
			
		public:
			using pcit::core::Optional<pcit::panther::sema::SameTypeCmpID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::SameTypeCmpID>::operator=;
	};

}



