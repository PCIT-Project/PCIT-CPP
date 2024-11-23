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

#include "./forward_decl_ids.h"
#include "./Expr.h"

namespace pcit::pir{


	class ModulePrinter{
		public:
			ModulePrinter(const class Module& _module, core::Printer& _printer) : module(_module), printer(_printer) {}
			~ModulePrinter() = default;

			auto print() -> void;

		private:
			auto print_function(const class Function& function) -> void;
			auto print_function_decl(const struct FunctionDecl& function_decl) -> void;
			auto print_struct_type(const struct StructType& struct_type) -> void;
			auto print_global_var(const struct GlobalVar& global_var) -> void;
			auto print_basic_block(const class BasicBlock& basic_block) -> void;
			auto print_type(const class Type& type) -> void;
			auto print_expr(const Expr& expr) -> void;
			auto print_expr_stmt(const Expr& expr) -> void;

			auto print_function_call_impl(
				const evo::Variant<FunctionID, FunctionDeclID, PtrCall>& call_target, evo::ArrayProxy<Expr> args
			) -> void;

			auto print_function_decl_impl(const struct FuncDeclRef& func_decl) -> void;

			auto print_non_standard_name(std::string_view) -> void;

			auto print_atomic_ordering(AtomicOrdering ordering) -> void;

	
		private:
			const class Module& module;
			core::Printer& printer;

			const class Function* func = nullptr;
	};


	inline auto printModule(const class Module& module, core::Printer& printer) -> void {
		ModulePrinter(module, printer).print();
	}


}

