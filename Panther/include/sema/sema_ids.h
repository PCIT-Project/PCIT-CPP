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
	struct UninitIDOptInterface{
		static constexpr auto init(UninitID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const UninitID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	struct ZeroinitID : public core::UniqueID<uint32_t, struct ZeroinitID> {
		using core::UniqueID<uint32_t, ZeroinitID>::UniqueID;
	};
	struct ZeroinitIDOptInterface{
		static constexpr auto init(ZeroinitID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const ZeroinitID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};


	struct IntValueID : public core::UniqueID<uint32_t, struct IntValueID> {
		using core::UniqueID<uint32_t, IntValueID>::UniqueID;
	};
	struct IntValueIDOptInterface{
		static constexpr auto init(IntValueID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const IntValueID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	struct FloatValueID : public core::UniqueID<uint32_t, struct FloatValueID> {
		using core::UniqueID<uint32_t, FloatValueID>::UniqueID;
	};
	struct FloatValueIDOptInterface{
		static constexpr auto init(FloatValueID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const FloatValueID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	struct BoolValueID : public core::UniqueID<uint32_t, struct BoolValueID> {
		using core::UniqueID<uint32_t, BoolValueID>::UniqueID;
	};
	struct BoolValueIDOptInterface{
		static constexpr auto init(BoolValueID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const BoolValueID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	struct StringValueID : public core::UniqueID<uint32_t, struct StringValueID> {
		using core::UniqueID<uint32_t, StringValueID>::UniqueID;
	};
	struct StringValueIDOptInterface{
		static constexpr auto init(StringValueID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const StringValueID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	struct AggregateValueID : public core::UniqueID<uint32_t, struct AggregateValueID> {
		using core::UniqueID<uint32_t, AggregateValueID>::UniqueID;
	};
	struct AggregateValueIDOptInterface{
		static constexpr auto init(AggregateValueID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const AggregateValueID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	struct CharValueID : public core::UniqueID<uint32_t, struct CharValueID> {
		using core::UniqueID<uint32_t, CharValueID>::UniqueID;
	};
	struct CharValueIDOptInterface{
		static constexpr auto init(CharValueID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const CharValueID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};


	struct TemplateIntrinsicFuncInstantiationID 
		: public core::UniqueID<uint32_t, struct TemplateIntrinsicFuncInstantiationID> {
		using core::UniqueID<uint32_t, TemplateIntrinsicFuncInstantiationID>::UniqueID;
	};
	struct TemplateIntrinsicFuncInstantiationIDOptInterface{
		static constexpr auto init(TemplateIntrinsicFuncInstantiationID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const TemplateIntrinsicFuncInstantiationID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};


	struct CopyID : public core::UniqueID<uint32_t, struct CopyID> {
		using core::UniqueID<uint32_t, CopyID>::UniqueID;
	};
	struct CopyIDOptInterface{
		static constexpr auto init(CopyID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const CopyID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	struct MoveID : public core::UniqueID<uint32_t, struct MoveID> {
		using core::UniqueID<uint32_t, MoveID>::UniqueID;
	};
	struct MoveIDOptInterface{
		static constexpr auto init(MoveID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const MoveID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	struct ForwardID : public core::UniqueID<uint32_t, struct ForwardID> {
		using core::UniqueID<uint32_t, ForwardID>::UniqueID;
	};
	struct ForwardIDOptInterface{
		static constexpr auto init(ForwardID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const ForwardID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	struct AddrOfID : public core::UniqueID<uint32_t, struct AddrOfID> {
		using core::UniqueID<uint32_t, AddrOfID>::UniqueID;
	};
	struct AddrOfIDOptInterface{
		static constexpr auto init(AddrOfID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const AddrOfID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	struct DerefID : public core::UniqueID<uint32_t, struct DerefID> {
		using core::UniqueID<uint32_t, DerefID>::UniqueID;
	};
	struct DerefIDOptInterface{
		static constexpr auto init(DerefID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const DerefID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	struct AccessorID : public core::UniqueID<uint32_t, struct AccessorID> {
		using core::UniqueID<uint32_t, AccessorID>::UniqueID;
	};
	struct AccessorIDOptInterface{
		static constexpr auto init(AccessorID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const AccessorID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	struct PtrAccessorID : public core::UniqueID<uint32_t, struct PtrAccessorID> {
		using core::UniqueID<uint32_t, PtrAccessorID>::UniqueID;
	};
	struct PtrAccessorIDOptInterface{
		static constexpr auto init(PtrAccessorID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const PtrAccessorID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	struct TryElseID : public core::UniqueID<uint32_t, struct TryElseID> {
		using core::UniqueID<uint32_t, TryElseID>::UniqueID;
	};
	struct TryElseIDOptInterface{
		static constexpr auto init(TryElseID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const TryElseID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	struct BlockExprID : public core::UniqueID<uint32_t, struct BlockExprID> {
		using core::UniqueID<uint32_t, BlockExprID>::UniqueID;
	};
	struct BlockExprIDOptInterface{
		static constexpr auto init(BlockExprID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const BlockExprID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};


	struct FakeTermInfoID : public core::UniqueID<uint32_t, struct FakeTermInfoID> {
		using core::UniqueID<uint32_t, FakeTermInfoID>::UniqueID;
	};
	struct FakeTermInfoIDOptInterface{
		static constexpr auto init(FakeTermInfoID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const FakeTermInfoID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};


	//////////////////////////////////////////////////////////////////////
	// statements


	struct FuncCallID : public core::UniqueID<uint32_t, struct FuncCallID> {
		using core::UniqueID<uint32_t, FuncCallID>::UniqueID;
	};
	struct FuncCallIDOptInterface{
		static constexpr auto init(FuncCallID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const FuncCallID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	struct AssignID : public core::UniqueID<uint32_t, struct AssignID> {
		using core::UniqueID<uint32_t, AssignID>::UniqueID;
	};
	struct AssignIDOptInterface{
		static constexpr auto init(AssignID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const AssignID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	struct MultiAssignID : public core::UniqueID<uint32_t, struct MultiAssignID> {
		using core::UniqueID<uint32_t, MultiAssignID>::UniqueID;
	};
	struct MultiAssignIDOptInterface{
		static constexpr auto init(MultiAssignID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const MultiAssignID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	struct ReturnID : public core::UniqueID<uint32_t, struct ReturnID> {
		using core::UniqueID<uint32_t, ReturnID>::UniqueID;
	};
	struct ReturnIDOptInterface{
		static constexpr auto init(ReturnID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const ReturnID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	struct ErrorID : public core::UniqueID<uint32_t, struct ErrorID> {
		using core::UniqueID<uint32_t, ErrorID>::UniqueID;
	};
	struct ErrorIDOptInterface{
		static constexpr auto init(ErrorID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const ErrorID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	struct ConditionalID : public core::UniqueID<uint32_t, struct ConditionalID> {
		using core::UniqueID<uint32_t, ConditionalID>::UniqueID;
	};
	struct ConditionalIDOptInterface{
		static constexpr auto init(ConditionalID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const ConditionalID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	struct WhileID : public core::UniqueID<uint32_t, struct WhileID> {
		using core::UniqueID<uint32_t, WhileID>::UniqueID;
	};
	struct WhileIDOptInterface{
		static constexpr auto init(WhileID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const WhileID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	struct DeferID : public core::UniqueID<uint32_t, struct DeferID> {
		using core::UniqueID<uint32_t, DeferID>::UniqueID;
	};
	struct DeferIDOptInterface{
		static constexpr auto init(DeferID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const DeferID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};
	
	struct FuncID : public core::UniqueID<uint32_t, struct FuncID> {
		using core::UniqueID<uint32_t, FuncID>::UniqueID;
	};
	struct FuncIDOptInterface{
		static constexpr auto init(FuncID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const FuncID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	using Parent = evo::Variant<std::monostate, FuncID>;


	struct TemplatedFuncID : public core::UniqueID<uint32_t, struct TemplatedFuncID> {
		using core::UniqueID<uint32_t, TemplatedFuncID>::UniqueID;
	};
	struct TemplatedFuncIDOptInterface{
		static constexpr auto init(TemplatedFuncID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const TemplatedFuncID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	struct TemplatedStructID : public core::UniqueID<uint32_t, struct TemplatedStructID> {
		using core::UniqueID<uint32_t, TemplatedStructID>::UniqueID;
	};
	struct TemplatedStructIDOptInterface{
		static constexpr auto init(TemplatedStructID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const TemplatedStructID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};


	struct VarID : public core::UniqueID<uint32_t, struct VarID> {
		using core::UniqueID<uint32_t, VarID>::UniqueID;
	};
	struct VarIDOptInterface{
		static constexpr auto init(VarID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const VarID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	struct GlobalVarID : public core::UniqueID<uint32_t, struct GlobalVarID> {
		using core::UniqueID<uint32_t, GlobalVarID>::UniqueID;
	};
	struct GlobalVarIDOptInterface{
		static constexpr auto init(GlobalVarID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const GlobalVarID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	struct ParamID : public core::UniqueID<uint32_t, struct ParamID> {
		using core::UniqueID<uint32_t, ParamID>::UniqueID;
	};
	struct ParamIDOptInterface{
		static constexpr auto init(ParamID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const ParamID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	struct ReturnParamID : public core::UniqueID<uint32_t, struct ReturnParamID> {
		using core::UniqueID<uint32_t, ReturnParamID>::UniqueID;
	};
	struct ReturnParamIDOptInterface{
		static constexpr auto init(ReturnParamID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const ReturnParamID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	struct ErrorReturnParamID : public core::UniqueID<uint32_t, struct ErrorReturnParamID> {
		using core::UniqueID<uint32_t, ErrorReturnParamID>::UniqueID;
	};
	struct ErrorReturnParamIDOptInterface{
		static constexpr auto init(ErrorReturnParamID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const ErrorReturnParamID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	struct BlockExprOutputID : public core::UniqueID<uint32_t, struct BlockExprOutputID> {
		using core::UniqueID<uint32_t, BlockExprOutputID>::UniqueID;
	};
	struct BlockExprOutputIDOptInterface{
		static constexpr auto init(BlockExprOutputID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const BlockExprOutputID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

	struct ExceptParamID : public core::UniqueID<uint32_t, struct ExceptParamID> {
		using core::UniqueID<uint32_t, ExceptParamID>::UniqueID;
	};
	struct ExceptParamIDOptInterface{
		static constexpr auto init(ExceptParamID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const ExceptParamID& id) -> bool {
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
	class optional<pcit::panther::sema::UninitID>
		: public pcit::core::Optional<pcit::panther::sema::UninitID, pcit::panther::sema::UninitIDOptInterface>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::UninitID, pcit::panther::sema::UninitIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::UninitID, pcit::panther::sema::UninitIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::ZeroinitID>{
		auto operator()(pcit::panther::sema::ZeroinitID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::ZeroinitID>
		: public pcit::core::Optional<pcit::panther::sema::ZeroinitID, pcit::panther::sema::ZeroinitIDOptInterface>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::ZeroinitID, pcit::panther::sema::ZeroinitIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::ZeroinitID, pcit::panther::sema::ZeroinitIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::IntValueID>{
		auto operator()(pcit::panther::sema::IntValueID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::IntValueID>
		: public pcit::core::Optional<pcit::panther::sema::IntValueID, pcit::panther::sema::IntValueIDOptInterface>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::IntValueID, pcit::panther::sema::IntValueIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::IntValueID, pcit::panther::sema::IntValueIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::FloatValueID>{
		auto operator()(pcit::panther::sema::FloatValueID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::FloatValueID>
		: public pcit::core::Optional<pcit::panther::sema::FloatValueID, pcit::panther::sema::FloatValueIDOptInterface>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::FloatValueID, pcit::panther::sema::FloatValueIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::FloatValueID, pcit::panther::sema::FloatValueIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::BoolValueID>{
		auto operator()(pcit::panther::sema::BoolValueID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::BoolValueID>
		: public pcit::core::Optional<pcit::panther::sema::BoolValueID, pcit::panther::sema::BoolValueIDOptInterface>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::BoolValueID, pcit::panther::sema::BoolValueIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::BoolValueID, pcit::panther::sema::BoolValueIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::StringValueID>{
		auto operator()(pcit::panther::sema::StringValueID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::StringValueID>
		: public pcit::core::Optional<
			pcit::panther::sema::StringValueID, pcit::panther::sema::StringValueIDOptInterface
		>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::StringValueID, pcit::panther::sema::StringValueIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::StringValueID, pcit::panther::sema::StringValueIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::AggregateValueID>{
		auto operator()(pcit::panther::sema::AggregateValueID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::AggregateValueID>
		: public pcit::core::Optional<
			pcit::panther::sema::AggregateValueID, pcit::panther::sema::AggregateValueIDOptInterface
		>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::AggregateValueID, pcit::panther::sema::AggregateValueIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::AggregateValueID, pcit::panther::sema::AggregateValueIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::CharValueID>{
		auto operator()(pcit::panther::sema::CharValueID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::CharValueID>
		: public pcit::core::Optional<pcit::panther::sema::CharValueID, pcit::panther::sema::CharValueIDOptInterface>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::CharValueID, pcit::panther::sema::CharValueIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::CharValueID, pcit::panther::sema::CharValueIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::TemplateIntrinsicFuncInstantiationID>{
		auto operator()(pcit::panther::sema::TemplateIntrinsicFuncInstantiationID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::TemplateIntrinsicFuncInstantiationID>
		: public pcit::core::Optional<
			pcit::panther::sema::TemplateIntrinsicFuncInstantiationID,
			pcit::panther::sema::TemplateIntrinsicFuncInstantiationIDOptInterface
		>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::TemplateIntrinsicFuncInstantiationID,
				pcit::panther::sema::TemplateIntrinsicFuncInstantiationIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::TemplateIntrinsicFuncInstantiationID,
				pcit::panther::sema::TemplateIntrinsicFuncInstantiationIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::CopyID>{
		auto operator()(pcit::panther::sema::CopyID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::CopyID>
		: public pcit::core::Optional<pcit::panther::sema::CopyID, pcit::panther::sema::CopyIDOptInterface>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::CopyID, pcit::panther::sema::CopyIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::CopyID, pcit::panther::sema::CopyIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::MoveID>{
		auto operator()(pcit::panther::sema::MoveID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::MoveID>
		: public pcit::core::Optional<pcit::panther::sema::MoveID, pcit::panther::sema::MoveIDOptInterface>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::MoveID, pcit::panther::sema::MoveIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::MoveID, pcit::panther::sema::MoveIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::ForwardID>{
		auto operator()(pcit::panther::sema::ForwardID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::ForwardID>
		: public pcit::core::Optional<pcit::panther::sema::ForwardID, pcit::panther::sema::ForwardIDOptInterface>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::ForwardID, pcit::panther::sema::ForwardIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::ForwardID, pcit::panther::sema::ForwardIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::AddrOfID>{
		auto operator()(pcit::panther::sema::AddrOfID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::AddrOfID>
		: public pcit::core::Optional<pcit::panther::sema::AddrOfID, pcit::panther::sema::AddrOfIDOptInterface>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::AddrOfID, pcit::panther::sema::AddrOfIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::AddrOfID, pcit::panther::sema::AddrOfIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::DerefID>{
		auto operator()(pcit::panther::sema::DerefID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::DerefID>
		: public pcit::core::Optional<pcit::panther::sema::DerefID, pcit::panther::sema::DerefIDOptInterface>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::DerefID, pcit::panther::sema::DerefIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::DerefID, pcit::panther::sema::DerefIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::AccessorID>{
		auto operator()(pcit::panther::sema::AccessorID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::AccessorID>
		: public pcit::core::Optional<pcit::panther::sema::AccessorID, pcit::panther::sema::AccessorIDOptInterface>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::AccessorID, pcit::panther::sema::AccessorIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::AccessorID, pcit::panther::sema::AccessorIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::PtrAccessorID>{
		auto operator()(pcit::panther::sema::PtrAccessorID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::PtrAccessorID>
		: public pcit::core::Optional<
			pcit::panther::sema::PtrAccessorID, pcit::panther::sema::PtrAccessorIDOptInterface
		>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::PtrAccessorID, pcit::panther::sema::PtrAccessorIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::PtrAccessorID, pcit::panther::sema::PtrAccessorIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::TryElseID>{
		auto operator()(pcit::panther::sema::TryElseID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::TryElseID>
		: public pcit::core::Optional<pcit::panther::sema::TryElseID, pcit::panther::sema::TryElseIDOptInterface>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::TryElseID, pcit::panther::sema::TryElseIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::TryElseID, pcit::panther::sema::TryElseIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::BlockExprID>{
		auto operator()(pcit::panther::sema::BlockExprID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::BlockExprID>
		: public pcit::core::Optional<pcit::panther::sema::BlockExprID, pcit::panther::sema::BlockExprIDOptInterface>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::BlockExprID, pcit::panther::sema::BlockExprIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::BlockExprID, pcit::panther::sema::BlockExprIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::FakeTermInfoID>{
		auto operator()(pcit::panther::sema::FakeTermInfoID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::FakeTermInfoID>
		: public pcit::core::Optional<pcit::panther::sema::FakeTermInfoID, pcit::panther::sema::FakeTermInfoIDOptInterface>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::FakeTermInfoID, pcit::panther::sema::FakeTermInfoIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::FakeTermInfoID, pcit::panther::sema::FakeTermInfoIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::FuncCallID>{
		auto operator()(pcit::panther::sema::FuncCallID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::FuncCallID>
		: public pcit::core::Optional<pcit::panther::sema::FuncCallID, pcit::panther::sema::FuncCallIDOptInterface>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::FuncCallID, pcit::panther::sema::FuncCallIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::FuncCallID, pcit::panther::sema::FuncCallIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::AssignID>{
		auto operator()(pcit::panther::sema::AssignID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::AssignID>
		: public pcit::core::Optional<pcit::panther::sema::AssignID, pcit::panther::sema::AssignIDOptInterface>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::AssignID, pcit::panther::sema::AssignIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::AssignID, pcit::panther::sema::AssignIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::MultiAssignID>{
		auto operator()(pcit::panther::sema::MultiAssignID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::MultiAssignID>
		: public pcit::core::Optional<
			pcit::panther::sema::MultiAssignID, pcit::panther::sema::MultiAssignIDOptInterface
		>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::MultiAssignID, pcit::panther::sema::MultiAssignIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::MultiAssignID, pcit::panther::sema::MultiAssignIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::ReturnID>{
		auto operator()(pcit::panther::sema::ReturnID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::ReturnID>
		: public pcit::core::Optional<pcit::panther::sema::ReturnID, pcit::panther::sema::ReturnIDOptInterface>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::ReturnID, pcit::panther::sema::ReturnIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::ReturnID, pcit::panther::sema::ReturnIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::ErrorID>{
		auto operator()(pcit::panther::sema::ErrorID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::ErrorID>
		: public pcit::core::Optional<pcit::panther::sema::ErrorID, pcit::panther::sema::ErrorIDOptInterface>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::ErrorID, pcit::panther::sema::ErrorIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::ErrorID, pcit::panther::sema::ErrorIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::ConditionalID>{
		auto operator()(pcit::panther::sema::ConditionalID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::ConditionalID>
		: public pcit::core::Optional<
			pcit::panther::sema::ConditionalID, pcit::panther::sema::ConditionalIDOptInterface
		>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::ConditionalID, pcit::panther::sema::ConditionalIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::ConditionalID, pcit::panther::sema::ConditionalIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::WhileID>{
		auto operator()(pcit::panther::sema::WhileID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::WhileID>
		: public pcit::core::Optional<pcit::panther::sema::WhileID, pcit::panther::sema::WhileIDOptInterface>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::WhileID, pcit::panther::sema::WhileIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::WhileID, pcit::panther::sema::WhileIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::DeferID>{
		auto operator()(pcit::panther::sema::DeferID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::DeferID>
		: public pcit::core::Optional<pcit::panther::sema::DeferID, pcit::panther::sema::DeferIDOptInterface>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::DeferID, pcit::panther::sema::DeferIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::DeferID, pcit::panther::sema::DeferIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::FuncID>{
		auto operator()(pcit::panther::sema::FuncID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::FuncID>
		: public pcit::core::Optional<pcit::panther::sema::FuncID, pcit::panther::sema::FuncIDOptInterface>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::FuncID, pcit::panther::sema::FuncIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::FuncID, pcit::panther::sema::FuncIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::TemplatedFuncID>{
		auto operator()(pcit::panther::sema::TemplatedFuncID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::TemplatedFuncID>
		: public pcit::core::Optional<
			pcit::panther::sema::TemplatedFuncID, pcit::panther::sema::TemplatedFuncIDOptInterface
		>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::TemplatedFuncID, pcit::panther::sema::TemplatedFuncIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::TemplatedFuncID, pcit::panther::sema::TemplatedFuncIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::TemplatedStructID>{
		auto operator()(pcit::panther::sema::TemplatedStructID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::TemplatedStructID>
		: public pcit::core::Optional<
			pcit::panther::sema::TemplatedStructID, pcit::panther::sema::TemplatedStructIDOptInterface
		>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::TemplatedStructID, pcit::panther::sema::TemplatedStructIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::TemplatedStructID, pcit::panther::sema::TemplatedStructIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::VarID>{
		auto operator()(pcit::panther::sema::VarID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::VarID>
		: public pcit::core::Optional<pcit::panther::sema::VarID, pcit::panther::sema::VarIDOptInterface>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::VarID, pcit::panther::sema::VarIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::VarID, pcit::panther::sema::VarIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::GlobalVarID>{
		auto operator()(pcit::panther::sema::GlobalVarID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::GlobalVarID>
		: public pcit::core::Optional<pcit::panther::sema::GlobalVarID, pcit::panther::sema::GlobalVarIDOptInterface>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::GlobalVarID, pcit::panther::sema::GlobalVarIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::GlobalVarID, pcit::panther::sema::GlobalVarIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::ParamID>{
		auto operator()(pcit::panther::sema::ParamID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::ParamID>
		: public pcit::core::Optional<pcit::panther::sema::ParamID, pcit::panther::sema::ParamIDOptInterface>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::ParamID, pcit::panther::sema::ParamIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::ParamID, pcit::panther::sema::ParamIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::ReturnParamID>{
		auto operator()(pcit::panther::sema::ReturnParamID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::ReturnParamID>
		: public pcit::core::Optional<
			pcit::panther::sema::ReturnParamID, pcit::panther::sema::ReturnParamIDOptInterface
		>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::ReturnParamID, pcit::panther::sema::ReturnParamIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::ReturnParamID, pcit::panther::sema::ReturnParamIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::ErrorReturnParamID>{
		auto operator()(pcit::panther::sema::ErrorReturnParamID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::ErrorReturnParamID>
		: public pcit::core::Optional<
			pcit::panther::sema::ErrorReturnParamID, pcit::panther::sema::ErrorReturnParamIDOptInterface
		>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::ErrorReturnParamID, pcit::panther::sema::ErrorReturnParamIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::ErrorReturnParamID, pcit::panther::sema::ErrorReturnParamIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::BlockExprOutputID>{
		auto operator()(pcit::panther::sema::BlockExprOutputID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::BlockExprOutputID>
		: public pcit::core::Optional<
			pcit::panther::sema::BlockExprOutputID, pcit::panther::sema::BlockExprOutputIDOptInterface
		>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::BlockExprOutputID, pcit::panther::sema::BlockExprOutputIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::BlockExprOutputID, pcit::panther::sema::BlockExprOutputIDOptInterface
			>::operator=;
	};



	template<>
	struct hash<pcit::panther::sema::ExceptParamID>{
		auto operator()(pcit::panther::sema::ExceptParamID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::sema::ExceptParamID>
		: public pcit::core::Optional<pcit::panther::sema::ExceptParamID, pcit::panther::sema::ExceptParamIDOptInterface>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::ExceptParamID, pcit::panther::sema::ExceptParamIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::sema::ExceptParamID, pcit::panther::sema::ExceptParamIDOptInterface
			>::operator=;
	};



}



