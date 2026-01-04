////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <unordered_set>

#include <Evo.h>
#include <PCIT_core.h>

#include "../../include/source/source_data.h"
#include "../../include/AST/AST.h"
#include "../sema/TermInfo.h"
#include "./symbol_proc_ids.h"
#include "../../include/TypeManager.h"
#include "../sema/ScopeManager.h"


namespace pcit::panther{

	struct SymbolProcTermInfoID : public core::UniqueID<uint32_t, struct SymbolProcTermInfoID> { 
		using core::UniqueID<uint32_t, SymbolProcTermInfoID>::UniqueID;
	};


	struct SymbolProcTypeID : public core::UniqueID<uint32_t, struct SymbolProcTypeID> { 
		using core::UniqueID<uint32_t, SymbolProcTypeID>::UniqueID;
	};


	struct SymbolProcStructInstantiationID : public core::UniqueID<uint32_t, struct SymbolProcStructInstantiationID> { 
		using core::UniqueID<uint32_t, SymbolProcStructInstantiationID>::UniqueID;
	};



	struct SymbolProcInstructionIndex : public core::UniqueID<uint32_t, struct SymbolProcInstructionIndex> { 
		using core::UniqueID<uint32_t, SymbolProcInstructionIndex>::UniqueID;
	};
}




namespace pcit::core{

	template<>
	struct OptionalInterface<panther::SymbolProcTermInfoID>{
		static constexpr auto init(panther::SymbolProcTermInfoID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::SymbolProcTermInfoID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};


	template<>
	struct OptionalInterface<panther::SymbolProcTypeID>{
		static constexpr auto init(panther::SymbolProcTypeID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::SymbolProcTypeID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};


	template<>
	struct OptionalInterface<panther::SymbolProcStructInstantiationID>{
		static constexpr auto init(panther::SymbolProcStructInstantiationID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::SymbolProcStructInstantiationID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};



	template<>
	struct OptionalInterface<panther::SymbolProcInstructionIndex>{
		static constexpr auto init(panther::SymbolProcInstructionIndex* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::SymbolProcInstructionIndex& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

}





namespace std{

	template<>
	class optional<pcit::panther::SymbolProcTermInfoID> 
		: public pcit::core::Optional<pcit::panther::SymbolProcTermInfoID>{
		public:
			using pcit::core::Optional<pcit::panther::SymbolProcTermInfoID>::Optional;
			using pcit::core::Optional<pcit::panther::SymbolProcTermInfoID>::operator=;
	};


	template<>
	class optional<pcit::panther::SymbolProcTypeID> 
		: public pcit::core::Optional<pcit::panther::SymbolProcTypeID>{
		public:
			using pcit::core::Optional<pcit::panther::SymbolProcTypeID>::Optional;
			using pcit::core::Optional<pcit::panther::SymbolProcTypeID>::operator=;
	};


	template<>
	class optional<pcit::panther::SymbolProcStructInstantiationID> 
		: public pcit::core::Optional<pcit::panther::SymbolProcStructInstantiationID>{
		public:
			using pcit::core::Optional<pcit::panther::SymbolProcStructInstantiationID>::Optional;
			using pcit::core::Optional<pcit::panther::SymbolProcStructInstantiationID>::operator=;
	};


	template<>
	class optional<pcit::panther::SymbolProcInstructionIndex> 
		: public pcit::core::Optional<pcit::panther::SymbolProcInstructionIndex>{
		public:
			using pcit::core::Optional<pcit::panther::SymbolProcInstructionIndex>::Optional;
			using pcit::core::Optional<pcit::panther::SymbolProcInstructionIndex>::operator=;
	};

}


namespace pcit::panther{


	// TODO(PERF): make this data oriented
	struct SymbolProcInstruction{
		using AttributeParams = evo::StaticVector<SymbolProcTermInfoID, 2>;

		struct TemplateParamInfo{
			const AST::TemplatePack::Param& param;
			std::optional<SymbolProcTypeID> type_id; // nullopt if is `Type`
			std::optional<SymbolProcTermInfoID> default_value;

			auto operator=(const TemplateParamInfo& rhs) -> TemplateParamInfo& {
				std::destroy_at(this); // just in case destruction becomes non-trivial
				std::construct_at(this, rhs);
				return *this;
			}
		};



		//////////////////
		// stmts valid in global scope

		struct NonLocalVarDecl{
			const AST::VarDef& var_def;
			evo::SmallVector<AttributeParams> attribute_params_info;
			SymbolProcTypeID type_id;
		};

		struct NonLocalVarDef{
			const AST::VarDef& var_def;
			std::optional<SymbolProcTermInfoID> value_id;
		};

		struct NonLocalVarDeclDef{
			const AST::VarDef& var_def;
			evo::SmallVector<AttributeParams> attribute_params_info;
			std::optional<SymbolProcTypeID> type_id;
			SymbolProcTermInfoID value_id;
		};


		struct WhenCond{
			const AST::WhenConditional& when_cond;
			SymbolProcTermInfoID cond;
		};


		struct Alias{
			const AST::AliasDef& alias_def;
			evo::SmallVector<AttributeParams> attribute_params_info;
			SymbolProcTypeID aliased_type;
		};


		template<bool IS_INSTANTIATION>
		struct StructDecl{
			const AST::StructDef& struct_def;
			evo::SmallVector<AttributeParams> attribute_params_info;
			std::optional<BaseType::StructTemplate::ID> struct_template_id{};
			uint32_t instantiation_id = std::numeric_limits<uint32_t>::max();

			StructDecl(
				const AST::StructDef& _struct_def, evo::SmallVector<AttributeParams>&& _attribute_params_info
			) requires(!IS_INSTANTIATION) : struct_def(_struct_def), attribute_params_info(_attribute_params_info) {}

			StructDecl(
				const AST::StructDef& _struct_def,
				evo::SmallVector<AttributeParams>&& _attribute_params_info,
				BaseType::StructTemplate::ID _struct_template_id,
				uint32_t _instantiation_id
			) requires(IS_INSTANTIATION) :
				struct_def(_struct_def),
				attribute_params_info(_attribute_params_info),
				struct_template_id(_struct_template_id),
				instantiation_id(_instantiation_id)
			{}
		};


		struct TemplateStruct{
			const AST::StructDef& struct_def;
			evo::SmallVector<TemplateParamInfo> template_param_infos;
		};


		struct UnionDecl{
			const AST::UnionDef& union_def;
			evo::SmallVector<AttributeParams> attribute_params_info;
		};

		struct UnionAddFields{
			const AST::UnionDef& union_def;
			evo::SmallVector<SymbolProcTypeID> field_types;
		};


		struct EnumDecl{
			const AST::EnumDef& enum_def;
			std::optional<SymbolProcTypeID> underlying_type;
			evo::SmallVector<AttributeParams> attribute_params_info;
		};

		struct EnumAddEnumerators{
			const AST::EnumDef& enum_def;
			evo::SmallVector<std::optional<SymbolProcTermInfoID>> enumerator_values;
		};



		struct FuncDeclExtractDeducers{
			SymbolProcTypeID param_type;
			size_t param_index;
		};


		template<bool IS_INSTANTIATION>
		struct FuncDecl{
			const AST::FuncDef& func_def;
			evo::SmallVector<AttributeParams> attribute_params_info;
			evo::SmallVector<std::optional<SymbolProcTermInfoID>> default_param_values;
			uint32_t instantiation_id = std::numeric_limits<uint32_t>::max();
			size_t num_extra_variadics = 0;

			// param type is nullopt if the param is `this`
			EVO_NODISCARD auto params() const -> evo::ArrayProxy<std::optional<SymbolProcTypeID>> {
				return evo::ArrayProxy<std::optional<SymbolProcTypeID>>(
					this->types.data(), this->func_def.params.size()
				);
			}

			EVO_NODISCARD auto returns() const -> evo::ArrayProxy<SymbolProcTypeID> {
				return evo::ArrayProxy<SymbolProcTypeID>(
					(SymbolProcTypeID*)&this->types[this->func_def.params.size()],
					this->func_def.returns.size()
				);
			}

			EVO_NODISCARD auto error_returns() const -> evo::ArrayProxy<SymbolProcTypeID> {
				if(this->func_def.errorReturns.empty()){
					return evo::ArrayProxy<SymbolProcTypeID>();
					
				}else{
					return evo::ArrayProxy<SymbolProcTypeID>(
						(SymbolProcTypeID*)&this->types[this->func_def.params.size() + this->func_def.returns.size()],
						this->func_def.errorReturns.size()
					);
				}
			}


			FuncDecl(
				const AST::FuncDef& _func_def,
				evo::SmallVector<AttributeParams>&& _attribute_params_info,
				evo::SmallVector<std::optional<SymbolProcTermInfoID>>&& _default_param_values,
				evo::SmallVector<std::optional<SymbolProcTypeID>>&& _types
			) requires(!IS_INSTANTIATION) : 
				func_def(_func_def),
				attribute_params_info(std::move(_attribute_params_info)),
				default_param_values(std::move(_default_param_values)),
				types(std::move(_types))
			{
				#if defined(PCIT_CONFIG_DEBUG)
					const size_t correct_num_types = this->func_def.params.size() 
						+ this->func_def.returns.size() 
						+ this->func_def.errorReturns.size();

					evo::debugAssert(this->types.size() == correct_num_types, "Recieved the incorrect number of types");
				#endif
			}


			FuncDecl(
				const AST::FuncDef& _func_def,
				evo::SmallVector<AttributeParams>&& _attribute_params_info,
				evo::SmallVector<std::optional<SymbolProcTermInfoID>>&& _default_param_values,
				evo::SmallVector<std::optional<SymbolProcTypeID>>&& _types,
				uint32_t _instantiation_id,
				size_t _num_extra_variadics
			) requires(IS_INSTANTIATION) : 
				func_def(_func_def),
				attribute_params_info(std::move(_attribute_params_info)),
				default_param_values(std::move(_default_param_values)),
				types(std::move(_types)),
				instantiation_id(_instantiation_id),
				num_extra_variadics(_num_extra_variadics)
			{
				#if defined(PCIT_CONFIG_DEBUG)
					const size_t correct_num_types = this->func_def.params.size() 
						+ this->func_def.returns.size() 
						+ this->func_def.errorReturns.size();

					evo::debugAssert(this->types.size() == correct_num_types, "Recieved the incorrect number of types");
				#endif
			}


			private:
				evo::SmallVector<std::optional<SymbolProcTypeID>> types;

				static_assert( // check that SymbolProcTypeID uses small std::optional optimization
					sizeof(SymbolProcTypeID) == sizeof(std::optional<SymbolProcTypeID>),
					"\"magically\" getting rid of the optional in `.returns()` and `.errorReturns()` is invalid"
				);
		};


		// Stuff that needs to happen after the decl but before body. This is separate so type definitions can be gotten
		struct FuncPreBody{ 
			const AST::FuncDef& func_def;
		};

		struct FuncDef{
			const AST::FuncDef& func_def;
		};



		struct FuncPrepareConstexprPIRIfNeeded{
			const AST::FuncDef& func_def;
		};



		struct TemplateFuncBegin{
			const AST::FuncDef& func_def;
			evo::SmallVector<TemplateParamInfo> template_param_infos;
		};
		

		struct TemplateFuncSetParamIsDeducer{
			size_t param_index;
		};

		struct TemplateFuncEnd{
			const AST::FuncDef& func_def;
		};



		struct DeletedSpecialMethod{
			const AST::DeletedSpecialMethod& deleted_special_method;
		};

		struct FuncAliasDef{
			const AST::FuncAliasDef& func_alias_def;
			evo::SmallVector<AttributeParams> attribute_params_info;
			SymbolProcTermInfoID target;
		};



		struct InterfacePrepare{
			const AST::InterfaceDef& interface_def;
			evo::SmallVector<AttributeParams> attribute_params_info;
		};


		struct InterfaceFuncDef{
			const AST::FuncDef& func_def;
		};



		struct InterfaceImplDecl{
			const AST::InterfaceImpl& interface_impl;
			SymbolProcTypeID target;
		};

		struct InterfaceInDefImplDecl{
			const AST::InterfaceImpl& interface_impl;
			AST::Node interface_impl_node;
			SymbolProcTypeID target;
		};

		struct InterfaceDeducerImplInstantiationDecl{
			const AST::InterfaceImpl& interface_impl;
			TypeInfo::ID instantiation_type_id;
			BaseType::Interface::Impl& created_impl;
		};

		struct InterfaceImplMethodLookup{
			Token::ID method_name;
		};

		struct InterfaceInDefImplMethod{
			SymbolProcID symbol_proc_id;
		};

		struct InterfaceImplDef{
			const AST::InterfaceImpl& interface_impl;
		};



		//////////////////
		// stmt

		struct LocalVar{
			const AST::VarDef& var_def;
			evo::SmallVector<AttributeParams> attribute_params_info;
			std::optional<SymbolProcTypeID> type_id;
			SymbolProcTermInfoID value;
		};

		struct LocalFuncAlias{
			const AST::FuncAliasDef& func_alias_def;
			evo::SmallVector<AttributeParams> attribute_params_info;
			SymbolProcTermInfoID target;
		};

		struct LocalAlias{
			const AST::AliasDef& alias_def;
			evo::SmallVector<AttributeParams> attribute_params_info;
			SymbolProcTypeID aliased_type;
		};

		struct Return{
			const AST::Return& return_stmt;
			std::optional<SymbolProcTermInfoID>	value;
		};

		struct LabeledReturn{
			const AST::Return& return_stmt;
			std::optional<SymbolProcTermInfoID>	value;
		};

		struct Error{
			const AST::Error& error_stmt;
			std::optional<SymbolProcTermInfoID>	value;
		};

		struct Unreachable{
			Token::ID keyword;
		};

		struct Break{
			const AST::Break& break_stmt;
		};

		struct Continue{
			const AST::Continue& continue_stmt;
		};

		struct Delete{
			const AST::Delete& delete_stmt;
			SymbolProcTermInfoID delete_expr;
		};


		struct BeginCond{
			const AST::Conditional& conditional;
			SymbolProcTermInfoID cond_expr;
		};

		struct EndCondSet{
			Token::ID close_brace;
		};

		struct BeginLocalWhenCond{
			const AST::WhenConditional& when_cond;
			SymbolProcTermInfoID cond_expr;
			SymbolProcInstructionIndex else_index;
		};

		struct EndLocalWhenCond{
			SymbolProcInstructionIndex end_index;
		};





		struct BeginWhile{
			const AST::While& while_stmt;
			SymbolProcTermInfoID cond_expr;
		};

		struct EndWhile{
			Token::ID close_brace;
		};


		struct BeginFor{
			const AST::For& for_stmt;
			evo::SmallVector<SymbolProcTermInfoID> iterables;
			evo::SmallVector<SymbolProcTypeID> types;
		};

		struct EndFor{
			Token::ID close_brace;
		};



		struct BeginForUnroll{
			const AST::For& for_stmt;
			evo::SmallVector<SymbolProcTermInfoID> iterables;
			std::optional<SymbolProcTypeID> index_type_id;
		};

		struct ForUnrollCond{
			const AST::For& for_stmt;
			evo::SmallVector<SymbolProcTermInfoID> iterables;
			evo::SmallVector<SymbolProcTypeID> types;
			SymbolProcInstructionIndex end_index;

			ForUnrollCond(
				const AST::For& _for_stmt,
				evo::SmallVector<SymbolProcTermInfoID>&& _iterables,
				evo::SmallVector<SymbolProcTypeID>&& _types,
				SymbolProcInstructionIndex _end_index
			) : for_stmt(_for_stmt), iterables(std::move(_iterables)), types(std::move(_types)), end_index(_end_index){}

			EVO_NODISCARD auto get_index() const -> size_t { return this->index; }
			auto next_index() const -> void { this->index += 1; }

			private:
				mutable size_t index = 0;
		};

		struct ForUnrollContinue{
			Token::ID close_brace;
			SymbolProcInstructionIndex cond_index;
		};



		struct BeginSwitch{
			const AST::Switch& switch_stmt;
			evo::SmallVector<AttributeParams> attribute_params_info;
			SymbolProcTermInfoID cond;
			bool is_no_jump;
		};

		struct BeginCase{
			const AST::Switch::Case& switch_case;
			evo::SmallVector<SymbolProcTermInfoID> values;
			size_t index;
		};

		struct EndSwitch{
			const AST::Switch& switch_stmt;
		};



		struct BeginDefer{
			const AST::Defer& defer_stmt;
		};

		struct EndDefer{
			Token::ID close_brace;
		};


		struct BeginStmtBlock{
			const AST::Block& stmt_block;
		};

		struct EndStmtBlock{
			Token::ID close_brace;
		};



		struct FuncCall{
			const AST::FuncCall& func_call;
			SymbolProcTermInfoID target;
			evo::SmallVector<SymbolProcTermInfoID> template_args;
			evo::SmallVector<SymbolProcTermInfoID> args;
		};

		struct Assignment{
			const AST::Infix& infix;
			SymbolProcTermInfoID lhs;
			SymbolProcTermInfoID rhs;
			SymbolProcTermInfoID builtin_composite_expr_term_info_id;
		};

		struct AssignmentNew{
			const AST::Infix& infix;
			SymbolProcTermInfoID lhs;
			SymbolProcTypeID type_id;
			evo::SmallVector<SymbolProcTermInfoID> args;
		};

		struct AssignmentCopy{
			const AST::Infix& infix;
			SymbolProcTermInfoID lhs;
			SymbolProcTermInfoID target;
		};

		struct AssignmentMove{
			const AST::Infix& infix;
			SymbolProcTermInfoID lhs;
			SymbolProcTermInfoID target;
		};

		struct AssignmentForward{
			const AST::Infix& infix;
			SymbolProcTermInfoID lhs;
			SymbolProcTermInfoID target;
		};

		struct MultiAssign{
			const AST::MultiAssign& multi_assign;
			evo::SmallVector<std::optional<SymbolProcTermInfoID>> targets;
			SymbolProcTermInfoID value;
		};

		struct DiscardingAssignment{
			const AST::Infix& infix;
			SymbolProcTermInfoID rhs;
		};


		struct TryElseBegin{
			const AST::TryElse& try_else;
			SymbolProcTermInfoID func_call_target;
			evo::SmallVector<SymbolProcTermInfoID> func_call_template_args;
			evo::SmallVector<SymbolProcTermInfoID> func_call_args;
		};




		//////////////////
		// misc expr

		struct TypeToTerm{
			SymbolProcTypeID from;
			SymbolProcTermInfoID to;
		};

		struct WaitOnSubSymbolProcDecl{
			SymbolProcID symbol_proc_id;
		};

		struct WaitOnSubSymbolProcDef{
			SymbolProcID symbol_proc_id;
		};

		template<bool IS_CONSTEXPR, bool ERRORS>
		struct FuncCallExpr{
			const AST::FuncCall& func_call;
			evo::SmallVector<SymbolProcTermInfoID> template_args;
			evo::SmallVector<SymbolProcTermInfoID> args;
			SymbolProcTermInfoID target;
			SymbolProcTermInfoID output;
		};

		struct ConstexprFuncCallRun{
			const AST::FuncCall& func_call;
			SymbolProcTermInfoID target;
			SymbolProcTermInfoID output;
			evo::SmallVector<SymbolProcTermInfoID> args;
		};

		enum class Language{
			PANTHER,
			C,
			CPP,
		};

		template<Language LANGUAGE>
		struct Import{
			const AST::FuncCall& func_call;
			SymbolProcTermInfoID location;
			SymbolProcTermInfoID output;
		};

		struct IsMacroDefined{
			const AST::FuncCall& func_call;
			SymbolProcTermInfoID clang_module;
			SymbolProcTermInfoID macro_name;
			SymbolProcTermInfoID output;
		};

		template<bool IS_CONSTEXPR>
		struct TemplateIntrinsicFuncCall{
			const AST::FuncCall& func_call;
			evo::SmallVector<SymbolProcTermInfoID> template_args;
			evo::SmallVector<SymbolProcTermInfoID> args;
			SymbolProcTermInfoID target;
			SymbolProcTermInfoID output;
		};


		template<bool IS_CONSTEXPR>
		struct Indexer{
			const AST::Indexer& indexer;
			SymbolProcTermInfoID target;
			SymbolProcTermInfoID output;
			evo::SmallVector<SymbolProcTermInfoID> indices;
		};


		struct TemplatedTerm{
			const AST::TemplatedExpr& templated_expr;
			SymbolProcTermInfoID base;
			evo::SmallVector<evo::Variant<SymbolProcTermInfoID, SymbolProcTypeID>> arguments;
			SymbolProcStructInstantiationID instantiation;
		};

		template<bool WAIT_FOR_DEF>
		struct TemplatedTermWait{
			const AST::TemplatedExpr& templated_expr;
			SymbolProcStructInstantiationID instantiation;
			SymbolProcTermInfoID output;
		};

		struct AddTemplateDeclInstantiationType{
			std::string_view ident;
		};




		struct Copy{
			const AST::Prefix& prefix;
			SymbolProcTermInfoID target;
			SymbolProcTermInfoID output;
		};

		struct Move{
			const AST::Prefix& prefix;
			SymbolProcTermInfoID target;
			SymbolProcTermInfoID output;
		};

		struct Forward{
			const AST::Prefix& prefix;
			SymbolProcTermInfoID target;
			SymbolProcTermInfoID output;
		};

		struct AddrOf{
			const AST::Prefix& prefix;
			SymbolProcTermInfoID target;
			SymbolProcTermInfoID output;
		};

		template<bool IS_CONSTEXPR>
		struct PrefixNegate{
			const AST::Prefix& prefix;
			SymbolProcTermInfoID expr;
			SymbolProcTermInfoID output;
		};

		template<bool IS_CONSTEXPR>
		struct PrefixNot{
			const AST::Prefix& prefix;
			SymbolProcTermInfoID expr;
			SymbolProcTermInfoID output;
		};

		template<bool IS_CONSTEXPR>
		struct PrefixBitwiseNot{
			const AST::Prefix& prefix;
			SymbolProcTermInfoID expr;
			SymbolProcTermInfoID output;
		};

		struct Deref{
			const AST::Postfix& postfix;
			SymbolProcTermInfoID target;
			SymbolProcTermInfoID output;
		};

		struct Unwrap{
			const AST::Postfix& postfix;
			SymbolProcTermInfoID target;
			SymbolProcTermInfoID output;
		};

		template<bool IS_CONSTEXPR>
		struct New{
			const AST::New& ast_new;
			SymbolProcTypeID type_id;
			SymbolProcTermInfoID output;
			evo::SmallVector<SymbolProcTermInfoID> args;
		};

		template<bool IS_CONSTEXPR>
		struct ArrayInitNew{
			const AST::ArrayInitNew& array_init_new;
			SymbolProcTypeID type_id;
			SymbolProcTermInfoID output;
			evo::SmallVector<SymbolProcTermInfoID> values;
		};

		template<bool IS_CONSTEXPR>
		struct DesignatedInitNew{
			const AST::DesignatedInitNew& designated_init_new;
			SymbolProcTypeID type_id;
			SymbolProcTermInfoID output;
			evo::SmallVector<SymbolProcTermInfoID> member_init_exprs;
		};

		struct PrepareTryHandler{
			evo::ArrayProxy<Token::ID> except_params;
			SymbolProcTermInfoID attempt_expr;
			SymbolProcTermInfoID output_except_params;
			Token::ID handler_kind_token_id;
		};

		struct TryElseExpr{
			const AST::TryElse& try_else;
			SymbolProcTermInfoID attempt_expr;
			SymbolProcTermInfoID except_params;
			SymbolProcTermInfoID except_expr;
			SymbolProcTermInfoID output;
		};


		struct BeginExprBlock{
			const AST::Block& block;
			Token::ID label;
			evo::SmallVector<SymbolProcTypeID> output_types;
		};

		struct EndExprBlock{
			const AST::Block& block;
			SymbolProcTermInfoID output;
		};


		template<bool IS_CONSTEXPR>
		struct As{
			const AST::Infix& infix;
			SymbolProcTermInfoID expr;
			SymbolProcTypeID target_type;
			SymbolProcTermInfoID output;
		};

		struct OptionalNullCheck{
			const AST::Infix& infix;
			SymbolProcTermInfoID lhs;
			SymbolProcTermInfoID output;
		};

		enum class MathInfixKind{
			COMPARATIVE,
			INTEGRAL_MATH,
			LOGICAL,
			BITWISE_LOGICAL,
			MATH,
			SHIFT,
		};

		template<bool IS_CONSTEXPR, MathInfixKind MATH_INFIX_KIND>
		struct MathInfix{
			const AST::Infix& infix;
			SymbolProcTermInfoID lhs;
			SymbolProcTermInfoID rhs;
			SymbolProcTermInfoID output;
		};


		template<bool IS_CONSTEXPR>
		struct Accessor{
			const AST::Infix& infix;
			SymbolProcTermInfoID lhs;
			Token::ID rhs_ident;
			SymbolProcTermInfoID output;
		};



		//////////////////
		// types

		struct PrimitiveType{
			const AST::Type& ast_type;
			SymbolProcTypeID output;
		};

		struct ArrayType{
			const AST::ArrayType& array_type;
			SymbolProcTypeID elem_type;
			evo::SmallVector<SymbolProcTermInfoID> dimensions;
			std::optional<SymbolProcTermInfoID> terminator;
			SymbolProcTermInfoID output;
		};

		struct ArrayRef{
			const AST::ArrayType& array_type;
			SymbolProcTypeID elem_type;
			evo::SmallVector<std::optional<SymbolProcTermInfoID>> dimensions;
			std::optional<SymbolProcTermInfoID> terminator;
			SymbolProcTermInfoID output;
		};

		struct InterfaceMap{
			const AST::InterfaceMap& interface_map;
			std::optional<SymbolProcTypeID> base_type; // nullopt if polymorphic
			SymbolProcTypeID interface;
			SymbolProcTermInfoID output;
		};

		struct TypeIDConverter{
			const AST::TypeIDConverter& type_id_converter;
			SymbolProcTermInfoID expr;
			SymbolProcTermInfoID output;	
		};

		struct UserType{
			const AST::Type& ast_type;
			SymbolProcTermInfoID base_type;
			SymbolProcTypeID output;
		};

		struct BaseTypeIdent{
			Token::ID ident;
			SymbolProcTermInfoID output;
		};




		//////////////////
		// single token value

		template<bool NEEDS_DEF>
		struct Ident{
			Token::ID ident;
			SymbolProcTermInfoID output;
		};

		struct Intrinsic{
			Token::ID intrinsic;
			SymbolProcTermInfoID output;
		};

		template<bool NEEDS_DEF>
		struct TypeThis{
			Token::ID type_this;
			SymbolProcTermInfoID output;
		};

		struct Literal{
			Token::ID literal;
			SymbolProcTermInfoID output;
		};

		struct Uninit{
			Token::ID uninit_token;
			SymbolProcTermInfoID output;
		};

		struct Zeroinit{
			Token::ID zeroinit_token;
			SymbolProcTermInfoID output;
		};

		struct This{
			Token::ID this_token;
			SymbolProcTermInfoID output;
		};

		struct TypeDeducer{
			Token::ID type_deducer_token;
			SymbolProcTermInfoID output;
		};

		struct ExprDeducer{
			Token::ID expr_deducer_token;
			SymbolProcTermInfoID output;
		};




		enum class Kind{
			// misc symbol proc
			SUSPEND_SYMBOL_PROC,

			// stmts valid in global scope
			NON_LOCAL_VAR_DECL,
			NON_LOCAL_VAR_DEF,
			NON_LOCAL_VAR_DECL_DEF,
			WHEN_COND,
			ALIAS,
			STRUCT_DECL_INSTANTIATION,
			STRUCT_DECL,
			STRUCT_DEF,
			STRUCT_CREATED_SPECIAL_MEMBERS_PIR_IF_NEEDED,
			TEMPLATE_STRUCT,
			UNION_DECL,
			UNION_ADD_FIELDS,
			UNION_DEF,
			ENUM_DECL,
			ENUM_ADD_ENUMERATORS,
			ENUM_DEF,
			FUNC_DECL_EXTRACT_DEDUCERS,
			FUNC_DECL_INSTANTIATION,
			FUNC_DECL,
			FUNC_PRE_BODY,
			FUNC_DEF,
			FUNC_PREPARE_CONSTEXPR_PIR_IF_NEEDED,
			FUNC_CONSTEXPR_PIR_READY_IF_NEEDED,
			TEMPLATE_FUNC_BEGIN,
			TEMPLATE_FUNC_SET_PARAM_IS_DEDUCER,
			TEMPLATE_FUNC_END,
			DELETED_SPECIAL_METHOD,
			FUNC_ALIAS_DEF,
			INTERFACE_PREPARE,
			INTERFACE_DECL,
			INTERFACE_DEF,
			INTERFACE_FUNC_DEF,
			INTERFACE_IMPL_DECL,
			INTERFACE_IN_DEF_IMPL_DECL,
			INTERFACE_DEDUCER_IMPL_INSTANTIATION_DECL,
			INTERFACE_IMPL_METHOD_LOOKUP,
			INTERFACE_IN_DEF_IMPL_METHOD,
			INTERFACE_IMPL_DEF,
			INTERFACE_IMPL_CONSTEXPR_PIR,

			// stmt
			LOCAL_VAR,
			LOCAL_FUNC_ALIAS,
			LOCAL_ALIAS,
			RETURN,
			LABELED_RETURN,
			ERROR,
			UNREACHABLE,
			BREAK,
			CONTINUE,
			DELETE,
			BEGIN_COND,
			COND_NO_ELSE,
			COND_ELSE,
			COND_ELSE_IF,
			END_COND,
			END_COND_SET,
			BEGIN_LOCAL_WHEN_COND,
			END_LOCAL_WHEN_COND,
			BEGIN_WHILE,
			END_WHILE,
			BEGIN_FOR,
			END_FOR,
			BEGIN_FOR_UNROLL,
			FOR_UNROLL_COND,
			FOR_UNROLL_CONTINUE,
			BEGIN_SWITCH,
			BEGIN_CASE,
			END_CASE,
			END_SWITCH,
			BEGIN_DEFER,
			END_DEFER,
			BEGIN_STMT_BLOCK,
			END_STMT_BLOCK,
			FUNC_CALL,
			ASSIGNMENT,
			ASSIGNMENT_NEW,
			ASSIGNMENT_COPY,
			ASSIGNMENT_MOVE,
			ASSIGNMENT_FORWARD,
			MULTI_ASSIGN,
			DISCARDING_ASSIGNMENT,
			TRY_ELSE_BEGIN,
			TRY_ELSE_END,

			// misc expr
			TYPE_TO_TERM,
			REQUIRE_THIS_DEF,
			WAIT_ON_SUB_SYMBOL_PROC_DECL,
			WAIT_ON_SUB_SYMBOL_PROC_DEF,
			FUNC_CALL_EXPR_CONSTEXPR_ERRORS,
			FUNC_CALL_EXPR_CONSTEXPR,
			FUNC_CALL_EXPR_ERRORS,
			FUNC_CALL_EXPR,
			CONSTEXPR_FUNC_CALL_RUN,
			IMPORT_PANTHER,
			IMPORT_C,
			IMPORT_CPP,
			IS_MACRO_DEFINED,
			TEMPLATE_INTRINSIC_FUNC_CALL_CONSTEXPR,
			TEMPLATE_INTRINSIC_FUNC_CALL,
			INDEXER_CONSTEXPR,
			INDEXER,
			TEMPLATED_TERM,
			TEMPLATED_TERM_WAIT_FOR_DEF,
			TEMPLATED_TERM_WAIT_FOR_DECL,
			PUSH_TEMPLATE_DECL_INSTANTIATION_TYPES_SCOPE,
			POP_TEMPLATE_DECL_INSTANTIATION_TYPES_SCOPE,
			ADD_TEMPLATE_DECL_INSTANTIATION_TYPE,
			COPY,
			MOVE,
			FORWARD,
			ADDR_OF,
			PREFIX_NEGATE_CONSTEXPR,
			PREFIX_NEGATE,
			PREFIX_NOT_CONSTEXPR,
			PREFIX_NOT,
			PREFIX_BITWISE_NOT_CONSTEXPR,
			PREFIX_BITWISE_NOT,
			DEREF,
			UNWRAP,
			NEW_CONSTEXPR,
			NEW,
			ARRAY_INIT_NEW_CONSTEXPR,
			ARRAY_INIT_NEW,
			DESIGNATED_INIT_NEW_CONSTEXPR,
			DESIGNATED_INIT_NEW,
			PREPARE_TRY_HANDLER,
			TRY_ELSE_EXPR,
			BEGIN_EXPR_BLOCK,
			END_EXPR_BLOCK,
			AS_CONTEXPR,
			AS,
			OPTIONAL_NULL_CHECK,
			MATH_INFIX_CONSTEXPR_COMPARATIVE,
			MATH_INFIX_CONSTEXPR_MATH,
			MATH_INFIX_CONSTEXPR_INTEGRAL_MATH,
			MATH_INFIX_CONSTEXPR_LOGICAL,
			MATH_INFIX_CONSTEXPR_BITWISE_LOGICAL,
			MATH_INFIX_CONSTEXPR_SHIFT,
			MATH_INFIX_COMPARATIVE,
			MATH_INFIX_MATH,
			MATH_INFIX_INTEGRAL_MATH,
			MATH_INFIX_LOGICAL,
			MATH_INFIX_BITWISE_LOGICAL,
			MATH_INFIX_SHIFT,

			// accessors
			ACCESSOR_NEEDS_DEF,
			ACCESSOR,

			// types
			PRIMITIVE_TYPE,
			PRIMITIVE_TYPE_NEEDS_DEF,
			ARRAY_TYPE,
			ARRAY_REF,
			INTERFACE_MAP,
			TYPE_ID_CONVERTER,
			USER_TYPE,
			BASE_TYPE_IDENT,

			// single token value
			IDENT_NEEDS_DEF,
			IDENT,
			INTRINSIC,
			TYPE_THIS_NEEDS_DEF,
			TYPE_THIS,
			LITERAL,
			UNINIT,
			ZEROINIT,
			THIS,
			TYPE_DEDUCER,
			EXPR_DEDUCER,
		};


		SymbolProcInstruction(Kind instr_kind, uint32_t index) : _kind(instr_kind), _index(index) {}



		EVO_NODISCARD auto kind() const -> Kind { return this->_kind; }



		private:
			Kind _kind;
			uint32_t _index;

			friend class SymbolProcManager;
			friend class SymbolProcBuilder;
	};



	class SymbolProc{
		public:
			using ID = SymbolProcID;
			using Instruction = SymbolProcInstruction;
			using TermInfoID = SymbolProcTermInfoID;
			using StructInstantiationID = SymbolProcStructInstantiationID;
			using TypeID = SymbolProcTypeID;
			using InstructionIndex = SymbolProcInstructionIndex;

			using Namespace = SymbolProcNamespace;

			enum class Status{
				WAITING,
				SUSPENDED,
				IN_DEF_DEDUCER_IMPL_METHOD,
				IN_QUEUE,
				WORKING,
				PASSED_ON_BY_WHEN_COND,
				ERRORED,
				DONE,
			};
			
		public:
			SymbolProc(AST::Node node, SourceID _source_id, std::string_view _ident, SymbolProc* _parent)
				: ast_node(node), source_id(_source_id), ident(_ident), parent(_parent) {}
			~SymbolProc() = default;

			SymbolProc(const SymbolProc&) = delete;
			SymbolProc(SymbolProc&&) = delete;

			EVO_NODISCARD auto getASTNode() const -> AST::Node { return this->ast_node; }
			EVO_NODISCARD auto getSourceID() const -> SourceID { return this->source_id; }
			EVO_NODISCARD auto getIdent() const -> std::string_view { return this->ident; }
			EVO_NODISCARD auto isLocalSymbol() const -> bool { return this->is_local_symbol; }

			EVO_NODISCARD auto isPriority() const -> bool { return this->is_always_priority; }


			EVO_NODISCARD auto getInstruction() const -> const Instruction& {
				return this->instructions[this->inst_index];
			}
			auto nextInstruction() -> void {
				evo::debugAssert(this->isAtEnd() == false, "Symbol proc is already at end");
				evo::debugAssert(
					this->isTemplateSubSymbol() == false, "Cannot get next instruction of template sub-symbol"
				);
				this->inst_index += 1;
			}

			auto setInstructionIndex(InstructionIndex new_index) -> void { this->inst_index = size_t(new_index.get()); }

			EVO_NODISCARD auto isAtEnd() const -> bool {
				return this->inst_index >= this->instructions.size();
			}

			auto setIsTemplateSubSymbol() -> void { this->inst_index = std::numeric_limits<size_t>::max(); }
			EVO_NODISCARD auto isTemplateSubSymbol() const -> bool {
				return this->inst_index == std::numeric_limits<size_t>::max();
			}



			EVO_NODISCARD auto isDeclDone() const -> bool {
				const auto lock = std::scoped_lock( // TODO(FUTURE): needed to take all of these locks?
					this->waiting_for_lock, this->decl_waited_on_lock, this->def_waited_on_lock
				);
				return this->decl_done;
			}

			EVO_NODISCARD auto isPIRDeclDone() const -> bool {
				const auto lock = std::scoped_lock(this->waiting_for_lock, this->pir_decl_waited_on_lock);
				return this->pir_decl_done;
			}

			EVO_NODISCARD auto isDefDone() const -> bool {
				const auto lock = std::scoped_lock( // TODO(FUTURE): needed to take all of these locks?
					this->waiting_for_lock, this->decl_waited_on_lock, this->def_waited_on_lock
				);
				return this->def_done;
			}

			EVO_NODISCARD auto isPIRDefDone() const -> bool {
				const auto lock = std::scoped_lock(this->waiting_for_lock, this->pir_def_waited_on_lock);
				return this->pir_def_done;
			}

			EVO_NODISCARD auto hasErrored() const -> bool {
				const auto lock = std::scoped_lock(this->waiting_for_lock);
				return this->hasErroredNoLock();
			}

			EVO_NODISCARD auto hasErroredNoLock() const -> bool {
				return this->status == Status::ERRORED;
			}

			EVO_NODISCARD auto passedOnByWhenCond() const -> bool {
				const auto lock = std::scoped_lock(this->waiting_for_lock);
				return this->status == Status::PASSED_ON_BY_WHEN_COND;
			}

			

			EVO_NODISCARD auto isWaiting() const -> bool {
				const auto lock = std::scoped_lock(this->waiting_for_lock);
				return this->waiting_for.empty() == false;
			}

			EVO_NODISCARD auto isReadyToBeAddedToWorkQueue() const -> bool {
				return this->isWaiting() == false
					&& this->isTemplateSubSymbol() == false
					&& this->is_local_symbol == false;
			}

			// this should be called after starting to wait
			EVO_NODISCARD auto shouldContinueRunning() -> bool {
				evo::debugAssert(this->status == Status::WORKING, "only should call this func if being worked on");
				const auto lock = std::scoped_lock(this->waiting_for_lock);
				const bool is_waiting = this->waiting_for.empty() == false;
				// if(is_waiting){ this->status = Status::WAITING; }
				return is_waiting == false;
			}



			auto setStatusWaiting() -> void {
				#if defined(PCIT_CONFIG_DEBUG)
					const Status current_status = this->status.load();
					evo::debugAssert(
						current_status == Status::WORKING,
						"Can only set `WAITING` if status is `WORKING` (symbol: {})",
						this->ident
					);
				#endif

				this->status = Status::WAITING;
			}

			auto setStatusInQueue() -> void {
				#if defined(PCIT_CONFIG_DEBUG)
					const Status current_status = this->status.load();
					evo::debugAssert(
						current_status == Status::WAITING || current_status == Status::SUSPENDED,
						"Can only set `IN_QUEUE` if status is `WAITING` or `SUSPENDED` (symbol: {})",
						this->ident
					);
				#endif

				this->status = Status::IN_QUEUE;
			}

			auto setStatusWorking() -> void {
				#if defined(PCIT_CONFIG_DEBUG)
					const Status current_status = this->status.load();
					evo::debugAssert(
						current_status == Status::IN_QUEUE,
						"Can only set `WORKING` if status is `IN_QUEUE` (symbol: {})",
						this->ident
					);
				#endif

				this->status = Status::WORKING;
			}

			auto setStatusSuspended() -> void {
				#if defined(PCIT_CONFIG_DEBUG)
					const Status current_status = this->status.load();
					evo::debugAssert(
						current_status == Status::WORKING,
						"Can only set `SUSPENDED` if status is `WORKING` (symbol: {})",
						this->ident
					);
				#endif

				this->status = Status::SUSPENDED;
			}

			auto setStatusInDefDeducerImplMethod() -> void {
				#if defined(PCIT_CONFIG_DEBUG)
					const Status current_status = this->status.load();
					evo::debugAssert(
						current_status == Status::WAITING,
						"Can only set `IN_DEF_DEDUCER_IMPL_METHOD` if status is `WAITING` (symbol: {})",
						this->ident
					);
				#endif

				this->status = Status::IN_DEF_DEDUCER_IMPL_METHOD;
			}

			auto setStatusPassedOnByWhenCond() -> void { this->status = Status::PASSED_ON_BY_WHEN_COND; }
			auto setStatusErrored() -> void { this->status = Status::ERRORED; }
			auto setStatusDone() -> void { this->status = Status::DONE; }


			// returns `true` if unsuspended
			auto unsuspendIfNeeded() -> bool {
				Status expected = Status::SUSPENDED;
				const bool was_suspended = this->status.compare_exchange_strong(expected, Status::IN_QUEUE);
				return was_suspended;
			}




			enum class WaitOnResult{
				NOT_NEEDED,
				WAITING,
				WAS_ERRORED,
				WAS_PASSED_ON_BY_WHEN_COND,
				CIRCULAR_DEP_DETECTED,
			};

			auto waitOnDeclIfNeeded(ID id, class Context& context, ID self_id) -> WaitOnResult;
			auto waitOnPIRDeclIfNeeded(ID id, class Context& context, ID self_id) -> WaitOnResult;
			auto waitOnDefIfNeeded(ID id, class Context& context, ID self_id) -> WaitOnResult;
			auto waitOnPIRDefIfNeeded(ID id, class Context& context, ID self_id) -> WaitOnResult;




			enum class BuiltinSymbolKind{
				ARRAY_ITERABLE,
				ARRAY_ITERABLE_RT,
				ARRAY_REF_ITERABLE_REF,
				ARRAY_REF_ITERABLE_REF_RT,
				ARRAY_MUT_REF_ITERABLE_MUT_REF,
				ARRAY_MUT_REF_ITERABLE_MUT_REF_RT,

				_MAX_
			};


		private:
			enum class DependencyKind{
				DECL,
				DEF,
			};

			EVO_NODISCARD auto detect_circular_dependency(
				ID id, class Context& context, DependencyKind initial_dependency_kind
			) const -> evo::Result<>;

			auto emit_diagnostic_on_circular_dependency(
				ID id, class Context& constext, DependencyKind initial_dependency_kind
			) const -> void;

		private:
			AST::Node ast_node;
			SourceID source_id;
			std::string_view ident; // empty if symbol doesn't have an ident
									// 	(is when cond, func call, or operator function)
			SymbolProc* parent; // nullptr means no parent

			bool is_always_priority = false;
			std::optional<BuiltinSymbolKind> builtin_symbol_proc_kind;

			evo::StepVector<Instruction> instructions{};


			struct StructInstantiationInfo{
				evo::Variant<const BaseType::StructTemplate::Instantiation*, BaseType::StructTemplateDeducer::ID> id;
				bool requires_pub;
			};

			// TODO(PERF): optimize the memory usage here?
			evo::SmallVector<std::optional<TermInfo>> term_infos{};
			evo::SmallVector<std::optional<TypeInfo::VoidableID>> type_ids{};
			evo::SmallVector<std::optional<StructInstantiationInfo>> struct_instantiations{};


			std::atomic<bool> is_waiting_for_builtin = false;
			evo::SmallVector<ID> waiting_for{};
			mutable evo::SpinLock waiting_for_lock{};

			evo::SmallVector<ID> decl_waited_on_by{};
			mutable evo::SpinLock decl_waited_on_lock{};

			evo::SmallVector<ID> pir_decl_waited_on_by{};
			mutable evo::SpinLock pir_decl_waited_on_lock{};

			evo::SmallVector<ID> def_waited_on_by{};
			mutable evo::SpinLock def_waited_on_lock{};

			evo::SmallVector<ID> pir_def_waited_on_by{};
			mutable evo::SpinLock pir_def_waited_on_lock{};


			struct StructSpecialMemberFuncs{
				std::optional<sema::Func::ID> init_func{};
				std::optional<sema::Func::ID> delete_func{};
				std::optional<sema::Func::ID> copy_func{};
				std::optional<sema::Func::ID> move_func{};
			};

			using DataStackItem = evo::Variant<StructSpecialMemberFuncs>;
			std::stack<DataStackItem, evo::StepVector<DataStackItem>> data_stack{};



			struct NonLocalVarInfo{
				evo::Variant<sema::GlobalVar::ID, uint32_t> sema_id; // uint32_t is for member index
				                                                     //  (invalid after struct def)
			};


			struct WhenCondInfo{
				evo::SmallVector<SymbolProcID> then_ids;
				evo::SmallVector<SymbolProcID> else_ids;
			};

			struct AliasInfo{
				BaseType::Alias::ID alias_id;
			};

			// only needed for non-template structs or template struct instantiations
			struct StructInfo{
				BaseType::StructTemplate::Instantiation* instantiation = nullptr;
				evo::SmallVector<SymbolProcID> stmts{};
				Namespace member_symbols{};
				BaseType::Struct::ID struct_id = BaseType::Struct::ID::dummy();
			};

			struct UnionInfo{
				evo::SmallVector<SymbolProcID> stmts{};
				Namespace member_symbols{};
				BaseType::Union::ID union_id = BaseType::Union::ID::dummy();
			};

			struct EnumInfo{
				evo::SmallVector<SymbolProcID> stmts{};
				Namespace member_symbols{};
				BaseType::Enum::ID enum_id = BaseType::Enum::ID::dummy();
			};

			struct FuncInfo{
				std::stack<sema::Stmt, evo::SmallVector<sema::Stmt, 4>> subscopes{};

				size_t num_members_of_initializing_are_uninit = 0;

				evo::SmallVector<std::optional<TypeInfo::ID>> param_type_to_check_if_is_copy{};
				evo::SmallVector<sema::Param::ID> actual_variadic_params{};

				sema::TemplatedFunc::Instantiation* instantiation = nullptr;
				evo::SmallVector<std::optional<TypeInfo::ID>> instantiation_param_arg_types{};

				std::optional<sema::Func::ID> flipped_version{};

				std::unordered_set<sema::Func::ID> dependent_funcs{}; // Only needed if the func is comptime
				std::unordered_set<sema::GlobalVar::ID> dependent_vars{}; // Only needed if the func is comptime
			};

			struct TemplateFuncInfo{
				sema::TemplatedFunc::ID templated_func_id;
				sema::TemplatedFunc& templated_func;
			};

			struct InterfaceImplInfo{
				struct ParentTypeInfo{
					SymbolProcNamespace& namespaced_members;
					sema::ScopeLevel& scope_level;
					SourceID source_id;
				};

				BaseType::Interface::ID target_interface_id;
				BaseType::Interface& target_interface;
				evo::Variant<ParentTypeInfo, TypeInfo::ID> type_info;
				evo::Variant<BaseType::Interface::Impl*, BaseType::Interface::DeducerImpl*> interface_impl;
				evo::SmallVector<TermInfo> targets{};
			};

			evo::Variant<
				std::monostate,
				NonLocalVarInfo,
				WhenCondInfo,
				AliasInfo,
				StructInfo,
				UnionInfo,
				EnumInfo,
				FuncInfo,
				TemplateFuncInfo,
				InterfaceImplInfo
			> extra_info{};

			std::optional<sema::ScopeManager::Scope::ID> sema_scope_id{};


			size_t inst_index = 0;
			bool is_local_symbol = false;
			std::atomic<bool> decl_done = false;
			std::atomic<bool> def_done = false;
			std::atomic<bool> pir_decl_done = false;
			std::atomic<bool> pir_def_done = false;

			std::atomic<Status> status = Status::WAITING; // if changing this, probably get the lock `waiting_for_lock`

			friend class SymbolProcBuilder;
			friend class SymbolProcManager;
			friend class SemanticAnalyzer;
			friend class Context;
	};


}

