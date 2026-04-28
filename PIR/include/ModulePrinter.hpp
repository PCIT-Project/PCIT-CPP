////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.hpp>
#include <PCIT_core.hpp>

#include "./forward_decl_ids.hpp"
#include "./Expr.hpp"
#include "./GlobalVar.hpp"
#include "./InstrReader.hpp"
#include "./meta.hpp"

namespace pcit::pir{


	class ModulePrinter{
		public:
			ModulePrinter(const class Module& module, core::Printer& _printer) : reader(module), printer(_printer) {}
			~ModulePrinter() = default;

			auto print() -> void;

			auto printFunction(const class Function& function) -> void;
			auto printExternalFunction(const struct ExternalFunction& external_function) -> void;
			auto printStructType(const struct StructType& struct_type) -> void;
			auto printGlobalVar(const struct GlobalVar& global_var) -> void;
			auto printGlobalVarValue(const GlobalVar::Value& global_var_value) -> void;
			auto printBasicBlock(const class BasicBlock& basic_block) -> void;
			auto printType(const class Type& type) -> void;

		private:
			auto print_expr(Expr expr) -> void;
			auto print_expr_stmt(Expr expr) -> void;

			auto print_meta_file(const meta::File& file) -> void;
			auto print_meta_basic_type(const meta::BasicType& type) -> void;
			auto print_meta_qualified_type(const meta::QualifiedType& qualified_type) -> void;
			auto print_meta_struct_type(const meta::StructType& struct_type) -> void;
			auto print_meta_union_type(const meta::UnionType& union_type) -> void;
			auto print_meta_array_type(const meta::ArrayType& array_type) -> void;
			auto print_meta_enum_type(const meta::EnumType& enum_type) -> void;
			auto print_meta_function(const meta::Function& function) -> void;
			auto print_meta_type_id(meta::Type meta_type) -> void;
			auto print_meta_file_id(meta::File::ID meta_file_id) -> void;
			auto print_source_location(const std::optional<meta::SourceLocation>& source_location) -> void;
			auto print_meta_scope(const meta::Scope& scope) -> void;

			auto print_function_call_impl(
				const evo::Variant<FunctionID, ExternalFunctionID, PtrCall>& call_target,
				evo::ArrayProxy<Expr> args,
				const std::optional<meta::SourceLocation>& source_location,
				bool is_no_return
			) -> void;

			auto print_function_decl_impl(const struct FuncDeclRef& func_decl) -> void;

			auto print_non_standard_name(std::string_view, bool is_declaration) -> void;

			auto print_atomic_ordering(AtomicOrdering ordering) -> void;
			auto print_calling_convention(CallingConvention convention) -> void;
			auto print_linkage(Linkage linkage) -> void;


			[[nodiscard]] auto get_module() const -> const Module& { return this->reader.getModule(); }
			[[nodiscard]] auto get_current_func() const -> const Function& { return this->reader.getTargetFunction(); }
	
		private:
			InstrReader reader;
			core::Printer& printer;
	};


	inline auto printModule(const class Module& module, core::Printer& printer) -> void {
		ModulePrinter(module, printer).print();
	}


}

