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

#include "./deps.h"
#include "../ScopeManager.h"

namespace pcit::panther{


	class DepsBuffer{
		public:
			DepsBuffer() = default;
			~DepsBuffer() = default;


			EVO_NODISCARD auto operator[](deps::Node::ID id) const -> const deps::Node& {
				return this->nodes[id];
			}

			EVO_NODISCARD auto operator[](deps::Node::ID id) -> deps::Node& {
				return this->nodes[id];
			}

			using Iter = core::SyncLinearStepAlloc<deps::Node, deps::Node::ID>::Iter;
			using ConstIter = core::SyncLinearStepAlloc<deps::Node, deps::Node::ID>::ConstIter;

			EVO_NODISCARD auto begin()       ->      Iter { return this->nodes.begin(); }
			EVO_NODISCARD auto begin() const -> ConstIter { return this->nodes.begin(); }

			EVO_NODISCARD auto end()       ->      Iter { return this->nodes.end(); }
			EVO_NODISCARD auto end() const -> ConstIter { return this->nodes.end(); }


			EVO_NODISCARD auto getStruct(deps::Struct::ID id) const -> const deps::Struct& {
				return this->structs[id];
			}

			EVO_NODISCARD auto getStruct(deps::Struct::ID id) -> deps::Struct& {
				return this->structs[id];
			}


			EVO_NODISCARD auto getWhenCond(deps::WhenCond::ID id) const -> const deps::WhenCond& {
				return this->when_conds[id];
			}

			EVO_NODISCARD auto getWhenCond(deps::WhenCond::ID id) -> deps::WhenCond& {
				return this->when_conds[id];
			}


		private:
			EVO_NODISCARD auto create_node(auto&&... args) -> deps::Node::ID {
				this->num_nodes_sema_status_not_done += 1;
				return this->nodes.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto create_struct(auto&&... args) -> deps::Struct::ID {
				return this->structs.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto create_when_cond(auto&&... args) -> deps::WhenCond::ID {
				return this->when_conds.emplace_back(std::forward<decltype(args)>(args)...);
			}


			struct ScopeLevel{
				struct ID : public core::UniqueID<uint32_t, struct ID> {
					using core::UniqueID<uint32_t, ID>::UniqueID;
				};

				auto add_symbol(std::string_view ident, deps::Node::ID id) -> void {
					auto find = this->symbols.find(ident);
					if(this->symbols.end() == find){
						this->symbols.emplace(ident, evo::SmallVector<deps::Node::ID>()).first->second.emplace_back(id);
					}else{
						find->second.emplace_back(id);
					}
				}


				// TODO: switch to unordered_multimap?
				std::unordered_map<std::string_view, evo::SmallVector<deps::Node::ID>> symbols{};
			};

			using Scope = ScopeManager<ScopeLevel, evo::Variant<std::monostate, deps::Node::ID>>::Scope;


		private:
			core::SyncLinearStepAlloc<deps::Node, deps::Node::ID> nodes{}; // this being linear is depended on in Context
			core::SyncLinearStepAlloc<deps::Struct, deps::Struct::ID> structs{};
			core::SyncLinearStepAlloc<deps::WhenCond, deps::WhenCond::ID> when_conds{};

			std::atomic<size_t> num_nodes_sema_status_not_done = 0;

			ScopeManager<ScopeLevel, evo::Variant<std::monostate, deps::Node::ID>> scope_manager{};

			friend class Context;
			friend class Source;
			friend class SourceManager;
			friend class DependencyAnalysis;
			friend class SemanticAnalyzer;
	};

}
