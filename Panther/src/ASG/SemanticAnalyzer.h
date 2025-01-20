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

#include "../DG/DG.h"

namespace pcit::panther{


	class SemanticAnalyzer{
		public:
			SemanticAnalyzer(class Context& _context, DG::Node::ID _node_id);
			~SemanticAnalyzer() = default;

			auto analyzeDecl() -> bool;
			auto analyzeDef() -> bool;

		private:
			template<bool PROP_FOR_DECLS>
			auto propogate_to_required_by(DG::Node::ID dg_node_id) -> void;

			EVO_NODISCARD auto analyze_decl_var() -> bool;
			EVO_NODISCARD auto analyze_decl_func() -> bool;
			EVO_NODISCARD auto analyze_decl_alias() -> bool;
			EVO_NODISCARD auto analyze_decl_typedef() -> bool;
			EVO_NODISCARD auto analyze_decl_struct() -> bool;
			EVO_NODISCARD auto analyze_decl_when_cond() -> bool;

			EVO_NODISCARD auto analyze_def_var() -> bool;
			EVO_NODISCARD auto analyze_def_func() -> bool;
			EVO_NODISCARD auto analyze_def_alias() -> bool;
			EVO_NODISCARD auto analyze_def_typedef() -> bool;
			EVO_NODISCARD auto analyze_def_struct() -> bool;
	
		private:
			class Context& context;
			class Source& source;
			DG::Node::ID node_id;
			DG::Node& node;
	};



	inline auto analyze_semantics_decl(class Context& context, DG::Node::ID dg_node_id) -> bool {
		return SemanticAnalyzer(context, dg_node_id).analyzeDecl();
	}

	inline auto analyze_semantics_def(class Context& context, DG::Node::ID dg_node_id) -> bool {
		return SemanticAnalyzer(context, dg_node_id).analyzeDef();
	}

	inline auto analyze_semantics_decl_def(class Context& context, DG::Node::ID dg_node_id) -> bool {
		auto sema = SemanticAnalyzer(context, dg_node_id);
		if(sema.analyzeDecl() == false){ return false; }
		return sema.analyzeDef();	
	}


}