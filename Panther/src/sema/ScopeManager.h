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

#include "./ScopeLevel.h"
#include "../../include/type_ids.h"


namespace pcit::panther::sema{


	class ScopeManager{
		public:
			class Scope{
				public:
					struct ID : public core::UniqueID<uint32_t, struct ID> {
						using core::UniqueID<uint32_t, ID>::UniqueID;
					};

					using FakeObjectScope = std::monostate;
					using ObjectScope = evo::Variant<FakeObjectScope, sema::Func::ID, BaseType::Struct::ID>;

				public:
					Scope() = default;
					~Scope() = default;

					///////////////////////////////////
					// scope level

					auto pushLevel(ScopeLevel::ID id) -> void { this->scope_levels.emplace_back(id); }

					// The type of `object_scope` must be one of the ones in ObjectScope
					auto pushLevel(ScopeLevel::ID id, auto&& object_scope) -> void {
						this->pushLevel(id);
						this->object_scopes.emplace_back(
							ObjectScope(std::move(object_scope)), uint32_t(this->scope_levels.size())
						);
					}

					// The type of `object_scope` must be one of the ones in ObjectScope
					auto pushLevel(ScopeLevel::ID id, const auto& object_scope) -> void {
						this->pushLevel(id);
						this->object_scopes.emplace_back(
							ObjectScope(object_scope), uint32_t(this->scope_levels.size())
						);
					}

					auto popLevel() -> void {
						evo::debugAssert(!this->scope_levels.empty(), "cannot pop scope level as there are none");
						evo::debugAssert(
							this->getCurrentObjectScope().is<FakeObjectScope>() == false,
							"fake object scope was not popped"
						);

						if(
							this->inObjectScope() && 
							this->object_scopes.back().scope_level_index == uint32_t(this->scope_levels.size())
						){
							this->object_scopes.pop_back();
						}
						
						this->scope_levels.pop_back();
					}

					EVO_NODISCARD auto getGlobalLevel() const -> ScopeLevel::ID { return this->scope_levels.front(); }
					EVO_NODISCARD auto getCurrentLevel() const -> ScopeLevel::ID { return this->scope_levels.back(); }


					EVO_NODISCARD auto size() const -> size_t { return this->scope_levels.size(); }

					// note: these are purposely reverse iterators
					// TODO: figure out if doing lookup in reverse is indeed faster

					EVO_NODISCARD auto begin() -> evo::SmallVector<ScopeLevel::ID>::reverse_iterator {
						return this->scope_levels.rbegin();
					}

					EVO_NODISCARD auto begin() const -> evo::SmallVector<ScopeLevel::ID>::const_reverse_iterator {
						return this->scope_levels.rbegin();
					}

					EVO_NODISCARD auto end() -> evo::SmallVector<ScopeLevel::ID>::reverse_iterator {
						return this->scope_levels.rend();
					}

					EVO_NODISCARD auto end() const -> evo::SmallVector<ScopeLevel::ID>::const_reverse_iterator {
						return this->scope_levels.rend();
					}


					///////////////////////////////////
					// object scope

					// pushing / popping happens automatically with `pushLevel` / `popLevel`

					EVO_NODISCARD auto inObjectScope() const -> bool { return !this->object_scopes.empty(); }
					EVO_NODISCARD auto getCurrentObjectScope() const -> const ObjectScope& {
						evo::debugAssert(this->inObjectScope(), "not in object scope");
						return this->object_scopes.back().obj_scope;
					}

					EVO_NODISCARD auto getCurrentObjectScopeIndex() const -> uint32_t {
						if(this->object_scopes.empty()){ return 0; }
						return this->object_scopes.back().scope_level_index - 1;
					}

					EVO_NODISCARD auto inObjectMainScope() const -> bool {
						evo::debugAssert(this->object_scopes.size() >= 1, "not in object scope");
						return this->object_scopes.back().scope_level_index - 1 == this->size();
					}


					EVO_NODISCARD auto getCurrentTypeScopeIfExists() const -> std::optional<ObjectScope> {
						for(auto iter = this->object_scopes.rbegin(); iter != this->object_scopes.rend(); ++iter){
							if(iter->obj_scope.is<BaseType::Struct::ID>()){ return iter->obj_scope; }
						}

						return std::nullopt;
					}


					// must be popped manually
					// be careful - only use when declaring things like params
					EVO_NODISCARD auto pushFakeObjectScope() -> void {
						this->object_scopes.emplace_back(FakeObjectScope(), uint32_t(this->scope_levels.size()) + 1);
					}

					EVO_NODISCARD auto popFakeObjectScope() -> void {
						evo::debugAssert(
							this->getCurrentObjectScope().is<FakeObjectScope>(), "not in a fake object scope"
						);
						this->object_scopes.pop_back();
					}
			
				private:
					struct ObjectScopeData{
						ObjectScope obj_scope;
						uint32_t scope_level_index;
					};

					// TODO: use a stack?
					evo::SmallVector<ScopeLevel::ID> scope_levels{};
					evo::SmallVector<ObjectScopeData> object_scopes{};
			};

		public:
			ScopeManager() = default;
			~ScopeManager() = default;


			EVO_NODISCARD auto getScope(Scope::ID id) const -> const Scope& {
				return this->scopes[id];
			}

			EVO_NODISCARD auto getScope(Scope::ID id) -> Scope& {
				return this->scopes[id];
			}

			EVO_NODISCARD auto createScope() -> Scope::ID {
				return this->scopes.emplace_back();
			}

			EVO_NODISCARD auto copyScope(Scope::ID id) -> Scope::ID {
				return this->scopes.emplace_back(this->getScope(id));
			}


			EVO_NODISCARD auto getLevel(ScopeLevel::ID id) const -> const ScopeLevel& {
				return this->levels[id];
			}

			EVO_NODISCARD auto getLevel(ScopeLevel::ID id) -> ScopeLevel& {
				return this->levels[id];
			}

			EVO_NODISCARD auto createLevel(sema::StmtBlock* stmt_block = nullptr) -> ScopeLevel::ID {
				return this->levels.emplace_back(stmt_block);
			}

	
		private:
			core::SyncLinearStepAlloc<ScopeLevel, ScopeLevel::ID> levels{};
			core::SyncLinearStepAlloc<Scope, Scope::ID> scopes{};
	};


	struct ScopeManagerScopeIDOptInterface{
		static constexpr auto init(ScopeManager::Scope::ID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const ScopeManager::Scope::ID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};


}



namespace std{


	template<>
	class optional<pcit::panther::sema::ScopeManager::Scope::ID> 
		: public pcit::core::Optional<
			pcit::panther::sema::ScopeManager::Scope::ID, pcit::panther::sema::ScopeManagerScopeIDOptInterface
		>{

		public:
			using pcit::core::Optional<
				pcit::panther::sema::ScopeManager::Scope::ID, pcit::panther::sema::ScopeManagerScopeIDOptInterface
			>::Optional;

			using pcit::core::Optional<
				pcit::panther::sema::ScopeManager::Scope::ID, pcit::panther::sema::ScopeManagerScopeIDOptInterface
			>::operator=;
	};

	
}

