////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <stack>

#include <Evo.h>
#include <PCIT_core.h>
#include <PIR.h>

#include "../../include/sema/sema.h"
#include "./SemaToPIRData.h"


namespace pcit::panther{


	class SemaToPIR{
		public:
			using Data = SemaToPIRData;

		public:
			SemaToPIR(class Context& _context, pir::Module& _module, Data& _data)
				: context(_context), module(_module), agent(_module), data(_data) {}
			~SemaToPIR() = default;

			auto lower() -> void;

			auto lowerStruct(BaseType::Struct::ID struct_id) -> void;
			auto lowerGlobal(sema::GlobalVar::ID global_var_id) -> void;
			auto lowerFuncDecl(sema::Func::ID func_id) -> pir::Function::ID;
			auto lowerFuncDef(sema::Func::ID func_id) -> void;
			
			auto createJITEntry(sema::Func::ID target_entry_func) -> pir::Function::ID;

			EVO_NODISCARD auto createFuncJITInterface(sema::Func::ID func_id, pir::Function::ID pir_func_id)
				-> pir::Function::ID;


		private:
			auto lower_stmt(const sema::Stmt& stmt) -> void;

			EVO_NODISCARD auto get_expr_register(const sema::Expr expr) -> pir::Expr;
			EVO_NODISCARD auto get_expr_pointer(const sema::Expr expr) -> pir::Expr;
			EVO_NODISCARD auto get_expr_store(const sema::Expr expr, evo::ArrayProxy<pir::Expr> store_locations)
				-> void;

			enum class GetExprMode{
				REGISTER,
				POINTER,
				STORE,
			};

			template<GetExprMode MODE>
			auto get_expr_impl(const sema::Expr expr, evo::ArrayProxy<pir::Expr> store_locations)
				-> std::optional<pir::Expr>;

			template<GetExprMode MODE>
			auto template_intrinsic_func_call(
				const sema::FuncCall& func_call, evo::ArrayProxy<pir::Expr> store_locations
			) -> std::optional<pir::Expr>;

			auto intrinsic_func_call(const sema::FuncCall& func_call) -> void;

			EVO_NODISCARD auto get_global_var_value(const sema::Expr expr) -> pir::GlobalVar::Value;

			EVO_NODISCARD auto get_type(const TypeInfo::VoidableID voidable_type_id) -> pir::Type;
			EVO_NODISCARD auto get_type(const TypeInfo::ID type_id) -> pir::Type;
			EVO_NODISCARD auto get_type(const TypeInfo& type_info) -> pir::Type;
			EVO_NODISCARD auto get_type(const BaseType::ID base_type_id) -> pir::Type;

			EVO_NODISCARD auto mangle_name(const BaseType::Struct::ID struct_id) const -> std::string;
			EVO_NODISCARD auto mangle_name(const sema::GlobalVar::ID global_var_id) const -> std::string;
			EVO_NODISCARD auto mangle_name(const sema::Func::ID func_id) const -> std::string;


			EVO_NODISCARD auto name(std::string_view str) const -> std::string;
			
			template<class... Args>
			EVO_NODISCARD auto name(std::format_string<Args...> fmt, Args&&... args) const -> std::string;


	
		private:
			class Context& context;
			pir::Module& module;
			pir::Agent agent;

			class Source* current_source = nullptr;

			Data& data;
	};


}
