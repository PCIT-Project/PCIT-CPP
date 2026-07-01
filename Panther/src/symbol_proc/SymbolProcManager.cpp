////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./SymbolProcManager.hpp"

#include "../../include/Context.hpp"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

namespace pcit::panther{


	SymbolProcManager::SymbolProcManager(){
		this->builtin_symbol_kind_lookup.reserve(size_t(SymbolProc::BuiltinSymbolKind::_LAST_) + 1);

		this->builtin_symbol_kind_lookup.emplace(
			"panic", constevalLookupBuiltinSymbolKind("panic")
		);

		this->builtin_symbol_kind_lookup.emplace(
			"array.Iterable", constevalLookupBuiltinSymbolKind("array.Iterable")
		);
		this->builtin_symbol_kind_lookup.emplace(
			"array.IterableRT", constevalLookupBuiltinSymbolKind("array.IterableRT")
		);
		this->builtin_symbol_kind_lookup.emplace(
			"arrayRef.IterableRef", constevalLookupBuiltinSymbolKind("arrayRef.IterableRef")
		);
		this->builtin_symbol_kind_lookup.emplace(
			"arrayRef.IterableRefRT", constevalLookupBuiltinSymbolKind("arrayRef.IterableRefRT")
		);
		this->builtin_symbol_kind_lookup.emplace(
			"arrayMutRef.IterableMutRef", constevalLookupBuiltinSymbolKind("arrayMutRef.IterableMutRef")
		);
		this->builtin_symbol_kind_lookup.emplace(
			"arrayMutRef.IterableMutRefRT", constevalLookupBuiltinSymbolKind("arrayMutRef.IterableMutRefRT")
		);
	}


	auto SymbolProcManager::setBuiltinSymbol(
		SymbolProc::BuiltinSymbolKind kind, SymbolProc::ID symbol_proc_id, Context& context
	) -> evo::Expected<void, SymbolProc::ID> {
		BuiltinSymbolInfo& builtin_symbol = this->builtin_symbols[size_t(kind)];

		const std::optional<SymbolProc::ID> exchange_value = 
			this->builtin_symbols[size_t(kind)].symbol_proc_id.exchange(symbol_proc_id);

		if(exchange_value.has_value()){ return evo::Unexpected(*exchange_value); }

		const auto lock = std::scoped_lock(builtin_symbol.waited_on_by_lock);

		if(builtin_symbol.waited_on_by.empty() == false){
			this->num_builtin_symbols_waited_on -= 1;

			for(SymbolProc::ID waited_on_by_id : builtin_symbol.waited_on_by){
				SymbolProc& waited_on_by = this->getSymbolProc(waited_on_by_id);
				if(waited_on_by.hasErroredNoLock()){ continue; }

				if(waited_on_by.status != SymbolProc::Status::WORKING){ // prevent race condition
					waited_on_by.is_waiting_for_builtin = false;
					waited_on_by.setStatusInQueue();
					context.add_task_to_work_manager(waited_on_by_id);
				}
			}

			builtin_symbol.waited_on_by.clear();
		}

		return evo::Expected<void, SymbolProc::ID>();
	}


	auto SymbolProcManager::waitOnSymbolProcOfBuiltinSymbolIfNeeded(
		SymbolProc::BuiltinSymbolKind kind, SymbolProc::ID symbol_proc_id, class Context& context
	) -> bool {
		BuiltinSymbolInfo& builtin_symbol = this->builtin_symbols[size_t(kind)];

		//////////////////
		// try without having to take the lock

		std::optional<SymbolProc::ID> builtin_symbol_proc_id = builtin_symbol.symbol_proc_id.load();
		if(builtin_symbol_proc_id.has_value()){
			SymbolProc& builtin_symbol_proc = this->getSymbolProc(*builtin_symbol_proc_id);

			SymbolProc::WaitOnResult wait_on_result = 
				builtin_symbol_proc.waitOnPIRDefIfNeeded(symbol_proc_id, context);

			switch(wait_on_result){
				case SymbolProc::WaitOnResult::NOT_NEEDED:            return false;
				case SymbolProc::WaitOnResult::WAITING_UNSUSPEND:     evo::debugFatalBreak("Should never be suspended");
				case SymbolProc::WaitOnResult::WAITING:               return true;
				case SymbolProc::WaitOnResult::WAS_ERRORED:           return false;
				case SymbolProc::WaitOnResult::WAS_PASSED_ON_BY_WHEN: return false;
				case SymbolProc::WaitOnResult::CIRCULAR_DEP_DETECTED: return false;
			}
		}


		//////////////////
		// not ready, take the lock

		const auto lock = std::scoped_lock(builtin_symbol.waited_on_by_lock);


		//////////////////
		// try again just in case (prevent race condition)

		builtin_symbol_proc_id = builtin_symbol.symbol_proc_id.load();
		if(builtin_symbol_proc_id.has_value()){
			SymbolProc& builtin_symbol_proc = this->getSymbolProc(*builtin_symbol_proc_id);

			SymbolProc::WaitOnResult wait_on_result = 
				builtin_symbol_proc.waitOnPIRDefIfNeeded(symbol_proc_id, context);

			switch(wait_on_result){
				case SymbolProc::WaitOnResult::NOT_NEEDED:            return false;
				case SymbolProc::WaitOnResult::WAITING_UNSUSPEND:     evo::debugFatalBreak("Should never be suspended");
				case SymbolProc::WaitOnResult::WAITING:               return true;
				case SymbolProc::WaitOnResult::WAS_ERRORED:           return false;
				case SymbolProc::WaitOnResult::WAS_PASSED_ON_BY_WHEN: return false;
				case SymbolProc::WaitOnResult::CIRCULAR_DEP_DETECTED: return false;
			}
		}


		//////////////////
		// not ready, need to wait on

		if(builtin_symbol.waited_on_by.empty()){
			this->num_builtin_symbols_waited_on += 1;
		}

		builtin_symbol.waited_on_by.emplace_back(symbol_proc_id);
		this->getSymbolProc(symbol_proc_id).is_waiting_for_builtin = true;

		return true;
	}






	#if defined(PCIT_CONFIG_DEBUG) || defined(PCIT_BUILD_RELEASE)

		static auto print_field(core::Printer& printer, std::string_view field_name, auto&& value) -> void {
			printer.printBlue("\t{}", field_name);
			printer.printGray(": ");
			printer.printlnMagenta("{}", value);
		}


		static auto print_field_gray(core::Printer& printer, std::string_view field_name, auto&& value) -> void {
			printer.printBlue("\t{}", field_name);
			printer.printlnGray(": {}", value);
		}




		auto SymbolProcManager::debug_dump(bool minimize_done) -> void {
			auto printer = core::Printer::createString(true);


			printer.printlnGreen("SymbolProcManager debug dump:");
			printer.printlnGray("-----------------------------");

			printer.println(
				"{}/{} symbols were completed ({} not completed, {} suspended)",
				this->numProcs() - this->numProcsNotDone(),
				this->numProcs(),
				this->numProcsNotDone(),
				this->numProcsSuspended()
			);

			printer.println();

			for(size_t i = 0; const SymbolProc& symbol_proc : this->iterSymbolProcs()){
				EVO_DEFER([&](){ i += 1; });


				if(
					minimize_done && 
					(
						symbol_proc.status.load() == SymbolProc::Status::DONE
						|| symbol_proc.status.load() == SymbolProc::Status::IN_DEF_DEDUCER_IMPL_METHOD
						|| symbol_proc.status.load() == SymbolProc::Status::PASSED_ON_BY_WHEN
					)
				){
					printer.printGray("Symbol Proc {} ", i);
					if(symbol_proc.ident.empty()){
						printer.printlnGray("(UNNAMED):");
					}else{
						printer.printGray("\"{}\"", symbol_proc.ident);
						printer.printlnGray(":");
					}


					if(symbol_proc.parent == nullptr){
						printer.printlnGray("\tParent: (NONE)");
					}else{
						if(symbol_proc.parent->ident.empty()){
							printer.printlnGray("\tParent: (UNNAMED)");
						}else{
							printer.printlnGray("\tParent: \"{}\"", symbol_proc.parent->ident);
						}
					}

					switch(symbol_proc.ast_node.kind()){
						break; case AST::Kind::VAR_DEF:          printer.printlnGray("\tAST Node: VAR_DEF");
						break; case AST::Kind::FUNC_DEF:         printer.printlnGray("\tAST Node: FUNC_DEF");
						break; case AST::Kind::DELETED_SPECIAL_METHOD:
							printer.printlnGray("\tAST Node: DELETED_SPECIAL_METHOD");
						break; case AST::Kind::FUNC_ALIAS_DEF:   printer.printlnGray("\tAST Node: FUNC_ALIAS_DEF");
						break; case AST::Kind::ALIAS_DEF:        printer.printlnGray("\tAST Node: ALIAS_DEF");
						break; case AST::Kind::STRUCT_DEF:       printer.printlnGray("\tAST Node: STRUCT_DEF");
						break; case AST::Kind::UNION_DEF:        printer.printlnGray("\tAST Node: UNION_DEF");
						break; case AST::Kind::ENUM_DEF:         printer.printlnGray("\tAST Node: ENUM_DEF");
						break; case AST::Kind::INTERFACE_DEF:    printer.printlnGray("\tAST Node: INTERFACE_DEF");
						break; case AST::Kind::INTERFACE_IMPL:   printer.printlnGray("\tAST Node: INTERFACE_IMPL");
						break; case AST::Kind::WHEN_CONDITIONAL: printer.printlnGray("\tAST Node: WHEN_CONDITIONAL");
						break; case AST::Kind::WHEN_SWITCH:      printer.printlnGray("\tAST Node: WHEN_SWITCH");
						break; case AST::Kind::FUNC_CALL:        printer.printlnGray("\tAST Node: FUNC_CALL");
						break; default: 
							printer.printlnRed(
								"\nAST Node: {{UNKNOWN ({})}}", evo::to_underlying(symbol_proc.ast_node.kind())
							);
					}

					switch(symbol_proc.status.load()){
						break; case SymbolProc::Status::DONE: printer.printlnGray("\tStatus: DONE");

						break; case SymbolProc::Status::IN_DEF_DEDUCER_IMPL_METHOD:
							printer.printlnGray("\tStatus: IN_DEF_DEDUCER_IMPL_METHOD");

						break; case SymbolProc::Status::PASSED_ON_BY_WHEN:
							printer.printlnGray("\tStatus: PASSED_ON_BY_WHEN");

						break; default: printer.printlnRed("\tStatus: UNKNOWN MINIMIZED");
					}
					

					printer.println();
					continue;
				}


				printer.printCyan("Symbol Proc {} ", i);
				if(symbol_proc.ident.empty()){
					printer.printlnGray("(UNNAMED):");
				}else{
					printer.printMagenta("\"{}\"", symbol_proc.ident);
					printer.printlnGray(":");
				}


				if(symbol_proc.parent == nullptr){
					print_field_gray(printer, "Parent", "(NONE)");
				}else{
					if(symbol_proc.parent->ident.empty()){
						print_field(printer, "Parent", "(UNNAMED)");
					}else{
						print_field(printer, "Parent", std::format("\"{}\"", symbol_proc.parent->ident));
					}
				}

				switch(symbol_proc.ast_node.kind()){
					break; case AST::Kind::VAR_DEF:          print_field(printer, "AST Node", "VAR_DEF");
					break; case AST::Kind::FUNC_DEF:         print_field(printer, "AST Node", "FUNC_DEF");
					break; case AST::Kind::DELETED_SPECIAL_METHOD:
						print_field(printer, "AST Node", "DELETED_SPECIAL_METHOD");
					break; case AST::Kind::FUNC_ALIAS_DEF:   print_field(printer, "AST Node", "FUNC_ALIAS_DEF");
					break; case AST::Kind::ALIAS_DEF:        print_field(printer, "AST Node", "ALIAS_DEF");
					break; case AST::Kind::STRUCT_DEF:       print_field(printer, "AST Node", "STRUCT_DEF");
					break; case AST::Kind::UNION_DEF:        print_field(printer, "AST Node", "UNION_DEF");
					break; case AST::Kind::ENUM_DEF:         print_field(printer, "AST Node", "ENUM_DEF");
					break; case AST::Kind::INTERFACE_DEF:    print_field(printer, "AST Node", "INTERFACE_DEF");
					break; case AST::Kind::INTERFACE_IMPL:   print_field(printer, "AST Node", "INTERFACE_IMPL");
					break; case AST::Kind::WHEN_CONDITIONAL: print_field(printer, "AST Node", "WHEN_CONDITIONAL");
					break; case AST::Kind::WHEN_SWITCH:      print_field(printer, "AST Node", "WHEN_SWITCH");
					break; case AST::Kind::FUNC_CALL:        print_field(printer, "AST Node", "FUNC_CALL");
					break; default:
						printer.printlnRed(
							"\nAST Node: {{UNKNOWN ({})}}", evo::to_underlying(symbol_proc.ast_node.kind())
						);
				}

				print_field(printer, "Is Local Symbol", symbol_proc.is_local_symbol);

				symbol_proc.extra_info.visit([&](const auto& extra_info) -> void {
					using ExtraInfoType = std::decay_t<decltype(extra_info)>;

					if constexpr(std::is_same<ExtraInfoType, std::monostate>()){
						print_field_gray(printer, "Extra Info", "(NONE)");

					}else if constexpr(std::is_same<ExtraInfoType, SymbolProc::NonLocalVarInfo>()){
						print_field(printer, "Extra Info", "NonLocalVar");
						
					}else if constexpr(std::is_same<ExtraInfoType, SymbolProc::WhenCondInfo>()){
						print_field(printer, "Extra Info", "WhenCond");

					}else if constexpr(std::is_same<ExtraInfoType, SymbolProc::WhenSwitchInfo>()){
						print_field(printer, "Extra Info", "WhenSwitch");
						
					}else if constexpr(std::is_same<ExtraInfoType, SymbolProc::AliasInfo>()){
						print_field(printer, "Extra Info", "Alias");
						
					}else if constexpr(std::is_same<ExtraInfoType, SymbolProc::StructInfo>()){
						print_field(printer, "Extra Info", "Struct");
						
					}else if constexpr(std::is_same<ExtraInfoType, SymbolProc::UnionInfo>()){
						print_field(printer, "Extra Info", "Union");
						
					}else if constexpr(std::is_same<ExtraInfoType, SymbolProc::EnumInfo>()){
						print_field(printer, "Extra Info", "Enum");
						
					}else if constexpr(std::is_same<ExtraInfoType, SymbolProc::FuncInfo>()){
						print_field(printer, "Extra Info", "Func");
						
					}else if constexpr(std::is_same<ExtraInfoType, SymbolProc::TemplateFuncInfo>()){
						print_field(printer, "Extra Info", "TemplateFunc");
						
					}else if constexpr(std::is_same<ExtraInfoType, SymbolProc::InterfaceImplInfo>()){
						print_field(printer, "Extra Info", "InterfaceImpl");
						
					}else{
						static_assert(false, "Unknown SymboProc extra info type");
					}
				});

				printer.println();
					
				switch(symbol_proc.status.load()){
					case SymbolProc::Status::WAITING: {
						print_field(printer, "Status", "WAITING");
					} break;

					case SymbolProc::Status::SUSPENDED: {
						print_field(printer, "Status", "SUSPENDED");
					} break;

					case SymbolProc::Status::IN_DEF_DEDUCER_IMPL_METHOD: {
						print_field(printer, "Status", "IN_DEF_DEDUCER_IMPL_METHOD");
					} break;

					case SymbolProc::Status::IN_QUEUE: {
						print_field(printer, "Status", "IN_QUEUE");
					} break;

					case SymbolProc::Status::WORKING: {
						print_field(printer, "Status", "WORKING");
					} break;

					case SymbolProc::Status::PASSED_ON_BY_WHEN: {
						print_field(printer, "Status", "PASSED_ON_BY_WHEN");
					} break;

					case SymbolProc::Status::ERRORED: {
						print_field(printer, "Status", "ERRORED");
					} break;

					case SymbolProc::Status::DONE: {
						print_field(printer, "Status", "DONE");
					} break;
				}

				if(symbol_proc.isTemplateSubSymbol()){
					print_field_gray(printer, "Instruction Index", "(TEMPLATE SUB SYMBOL)");
				}else{
					print_field(printer, 
						"Instruction Index", 
						std::format("{}/{}", symbol_proc.inst_index, symbol_proc.instructions.size())
					);
				}

				print_field(printer, "Decl Done", symbol_proc.decl_done.load());
				print_field(printer, "Def Done", symbol_proc.def_done.load());
				print_field(printer, "PIR Decl Done", symbol_proc.pir_decl_done.load());
				print_field(printer, "PIR Def Done", symbol_proc.pir_def_done.load());

				printer.println();


				if(symbol_proc.waiting_for.empty()){
					if(symbol_proc.is_local_symbol){
						print_field_gray(printer, "Waiting For", "NONE (is local symbol)");
					}else{
						print_field_gray(printer, "Waiting For", "NONE");
					}
					
				}else{
					print_field(printer, "Waiting For", symbol_proc.waiting_for.size());

					for(SymbolProc::ID waiting_for_id : symbol_proc.waiting_for){
						const SymbolProc& waiting_for = this->getSymbolProc(waiting_for_id);

						if(waiting_for.ident.empty()){
							printer.printlnGray("\t\t{}: (UNNAMED)", waiting_for_id.get());
						}else{
							printer.printlnGray("\t\t{}: \"{}\"", waiting_for_id.get(), waiting_for.ident);
						}
					}
				}


				if(symbol_proc.decl_waited_on_by.empty()){
					print_field_gray(printer, "Decl Waited On By", "NONE");
					
				}else{
					print_field(printer, "Decl Waited On By", symbol_proc.decl_waited_on_by.size());

					for(SymbolProc::ID decl_waited_on_by_id : symbol_proc.decl_waited_on_by){
						const SymbolProc& decl_waited_on_by = this->getSymbolProc(decl_waited_on_by_id);

						if(decl_waited_on_by.ident.empty()){
							printer.printlnGray("\t\t{}: (UNNAMED)", decl_waited_on_by_id.get());
						}else{
							printer.printlnGray("\t\t{}: \"{}\"", decl_waited_on_by_id.get(), decl_waited_on_by.ident);
						}
					}
				}


				if(symbol_proc.pir_decl_waited_on_by.empty()){
					print_field_gray(printer, "PIR Decl Waited On By", "NONE");
					
				}else{
					print_field(printer, "PIR Decl Waited On By", symbol_proc.pir_decl_waited_on_by.size());

					for(SymbolProc::ID pir_decl_waited_on_by_id : symbol_proc.pir_decl_waited_on_by){
						const SymbolProc& pir_decl_waited_on_by = this->getSymbolProc(pir_decl_waited_on_by_id);

						if(pir_decl_waited_on_by.ident.empty()){
							printer.printlnGray("\t\t{}: (UNNAMED)", pir_decl_waited_on_by_id.get());
						}else{
							printer.printlnGray(
								"\t\t{}: \"{}\"", pir_decl_waited_on_by_id.get(), pir_decl_waited_on_by.ident
							);
						}
					}
				}


				if(symbol_proc.def_waited_on_by.empty()){
					print_field_gray(printer, "Def Waited On By", "NONE");
					
				}else{
					print_field(printer, "Def Waited On By", symbol_proc.def_waited_on_by.size());

					for(SymbolProc::ID def_waited_on_by_id : symbol_proc.def_waited_on_by){
						const SymbolProc& def_waited_on_by = this->getSymbolProc(def_waited_on_by_id);

						if(def_waited_on_by.ident.empty()){
							printer.printlnGray("\t\t{}: (UNNAMED)", def_waited_on_by_id.get());
						}else{
							printer.printlnGray("\t\t{}: \"{}\"", def_waited_on_by_id.get(), def_waited_on_by.ident);
						}
					}
				}


				if(symbol_proc.pir_def_waited_on_by.empty()){
					print_field_gray(printer, "PIR Def Waited On By", "NONE");
					
				}else{
					print_field(printer, "PIR Def Waited On By", symbol_proc.pir_def_waited_on_by.size());

					for(SymbolProc::ID pir_def_waited_on_by_id : symbol_proc.pir_def_waited_on_by){
						const SymbolProc& pir_def_waited_on_by = this->getSymbolProc(pir_def_waited_on_by_id);

						if(pir_def_waited_on_by.ident.empty()){
							printer.printlnGray("\t\t{}: (UNNAMED)", pir_def_waited_on_by_id.get());
						}else{
							printer.printlnGray(
								"\t\t{}: \"{}\"", pir_def_waited_on_by_id.get(), pir_def_waited_on_by.ident
							);
						}
					}
				}

				printer.println();
			}

			evo::print(printer.getString());
		}



		auto SymbolProcManager::debug_get_symbol(uint32_t id) const -> const SymbolProc& {
			return this->symbol_procs[SymbolProc::ID(id)];
		};

	#endif

}