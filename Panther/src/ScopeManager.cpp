//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
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

	auto ScopeManager::Level::hasStmtBlock() const -> bool {
		return this->_stmt_block != nullptr;
	}

	auto ScopeManager::Level::stmtBlock() const -> const ASG::StmtBlock& {
		evo::debugAssert(this->_stmt_block != nullptr, "this scope doesn't have a stmt block");

		return *this->_stmt_block;
	}

	auto ScopeManager::Level::stmtBlock() -> ASG::StmtBlock& {
		evo::debugAssert(this->_stmt_block != nullptr, "this scope doesn't have a stmt block");

		return *this->_stmt_block;
	}



	auto ScopeManager::Level::addSubScope() -> void {
		this->has_sub_scopes = true;
		this->num_sub_scopes_not_terminated += 1;
	}

	auto ScopeManager::Level::setSubScopeTerminated() -> void {
		evo::debugAssert(this->num_sub_scopes_not_terminated != 0, "setSubScopeTerminated called too many times");
		this->num_sub_scopes_not_terminated -= 1;
	}

	auto ScopeManager::Level::setTerminated() -> void {
		this->_stmt_block->setTerminated();
	}

	auto ScopeManager::Level::isTerminated() const -> bool {
		return this->_stmt_block->isTerminated() || (this->has_sub_scopes && this->num_sub_scopes_not_terminated == 0);
	}

	auto ScopeManager::Level::isNotTerminated() const -> bool {
		return !this->isTerminated();
	}



	auto ScopeManager::Level::addFunc(std::string_view ident, ASG::Func::ID id) -> void {
		decltype(this->ids)::iterator ident_find = this->ids.find(ident);
		if(ident_find == this->ids.end()){ // create new list
			// TODO: fix this after MSVC bug is fixed
			std::pair<decltype(this->ids)::iterator, bool> new_ident_id = this->ids.emplace(ident, IdentID());
			new_ident_id.first->second.emplace<evo::SmallVector<ASG::FuncID>>({id});

		}else{ // list already exists
			evo::debugAssert(ident_find->second.is<evo::SmallVector<ASG::FuncID>>(), "ident was not a func");
			ident_find->second.as<evo::SmallVector<ASG::FuncID>>().emplace_back(id);
		}
	}

	auto ScopeManager::Level::addTemplatedFunc(std::string_view ident, ASG::TemplatedFunc::ID id) -> void {
		evo::debugAssert(this->lookupIdent(ident) == nullptr, "Scope already has ident \"{}\"", ident);

		this->ids.emplace(ident, id);
	}

	auto ScopeManager::Level::addVar(std::string_view ident, ASG::Var::ID id) -> void {
		evo::debugAssert(this->lookupIdent(ident) == nullptr, "Scope already has ident \"{}\"", ident);

		this->ids.emplace(ident, id);
	}

	auto ScopeManager::Level::addParam(std::string_view ident, ASG::Param::ID id) -> void {
		evo::debugAssert(this->lookupIdent(ident) == nullptr, "Scope already has ident \"{}\"", ident);

		this->ids.emplace(ident, id);
	}

	auto ScopeManager::Level::addReturnParam(std::string_view ident, ASG::ReturnParam::ID id) -> void {
		evo::debugAssert(this->lookupIdent(ident) == nullptr, "Scope already has ident \"{}\"", ident);

		this->ids.emplace(ident, id);
	}

	auto ScopeManager::Level::addImport(std::string_view ident, Source::ID id, Token::ID location) -> void {
		evo::debugAssert(this->lookupIdent(ident) == nullptr, "Scope already has ident \"{}\"", ident);

		this->ids.emplace(ident, ImportInfo(id, location));
	}


	auto ScopeManager::Level::lookupIdent(std::string_view ident) const -> const IdentID* {
		const decltype(this->ids)::const_iterator ident_find = this->ids.find(ident);
		if(ident_find == this->ids.end()){ return nullptr; }

		return &ident_find->second;
	}


	//////////////////////////////////////////////////////////////////////
	// scope

	auto ScopeManager::Scope::pushLevel(Level::ID id) -> void {
		this->scope_levels.emplace_back(id);
	}

	auto ScopeManager::Scope::pushLevel(Level::ID id, ASG::Func::ID func_id) -> void {
		this->scope_levels.emplace_back(id);
		this->object_scopes.emplace_back(ObjectScope(func_id), uint32_t(this->scope_levels.size()));
	}

	auto ScopeManager::Scope::popLevel() -> void {
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