////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./SymbolProcManager.h"



#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

namespace pcit::panther{


	SymbolProcManager::SymbolProcManager(){
		this->builtin_symbol_kind_lookup.reserve(this->builtin_symbols.size());

		this->builtin_symbol_kind_lookup.emplace(
			"array_ref.Iterable.createIterator", constevalLookupBuiltinSymbolKind("array_ref.Iterable.createIterator")
		);
	}





	#if defined(PCIT_CONFIG_DEBUG)

		static auto print_field(std::string_view field_name, auto&& value) -> void {
			evo::printBlue("\t{}", field_name);
			evo::printGray(": ");
			evo::printlnMagenta("{}", value);
		}


		static auto print_field_gray(std::string_view field_name, auto&& value) -> void {
			evo::printBlue("\t{}", field_name);
			evo::printlnGray(": {}", value);
		}




		auto SymbolProcManager::debug_dump() -> void {
			evo::printlnGreen("SymbolProcManager debug dump:");
			evo::printlnGray("-----------------------------");

			evo::println(
				"{}/{} symbols were completed ({} not completed, {} suspended)",
				this->numProcs() - this->numProcsNotDone(),
				this->numProcs(),
				this->numProcsNotDone(),
				this->numProcsSuspended()
			);

			evo::println();

			for(size_t i = 0; const SymbolProc& symbol_proc : this->iterSymbolProcs()){
				EVO_DEFER([&](){ i += 1; });


				evo::printCyan("Symbol Proc {} ", i);
				if(symbol_proc.ident.empty()){
					evo::printlnGray("(UNNAMED):");
				}else{
					evo::printMagenta("\"{}\"", symbol_proc.ident);
					evo::printlnGray(":");
				}


				if(symbol_proc.parent == nullptr){
					print_field_gray("Parent", "(NONE)");
				}else{
					if(symbol_proc.parent->ident.empty()){
						print_field("Parent", "(UNNAMED)");
					}else{
						print_field("Parent", std::format("\"{}\"", symbol_proc.parent->ident));
					}
				}

				switch(symbol_proc.ast_node.kind()){
					break; case AST::Kind::VAR_DEF:                print_field("AST Node", "VAR_DEF");
					break; case AST::Kind::FUNC_DEF:               print_field("AST Node", "FUNC_DEF");
					break; case AST::Kind::DELETED_SPECIAL_METHOD: print_field("AST Node", "DELETED_SPECIAL_METHOD");
					break; case AST::Kind::ALIAS_DEF:              print_field("AST Node", "ALIAS_DEF");
					break; case AST::Kind::DISTINCT_ALIAS_DEF:     print_field("AST Node", "DISTINCT_ALIAS_DEF");
					break; case AST::Kind::STRUCT_DEF:             print_field("AST Node", "STRUCT_DEF");
					break; case AST::Kind::UNION_DEF:              print_field("AST Node", "UNION_DEF");
					break; case AST::Kind::ENUM_DEF:               print_field("AST Node", "ENUM_DEF");
					break; case AST::Kind::INTERFACE_DEF:          print_field("AST Node", "INTERFACE_DEF");
					break; case AST::Kind::INTERFACE_IMPL:         print_field("AST Node", "INTERFACE_IMPL");
					break; case AST::Kind::WHEN_CONDITIONAL:       print_field("AST Node", "WHEN_CONDITIONAL");
					break; case AST::Kind::FUNC_CALL:              print_field("AST Node", "FUNC_CALL");
					break; default: evo::printlnRed("\nAST Node: {{UNKNOWN}}");
				}

				print_field("Is Local Symbol", symbol_proc.is_local_symbol);

				evo::println();

				symbol_proc.extra_info.visit([&](const auto& extra_info) -> void {
					using ExtraInfoType = std::decay_t<decltype(extra_info)>;

					if constexpr(std::is_same<ExtraInfoType, std::monostate>()){
						print_field_gray("Extra Info", "(NONE)");

					}else if constexpr(std::is_same<ExtraInfoType, SymbolProc::NonLocalVarInfo>()){
						print_field("Extra Info", "NonLocalVar");
						
					}else if constexpr(std::is_same<ExtraInfoType, SymbolProc::WhenCondInfo>()){
						print_field("Extra Info", "WhenCond");
						
					}else if constexpr(std::is_same<ExtraInfoType, SymbolProc::AliasInfo>()){
						print_field("Extra Info", "Alias");
						
					}else if constexpr(std::is_same<ExtraInfoType, SymbolProc::StructInfo>()){
						print_field("Extra Info", "Struct");
						
					}else if constexpr(std::is_same<ExtraInfoType, SymbolProc::UnionInfo>()){
						print_field("Extra Info", "Union");
						
					}else if constexpr(std::is_same<ExtraInfoType, SymbolProc::EnumInfo>()){
						print_field("Extra Info", "Enum");
						
					}else if constexpr(std::is_same<ExtraInfoType, SymbolProc::FuncInfo>()){
						print_field("Extra Info", "Func");
						
					}else if constexpr(std::is_same<ExtraInfoType, SymbolProc::TemplateFuncInfo>()){
						print_field("Extra Info", "TemplateFunc");
						
					}else if constexpr(std::is_same<ExtraInfoType, SymbolProc::InterfaceImplInfo>()){
						print_field("Extra Info", "InterfaceImpl");
						
					}else{
						static_assert(false, "Unknown SymboProc extra info type");
					}
				});

				evo::println();
					
				switch(symbol_proc.status.load()){
					case SymbolProc::Status::WAITING: {
						print_field("Status", "WAITING");
					} break;

					case SymbolProc::Status::SUSPENDED: {
						print_field("Status", "SUSPENDED");
					} break;

					case SymbolProc::Status::IN_DEF_DEDUCER_IMPL_METHOD: {
						print_field("Status", "IN_DEF_DEDUCER_IMPL_METHOD");
					} break;

					case SymbolProc::Status::IN_QUEUE: {
						print_field("Status", "IN_QUEUE");
					} break;

					case SymbolProc::Status::WORKING: {
						print_field("Status", "WORKING");
					} break;

					case SymbolProc::Status::PASSED_ON_BY_WHEN_COND: {
						print_field("Status", "PASSED_ON_BY_WHEN_COND");
					} break;

					case SymbolProc::Status::ERRORED: {
						print_field("Status", "ERRORED");
					} break;

					case SymbolProc::Status::DONE: {
						print_field("Status", "DONE");
					} break;
				}

				if(symbol_proc.isTemplateSubSymbol()){
					print_field_gray("Instruction Index", "(TEMPLATE SUB SYMBOL)");
				}else{
					print_field(
						"Instruction Index", 
						std::format("{}/{}", symbol_proc.inst_index, symbol_proc.instructions.size())
					);
				}

				print_field("Decl Done", symbol_proc.decl_done.load());
				print_field("Def Done", symbol_proc.def_done.load());
				print_field("PIR Decl Done", symbol_proc.pir_decl_done.load());
				print_field("PIR Def Done", symbol_proc.pir_def_done.load());

				evo::println();


				if(symbol_proc.waiting_for.empty()){
					if(symbol_proc.is_local_symbol){
						print_field_gray("Waiting For", "NONE (is local symbol)");
					}else{
						print_field_gray("Waiting For", "NONE");
					}
					
				}else{
					print_field("Waiting For", symbol_proc.waiting_for.size());

					for(SymbolProc::ID waiting_for_id : symbol_proc.waiting_for){
						const SymbolProc& waiting_for = this->getSymbolProc(waiting_for_id);

						if(waiting_for.ident.empty()){
							evo::printlnGray("\t\t{}: (UNNAMED)", waiting_for_id.get());
						}else{
							evo::printlnGray("\t\t{}: \"{}\"", waiting_for_id.get(), waiting_for.ident);
						}
					}
				}


				if(symbol_proc.decl_waited_on_by.empty()){
					print_field_gray("Decl Waited On By", "NONE");
					
				}else{
					print_field("Decl Waited On By", symbol_proc.decl_waited_on_by.size());

					for(SymbolProc::ID decl_waited_on_by_id : symbol_proc.decl_waited_on_by){
						const SymbolProc& decl_waited_on_by = this->getSymbolProc(decl_waited_on_by_id);

						if(decl_waited_on_by.ident.empty()){
							evo::printlnGray("\t\t{}: (UNNAMED)", decl_waited_on_by_id.get());
						}else{
							evo::printlnGray("\t\t{}: \"{}\"", decl_waited_on_by_id.get(), decl_waited_on_by.ident);
						}
					}
				}


				if(symbol_proc.pir_decl_waited_on_by.empty()){
					print_field_gray("PIR Decl Waited On By", "NONE");
					
				}else{
					print_field("PIR Decl Waited On By", symbol_proc.pir_decl_waited_on_by.size());

					for(SymbolProc::ID pir_decl_waited_on_by_id : symbol_proc.pir_decl_waited_on_by){
						const SymbolProc& pir_decl_waited_on_by = this->getSymbolProc(pir_decl_waited_on_by_id);

						if(pir_decl_waited_on_by.ident.empty()){
							evo::printlnGray("\t\t{}: (UNNAMED)", pir_decl_waited_on_by_id.get());
						}else{
							evo::printlnGray(
								"\t\t{}: \"{}\"", pir_decl_waited_on_by_id.get(), pir_decl_waited_on_by.ident
							);
						}
					}
				}


				if(symbol_proc.def_waited_on_by.empty()){
					print_field_gray("Def Waited On By", "NONE");
					
				}else{
					print_field("Def Waited On By", symbol_proc.def_waited_on_by.size());

					for(SymbolProc::ID def_waited_on_by_id : symbol_proc.def_waited_on_by){
						const SymbolProc& def_waited_on_by = this->getSymbolProc(def_waited_on_by_id);

						if(def_waited_on_by.ident.empty()){
							evo::printlnGray("\t\t{}: (UNNAMED)", def_waited_on_by_id.get());
						}else{
							evo::printlnGray("\t\t{}: \"{}\"", def_waited_on_by_id.get(), def_waited_on_by.ident);
						}
					}
				}


				if(symbol_proc.pir_def_waited_on_by.empty()){
					print_field_gray("PIR Def Waited On By", "NONE");
					
				}else{
					print_field("PIR Def Waited On By", symbol_proc.pir_def_waited_on_by.size());

					for(SymbolProc::ID pir_def_waited_on_by_id : symbol_proc.pir_def_waited_on_by){
						const SymbolProc& pir_def_waited_on_by = this->getSymbolProc(pir_def_waited_on_by_id);

						if(pir_def_waited_on_by.ident.empty()){
							evo::printlnGray("\t\t{}: (UNNAMED)", pir_def_waited_on_by_id.get());
						}else{
							evo::printlnGray(
								"\t\t{}: \"{}\"", pir_def_waited_on_by_id.get(), pir_def_waited_on_by.ident
							);
						}
					}
				}

				evo::println();
			}
		}

	#endif

}