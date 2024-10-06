//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>
#include <PCIT_core.h>

#include "./source_data.h"


namespace pcit::panther::ASG{

	// All IDs defined here are used as indexes into ASGBuffer (found in Source)

	//////////////////////////////////////////////////////////////////////
	// expressions

	struct UninitID : public core::UniqueID<uint32_t, struct UninitID> {
		using core::UniqueID<uint32_t, UninitID>::UniqueID;
	};

	struct ZeroinitID : public core::UniqueID<uint32_t, struct ZeroinitID> {
		using core::UniqueID<uint32_t, ZeroinitID>::UniqueID;
	};


	struct LiteralIntID : public core::UniqueID<uint32_t, struct LiteralIntID> {
		using core::UniqueID<uint32_t, LiteralIntID>::UniqueID;
	};

	struct LiteralFloatID : public core::UniqueID<uint32_t, struct LiteralFloatID> {
		using core::UniqueID<uint32_t, LiteralFloatID>::UniqueID;
	};

	struct LiteralBoolID : public core::UniqueID<uint32_t, struct LiteralBoolID> {
		using core::UniqueID<uint32_t, LiteralBoolID>::UniqueID;
	};

	struct LiteralCharID : public core::UniqueID<uint32_t, struct LiteralCharID> {
		using core::UniqueID<uint32_t, LiteralCharID>::UniqueID;
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


	struct FuncID : public core::UniqueID<uint32_t, struct FuncID> {using core::UniqueID<uint32_t, FuncID>::UniqueID;};

	using Parent = evo::Variant<std::monostate, FuncID>;


	struct FuncLinkID{
		FuncLinkID(SourceID source_id, FuncID func_id) : _source_id(source_id), _func_id(func_id) {}

		EVO_NODISCARD auto sourceID() const -> SourceID { return this->_source_id; }
		EVO_NODISCARD auto funcID() const -> FuncID { return this->_func_id; }

		EVO_NODISCARD auto operator==(const FuncLinkID& rhs) const -> bool {
			return this->_source_id == rhs._source_id && this->_func_id == rhs._func_id;
		}

		EVO_NODISCARD auto operator!=(const FuncLinkID& rhs) const -> bool {
			return this->_source_id != rhs._source_id || this->_func_id != rhs._func_id;
		}
		
		private:
			SourceID _source_id;
			FuncID _func_id;
	};


	struct TemplatedFuncID : public core::UniqueID<uint32_t, struct TemplatedFuncID> {
		using core::UniqueID<uint32_t, TemplatedFuncID>::UniqueID;
	};


	struct VarID : public core::UniqueID<uint32_t, struct VarID> { using core::UniqueID<uint32_t, VarID>::UniqueID;	};

	struct VarLinkID{
		VarLinkID(SourceID source_id, VarID var_id) : _source_id(source_id), _var_id(var_id) {}

		EVO_NODISCARD auto sourceID() const -> SourceID { return this->_source_id; }
		EVO_NODISCARD auto varID() const -> VarID { return this->_var_id; }

		EVO_NODISCARD auto operator==(const VarLinkID& rhs) const -> bool {
			return this->_source_id == rhs._source_id && this->_var_id == rhs._var_id;
		}

		EVO_NODISCARD auto operator!=(const VarLinkID& rhs) const -> bool {
			return this->_source_id != rhs._source_id || this->_var_id != rhs._var_id;
		}
		
		private:
			SourceID _source_id;
			VarID _var_id;
	};


	struct ParamID : public core::UniqueID<uint32_t, struct ParamID> {
		using core::UniqueID<uint32_t, ParamID>::UniqueID;
	};

	struct ParamLinkID{
		ParamLinkID(FuncLinkID func_link_id, ParamID param_id) : _func_link_id(func_link_id), _param_id(param_id) {}

		EVO_NODISCARD auto funcLinkID() const -> FuncLinkID { return this->_func_link_id; }
		EVO_NODISCARD auto paramID() const -> ParamID { return this->_param_id; }

		EVO_NODISCARD auto operator==(const ParamLinkID& rhs) const -> bool {
			return this->_func_link_id == rhs._func_link_id && this->_param_id == rhs._param_id;
		}

		EVO_NODISCARD auto operator!=(const ParamLinkID& rhs) const -> bool {
			return this->_func_link_id != rhs._func_link_id || this->_param_id != rhs._param_id;
		}
		
		private:
			FuncLinkID _func_link_id;
			ParamID _param_id;
	};


	struct ReturnParamID : public core::UniqueID<uint32_t, struct ReturnParamID> {
		using core::UniqueID<uint32_t, ReturnParamID>::UniqueID;
	};

	struct ReturnParamLinkID{
		ReturnParamLinkID(FuncLinkID func_link_id, ReturnParamID return_param_id)
			: _func_link_id(func_link_id), _return_param_id(return_param_id) {}

		EVO_NODISCARD auto funcLinkID() const -> FuncLinkID { return this->_func_link_id; }
		EVO_NODISCARD auto returnParamID() const -> ReturnParamID { return this->_return_param_id; }

		EVO_NODISCARD auto operator==(const ReturnParamLinkID&) const -> bool = default;

		
		private:
			FuncLinkID _func_link_id;
			ReturnParamID _return_param_id;
	};
}


template<>
struct std::hash<pcit::panther::ASG::FuncLinkID>{
	auto operator()(const pcit::panther::ASG::FuncLinkID& link_id) const noexcept -> size_t {
		auto hasher = std::hash<uint32_t>{};
		return evo::hashCombine(hasher(link_id.sourceID().get()), hasher(link_id.funcID().get()));
	};
};


template<>
struct std::hash<pcit::panther::ASG::VarLinkID>{
	auto operator()(const pcit::panther::ASG::VarLinkID& link_id) const noexcept -> size_t {
		auto hasher = std::hash<uint32_t>{};
		return evo::hashCombine(hasher(link_id.sourceID().get()), hasher(link_id.varID().get()));
	};
};

template<>
struct std::hash<pcit::panther::ASG::ParamLinkID>{
	auto operator()(const pcit::panther::ASG::ParamLinkID& link_id) const noexcept -> size_t {
		return evo::hashCombine(
			std::hash<pcit::panther::ASG::FuncLinkID>{}(link_id.funcLinkID()),
			std::hash<uint32_t>{}(link_id.paramID().get())
		);
	};
};


template<>
struct std::hash<pcit::panther::ASG::ReturnParamLinkID>{
	auto operator()(const pcit::panther::ASG::ReturnParamLinkID& link_id) const noexcept -> size_t {
		return evo::hashCombine(
			std::hash<pcit::panther::ASG::FuncLinkID>{}(link_id.funcLinkID()),
			std::hash<uint32_t>{}(link_id.returnParamID().get())
		);
	};
};


