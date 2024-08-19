//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
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


	struct CopyID : public core::UniqueID<uint32_t, struct CopyID> {
		using core::UniqueID<uint32_t, CopyID>::UniqueID;
	};


	//////////////////////////////////////////////////////////////////////
	// statements


	struct FuncCallID : public core::UniqueID<uint32_t, struct FuncCallID> {
		using core::UniqueID<uint32_t, FuncCallID>::UniqueID;
	};

	struct AssignID : public core::UniqueID<uint32_t, struct AssignID> {
		using core::UniqueID<uint32_t, AssignID>::UniqueID;
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

