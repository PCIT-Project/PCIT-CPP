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

#include "./SymbolProc.h"
#include "../sema/sema_ids.h"


namespace pcit::panther{

	
	class SymbolProcManager{
		public:
			using Instruction = SymbolProc::Instruction;

		public:
			SymbolProcManager();
			~SymbolProcManager() = default;


			EVO_NODISCARD auto getSymbolProc(SymbolProc::ID id) const -> const SymbolProc& {
				return this->symbol_procs[id];
			};

			EVO_NODISCARD auto getSymbolProc(SymbolProc::ID id) -> SymbolProc& {
				return this->symbol_procs[id];
			};

			using SymbolProcIter = evo::IterRange<core::SyncLinearStepAlloc<SymbolProc, SymbolProc::ID>::ConstIter>;
			EVO_NODISCARD auto iterSymbolProcs() -> SymbolProcIter {
				return evo::IterRange(this->symbol_procs.cbegin(), this->symbol_procs.cend());
			}


			EVO_NODISCARD auto allProcsDone() const -> bool {
				return this->num_procs_not_done.load() - this->num_procs_suspended.load() == 0;
			}
			EVO_NODISCARD auto notAllProcsDone() const -> bool { return !this->allProcsDone(); }

			EVO_NODISCARD auto numProcsNotDone() const -> size_t { return this->num_procs_not_done.load(); }
			EVO_NODISCARD auto numProcsSuspended() const -> size_t { return this->num_procs_suspended.load(); }
			EVO_NODISCARD auto numProcs() const -> size_t { return this->symbol_procs.size(); }

			EVO_NODISCARD auto numBuiltinSymbolsWaitedOn() const -> size_t {
				return this->num_builtin_symbols_waited_on.load();
			}


			auto addTypeSymbolProc(TypeInfo::ID type_info_id, SymbolProc::ID symbol_proc_id) -> void {
				const auto lock = std::scoped_lock(this->type_symbol_procs_lock);
				this->type_symbol_procs.emplace(type_info_id, symbol_proc_id);
			}

			EVO_NODISCARD auto getTypeSymbolProc(TypeInfo::ID type_info_id) const -> std::optional<SymbolProc::ID> {
				const auto lock = std::scoped_lock(this->type_symbol_procs_lock);
				const std::unordered_map<TypeInfo::ID, SymbolProc::ID>::const_iterator find =
					this->type_symbol_procs.find(type_info_id);

				if(find != this->type_symbol_procs.end()){ return find->second; }
				return std::nullopt;
			}


			///////////////////////////////////
			// instructions


			//////////////////
			// SuspendSymbolProc

			EVO_NODISCARD auto createSuspendSymbolProc() -> Instruction {
				return Instruction(Instruction::Kind::SUSPEND_SYMBOL_PROC, 0);
			}




			//////////////////
			// NonLocalVarDecl

			EVO_NODISCARD auto createNonLocalVarDecl(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::NON_LOCAL_VAR_DECL,
					this->non_local_var_decls.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getNonLocalVarDecl(Instruction instr) const -> const Instruction::NonLocalVarDecl& {
				evo::debugAssert(instr.kind() == Instruction::Kind::NON_LOCAL_VAR_DECL, "Not a NonLocalVarDecl");
				return this->non_local_var_decls[instr._index];
			}



			//////////////////
			// NonLocalVarDef

			EVO_NODISCARD auto createNonLocalVarDef(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::NON_LOCAL_VAR_DEF,
					this->non_local_var_defs.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getNonLocalVarDef(Instruction instr) const -> const Instruction::NonLocalVarDef& {
				evo::debugAssert(instr.kind() == Instruction::Kind::NON_LOCAL_VAR_DEF, "Not a NonLocalVarDef");
				return this->non_local_var_defs[instr._index];
			}



			//////////////////
			// NonLocalVarDeclDef

			EVO_NODISCARD auto createNonLocalVarDeclDef(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::NON_LOCAL_VAR_DECL_DEF,
					this->non_local_var_decl_defs.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getNonLocalVarDeclDef(Instruction instr) const
			-> const Instruction::NonLocalVarDeclDef& {
				evo::debugAssert(instr.kind() == Instruction::Kind::NON_LOCAL_VAR_DECL_DEF, "Not a NonLocalVarDeclDef");
				return this->non_local_var_decl_defs[instr._index];
			}



			//////////////////
			// WhenCond

			EVO_NODISCARD auto createWhenCond(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::WHEN_COND,
					this->when_conds.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getWhenCond(Instruction instr) const -> const Instruction::WhenCond& {
				evo::debugAssert(instr.kind() == Instruction::Kind::WHEN_COND, "Not a WhenCond");
				return this->when_conds[instr._index];
			}



			//////////////////
			// Alias

			EVO_NODISCARD auto createAlias(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::ALIAS, this->aliases.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getAlias(Instruction instr) const -> const Instruction::Alias& {
				evo::debugAssert(instr.kind() == Instruction::Kind::ALIAS, "Not a Alias");
				return this->aliases[instr._index];
			}



			//////////////////
			// StructDecl<true>

			EVO_NODISCARD auto createStructDeclInstatiation(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::STRUCT_DECL_INSTANTIATION,
					this->struct_decl_instantiations.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getStructDeclInstatiation(Instruction instr) const
			-> const Instruction::StructDecl<true>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::STRUCT_DECL_INSTANTIATION, "Not a StructDecl<true>"
				);
				return this->struct_decl_instantiations[instr._index];
			}



			//////////////////
			// StructDecl<false>

			EVO_NODISCARD auto createStructDecl(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::STRUCT_DECL,
					this->struct_decls.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getStructDecl(Instruction instr) const -> const Instruction::StructDecl<false>& {
				evo::debugAssert(instr.kind() == Instruction::Kind::STRUCT_DECL, "Not a StructDecl<false>");
				return this->struct_decls[instr._index];
			}



			//////////////////
			// StructDef

			EVO_NODISCARD auto createStructDef() -> Instruction {
				return Instruction(Instruction::Kind::STRUCT_DEF, 0);
			}


			//////////////////
			// StructCreatedSpecialMembersPIRIfNeeded

			EVO_NODISCARD auto createStructCreatedSpecialMembersPIRIfNeeded() -> Instruction {
				return Instruction(Instruction::Kind::STRUCT_CREATED_SPECIAL_MEMBERS_PIR_IF_NEEDED, 0);
			}



			//////////////////
			// TemplateStruct

			EVO_NODISCARD auto createTemplateStruct(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::TEMPLATE_STRUCT,
					this->template_structs.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getTemplateStruct(Instruction instr) const -> const Instruction::TemplateStruct& {
				evo::debugAssert(instr.kind() == Instruction::Kind::TEMPLATE_STRUCT, "Not a TemplateStruct");
				return this->template_structs[instr._index];
			}



			//////////////////
			// UnionDecl

			EVO_NODISCARD auto createUnionDecl(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::UNION_DECL,
					this->union_decls.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getUnionDecl(Instruction instr) const -> const Instruction::UnionDecl& {
				evo::debugAssert(instr.kind() == Instruction::Kind::UNION_DECL, "Not a UnionDecl");
				return this->union_decls[instr._index];
			}



			//////////////////
			// UnionAddFields

			EVO_NODISCARD auto createUnionAddFields(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::UNION_ADD_FIELDS,
					this->union_add_fieldss.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getUnionAddFields(Instruction instr) const -> const Instruction::UnionAddFields& {
				evo::debugAssert(instr.kind() == Instruction::Kind::UNION_ADD_FIELDS, "Not a UnionAddFields");
				return this->union_add_fieldss[instr._index];
			}



			//////////////////
			// UnionDef

			EVO_NODISCARD auto createUnionDef() -> Instruction {
				return Instruction(Instruction::Kind::UNION_DEF, 0);
			}



			//////////////////
			// EnumDecl

			EVO_NODISCARD auto createEnumDecl(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::ENUM_DECL,
					this->enum_decls.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getEnumDecl(Instruction instr) const -> const Instruction::EnumDecl& {
				evo::debugAssert(instr.kind() == Instruction::Kind::ENUM_DECL, "Not a EnumDecl");
				return this->enum_decls[instr._index];
			}



			//////////////////
			// EnumAddEnumerators

			EVO_NODISCARD auto createEnumAddEnumerators(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::ENUM_ADD_ENUMERATORS,
					this->enum_add_enumeratorss.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getEnumAddEnumerators(Instruction instr) const
			-> const Instruction::EnumAddEnumerators& {
				evo::debugAssert(instr.kind() == Instruction::Kind::ENUM_ADD_ENUMERATORS, "Not a EnumAddEnumerators");
				return this->enum_add_enumeratorss[instr._index];
			}



			//////////////////
			// EnumDef

			EVO_NODISCARD auto createEnumDef() -> Instruction {
				return Instruction(Instruction::Kind::ENUM_DEF, 0);
			}



			//////////////////
			// FuncDeclExtractDeducers

			EVO_NODISCARD auto createFuncDeclExtractDeducers(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::FUNC_DECL_EXTRACT_DEDUCERS,
					this->func_decl_extract_deducerss.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getFuncDeclExtractDeducers(Instruction instr) const
			-> const Instruction::FuncDeclExtractDeducers& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::FUNC_DECL_EXTRACT_DEDUCERS, "Not a FuncDeclExtractDeducers"
				);
				return this->func_decl_extract_deducerss[instr._index];
			}



			//////////////////
			// FuncDecl<true>

			EVO_NODISCARD auto createFuncDeclInstantiation(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::FUNC_DECL_INSTANTIATION,
					this->func_decl_instantiations.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getFuncDeclInstantiation(Instruction instr) const -> const Instruction::FuncDecl<true>& {
				evo::debugAssert(instr.kind() == Instruction::Kind::FUNC_DECL_INSTANTIATION, "Not a FuncDecl<true>");
				return this->func_decl_instantiations[instr._index];
			}



			//////////////////
			// FuncDecl<false>

			EVO_NODISCARD auto createFuncDecl(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::FUNC_DECL,
					this->func_decls.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getFuncDecl(Instruction instr) const -> const Instruction::FuncDecl<false>& {
				evo::debugAssert(instr.kind() == Instruction::Kind::FUNC_DECL, "Not a FuncDecl<false>");
				return this->func_decls[instr._index];
			}



			//////////////////
			// FuncPreBody

			EVO_NODISCARD auto createFuncPreBody(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::FUNC_PRE_BODY,
					this->func_pre_bodys.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getFuncPreBody(Instruction instr) const -> const Instruction::FuncPreBody& {
				evo::debugAssert(instr.kind() == Instruction::Kind::FUNC_PRE_BODY, "Not a FuncPreBody");
				return this->func_pre_bodys[instr._index];
			}



			//////////////////
			// FuncDef

			EVO_NODISCARD auto createFuncDef(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::FUNC_DEF,
					this->func_defs.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getFuncDef(Instruction instr) const -> const Instruction::FuncDef& {
				evo::debugAssert(instr.kind() == Instruction::Kind::FUNC_DEF, "Not a FuncDef");
				return this->func_defs[instr._index];
			}



			//////////////////
			// FuncPrepareComptimePIRIfNeeded

			EVO_NODISCARD auto createFuncPrepareComptimePIRIfNeeded(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::FUNC_PREPARE_COMPTIME_PIR_IF_NEEDED,
					this->func_prepare_comptime_pir_if_neededs.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getFuncPrepareComptimePIRIfNeeded(Instruction instr) const
			-> const Instruction::FuncPrepareComptimePIRIfNeeded& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::FUNC_PREPARE_COMPTIME_PIR_IF_NEEDED,
					"Not a FuncPrepareComptimePIRIfNeeded"
				);
				return this->func_prepare_comptime_pir_if_neededs[instr._index];
			}



			//////////////////
			// FuncComptimePIRReadyIfNeeded

			EVO_NODISCARD auto createFuncComptimePIRReadyIfNeeded() -> Instruction {
				return Instruction(Instruction::Kind::FUNC_COMPTIME_PIR_READY_IF_NEEDED, 0);
			}


			//////////////////
			// TemplateFuncBegin

			EVO_NODISCARD auto createTemplateFuncBegin(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::TEMPLATE_FUNC_BEGIN,
					this->template_func_begins.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getTemplateFuncBegin(Instruction instr) const -> const Instruction::TemplateFuncBegin& {
				evo::debugAssert(instr.kind() == Instruction::Kind::TEMPLATE_FUNC_BEGIN, "Not a TemplateFuncBegin");
				return this->template_func_begins[instr._index];
			}




			//////////////////
			// TemplateFuncSetParamIsDeducer

			EVO_NODISCARD auto createTemplateFuncSetParamIsDeducer(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::TEMPLATE_FUNC_SET_PARAM_IS_DEDUCER,
					this->template_func_set_param_is_deducers.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getTemplateFuncSetParamIsDeducer(Instruction instr) const
			-> const Instruction::TemplateFuncSetParamIsDeducer& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::TEMPLATE_FUNC_SET_PARAM_IS_DEDUCER,
					"Not a TemplateFuncSetParamIsDeducer"
				);
				return this->template_func_set_param_is_deducers[instr._index];
			}



			//////////////////
			// TemplateFuncEnd

			EVO_NODISCARD auto createTemplateFuncEnd(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::TEMPLATE_FUNC_END,
					this->template_func_ends.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getTemplateFuncEnd(Instruction instr) const -> const Instruction::TemplateFuncEnd& {
				evo::debugAssert(instr.kind() == Instruction::Kind::TEMPLATE_FUNC_END, "Not a TemplateFuncEnd");
				return this->template_func_ends[instr._index];
			}


			//////////////////
			// DeletedSpecialMethod

			EVO_NODISCARD auto createDeletedSpecialMethod(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::DELETED_SPECIAL_METHOD,
					this->deleted_special_methods.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getDeletedSpecialMethod(Instruction instr) const
			-> const Instruction::DeletedSpecialMethod& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::DELETED_SPECIAL_METHOD, "Not a DeletedSpecialMethod"
				);
				return this->deleted_special_methods[instr._index];
			}


			//////////////////
			// FuncAliasDef

			EVO_NODISCARD auto createFuncAliasDef(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::FUNC_ALIAS_DEF,
					this->func_alias_def.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getFuncAliasDef(Instruction instr) const
			-> const Instruction::FuncAliasDef& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::FUNC_ALIAS_DEF, "Not a FuncAliasDef"
				);
				return this->func_alias_def[instr._index];
			}


			//////////////////
			// InterfacePrepare

			EVO_NODISCARD auto createInterfacePrepare(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::INTERFACE_PREPARE,
					this->interface_prepares.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getInterfacePrepare(Instruction instr) const -> const Instruction::InterfacePrepare& {
				evo::debugAssert(instr.kind() == Instruction::Kind::INTERFACE_PREPARE, "Not a InterfacePrepare");
				return this->interface_prepares[instr._index];
			}


			//////////////////
			// InterfaceDecl

			EVO_NODISCARD auto createInterfaceDecl() -> Instruction {
				return Instruction(Instruction::Kind::INTERFACE_DECL, 0);
			}


			//////////////////
			// InterfaceDef

			EVO_NODISCARD auto createInterfaceDef() -> Instruction {
				return Instruction(Instruction::Kind::INTERFACE_DEF, 0);
			}



			//////////////////
			// InterfaceFuncDef

			EVO_NODISCARD auto createInterfaceFuncDef(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::INTERFACE_FUNC_DEF,
					this->interface_func_defs.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getInterfaceFuncDef(Instruction instr) const -> const Instruction::InterfaceFuncDef& {
				evo::debugAssert(instr.kind() == Instruction::Kind::INTERFACE_FUNC_DEF, "Not a InterfaceFuncDef");
				return this->interface_func_defs[instr._index];
			}



			//////////////////
			// InterfaceImplDecl

			EVO_NODISCARD auto createInterfaceImplDecl(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::INTERFACE_IMPL_DECL,
					this->interface_impl_decls.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getInterfaceImplDecl(Instruction instr) const -> const Instruction::InterfaceImplDecl& {
				evo::debugAssert(instr.kind() == Instruction::Kind::INTERFACE_IMPL_DECL, "Not a InterfaceImplDecl");
				return this->interface_impl_decls[instr._index];
			}


			//////////////////
			// InterfaceInDefImplDecl

			EVO_NODISCARD auto createInterfaceInDefImplDecl(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::INTERFACE_IN_DEF_IMPL_DECL,
					this->interface_in_def_impl_decls.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getInterfaceInDefImplDecl(Instruction instr) const
			-> const Instruction::InterfaceInDefImplDecl& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::INTERFACE_IN_DEF_IMPL_DECL, "Not a InterfaceInDefImplDecl"
				);
				return this->interface_in_def_impl_decls[instr._index];
			}


			//////////////////
			// InterfaceDeducerImplInstantiationDecl

			EVO_NODISCARD auto createInterfaceDeducerImplInstantiationDecl(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::INTERFACE_DEDUCER_IMPL_INSTANTIATION_DECL,
					this->interface_deducer_impl_instantiation_decls.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getInterfaceDeducerImplInstantiationDecl(Instruction instr) const
			-> const Instruction::InterfaceDeducerImplInstantiationDecl& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::INTERFACE_DEDUCER_IMPL_INSTANTIATION_DECL,\
					"Not a InterfaceDeducerImplInstantiationDecl"
				);
				return this->interface_deducer_impl_instantiation_decls[instr._index];
			}



			//////////////////
			// InterfaceImplMethodLookup

			EVO_NODISCARD auto createInterfaceImplMethodLookup(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::INTERFACE_IMPL_METHOD_LOOKUP,
					this->interface_impl_method_lookups.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getInterfaceImplMethodLookup(Instruction instr) const
			-> const Instruction::InterfaceImplMethodLookup& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::INTERFACE_IMPL_METHOD_LOOKUP,
					"Not a InterfaceImplMethodLookup"
				);
				return this->interface_impl_method_lookups[instr._index];
			}



			//////////////////
			// InterfaceInDefImplMethod

			EVO_NODISCARD auto createInterfaceInDefImplMethod(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::INTERFACE_IN_DEF_IMPL_METHOD,
					this->interface_in_def_impl_method.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getInterfaceInDefImplMethod(Instruction instr) const
			-> const Instruction::InterfaceInDefImplMethod& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::INTERFACE_IN_DEF_IMPL_METHOD,
					"Not a InterfaceInDefImplMethod"
				);
				return this->interface_in_def_impl_method[instr._index];
			}




			//////////////////
			// InterfaceImplDef

			EVO_NODISCARD auto createInterfaceImplDef(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::INTERFACE_IMPL_DEF,
					this->interface_impl_defs.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getInterfaceImplDef(Instruction instr) const -> const Instruction::InterfaceImplDef& {
				evo::debugAssert(instr.kind() == Instruction::Kind::INTERFACE_IMPL_DEF, "Not a InterfaceImplDef");
				return this->interface_impl_defs[instr._index];
			}



			//////////////////
			// InterfaceImplComptimePIR

			EVO_NODISCARD auto createInterfaceImplComptimePIR() -> Instruction {
				return Instruction(Instruction::Kind::INTERFACE_IMPL_COMPTIME_PIR, 0);
			}



			//////////////////
			// LocalVar

			EVO_NODISCARD auto createLocalVar(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::LOCAL_VAR,
					this->local_vars.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getLocalVar(Instruction instr) const -> const Instruction::LocalVar& {
				evo::debugAssert(instr.kind() == Instruction::Kind::LOCAL_VAR, "Not a LocalVar");
				return this->local_vars[instr._index];
			}


			//////////////////
			// LocalFuncAlias

			EVO_NODISCARD auto createLocalFuncAlias(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::LOCAL_FUNC_ALIAS,
					this->local_func_aliass.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getLocalFuncAlias(Instruction instr) const -> const Instruction::LocalFuncAlias& {
				evo::debugAssert(instr.kind() == Instruction::Kind::LOCAL_FUNC_ALIAS, "Not a LocalFuncAlias");
				return this->local_func_aliass[instr._index];
			}



			//////////////////
			// LocalAlias

			EVO_NODISCARD auto createLocalAlias(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::LOCAL_ALIAS,
					this->local_aliass.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getLocalAlias(Instruction instr) const -> const Instruction::LocalAlias& {
				evo::debugAssert(instr.kind() == Instruction::Kind::LOCAL_ALIAS, "Not a LocalAlias");
				return this->local_aliass[instr._index];
			}



			//////////////////
			// Return

			EVO_NODISCARD auto createReturn(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::RETURN,
					this->returns.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getReturn(Instruction instr) const -> const Instruction::Return& {
				evo::debugAssert(instr.kind() == Instruction::Kind::RETURN, "Not a Return");
				return this->returns[instr._index];
			}



			//////////////////
			// LabeledReturn

			EVO_NODISCARD auto createLabeledReturn(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::LABELED_RETURN,
					this->labeled_returns.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getLabeledReturn(Instruction instr) const -> const Instruction::LabeledReturn& {
				evo::debugAssert(instr.kind() == Instruction::Kind::LABELED_RETURN, "Not a LabeledReturn");
				return this->labeled_returns[instr._index];
			}



			//////////////////
			// Error

			EVO_NODISCARD auto createError(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::ERROR,
					this->errors.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getError(Instruction instr) const -> const Instruction::Error& {
				evo::debugAssert(instr.kind() == Instruction::Kind::ERROR, "Not a Error");
				return this->errors[instr._index];
			}



			//////////////////
			// Unreachable

			EVO_NODISCARD auto createUnreachable(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::UNREACHABLE,
					this->unreachables.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getUnreachable(Instruction instr) const -> const Instruction::Unreachable& {
				evo::debugAssert(instr.kind() == Instruction::Kind::UNREACHABLE, "Not a Unreachable");
				return this->unreachables[instr._index];
			}



			//////////////////
			// Break

			EVO_NODISCARD auto createBreak(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::BREAK,
					this->breaks.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getBreak(Instruction instr) const -> const Instruction::Break& {
				evo::debugAssert(instr.kind() == Instruction::Kind::BREAK, "Not a Break");
				return this->breaks[instr._index];
			}



			//////////////////
			// Continue

			EVO_NODISCARD auto createContinue(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::CONTINUE,
					this->continues.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getContinue(Instruction instr) const -> const Instruction::Continue& {
				evo::debugAssert(instr.kind() == Instruction::Kind::CONTINUE, "Not a Continue");
				return this->continues[instr._index];
			}



			//////////////////
			// Delete

			EVO_NODISCARD auto createDelete(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::DELETE,
					this->deletes.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getDelete(Instruction instr) const -> const Instruction::Delete& {
				evo::debugAssert(instr.kind() == Instruction::Kind::DELETE, "Not a Delete");
				return this->deletes[instr._index];
			}



			//////////////////
			// BeginCond

			EVO_NODISCARD auto createBeginCond(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::BEGIN_COND,
					this->begin_conds.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getBeginCond(Instruction instr) const -> const Instruction::BeginCond& {
				evo::debugAssert(instr.kind() == Instruction::Kind::BEGIN_COND, "Not a BeginCond");
				return this->begin_conds[instr._index];
			}



			//////////////////
			// CondNoElse

			EVO_NODISCARD auto createCondNoElse() -> Instruction {
				return Instruction(Instruction::Kind::COND_NO_ELSE, 0);
			}


			//////////////////
			// CondElse

			EVO_NODISCARD auto createCondElse() -> Instruction {
				return Instruction(Instruction::Kind::COND_ELSE, 0);
			}



			//////////////////
			// CondElseIf

			EVO_NODISCARD auto createCondElseIf() -> Instruction {
				return Instruction(Instruction::Kind::COND_ELSE_IF, 0);
			}


			//////////////////
			// EndCond

			EVO_NODISCARD auto createEndCond() -> Instruction {
				return Instruction(Instruction::Kind::END_COND, 0);
			}


			//////////////////
			// EndCondSet

			EVO_NODISCARD auto createEndCondSet(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::END_COND_SET,
					this->end_cond_sets.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getEndCondSet(Instruction instr) const -> const Instruction::EndCondSet& {
				evo::debugAssert(instr.kind() == Instruction::Kind::END_COND_SET, "Not a EndCondSet");
				return this->end_cond_sets[instr._index];
			}


			//////////////////
			// BeginLocalWhenCond

			EVO_NODISCARD auto createBeginLocalWhenCond(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::BEGIN_LOCAL_WHEN_COND,
					this->begin_local_when_conds.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getBeginLocalWhenCond(Instruction instr) const
			-> const Instruction::BeginLocalWhenCond& {
				evo::debugAssert(instr.kind() == Instruction::Kind::BEGIN_LOCAL_WHEN_COND, "Not a BeginLocalWhenCond");
				return this->begin_local_when_conds[instr._index];
			}



			//////////////////
			// EndLocalWhenCond

			EVO_NODISCARD auto createEndLocalWhenCond(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::END_LOCAL_WHEN_COND,
					this->end_local_when_conds.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getEndLocalWhenCond(Instruction instr) const -> const Instruction::EndLocalWhenCond& {
				evo::debugAssert(instr.kind() == Instruction::Kind::END_LOCAL_WHEN_COND, "Not a EndLocalWhenCond");
				return this->end_local_when_conds[instr._index];
			}



			//////////////////
			// BeginWhile

			EVO_NODISCARD auto createBeginWhile(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::BEGIN_WHILE,
					this->begin_whiles.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getBeginWhile(Instruction instr) const -> const Instruction::BeginWhile& {
				evo::debugAssert(instr.kind() == Instruction::Kind::BEGIN_WHILE, "Not a BeginWhile");
				return this->begin_whiles[instr._index];
			}



			//////////////////
			// EndWhile

			EVO_NODISCARD auto createEndWhile(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::END_WHILE,
					this->end_whiles.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getEndWhile(Instruction instr) const -> const Instruction::EndWhile& {
				evo::debugAssert(instr.kind() == Instruction::Kind::END_WHILE, "Not a EndWhile");
				return this->end_whiles[instr._index];
			}


			//////////////////
			// BeginFor

			EVO_NODISCARD auto createBeginFor(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::BEGIN_FOR,
					this->begin_fors.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getBeginFor(Instruction instr) const -> const Instruction::BeginFor& {
				evo::debugAssert(instr.kind() == Instruction::Kind::BEGIN_FOR, "Not a BeginFor");
				return this->begin_fors[instr._index];
			}



			//////////////////
			// EndFor

			EVO_NODISCARD auto createEndFor(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::END_FOR,
					this->end_fors.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getEndFor(Instruction instr) const -> const Instruction::EndFor& {
				evo::debugAssert(instr.kind() == Instruction::Kind::END_FOR, "Not a EndFor");
				return this->end_fors[instr._index];
			}


			//////////////////
			// BeginForUnroll

			EVO_NODISCARD auto createBeginForUnroll(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::BEGIN_FOR_UNROLL,
					this->begin_for_unrolls.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getBeginForUnroll(Instruction instr) const -> const Instruction::BeginForUnroll& {
				evo::debugAssert(instr.kind() == Instruction::Kind::BEGIN_FOR_UNROLL, "Not a BeginForUnroll");
				return this->begin_for_unrolls[instr._index];
			}


			//////////////////
			// ForUnrollCond

			EVO_NODISCARD auto createForUnrollCond(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::FOR_UNROLL_COND,
					this->for_unroll_conds.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getForUnrollCond(Instruction instr) const -> const Instruction::ForUnrollCond& {
				evo::debugAssert(instr.kind() == Instruction::Kind::FOR_UNROLL_COND, "Not a ForUnrollCond");
				return this->for_unroll_conds[instr._index];
			}


			//////////////////
			// ForUnrollContinue

			EVO_NODISCARD auto createForUnrollContinue(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::FOR_UNROLL_CONTINUE,
					this->for_unroll_continues.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getForUnrollContinue(Instruction instr) const -> const Instruction::ForUnrollContinue& {
				evo::debugAssert(instr.kind() == Instruction::Kind::FOR_UNROLL_CONTINUE, "Not a ForUnrollContinue");
				return this->for_unroll_continues[instr._index];
			}



			//////////////////
			// BeginSwitch

			EVO_NODISCARD auto createBeginSwitch(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::BEGIN_SWITCH,
					this->begin_switches.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getBeginSwitch(Instruction instr) const -> const Instruction::BeginSwitch& {
				evo::debugAssert(instr.kind() == Instruction::Kind::BEGIN_SWITCH, "Not a BeginSwitch");
				return this->begin_switches[instr._index];
			}


			//////////////////
			// BeginCase

			EVO_NODISCARD auto createBeginCase(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::BEGIN_CASE,
					this->begin_cases.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getBeginCase(Instruction instr) const -> const Instruction::BeginCase& {
				evo::debugAssert(instr.kind() == Instruction::Kind::BEGIN_CASE, "Not a BeginCase");
				return this->begin_cases[instr._index];
			}


			//////////////////
			// EndCase

			EVO_NODISCARD auto createEndCase() -> Instruction {
				return Instruction(Instruction::Kind::END_CASE, 0);
			}


			//////////////////
			// EndCase

			EVO_NODISCARD auto createEndSwitch(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::END_SWITCH,
					this->end_switches.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getEndSwitch(Instruction instr) const -> const Instruction::EndSwitch& {
				evo::debugAssert(instr.kind() == Instruction::Kind::END_SWITCH, "Not a EndSwitch");
				return this->end_switches[instr._index];
			}



			//////////////////
			// BeginDefer

			EVO_NODISCARD auto createBeginDefer(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::BEGIN_DEFER,
					this->begin_defers.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getBeginDefer(Instruction instr) const -> const Instruction::BeginDefer& {
				evo::debugAssert(instr.kind() == Instruction::Kind::BEGIN_DEFER, "Not a BeginDefer");
				return this->begin_defers[instr._index];
			}


			//////////////////
			// EndDefer

			EVO_NODISCARD auto createEndDefer(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::END_DEFER,
					this->end_defers.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getEndDefer(Instruction instr) const -> const Instruction::EndDefer& {
				evo::debugAssert(instr.kind() == Instruction::Kind::END_DEFER, "Not an EndDefer");
				return this->end_defers[instr._index];
			}


			//////////////////
			// BeginStmtBlock

			EVO_NODISCARD auto createBeginStmtBlock(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::BEGIN_STMT_BLOCK,
					this->begin_stmt_blocks.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getBeginStmtBlock(Instruction instr) const -> const Instruction::BeginStmtBlock& {
				evo::debugAssert(instr.kind() == Instruction::Kind::BEGIN_STMT_BLOCK, "Not a BeginStmtBlock");
				return this->begin_stmt_blocks[instr._index];
			}


			//////////////////
			// EndStmtBlock

			EVO_NODISCARD auto createEndStmtBlock(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::END_STMT_BLOCK,
					this->end_stmt_blocks.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getEndStmtBlock(Instruction instr) const -> const Instruction::EndStmtBlock& {
				evo::debugAssert(instr.kind() == Instruction::Kind::END_STMT_BLOCK, "Not a EndStmtBlock");
				return this->end_stmt_blocks[instr._index];
			}


			//////////////////
			// FuncCall

			EVO_NODISCARD auto createFuncCall(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::FUNC_CALL,
					this->func_calls.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getFuncCall(Instruction instr) const -> const Instruction::FuncCall& {
				evo::debugAssert(instr.kind() == Instruction::Kind::FUNC_CALL, "Not a FuncCall");
				return this->func_calls[instr._index];
			}



			//////////////////
			// Assignment

			EVO_NODISCARD auto createAssignment(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::ASSIGNMENT,
					this->assignments.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getAssignment(Instruction instr) const -> const Instruction::Assignment& {
				evo::debugAssert(instr.kind() == Instruction::Kind::ASSIGNMENT, "Not a Assignment");
				return this->assignments[instr._index];
			}


			//////////////////
			// AssignmentNew

			EVO_NODISCARD auto createAssignmentNew(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::ASSIGNMENT_NEW,
					this->assignment_news.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getAssignmentNew(Instruction instr) const -> const Instruction::AssignmentNew& {
				evo::debugAssert(instr.kind() == Instruction::Kind::ASSIGNMENT_NEW, "Not a AssignmentNew");
				return this->assignment_news[instr._index];
			}


			//////////////////
			// AssignmentCopy

			EVO_NODISCARD auto createAssignmentCopy(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::ASSIGNMENT_COPY,
					this->assignment_copies.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getAssignmentCopy(Instruction instr) const -> const Instruction::AssignmentCopy& {
				evo::debugAssert(instr.kind() == Instruction::Kind::ASSIGNMENT_COPY, "Not a AssignmentCopy");
				return this->assignment_copies[instr._index];
			}


			//////////////////
			// AssignmentMove

			EVO_NODISCARD auto createAssignmentMove(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::ASSIGNMENT_MOVE,
					this->assignment_moves.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getAssignmentMove(Instruction instr) const -> const Instruction::AssignmentMove& {
				evo::debugAssert(instr.kind() == Instruction::Kind::ASSIGNMENT_MOVE, "Not a AssignmentMove");
				return this->assignment_moves[instr._index];
			}


			//////////////////
			// AssignmentForward

			EVO_NODISCARD auto createAssignmentForward(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::ASSIGNMENT_FORWARD,
					this->assignment_forwards.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getAssignmentForward(Instruction instr) const -> const Instruction::AssignmentForward& {
				evo::debugAssert(instr.kind() == Instruction::Kind::ASSIGNMENT_FORWARD, "Not a AssignmentForward");
				return this->assignment_forwards[instr._index];
			}



			//////////////////
			// MultiAssign

			EVO_NODISCARD auto createMultiAssign(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::MULTI_ASSIGN,
					this->multi_assigns.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getMultiAssign(Instruction instr) const -> const Instruction::MultiAssign& {
				evo::debugAssert(instr.kind() == Instruction::Kind::MULTI_ASSIGN, "Not a MultiAssign");
				return this->multi_assigns[instr._index];
			}



			//////////////////
			// DiscardingAssignment

			EVO_NODISCARD auto createDiscardingAssignment(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::DISCARDING_ASSIGNMENT,
					this->discarding_assignments.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getDiscardingAssignment(Instruction instr) const
			-> const Instruction::DiscardingAssignment& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::DISCARDING_ASSIGNMENT, "Not a DiscardingAssignment"
				);
				return this->discarding_assignments[instr._index];
			}


			//////////////////
			// TryElseBegin

			EVO_NODISCARD auto createTryElseBegin(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::TRY_ELSE_BEGIN,
					this->try_else_begins.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getTryElseBegin(Instruction instr) const
			-> const Instruction::TryElseBegin& {
				evo::debugAssert(instr.kind() == Instruction::Kind::TRY_ELSE_BEGIN, "Not a TryElseBegin");
				return this->try_else_begins[instr._index];
			}


			//////////////////
			// TryElseEnd

			EVO_NODISCARD auto createTryElseEnd() -> Instruction {
				return Instruction(Instruction::Kind::TRY_ELSE_END, 0);
			}



			//////////////////
			// BeginUnsafe

			EVO_NODISCARD auto createBeginUnsafe(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::BEGIN_UNSAFE,
					this->begin_unsafes.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getBeginUnsafe(Instruction instr) const
			-> const Instruction::BeginUnsafe& {
				evo::debugAssert(instr.kind() == Instruction::Kind::BEGIN_UNSAFE, "Not a BeginUnsafe");
				return this->begin_unsafes[instr._index];
			}


			//////////////////
			// EndUnsafe

			EVO_NODISCARD auto createEndUnsafe() -> Instruction {
				return Instruction(Instruction::Kind::END_UNSAFE, 0);
			}




			//////////////////
			// TypeToTerm

			EVO_NODISCARD auto createTypeToTerm(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::TYPE_TO_TERM,
					this->type_to_terms.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getTypeToTerm(Instruction instr) const -> const Instruction::TypeToTerm& {
				evo::debugAssert(instr.kind() == Instruction::Kind::TYPE_TO_TERM, "Not a TypeToTerm");
				return this->type_to_terms[instr._index];
			}



			//////////////////
			// RequireThisDef

			EVO_NODISCARD auto createRequireThisDef() -> Instruction {
				return Instruction(Instruction::Kind::REQUIRE_THIS_DEF, 0);
			}



			//////////////////
			// WaitOnSubSymbolProcDecl

			EVO_NODISCARD auto createWaitOnSubSymbolProcDecl(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::WAIT_ON_SUB_SYMBOL_PROC_DECL,
					this->wait_on_sub_symbol_proc_decls.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getWaitOnSubSymbolProcDecl(Instruction instr) const
			-> const Instruction::WaitOnSubSymbolProcDecl& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::WAIT_ON_SUB_SYMBOL_PROC_DECL, "Not a WaitOnSubSymbolProcDecl"
				);
				return this->wait_on_sub_symbol_proc_decls[instr._index];
			}



			//////////////////
			// WaitOnSubSymbolProcDef

			EVO_NODISCARD auto createWaitOnSubSymbolProcDef(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::WAIT_ON_SUB_SYMBOL_PROC_DEF,
					this->wait_on_sub_symbol_proc_defs.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getWaitOnSubSymbolProcDef(Instruction instr) const
			-> const Instruction::WaitOnSubSymbolProcDef& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::WAIT_ON_SUB_SYMBOL_PROC_DEF, "Not a WaitOnSubSymbolProcDef"
				);
				return this->wait_on_sub_symbol_proc_defs[instr._index];
			}



			//////////////////
			// FuncCallExpr<true, true>

			EVO_NODISCARD auto createFuncCallExprComptimeErrors(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::FUNC_CALL_EXPR_COMPTIME_ERRORS,
					this->func_call_expr_comptime_errorss.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getFuncCallExprComptimeErrors(Instruction instr) const
			-> const Instruction::FuncCallExpr<true, true>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::FUNC_CALL_EXPR_COMPTIME_ERRORS, "Not a FuncCallExpr<true, true>"
				);
				return this->func_call_expr_comptime_errorss[instr._index];
			}



			//////////////////
			// FuncCallExpr<true, false>

			EVO_NODISCARD auto createFuncCallExprComptime(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::FUNC_CALL_EXPR_COMPTIME,
					this->func_call_expr_comptimes.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getFuncCallExprComptime(Instruction instr) const
			-> const Instruction::FuncCallExpr<true, false>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::FUNC_CALL_EXPR_COMPTIME, "Not a FuncCallExpr<true, false>"
				);
				return this->func_call_expr_comptimes[instr._index];
			}



			//////////////////
			// FuncCallExpr<false, true>

			EVO_NODISCARD auto createFuncCallExprErrors(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::FUNC_CALL_EXPR_ERRORS,
					this->func_call_expr_errorss.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getFuncCallExprErrors(Instruction instr) const
			-> const Instruction::FuncCallExpr<false, true>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::FUNC_CALL_EXPR_ERRORS, "Not a FuncCallExpr<false, true>"
				);
				return this->func_call_expr_errorss[instr._index];
			}



			//////////////////
			// FuncCallExpr<false, false>

			EVO_NODISCARD auto createFuncCallExpr(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::FUNC_CALL_EXPR,
					this->func_call_exprs.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getFuncCallExpr(Instruction instr) const
			-> const Instruction::FuncCallExpr<false, false>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::FUNC_CALL_EXPR, "Not a FuncCallExpr<false, false>"
				);
				return this->func_call_exprs[instr._index];
			}



			//////////////////
			// ComptimeFuncCallRun

			EVO_NODISCARD auto createComptimeFuncCallRun(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::COMPTIME_FUNC_CALL_RUN,
					this->comptime_func_call_runs.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getComptimeFuncCallRun(Instruction instr) const
			-> const Instruction::ComptimeFuncCallRun& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::COMPTIME_FUNC_CALL_RUN, "Not a ComptimeFuncCallRun"
				);
				return this->comptime_func_call_runs[instr._index];
			}



			//////////////////
			// Import<Language::PANTHER>

			EVO_NODISCARD auto createImportPanther(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::IMPORT_PANTHER,
					this->import_panthers.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getImportPanther(Instruction instr) const
			-> const Instruction::Import<Instruction::Language::PANTHER>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::IMPORT_PANTHER, "Not a Import<Language::PANTHER>"
				);
				return this->import_panthers[instr._index];
			}



			//////////////////
			// Import<Language::C>

			EVO_NODISCARD auto createImportC(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::IMPORT_C,
					this->import_cs.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getImportC(Instruction instr) const
			-> const Instruction::Import<Instruction::Language::C>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::IMPORT_C, "Not a Import<Language::C>"
				);
				return this->import_cs[instr._index];
			}



			//////////////////
			// Import<Language::CPP>

			EVO_NODISCARD auto createImportCpp(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::IMPORT_CPP,
					this->import_cpps.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getImportCpp(Instruction instr) const
			-> const Instruction::Import<Instruction::Language::CPP>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::IMPORT_CPP, "Not a Import<Language::CPP>"
				);
				return this->import_cpps[instr._index];
			}



			//////////////////
			// IsMacroDefined

			EVO_NODISCARD auto createIsMacroDefined(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::IS_MACRO_DEFINED,
					this->is_macro_defineds.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getIsMacroDefined(Instruction instr) const -> const Instruction::IsMacroDefined& {
				evo::debugAssert(instr.kind() == Instruction::Kind::IS_MACRO_DEFINED, "Not an IsMacroDefined");
				return this->is_macro_defineds[instr._index];
			}



			//////////////////
			// MakeInitPtr

			EVO_NODISCARD auto createMakeInitPtr(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::MAKE_INIT_PTR,
					this->make_init_ptrs.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getMakeInitPtr(Instruction instr) const -> const Instruction::MakeInitPtr& {
				evo::debugAssert(instr.kind() == Instruction::Kind::MAKE_INIT_PTR, "Not an MakeInitPtr");
				return this->make_init_ptrs[instr._index];
			}


			//////////////////
			// TemplateIntrinsicFuncCall

			EVO_NODISCARD auto createTemplateIntrinsicFuncCall(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::TEMPLATE_INTRINSIC_FUNC_CALL,
					this->template_intrinsic_func_calls.emplace_back(
						std::forward<decltype(args)>(args)...
					)
				);
			}

			EVO_NODISCARD auto getTemplateIntrinsicFuncCall(Instruction instr) const
			-> const Instruction::TemplateIntrinsicFuncCall& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::TEMPLATE_INTRINSIC_FUNC_CALL,
					"Not a TemplateIntrinsicFuncCall"
				);
				return this->template_intrinsic_func_calls[instr._index];
			}




			//////////////////
			// TemplateIntrinsicFuncCallExpr<true>

			EVO_NODISCARD auto createTemplateIntrinsicFuncCallExprComptime(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::TEMPLATE_INTRINSIC_FUNC_CALL_EXPR_COMPTIME,
					this->template_intrinsic_func_call_expr_comptimes.emplace_back(
						std::forward<decltype(args)>(args)...
					)
				);
			}

			EVO_NODISCARD auto getTemplateIntrinsicFuncCallExprComptime(Instruction instr) const
			-> const Instruction::TemplateIntrinsicFuncCallExpr<true>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::TEMPLATE_INTRINSIC_FUNC_CALL_EXPR_COMPTIME,
					"Not a TemplateIntrinsicFuncCallExpr<true>"
				);
				return this->template_intrinsic_func_call_expr_comptimes[instr._index];
			}



			//////////////////
			// TemplateIntrinsicFuncCallExpr<false>

			EVO_NODISCARD auto createTemplateIntrinsicFuncCallExpr(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::TEMPLATE_INTRINSIC_FUNC_CALL_EXPR,
					this->template_intrinsic_func_call_exprs.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getTemplateIntrinsicFuncCallExpr(Instruction instr) const
			-> const Instruction::TemplateIntrinsicFuncCallExpr<false>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::TEMPLATE_INTRINSIC_FUNC_CALL_EXPR,
					"Not a TemplateIntrinsicFuncCallExpr<false>"
				);
				return this->template_intrinsic_func_call_exprs[instr._index];
			}



			//////////////////
			// Indexer<true>

			EVO_NODISCARD auto createIndexerComptime(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::INDEXER_COMPTIME,
					this->indexer_comptimes.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getIndexerComptime(Instruction instr) const -> const Instruction::Indexer<true>& {
				evo::debugAssert(instr.kind() == Instruction::Kind::INDEXER_COMPTIME, "Not a Indexer<true>");
				return this->indexer_comptimes[instr._index];
			}



			//////////////////
			// Indexer<false>

			EVO_NODISCARD auto createIndexer(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::INDEXER,
					this->indexers.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getIndexer(Instruction instr) const -> const Instruction::Indexer<false>& {
				evo::debugAssert(instr.kind() == Instruction::Kind::INDEXER, "Not a Indexer<false>");
				return this->indexers[instr._index];
			}



			//////////////////
			// TemplatedTerm

			EVO_NODISCARD auto createTemplatedTerm(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::TEMPLATED_TERM,
					this->templated_terms.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getTemplatedTerm(Instruction instr) const -> const Instruction::TemplatedTerm& {
				evo::debugAssert(instr.kind() == Instruction::Kind::TEMPLATED_TERM, "Not a TemplatedTerm");
				return this->templated_terms[instr._index];
			}



			//////////////////
			// TemplatedTermWait<true>

			EVO_NODISCARD auto createTemplatedTermWaitForDef(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::TEMPLATED_TERM_WAIT_FOR_DEF,
					this->templated_term_wait_for_defs.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getTemplatedTermWaitForDef(Instruction instr) const
			-> const Instruction::TemplatedTermWait<true>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::TEMPLATED_TERM_WAIT_FOR_DEF, "Not a TemplatedTermWait<true>"
				);
				return this->templated_term_wait_for_defs[instr._index];
			}



			//////////////////
			// TemplatedTermWait<false>

			EVO_NODISCARD auto createTemplatedTermWaitForDecl(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::TEMPLATED_TERM_WAIT_FOR_DECL,
					this->templated_term_wait_for_decls.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getTemplatedTermWaitForDecl(Instruction instr) const
			-> const Instruction::TemplatedTermWait<false>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::TEMPLATED_TERM_WAIT_FOR_DECL, "Not a TemplatedTermWait<false>"
				);
				return this->templated_term_wait_for_decls[instr._index];
			}



			//////////////////
			// PushTemplateDeclInstantiationTypesScope

			EVO_NODISCARD auto createPushTemplateDeclInstantiationTypesScope() -> Instruction {
				return Instruction(Instruction::Kind::PUSH_TEMPLATE_DECL_INSTANTIATION_TYPES_SCOPE, 0);
			}


			//////////////////
			// PopTemplateDeclInstantiationTypesScope

			EVO_NODISCARD auto createPopTemplateDeclInstantiationTypesScope() -> Instruction {
				return Instruction(Instruction::Kind::POP_TEMPLATE_DECL_INSTANTIATION_TYPES_SCOPE, 0);
			}


			//////////////////
			// AddTemplateDeclInstantiationType

			EVO_NODISCARD auto createAddTemplateDeclInstantiationType(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::ADD_TEMPLATE_DECL_INSTANTIATION_TYPE,
					this->add_template_decl_instantiation_types.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getAddTemplateDeclInstantiationType(Instruction instr) const
			-> const Instruction::AddTemplateDeclInstantiationType& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::ADD_TEMPLATE_DECL_INSTANTIATION_TYPE,
					"Not a AddTemplateDeclInstantiationType"
				);
				return this->add_template_decl_instantiation_types[instr._index];
			}



			//////////////////
			// Copy

			EVO_NODISCARD auto createCopy(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::COPY,
					this->copys.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getCopy(Instruction instr) const -> const Instruction::Copy& {
				evo::debugAssert(instr.kind() == Instruction::Kind::COPY, "Not a Copy");
				return this->copys[instr._index];
			}



			//////////////////
			// Move

			EVO_NODISCARD auto createMove(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::MOVE,
					this->moves.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getMove(Instruction instr) const -> const Instruction::Move& {
				evo::debugAssert(instr.kind() == Instruction::Kind::MOVE, "Not a Move");
				return this->moves[instr._index];
			}



			//////////////////
			// Forward

			EVO_NODISCARD auto createForward(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::FORWARD,
					this->forwards.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getForward(Instruction instr) const -> const Instruction::Forward& {
				evo::debugAssert(instr.kind() == Instruction::Kind::FORWARD, "Not a Forward");
				return this->forwards[instr._index];
			}


			//////////////////
			// AddrOf

			EVO_NODISCARD auto createAddrOf(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::ADDR_OF,
					this->addr_ofs.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getAddrOf(Instruction instr) const -> const Instruction::AddrOf& {
				evo::debugAssert(instr.kind() == Instruction::Kind::ADDR_OF, "Not a AddrOf");
				return this->addr_ofs[instr._index];
			}



			//////////////////
			// PrefixNegate<true>

			EVO_NODISCARD auto createPrefixNegateComptime(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::PREFIX_NEGATE_COMPTIME,
					this->prefix_negate_comptimes.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getPrefixNegateComptime(Instruction instr) const
			-> const Instruction::PrefixNegate<true>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::PREFIX_NEGATE_COMPTIME, "Not a PrefixNegate<true>"
				);
				return this->prefix_negate_comptimes[instr._index];
			}



			//////////////////
			// PrefixNegate<false>

			EVO_NODISCARD auto createPrefixNegate(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::PREFIX_NEGATE,
					this->prefix_negates.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getPrefixNegate(Instruction instr) const -> const Instruction::PrefixNegate<false>& {
				evo::debugAssert(instr.kind() == Instruction::Kind::PREFIX_NEGATE, "Not a PrefixNegate<false>");
				return this->prefix_negates[instr._index];
			}



			//////////////////
			// PrefixNot<true>

			EVO_NODISCARD auto createPrefixNotComptime(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::PREFIX_NOT_COMPTIME,
					this->prefix_not_comptimes.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getPrefixNotComptime(Instruction instr) const -> const Instruction::PrefixNot<true>& {
				evo::debugAssert(instr.kind() == Instruction::Kind::PREFIX_NOT_COMPTIME, "Not a PrefixNot<true>");
				return this->prefix_not_comptimes[instr._index];
			}



			//////////////////
			// PrefixNot<false>

			EVO_NODISCARD auto createPrefixNot(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::PREFIX_NOT,
					this->prefix_nots.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getPrefixNot(Instruction instr) const -> const Instruction::PrefixNot<false>& {
				evo::debugAssert(instr.kind() == Instruction::Kind::PREFIX_NOT, "Not a PrefixNot<false>");
				return this->prefix_nots[instr._index];
			}



			//////////////////
			// PrefixBitwiseNot<true>

			EVO_NODISCARD auto createPrefixBitwiseNotComptime(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::PREFIX_BITWISE_NOT_COMPTIME,
					this->prefix_bitwise_not_comptimes.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getPrefixBitwiseNotComptime(Instruction instr) const
			-> const Instruction::PrefixBitwiseNot<true>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::PREFIX_BITWISE_NOT_COMPTIME, "Not a PrefixBitwiseNot<true>"
				);
				return this->prefix_bitwise_not_comptimes[instr._index];
			}



			//////////////////
			// PrefixBitwiseNot<false>

			EVO_NODISCARD auto createPrefixBitwiseNot(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::PREFIX_BITWISE_NOT,
					this->prefix_bitwise_nots.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getPrefixBitwiseNot(Instruction instr) const
			-> const Instruction::PrefixBitwiseNot<false>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::PREFIX_BITWISE_NOT, "Not a PrefixBitwiseNot<false>"
					);
				return this->prefix_bitwise_nots[instr._index];
			}



			//////////////////
			// Deref

			EVO_NODISCARD auto createDeref(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::DEREF,
					this->derefs.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getDeref(Instruction instr) const -> const Instruction::Deref& {
				evo::debugAssert(instr.kind() == Instruction::Kind::DEREF, "Not a Deref");
				return this->derefs[instr._index];
			}



			//////////////////
			// Unwrap

			EVO_NODISCARD auto createUnwrap(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::UNWRAP,
					this->unwraps.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getUnwrap(Instruction instr) const -> const Instruction::Unwrap& {
				evo::debugAssert(instr.kind() == Instruction::Kind::UNWRAP, "Not a Unwrap");
				return this->unwraps[instr._index];
			}


			//////////////////
			// New<true>

			EVO_NODISCARD auto createNewComptime(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::NEW_COMPTIME,
					this->new_comptimes.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getNewComptime(Instruction instr) const
			-> const Instruction::New<true>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::NEW_COMPTIME, "Not a New<true>"
				);
				return this->new_comptimes[instr._index];
			}



			//////////////////
			// New<false>

			EVO_NODISCARD auto createNew(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::NEW,
					this->news.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getNew(Instruction instr) const -> const Instruction::New<false>& {
				evo::debugAssert(instr.kind() == Instruction::Kind::NEW, "Not a New<false>");
				return this->news[instr._index];
			}



			//////////////////
			// ArrayInitNew<true>

			EVO_NODISCARD auto createArrayInitNewComptime(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::ARRAY_INIT_NEW_COMPTIME,
					this->array_init_new_comptimes.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getArrayInitNewComptime(Instruction instr) const
			-> const Instruction::ArrayInitNew<true>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::ARRAY_INIT_NEW_COMPTIME, "Not a ArrayInitNew<true>"
				);
				return this->array_init_new_comptimes[instr._index];
			}



			//////////////////
			// ArrayInitNew<false>

			EVO_NODISCARD auto createArrayInitNew(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::ARRAY_INIT_NEW,
					this->array_init_news.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getArrayInitNew(Instruction instr) const -> const Instruction::ArrayInitNew<false>& {
				evo::debugAssert(instr.kind() == Instruction::Kind::ARRAY_INIT_NEW, "Not a ArrayInitNew<false>");
				return this->array_init_news[instr._index];
			}



			//////////////////
			// DesignatedInitNew<true>

			EVO_NODISCARD auto createDesignatedInitNewComptime(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::DESIGNATED_INIT_NEW_COMPTIME,
					this->designated_init_new_comptimes.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getDesignatedInitNewComptime(Instruction instr) const
			-> const Instruction::DesignatedInitNew<true>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::DESIGNATED_INIT_NEW_COMPTIME, "Not a DesignatedInitNew<true>"
				);
				return this->designated_init_new_comptimes[instr._index];
			}



			//////////////////
			// DesignatedInitNew<false>

			EVO_NODISCARD auto createDesignatedInitNew(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::DESIGNATED_INIT_NEW,
					this->designated_init_news.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getDesignatedInitNew(Instruction instr) const
			-> const Instruction::DesignatedInitNew<false>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::DESIGNATED_INIT_NEW, "Not a DesignatedInitNew<false>"
				);
				return this->designated_init_news[instr._index];
			}



			//////////////////
			// PrepareTryHandler

			EVO_NODISCARD auto createPrepareTryHandler(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::PREPARE_TRY_HANDLER,
					this->prepare_try_handlers.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getPrepareTryHandler(Instruction instr) const -> const Instruction::PrepareTryHandler& {
				evo::debugAssert(instr.kind() == Instruction::Kind::PREPARE_TRY_HANDLER, "Not a PrepareTryHandler");
				return this->prepare_try_handlers[instr._index];
			}



			//////////////////
			// TryElseExpr

			EVO_NODISCARD auto createTryElseExpr(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::TRY_ELSE_EXPR,
					this->try_else_exprs.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getTryElseExpr(Instruction instr) const -> const Instruction::TryElseExpr& {
				evo::debugAssert(instr.kind() == Instruction::Kind::TRY_ELSE_EXPR, "Not a TryElseExpr");
				return this->try_else_exprs[instr._index];
			}



			//////////////////
			// BeginExprBlock

			EVO_NODISCARD auto createBeginExprBlock(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::BEGIN_EXPR_BLOCK,
					this->begin_expr_blocks.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getBeginExprBlock(Instruction instr) const -> const Instruction::BeginExprBlock& {
				evo::debugAssert(instr.kind() == Instruction::Kind::BEGIN_EXPR_BLOCK, "Not a BeginExprBlock");
				return this->begin_expr_blocks[instr._index];
			}



			//////////////////
			// EndExprBlock

			EVO_NODISCARD auto createEndExprBlock(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::END_EXPR_BLOCK,
					this->end_expr_blocks.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getEndExprBlock(Instruction instr) const -> const Instruction::EndExprBlock& {
				evo::debugAssert(instr.kind() == Instruction::Kind::END_EXPR_BLOCK, "Not a EndExprBlock");
				return this->end_expr_blocks[instr._index];
			}



			//////////////////
			// As<true>

			EVO_NODISCARD auto createAsComptime(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::AS_CONTEXPR,
					this->as_contexprs.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getAsComptime(Instruction instr) const -> const Instruction::As<true>& {
				evo::debugAssert(instr.kind() == Instruction::Kind::AS_CONTEXPR, "Not a As<true>");
				return this->as_contexprs[instr._index];
			}



			//////////////////
			// As<false>

			EVO_NODISCARD auto createAs(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::AS,
					this->ass.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getAs(Instruction instr) const -> const Instruction::As<false>& {
				evo::debugAssert(instr.kind() == Instruction::Kind::AS, "Not a As<false>");
				return this->ass[instr._index];
			}



			//////////////////
			// OptionalNullCheck

			EVO_NODISCARD auto createOptionalNullCheck(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::OPTIONAL_NULL_CHECK,
					this->optional_null_checks.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getOptionalNullCheck(Instruction instr) const -> const Instruction::OptionalNullCheck& {
				evo::debugAssert(instr.kind() == Instruction::Kind::OPTIONAL_NULL_CHECK, "Not a OptionalNullCheck");
				return this->optional_null_checks[instr._index];
			}



			//////////////////
			// MathInfix<true, Instruction::MathInfixKind::COMPARATIVE>

			EVO_NODISCARD auto createMathInfixComptimeComparative(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::MATH_INFIX_COMPTIME_COMPARATIVE,
					this->math_infix_comptime_comparatives.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getMathInfixComptimeComparative(Instruction instr) const
			-> const Instruction::MathInfix<true, Instruction::MathInfixKind::COMPARATIVE>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::MATH_INFIX_COMPTIME_COMPARATIVE,
					"Not a MathInfix<true, Instruction::MathInfixKind::COMPARATIVE>"
				);
				return this->math_infix_comptime_comparatives[instr._index];
			}



			//////////////////
			// MathInfix<true, Instruction::MathInfixKind::MATH>

			EVO_NODISCARD auto createMathInfixComptimeMath(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::MATH_INFIX_COMPTIME_MATH,
					this->math_infix_comptime_maths.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getMathInfixComptimeMath(Instruction instr) const
			-> const Instruction::MathInfix<true, Instruction::MathInfixKind::MATH>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::MATH_INFIX_COMPTIME_MATH,
					"Not a MathInfix<true, Instruction::MathInfixKind::MATH>"
				);
				return this->math_infix_comptime_maths[instr._index];
			}



			//////////////////
			// MathInfix<true, Instruction::MathInfixKind::INTEGRAL_MATH>

			EVO_NODISCARD auto createMathInfixComptimeIntegralMath(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::MATH_INFIX_COMPTIME_INTEGRAL_MATH,
					this->math_infix_comptime_integral_maths.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getMathInfixComptimeIntegralMath(Instruction instr) const
			-> const Instruction::MathInfix<true, Instruction::MathInfixKind::INTEGRAL_MATH>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::MATH_INFIX_COMPTIME_INTEGRAL_MATH,
					"Not a MathInfix<true, Instruction::MathInfixKind::INTEGRAL_MATH>"
				);
				return this->math_infix_comptime_integral_maths[instr._index];
			}



			//////////////////
			// MathInfix<true, Instruction::MathInfixKind::LOGICAL>

			EVO_NODISCARD auto createMathInfixComptimeLogical(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::MATH_INFIX_COMPTIME_LOGICAL,
					this->math_infix_comptime_logicals.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getMathInfixComptimeLogical(Instruction instr) const
			-> const Instruction::MathInfix<true, Instruction::MathInfixKind::LOGICAL>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::MATH_INFIX_COMPTIME_LOGICAL,
					"Not a MathInfix<true, Instruction::MathInfixKind::LOGICAL>"
				);
				return this->math_infix_comptime_logicals[instr._index];
			}



			//////////////////
			// MathInfix<true, Instruction::MathInfixKind::BITWISE_LOGICAL>

			EVO_NODISCARD auto createMathInfixComptimeBitwiseLogical(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::MATH_INFIX_COMPTIME_BITWISE_LOGICAL,
					this->math_infix_comptime_bitwise_logicals.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getMathInfixComptimeBitwiseLogical(Instruction instr) const
			-> const Instruction::MathInfix<true, Instruction::MathInfixKind::BITWISE_LOGICAL>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::MATH_INFIX_COMPTIME_BITWISE_LOGICAL,
					"Not a MathInfix<true, Instruction::MathInfixKind::BITWISE_LOGICAL>"
				);
				return this->math_infix_comptime_bitwise_logicals[instr._index];
			}



			//////////////////
			// MathInfix<true, Instruction::MathInfixKind::SHIFT>

			EVO_NODISCARD auto createMathInfixComptimeShift(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::MATH_INFIX_COMPTIME_SHIFT,
					this->math_infix_comptime_shifts.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getMathInfixComptimeShift(Instruction instr) const
			-> const Instruction::MathInfix<true, Instruction::MathInfixKind::SHIFT>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::MATH_INFIX_COMPTIME_SHIFT,
					"Not a MathInfix<true, Instruction::MathInfixKind::SHIFT>"
				);
				return this->math_infix_comptime_shifts[instr._index];
			}



			//////////////////
			// MathInfix<false, Instruction::MathInfixKind::COMPARATIVE>

			EVO_NODISCARD auto createMathInfixComparative(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::MATH_INFIX_COMPARATIVE,
					this->math_infix_comparatives.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getMathInfixComparative(Instruction instr) const
			-> const Instruction::MathInfix<false, Instruction::MathInfixKind::COMPARATIVE>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::MATH_INFIX_COMPARATIVE,
					"Not a MathInfix<false, Instruction::MathInfixKind::COMPARATIVE>"
				);
				return this->math_infix_comparatives[instr._index];
			}



			//////////////////
			// MathInfix<false, Instruction::MathInfixKind::MATH>

			EVO_NODISCARD auto createMathInfixMath(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::MATH_INFIX_MATH,
					this->math_infix_maths.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getMathInfixMath(Instruction instr) const
			-> const Instruction::MathInfix<false, Instruction::MathInfixKind::MATH>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::MATH_INFIX_MATH,
					"Not a MathInfix<false, Instruction::MathInfixKind::MATH>"
				);
				return this->math_infix_maths[instr._index];
			}



			//////////////////
			// MathInfix<false, Instruction::MathInfixKind::INTEGRAL_MATH>

			EVO_NODISCARD auto createMathInfixIntegralMath(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::MATH_INFIX_INTEGRAL_MATH,
					this->math_infix_integral_maths.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getMathInfixIntegralMath(Instruction instr) const
			-> const Instruction::MathInfix<false, Instruction::MathInfixKind::INTEGRAL_MATH>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::MATH_INFIX_INTEGRAL_MATH,
					"Not a MathInfix<false, Instruction::MathInfixKind::INTEGRAL_MATH>"
				);
				return this->math_infix_integral_maths[instr._index];
			}
			


			//////////////////
			// MathInfix<false, Instruction::MathInfixKind::LOGICAL>

			EVO_NODISCARD auto createMathInfixLogical(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::MATH_INFIX_LOGICAL,
					this->math_infix_logicals.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getMathInfixLogical(Instruction instr) const
			-> const Instruction::MathInfix<false, Instruction::MathInfixKind::LOGICAL>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::MATH_INFIX_LOGICAL,
					"Not a MathInfix<false, Instruction::MathInfixKind::LOGICAL>"
				);
				return this->math_infix_logicals[instr._index];
			}



			//////////////////
			// MathInfix<false, Instruction::MathInfixKind::BITWISE_LOGICAL>

			EVO_NODISCARD auto createMathInfixBitwiseLogical(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::MATH_INFIX_BITWISE_LOGICAL,
					this->math_infix_bitwise_logicals.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getMathInfixBitwiseLogical(Instruction instr) const
			-> const Instruction::MathInfix<false, Instruction::MathInfixKind::BITWISE_LOGICAL>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::MATH_INFIX_BITWISE_LOGICAL,
					"Not a MathInfix<false, Instruction::MathInfixKind::BITWISE_LOGICAL>"
				);
				return this->math_infix_bitwise_logicals[instr._index];
			}



			//////////////////
			// MathInfix<false, Instruction::MathInfixKind::SHIFT>

			EVO_NODISCARD auto createMathInfixShift(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::MATH_INFIX_SHIFT,
					this->math_infix_shifts.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getMathInfixShift(Instruction instr) const
			-> const Instruction::MathInfix<false, Instruction::MathInfixKind::SHIFT>& {
				evo::debugAssert(
					instr.kind() == Instruction::Kind::MATH_INFIX_SHIFT,
					"Not a MathInfix<false, Instruction::MathInfixKind::SHIFT>"
				);
				return this->math_infix_shifts[instr._index];
			}



			//////////////////
			// Accessor<true>

			EVO_NODISCARD auto createAccessorNeedsDef(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::ACCESSOR_NEEDS_DEF,
					this->accessor_needs_defs.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getAccessorNeedsDef(Instruction instr) const -> const Instruction::Accessor<true>& {
				evo::debugAssert(instr.kind() == Instruction::Kind::ACCESSOR_NEEDS_DEF, "Not a Accessor<true>");
				return this->accessor_needs_defs[instr._index];
			}



			//////////////////
			// Accessor<false>

			EVO_NODISCARD auto createAccessor(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::ACCESSOR,
					this->accessors.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getAccessor(Instruction instr) const -> const Instruction::Accessor<false>& {
				evo::debugAssert(instr.kind() == Instruction::Kind::ACCESSOR, "Not a Accessor<false>");
				return this->accessors[instr._index];
			}



			//////////////////
			// PrimitiveType

			EVO_NODISCARD auto createPrimitiveType(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::PRIMITIVE_TYPE,
					this->primitive_types.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getPrimitiveType(Instruction instr) const -> const Instruction::PrimitiveType& {
				evo::debugAssert(instr.kind() == Instruction::Kind::PRIMITIVE_TYPE, "Not a PrimitiveType");
				return this->primitive_types[instr._index];
			}


			//////////////////
			// PrimitiveTypeTerm

			EVO_NODISCARD auto createPrimitiveTypeTerm(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::PRIMITIVE_TYPE_TERM,
					this->primitive_type_terms.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getPrimitiveTypeTerm(Instruction instr) const -> const Instruction::PrimitiveTypeTerm& {
				evo::debugAssert(instr.kind() == Instruction::Kind::PRIMITIVE_TYPE_TERM, "Not a PrimitiveTypeTerm");
				return this->primitive_type_terms[instr._index];
			}



			//////////////////
			// ArrayType

			EVO_NODISCARD auto createArrayType(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::ARRAY_TYPE,
					this->array_types.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getArrayType(Instruction instr) const -> const Instruction::ArrayType& {
				evo::debugAssert(instr.kind() == Instruction::Kind::ARRAY_TYPE, "Not a ArrayType");
				return this->array_types[instr._index];
			}



			//////////////////
			// ArrayRef

			EVO_NODISCARD auto createArrayRef(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::ARRAY_REF,
					this->array_refs.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getArrayRef(Instruction instr) const -> const Instruction::ArrayRef& {
				evo::debugAssert(instr.kind() == Instruction::Kind::ARRAY_REF, "Not a ArrayRef");
				return this->array_refs[instr._index];
			}



			//////////////////
			// InterfaceMap

			EVO_NODISCARD auto createInterfaceMap(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::INTERFACE_MAP,
					this->interface_maps.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getInterfaceMap(Instruction instr) const
			-> const Instruction::InterfaceMap& {
				evo::debugAssert(instr.kind() == Instruction::Kind::INTERFACE_MAP, "Not a InterfaceMap");
				return this->interface_maps[instr._index];
			}



			//////////////////
			// TypeIDConverter

			EVO_NODISCARD auto createTypeIDConverter(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::TYPE_ID_CONVERTER,
					this->type_id_converters.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getTypeIDConverter(Instruction instr) const -> const Instruction::TypeIDConverter& {
				evo::debugAssert(instr.kind() == Instruction::Kind::TYPE_ID_CONVERTER, "Not a TypeIDConverter");
				return this->type_id_converters[instr._index];
			}



			//////////////////
			// QualifiedType

			EVO_NODISCARD auto createQualifiedType(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::QUALIFIED_TYPE,
					this->qualified_types.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getQualifiedType(Instruction instr) const -> const Instruction::QualifiedType& {
				evo::debugAssert(instr.kind() == Instruction::Kind::QUALIFIED_TYPE, "Not a QualifiedType");
				return this->qualified_types[instr._index];
			}


			//////////////////
			// QualifiedTypeTerm

			EVO_NODISCARD auto createQualifiedTypeTerm(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::QUALIFIED_TYPE_TERM,
					this->qualified_type_terms.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getQualifiedTypeTerm(Instruction instr) const -> const Instruction::QualifiedTypeTerm& {
				evo::debugAssert(instr.kind() == Instruction::Kind::QUALIFIED_TYPE_TERM, "Not a QualifiedTypeTerm");
				return this->qualified_type_terms[instr._index];
			}



			//////////////////
			// BaseTypeIdent

			EVO_NODISCARD auto createBaseTypeIdent(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::BASE_TYPE_IDENT,
					this->base_type_idents.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getBaseTypeIdent(Instruction instr) const -> const Instruction::BaseTypeIdent& {
				evo::debugAssert(instr.kind() == Instruction::Kind::BASE_TYPE_IDENT, "Not a BaseTypeIdent");
				return this->base_type_idents[instr._index];
			}



			//////////////////
			// Ident<false>

			EVO_NODISCARD auto createIdentNeedsDef(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::IDENT_NEEDS_DEF,
					this->ident_needs_defs.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getIdentNeedsDef(Instruction instr) const -> const Instruction::Ident<true>& {
				evo::debugAssert(instr.kind() == Instruction::Kind::IDENT_NEEDS_DEF, "Not a Ident<false>");
				return this->ident_needs_defs[instr._index];
			}



			//////////////////
			// Ident<true>

			EVO_NODISCARD auto createIdent(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::IDENT,
					this->idents.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getIdent(Instruction instr) const -> const Instruction::Ident<false>& {
				evo::debugAssert(instr.kind() == Instruction::Kind::IDENT, "Not a Ident<true>");
				return this->idents[instr._index];
			}



			//////////////////
			// Intrinsic

			EVO_NODISCARD auto createIntrinsic(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::INTRINSIC,
					this->intrinsics.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getIntrinsic(Instruction instr) const -> const Instruction::Intrinsic& {
				evo::debugAssert(instr.kind() == Instruction::Kind::INTRINSIC, "Not a Intrinsic");
				return this->intrinsics[instr._index];
			}

			//////////////////
			// TypeThis<true>

			EVO_NODISCARD auto createTypeThisNeedsDef(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::TYPE_THIS_NEEDS_DEF,
					this->type_this_needs_defs.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getTypeThisNeedsDef(Instruction instr) const -> const Instruction::TypeThis<true>& {
				evo::debugAssert(instr.kind() == Instruction::Kind::TYPE_THIS_NEEDS_DEF, "Not a TypeThis<true>");
				return this->type_this_needs_defs[instr._index];
			}



			//////////////////
			// TypeThis<false>

			EVO_NODISCARD auto createTypeThis(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::TYPE_THIS,
					this->type_thiss.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getTypeThis(Instruction instr) const -> const Instruction::TypeThis<false>& {
				evo::debugAssert(instr.kind() == Instruction::Kind::TYPE_THIS, "Not a TypeThis<false>");
				return this->type_thiss[instr._index];
			}



			//////////////////
			// Literal

			EVO_NODISCARD auto createLiteral(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::LITERAL,
					this->literals.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getLiteral(Instruction instr) const -> const Instruction::Literal& {
				evo::debugAssert(instr.kind() == Instruction::Kind::LITERAL, "Not a Literal");
				return this->literals[instr._index];
			}



			//////////////////
			// Uninit

			EVO_NODISCARD auto createUninit(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::UNINIT,
					this->uninits.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getUninit(Instruction instr) const -> const Instruction::Uninit& {
				evo::debugAssert(instr.kind() == Instruction::Kind::UNINIT, "Not a Uninit");
				return this->uninits[instr._index];
			}



			//////////////////
			// Zeroinit

			EVO_NODISCARD auto createZeroinit(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::ZEROINIT,
					this->zeroinits.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getZeroinit(Instruction instr) const -> const Instruction::Zeroinit& {
				evo::debugAssert(instr.kind() == Instruction::Kind::ZEROINIT, "Not a Zeroinit");
				return this->zeroinits[instr._index];
			}



			//////////////////
			// This

			EVO_NODISCARD auto createThis(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::THIS,
					this->thiss.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getThis(Instruction instr) const -> const Instruction::This& {
				evo::debugAssert(instr.kind() == Instruction::Kind::THIS, "Not a This");
				return this->thiss[instr._index];
			}



			//////////////////
			// TypeDeducer

			EVO_NODISCARD auto createTypeDeducer(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::TYPE_DEDUCER,
					this->type_deducers.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getTypeDeducer(Instruction instr) const -> const Instruction::TypeDeducer& {
				evo::debugAssert(instr.kind() == Instruction::Kind::TYPE_DEDUCER, "Not a TypeDeducer");
				return this->type_deducers[instr._index];
			}



			//////////////////
			// ExprDeducer

			EVO_NODISCARD auto createExprDeducer(auto&&... args) -> Instruction {
				return Instruction(
					Instruction::Kind::EXPR_DEDUCER,
					this->expr_deducers.emplace_back(std::forward<decltype(args)>(args)...)
				);
			}

			EVO_NODISCARD auto getExprDeducer(Instruction instr) const -> const Instruction::ExprDeducer& {
				evo::debugAssert(instr.kind() == Instruction::Kind::EXPR_DEDUCER, "Not a ExprDeducer");
				return this->expr_deducers[instr._index];
			}



			///////////////////////////////////
			// builtin symbols


			EVO_NODISCARD auto lookupBuiltinSymbolKind(std::string_view str)
			-> evo::Result<SymbolProc::BuiltinSymbolKind> {
				const auto find = this->builtin_symbol_kind_lookup.find(str);
				if(find != this->builtin_symbol_kind_lookup.end()){ return find->second; }
				return evo::resultError;
			}

			EVO_NODISCARD static consteval auto constevalLookupBuiltinSymbolKind(std::string_view str)
			-> SymbolProc::BuiltinSymbolKind {
				if(str == "array.IIterable"){ return SymbolProc::BuiltinSymbolKind::ARRAY_ITERABLE; }
				if(str == "array.IIterableRT"){ return SymbolProc::BuiltinSymbolKind::ARRAY_ITERABLE_RT; }
				if(str == "arrayRef.IIterableRef"){ return SymbolProc::BuiltinSymbolKind::ARRAY_REF_ITERABLE_REF; }
				if(str == "arrayRef.IIterableRefRT"){ return SymbolProc::BuiltinSymbolKind::ARRAY_REF_ITERABLE_REF_RT; }
				if(str == "arrayMutRef.IIterableMutRef"){
					return SymbolProc::BuiltinSymbolKind::ARRAY_MUT_REF_ITERABLE_MUT_REF;
				}
				if(str == "arrayMutRef.IIterableMutRefRT"){
					return SymbolProc::BuiltinSymbolKind::ARRAY_MUT_REF_ITERABLE_MUT_REF_RT;
				}

				evo::debugFatalBreak("Unknown or unsupported builtin symbol str ({})", str);
			}


			EVO_NODISCARD auto getBuiltinSymbol(SymbolProc::BuiltinSymbolKind kind) const -> SymbolProc::ID {
				return *this->builtin_symbols[size_t(kind)].symbol_proc_id.load();
			}

			// error is previously defined symbol
			EVO_NODISCARD auto setBuiltinSymbol(
				SymbolProc::BuiltinSymbolKind kind, SymbolProc::ID symbol_proc_id, class Context& context
			) -> evo::Expected<void, SymbolProc::ID>;

			// returns true if needs to wait
			EVO_NODISCARD auto waitOnSymbolProcOfBuiltinSymbolIfNeeded(
				SymbolProc::BuiltinSymbolKind kind, SymbolProc::ID symbol_proc_id, class Context& context
			) -> bool;


		private:
			EVO_NODISCARD auto create_symbol_proc(auto&&... args) -> SymbolProc::ID {
				this->num_procs_not_done += 1;
				return this->symbol_procs.emplace_back(std::forward<decltype(args)>(args)...);
			}

			auto symbol_proc_done() -> void {
				#if defined(PCIT_CONFIG_DEBUG)
					evo::debugAssert(this->num_procs_not_done.fetch_sub(1) > 0, "Already completed all symbols");
				#else
					this->num_procs_not_done -= 1;
				#endif
			}

			auto symbol_proc_suspended() -> void {
				this->num_procs_suspended += 1;
			}

			auto symbol_proc_unsuspended() -> void {
				#if defined(PCIT_CONFIG_DEBUG)
					evo::debugAssert(this->num_procs_suspended.fetch_sub(1) > 0, "No symbols currently suspended");
				#else
					this->num_procs_suspended -= 1;
				#endif
			}


			#if defined(PCIT_CONFIG_DEBUG)
				auto debug_dump(bool minimize_done) -> void;
			#endif

	
		private:
			core::SyncLinearStepAlloc<SymbolProc, SymbolProc::ID> symbol_procs{};

			std::unordered_map<TypeInfo::ID, SymbolProc::ID> type_symbol_procs{};
			mutable evo::SpinLock type_symbol_procs_lock{};

			std::atomic<size_t> num_procs_not_done = 0;
			std::atomic<size_t> num_procs_suspended = 0;


			struct BuiltinSymbolInfo{
				std::atomic<std::optional<SymbolProc::ID>> symbol_proc_id{};

				evo::SmallVector<SymbolProc::ID> waited_on_by{};
				mutable evo::SpinLock waited_on_by_lock{};
			};

			std::array<BuiltinSymbolInfo, size_t(SymbolProc::BuiltinSymbolKind::_LAST_) + 1> builtin_symbols{};
			std::atomic<size_t> num_builtin_symbols_waited_on = 0;
			std::unordered_map<std::string_view, SymbolProc::BuiltinSymbolKind> builtin_symbol_kind_lookup{};


			core::SyncLinearStepAlloc<Instruction::NonLocalVarDecl, uint32_t> non_local_var_decls{};
			core::SyncLinearStepAlloc<Instruction::NonLocalVarDef, uint32_t> non_local_var_defs{};
			core::SyncLinearStepAlloc<Instruction::NonLocalVarDeclDef, uint32_t> non_local_var_decl_defs{};
			core::SyncLinearStepAlloc<Instruction::WhenCond, uint32_t> when_conds{};
			core::SyncLinearStepAlloc<Instruction::Alias, uint32_t> aliases{};
			core::SyncLinearStepAlloc<Instruction::StructDecl<true>, uint32_t> struct_decl_instantiations{};
			core::SyncLinearStepAlloc<Instruction::StructDecl<false>, uint32_t> struct_decls{};
			core::SyncLinearStepAlloc<Instruction::TemplateStruct, uint32_t> template_structs{};
			core::SyncLinearStepAlloc<Instruction::UnionDecl, uint32_t> union_decls{};
			core::SyncLinearStepAlloc<Instruction::UnionAddFields, uint32_t> union_add_fieldss{};
			core::SyncLinearStepAlloc<Instruction::EnumDecl, uint32_t> enum_decls{};
			core::SyncLinearStepAlloc<Instruction::EnumAddEnumerators, uint32_t> enum_add_enumeratorss{};
			core::SyncLinearStepAlloc<Instruction::FuncDeclExtractDeducers, uint32_t> func_decl_extract_deducerss{};
			core::SyncLinearStepAlloc<Instruction::FuncDecl<true>, uint32_t> func_decl_instantiations{};
			core::SyncLinearStepAlloc<Instruction::FuncDecl<false>, uint32_t> func_decls{};
			core::SyncLinearStepAlloc<Instruction::FuncPreBody, uint32_t> func_pre_bodys{};
			core::SyncLinearStepAlloc<Instruction::FuncDef, uint32_t> func_defs{};
			core::SyncLinearStepAlloc<Instruction::FuncPrepareComptimePIRIfNeeded, uint32_t>
				func_prepare_comptime_pir_if_neededs{};
			core::SyncLinearStepAlloc<Instruction::TemplateFuncBegin, uint32_t> template_func_begins{};
			core::SyncLinearStepAlloc<Instruction::TemplateFuncSetParamIsDeducer, uint32_t>
				template_func_set_param_is_deducers{};
			core::SyncLinearStepAlloc<Instruction::TemplateFuncEnd, uint32_t> template_func_ends{};
			core::SyncLinearStepAlloc<Instruction::DeletedSpecialMethod, uint32_t> deleted_special_methods{};
			core::SyncLinearStepAlloc<Instruction::FuncAliasDef, uint32_t> func_alias_def{};
			core::SyncLinearStepAlloc<Instruction::InterfacePrepare, uint32_t> interface_prepares{};
			core::SyncLinearStepAlloc<Instruction::InterfaceFuncDef, uint32_t> interface_func_defs{};
			core::SyncLinearStepAlloc<Instruction::InterfaceImplDecl, uint32_t> interface_impl_decls{};
			core::SyncLinearStepAlloc<Instruction::InterfaceInDefImplDecl, uint32_t> interface_in_def_impl_decls{};
			core::SyncLinearStepAlloc<Instruction::InterfaceDeducerImplInstantiationDecl, uint32_t>
				interface_deducer_impl_instantiation_decls{};
			core::SyncLinearStepAlloc<Instruction::InterfaceImplMethodLookup, uint32_t> interface_impl_method_lookups{};
			core::SyncLinearStepAlloc<Instruction::InterfaceInDefImplMethod, uint32_t> interface_in_def_impl_method{};
			core::SyncLinearStepAlloc<Instruction::InterfaceImplDef, uint32_t> interface_impl_defs{};
			core::SyncLinearStepAlloc<Instruction::LocalVar, uint32_t> local_vars{};
			core::SyncLinearStepAlloc<Instruction::LocalFuncAlias, uint32_t> local_func_aliass{};
			core::SyncLinearStepAlloc<Instruction::LocalAlias, uint32_t> local_aliass{};
			core::SyncLinearStepAlloc<Instruction::Return, uint32_t> returns{};
			core::SyncLinearStepAlloc<Instruction::LabeledReturn, uint32_t> labeled_returns{};
			core::SyncLinearStepAlloc<Instruction::Error, uint32_t> errors{};
			core::SyncLinearStepAlloc<Instruction::Unreachable, uint32_t> unreachables{};
			core::SyncLinearStepAlloc<Instruction::Break, uint32_t> breaks{};
			core::SyncLinearStepAlloc<Instruction::Continue, uint32_t> continues{};
			core::SyncLinearStepAlloc<Instruction::Delete, uint32_t> deletes{};
			core::SyncLinearStepAlloc<Instruction::BeginCond, uint32_t> begin_conds{};
			core::SyncLinearStepAlloc<Instruction::EndCondSet, uint32_t> end_cond_sets{};
			core::SyncLinearStepAlloc<Instruction::BeginLocalWhenCond, uint32_t> begin_local_when_conds{};
			core::SyncLinearStepAlloc<Instruction::EndLocalWhenCond, uint32_t> end_local_when_conds{};
			core::SyncLinearStepAlloc<Instruction::BeginWhile, uint32_t> begin_whiles{};
			core::SyncLinearStepAlloc<Instruction::EndWhile, uint32_t> end_whiles{};
			core::SyncLinearStepAlloc<Instruction::BeginFor, uint32_t> begin_fors{};
			core::SyncLinearStepAlloc<Instruction::EndFor, uint32_t> end_fors{};
			core::SyncLinearStepAlloc<Instruction::BeginForUnroll, uint32_t> begin_for_unrolls{};
			core::SyncLinearStepAlloc<Instruction::ForUnrollCond, uint32_t> for_unroll_conds{};
			core::SyncLinearStepAlloc<Instruction::ForUnrollContinue, uint32_t> for_unroll_continues{};
			core::SyncLinearStepAlloc<Instruction::BeginSwitch, uint32_t> begin_switches{};
			core::SyncLinearStepAlloc<Instruction::BeginCase, uint32_t> begin_cases{};

			core::SyncLinearStepAlloc<Instruction::EndSwitch, uint32_t> end_switches{};
			core::SyncLinearStepAlloc<Instruction::BeginDefer, uint32_t> begin_defers{};
			core::SyncLinearStepAlloc<Instruction::EndDefer, uint32_t> end_defers{};
			core::SyncLinearStepAlloc<Instruction::BeginStmtBlock, uint32_t> begin_stmt_blocks{};
			core::SyncLinearStepAlloc<Instruction::EndStmtBlock, uint32_t> end_stmt_blocks{};
			core::SyncLinearStepAlloc<Instruction::FuncCall, uint32_t> func_calls{};
			core::SyncLinearStepAlloc<Instruction::Assignment, uint32_t> assignments{};
			core::SyncLinearStepAlloc<Instruction::AssignmentNew, uint32_t> assignment_news{};
			core::SyncLinearStepAlloc<Instruction::AssignmentCopy, uint32_t> assignment_copies{};
			core::SyncLinearStepAlloc<Instruction::AssignmentMove, uint32_t> assignment_moves{};
			core::SyncLinearStepAlloc<Instruction::AssignmentForward, uint32_t> assignment_forwards{};
			core::SyncLinearStepAlloc<Instruction::MultiAssign, uint32_t> multi_assigns{};
			core::SyncLinearStepAlloc<Instruction::DiscardingAssignment, uint32_t> discarding_assignments{};
			core::SyncLinearStepAlloc<Instruction::TryElseBegin, uint32_t> try_else_begins{};
			core::SyncLinearStepAlloc<Instruction::BeginUnsafe, uint32_t> begin_unsafes{};
			core::SyncLinearStepAlloc<Instruction::TypeToTerm, uint32_t> type_to_terms{};
			core::SyncLinearStepAlloc<Instruction::WaitOnSubSymbolProcDecl, uint32_t> wait_on_sub_symbol_proc_decls{};
			core::SyncLinearStepAlloc<Instruction::WaitOnSubSymbolProcDef, uint32_t> wait_on_sub_symbol_proc_defs{};

			core::SyncLinearStepAlloc<Instruction::FuncCallExpr<true, true>, uint32_t>
				func_call_expr_comptime_errorss{};

			core::SyncLinearStepAlloc<Instruction::FuncCallExpr<true, false>, uint32_t> func_call_expr_comptimes{};
			core::SyncLinearStepAlloc<Instruction::FuncCallExpr<false, true>, uint32_t> func_call_expr_errorss{};
			core::SyncLinearStepAlloc<Instruction::FuncCallExpr<false, false>, uint32_t> func_call_exprs{};
			core::SyncLinearStepAlloc<Instruction::ComptimeFuncCallRun, uint32_t> comptime_func_call_runs{};
			core::SyncLinearStepAlloc<Instruction::Import<Instruction::Language::PANTHER>, uint32_t> import_panthers{};
			core::SyncLinearStepAlloc<Instruction::Import<Instruction::Language::C>, uint32_t> import_cs{};
			core::SyncLinearStepAlloc<Instruction::Import<Instruction::Language::CPP>, uint32_t> import_cpps{};
			core::SyncLinearStepAlloc<Instruction::IsMacroDefined, uint32_t> is_macro_defineds{};
			core::SyncLinearStepAlloc<Instruction::MakeInitPtr, uint32_t> make_init_ptrs{};
			
			core::SyncLinearStepAlloc<Instruction::TemplateIntrinsicFuncCall, uint32_t>
				template_intrinsic_func_calls{};

			core::SyncLinearStepAlloc<Instruction::TemplateIntrinsicFuncCallExpr<true>, uint32_t>
				template_intrinsic_func_call_expr_comptimes{};

			core::SyncLinearStepAlloc<Instruction::TemplateIntrinsicFuncCallExpr<false>, uint32_t>
				template_intrinsic_func_call_exprs{};

			core::SyncLinearStepAlloc<Instruction::Indexer<true>, uint32_t> indexer_comptimes{};
			core::SyncLinearStepAlloc<Instruction::Indexer<false>, uint32_t> indexers{};
			core::SyncLinearStepAlloc<Instruction::TemplatedTerm, uint32_t> templated_terms{};
			core::SyncLinearStepAlloc<Instruction::TemplatedTermWait<true>, uint32_t> templated_term_wait_for_defs{};
			core::SyncLinearStepAlloc<Instruction::TemplatedTermWait<false>, uint32_t> templated_term_wait_for_decls{};

			core::SyncLinearStepAlloc<Instruction::AddTemplateDeclInstantiationType, uint32_t>
				add_template_decl_instantiation_types{};

			core::SyncLinearStepAlloc<Instruction::Copy, uint32_t> copys{};
			core::SyncLinearStepAlloc<Instruction::Move, uint32_t> moves{};
			core::SyncLinearStepAlloc<Instruction::Forward, uint32_t> forwards{};
			core::SyncLinearStepAlloc<Instruction::AddrOf, uint32_t> addr_ofs{};
			core::SyncLinearStepAlloc<Instruction::PrefixNegate<true>, uint32_t> prefix_negate_comptimes{};
			core::SyncLinearStepAlloc<Instruction::PrefixNegate<false>, uint32_t> prefix_negates{};
			core::SyncLinearStepAlloc<Instruction::PrefixNot<true>, uint32_t> prefix_not_comptimes{};
			core::SyncLinearStepAlloc<Instruction::PrefixNot<false>, uint32_t> prefix_nots{};
			core::SyncLinearStepAlloc<Instruction::PrefixBitwiseNot<true>, uint32_t> prefix_bitwise_not_comptimes{};
			core::SyncLinearStepAlloc<Instruction::PrefixBitwiseNot<false>, uint32_t> prefix_bitwise_nots{};
			core::SyncLinearStepAlloc<Instruction::Deref, uint32_t> derefs{};
			core::SyncLinearStepAlloc<Instruction::Unwrap, uint32_t> unwraps{};
			core::SyncLinearStepAlloc<Instruction::New<true>, uint32_t> new_comptimes{};
			core::SyncLinearStepAlloc<Instruction::New<false>, uint32_t> news{};
			core::SyncLinearStepAlloc<Instruction::ArrayInitNew<true>, uint32_t> array_init_new_comptimes{};
			core::SyncLinearStepAlloc<Instruction::ArrayInitNew<false>, uint32_t> array_init_news{};
			core::SyncLinearStepAlloc<Instruction::DesignatedInitNew<true>, uint32_t> designated_init_new_comptimes{};
			core::SyncLinearStepAlloc<Instruction::DesignatedInitNew<false>, uint32_t> designated_init_news{};
			core::SyncLinearStepAlloc<Instruction::PrepareTryHandler, uint32_t> prepare_try_handlers{};
			core::SyncLinearStepAlloc<Instruction::TryElseExpr, uint32_t> try_else_exprs{};
			core::SyncLinearStepAlloc<Instruction::BeginExprBlock, uint32_t> begin_expr_blocks{};
			core::SyncLinearStepAlloc<Instruction::EndExprBlock, uint32_t> end_expr_blocks{};
			core::SyncLinearStepAlloc<Instruction::As<true>, uint32_t> as_contexprs{};
			core::SyncLinearStepAlloc<Instruction::As<false>, uint32_t> ass{};
			core::SyncLinearStepAlloc<Instruction::OptionalNullCheck, uint32_t> optional_null_checks{};

			core::SyncLinearStepAlloc<Instruction::MathInfix<true, Instruction::MathInfixKind::COMPARATIVE>, uint32_t>
				math_infix_comptime_comparatives{};

			core::SyncLinearStepAlloc<Instruction::MathInfix<true, Instruction::MathInfixKind::MATH>, uint32_t>
				math_infix_comptime_maths{};

			core::SyncLinearStepAlloc<Instruction::MathInfix<true, Instruction::MathInfixKind::INTEGRAL_MATH>, uint32_t>
				math_infix_comptime_integral_maths{};

			core::SyncLinearStepAlloc<Instruction::MathInfix<true, Instruction::MathInfixKind::LOGICAL>, uint32_t>
				math_infix_comptime_logicals{};

			core::SyncLinearStepAlloc<Instruction::MathInfix<
				true, Instruction::MathInfixKind::BITWISE_LOGICAL>, uint32_t
			> math_infix_comptime_bitwise_logicals{};

			core::SyncLinearStepAlloc<Instruction::MathInfix<true, Instruction::MathInfixKind::SHIFT>, uint32_t>
				math_infix_comptime_shifts{};

			core::SyncLinearStepAlloc<Instruction::MathInfix<false, Instruction::MathInfixKind::COMPARATIVE>, uint32_t>
				math_infix_comparatives{};

			core::SyncLinearStepAlloc<Instruction::MathInfix<false, Instruction::MathInfixKind::MATH>, uint32_t>
				math_infix_maths{};

			core::SyncLinearStepAlloc<Instruction::MathInfix<
				false, Instruction::MathInfixKind::INTEGRAL_MATH>, uint32_t
			> math_infix_integral_maths{};

			core::SyncLinearStepAlloc<Instruction::MathInfix<false, Instruction::MathInfixKind::LOGICAL>, uint32_t>
				math_infix_logicals{};

			core::SyncLinearStepAlloc<
				Instruction::MathInfix<false, Instruction::MathInfixKind::BITWISE_LOGICAL>, uint32_t
			> math_infix_bitwise_logicals{};

			core::SyncLinearStepAlloc<Instruction::MathInfix<false, Instruction::MathInfixKind::SHIFT>, uint32_t>
				math_infix_shifts{};

			core::SyncLinearStepAlloc<Instruction::Accessor<true>, uint32_t> accessor_needs_defs{};
			core::SyncLinearStepAlloc<Instruction::Accessor<false>, uint32_t> accessors{};
			core::SyncLinearStepAlloc<Instruction::PrimitiveType, uint32_t> primitive_types{};
			core::SyncLinearStepAlloc<Instruction::PrimitiveTypeTerm, uint32_t> primitive_type_terms{};
			core::SyncLinearStepAlloc<Instruction::ArrayType, uint32_t> array_types{};
			core::SyncLinearStepAlloc<Instruction::ArrayRef, uint32_t> array_refs{};
			core::SyncLinearStepAlloc<Instruction::InterfaceMap, uint32_t> interface_maps{};
			core::SyncLinearStepAlloc<Instruction::TypeIDConverter, uint32_t> type_id_converters{};
			core::SyncLinearStepAlloc<Instruction::QualifiedType, uint32_t> qualified_types{};
			core::SyncLinearStepAlloc<Instruction::QualifiedTypeTerm, uint32_t> qualified_type_terms{};
			core::SyncLinearStepAlloc<Instruction::BaseTypeIdent, uint32_t> base_type_idents{};
			core::SyncLinearStepAlloc<Instruction::Ident<true>, uint32_t> ident_needs_defs{};
			core::SyncLinearStepAlloc<Instruction::Ident<false>, uint32_t> idents{};
			core::SyncLinearStepAlloc<Instruction::Intrinsic, uint32_t> intrinsics{};
			core::SyncLinearStepAlloc<Instruction::TypeThis<true>, uint32_t> type_this_needs_defs{};
			core::SyncLinearStepAlloc<Instruction::TypeThis<false>, uint32_t> type_thiss{};
			core::SyncLinearStepAlloc<Instruction::Literal, uint32_t> literals{};
			core::SyncLinearStepAlloc<Instruction::Uninit, uint32_t> uninits{};
			core::SyncLinearStepAlloc<Instruction::Zeroinit, uint32_t> zeroinits{};
			core::SyncLinearStepAlloc<Instruction::This, uint32_t> thiss{};
			core::SyncLinearStepAlloc<Instruction::TypeDeducer, uint32_t> type_deducers{};
			core::SyncLinearStepAlloc<Instruction::ExprDeducer, uint32_t> expr_deducers{};


			friend class SymbolProcBuilder;
			friend class SemanticAnalyzer;
			friend class SymbolProc;
			friend class Context;
	};


}
