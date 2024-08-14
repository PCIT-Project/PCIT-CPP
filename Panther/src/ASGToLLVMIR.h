//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once

#include <Evo.h>

#include <llvm_interface.h>

#include "../include/ASG.h"


namespace pcit::panther{

	class Source;
	class Context;

	class ASGToLLVMIR{
		public:
			struct Config{
				bool optimize;
			};

		public:
			ASGToLLVMIR(
				Context& _context, llvmint::LLVMContext& llvm_context, llvmint::Module& _module, const Config& _config
			) : context(_context), module(_module), builder(llvm_context), config(_config) {};

			~ASGToLLVMIR() = default;

			auto lower() -> void;

		private:
			auto lower_func_decl(ASG::Func::ID func_id) -> void;
			auto lower_func_body(ASG::Func::ID func_id) -> void;

			auto lower_stmt(const ASG::Stmt& stmt) -> void;
			auto lower_var(const ASG::Var& var) -> void;
			auto lower_func_call(const ASG::FuncCall& func_call) -> void;


			EVO_NODISCARD auto get_type(const TypeInfo::ID& type_info_id) const -> llvmint::Type;
			EVO_NODISCARD auto get_type(const TypeInfo& type_info) const -> llvmint::Type;

			// TODO: make get_pointer_to_value a template?
			EVO_NODISCARD auto get_value(const ASG::Expr& expr, bool get_pointer_to_value = false) const 
				-> llvmint::Value;


			EVO_NODISCARD auto mangle_name(const ASG::Func& func) const -> std::string;
			EVO_NODISCARD auto submangle_parent(const ASG::Parent& parent) const -> std::string;
			EVO_NODISCARD auto get_func_ident_name(const ASG::Func& func) const -> std::string;

			template<class... Args>
			EVO_NODISCARD auto stmt_name(std::format_string<Args...> fmt, Args&&... args) const -> std::string;
	
		private:
			Context& context;
			llvmint::Module& module;

			llvmint::IRBuilder builder;

			Config config;

			Source* current_source = nullptr;

			struct FuncInfo{
				llvmint::Function func;
			};

			std::unordered_map<ASG::Func::LinkID, FuncInfo> func_infos{};
	};

	
}