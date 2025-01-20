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

#include "./DG.h"
#include "../ScopeManager.h"

namespace pcit::panther{


	class DGBuffer{
		public:
			DGBuffer() = default;
			~DGBuffer() = default;


			EVO_NODISCARD auto operator[](DG::Node::ID id) const -> const DG::Node& {
				return this->nodes[id];
			}

			EVO_NODISCARD auto operator[](DG::Node::ID id) -> DG::Node& {
				return this->nodes[id];
			}

			using Iter = core::SyncLinearStepAlloc<DG::Node, DG::Node::ID>::Iter;
			using ConstIter = core::SyncLinearStepAlloc<DG::Node, DG::Node::ID>::ConstIter;

			EVO_NODISCARD auto begin()       ->      Iter { return this->nodes.begin(); }
			EVO_NODISCARD auto begin() const -> ConstIter { return this->nodes.begin(); }

			EVO_NODISCARD auto end()       ->      Iter { return this->nodes.end(); }
			EVO_NODISCARD auto end() const -> ConstIter { return this->nodes.end(); }


			EVO_NODISCARD auto getStruct(DG::Struct::ID id) const -> const DG::Struct& {
				return this->structs[id];
			}

			EVO_NODISCARD auto getStruct(DG::Struct::ID id) -> DG::Struct& {
				return this->structs[id];
			}


			EVO_NODISCARD auto getWhenCond(DG::WhenCond::ID id) const -> const DG::WhenCond& {
				return this->when_conds[id];
			}

			EVO_NODISCARD auto getWhenCond(DG::WhenCond::ID id) -> DG::WhenCond& {
				return this->when_conds[id];
			}


		private:
			EVO_NODISCARD auto create_node(auto&&... args) -> DG::Node::ID {
				this->num_nodes_sema_status_not_done += 1;
				return this->nodes.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto create_struct(auto&&... args) -> DG::Struct::ID {
				return this->structs.emplace_back(std::forward<decltype(args)>(args)...);
			}

			EVO_NODISCARD auto create_when_cond(auto&&... args) -> DG::WhenCond::ID {
				return this->when_conds.emplace_back(std::forward<decltype(args)>(args)...);
			}


			struct ScopeLevel{
				struct ID : public core::UniqueID<uint32_t, struct ID> {
					using core::UniqueID<uint32_t, ID>::UniqueID;
				};

				auto add_symbol(std::string_view ident, DG::Node::ID id) -> void {
					auto find = this->symbols.find(ident);
					if(this->symbols.end() == find){
						this->symbols.emplace(ident, evo::SmallVector<DG::Node::ID>()).first->second.emplace_back(id);
					}else{
						find->second.emplace_back(id);
					}
				}


				// TODO: switch to unordered_multimap?
				std::unordered_map<std::string_view, evo::SmallVector<DG::Node::ID>> symbols{};
			};

			using Scope = ScopeManager<ScopeLevel, evo::Variant<std::monostate, DG::Node::ID>>::Scope;


		private:
			core::SyncLinearStepAlloc<DG::Node, DG::Node::ID> nodes{}; // this being linear is depended on in Context
			core::SyncLinearStepAlloc<DG::Struct, DG::Struct::ID> structs{};
			core::SyncLinearStepAlloc<DG::WhenCond, DG::WhenCond::ID> when_conds{};
			ScopeManager<ScopeLevel, evo::Variant<std::monostate, DG::Node::ID>> scope_manager{};

			std::atomic<size_t> num_nodes_sema_status_not_done = 0; 


			friend class DependencyAnalysis;
			friend class Context;
			friend class Source;
			friend class SemanticAnalyzer;
	};

}
