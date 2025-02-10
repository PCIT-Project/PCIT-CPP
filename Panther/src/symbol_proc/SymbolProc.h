////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>
#include <PCIT_core.h>

#include "../../include/source/source_data.h"
#include "../../include/AST/AST.h"
#include "../sema/ExprInfo.h"
// #include "../sema/sema.h"
#include "../../include/TypeManager.h"

namespace pcit::panther{

	struct SymbolProcExprInfoID : public core::UniqueID<uint32_t, struct SymbolProcExprInfoID> { 
		using core::UniqueID<uint32_t, SymbolProcExprInfoID>::UniqueID;
	};

	struct SymbolProcExprInfoIDOptInterface{
		static constexpr auto init(SymbolProcExprInfoID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const SymbolProcExprInfoID& id) -> bool {
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

}


namespace std{

	template<>
	class optional<pcit::panther::SymbolProcExprInfoID> 
		: public pcit::core::Optional<
			pcit::panther::SymbolProcExprInfoID, pcit::panther::SymbolProcExprInfoIDOptInterface
		>{

		public:
			using pcit::core::Optional<
				pcit::panther::SymbolProcExprInfoID, pcit::panther::SymbolProcExprInfoIDOptInterface
			>::Optional;
			using pcit::core::Optional<
				pcit::panther::SymbolProcExprInfoID, pcit::panther::SymbolProcExprInfoIDOptInterface
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

	// For lookup in Context::symbol_proc_manager
	struct SymbolProcID : public core::UniqueID<uint32_t, struct SymbolProcID> { 
		using core::UniqueID<uint32_t, SymbolProcID>::UniqueID;
	};


	// TODO: make this data oriented
	struct SymbolProcInstruction{
		using AttributeExprs = evo::SmallVector<evo::StaticVector<SymbolProcExprInfoID, 2>>;

		//////////////////
		// globals

		struct GlobalVarDecl{
			const AST::VarDecl& var_decl;
			AttributeExprs attribute_exprs;
			SymbolProcTypeID type_id;
		};

		struct GlobalVarDef{
			const AST::VarDecl& var_decl;
			SymbolProcExprInfoID value_id;
		};

		struct GlobalVarDeclDef{
			const AST::VarDecl& var_decl;
			AttributeExprs attribute_exprs;
			std::optional<SymbolProcTypeID> type_id;
			SymbolProcExprInfoID value_id;
		};


		struct GlobalWhenCond{
			const AST::WhenConditional& when_cond;
			SymbolProcExprInfoID cond;
		};


		struct GlobalAliasDecl{
			const AST::AliasDecl& alias_decl;
			AttributeExprs attribute_exprs;
		};

		struct GlobalAliasDef{
			const AST::AliasDecl& alias_decl;
			SymbolProcTypeID aliased_type;
		};


		//////////////////
		// misc expr

		struct FuncCall{
			const AST::FuncCall& func_call;
			SymbolProcExprInfoID target;
			SymbolProcExprInfoID output;
			evo::SmallVector<SymbolProcExprInfoID> args;
		};

		struct Import{
			const AST::FuncCall& func_call;
			SymbolProcExprInfoID location;
			SymbolProcExprInfoID output;
		};


		//////////////////
		// accessors

		struct TypeAccessor{
			const AST::Infix& infix;
			SymbolProcExprInfoID lhs;
			Token::ID rhs_ident;
			SymbolProcExprInfoID output;
		};

		struct ComptimeExprAccessor{
			const AST::Infix& infix;
			SymbolProcExprInfoID lhs;
			Token::ID rhs_ident;
			SymbolProcExprInfoID output;
		};

		struct ExprAccessor{
			const AST::Infix& infix;
			SymbolProcExprInfoID lhs;
			Token::ID rhs_ident;
			SymbolProcExprInfoID output;
		};



		//////////////////
		// types

		struct PrimitiveType{
			const AST::Type& ast_type;
			SymbolProcTypeID output;
		};

		struct UserType{
			const AST::Type& ast_type;
			SymbolProcExprInfoID base_type;
			SymbolProcTypeID output;
		};

		struct BaseTypeIdent{
			Token::ID ident;
			SymbolProcExprInfoID output;
		};

		struct ComptimeIdent{
			Token::ID ident;
			SymbolProcExprInfoID output;
		};


		//////////////////
		// single token value

		struct Ident{
			Token::ID ident;
			SymbolProcExprInfoID output;
		};

		struct Intrinsic{
			Token::ID intrinsic;
			SymbolProcExprInfoID output;
		};

		struct Literal{
			Token::ID literal;
			SymbolProcExprInfoID output;
		};


		//////////////////
		// instruction impl

		auto visit(auto callable) const { return this->inst.visit(callable); }

		template<class T>
		EVO_NODISCARD auto is() const -> bool { return this->inst.is<T>(); }

		template<class T>
		EVO_NODISCARD auto as() const -> const T& { return this->inst.as<T>(); }

		evo::Variant<
			GlobalWhenCond,
			GlobalVarDecl,
			GlobalVarDef,
			GlobalVarDeclDef,
			GlobalAliasDecl,
			GlobalAliasDef,
			FuncCall,
			Import,
			TypeAccessor,
			ComptimeExprAccessor,
			ExprAccessor,
			PrimitiveType,
			UserType,
			BaseTypeIdent,
			ComptimeIdent,
			Ident,
			Intrinsic,
			Literal
		> inst;
	};



	// class SymbolProcTemplate{
	// 	public:

	// 		// For lookup in Context::symbol_proc_manager
	// 		struct ID : public core::UniqueID<uint32_t, struct ID> { 
	// 			using core::UniqueID<uint32_t, ID>::UniqueID;
	// 		};

	// 	public:
	// 		SymbolProcTemplate(SourceID _source_id, std::string_view _ident) : source_id(_source_id), ident(_ident) {}
	// 		~SymbolProcTemplate() = default;

	// 		EVO_NODISCARD auto getInstruction(size_t i) const -> const Instruction& { return this->instructions[i]; }
	// 		EVO_NODISCARD auto numInstructions() const -> size_t { return this->instructions.size(); }

	// 		EVO_NODISCARD auto numExprInfos() const -> uint32_t { return this->num_expr_infos; }
	// 		EVO_NODISCARD auto numTypeIDs() const -> uint32_t { return this->num_type_ids; }

	// 		EVO_NODISCARD auto getSourceID() const -> SourceID { return this->source_id; }


	// 	private:
	// 		SourceID source_id;
	// 		std::string_view ident; // size 0 if it symbol doesn't have an ident
	// 								// 	(is when cond, func call, or operator function)

	// 		std::vector<Instruction> instructions{};
	// 		uint32_t num_expr_infos = 0;
	// 		uint32_t num_type_ids = 0;

	// 		friend class SymbolProcBuilder;
	// };



	class SymbolProc{
		public:
			using ID = SymbolProcID;
			using Instruction = SymbolProcInstruction;
			using ExprInfoID = SymbolProcExprInfoID;
			using TypeID = SymbolProcTypeID;

			
		public:
			SymbolProc(AST::Node node, SourceID _source_id, std::string_view _ident)
				: ast_node(node), source_id(_source_id), ident(_ident) {}
			~SymbolProc() = default;


			EVO_NODISCARD auto getInstruction() const -> const SymbolProc::Instruction& {
				return this->instructions[this->inst_index];
			}
			EVO_NODISCARD auto nextInstruction() -> void { this->inst_index += 1; }

			EVO_NODISCARD auto isAtEnd() const -> bool {
				return this->inst_index >= this->instructions.size();
			}


			EVO_NODISCARD auto getExprInfo(SymbolProc::ExprInfoID id) -> const std::optional<ExprInfo>& {
				return this->expr_infos[id.get()];
			}

			EVO_NODISCARD auto getTypeID(SymbolProc::TypeID id) -> const std::optional<TypeInfo::VoidableID>& {
				return this->type_ids[id.get()];
			}


			EVO_NODISCARD auto isDeclDone() const -> bool {
				const auto lock = std::scoped_lock(this->waiting_lock);
				return this->decl_done;
			}

			EVO_NODISCARD auto isDefDone() const -> bool {
				const auto lock = std::scoped_lock(this->waiting_lock);
				return this->def_done;
			}

			EVO_NODISCARD auto hasErrored() const -> bool { return this->errored; }
			EVO_NODISCARD auto passedOnByWhenCond() const -> bool { return this->passed_on_by_when_cond; }

			EVO_NODISCARD auto getSourceID() const -> SourceID { return this->source_id; }
			EVO_NODISCARD auto getIdent() const -> std::string_view { return this->ident; }

			// TODO: needs to take lock?
			EVO_NODISCARD auto isWaiting() const -> bool { return this->waiting_for.empty() == false; };


			enum class WaitOnResult{
				NotNeeded,
				Waiting,
				WasErrored,
				WasPassedOnByWhenCond,
				CircularDepDetected,
			};

			auto waitOnDeclIfNeeded(ID id, class Context& context, ID self_id) -> WaitOnResult;
			auto waitOnDefIfNeeded(ID id, class Context& context, ID self_id) -> WaitOnResult;


		private:
			auto detect_circular_dependency(ID id, class Context& context) const -> bool;


		private:
			AST::Node ast_node;
			SourceID source_id;
			std::string_view ident; // size 0 if it symbol doesn't have an ident
									// 	(is when cond, func call, or operator function)

			std::vector<Instruction> instructions{};
			std::vector<std::optional<ExprInfo>> expr_infos{};
			std::vector<std::optional<TypeInfo::VoidableID>> type_ids{};

			mutable core::SpinLock waiting_lock{};

			evo::SmallVector<ID> waiting_for{};
			evo::SmallVector<ID> decl_waited_on_by{};
			evo::SmallVector<ID> def_waited_on_by{};


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


			evo::Variant<std::monostate, VarInfo, WhenCondInfo, AliasInfo> extra_info{};


			size_t inst_index = 0;
			bool decl_done = false;
			bool def_done = false;
			bool passed_on_by_when_cond = false;
			bool errored = false;

			friend class SymbolProcBuilder;
			friend class SemanticAnalyzer;
	};


}

