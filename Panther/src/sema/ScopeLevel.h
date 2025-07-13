////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <Evo.h>

#include "../source/source_data.h"
#include "../../include/type_ids.h"
#include "../../include/sema/Stmt.h"
#include "../../include/sema/Expr.h"

namespace pcit::panther{
	class Context;
}

namespace pcit::panther::sema{

	
	class ScopeLevel{
		public:
			struct ID : public core::UniqueID<uint32_t, struct ID> {
				using core::UniqueID<uint32_t, ID>::UniqueID;
			};

			struct ModuleInfo{
				SourceID sourceID;
				Token::ID tokenID;
				bool isPub;
			};

			struct TemplateTypeParam{
				TypeInfoVoidableID typeID;
				Token::ID location;
			};
			struct TemplateTypeParamFlag{}; // to differentiate which overload of addIdent

			struct TemplateExprParam{
				TypeInfoID typeID;
				sema::Expr value;
				Token::ID location;	
			};

			struct DeducedType{
				TypeInfoVoidableID typeID;
				Token::ID location;
			};
			struct DeducedTypeFlag{}; // to differentiate which overload of addIdent

			struct MemberVar{
				Token::ID location;
			};
			struct MemberVarFlag{};

			using FuncOverloadList = evo::SmallVector<evo::Variant<sema::FuncID, sema::TemplatedFuncID>>;

			using IdentID = evo::Variant<
				FuncOverloadList,
				sema::VarID,
				sema::GlobalVarID,
				sema::ParamID,
				sema::ReturnParamID,
				sema::ErrorReturnParamID,
				sema::BlockExprOutputID,
				sema::ExceptParamID,
				ModuleInfo,
				BaseType::AliasID,
				BaseType::TypedefID,
				BaseType::StructID,
				BaseType::InterfaceID,
				sema::TemplatedStructID,
				TemplateTypeParam,
				TemplateExprParam,
				DeducedType,
				MemberVar
			>;

			using NoLabelNode = std::monostate;
			using LabelNode = evo::Variant<NoLabelNode, sema::BlockExprID, sema::WhileID>;

		public:
			ScopeLevel(sema::StmtBlock* stmt_block = nullptr)
				: _stmt_block(stmt_block), _label(std::nullopt), _label_node(NoLabelNode()) {}

			ScopeLevel(sema::StmtBlock& stmt_block)
				: _stmt_block(&stmt_block), _label(std::nullopt), _label_node(NoLabelNode()) {}

			ScopeLevel(sema::StmtBlock& stmt_block, Token::ID label, LabelNode label_node)
				: _stmt_block(&stmt_block), _label(label), _label_node(label_node) {}


			~ScopeLevel() = default;

			EVO_NODISCARD auto hasStmtBlock() const -> bool;
			EVO_NODISCARD auto stmtBlock() const -> const sema::StmtBlock&;
			EVO_NODISCARD auto stmtBlock()       ->       sema::StmtBlock&;

			EVO_NODISCARD auto hasLabel() const -> bool { return this->_label.has_value(); };
			EVO_NODISCARD auto getLabel() const -> Token::ID {
				evo::debugAssert(this->hasLabel(), "Doesn't have label");
				return *this->_label;
			}
			EVO_NODISCARD auto getLabelNode() const -> const LabelNode& {
				evo::debugAssert(this->hasLabel(), "Doesn't have label node");
				return this->_label_node;
			}

			auto setIsDeferMainScope() -> void { this->is_defer_main_scope = true; }
			EVO_NODISCARD auto isDeferMainScope() const -> bool { return this->is_defer_main_scope; }

			auto setIsLoopMainScope() -> void { this->is_loop_main_scope = true; }
			EVO_NODISCARD auto isLoopMainScope() const -> bool { return this->is_loop_main_scope; }

			auto setDontDoShadowingChecks() -> void { this->do_shadowing_checks = false; }
			EVO_NODISCARD auto doesShadowingChecks() const -> bool { return this->do_shadowing_checks; }


			auto addSubScope() -> void;
			auto setSubScopeTerminated() -> void;
			auto setTerminated() -> void;
			auto setLabelTerminated() -> void;
			EVO_NODISCARD auto isTerminated() const -> bool;
			EVO_NODISCARD auto isLabelTerminated() const -> bool;

			auto resetSubScopes() -> void;



			using IsShadowRedef = bool;
			using AddIdentResult = evo::Expected<const IdentID*, IsShadowRedef>;

			EVO_NODISCARD auto addIdent(std::string_view ident, sema::FuncID id, const class panther::Context& context)
				-> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, sema::TemplatedFuncID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, sema::VarID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, sema::GlobalVarID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, sema::ParamID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, sema::ReturnParamID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, sema::ErrorReturnParamID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, sema::BlockExprOutputID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, sema::ExceptParamID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, SourceID id, Token::ID location, bool is_pub)
				-> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, BaseType::AliasID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, BaseType::TypedefID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, BaseType::StructID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, BaseType::InterfaceID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, sema::TemplatedStructID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(
				std::string_view ident, TypeInfoVoidableID typeID, Token::ID location, TemplateTypeParamFlag
			) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, TypeInfoID typeID, sema::Expr value, Token::ID location)
				-> AddIdentResult;
			EVO_NODISCARD auto addIdent(
				std::string_view ident, TypeInfoVoidableID typeID, Token::ID location, DeducedTypeFlag
			) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, Token::ID location, MemberVarFlag) -> AddIdentResult;



			// returns false if is a redefinition
			EVO_NODISCARD auto disallowIdentForShadowing(std::string_view ident, const IdentID* id) -> bool;

			// returns nullptr if doesnt exist
			EVO_NODISCARD auto lookupIdent(std::string_view ident) const -> const IdentID*;
			EVO_NODISCARD auto lookupDisallowedIdentForShadowing(std::string_view ident) const -> const IdentID*;



		private:
			EVO_NODISCARD auto add_ident_default_impl(std::string_view ident, auto id) -> AddIdentResult;
	
		private:
			std::unordered_map<std::string_view, IdentID> ids{};
			std::unordered_map<std::string_view, const IdentID*> disallowed_idents_for_shadowing{};
			mutable core::SpinLock idents_lock{};

			sema::StmtBlock* _stmt_block;
			std::optional<Token::ID> _label;
			LabelNode _label_node;
			bool is_defer_main_scope = false;
			bool is_loop_main_scope = false;
			bool do_shadowing_checks = true; // only for this level, doesn't affect sub-scopes or super-scopes

			unsigned num_sub_scopes_not_terminated = 0;
			bool has_sub_scopes = false;
			mutable core::SpinLock sub_scopes_and_stmt_block_lock{};

	};


}
