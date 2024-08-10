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
	// scope

	auto ScopeManager::Scope::pushScopeLevel(ScopeLevel::ID id) -> void {
		this->scope_levels.emplace_back(id);
	}

	auto ScopeManager::Scope::pushScopeLevel(ScopeLevel::ID id, ASG::Func::ID func_id) -> void {
		this->scope_levels.emplace_back(id);
		this->object_scopes.emplace_back(ObjectScope(func_id), evo::uint(this->scope_levels.size()));
	}

	auto ScopeManager::Scope::popScopeLevel() -> void {
		evo::debugAssert(!this->scope_levels.empty(), "cannot pop scope level as there are none");

		if(
			this->inObjectScope() && 
			this->object_scopes.back().scope_level_index == evo::uint(this->scope_levels.size())
		){
			this->object_scopes.pop_back();		
		}

		this->scope_levels.pop_back();
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



	//////////////////////////////////////////////////////////////////////
	// global scope

	auto GlobalScope::addFunc(const AST::FuncDecl& ast_func, ASG::Func::ID asg_func) -> void {
		this->funcs.emplace_back(ast_func, asg_func);
	}

	auto GlobalScope::getFuncs() const -> evo::ArrayProxy<Func> {
		return this->funcs;
	}


}