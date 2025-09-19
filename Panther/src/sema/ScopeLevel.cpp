////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./ScopeLevel.h"

#include "../../include/Context.h"


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

		this->num_sub_scopes += 1;
	}

	auto ScopeLevel::setSubScopeTerminated() -> void {
		const auto lock = std::scoped_lock(this->sub_scopes_and_stmt_block_lock);

		evo::debugAssert(
			this->num_sub_scopes != this->num_sub_scopes_terminated, "setSubScopeTerminated called too many times"
		);

		this->num_sub_scopes_terminated += 1;
	}

	auto ScopeLevel::setTerminated() -> void {
		const auto lock = std::scoped_lock(this->sub_scopes_and_stmt_block_lock);
		this->_stmt_block->setTerminated();
	}

	auto ScopeLevel::setLabelTerminated() -> void {
		const auto lock = std::scoped_lock(this->sub_scopes_and_stmt_block_lock);
		this->_stmt_block->setLabelTerminated();
	}

	auto ScopeLevel::isTerminated() const -> bool {
		const auto lock = std::scoped_lock(this->sub_scopes_and_stmt_block_lock);
		
		return (this->hasStmtBlock() && this->stmtBlock().isTerminated())
			|| (this->num_sub_scopes > 0 && this->num_sub_scopes == this->num_sub_scopes_terminated);
	}


	auto ScopeLevel::isLabelTerminated() const -> bool {
		const auto lock = std::scoped_lock(this->sub_scopes_and_stmt_block_lock);
		return this->_stmt_block->isLabelTerminated();
	}

	auto ScopeLevel::numUnterminatedSubScopes() const -> unsigned {
		const auto lock = std::scoped_lock(this->sub_scopes_and_stmt_block_lock);
		return this->num_sub_scopes - this->num_sub_scopes_terminated;
	}


	auto ScopeLevel::resetSubScopes() -> void {
		this->num_sub_scopes = 0;
		this->num_sub_scopes_terminated = 0;
	}






	auto ScopeLevel::addIdent(std::string_view ident, sema::FuncID id, const Context& context) -> AddIdentResult {
		const auto lock = std::scoped_lock(this->idents_lock);

		if(this->disallowed_idents_for_shadowing.contains(ident)){ return evo::Unexpected(true); }

		const std::unordered_map<std::string_view, IdentID>::iterator ident_find = this->ids.find(ident);
		if(ident_find == this->ids.end()){
			IdentID& new_ident_id = this->ids.emplace(ident, IdentID()).first->second;
			new_ident_id.as<FuncOverloadList>().emplace_back(evo::Variant<sema::FuncID, sema::TemplatedFuncID>(id));
			return &new_ident_id;

		}else{
			if(ident_find->second.is<FuncOverloadList>()){
				FuncOverloadList& overload_list = ident_find->second.as<FuncOverloadList>();

				const sema::Func& new_sema = context.getSemaBuffer().getFunc(id);
				
				for(const evo::Variant<sema::FuncID, sema::TemplatedFuncID>& overload : overload_list){
					if(overload.is<sema::TemplatedFuncID>()){ continue; };

					const sema::Func& overload_sema = context.getSemaBuffer().getFunc(overload.as<sema::FuncID>());
					if(overload_sema.instanceID != std::numeric_limits<uint32_t>::max()){ continue; }

					if(new_sema.isEquivalentOverload(overload_sema, context)){ return evo::Unexpected(false); }
				}

				overload_list.emplace_back(evo::Variant<sema::FuncID, sema::TemplatedFuncID>(id));
				return &ident_find->second;
			}else{
				return evo::Unexpected(false);
			}
		}
	}

	auto ScopeLevel::addIdent(std::string_view ident, sema::TemplatedFuncID id) -> AddIdentResult {
		const auto lock = std::scoped_lock(this->idents_lock);

		if(this->disallowed_idents_for_shadowing.contains(ident)){ return evo::Unexpected(true); }

		const std::unordered_map<std::string_view, IdentID>::iterator ident_find = this->ids.find(ident);
		if(ident_find == this->ids.end()){
			IdentID& new_ident_id = this->ids.emplace(ident, IdentID()).first->second;
			new_ident_id.as<FuncOverloadList>().emplace_back(evo::Variant<sema::FuncID, sema::TemplatedFuncID>(id));
			return &new_ident_id;

		}else{
			if(ident_find->second.is<FuncOverloadList>()){
				ident_find->second.as<FuncOverloadList>().emplace_back(
					evo::Variant<sema::FuncID, sema::TemplatedFuncID>(id)
				);
				return &ident_find->second;
			}else{
				return evo::Unexpected(false);
			}
		}
	}


	auto ScopeLevel::addIdent(std::string_view ident, sema::VarID id) -> AddIdentResult {
		return this->add_ident_default_impl(ident, id);
	}

	auto ScopeLevel::addIdent(std::string_view ident, sema::GlobalVarID id) -> AddIdentResult {
		return this->add_ident_default_impl(ident, id);
	}

	auto ScopeLevel::addIdent(std::string_view ident, sema::ParamID id) -> AddIdentResult {
		return this->add_ident_default_impl(ident, id);
	}

	auto ScopeLevel::addIdent(std::string_view ident, sema::ReturnParamID id) -> AddIdentResult {
		return this->add_ident_default_impl(ident, id);
	}

	auto ScopeLevel::addIdent(std::string_view ident, sema::ErrorReturnParamID id) -> AddIdentResult {
		return this->add_ident_default_impl(ident, id);
	}

	auto ScopeLevel::addIdent(std::string_view ident, sema::BlockExprOutputID id) -> AddIdentResult {
		return this->add_ident_default_impl(ident, id);
	}

	auto ScopeLevel::addIdent(std::string_view ident, sema::ExceptParamID id) -> AddIdentResult {
		return this->add_ident_default_impl(ident, id);
	}

	auto ScopeLevel::addIdent(std::string_view ident, SourceID id, Token::ID location, bool is_pub) -> AddIdentResult {
		const auto lock = std::scoped_lock(this->idents_lock);

		if(this->ids.contains(ident)){ return evo::Unexpected(false); }
		if(this->disallowed_idents_for_shadowing.contains(ident)){ return evo::Unexpected(true); }
		
		return &this->ids.emplace(ident, ModuleInfo(id, location, is_pub)).first->second;
	}

	auto ScopeLevel::addIdent(std::string_view ident, ClangSourceID id, Token::ID location, bool is_pub)
	-> AddIdentResult {
		const auto lock = std::scoped_lock(this->idents_lock);

		if(this->ids.contains(ident)){ return evo::Unexpected(false); }
		if(this->disallowed_idents_for_shadowing.contains(ident)){ return evo::Unexpected(true); }
		
		return &this->ids.emplace(ident, ClangModuleInfo(id, location, is_pub)).first->second;
	}


	auto ScopeLevel::addIdent(std::string_view ident, BaseType::AliasID id) -> AddIdentResult {
		return this->add_ident_default_impl(ident, id);
	}

	auto ScopeLevel::addIdent(std::string_view ident, BaseType::DistinctAliasID id) -> AddIdentResult {
		return this->add_ident_default_impl(ident, id);
	}

	auto ScopeLevel::addIdent(std::string_view ident, BaseType::StructID id) -> AddIdentResult {
		return this->add_ident_default_impl(ident, id);
	}

	auto ScopeLevel::addIdent(std::string_view ident, BaseType::UnionID id) -> AddIdentResult {
		return this->add_ident_default_impl(ident, id);
	}

	auto ScopeLevel::addIdent(std::string_view ident, BaseType::InterfaceID id) -> AddIdentResult {
		return this->add_ident_default_impl(ident, id);
	}


	auto ScopeLevel::addIdent(std::string_view ident, sema::TemplatedStructID id) -> AddIdentResult {
		return this->add_ident_default_impl(ident, id);
	}

	auto ScopeLevel::addIdent(
		std::string_view ident, TypeInfoVoidableID typeID, Token::ID location, TemplateTypeParamFlag
	) -> AddIdentResult {
		const auto lock = std::scoped_lock(this->idents_lock);

		if(this->ids.contains(ident)){ return evo::Unexpected(false); }
		if(this->do_shadowing_checks && this->disallowed_idents_for_shadowing.contains(ident)){
			return evo::Unexpected(true);
		}

		return &this->ids.emplace(ident, TemplateTypeParam(typeID, location)).first->second;
	}

	auto ScopeLevel::addIdent(std::string_view ident, TypeInfoID typeID, sema::Expr value, Token::ID location)
	-> AddIdentResult {
		const auto lock = std::scoped_lock(this->idents_lock);

		if(this->ids.contains(ident)){ return evo::Unexpected(false); }
		if(this->do_shadowing_checks && this->disallowed_idents_for_shadowing.contains(ident)){
			return evo::Unexpected(true);
		}
		
		return &this->ids.emplace(ident, TemplateExprParam(typeID, value, location)).first->second;
	}

	auto ScopeLevel::addIdent(
		std::string_view ident, TypeInfoVoidableID typeID, Token::ID location, DeducedTypeFlag
	) -> AddIdentResult {
		const auto lock = std::scoped_lock(this->idents_lock);

		if(this->ids.contains(ident)){ return evo::Unexpected(false); }
		if(this->do_shadowing_checks && this->disallowed_idents_for_shadowing.contains(ident)){
			return evo::Unexpected(true);
		}

		return &this->ids.emplace(ident, DeducedType(typeID, location)).first->second;
	}

	auto ScopeLevel::addIdent(std::string_view ident, Token::ID location, MemberVarFlag) -> AddIdentResult {
		const auto lock = std::scoped_lock(this->idents_lock);

		if(this->ids.contains(ident)){ return evo::Unexpected(false); }
		if(this->do_shadowing_checks && this->disallowed_idents_for_shadowing.contains(ident)){
			return evo::Unexpected(true);
		}

		return &this->ids.emplace(ident, MemberVar(location)).first->second;
	}

	auto ScopeLevel::addIdent(std::string_view ident, Token::ID location, uint32_t field_index, UnionFieldFlag)
	-> AddIdentResult {
		const auto lock = std::scoped_lock(this->idents_lock);

		if(this->ids.contains(ident)){ return evo::Unexpected(false); }
		if(this->do_shadowing_checks && this->disallowed_idents_for_shadowing.contains(ident)){
			return evo::Unexpected(true);
		}

		return &this->ids.emplace(ident, UnionField(location, field_index)).first->second;
	}




	auto ScopeLevel::disallowIdentForShadowing(std::string_view ident, const IdentID* id) -> bool {
		evo::debugAssert(id != nullptr, "`id` cannot be nullptr");

		if(this->do_shadowing_checks == false){ return true; }

		const auto lock = std::scoped_lock(this->idents_lock);

		const auto find = this->ids.find(ident);
		if(find != this->ids.end()){
			return find->second.is<MemberVar>(); // done this way as you can't shadow a member var
		}

		this->disallowed_idents_for_shadowing.emplace(ident, id);
		return true;
	}



	auto ScopeLevel::lookupIdent(std::string_view ident) const -> const IdentID* {
		const auto lock = std::scoped_lock(this->idents_lock);

		const std::unordered_map<std::string_view, IdentID>::const_iterator ident_find = this->ids.find(ident);
		if(ident_find == this->ids.end()){ return nullptr; }

		return &ident_find->second;
	}


	auto ScopeLevel::lookupDisallowedIdentForShadowing(std::string_view ident) const -> const IdentID* {
		evo::debugAssert(this->do_shadowing_checks, "This scope level doesn't do shadowing checks");

		const auto lock = std::scoped_lock(this->idents_lock);

		const std::unordered_map<std::string_view, const IdentID*>::const_iterator ident_find =
			this->disallowed_idents_for_shadowing.find(ident);
		if(ident_find == this->disallowed_idents_for_shadowing.end()){ return nullptr; }

		return ident_find->second;
	}





	auto ScopeLevel::addIdentValueState(ValueStateID value_state_id, ValueState state) -> void {
		const auto lock = std::scoped_lock(this->value_states_lock);
		this->value_states.emplace(value_state_id, ValueStateInfo(state, ValueStateInfo::DeclInfo()));
	}

	auto ScopeLevel::setIdentValueState(ValueStateID value_state_id, ValueState state) -> void {
		const auto lock = std::scoped_lock(this->value_states_lock);

		auto find = this->value_states.find(value_state_id);

		if(find == this->value_states.end()){
			this->value_states.emplace(value_state_id, ValueStateInfo(state, ValueStateInfo::ModifyInfo()));

		}else{
			find->second.state = state;
		}
	}

	auto ScopeLevel::setIdentValueStateFromSubScope(ValueStateID value_state_id, ValueState state)
	-> evo::Expected<void, ValueStateID> {
		const auto lock = std::scoped_lock(this->value_states_lock);

		auto find = this->value_states.find(value_state_id);

		if(find == this->value_states.end()){
			this->value_states.emplace(value_state_id, ValueStateInfo(state, ValueStateInfo::ModifyInfo()));
			return evo::Expected<void, ValueStateID>();

		}else{
			if(find->second.info.is<ValueStateInfo::DeclInfo>()){
				ValueStateInfo::DeclInfo& decl_info = find->second.info.as<ValueStateInfo::DeclInfo>();

				if(decl_info.potential_state_change.has_value()){
					if(state != *decl_info.potential_state_change){
						return evo::Unexpected<ValueStateID>(value_state_id);
					}

					decl_info.num_sub_scopes += 1;
					return evo::Expected<void, ValueStateID>();

				}else{
					decl_info.potential_state_change = state;
					decl_info.num_sub_scopes += 1;
					return evo::Expected<void, ValueStateID>();
				}

			}else{
				evo::debugAssert(find->second.info.is<ValueStateInfo::ModifyInfo>(), "Unknown info kind");

				if(state != find->second.state){
					return evo::Unexpected<ValueStateID>(value_state_id);
				}else{
					find->second.info.as<ValueStateInfo::ModifyInfo>().num_sub_scopes += 1;
					return evo::Expected<void, ValueStateID>();
				}
			}
		}
	}

	auto ScopeLevel::getIdentValueState(ValueStateID value_state_id) const -> std::optional<ValueState> {
		const auto lock = std::scoped_lock(this->value_states_lock);

		const auto find = this->value_states.find(value_state_id);
		if(find != this->value_states.end()){ return find->second.state; }
		return std::nullopt;
	}


	auto ScopeLevel::getValueStateInfos() const
	-> evo::IterRange<std::unordered_map<ValueStateID, ValueStateInfo>::const_iterator> {
		return evo::IterRange<std::unordered_map<ValueStateID, ValueStateInfo>::const_iterator>(
			this->value_states.cbegin(), this->value_states.cend()
		);
	}

	auto ScopeLevel::getValueStateInfos()
	-> evo::IterRange<std::unordered_map<ValueStateID, ValueStateInfo>::iterator> {
		return evo::IterRange<std::unordered_map<ValueStateID, ValueStateInfo>::iterator>(
			this->value_states.begin(), this->value_states.end()
		);
	}



	auto ScopeLevel::add_ident_default_impl(std::string_view ident, auto id) -> AddIdentResult {
		const auto lock = std::scoped_lock(this->idents_lock);

		if(this->ids.contains(ident)){ return evo::Unexpected(false); }
		if(this->do_shadowing_checks && this->disallowed_idents_for_shadowing.contains(ident)){
			return evo::Unexpected(true);
		}
		
		return &this->ids.emplace(ident, id).first->second;
	}


}