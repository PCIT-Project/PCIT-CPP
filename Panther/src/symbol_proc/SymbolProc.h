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

	struct SymbolProcTermInfoIDOptInterface{
		static constexpr auto init(SymbolProcTermInfoID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const SymbolProcTermInfoID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};


	struct SymbolProcTypeID : public core::UniqueID<uint32_t, struct SymbolProcTypeID> { 
		using core::UniqueID<uint32_t, SymbolProcTypeID>::UniqueID;
	};

	struct SymbolProcTypeIDOptInterface{
		static constexpr auto init(SymbolProcTypeID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const SymbolProcTypeID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};


	struct SymbolProcStructInstantiationID : public core::UniqueID<uint32_t, struct SymbolProcStructInstantiationID> { 
		using core::UniqueID<uint32_t, SymbolProcStructInstantiationID>::UniqueID;
	};

	struct SymbolProcStructInstantiationIDOptInterface{
		static constexpr auto init(SymbolProcStructInstantiationID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const SymbolProcStructInstantiationID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

}


namespace std{

	template<>
	class optional<pcit::panther::SymbolProcTermInfoID> 
		: public pcit::core::Optional<
			pcit::panther::SymbolProcTermInfoID, pcit::panther::SymbolProcTermInfoIDOptInterface
		>{

		public:
			using pcit::core::Optional<
				pcit::panther::SymbolProcTermInfoID, pcit::panther::SymbolProcTermInfoIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::SymbolProcTermInfoID, pcit::panther::SymbolProcTermInfoIDOptInterface
			>::operator=;
	};


	template<>
	class optional<pcit::panther::SymbolProcTypeID> 
		: public pcit::core::Optional<
			pcit::panther::SymbolProcTypeID, pcit::panther::SymbolProcTypeIDOptInterface
		>{

		public:
			using pcit::core::Optional<
				pcit::panther::SymbolProcTypeID, pcit::panther::SymbolProcTypeIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::SymbolProcTypeID, pcit::panther::SymbolProcTypeIDOptInterface
			>::operator=;
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
			const AST::VarDecl& var_decl;
			evo::SmallVector<AttributeParams> attribute_params_info;
			SymbolProcTypeID type_id;
		};

		struct NonLocalVarDef{
			const AST::VarDecl& var_decl;
			std::optional<SymbolProcTermInfoID> value_id;
		};

		struct NonLocalVarDeclDef{
			const AST::VarDecl& var_decl;
			evo::SmallVector<AttributeParams> attribute_params_info;
			std::optional<SymbolProcTypeID> type_id;
			SymbolProcTermInfoID value_id;
		};


		struct WhenCond{
			const AST::WhenConditional& when_cond;
			SymbolProcTermInfoID cond;
		};


		struct AliasDecl{
			const AST::AliasDecl& alias_decl;
			evo::SmallVector<AttributeParams> attribute_params_info;
		};

		struct AliasDef{
			const AST::AliasDecl& alias_decl;
			SymbolProcTypeID aliased_type;
		};


		template<bool IS_INSTANTIATION>
		struct StructDecl{
			const AST::StructDecl& struct_decl;
			evo::SmallVector<AttributeParams> attribute_params_info;
			uint32_t instantiation_id = std::numeric_limits<uint32_t>::max();

			StructDecl(
				const AST::StructDecl& _struct_decl, evo::SmallVector<AttributeParams>&& _attribute_params_info
			) requires(!IS_INSTANTIATION) : struct_decl(_struct_decl), attribute_params_info(_attribute_params_info) {}

			StructDecl(
				const AST::StructDecl& _struct_decl,
				evo::SmallVector<AttributeParams>&& _attribute_params_info,
				uint32_t _instantiation_id
			) requires(IS_INSTANTIATION) :
				struct_decl(_struct_decl),
				attribute_params_info(_attribute_params_info),
				instantiation_id(_instantiation_id)
			{}
		};

		struct StructDef{};

		struct TemplateStruct{
			const AST::StructDecl& struct_decl;
			evo::SmallVector<TemplateParamInfo> template_param_infos;
		};


		template<bool IS_INSTANTIATION>
		struct FuncDecl{
			const AST::FuncDecl& func_decl;
			evo::SmallVector<AttributeParams> attribute_params_info;
			evo::SmallVector<std::optional<SymbolProcTermInfoID>> default_param_values;
			uint32_t instantiation_id = std::numeric_limits<uint32_t>::max();

			// param type is nullopt if the param is `this`
			EVO_NODISCARD auto params() const -> evo::ArrayProxy<std::optional<SymbolProcTypeID>> {
				return evo::ArrayProxy<std::optional<SymbolProcTypeID>>(
					this->types.data(), this->func_decl.params.size()
				);
			}

			EVO_NODISCARD auto returns() const -> evo::ArrayProxy<SymbolProcTypeID> {
				return evo::ArrayProxy<SymbolProcTypeID>(
					(SymbolProcTypeID*)&this->types[this->func_decl.params.size()],
					this->func_decl.returns.size()
				);
			}

			EVO_NODISCARD auto errorReturns() const -> evo::ArrayProxy<SymbolProcTypeID> {
				if(this->func_decl.errorReturns.empty()){
					return evo::ArrayProxy<SymbolProcTypeID>();
					
				}else{
					return evo::ArrayProxy<SymbolProcTypeID>(
						(SymbolProcTypeID*)&this->types[this->func_decl.params.size() + this->func_decl.returns.size()],
						this->func_decl.errorReturns.size()
					);
				}
			}


			FuncDecl(
				const AST::FuncDecl& _func_decl,
				evo::SmallVector<AttributeParams>&& _attribute_params_info,
				evo::SmallVector<std::optional<SymbolProcTermInfoID>>&& _default_param_values,
				evo::SmallVector<std::optional<SymbolProcTypeID>>&& _types
			) requires(!IS_INSTANTIATION) : 
				func_decl(_func_decl),
				attribute_params_info(_attribute_params_info),
				default_param_values(_default_param_values),
				types(_types)
			{
				#if defined(PCIT_CONFIG_DEBUG)
					const size_t correct_num_types = this->func_decl.params.size() 
						+ this->func_decl.returns.size() 
						+ this->func_decl.errorReturns.size();

					evo::debugAssert(this->types.size() == correct_num_types, "Recieved the incorrect number of types");
				#endif
			}


			FuncDecl(
				const AST::FuncDecl& _func_decl,
				evo::SmallVector<AttributeParams>&& _attribute_params_info,
				evo::SmallVector<std::optional<SymbolProcTermInfoID>>&& _default_param_values,
				evo::SmallVector<std::optional<SymbolProcTypeID>>&& _types,
				uint32_t _instantiation_id
			) requires(IS_INSTANTIATION) : 
				func_decl(_func_decl),
				attribute_params_info(_attribute_params_info),
				default_param_values(_default_param_values),
				types(_types),
				instantiation_id(_instantiation_id)
			{
				#if defined(PCIT_CONFIG_DEBUG)
					const size_t correct_num_types = this->func_decl.params.size() 
						+ this->func_decl.returns.size() 
						+ this->func_decl.errorReturns.size();

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

		struct FuncDef{
			const AST::FuncDecl& func_decl;
		};

		struct FuncConstexprPIRReadyIfNeeded{};


		struct TemplateFunc{
			const AST::FuncDecl& func_decl;
			evo::SmallVector<TemplateParamInfo> template_param_infos;
		};


		//////////////////
		// stmt

		struct LocalVar{
			const AST::VarDecl& var_decl;
			evo::SmallVector<AttributeParams> attribute_params_info;
			std::optional<SymbolProcTypeID> type_id;
			SymbolProcTermInfoID value;
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

		struct BeginDefer{
			const AST::Defer& defer_stmt;
		};

		struct EndDefer{};

		struct Unreachable{
			Token::ID keyword;
		};

		struct BeginStmtBlock{
			const AST::Block& stmt_block;
		};

		struct EndStmtBlock{};

		struct FuncCall{
			const AST::FuncCall& func_call;
			SymbolProcTermInfoID target;
			evo::SmallVector<SymbolProcTermInfoID> args;
		};

		struct Assignment{
			const AST::Infix& infix;
			SymbolProcTermInfoID lhs;
			SymbolProcTermInfoID rhs;
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




		//////////////////
		// misc expr

		struct TypeToTerm{
			SymbolProcTypeID from;
			SymbolProcTermInfoID to;
		};

		template<bool IS_CONSTEXPR, bool ERRORS>
		struct FuncCallExpr{
			const AST::FuncCall& func_call;
			SymbolProcTermInfoID target;
			SymbolProcTermInfoID output;
			evo::SmallVector<SymbolProcTermInfoID> args;
		};

		struct ConstexprFuncCallRun{
			const AST::FuncCall& func_call;
			SymbolProcTermInfoID target;
			SymbolProcTermInfoID output;
			evo::SmallVector<SymbolProcTermInfoID> args;
		};

		struct Import{
			const AST::FuncCall& func_call;
			SymbolProcTermInfoID location;
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


		struct TemplatedTerm{
			const AST::TemplatedExpr& templated_expr;
			SymbolProcTermInfoID base;
			evo::SmallVector<evo::Variant<SymbolProcTermInfoID, SymbolProcTypeID>> arguments;
			SymbolProcStructInstantiationID instantiation;
		};

		struct TemplatedTermWait{
			SymbolProcStructInstantiationID instantiation;
			SymbolProcTermInfoID output;
		};

		struct PushTemplateDeclInstantiationTypesScope{};
		struct PopTemplateDeclInstantiationTypesScope{};
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

		template<bool IS_READ_ONLY>
		struct AddrOf{
			const AST::Prefix& prefix;
			SymbolProcTermInfoID target;
			SymbolProcTermInfoID output;
		};

		struct Deref{
			const AST::Postfix& postfix;
			SymbolProcTermInfoID target;
			SymbolProcTermInfoID output;
		};


		struct PrepareTryHandler{
			evo::ArrayProxy<Token::ID> except_params;
			SymbolProcTermInfoID attempt_expr;
			SymbolProcTermInfoID output_except_params;
			Token::ID handler_kind_token_id;
		};

		struct TryElse{
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


		//////////////////
		// accessors

		template<bool NEEDS_DEF>
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

		struct TypeDeducer{
			Token::ID type_deducer_token;
			SymbolProcTermInfoID output;
		};


		//////////////////
		// instruction impl

		auto visit(auto callable) const { return this->inst.visit(callable); }

		template<class T>
		EVO_NODISCARD auto is() const -> bool { return this->inst.is<T>(); }

		template<class T>
		EVO_NODISCARD auto as() const -> const T& { return this->inst.as<T>(); }

		evo::Variant<
			// stmts valid in global scope
			WhenCond,
			NonLocalVarDecl,
			NonLocalVarDef,
			NonLocalVarDeclDef,
			AliasDecl,
			AliasDef,
			StructDecl<true>,
			StructDecl<false>,
			StructDef,
			TemplateStruct,
			FuncDecl<false>,
			FuncDef,
			FuncConstexprPIRReadyIfNeeded,
			TemplateFunc,

			// stmt
			LocalVar,
			Return,
			LabeledReturn,
			Error,
			BeginDefer,
			EndDefer,
			Unreachable,
			BeginStmtBlock,
			EndStmtBlock,
			FuncCall,
			Assignment,
			MultiAssign,
			DiscardingAssignment,

			// misc expr
			TypeToTerm,
			// FuncCallExpr<true, true>,
			FuncCallExpr<true, false>,
			FuncCallExpr<false, true>,
			FuncCallExpr<false, false>,
			ConstexprFuncCallRun,
			Import,
			TemplateIntrinsicFuncCall<true>,
			TemplateIntrinsicFuncCall<false>,
			TemplatedTerm,
			TemplatedTermWait,
			PushTemplateDeclInstantiationTypesScope,
			PopTemplateDeclInstantiationTypesScope,
			AddTemplateDeclInstantiationType,
			Copy,
			Move,
			AddrOf<true>,
			AddrOf<false>,
			Deref,
			PrepareTryHandler,
			TryElse,
			BeginExprBlock,
			EndExprBlock,

			// accessors
			Accessor<true>,
			Accessor<false>,

			// types
			PrimitiveType,
			UserType,
			BaseTypeIdent,

			// single token value
			Ident<false>,
			Ident<true>,
			Intrinsic,
			Literal,
			Uninit,
			Zeroinit,
			TypeDeducer
		> inst;
	};



	class SymbolProc{
		public:
			using ID = SymbolProcID;
			using Instruction = SymbolProcInstruction;
			using TermInfoID = SymbolProcTermInfoID;
			using StructInstantiationID = SymbolProcStructInstantiationID;
			using TypeID = SymbolProcTypeID;

			using Namespace = SymbolProcNamespace;
			
		public:
			SymbolProc(AST::Node node, SourceID _source_id, std::string_view _ident, SymbolProc* _parent)
				: ast_node(node), source_id(_source_id), ident(_ident), parent(_parent) {}
			~SymbolProc() = default;


			EVO_NODISCARD auto getInstruction() const -> const SymbolProc::Instruction& {
				return this->instructions[this->inst_index];
			}
			EVO_NODISCARD auto nextInstruction() -> void { this->inst_index += 1; }

			EVO_NODISCARD auto isAtEnd() const -> bool {
				return this->inst_index >= this->instructions.size();
			}

			auto setIsTemplateSubSymbol() -> void { this->inst_index = std::numeric_limits<size_t>::max(); }
			EVO_NODISCARD auto isTemplateSubSymbol() const -> bool {
				return this->inst_index == std::numeric_limits<size_t>::max();
			}


			EVO_NODISCARD auto getTermInfo(SymbolProc::TermInfoID id) -> const std::optional<TermInfo>& {
				return this->term_infos[id.get()];
			}

			EVO_NODISCARD auto getTypeID(SymbolProc::TypeID id) -> const std::optional<TypeInfo::VoidableID>& {
				return this->type_ids[id.get()];
			}

			EVO_NODISCARD auto getStructInstantiationID(SymbolProc::StructInstantiationID id)
			-> const BaseType::StructTemplate::Instantiation& {
				return *this->struct_instantiations[id.get()];
			}


			EVO_NODISCARD auto isDeclDone() const -> bool {
				const auto lock = std::scoped_lock( // TODO(FUTURE): needed to take all of these locks?
					this->waiting_for_lock, this->decl_waited_on_lock, this->def_waited_on_lock
				);
				return this->decl_done;
			}

			EVO_NODISCARD auto isDefDone() const -> bool {
				const auto lock = std::scoped_lock( // TODO(FUTURE): needed to take all of these locks?
					this->waiting_for_lock, this->decl_waited_on_lock, this->def_waited_on_lock
				);
				return this->def_done;
			}

			EVO_NODISCARD auto isPIRReadyDone() const -> bool {
				const auto lock = std::scoped_lock( // TODO(FUTURE): needed to take all of these locks?
					this->waiting_for_lock, this->pir_ready_waited_on_lock
				);
				return this->pir_ready;
			}

			EVO_NODISCARD auto hasErrored() const -> bool { return this->errored; }
			EVO_NODISCARD auto passedOnByWhenCond() const -> bool { return this->passed_on_by_when_cond; }

			EVO_NODISCARD auto getSourceID() const -> SourceID { return this->source_id; }
			EVO_NODISCARD auto getIdent() const -> std::string_view { return this->ident; }

			EVO_NODISCARD auto isWaiting() const -> bool {
				const auto lock = std::scoped_lock(this->waiting_for_lock);
				return this->waiting_for.empty() == false;
			}

			EVO_NODISCARD auto isReadyToBeAddedToWorkQueue() const -> bool {
				return this->isWaiting() == false && this->isTemplateSubSymbol() == false;
			}

			// this should be called after starting to wait
			EVO_NODISCARD auto shouldContinueRunning() -> bool {
				evo::debugAssert(this->being_worked_on.load(), "only should call this func if being worked on");
				const auto lock = std::scoped_lock(this->waiting_for_lock);
				const bool is_waiting = this->waiting_for.empty() == false;
				if(is_waiting){ this->being_worked_on = false; }
				return is_waiting == false;
			}


			enum class WaitOnResult{
				NOT_NEEDED,
				WAITING,
				WAS_ERRORED,
				WAS_PASSED_ON_BY_WHEN_COND,
				CIRCULAR_DEP_DETECTED,
			};

			auto waitOnDeclIfNeeded(ID id, class Context& context, ID self_id) -> WaitOnResult;
			auto waitOnDefIfNeeded(ID id, class Context& context, ID self_id) -> WaitOnResult;
			auto waitOnPIRReadyIfNeeded(ID id, class Context& context, ID self_id) -> WaitOnResult;


		private:
			auto detect_circular_dependency(ID id, class Context& context) const -> bool;


		private:
			AST::Node ast_node;
			SourceID source_id;
			std::string_view ident; // size 0 if it symbol doesn't have an ident
									// 	(is when cond, func call, or operator function)
			SymbolProc* parent; // nullptr means no parent

			evo::SmallVector<Instruction> instructions{};

			// TODO(PERF): optimize the memory usage here?
			evo::SmallVector<std::optional<TermInfo>> term_infos{};
			evo::SmallVector<std::optional<TypeInfo::VoidableID>> type_ids{};
			evo::SmallVector<const BaseType::StructTemplate::Instantiation*> struct_instantiations{};



			evo::SmallVector<ID> waiting_for{};
			mutable core::SpinLock waiting_for_lock{};

			evo::SmallVector<ID> decl_waited_on_by{};
			mutable core::SpinLock decl_waited_on_lock{};

			evo::SmallVector<ID> def_waited_on_by{};
			mutable core::SpinLock def_waited_on_lock{};

			evo::SmallVector<ID> pir_ready_waited_on_by{};
			mutable core::SpinLock pir_ready_waited_on_lock{};


			struct NonLocalVarInfo{
				evo::Variant<sema::GlobalVar::ID, uint32_t> sema_id; // uint32_t is for member index
				                                                     //  (invalid after struct def)
			};

			struct VarInfo{
				sema::Var::ID sema_var_id;
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


			// Only needed if the func is comptime
			struct FuncInfo{
				std::unordered_set<sema::Func::ID> dependent_funcs{};
				std::unordered_set<sema::GlobalVar::ID> dependent_vars{};
			};

			evo::Variant<
				std::monostate, NonLocalVarInfo, VarInfo, WhenCondInfo, AliasInfo, StructInfo, FuncInfo
			> extra_info{};

			std::optional<sema::ScopeManager::Scope::ID> sema_scope_id{};


			size_t inst_index = 0;
			bool decl_done = false;
			bool def_done = false;
			bool pir_lower_done = false;
			bool pir_ready = false;
			std::atomic<bool> passed_on_by_when_cond = false;
			std::atomic<bool> errored = false;

			std::atomic<bool> being_worked_on = false;

			friend class SymbolProcBuilder;
			friend class SemanticAnalyzer;
			friend struct Diagnostic;
	};


}

