//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once

#include <shared_mutex>

#include <PCIT_core.h>

#include "../../include/ASG.h"

namespace pcit::panther::sema{

	
	class ScopeManager{
		public:
			class ScopeLevel{
				public:
					class ID : public core::UniqueID<uint32_t, class ID> {
						public:
							using core::UniqueID<uint32_t, ID>::UniqueID;
					};

				public:
					ScopeLevel() = default;
					~ScopeLevel() = default;

					EVO_NODISCARD auto lookupFunc(std::string_view ident) const -> std::optional<ASG::Func::ID>;
					auto addFunc(std::string_view ident, ASG::Func::ID id) -> void;


				private:
					std::unordered_map<std::string_view, ASG::Func::ID> funcs{};
			};


			class Scope{ // not thread-safe
				public:
					using ObjectScope = evo::Variant<ASG::Func::ID>;

				public:
					Scope() = default;
					~Scope() = default;

					///////////////////////////////////
					// scope level

					auto pushScopeLevel(ScopeLevel::ID id) -> void;
					auto pushScopeLevel(ScopeLevel::ID id, ASG::Func::ID func_id) -> void;
					auto popScopeLevel() -> void;

					EVO_NODISCARD auto getCurrentScopeLevel() const -> ScopeLevel::ID {
						return this->scope_levels.back();
					}

					EVO_NODISCARD auto begin() -> evo::SmallVector<ScopeLevel::ID>::iterator {
						return this->scope_levels.begin();
					}

					EVO_NODISCARD auto begin() const -> evo::SmallVector<ScopeLevel::ID>::const_iterator {
						return this->scope_levels.begin();
					}

					EVO_NODISCARD auto end() -> evo::SmallVector<ScopeLevel::ID>::iterator {
						return this->scope_levels.end();
					}

					EVO_NODISCARD auto end() const -> evo::SmallVector<ScopeLevel::ID>::const_iterator {
						return this->scope_levels.end();
					}


					///////////////////////////////////
					// object scope

					EVO_NODISCARD auto inObjectScope() const -> bool { return !this->object_scopes.empty(); }
					EVO_NODISCARD auto getCurrentObjectScope() const -> const ObjectScope& {
						evo::debugAssert(this->inObjectScope(), "not in object scope");
						return this->object_scopes.back().obj_scope;
					}

				private:
					struct ObjectScopeData{
						ObjectScope obj_scope;
						evo::uint scope_level_index;
					};

					// TODO: use a stack?
					evo::SmallVector<ScopeLevel::ID> scope_levels{};
					evo::SmallVector<ObjectScopeData> object_scopes{};
			};
		
		public:
			ScopeManager() = default;
			~ScopeManager();

			EVO_NODISCARD auto operator[](ScopeLevel::ID id) const -> const ScopeLevel& {
				const auto lock = std::shared_lock(this->mutex);
				return *this->scope_levels[id.get()];
			}

			EVO_NODISCARD auto operator[](ScopeLevel::ID id) -> ScopeLevel& {
				const auto lock = std::shared_lock(this->mutex);
				return *this->scope_levels[id.get()];
			}


			EVO_NODISCARD auto createScopeLevel() -> ScopeLevel::ID;
	
		private:
			std::vector<ScopeLevel*> scope_levels{};
			mutable std::shared_mutex mutex{};
	};


	class GlobalScope{
		public:
			struct Func{
				const AST::FuncDecl& ast_func;
				ASG::Func::ID asg_func;
			};

		public:
			GlobalScope() = default;
			~GlobalScope() = default;

			auto addFunc(const AST::FuncDecl& ast_func, ASG::Func::ID asg_func) -> void;
			EVO_NODISCARD auto getFuncs() const -> evo::ArrayProxy<Func>;
	
		private:
			evo::SmallVector<Func> funcs{};
	};
	
	
}