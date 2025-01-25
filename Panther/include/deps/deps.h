////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <unordered_set>

#include <Evo.h>
#include <PCIT_core.h>

#include "../source/source_data.h"
#include "../../include/AST/AST.h"
#include "../ScopeManager.h"

namespace pcit::panther::deps{

	struct StructID : public core::UniqueID<uint32_t, struct StructID> {
		using core::UniqueID<uint32_t, StructID>::UniqueID;
	};

	struct WhenCondID : public core::UniqueID<uint32_t, struct WhenCondID> {
		using core::UniqueID<uint32_t, WhenCondID>::UniqueID;
	};



	// TODO: reorgamize and optimize ordering
	// TODO: move everything not used by all AST Kinds to `extraData`
	struct Node{
		// for lookup in Context::getDepsBuffer()
		struct ID : public core::UniqueID<uint32_t, struct ID> {
			using core::UniqueID<uint32_t, ID>::UniqueID;
		};

		enum class ValueStage{
			Comptime,
			Constexpr,
			Runtime,
			Unknown,
		};

		enum class SemaStatus : uint8_t {
			NotYet,
			ReadyWhenDeclDone,
			InQueue,
			Done,
		};

		using NoExtraData = std::monostate;
		using ExtraData = evo::Variant<NoExtraData, StructID, WhenCondID>;


		AST::Node astNode; // may only be VarDecl, FuncDecl, AliasDecl, TypedefDecl, StructDecl, WhenConditional
		SourceID sourceID;
		ValueStage valueStage;
		ExtraData extraData;

		bool usedInComptime = false; // this being true doesn't mean it's allowed to be

		mutable core::SpinLock lock{};


		// This is required for some reason (specializing std::hash for deps::Node::ID causes redefinition errors)
		struct IDHash{
			auto operator()(const ID& id) const -> size_t { return std::hash<uint32_t>{}(id.get()); };
		};

		// declaration dependencies
		struct {
			std::unordered_set<ID, IDHash> decls{};
			std::unordered_set<ID, IDHash> defs{};
		} declDeps;

		// definition dependencies
		struct {
			std::unordered_set<ID, IDHash> decls{};
			std::unordered_set<ID, IDHash> defs{};
		} defDeps;

		std::unordered_set<ID, IDHash> requiredBy{};

		SemaStatus declSemaStatus = SemaStatus::NotYet;
		SemaStatus defSemaStatus = SemaStatus::NotYet;


		EVO_NODISCARD auto hasNoDeclDeps() const -> bool {
			return this->declDeps.decls.empty() && this->declDeps.defs.empty();
		}

		EVO_NODISCARD auto hasNoDefDeps() const -> bool {
			return this->defDeps.decls.empty() && this->defDeps.defs.empty();
		}


		Node(const AST::Node& ast_node, SourceID source_id, ValueStage value_stage, ExtraData extra_data) 
			: astNode(ast_node), sourceID(source_id), valueStage(value_stage), extraData(extra_data) {}
	};


	struct Struct{
		using ID = StructID;

		ScopeManagerScopeID scopeID; // TODO: needed?
		std::unordered_multimap<std::string_view, Node::ID> members{};
	};


	struct WhenCond{
		using ID = WhenCondID;

		// These are the nodes that are in the body of the WhenCond (only 1 scope deep - to find all, you must recurse)
		evo::SmallVector<Node::ID> thenNodes{};
		evo::SmallVector<Node::ID> elseNodes{};
	};

}




