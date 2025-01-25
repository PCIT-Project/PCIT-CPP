////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../../include/sema/ScopeLevel.h"


namespace pcit::panther::sema{


	auto ScopeLevel::hasStmtBlock() const -> bool {
		return this->_stmt_block != nullptr;
	}

	auto ScopeLevel::stmtBlock() const -> const sema::StmtBlock& {
		evo::debugAssert(this->_stmt_block != nullptr, "this scope doesn't have a stmt block");

		return *this->_stmt_block;
	}

	auto ScopeLevel::stmtBlock() -> sema::StmtBlock& {
		evo::debugAssert(this->_stmt_block != nullptr, "this scope doesn't have a stmt block");

		return *this->_stmt_block;
	}



	auto ScopeLevel::addSubScope() -> void {
		const auto lock = std::scoped_lock(this->sub_scopes_and_stmt_block_lock);

		this->has_sub_scopes = true;
		this->num_sub_scopes_not_terminated += 1;
	}

	auto ScopeLevel::setSubScopeTerminated() -> void {
		evo::debugAssert(this->num_sub_scopes_not_terminated != 0, "setSubScopeTerminated called too many times");

		const auto lock = std::scoped_lock(this->sub_scopes_and_stmt_block_lock);

		this->num_sub_scopes_not_terminated -= 1;
	}

	auto ScopeLevel::setTerminated() -> void {
		const auto lock = std::scoped_lock(this->sub_scopes_and_stmt_block_lock);
		this->_stmt_block->setTerminated();
	}

	auto ScopeLevel::isTerminated() const -> bool {
		const auto lock = std::scoped_lock(this->sub_scopes_and_stmt_block_lock);
		return this->_stmt_block->isTerminated() || (this->has_sub_scopes && this->num_sub_scopes_not_terminated == 0);
	}

	auto ScopeLevel::isNotTerminated() const -> bool {
		const auto lock = std::scoped_lock(this->sub_scopes_and_stmt_block_lock);
		return !this->isTerminated();
	}




	auto ScopeLevel::addIdent(std::string_view ident, Token::ID location) -> bool {
		const auto lock = std::scoped_lock(this->ids_lock);
		
		if(this->ids.contains(ident)){ return false; }

		this->ids.emplace(ident, IDNotReady(location));
		return true;
	}


	auto ScopeLevel::setFunc(std::string_view ident, sema::FuncID id) -> void {
		const auto lock = std::scoped_lock(this->ids_lock);

		IdentID& ident_id = this->ids.at(ident);

		if(ident_id.is<IDNotReady>()){
			ident_id.emplace<evo::SmallVector<sema::FuncID>>({id});
			
		}else{
			evo::debugAssert(
				ident_id.is<evo::SmallVector<sema::FuncID>>(), "ident \"{}\" was previously set (not a func)", ident
			);

			ident_id.as<evo::SmallVector<sema::FuncID>>().emplace_back(id);
		}
	}

	auto ScopeLevel::setTemplatedFunc(std::string_view ident, sema::TemplatedFuncID id) -> void {
		const auto lock = std::scoped_lock(this->ids_lock);

		evo::debugAssert(this->lookup_ident_without_locking(ident) != nullptr, "Scope does't ident \"{}\"", ident);
		evo::debugAssert(this->ids.at(ident).is<IDNotReady>(), "ident \"{}\" was already set", ident);

		this->ids.at(ident).emplace<sema::TemplatedFuncID>(id);
	}

	auto ScopeLevel::setVar(std::string_view ident, sema::VarID id) -> void {
		const auto lock = std::scoped_lock(this->ids_lock);

		evo::debugAssert(this->lookup_ident_without_locking(ident) != nullptr, "Scope does't ident \"{}\"", ident);
		evo::debugAssert(this->ids.at(ident).is<IDNotReady>(), "ident \"{}\" was already set", ident);

		this->ids.at(ident).emplace<sema::VarID>(id);
	}

	auto ScopeLevel::setParam(std::string_view ident, sema::ParamID id) -> void {
		const auto lock = std::scoped_lock(this->ids_lock);

		evo::debugAssert(this->lookup_ident_without_locking(ident) != nullptr, "Scope does't ident \"{}\"", ident);
		evo::debugAssert(this->ids.at(ident).is<IDNotReady>(), "ident \"{}\" was already set", ident);

		this->ids.at(ident).emplace<sema::ParamID>(id);
	}

	auto ScopeLevel::setReturnParam(std::string_view ident, sema::ReturnParamID id) -> void {
		const auto lock = std::scoped_lock(this->ids_lock);

		evo::debugAssert(this->lookup_ident_without_locking(ident) != nullptr, "Scope does't ident \"{}\"", ident);
		evo::debugAssert(this->ids.at(ident).is<IDNotReady>(), "ident \"{}\" was already set", ident);

		this->ids.at(ident).emplace<sema::ReturnParamID>(id);
	}

	auto ScopeLevel::setModule(std::string_view ident, SourceID id, Token::ID location) -> void {
		const auto lock = std::scoped_lock(this->ids_lock);

		evo::debugAssert(this->lookup_ident_without_locking(ident) != nullptr, "Scope does't ident \"{}\"", ident);
		evo::debugAssert(this->ids.at(ident).is<IDNotReady>(), "ident \"{}\" was already set", ident);

		this->ids.at(ident).emplace<ModuleInfo>(ModuleInfo(id, location));
	}


	auto ScopeLevel::setAlias(std::string_view ident, BaseType::Alias::ID id) -> void {
		const auto lock = std::scoped_lock(this->ids_lock);

		evo::debugAssert(this->lookup_ident_without_locking(ident) != nullptr, "Scope does't ident \"{}\"", ident);
		evo::debugAssert(this->ids.at(ident).is<IDNotReady>(), "ident \"{}\" was already set", ident);

		this->ids.at(ident).emplace<BaseType::Alias::ID>(id);
	}

	auto ScopeLevel::setTypedef(std::string_view ident, BaseType::Typedef::ID id) -> void {
		const auto lock = std::scoped_lock(this->ids_lock);

		evo::debugAssert(this->lookup_ident_without_locking(ident) != nullptr, "Scope does't ident \"{}\"", ident);
		evo::debugAssert(this->ids.at(ident).is<IDNotReady>(), "ident \"{}\" was already set", ident);

		this->ids.at(ident).emplace<BaseType::Typedef::ID>(id);
	}


	auto ScopeLevel::lookupIdent(std::string_view ident) const -> const IdentID* {
		const auto lock = std::scoped_lock(this->ids_lock);

		return this->lookup_ident_without_locking(ident);
	}


	auto ScopeLevel::lookup_ident_without_locking(std::string_view ident) const -> const IdentID* {
		const std::unordered_map<std::string_view, IdentID>::const_iterator ident_find = this->ids.find(ident);
		if(ident_find == this->ids.end()){ return nullptr; }

		return &ident_find->second;
	}


}