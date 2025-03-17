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

#include "../../include/Context.h"
#include "../../include/sema/sema.h"


namespace pcit::panther{


	class SemaToPIR{
		public:
			struct Config{
				bool useReadableNames;
				bool checkedMath;
				bool isJIT;
				bool addSourceLocations;
			};

		public:
			SemaToPIR(Context& _context, pir::Module& _module, Config&& _config)
				: context(_context), module(_module), agent(_module), config(_config) {}
			~SemaToPIR() = default;

			auto lower() -> void;


		private:
			auto lower_struct(const BaseType::Struct::ID struct_id) -> void;
			auto lower_global(const sema::GlobalVar::ID global_var_id) -> void;
			auto lower_func_decl(const sema::Func::ID func_id) -> void;
			auto lower_func_def(const sema::Func::ID func_id) -> void;

			auto lower_stmt(const sema::Stmt& stmt) -> void;

			EVO_NODISCARD auto get_expr(const sema::Expr expr) -> pir::Expr;
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
			Context& context;
			pir::Module& module;
			pir::Agent agent;
			Config config;

			Source* current_source = nullptr;

			evo::SmallVector<pir::Type> structs{};
			evo::SmallVector<pir::GlobalVar::ID> global_vars{};
			evo::SmallVector<pir::Function::ID> funcs{};
	};


}
