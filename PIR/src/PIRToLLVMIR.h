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


#include "../include/Module.h"
#include "../include/ReaderAgent.h"

#include <llvm_interface.h>


namespace pcit::pir{

	class PIRToLLVMIR{
		public:
			PIRToLLVMIR(
				const Module& _module, llvmint::LLVMContext& llvm_context, llvmint::Module& _llvm_module
			) : module(_module), llvm_module(_llvm_module), builder(llvm_context), reader(this->module) {}
			~PIRToLLVMIR() = default;

			auto lower() -> void;


		private:
			auto lower_struct_type(const StructType& struct_type) -> void;
			auto lower_global_var(const GlobalVar& global) -> void;
			auto lower_function_decl(const FunctionDecl& func_decl) -> void;

			struct FuncLoweredSetup{
				const Function& func;
				llvmint::Function llvm_func;
			};
			auto lower_function_setup(const Function& func) -> FuncLoweredSetup;

			auto lower_func_body(const Function& func, const llvmint::Function& llvm_func) -> void;

			EVO_NODISCARD auto get_constant_value(const Expr& expr) -> llvmint::Constant;
			EVO_NODISCARD auto get_value(const Expr& expr) -> llvmint::Value;
			EVO_NODISCARD auto get_type(const Type& type) -> llvmint::Type;
			EVO_NODISCARD auto get_func_type(const Type& type) -> llvmint::FunctionType;
			EVO_NODISCARD static auto get_linkage(const Linkage& linkage) -> llvmint::LinkageType;
			EVO_NODISCARD static auto get_calling_conv(const CallingConvention& calling_conv) -> llvmint::CallingConv;
	
		private:
			const Module& module;
			llvmint::Module& llvm_module;
			llvmint::IRBuilder builder;

			ReaderAgent reader;

			std::unordered_map<std::string_view, llvmint::Type> struct_types{};
			std::unordered_map<std::string_view, llvmint::Function> funcs{};
			std::unordered_map<std::string_view, llvmint::GlobalVariable> global_vars{};
			std::unordered_map<Expr, llvmint::Value> stmt_values{};
			std::unordered_map<const Alloca*, llvmint::Alloca> allocas{};
			std::vector<llvmint::Argument> args{};
	};

}

