//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
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

	struct ASGToLLVMIRConfig{
		bool useReadableRegisters;
		bool checkedArithmetic;
		bool isJIT;
		bool addSourceLocations;
	};

	class ASGToLLVMIR{
		public:
			using Config = ASGToLLVMIRConfig;

		public:
			ASGToLLVMIR(
				Context& _context, llvmint::LLVMContext& llvm_context, llvmint::Module& _module, const Config& _config
			) : context(_context), module(_module), builder(llvm_context), config(_config) {
				this->add_links();
				if(this->config.isJIT){ this->add_links_for_JIT(); }
			};

			~ASGToLLVMIR() = default;

			auto lower() -> void;
			auto lowerFunc(const ASG::Func::LinkID& func_link_id) -> void; // TODO: Do I need to add mutexes to
																		// synchronize this with recreating engine in
																		// ComptimeExecutor? I don't think it's needed

			auto addRuntime() -> void;


			EVO_NODISCARD auto getFuncMangledName(ASG::Func::LinkID link_id) -> std::string_view;


		private:
			auto add_links() -> void;
			auto add_links_for_JIT() -> void;

			auto lower_global_var(const ASG::Var::ID& var_id) -> void;

			auto lower_func_decl(const ASG::Func::ID& func_id) -> void;
			auto lower_func_body(const ASG::Func::ID& func_id) -> void;

			auto lower_stmt(const ASG::Stmt& stmt) -> void;
			auto lower_var(const ASG::Var::ID& var_id) -> void;
			auto lower_func_call(const ASG::FuncCall& func_call) -> void;
			auto lower_assign(const ASG::Assign& assign) -> void;
			auto lower_multi_assign(const ASG::MultiAssign& multi_assign) -> void;
			auto lower_return(const ASG::Return& return_stmt) -> void;
			auto lower_conditional(const ASG::Conditional& conditional_stmt) -> void;



			EVO_NODISCARD auto get_type(const TypeInfo::VoidableID& type_info_voidable_id) const -> llvmint::Type;
			EVO_NODISCARD auto get_type(const TypeInfo::ID& type_info_id) const -> llvmint::Type;
			EVO_NODISCARD auto get_type(const TypeInfo& type_info) const -> llvmint::Type;
			EVO_NODISCARD auto get_type(const BaseType::Primitive& primitive) const -> llvmint::Type;

			EVO_NODISCARD auto get_func_type(const BaseType::Function& func_type) const -> llvmint::FunctionType;

			// TODO: make get_pointer_to_value a template?
			EVO_NODISCARD auto get_concrete_value(const ASG::Expr& expr) -> llvmint::Value;
			EVO_NODISCARD auto get_value(const ASG::Expr& expr, bool get_pointer_to_value = false) -> llvmint::Value;
			EVO_NODISCARD auto get_constant_value(const ASG::Expr& expr) -> llvmint::Constant;
			EVO_NODISCARD auto lower_returning_func_call(const ASG::FuncCall& func_call, bool get_pointer_to_value)
				-> evo::SmallVector<llvmint::Value>;
			EVO_NODISCARD auto lower_returning_intrinsic_call(const ASG::FuncCall& func_call, bool get_pointer_to_value)
				-> evo::SmallVector<llvmint::Value>;


			EVO_NODISCARD auto add_panic(std::string_view message, const ASG::Location& location) -> void;
			EVO_NODISCARD auto add_assertion(
				const llvmint::Value& cond,
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
				llvmint::Function func;
				std::string mangled_name;
			};
			EVO_NODISCARD auto get_func_info(ASG::Func::LinkID link_id) const -> const FuncInfo&;
			EVO_NODISCARD auto get_current_func_info() const -> const FuncInfo&;

			struct VarInfo{
				evo::Variant<llvmint::Alloca, llvmint::GlobalVariable> value;
			};
			EVO_NODISCARD auto get_var_info(ASG::Var::LinkID link_id) const -> const VarInfo&;

			struct ParamInfo{
				llvmint::Alloca alloca;
				llvmint::Type type;
				unsigned index;
			};
			EVO_NODISCARD auto get_param_info(ASG::Param::LinkID link_id) const -> const ParamInfo&;

			struct ReturnParamInfo{
				llvmint::Argument arg;
				llvmint::Type type;
				unsigned index;
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
			std::optional<ASG::Func::LinkID> current_func_link_id{};

			std::unordered_map<ASG::Func::LinkID, FuncInfo> func_infos{};
			std::unordered_map<ASG::Var::LinkID, VarInfo> var_infos{};
			std::unordered_map<ASG::Param::LinkID, ParamInfo> param_infos{};
			std::unordered_map<ASG::ReturnParam::LinkID, ReturnParamInfo> return_param_infos{};

			struct LinkedFunctions{
				std::optional<llvmint::Function> print_hello_world{};

				// for JIT
				std::optional<llvmint::Function> panic{};
				std::optional<llvmint::Function> panic_with_location{};
			} linked_functions;
	};

	
}