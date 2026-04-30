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


#include "../include/Module.hpp"
#include "../include/InstrReader.hpp"

#include <llvm_interface.hpp>


namespace pcit::pir{

	class PIRToLLVMIR{
		public:
			PIRToLLVMIR(
				const Module& _module,
				llvmint::LLVMContext& llvm_context,
				llvmint::Module& _llvm_module,
				bool _add_debug_info
			) : 
				module(_module),
				llvm_module(_llvm_module),
				builder(llvm_context),
				di_builder(_llvm_module),
				reader(this->module),
				add_debug_info(_add_debug_info)
			{}

			~PIRToLLVMIR() = default;

			auto lower() -> void;

			struct Subset{
				evo::ArrayProxy<Type> structs;
				evo::ArrayProxy<GlobalVar::ID> globalVars;
				evo::ArrayProxy<GlobalVar::ID> globalVarDecls;
				evo::ArrayProxy<ExternalFunction::ID> externFuncs;
				evo::ArrayProxy<Function::ID> funcDecls;
				evo::ArrayProxy<Function::ID> funcs;
			};
			auto lowerSubset(const Subset& subset) -> void;
			auto lowerSubsetWithWeakDependencies(const Subset& subset) -> void;


			auto addModuleLevelDebugInfo() -> void;


		private:
			auto lower_meta_file(meta::File::ID meta_file_id) -> void;
			auto lower_meta_subscope(meta::Subscope::ID meta_subscope_id) -> void;
			auto lower_meta_basic_type(meta::BasicType::ID meta_basic_type_id) -> void;
			auto lower_meta_qualified_type(meta::QualifiedType::ID meta_qualified_type_id)
				-> llvmint::DIBuilder::DerivedType;
			auto lower_meta_struct_type(meta::StructType::ID meta_struct_type_id) -> llvmint::DIBuilder::CompositeType;
			auto lower_meta_union_type(meta::UnionType::ID meta_union_type_id) -> llvmint::DIBuilder::CompositeType;
			auto lower_meta_array_type(meta::ArrayType::ID meta_array_type_id) -> llvmint::DIBuilder::CompositeType;
			auto lower_meta_enum_type(meta::EnumType::ID meta_enum_type_id) -> llvmint::DIBuilder::CompositeType;
			auto lower_meta_function(std::string_view func_name, meta::Function::ID meta_function_id)
				-> llvmint::DIBuilder::Subprogram;

			auto lower_meta_source_location(meta::SourceLocation source_location) -> llvmint::DIBuilder::Location;


			template<bool ADD_WEAK_DEPS>
			auto lower_subset_impl(const Subset& subset) -> void;

			template<bool ADD_WEAK_DEPS>
			auto lower_struct_type(const pir::StructType& struct_type) -> void;

						
			template<bool ADD_WEAK_DEPS>
			auto lower_global_var_decl(const GlobalVar& global) -> void;

			template<bool ADD_WEAK_DEPS>
			auto lower_global_var_def(const GlobalVar& global) -> void;
			
			template<bool ADD_WEAK_DEPS>
			auto lower_external_func(const ExternalFunction& external_func) -> void;
			
			template<bool ADD_WEAK_DEPS>
			auto lower_function_decl(const Function& func) -> void;

			struct FuncLoweredSetup{
				const Function& func;
				llvmint::Function llvm_func;
			};
			template<bool ADD_WEAK_DEPS>
			auto lower_function_setup(const Function& func) -> FuncLoweredSetup;

			template<bool ADD_WEAK_DEPS>
			auto lower_func_body(const Function& func, const llvmint::Function& llvm_func) -> void;

			template<bool ADD_WEAK_DEPS>
			[[nodiscard]] auto get_constant_value(const Expr& expr) -> llvmint::Constant;

			template<bool ADD_WEAK_DEPS>
			[[nodiscard]] auto get_global_var_value(const GlobalVar::Value& global_var_value, const Type& type)
				-> llvmint::Constant;

			template<bool ADD_WEAK_DEPS>
			[[nodiscard]] auto get_value(const Expr& expr) -> llvmint::Value;

			template<bool ADD_WEAK_DEPS>
			[[nodiscard]] auto get_type(const Type& type) -> llvmint::Type;

			template<bool ADD_WEAK_DEPS>
			[[nodiscard]] auto get_func_type(const Type& type) -> llvmint::FunctionType;

			template<bool ADD_WEAK_DEPS>
			[[nodiscard]] auto get_struct_type(const StructType& type) -> llvmint::StructType;

			template<bool ADD_WEAK_DEPS> [[nodiscard]] auto get_func(const Function& func) -> llvmint::Function;
			template<bool ADD_WEAK_DEPS> [[nodiscard]] auto get_func(const ExternalFunction& func) -> llvmint::Function;

			template<bool ADD_WEAK_DEPS>
			[[nodiscard]] auto get_global_var(const GlobalVar& global_var) -> llvmint::GlobalVariable;


			[[nodiscard]] auto get_meta_scope(meta::Scope scope) -> llvmint::DIBuilder::Scope;
			[[nodiscard]] auto get_meta_local_scope(meta::LocalScope local_scope) -> llvmint::DIBuilder::LocalScope;
			[[nodiscard]] auto get_meta_type(meta::Type type) -> llvmint::DIBuilder::Type;


			[[nodiscard]] static auto get_linkage(const Linkage& linkage) -> llvmint::LinkageType;
			[[nodiscard]] static auto get_calling_conv(const CallingConvention& calling_conv) -> llvmint::CallingConv;
			[[nodiscard]] static auto get_atomic_ordering(const AtomicOrdering& atomic_ordering)
				-> llvmint::AtomicOrdering;
	
		private:
			const Module& module;
			llvmint::Module& llvm_module;
			llvmint::IRBuilder builder;
			llvmint::DIBuilder di_builder;

			InstrReader reader;
			bool add_debug_info;

			std::unordered_map<const StructType*, llvmint::StructType> struct_types{};
			std::unordered_map<const void*, llvmint::Function> funcs{}; // void* for funcs and extern funcs
			std::unordered_map<const GlobalVar*, llvmint::GlobalVariable> global_vars{};
			std::unordered_map<Expr, llvmint::Value> stmt_values{};
			std::unordered_map<const Alloca*, llvmint::Alloca> allocas{};
			evo::SmallVector<llvmint::Argument, 8> args{};

			std::unordered_map<meta::File::ID, llvmint::DIBuilder::File> meta_files{};
			std::unordered_map<meta::Subscope::ID, llvmint::DIBuilder::LocalScope> meta_subscopes{};
			std::unordered_map<meta::BasicType::ID, llvmint::DIBuilder::BasicType> meta_basic_types{};
			std::unordered_map<meta::QualifiedType::ID, llvmint::DIBuilder::DerivedType> meta_qualified_types{};
			std::unordered_map<meta::Function::ID, llvmint::DIBuilder::Subprogram> meta_functions{};
			std::unordered_map<meta::StructType::ID, llvmint::DIBuilder::CompositeType> meta_struct_types{};
			std::unordered_map<meta::UnionType::ID, llvmint::DIBuilder::CompositeType> meta_union_types{};
			std::unordered_map<meta::ArrayType::ID, llvmint::DIBuilder::CompositeType> meta_array_types{};
			std::unordered_map<meta::EnumType::ID, llvmint::DIBuilder::CompositeType> meta_enum_types{};
			bool added_compile_unit = false;
	};

}

