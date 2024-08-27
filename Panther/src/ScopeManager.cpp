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

	auto ScopeManager::ScopeLevel::addSubScope() -> void {
		this->has_sub_scopes = true;
		this->num_sub_scopes_not_terminated += 1;
	}

	auto ScopeManager::ScopeLevel::setSubScopeTerminated() -> void {
		evo::debugAssert(this->num_sub_scopes_not_terminated != 0, "setSubScopeTerminated called too many times");
		this->num_sub_scopes_not_terminated -= 1;
	}

	auto ScopeManager::ScopeLevel::setTerminated() -> void {
		this->is_terminated = true;
	}

	auto ScopeManager::ScopeLevel::isTerminated() const -> bool {
		return this->is_terminated || (this->has_sub_scopes && this->num_sub_scopes_not_terminated == 0);
	}

	auto ScopeManager::ScopeLevel::isNotTerminated() const -> bool {
		return !this->isTerminated();
	}



	auto ScopeManager::ScopeLevel::addFunc(std::string_view ident, ASG::Func::ID id) -> void {
		evo::debugAssert(this->lookupIdent(ident).has_value() == false, "Scope already has ident \"{}\"", ident);

		this->ids.emplace(ident, id);
	}

	auto ScopeManager::ScopeLevel::addTemplatedFunc(std::string_view ident, ASG::TemplatedFunc::ID id) -> void {
		evo::debugAssert(this->lookupIdent(ident).has_value() == false, "Scope already has ident \"{}\"", ident);

		this->ids.emplace(ident, id);
	}

	auto ScopeManager::ScopeLevel::addVar(std::string_view ident, ASG::Var::ID id) -> void {
		evo::debugAssert(this->lookupIdent(ident).has_value() == false, "Scope already has ident \"{}\"", ident);

		this->ids.emplace(ident, id);
	}

	auto ScopeManager::ScopeLevel::addParam(std::string_view ident, ASG::Param::ID id) -> void {
		evo::debugAssert(this->lookupIdent(ident).has_value() == false, "Scope already has ident \"{}\"", ident);

		this->ids.emplace(ident, id);
	}

	auto ScopeManager::ScopeLevel::addReturnParam(std::string_view ident, ASG::ReturnParam::ID id) -> void {
		evo::debugAssert(this->lookupIdent(ident).has_value() == false, "Scope already has ident \"{}\"", ident);

		this->ids.emplace(ident, id);
	}

	auto ScopeManager::ScopeLevel::addImport(std::string_view ident, Source::ID id, Token::ID location) -> void {
		evo::debugAssert(this->lookupIdent(ident).has_value() == false, "Scope already has ident \"{}\"", ident);

		this->ids.emplace(ident, ImportInfo(id, location));
	}


	auto ScopeManager::ScopeLevel::lookupIdent(std::string_view ident) const -> std::optional<IdentID> {
		const auto& ident_find = this->ids.find(ident);
		if(ident_find == this->ids.end()){ return std::nullopt; }

		return ident_find->second;
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