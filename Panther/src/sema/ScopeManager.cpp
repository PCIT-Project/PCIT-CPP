//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "./ScopeManager.h"


namespace pcit::panther::sema{

	//////////////////////////////////////////////////////////////////////
	// scope level

	auto ScopeManager::ScopeLevel::lookupFunc(std::string_view ident) const -> std::optional<ASG::Func::ID> {
		const std::unordered_map<std::string_view, ASG::Func::ID>::const_iterator lookup_iter = this->funcs.find(ident);
		if(lookup_iter == this->funcs.end()){ return std::nullopt; }

		return lookup_iter->second;
	}

	auto ScopeManager::ScopeLevel::addFunc(std::string_view ident, ASG::Func::ID id) -> void {
		evo::debugAssert(this->lookupFunc(ident).has_value() == false, "Scope already has function \"{}\"", ident);

		this->funcs.emplace(ident, id);
	}



	//////////////////////////////////////////////////////////////////////
	// scope manager
	
	ScopeManager::~ScopeManager(){
		for(ScopeLevel* scope_level : this->scope_levels){
			delete scope_level;
		}
	}


	auto ScopeManager::createScopeLevel() -> ScopeLevel::ID {
		const auto lock = std::unique_lock(this->mutex);

		const auto new_scope_level_id = ScopeLevel::ID(uint32_t(this->scope_levels.size()));
		// TODO: better allocation method
		this->scope_levels.emplace_back(new ScopeLevel());
		return new_scope_level_id;
	}



}