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

					using ObjectScope = evo::Variant<sema::Func::ID, BaseType::Struct::ID, BaseType::Interface::ID>;

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

						if(
							this->inObjectScope() && 
							this->object_scopes.back().scope_level_index == uint32_t(this->scope_levels.size())
						){
							this->object_scopes.pop_back();
						}
						
						this->scope_levels.pop_back();
					}

					EVO_NODISCARD auto isGlobalScope() const -> bool { return this->scope_levels.size() == 1; }
					EVO_NODISCARD auto getGlobalLevel() const -> ScopeLevel::ID { return this->scope_levels.front(); }
					EVO_NODISCARD auto getCurrentLevel() const -> ScopeLevel::ID { return this->scope_levels.back(); }


					EVO_NODISCARD auto size() const -> size_t { return this->scope_levels.size(); }

					// note: these are purposely reverse iterators
					// TODO(PERF): figure out if doing lookup in reverse is indeed faster
					//             	If not, make sure to fix any place that assumes going in reverse

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
						evo::debugAssert(this->inObjectScope(), "not in object scope");
						return this->object_scopes.back().scope_level_index - 1 == this->size();
					}


					EVO_NODISCARD auto getCurrentTypeScopeIfExists() const -> std::optional<ObjectScope> {
						for(auto iter = this->object_scopes.rbegin(); iter != this->object_scopes.rend(); ++iter){
							if(iter->obj_scope.is<BaseType::Struct::ID>()){ return iter->obj_scope; }
						}

						return std::nullopt;
					}

					EVO_NODISCARD auto getCurrentInterfaceScopeIfExists() const -> std::optional<ObjectScope> {
						for(auto iter = this->object_scopes.rbegin(); iter != this->object_scopes.rend(); ++iter){
							if(iter->obj_scope.is<BaseType::Interface::ID>()){ return iter->obj_scope; }
						}

						return std::nullopt;
					}



					auto addThisParam(sema::Param::ID param_id) -> void {
						evo::debugAssert(
							this->object_scopes.empty() == false
								&& this->object_scopes.back().obj_scope.is<sema::Func::ID>(),
							"Cannot set [this] param not in an a func object scope"
						);

						evo::debugAssert(
							this->object_scopes.back().this_param.has_value() == false,
							"[this] param was already set for this function scope"
						);

						this->object_scopes.back().this_param = param_id;
					}


					EVO_NODISCARD auto getThisParam() const -> std::optional<sema::Param::ID> {
						evo::debugAssert(
							this->object_scopes.empty() == false
								&& this->object_scopes.back().obj_scope.is<sema::Func::ID>(),
							"Cannot get [this] param not in an a func object scope"
						);

						return this->object_scopes.back().this_param;
					}


					///////////////////////////////////
					// template instantiation types

					auto pushTemplateDeclInstantiationTypesScope() -> void {
						this->template_decl_instantiation_types.emplace_back();
					}

					auto popTemplateDeclInstantiationTypesScope() -> void {
						evo::debugAssert(
							this->template_decl_instantiation_types.empty() == false,
							"no template instantiation type scopes exist"
						);

						this->template_decl_instantiation_types.pop_back();
					}

					auto addTemplateDeclInstantiationType(
						std::string_view ident, std::optional<TypeInfo::VoidableID> type
					) -> void {
						evo::debugAssert(
							this->template_decl_instantiation_types.empty() == false,
							"no template instantiation type scopes exist"
						);

						this->template_decl_instantiation_types.back().emplace(ident, type);
					}

					auto lookupTemplateDeclInstantiationType(std::string_view ident) const
					-> evo::Result<std::optional<TypeInfo::VoidableID>> {
						if(this->template_decl_instantiation_types.empty()){ return evo::resultError; }
						
						const auto find = this->template_decl_instantiation_types.back().find(ident);
						if(find == this->template_decl_instantiation_types.back().end()){ return evo::resultError; }
						return find->second;
					}


				private:
					struct ObjectScopeData{
						ObjectScope obj_scope;
						uint32_t scope_level_index;
						std::optional<sema::Param::ID> this_param;
					};

					// TODO(PERF): use a stack?
					evo::SmallVector<ScopeLevel::ID> scope_levels{};
					evo::SmallVector<ObjectScopeData> object_scopes{};
					evo::SmallVector<
						std::unordered_map<std::string_view, std::optional<TypeInfo::VoidableID>>
					> template_decl_instantiation_types{};
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

			EVO_NODISCARD auto createLevel(auto&&... args) -> ScopeLevel::ID {
				return this->levels.emplace_back(std::forward<decltype(args)>(args)...);
			}

	
		private:
			core::SyncLinearStepAlloc<ScopeLevel, ScopeLevel::ID> levels{};
			core::SyncLinearStepAlloc<Scope, Scope::ID> scopes{};
	};



}



namespace pcit::core{

	template<>
	struct OptionalInterface<panther::sema::ScopeManager::Scope::ID>{
		static constexpr auto init(panther::sema::ScopeManager::Scope::ID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::sema::ScopeManager::Scope::ID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

}



namespace std{


	template<>
	class optional<pcit::panther::sema::ScopeManager::Scope::ID> 
		: public pcit::core::Optional<pcit::panther::sema::ScopeManager::Scope::ID>{
		public:
			using pcit::core::Optional<pcit::panther::sema::ScopeManager::Scope::ID>::Optional;

			using pcit::core::Optional<pcit::panther::sema::ScopeManager::Scope::ID>::operator=;
	};

	
}

