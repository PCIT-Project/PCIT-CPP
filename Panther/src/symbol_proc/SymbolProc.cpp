////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./SymbolProc.h"

#include <queue>

#include "../../include/Context.h"



#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

namespace pcit::panther{

	// TODO(PERF): Is there a faster way of doing this than just locking the whole thing?
	static core::SpinLock wait_on_if_needed_lock{};
	

	auto SymbolProc::waitOnDeclIfNeeded(ID id, Context& context, ID self_id) -> WaitOnResult {
		const auto wait_on_lock = std::scoped_lock(wait_on_if_needed_lock);
		
		// TODO(PERF): is doing these checks before taking locks faster
		if(this->decl_done){ return WaitOnResult::NOT_NEEDED; }
		if(this->status == Status::PASSED_ON_BY_WHEN_COND){ return WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND; }
		if(this->status == Status::ERRORED){ return WaitOnResult::WAS_ERRORED; }

		if(id == self_id){
			context.emitError(
				Diagnostic::Code::SYMBOL_PROC_CIRCULAR_DEP,
				Diagnostic::Location::get(this->ast_node, context.getSourceManager()[this->source_id]),
				"Detected a circular dependency when analyzing this symbol:",
				Diagnostic::Info("Self dependency")
			);
			return WaitOnResult::CIRCULAR_DEP_DETECTED;
		}

		if(this->detect_circular_dependency(id, context, DependencyKind::DEF) == false){
			return WaitOnResult::CIRCULAR_DEP_DETECTED;
		}

		SymbolProc& waiting_symbol = context.symbol_proc_manager.getSymbolProc(id);

		const auto lock = std::scoped_lock(this->decl_waited_on_lock, waiting_symbol.waiting_for_lock);

		if(this->decl_done){ return WaitOnResult::NOT_NEEDED; }
		if(this->status == Status::PASSED_ON_BY_WHEN_COND){ return WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND; }
		if(this->status == Status::ERRORED){ return WaitOnResult::WAS_ERRORED; }

		this->decl_waited_on_by.emplace_back(id);
		waiting_symbol.waiting_for.emplace_back(self_id);

		return WaitOnResult::WAITING;
	}

	auto SymbolProc::waitOnPIRDeclIfNeeded(ID id, Context& context, ID self_id) -> WaitOnResult {
		const auto wait_on_lock = std::scoped_lock(wait_on_if_needed_lock);

		if(this->status == Status::PASSED_ON_BY_WHEN_COND){ return WaitOnResult::NOT_NEEDED; }
		if(this->status == Status::ERRORED){ return WaitOnResult::WAS_ERRORED; }

		SymbolProc& waiting_symbol = context.symbol_proc_manager.getSymbolProc(id);

		const auto lock = std::scoped_lock(this->pir_decl_waited_on_lock, waiting_symbol.waiting_for_lock);

		if(this->pir_decl_done){ return WaitOnResult::NOT_NEEDED; }
		if(this->status == Status::PASSED_ON_BY_WHEN_COND){ return WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND; }
		if(this->status == Status::ERRORED){ return WaitOnResult::WAS_ERRORED; }

		this->pir_decl_waited_on_by.emplace_back(id);
		waiting_symbol.waiting_for.emplace_back(self_id);

		return WaitOnResult::WAITING;
	}

	auto SymbolProc::waitOnDefIfNeeded(ID id, Context& context, ID self_id) -> WaitOnResult {
		const auto wait_on_lock = std::scoped_lock(wait_on_if_needed_lock);

		if(this->def_done){ return WaitOnResult::NOT_NEEDED; }
		if(this->status == Status::PASSED_ON_BY_WHEN_COND){ return WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND; }
		if(this->status == Status::ERRORED){ return WaitOnResult::WAS_ERRORED; }

		if(id == self_id){
			context.emitError(
				Diagnostic::Code::SYMBOL_PROC_CIRCULAR_DEP,
				Diagnostic::Location::get(this->ast_node, context.getSourceManager()[this->source_id]),
				"Detected a circular dependency when analyzing this symbol:",
				Diagnostic::Info("Self dependency")
			);
			return WaitOnResult::CIRCULAR_DEP_DETECTED;
		}

		if(this->detect_circular_dependency(id, context, DependencyKind::DEF) == false){
			return WaitOnResult::CIRCULAR_DEP_DETECTED;
		}

		SymbolProc& waiting_symbol = context.symbol_proc_manager.getSymbolProc(id);

		const auto lock = std::scoped_lock(this->def_waited_on_lock, waiting_symbol.waiting_for_lock);

		if(this->def_done){ return WaitOnResult::NOT_NEEDED; }
		if(this->status == Status::PASSED_ON_BY_WHEN_COND){ return WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND; }
		if(this->status == Status::ERRORED){ return WaitOnResult::WAS_ERRORED; }

		this->def_waited_on_by.emplace_back(id);
		waiting_symbol.waiting_for.emplace_back(self_id);

		return WaitOnResult::WAITING;
	}


	auto SymbolProc::waitOnPIRDefIfNeeded(ID id, Context& context, ID self_id) -> WaitOnResult {
		const auto wait_on_lock = std::scoped_lock(wait_on_if_needed_lock);

		if(this->status == Status::PASSED_ON_BY_WHEN_COND){ return WaitOnResult::NOT_NEEDED; }
		if(this->status == Status::ERRORED){ return WaitOnResult::WAS_ERRORED; }

		SymbolProc& waiting_symbol = context.symbol_proc_manager.getSymbolProc(id);

		const auto lock = std::scoped_lock(this->pir_decl_waited_on_lock, waiting_symbol.waiting_for_lock);

		if(this->pir_def_done){ return WaitOnResult::NOT_NEEDED; }
		if(this->status == Status::PASSED_ON_BY_WHEN_COND){ return WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND; }
		if(this->status == Status::ERRORED){ return WaitOnResult::WAS_ERRORED; }

		this->pir_def_waited_on_by.emplace_back(id);
		waiting_symbol.waiting_for.emplace_back(self_id);

		return WaitOnResult::WAITING;
	}



	auto SymbolProc::detect_circular_dependency(ID id, Context& context, DependencyKind initial_dependency_kind) const
	-> bool {
		auto visited_queue = std::queue<ID>();

		{
			const auto lock = std::scoped_lock(this->waiting_for_lock);
			for(const ID& waiting_for_id : this->waiting_for){
				visited_queue.push(waiting_for_id);
			}
		}
		
		while(visited_queue.empty() == false){
			const ID visited_id = visited_queue.front();
			visited_queue.pop();

			const SymbolProc& visited = context.symbol_proc_manager.getSymbolProc(visited_id);

			if(visited_id == id){
				this->emit_diagnostic_on_circular_dependency(id, context, initial_dependency_kind);
				return false;
			}


			{
				const auto lock = std::scoped_lock(visited.waiting_for_lock);
				for(const ID& waiting_for_id : visited.waiting_for){
					visited_queue.push(waiting_for_id);
				}
			}
		}

		return true;
	}




	// This is a separate function from `detect_circular_dependency` since the computation required to have the 
	//     extra information is much slower, so only do the slow case if a circular dependency is known to exist
	auto SymbolProc::emit_diagnostic_on_circular_dependency(
		ID id, Context& context, DependencyKind initial_dependency_kind
	) const -> void {
		struct VisitedInfo{
			ID id;
			DependencyKind kind;
		};


		auto visited_queue = std::queue<VisitedInfo>();

		{
			const auto lock = std::scoped_lock(this->waiting_for_lock);
			for(const ID& waiting_for_id : this->waiting_for){
				visited_queue.emplace(waiting_for_id, initial_dependency_kind);
			}
		}
		
		while(true){
			const VisitedInfo visited_info = visited_queue.front();
			visited_queue.pop();

			const SymbolProc& visited = context.symbol_proc_manager.getSymbolProc(visited_info.id);

			if(visited_info.id == id){
				auto infos = evo::SmallVector<Diagnostic::Info>();

				switch(visited_info.kind){
					case DependencyKind::DECL: {
						infos.emplace_back(
							"Requries declaration of this symbol:",
							Diagnostic::Location::get(this->ast_node, context.getSourceManager()[this->source_id])
						);
					} break;

					case DependencyKind::DEF: {
						infos.emplace_back(
							"Requries definition of this symbol:",
							Diagnostic::Location::get(this->ast_node, context.getSourceManager()[this->source_id])
						);
					} break;
				}

				context.emitError(
					Diagnostic::Code::SYMBOL_PROC_CIRCULAR_DEP,
					Diagnostic::Location::get(visited.ast_node, context.getSourceManager()[visited.source_id]),
					"Detected a circular dependency when analyzing this symbol:",
					std::move(infos)
				);

				return;
			}


			{
				const auto lock = std::scoped_lock(visited.waiting_for_lock);
				for(const ID& waiting_for_id : visited.waiting_for){
					const SymbolProc& waiting_for_symbol = context.symbol_proc_manager.getSymbolProc(waiting_for_id);

					{ // declaration
						const auto decl_lock = std::scoped_lock(waiting_for_symbol.decl_waited_on_lock);

						if(
							std::ranges::find(waiting_for_symbol.decl_waited_on_by, visited_info.id)
							!= waiting_for_symbol.decl_waited_on_by.end()
						){
							visited_queue.emplace(waiting_for_id, DependencyKind::DECL);
							continue;
						}
					}


					{ // definition
						const auto def_lock = std::scoped_lock(waiting_for_symbol.def_waited_on_lock);

						if(
							std::ranges::find(waiting_for_symbol.def_waited_on_by, visited_info.id)
							!= waiting_for_symbol.def_waited_on_by.end()
						){
							visited_queue.emplace(waiting_for_id, DependencyKind::DEF);
							continue;
						}
					}

				}
			}
			
		}

	}






	//////////////////////////////////////////////////////////////////////
	// instruction

	#if defined(PCIT_CONFIG_DEBUG)

		auto SymbolProc::Instruction::print() const -> std::string_view {
			return this->inst.visit([](const auto& instr) -> std::string_view {
				using InstrType = std::decay_t<decltype(instr)>;


				if constexpr(std::is_same<InstrType, SuspendSymbolProc>()){
					return "SuspendSymbolProc";

				}else if constexpr(std::is_same<InstrType, NonLocalVarDecl>()){
					return "NonLocalVarDecl";

				}else if constexpr(std::is_same<InstrType, NonLocalVarDef>()){
					return "NonLocalVarDef";

				}else if constexpr(std::is_same<InstrType, NonLocalVarDeclDef>()){
					return "NonLocalVarDeclDef";

				}else if constexpr(std::is_same<InstrType, WhenCond>()){
					return "WhenCond";

				}else if constexpr(std::is_same<InstrType, AliasDecl>()){
					return "AliasDecl";

				}else if constexpr(std::is_same<InstrType, AliasDef>()){
					return "AliasDef";

				}else if constexpr(std::is_same<InstrType, StructDecl<true>>()){
					return "StructDecl<true>";

				}else if constexpr(std::is_same<InstrType, StructDecl<false>>()){
					return "StructDecl<false>";

				}else if constexpr(std::is_same<InstrType, StructDef>()){
					return "StructDef";

				}else if constexpr(std::is_same<InstrType, TemplateStruct>()){
					return "TemplateStruct";

				}else if constexpr(std::is_same<InstrType, FuncDeclExtractDeducersIfNeeded>()){
					return "FuncDeclExtractDeducersIfNeeded";

				}else if constexpr(std::is_same<InstrType, FuncDecl<true>>()){
					return "FuncDecl<true>";

				}else if constexpr(std::is_same<InstrType, FuncDecl<false>>()){
					return "FuncDecl<false>";

				}else if constexpr(std::is_same<InstrType, FuncPreBody>()){
					return "FuncPreBody";

				}else if constexpr(std::is_same<InstrType, FuncDef>()){
					return "FuncDef";

				}else if constexpr(std::is_same<InstrType, FuncPrepareConstexprPIRIfNeeded>()){
					return "FuncPrepareConstexprPIRIfNeeded";

				}else if constexpr(std::is_same<InstrType, FuncConstexprPIRReadyIfNeeded>()){
					return "FuncConstexprPIRReadyIfNeeded";

				}else if constexpr(std::is_same<InstrType, TemplateFuncBegin>()){
					return "TemplateFuncBegin";

				}else if constexpr(std::is_same<InstrType, TemplateFuncCheckParamIsInterface>()){
					return "TemplateFuncCheckParamIsInterface";

				}else if constexpr(std::is_same<InstrType, TemplateFuncSetParamIsDeducer>()){
					return "TemplateFuncSetParamIsDeducer";

				}else if constexpr(std::is_same<InstrType, TemplateFuncEnd>()){
					return "TemplateFuncEnd";

				}else if constexpr(std::is_same<InstrType, InterfaceDecl>()){
					return "InterfaceDecl";

				}else if constexpr(std::is_same<InstrType, InterfaceDef>()){
					return "InterfaceDef";

				}else if constexpr(std::is_same<InstrType, InterfaceFuncDef>()){
					return "InterfaceFuncDef";

				}else if constexpr(std::is_same<InstrType, InterfaceImplDecl>()){
					return "InterfaceImplDecl";

				}else if constexpr(std::is_same<InstrType, InterfaceImplMethodLookup>()){
					return "InterfaceImplMethodLookup";

				}else if constexpr(std::is_same<InstrType, InterfaceImplDef>()){
					return "InterfaceImplDef";

				}else if constexpr(std::is_same<InstrType, InterfaceImplConstexprPIR>()){
					return "InterfaceImplConstexprPIR";

				}else if constexpr(std::is_same<InstrType, LocalVar>()){
					return "LocalVar";

				}else if constexpr(std::is_same<InstrType, LocalAlias>()){
					return "LocalAlias";

				}else if constexpr(std::is_same<InstrType, Return>()){
					return "Return";

				}else if constexpr(std::is_same<InstrType, LabeledReturn>()){
					return "LabeledReturn";

				}else if constexpr(std::is_same<InstrType, Error>()){
					return "Error";

				}else if constexpr(std::is_same<InstrType, Unreachable>()){
					return "Unreachable";

				}else if constexpr(std::is_same<InstrType, Break>()){
					return "Break";

				}else if constexpr(std::is_same<InstrType, Continue>()){
					return "Continue";

				}else if constexpr(std::is_same<InstrType, BeginCond>()){
					return "BeginCond";

				}else if constexpr(std::is_same<InstrType, CondNoElse>()){
					return "CondNoElse";

				}else if constexpr(std::is_same<InstrType, CondElse>()){
					return "CondElse";

				}else if constexpr(std::is_same<InstrType, CondElseIf>()){
					return "CondElseIf";

				}else if constexpr(std::is_same<InstrType, EndCond>()){
					return "EndCond";

				}else if constexpr(std::is_same<InstrType, EndCondSet>()){
					return "EndCondSet";

				}else if constexpr(std::is_same<InstrType, BeginLocalWhenCond>()){
					return "BeginLocalWhenCond";

				}else if constexpr(std::is_same<InstrType, EndLocalWhenCond>()){
					return "EndLocalWhenCond";

				}else if constexpr(std::is_same<InstrType, BeginWhile>()){
					return "BeginWhile";

				}else if constexpr(std::is_same<InstrType, EndWhile>()){
					return "EndWhile";

				}else if constexpr(std::is_same<InstrType, BeginDefer>()){
					return "BeginDefer";

				}else if constexpr(std::is_same<InstrType, EndDefer>()){
					return "EndDefer";

				}else if constexpr(std::is_same<InstrType, BeginStmtBlock>()){
					return "BeginStmtBlock";

				}else if constexpr(std::is_same<InstrType, EndStmtBlock>()){
					return "EndStmtBlock";

				}else if constexpr(std::is_same<InstrType, FuncCall>()){
					return "FuncCall";

				}else if constexpr(std::is_same<InstrType, Assignment>()){
					return "Assignment";

				}else if constexpr(std::is_same<InstrType, MultiAssign>()){
					return "MultiAssign";

				}else if constexpr(std::is_same<InstrType, DiscardingAssignment>()){
					return "DiscardingAssignment";

				}else if constexpr(std::is_same<InstrType, TypeToTerm>()){
					return "TypeToTerm";

				}else if constexpr(std::is_same<InstrType, RequireThisDef>()){
					return "RequireThisDef";

				}else if constexpr(std::is_same<InstrType, WaitOnSubSymbolProcDef>()){
					return "WaitOnSubSymbolProcDef";

				}else if constexpr(std::is_same<InstrType, FuncCallExpr<true, false>>()){
					return "FuncCallExpr<true, false>";

				}else if constexpr(std::is_same<InstrType, FuncCallExpr<false, true>>()){
					return "FuncCallExpr<false, true>";

				}else if constexpr(std::is_same<InstrType, FuncCallExpr<false, false>>()){
					return "FuncCallExpr<false, false>";

				}else if constexpr(std::is_same<InstrType, ConstexprFuncCallRun>()){
					return "ConstexprFuncCallRun";

				}else if constexpr(std::is_same<InstrType, Import<Language::PANTHER>>()){
					return "Import<Language::PANTHER>";

				}else if constexpr(std::is_same<InstrType, Import<Language::C>>()){
					return "Import<Language::C>";

				}else if constexpr(std::is_same<InstrType, Import<Language::CPP>>()){
					return "Import<Language::CPP>";

				}else if constexpr(std::is_same<InstrType, TemplateIntrinsicFuncCall<true>>()){
					return "TemplateIntrinsicFuncCall<true>";

				}else if constexpr(std::is_same<InstrType, TemplateIntrinsicFuncCall<false>>()){
					return "TemplateIntrinsicFuncCall<false>";

				}else if constexpr(std::is_same<InstrType, Indexer<true>>()){
					return "Indexer<true>";

				}else if constexpr(std::is_same<InstrType, Indexer<false>>()){
					return "Indexer<false>";

				}else if constexpr(std::is_same<InstrType, TemplatedTerm>()){
					return "TemplatedTerm";

				}else if constexpr(std::is_same<InstrType, TemplatedTermWait<true>>()){
					return "TemplatedTermWait<true>";

				}else if constexpr(std::is_same<InstrType, TemplatedTermWait<false>>()){
					return "TemplatedTermWait<false>";

				}else if constexpr(std::is_same<InstrType, PushTemplateDeclInstantiationTypesScope>()){
					return "PushTemplateDeclInstantiationTypesScope";

				}else if constexpr(std::is_same<InstrType, PopTemplateDeclInstantiationTypesScope>()){
					return "PopTemplateDeclInstantiationTypesScope";

				}else if constexpr(std::is_same<InstrType, AddTemplateDeclInstantiationType>()){
					return "AddTemplateDeclInstantiationType";

				}else if constexpr(std::is_same<InstrType, Copy>()){
					return "Copy";

				}else if constexpr(std::is_same<InstrType, Move>()){
					return "Move";

				}else if constexpr(std::is_same<InstrType, Forward>()){
					return "Forward";

				}else if constexpr(std::is_same<InstrType, AddrOf<true>>()){
					return "AddrOf<true>";

				}else if constexpr(std::is_same<InstrType, AddrOf<false>>()){
					return "AddrOf<false>";

				}else if constexpr(std::is_same<InstrType, PrefixNegate<true>>()){
					return "PrefixNegate<true>";

				}else if constexpr(std::is_same<InstrType, PrefixNegate<false>>()){
					return "PrefixNegate<false>";

				}else if constexpr(std::is_same<InstrType, PrefixNot<true>>()){
					return "PrefixNot<true>";

				}else if constexpr(std::is_same<InstrType, PrefixNot<false>>()){
					return "PrefixNot<false>";

				}else if constexpr(std::is_same<InstrType, PrefixBitwiseNot<true>>()){
					return "PrefixBitwiseNot<true>";

				}else if constexpr(std::is_same<InstrType, PrefixBitwiseNot<false>>()){
					return "PrefixBitwiseNot<false>";

				}else if constexpr(std::is_same<InstrType, Deref>()){
					return "Deref";

				}else if constexpr(std::is_same<InstrType, StructInitNew<true>>()){
					return "StructInitNew<true>";

				}else if constexpr(std::is_same<InstrType, StructInitNew<false>>()){
					return "StructInitNew<false>";

				}else if constexpr(std::is_same<InstrType, PrepareTryHandler>()){
					return "PrepareTryHandler";

				}else if constexpr(std::is_same<InstrType, TryElse>()){
					return "TryElse";

				}else if constexpr(std::is_same<InstrType, BeginExprBlock>()){
					return "BeginExprBlock";

				}else if constexpr(std::is_same<InstrType, EndExprBlock>()){
					return "EndExprBlock";

				}else if constexpr(std::is_same<InstrType, As<true>>()){
					return "As<true>";

				}else if constexpr(std::is_same<InstrType, As<false>>()){
					return "As<false>";

				}else if constexpr(std::is_same<InstrType, MathInfix<true, MathInfixKind::COMPARATIVE>>()){
					return "MathInfix<true, MathInfixKind::COMPARATIVE>";

				}else if constexpr(std::is_same<InstrType, MathInfix<true, MathInfixKind::MATH>>()){
					return "MathInfix<true, MathInfixKind::MATH>";

				}else if constexpr(std::is_same<InstrType, MathInfix<true, MathInfixKind::INTEGRAL_MATH>>()){
					return "MathInfix<true, MathInfixKind::INTEGRAL_MATH>";

				}else if constexpr(std::is_same<InstrType, MathInfix<true, MathInfixKind::SHIFT>>()){
					return "MathInfix<true, MathInfixKind::SHIFT>";

				}else if constexpr(std::is_same<InstrType, MathInfix<false, MathInfixKind::COMPARATIVE>>()){
					return "MathInfix<false, MathInfixKind::COMPARATIVE>";

				}else if constexpr(std::is_same<InstrType, MathInfix<false, MathInfixKind::MATH>>()){
					return "MathInfix<false, MathInfixKind::MATH>";

				}else if constexpr(std::is_same<InstrType, MathInfix<false, MathInfixKind::INTEGRAL_MATH>>()){
					return "MathInfix<false, MathInfixKind::INTEGRAL_MATH>";

				}else if constexpr(std::is_same<InstrType, MathInfix<false, MathInfixKind::SHIFT>>()){
					return "MathInfix<false, MathInfixKind::SHIFT>";

				}else if constexpr(std::is_same<InstrType, Accessor<true>>()){
					return "Accessor<true>";

				}else if constexpr(std::is_same<InstrType, Accessor<false>>()){
					return "Accessor<false>";

				}else if constexpr(std::is_same<InstrType, PrimitiveType>()){
					return "PrimitiveType";

				}else if constexpr(std::is_same<InstrType, ArrayType>()){
					return "ArrayType";

				}else if constexpr(std::is_same<InstrType, TypeIDConverter>()){
					return "TypeIDConverter";

				}else if constexpr(std::is_same<InstrType, UserType>()){
					return "UserType";

				}else if constexpr(std::is_same<InstrType, BaseTypeIdent>()){
					return "BaseTypeIdent";

				}else if constexpr(std::is_same<InstrType, Ident<false>>()){
					return "Ident<false>";

				}else if constexpr(std::is_same<InstrType, Ident<true>>()){
					return "Ident<true>";

				}else if constexpr(std::is_same<InstrType, Intrinsic>()){
					return "Intrinsic";

				}else if constexpr(std::is_same<InstrType, Literal>()){
					return "Literal";

				}else if constexpr(std::is_same<InstrType, Uninit>()){
					return "Uninit";

				}else if constexpr(std::is_same<InstrType, Zeroinit>()){
					return "Zeroinit";

				}else if constexpr(std::is_same<InstrType, This>()){
					return "This";

				}else if constexpr(std::is_same<InstrType, TypeDeducer>()){
					return "TypeDeducer";

				}else{
					static_assert(false, "Unsupported instruction kind");
				}
			});
		}

	#endif


}