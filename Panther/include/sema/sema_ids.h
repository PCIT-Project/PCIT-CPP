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

	struct ImplicitConversionToOptionalID : public core::UniqueID<uint32_t, struct ImplicitConversionToOptionalID> {
		using core::UniqueID<uint32_t, ImplicitConversionToOptionalID>::UniqueID;
	};

	struct OptionalNullCheckID : public core::UniqueID<uint32_t, struct OptionalNullCheckID> {
		using core::UniqueID<uint32_t, OptionalNullCheckID>::UniqueID;
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

	struct PtrAccessorID : public core::UniqueID<uint32_t, struct PtrAccessorID> {
		using core::UniqueID<uint32_t, PtrAccessorID>::UniqueID;
	};

	struct UnionAccessorID : public core::UniqueID<uint32_t, struct UnionAccessorID> {
		using core::UniqueID<uint32_t, UnionAccessorID>::UniqueID;
	};

	struct PtrUnionAccessorID : public core::UniqueID<uint32_t, struct PtrUnionAccessorID> {
		using core::UniqueID<uint32_t, PtrUnionAccessorID>::UniqueID;
	};

	struct TryElseID : public core::UniqueID<uint32_t, struct TryElseID> {
		using core::UniqueID<uint32_t, TryElseID>::UniqueID;
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

	struct InterfaceCallID : public core::UniqueID<uint32_t, struct InterfaceCallID> {
		using core::UniqueID<uint32_t, InterfaceCallID>::UniqueID;
	};

	struct IndexerID : public core::UniqueID<uint32_t, struct IndexerID> {
		using core::UniqueID<uint32_t, IndexerID>::UniqueID;
	};

	struct PtrIndexerID : public core::UniqueID<uint32_t, struct PtrIndexerID> {
		using core::UniqueID<uint32_t, PtrIndexerID>::UniqueID;
	};

	struct UnionDesignatedInitNewID : public core::UniqueID<uint32_t, struct UnionDesignatedInitNewID> {
		using core::UniqueID<uint32_t, UnionDesignatedInitNewID>::UniqueID;
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

	struct BreakID : public core::UniqueID<uint32_t, struct BreakID> {
		using core::UniqueID<uint32_t, BreakID>::UniqueID;
	};

	struct ContinueID : public core::UniqueID<uint32_t, struct ContinueID> {
		using core::UniqueID<uint32_t, ContinueID>::UniqueID;
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



namespace pcit::core{
		
	template<>
	struct core::OptionalInterface<panther::sema::UninitID>{
		static constexpr auto init(panther::sema::UninitID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::UninitID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::ZeroinitID>{
		static constexpr auto init(panther::sema::ZeroinitID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::ZeroinitID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::IntValueID>{
		static constexpr auto init(panther::sema::IntValueID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::IntValueID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::FloatValueID>{
		static constexpr auto init(panther::sema::FloatValueID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::FloatValueID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::BoolValueID>{
		static constexpr auto init(panther::sema::BoolValueID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::BoolValueID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::StringValueID>{
		static constexpr auto init(panther::sema::StringValueID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::StringValueID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::AggregateValueID>{
		static constexpr auto init(panther::sema::AggregateValueID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::AggregateValueID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::CharValueID>{
		static constexpr auto init(panther::sema::CharValueID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::CharValueID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::TemplateIntrinsicFuncInstantiationID>{
		static constexpr auto init(panther::sema::TemplateIntrinsicFuncInstantiationID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::TemplateIntrinsicFuncInstantiationID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::CopyID>{
		static constexpr auto init(panther::sema::CopyID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::CopyID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::MoveID>{
		static constexpr auto init(panther::sema::MoveID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::MoveID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::ForwardID>{
		static constexpr auto init(panther::sema::ForwardID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::ForwardID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::AddrOfID>{
		static constexpr auto init(panther::sema::AddrOfID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::AddrOfID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::DerefID>{
		static constexpr auto init(panther::sema::DerefID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::DerefID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::AccessorID>{
		static constexpr auto init(panther::sema::AccessorID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::AccessorID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::PtrAccessorID>{
		static constexpr auto init(panther::sema::PtrAccessorID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::PtrAccessorID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::UnionAccessorID>{
		static constexpr auto init(panther::sema::UnionAccessorID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::UnionAccessorID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::PtrUnionAccessorID>{
		static constexpr auto init(panther::sema::PtrUnionAccessorID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::PtrUnionAccessorID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::TryElseID>{
		static constexpr auto init(panther::sema::TryElseID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::TryElseID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::BlockExprID>{
		static constexpr auto init(panther::sema::BlockExprID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::BlockExprID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::FakeTermInfoID>{
		static constexpr auto init(panther::sema::FakeTermInfoID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::FakeTermInfoID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::MakeInterfacePtrID>{
		static constexpr auto init(panther::sema::MakeInterfacePtrID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::MakeInterfacePtrID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::InterfaceCallID>{
		static constexpr auto init(panther::sema::InterfaceCallID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::InterfaceCallID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::FuncCallID>{
		static constexpr auto init(panther::sema::FuncCallID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::FuncCallID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::AssignID>{
		static constexpr auto init(panther::sema::AssignID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::AssignID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::MultiAssignID>{
		static constexpr auto init(panther::sema::MultiAssignID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::MultiAssignID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::ReturnID>{
		static constexpr auto init(panther::sema::ReturnID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::ReturnID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::ErrorID>{
		static constexpr auto init(panther::sema::ErrorID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::ErrorID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::BreakID>{
		static constexpr auto init(panther::sema::BreakID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::BreakID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::ContinueID>{
		static constexpr auto init(panther::sema::ContinueID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::ContinueID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::ConditionalID>{
		static constexpr auto init(panther::sema::ConditionalID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::ConditionalID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::WhileID>{
		static constexpr auto init(panther::sema::WhileID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::WhileID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::DeferID>{
		static constexpr auto init(panther::sema::DeferID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::DeferID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::FuncID>{
		static constexpr auto init(panther::sema::FuncID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::FuncID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::TemplatedFuncID>{
		static constexpr auto init(panther::sema::TemplatedFuncID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::TemplatedFuncID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::TemplatedStructID>{
		static constexpr auto init(panther::sema::TemplatedStructID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::TemplatedStructID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::VarID>{
		static constexpr auto init(panther::sema::VarID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::VarID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::GlobalVarID>{
		static constexpr auto init(panther::sema::GlobalVarID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::GlobalVarID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::ParamID>{
		static constexpr auto init(panther::sema::ParamID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::ParamID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::ReturnParamID>{
		static constexpr auto init(panther::sema::ReturnParamID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::ReturnParamID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::ErrorReturnParamID>{
		static constexpr auto init(panther::sema::ErrorReturnParamID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::ErrorReturnParamID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::BlockExprOutputID>{
		static constexpr auto init(panther::sema::BlockExprOutputID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::BlockExprOutputID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::ExceptParamID>{
		static constexpr auto init(panther::sema::ExceptParamID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::ExceptParamID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::IndexerID>{
		static constexpr auto init(panther::sema::IndexerID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::IndexerID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::PtrIndexerID>{
		static constexpr auto init(panther::sema::PtrIndexerID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::PtrIndexerID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	template<>
	struct core::OptionalInterface<panther::sema::UnionDesignatedInitNewID>{
		static constexpr auto init(panther::sema::UnionDesignatedInitNewID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::UnionDesignatedInitNewID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};


}








namespace std{
	
	
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
	struct hash<pcit::panther::sema::PtrAccessorID>{
		auto operator()(pcit::panther::sema::PtrAccessorID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::PtrAccessorID> 
		: public pcit::core::Optional<pcit::panther::sema::PtrAccessorID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::PtrAccessorID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::PtrAccessorID>::operator=;
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
	struct hash<pcit::panther::sema::PtrUnionAccessorID>{
		auto operator()(pcit::panther::sema::PtrUnionAccessorID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::PtrUnionAccessorID> 
		: public pcit::core::Optional<pcit::panther::sema::PtrUnionAccessorID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::PtrUnionAccessorID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::PtrUnionAccessorID>::operator=;
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
	struct hash<pcit::panther::sema::PtrIndexerID>{
		auto operator()(pcit::panther::sema::PtrIndexerID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::PtrIndexerID> : public pcit::core::Optional<pcit::panther::sema::PtrIndexerID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::PtrIndexerID>::Optional;
			using pcit::core::Optional<pcit::panther::sema::PtrIndexerID>::operator=;
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

}



