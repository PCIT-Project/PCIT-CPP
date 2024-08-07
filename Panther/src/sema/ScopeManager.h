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
					Scope() = default;
					~Scope() = default;

					auto addScopeLevel(ScopeLevel::ID id) -> void { this->scope_levels.emplace_back(id); };
					auto removeScopeLevel() -> void { this->scope_levels.pop_back(); };

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
			
				private:
					// TODO: use a stack?
					evo::SmallVector<ScopeLevel::ID> scope_levels;
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
	
	
}