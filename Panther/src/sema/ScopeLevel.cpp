////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./ScopeLevel.h"


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






	auto ScopeLevel::addIdent(std::string_view ident, sema::FuncID id) -> bool {
		const auto lock = std::scoped_lock(this->ids_lock);

		const std::unordered_map<std::string_view, IdentID>::iterator ident_find = this->ids.find(ident);
		if(ident_find == this->ids.end()){
			this->ids.emplace(ident, IdentID()).first->second.as<FuncOverloadList>().emplace_back();
			return true;
		}else{
			if(ident_find->second.is<FuncOverloadList>()){
				ident_find->second.as<FuncOverloadList>().emplace_back(
					evo::Variant<sema::FuncID, sema::TemplatedFuncID>(id)
				);
				return true;
			}else{
				return false;
			}
		}
	}

	auto ScopeLevel::addIdent(std::string_view ident, sema::TemplatedFuncID id) -> bool {
		const auto lock = std::scoped_lock(this->ids_lock);

		const std::unordered_map<std::string_view, IdentID>::iterator ident_find = this->ids.find(ident);
		if(ident_find == this->ids.end()){
			this->ids.emplace(ident, IdentID()).first->second.as<FuncOverloadList>().emplace_back();
			return true;
		}else{
			if(ident_find->second.is<FuncOverloadList>()){
				ident_find->second.as<FuncOverloadList>().emplace_back(
					evo::Variant<sema::FuncID, sema::TemplatedFuncID>(id)
				);
				return true;
			}else{
				return false;
			}
		}
	}

	auto ScopeLevel::addIdent(std::string_view ident, sema::VarID id) -> bool {
		const auto lock = std::scoped_lock(this->ids_lock);

		if(this->ids.contains(ident)){ return false; }

		this->ids.emplace(ident, id);
		return true;
	}

	auto ScopeLevel::addIdent(std::string_view ident, sema::StructID id) -> bool {
		const auto lock = std::scoped_lock(this->ids_lock);

		if(this->ids.contains(ident)){ return false; }
		
		this->ids.emplace(ident, id);
		return true;
	}

	auto ScopeLevel::addIdent(std::string_view ident, sema::ParamID id) -> bool {
		const auto lock = std::scoped_lock(this->ids_lock);

		if(this->ids.contains(ident)){ return false; }
		
		this->ids.emplace(ident, id);
		return true;
	}

	auto ScopeLevel::addIdent(std::string_view ident, sema::ReturnParamID id) -> bool {
		const auto lock = std::scoped_lock(this->ids_lock);

		if(this->ids.contains(ident)){ return false; }
		
		this->ids.emplace(ident, id);
		return true;
	}

	auto ScopeLevel::addIdent(std::string_view ident, SourceID id, Token::ID location, bool is_pub) -> bool {
		const auto lock = std::scoped_lock(this->ids_lock);

		if(this->ids.contains(ident)){ return false; }
		
		this->ids.emplace(ident, ModuleInfo(id, location, is_pub));
		return true;
	}


	auto ScopeLevel::addIdent(std::string_view ident, BaseType::Alias::ID id) -> bool {
		const auto lock = std::scoped_lock(this->ids_lock);

		if(this->ids.contains(ident)){ return false; }
		
		this->ids.emplace(ident, id);
		return true;
	}

	auto ScopeLevel::addIdent(std::string_view ident, BaseType::Typedef::ID id) -> bool {
		const auto lock = std::scoped_lock(this->ids_lock);

		if(this->ids.contains(ident)){ return false; }
		
		this->ids.emplace(ident, id);
		return true;
	}


	auto ScopeLevel::lookupIdent(std::string_view ident) const -> const IdentID* {
		const auto lock = std::scoped_lock(this->ids_lock);

		const std::unordered_map<std::string_view, IdentID>::const_iterator ident_find = this->ids.find(ident);
		if(ident_find == this->ids.end()){ return nullptr; }

		return &ident_find->second;
	}

}