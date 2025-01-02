////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <Evo.h>

#include <PIR.h>

#include "../include/ASG.h"



namespace pcit::panther{

	class Source;
	class Context;

	struct ASGToPIRConfig{
		bool useReadableRegisters;
		bool checkedMath;
		bool isJIT;
		bool addSourceLocations;
	};

	
	class ASGToPIR{
		public:
			using Config = ASGToPIRConfig;

		public:
			ASGToPIR(const Context& _context, pir::Module& _module, const Config& _config) 
				: context(_context), module(_module), config(_config), agent(this->module) {}

			~ASGToPIR() = default;
		

			auto lower() -> void;
			auto lowerFunc(const ASG::Func::LinkID& func_link_id) -> void;


			auto addRuntime() -> void;

			EVO_NODISCARD auto getPIRFunctionID(const ASG::Func::LinkID& link_id) const -> const pir::Function::ID& {
				return this->get_func_info(link_id).func;
			}


		private:
			auto lower_global_var(const ASG::Var::ID& var_id) -> void;

			auto lower_func_decl(const ASG::Func::ID& func_id) -> void;
			auto lower_func_body(const ASG::Func::ID& func_id) -> void;

			auto lower_stmt(const ASG::Stmt& stmt) -> void;
			auto lower_var(const ASG::Var::ID& var_id) -> void;
			auto lower_func_call(const ASG::FuncCall& func_call) -> void;
			auto lower_intrinsic_call(const ASG::FuncCall& func_call) -> void;
			auto lower_assign(const ASG::Assign& assign) -> void;
			auto lower_multi_assign(const ASG::MultiAssign& multi_assign) -> void;
			auto lower_return(const ASG::Return& return_stmt) -> void;
			auto lower_conditional(const ASG::Conditional& conditional_stmt) -> void;
			auto lower_while(const ASG::While& while_loop) -> void;

			EVO_NODISCARD auto get_type(const TypeInfo::VoidableID& type_info_voidable_id) const -> pir::Type;
			EVO_NODISCARD auto get_type(const TypeInfo::ID& type_info_id) const -> pir::Type;
			EVO_NODISCARD auto get_type(const TypeInfo& type_info) const -> pir::Type;
			EVO_NODISCARD auto get_type(const BaseType::Primitive& primitive) const -> pir::Type;

			template<bool GET_POINTER_TO_VALUE>
			EVO_NODISCARD auto get_value(const ASG::Expr& expr) -> pir::Expr;

			EVO_NODISCARD auto get_global_var_value(const pir::Type& type, const ASG::Expr& expr) const
				-> pir::GlobalVar::Value;

			template<bool GET_POINTER_TO_VALUE>
			EVO_NODISCARD auto lower_returning_func_call(const ASG::FuncCall& func_call) -> evo::SmallVector<pir::Expr>;

			template<bool GET_POINTER_TO_VALUE>
			EVO_NODISCARD auto lower_returning_intrinsic_call(const ASG::FuncCall& func_call)
				-> evo::SmallVector<pir::Expr>;

			EVO_NODISCARD auto add_panic(std::string_view message, const ASG::Location& location) -> void;
			EVO_NODISCARD auto add_fail_assertion(
				const pir::Expr& cond,
				std::string_view block_name,
				std::string_view message,
				const ASG::Location& location
			) -> void;


			EVO_NODISCARD auto mangle_name(const ASG::Func& func) const -> std::string;
			EVO_NODISCARD auto mangle_name(const ASG::Var& var) const -> std::string;
			EVO_NODISCARD auto submangle_parent(const ASG::Parent& parent) const -> std::string;
			EVO_NODISCARD auto get_func_ident_name(const ASG::Func& func) const -> std::string;
			EVO_NODISCARD auto get_var_ident_name(const ASG::Var& func) const -> std::string;

			template<class... Args>
			EVO_NODISCARD auto stmt_name(std::format_string<Args...> fmt, Args&&... args) const -> std::string;
			EVO_NODISCARD auto stmt_name(std::string_view str) const -> std::string;


			struct FuncInfo{
				pir::Function::ID func;
				std::string mangled_name;
			};
			EVO_NODISCARD auto get_func_info(ASG::Func::LinkID link_id) const -> const FuncInfo&;
			EVO_NODISCARD auto get_current_func_info() const -> const FuncInfo&;

			struct VarInfo{
				evo::Variant<pir::Expr, pir::GlobalVar::ID> value;
			};
			EVO_NODISCARD auto get_var_info(ASG::Var::LinkID link_id) const -> const VarInfo&;

			struct ParamInfo{
				pir::Expr alloca;
				pir::Type type;
				unsigned index;
			};
			EVO_NODISCARD auto get_param_info(ASG::Param::LinkID link_id) const -> const ParamInfo&;

			struct ReturnParamInfo{
				pir::Expr param;
				pir::Type type;
				// unsigned index;
			};
			EVO_NODISCARD auto get_return_param_info(ASG::ReturnParam::LinkID link_id) const -> const ReturnParamInfo&;


			auto link_jit_std_out_if_needed() -> void;
			auto link_jit_std_err_if_needed() -> void;
			auto link_libc_puts_if_needed() -> void;


		private:
			const Context& context;
			pir::Module& module;
			pir::Agent agent;

			const Config& config;

			const Source* current_source = nullptr;
			const ASG::Func* current_func = nullptr;
			std::optional<ASG::Func::LinkID> current_func_link_id{};

			std::unordered_map<ASG::Func::LinkID, FuncInfo> func_infos{};
			std::unordered_map<ASG::Var::LinkID, VarInfo> var_infos{};
			std::unordered_map<ASG::Param::LinkID, ParamInfo> param_infos{};
			std::unordered_map<ASG::ReturnParam::LinkID, ReturnParamInfo> return_param_infos{};


			struct /* jit_links */ {
				std::optional<pir::FunctionDecl::ID> std_out{};
				std::optional<pir::FunctionDecl::ID> std_err{};
				std::optional<pir::FunctionDecl::ID> panic{};
			} jit_links;

			struct /* libc_links */ {
				std::optional<pir::FunctionDecl::ID> puts{};
			} libc_links;

			struct /* non_jit_runtime */ {
				std::optional<pir::Function::ID> panic{};
			} non_jit_runtime;

			struct /* globals */ {
				std::optional<pir::GlobalVar::ID> hello_world_str{};
				std::unordered_map<std::string, pir::GlobalVar::ID> panic_messages{};
			} globals;
	};

	
}