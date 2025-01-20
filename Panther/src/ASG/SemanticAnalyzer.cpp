////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./SemanticAnalyzer.h"

#include "../../include/Context.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::panther{

	// static auto remove_dep(evo::SmallVector<DG::Node::ID>& deps, DG::Node::ID dep_to_remove) -> void {
	// 	if(deps.empty()){ return; }

	// 	for(DG::Node::ID& dep : deps){
	// 		if(dep == dep_to_remove){
	// 			if(deps.back() != dep){
	// 				dep = deps.back();
	// 			}
	// 		}
	// 	}

	// 	deps.pop_back();
	// }


	SemanticAnalyzer::SemanticAnalyzer(Context& _context, DG::Node::ID _node_id)
		: context(_context),
		node_id(_node_id),
		node(this->context.dg_buffer[_node_id]),
		source(this->context.getSourceManager()[this->context.dg_buffer[_node_id].sourceID]) {}

	
	auto SemanticAnalyzer::analyzeDecl() -> bool {
		evo::debugAssert(
			this->node.declSemaStatus == DG::Node::SemaStatus::InQueue,
			"Incorrect status for node to analyze semantics of decl"
		);

		switch(this->node.astNode.kind()){
			break; case AST::Kind::None: evo::debugFatalBreak("Invalid AST::Node");

			break; case AST::Kind::VarDecl:         if(this->analyze_decl_var() == false){       return false; }
			break; case AST::Kind::FuncDecl:        if(this->analyze_decl_func() == false){      return false; }
			break; case AST::Kind::AliasDecl:       if(this->analyze_decl_alias() == false){     return false; }
			break; case AST::Kind::TypedefDecl:     if(this->analyze_decl_typedef() == false){   return false; }
			break; case AST::Kind::StructDecl:      if(this->analyze_decl_struct() == false){    return false; }
			break; case AST::Kind::WhenConditional: if(this->analyze_decl_when_cond() == false){ return false; }

			break; default: evo::debugFatalBreak("Unsupported decl kind");
		}

		this->node.declSemaStatus = DG::Node::SemaStatus::Done;

		this->context.trace("SemanticAnalyzer::analyzeDecl");
		return true;
	}


	auto SemanticAnalyzer::analyzeDef() -> bool {
		evo::debugAssert(
			this->node.defSemaStatus == DG::Node::SemaStatus::InQueue,
			"Incorrect status for node to analyze semantics of def"
		);

		switch(this->node.astNode.kind()){
			break; case AST::Kind::None: evo::debugFatalBreak("Invalid AST::Node");

			break; case AST::Kind::VarDecl:         if(this->analyze_def_var() == false){     return false; }
			break; case AST::Kind::FuncDecl:        if(this->analyze_def_func() == false){    return false; }
			break; case AST::Kind::AliasDecl:       if(this->analyze_def_alias() == false){   return false; }
			break; case AST::Kind::TypedefDecl:     if(this->analyze_def_typedef() == false){ return false; }
			break; case AST::Kind::StructDecl:      if(this->analyze_def_struct() == false){  return false; }
			break; case AST::Kind::WhenConditional: evo::debugFatalBreak("Should never analyze def of when cond");
			
			break; default: evo::debugFatalBreak("Unsupported decl kind");
		}

		for(const DG::Node::ID& required_by_id : this->node.requiredBy){
			this->propogate_to_required_by<false>(required_by_id);
		}
		this->node.defSemaStatus = DG::Node::SemaStatus::Done;

		this->context.trace("SemanticAnalyzer::analyzeDef");
		this->context.dg_buffer.num_nodes_sema_status_not_done -= 1;
		return true;
	}



	template<bool PROP_FOR_DECLS>
	auto SemanticAnalyzer::propogate_to_required_by(DG::Node::ID required_by_id) -> void {
		DG::Node& required_by = this->context.dg_buffer[required_by_id];


		const auto lock = std::scoped_lock(required_by.lock);

		if(required_by.defSemaStatus != DG::Node::SemaStatus::NotYet){ return; }

		if constexpr(PROP_FOR_DECLS){
			required_by.defDeps.decls.erase(this->node_id);
		}else{
			required_by.defDeps.defs.erase(this->node_id);
		}

		if(required_by.declSemaStatus != DG::Node::SemaStatus::NotYet){
			if(required_by.hasNoDefDeps()){
				required_by.defSemaStatus = DG::Node::SemaStatus::InQueue;
				this->context.add_task_to_work_manager(Context::SemaDef(required_by_id));
			}

		}else{
			if constexpr(PROP_FOR_DECLS){
				required_by.declDeps.decls.erase(this->node_id);
			}else{
				required_by.declDeps.defs.erase(this->node_id);
			}

			if(required_by.hasNoDeclDeps()){
				if(required_by.hasNoDeclDeps()){
					if(required_by.hasNoDefDeps() && required_by.astNode.kind() != AST::Kind::WhenConditional){
						required_by.declSemaStatus = DG::Node::SemaStatus::InQueue;
						required_by.defSemaStatus = DG::Node::SemaStatus::InQueue;
						this->context.add_task_to_work_manager(Context::SemaDeclDef(required_by_id));
					}else{
						required_by.declSemaStatus = DG::Node::SemaStatus::InQueue;
						this->context.add_task_to_work_manager(Context::SemaDecl(required_by_id));
					}
				}
			}
		}
	}




	auto SemanticAnalyzer::analyze_decl_var() -> bool {
		evo::debugAssert(this->node.astNode.kind() == AST::Kind::VarDecl, "Incorrect AST::Kind");

		for(const DG::Node::ID& required_by_id : this->node.requiredBy){
			this->propogate_to_required_by<true>(required_by_id);
		}
		return true;
	}

	auto SemanticAnalyzer::analyze_decl_func() -> bool {
		evo::debugAssert(this->node.astNode.kind() == AST::Kind::FuncDecl, "Incorrect AST::Kind");

		for(const DG::Node::ID& required_by_id : this->node.requiredBy){
			this->propogate_to_required_by<true>(required_by_id);
		}
		return true;
	}

	auto SemanticAnalyzer::analyze_decl_alias() -> bool {
		evo::debugAssert(this->node.astNode.kind() == AST::Kind::AliasDecl, "Incorrect AST::Kind");

		for(const DG::Node::ID& required_by_id : this->node.requiredBy){
			this->propogate_to_required_by<true>(required_by_id);
		}
		return true;
	}

	auto SemanticAnalyzer::analyze_decl_typedef() -> bool {
		evo::debugAssert(this->node.astNode.kind() == AST::Kind::TypedefDecl, "Incorrect AST::Kind");

		for(const DG::Node::ID& required_by_id : this->node.requiredBy){
			this->propogate_to_required_by<true>(required_by_id);
		}
		return true;
	}

	auto SemanticAnalyzer::analyze_decl_struct() -> bool {
		evo::debugAssert(this->node.astNode.kind() == AST::Kind::StructDecl, "Incorrect AST::Kind");

		for(const DG::Node::ID& required_by_id : this->node.requiredBy){
			this->propogate_to_required_by<true>(required_by_id);
		}
		return true;
	}

	auto SemanticAnalyzer::analyze_decl_when_cond() -> bool {
		evo::debugAssert(this->node.astNode.kind() == AST::Kind::WhenConditional, "Incorrect AST::Kind");

		// TODO: only propogate to actually used block
		for(const DG::Node::ID& required_by_id : this->node.requiredBy){
			this->propogate_to_required_by<true>(required_by_id);
		}
		this->context.dg_buffer.num_nodes_sema_status_not_done -= 1;
		return true;
	}





	auto SemanticAnalyzer::analyze_def_var() -> bool {
		evo::debugAssert(this->node.astNode.kind() == AST::Kind::VarDecl, "Incorrect AST::Kind");
		
		return true;
	}

	auto SemanticAnalyzer::analyze_def_func() -> bool {
		evo::debugAssert(this->node.astNode.kind() == AST::Kind::FuncDecl, "Incorrect AST::Kind");
		
		return true;
	}

	auto SemanticAnalyzer::analyze_def_alias() -> bool {
		evo::debugAssert(this->node.astNode.kind() == AST::Kind::AliasDecl, "Incorrect AST::Kind");
		
		return true;
	}

	auto SemanticAnalyzer::analyze_def_typedef() -> bool {
		evo::debugAssert(this->node.astNode.kind() == AST::Kind::TypedefDecl, "Incorrect AST::Kind");
		
		return true;
	}

	auto SemanticAnalyzer::analyze_def_struct() -> bool {
		evo::debugAssert(this->node.astNode.kind() == AST::Kind::StructDecl, "Incorrect AST::Kind");
		
		return true;
	}



}