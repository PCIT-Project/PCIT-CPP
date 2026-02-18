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

	struct ReturnParamAccessorValueStateID{
		ReturnParamID id;
		uint32_t index;

		EVO_NODISCARD auto operator==(const ReturnParamAccessorValueStateID&) const -> bool = default;
	};

	struct OpDeleteThisAccessorValueStateID{ // this.[MEMBER] in overload operator [delete]
		uint32_t abiIndex;

		EVO_NODISCARD auto operator==(const OpDeleteThisAccessorValueStateID&) const -> bool = default;
	};

	struct UninitPtrLocalVar{
		VarID varID;

		EVO_NODISCARD auto operator==(const UninitPtrLocalVar&) const -> bool = default;
	};

}


namespace std{

	template<>
	struct hash<pcit::panther::sema::ReturnParamAccessorValueStateID>{
		auto operator()(pcit::panther::sema::ReturnParamAccessorValueStateID id) const noexcept -> size_t {
			return evo::hashCombine(
				std::hash<pcit::panther::sema::ReturnParamID>{}(id.id), std::hash<uint32_t>{}(id.index)
			);
		};
	};

	template<>
	struct hash<pcit::panther::sema::OpDeleteThisAccessorValueStateID>{
		auto operator()(pcit::panther::sema::OpDeleteThisAccessorValueStateID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.abiIndex);
		};
	};

	template<>
	struct hash<pcit::panther::sema::UninitPtrLocalVar>{
		auto operator()(pcit::panther::sema::UninitPtrLocalVar local_var) const noexcept -> size_t {
			return std::hash<pcit::panther::sema::VarID>{}(local_var.varID);
		};
	};

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
				std::optional<EncapsulatingSymbolID> parent;
				bool isPub;
				bool isPriv;
			};

			struct ClangModuleInfo{
				ClangSourceID clangSourceID;
				Token::ID tokenID;
				std::optional<EncapsulatingSymbolID> parent;
				bool isPub;
				bool isPriv;
			};

			struct TemplateTypeParamFlag{};
			struct TemplateTypeParam{
				TypeInfoVoidableID typeID;
				Token::ID location;
			};

			struct TemplateExprParamFlag{};
			struct TemplateExprParam{
				TypeInfoID typeID;
				sema::Expr value;
				Token::ID location;	
			};

			struct DeducedTypeFlag{};
			struct DeducedType{
				TypeInfoVoidableID typeID;
				Token::ID location;
			};

			struct DeducedExprFlag{};
			struct DeducedExpr{
				TypeInfoID typeID;
				sema::Expr value;
				Token::ID location;
			};

			struct MemberVarFlag{};
			struct MemberVar{
				Token::ID location;
			};

			struct UnionFieldFlag{};
			struct UnionField{
				Token::ID location;
				uint32_t field_index;
			};

			using Overload = evo::Variant<sema::FuncID, sema::TemplatedFuncID>;
			using OverloadList = evo::SmallVector<Overload>;

			struct FuncOverloadList{
				OverloadList funcs;
			};

			struct MethodOverloadFlag{};
			struct MethodOverloadList{
				OverloadList funcs;
			};

			struct ExtractedVariadicParam{
				ParamID param_id;
				TypeInfoID type_id;
				bool is_mut;
			};


			struct ForUnrollIndexFlag{};
			struct ForUnrollIndex{
				TypeInfoID typeID;
				sema::Expr value;
				Token::ID location;
			};


			using IdentID = evo::Variant<
				FuncOverloadList,
				MethodOverloadList,
				sema::FuncAliasID,
				sema::VarID,
				sema::GlobalVarID,
				sema::ParamID,
				sema::VariadicParamID,
				ExtractedVariadicParam,
				sema::ReturnParamID,
				sema::ErrorReturnParamID,
				sema::BlockExprOutputID,
				sema::ExceptParamID,
				sema::ForParamID,
				ModuleInfo,
				ClangModuleInfo,
				BaseType::AliasID,
				BaseType::DistinctAliasID,
				BaseType::StructID,
				BaseType::UnionID,
				BaseType::EnumID,
				BaseType::InterfaceID,
				sema::TemplatedStructID,
				sema::StructTemplateAliasID,
				TemplateTypeParam,
				TemplateExprParam,
				DeducedType,
				DeducedExpr,
				MemberVar,
				UnionField,
				ForUnrollIndex
			>;

			using NoLabelNode = std::monostate;
			using LabelNode = evo::Variant<
				NoLabelNode, sema::BlockExprID, sema::WhileID, sema::ForID, sema::ForUnrollID
			>;

			using ValueStateID = evo::Variant<
				sema::VarID,
				sema::ParamID,
				sema::ReturnParamID,
				sema::ErrorReturnParamID,
				sema::BlockExprOutputID,
				sema::ExceptParamID,
				sema::ForParamID,
				ReturnParamAccessorValueStateID,
				OpDeleteThisAccessorValueStateID,
				UninitPtrLocalVar
			>;
			enum class ValueState{
				UNINIT,
				INIT,
				INITIALIZING,
				MOVED_FROM,
			};

			struct ValueStateInfo{
				struct DeclInfo{
					std::optional<ValueState> potential_state_change = std::nullopt;
					unsigned num_sub_scopes = 0;
				};

				struct ModifyInfo{
					unsigned num_sub_scopes;

					explicit ModifyInfo(unsigned _num_sub_scopes) : num_sub_scopes(_num_sub_scopes) {}
				};


				ValueState state;
				evo::Variant<DeclInfo, ModifyInfo> info;
				size_t creation_index;
			};

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

			auto setIsUnsafe() -> void { this->is_unsafe = true; }
			EVO_NODISCARD auto isUnsafe() const -> bool { return this->is_unsafe; }


			auto addSubScope() -> void;
			auto setSubScopeTerminated() -> void;
			auto setSubScopeLabelTerminated() -> void;
			auto setTerminated() -> void;
			auto setLabelTerminated() -> void;
			EVO_NODISCARD auto isTerminated() const -> bool;
			EVO_NODISCARD auto isLabelTerminated() const -> bool;
			EVO_NODISCARD auto allTerminatedSubScopesAreLabelTerminated() const -> bool;

			EVO_NODISCARD auto numUnterminatedSubScopes() const -> unsigned;

			auto resetSubScopes() -> void;



			using IsShadowRedef = bool;
			using AddIdentResult = evo::Expected<const IdentID*, IsShadowRedef>;

			EVO_NODISCARD auto addIdent(std::string_view ident, sema::FuncID id, const class panther::Context& context)
				-> AddIdentResult;
			EVO_NODISCARD auto addIdent(
				std::string_view ident, MethodOverloadFlag, sema::FuncID id, const class panther::Context& context
			) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, sema::TemplatedFuncID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, MethodOverloadFlag, sema::TemplatedFuncID id)
				-> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, sema::FuncAliasID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, sema::VarID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, sema::GlobalVarID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, sema::ParamID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, sema::VariadicParamID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, ExtractedVariadicParam id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, sema::ReturnParamID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, sema::ErrorReturnParamID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, sema::BlockExprOutputID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, sema::ExceptParamID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, sema::ForParamID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(
				std::string_view ident,
				SourceID id,
				Token::ID location,
				std::optional<EncapsulatingSymbolID> parent,
				bool is_pub,
				bool is_priv
			) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(
				std::string_view ident,
				ClangSourceID id,
				Token::ID location,
				std::optional<EncapsulatingSymbolID> parent,
				bool is_pub,
				bool is_priv
			) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, BaseType::AliasID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, BaseType::DistinctAliasID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, BaseType::StructID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, BaseType::UnionID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, BaseType::EnumID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, BaseType::InterfaceID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, sema::TemplatedStructID id) -> AddIdentResult;
			EVO_NODISCARD auto addIdent(std::string_view ident, sema::StructTemplateAliasID id) -> AddIdentResult;


			//////////////////
			// addIdent overloads that require selector flags

			EVO_NODISCARD auto addIdent(
				std::string_view ident, TemplateTypeParamFlag, TypeInfoVoidableID typeID, Token::ID location
			) -> AddIdentResult;

			EVO_NODISCARD auto addIdent(
				std::string_view ident, TemplateExprParamFlag, TypeInfoID typeID, sema::Expr value, Token::ID location
			) -> AddIdentResult;

			EVO_NODISCARD auto addIdent(
				std::string_view ident, DeducedTypeFlag, TypeInfoVoidableID typeID, Token::ID location
			) -> AddIdentResult;

			EVO_NODISCARD auto addIdent(
				std::string_view ident, DeducedExprFlag, TypeInfoID typeID, sema::Expr value, Token::ID location
			) -> AddIdentResult;

			EVO_NODISCARD auto addIdent(std::string_view ident, MemberVarFlag, Token::ID location) -> AddIdentResult;

			EVO_NODISCARD auto addIdent(
				std::string_view ident, UnionFieldFlag, Token::ID location, uint32_t field_index
			) -> AddIdentResult;

			EVO_NODISCARD auto addIdent(
				std::string_view ident, ForUnrollIndexFlag, TypeInfoID typeID, sema::Expr value, Token::ID location
			) -> AddIdentResult;



			// returns false if is a redefinition
			EVO_NODISCARD auto disallowIdentForShadowing(std::string_view ident, const IdentID* id) -> bool;

			// returns nullptr if doesnt exist
			EVO_NODISCARD auto lookupIdent(std::string_view ident) const -> const IdentID*;
			EVO_NODISCARD auto lookupDisallowedIdentForShadowing(std::string_view ident) const -> const IdentID*;


			EVO_NODISCARD auto getIdents() const 
				-> evo::IterRange<std::unordered_map<std::string_view, IdentID>::const_iterator>;


			auto addIdentValueState(ValueStateID value_state_id, ValueState state) -> void;
			auto setIdentValueState(ValueStateID value_state_id, ValueState state) -> void;
			EVO_NODISCARD auto setIdentValueStateFromSubScope(ValueStateID value_state_id, ValueState state)
				-> evo::Expected<void, ValueStateID>;
			EVO_NODISCARD auto getIdentValueState(ValueStateID value_state_id) const -> std::optional<ValueState>;

			EVO_NODISCARD auto getValueStateInfos() const
				-> evo::IterRange<std::unordered_map<ValueStateID, ValueStateInfo>::const_iterator>;

			EVO_NODISCARD auto getValueStateInfos()
				-> evo::IterRange<std::unordered_map<ValueStateID, ValueStateInfo>::iterator>;



		private:
			EVO_NODISCARD auto add_ident_default_impl(std::string_view ident, auto id) -> AddIdentResult;
	
		private:
			std::unordered_map<std::string_view, IdentID> ids{};
			std::unordered_map<std::string_view, const IdentID*> disallowed_idents_for_shadowing{};
			mutable evo::SpinLock idents_lock{};

			sema::StmtBlock* _stmt_block;
			std::optional<Token::ID> _label;
			LabelNode _label_node;
			bool is_defer_main_scope = false;
			bool is_loop_main_scope = false;
			bool do_shadowing_checks = true; // only for this level, doesn't affect sub-scopes or super-scopes
			bool is_unsafe = false;

		private:
			unsigned num_sub_scopes = false;
			unsigned num_sub_scopes_terminated = 0;
			unsigned num_sub_scopes_label_terminated = 0;
			mutable evo::SpinLock sub_scopes_and_stmt_block_lock{};


			std::unordered_map<ValueStateID, ValueStateInfo> value_states{};
			mutable evo::SpinLock value_states_lock{};

	};


}
