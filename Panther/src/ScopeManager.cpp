//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "../include/ScopeManager.h"

#include "../include/ASG.h"
#include "../include/Source.h"


namespace pcit::panther{

	//////////////////////////////////////////////////////////////////////
	// scope level

	auto ScopeManager::ScopeLevel::lookupFunc(std::string_view ident) const -> std::optional<ASG::Func::ID> {
		const std::unordered_map<std::string_view, ASG::Func::ID>::const_iterator lookup_iter = this->funcs.find(ident);
		if(lookup_iter == this->funcs.end()){ return std::nullopt; }

		return lookup_iter->second;
	}
	auto ScopeManager::ScopeLevel::addFunc(std::string_view ident, ASG::Func::ID id) -> void {
		evo::debugAssert(this->lookupFunc(ident).has_value() == false, "Scope already has func \"{}\"", ident);

		this->funcs.emplace(ident, id);
	}


	auto ScopeManager::ScopeLevel::lookupTemplatedFunc(std::string_view ident) const
	-> std::optional<ASG::TemplatedFunc::ID> {
		using LookupIterator = std::unordered_map<std::string_view, ASG::TemplatedFunc::ID>::const_iterator;
		const LookupIterator lookup_iter = this->templated_funcs.find(ident);
		if(lookup_iter == this->templated_funcs.end()){ return std::nullopt; }

		return lookup_iter->second;
	}
	auto ScopeManager::ScopeLevel::addTemplatedFunc(std::string_view ident, ASG::TemplatedFunc::ID id) -> void {
		evo::debugAssert(
			this->lookupTemplatedFunc(ident).has_value() == false, 
			"Scope already has templated func \"{}\"", ident
		);

		this->templated_funcs.emplace(ident, id);
	}


	auto ScopeManager::ScopeLevel::lookupVar(std::string_view ident) const -> std::optional<ASG::Var::ID> {
		const std::unordered_map<std::string_view, ASG::Var::ID>::const_iterator lookup_iter = this->vars.find(ident);
		if(lookup_iter == this->vars.end()){ return std::nullopt; }

		return lookup_iter->second;
	}
	auto ScopeManager::ScopeLevel::addVar(std::string_view ident, ASG::Var::ID id) -> void {
		evo::debugAssert(this->lookupVar(ident).has_value() == false, "Scope already has var \"{}\"", ident);

		this->vars.emplace(ident, id);
	}



	auto ScopeManager::ScopeLevel::lookupImport(std::string_view ident) const -> std::optional<Source::ID> {
		const std::unordered_map<std::string_view, Source::ID>::const_iterator lookup_iter = this->imports.find(ident);
		if(lookup_iter == this->imports.end()){ return std::nullopt; }

		return lookup_iter->second;
	}
	auto ScopeManager::ScopeLevel::getImportLocation(std::string_view ident) const -> Token::ID {
		using LookupIter = std::unordered_map<std::string_view, Token::ID>::const_iterator;
		const LookupIter lookup_iter = this->import_locations.find(ident);
		evo::debugAssert(lookup_iter != this->import_locations.end(), "scope level does not have import \"{}\"", ident);

		return lookup_iter->second;
	}
	auto ScopeManager::ScopeLevel::addImport(std::string_view ident, Source::ID id, Token::ID location) -> void {
		evo::debugAssert(this->lookupImport(ident).has_value() == false, "Scope already has var \"{}\"", ident);

		this->imports.emplace(ident, id);
		this->import_locations.emplace(ident, location);
	}


	//////////////////////////////////////////////////////////////////////
	// scope

	auto ScopeManager::Scope::pushScopeLevel(ScopeLevel::ID id) -> void {
		this->scope_levels.emplace_back(id);
	}

	auto ScopeManager::Scope::pushScopeLevel(ScopeLevel::ID id, ASG::Func::ID func_id) -> void {
		this->scope_levels.emplace_back(id);
		this->object_scopes.emplace_back(ObjectScope(func_id), uint32_t(this->scope_levels.size()));
	}

	auto ScopeManager::Scope::popScopeLevel() -> void {
		evo::debugAssert(!this->scope_levels.empty(), "cannot pop scope level as there are none");
		evo::debugAssert(
			this->getCurrentObjectScope().is<std::monostate>() == false, "fake object scope was not popped"
		);

		if(
			this->inObjectScope() && 
			this->object_scopes.back().scope_level_index == uint32_t(this->scope_levels.size())
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


	auto GlobalScope::addVar(const AST::VarDecl& ast_var, ASG::Var::ID asg_var) -> void {
		this->vars.emplace_back(ast_var, asg_var);
	}

	auto GlobalScope::getVars() const -> evo::ArrayProxy<Var> {
		return this->vars;
	}


}