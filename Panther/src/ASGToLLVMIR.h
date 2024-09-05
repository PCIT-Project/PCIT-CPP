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
				bool useReadableRegisters;
			};

		public:
			ASGToLLVMIR(
				Context& _context, llvmint::LLVMContext& llvm_context, llvmint::Module& _module, const Config& _config
			) : context(_context), module(_module), builder(llvm_context), config(_config) {};

			~ASGToLLVMIR() = default;

			auto lower() -> void;

			auto addRuntime() -> void;


		private:
			auto lower_func_decl(ASG::Func::ID func_id) -> void;
			auto lower_func_body(ASG::Func::ID func_id) -> void;

			auto lower_stmt(const ASG::Stmt& stmt) -> void;
			auto lower_var(const ASG::Var::ID var_id) -> void;
			auto lower_func_call(const ASG::FuncCall& func_call) -> void;
			auto lower_assign(const ASG::Assign& assign) -> void;
			auto lower_multi_assign(const ASG::MultiAssign& multi_assign) -> void;
			auto lower_return(const ASG::Return& return_stmt) -> void;



			EVO_NODISCARD auto get_type(const TypeInfo::VoidableID& type_info_voidable_id) const -> llvmint::Type;
			EVO_NODISCARD auto get_type(const TypeInfo::ID& type_info_id) const -> llvmint::Type;
			EVO_NODISCARD auto get_type(const TypeInfo& type_info) const -> llvmint::Type;
			EVO_NODISCARD auto get_func_type(const BaseType::Function& func_type) const -> llvmint::FunctionType;

			// TODO: make get_pointer_to_value a template?
			EVO_NODISCARD auto get_concrete_value(const ASG::Expr& expr) -> llvmint::Value;
			EVO_NODISCARD auto get_value(const ASG::Expr& expr, bool get_pointer_to_value = false) -> llvmint::Value;
			EVO_NODISCARD auto lower_returning_func_call(const ASG::FuncCall& func_call, bool get_pointer_to_value)
				-> evo::SmallVector<llvmint::Value>;


			EVO_NODISCARD auto mangle_name(const ASG::Func& func) const -> std::string;
			EVO_NODISCARD auto submangle_parent(const ASG::Parent& parent) const -> std::string;
			EVO_NODISCARD auto get_func_ident_name(const ASG::Func& func) const -> std::string;

			template<class... Args>
			EVO_NODISCARD auto stmt_name(std::format_string<Args...> fmt, Args&&... args) const -> std::string;
			EVO_NODISCARD auto stmt_name(std::string_view str) const -> std::string;

			struct FuncInfo{
				llvmint::Function func;
			};
			EVO_NODISCARD auto get_func_info(ASG::Func::LinkID link_id) const -> const FuncInfo&;

			struct VarInfo{
				llvmint::Alloca alloca;
			};
			EVO_NODISCARD auto get_var_info(ASG::Var::LinkID link_id) const -> const VarInfo&;

			struct ParamInfo{
				llvmint::Alloca alloca;
				llvmint::Type type;
				evo::uint index;
			};
			EVO_NODISCARD auto get_param_info(ASG::Param::LinkID link_id) const -> const ParamInfo&;

			struct ReturnParamInfo{
				llvmint::Argument arg;
				llvmint::Type type;
				evo::uint index;
			};
			EVO_NODISCARD auto get_return_param_info(ASG::ReturnParam::LinkID link_id) const -> const ReturnParamInfo&;


			EVO_NODISCARD auto get_value_size(uint64_t val) const -> llvmint::ConstantInt;
	
		private:
			Context& context;
			llvmint::Module& module;

			llvmint::IRBuilder builder;

			Config config;

			Source* current_source = nullptr;
			const ASG::Func* current_func = nullptr;
			

			std::unordered_map<ASG::Func::LinkID, FuncInfo> func_infos{};
			std::unordered_map<ASG::Var::LinkID, VarInfo> var_infos{};
			std::unordered_map<ASG::Param::LinkID, ParamInfo> param_infos{};
			std::unordered_map<ASG::ReturnParam::LinkID, ReturnParamInfo> return_param_infos{};
	};

	
}