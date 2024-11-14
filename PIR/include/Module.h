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

#include "./Function.h"
#include "./GlobalVar.h"


namespace pcit::pir{


	class Module{
		public:
			// Module(std::string_view _name) : name(_name) {}
			Module(std::string&& _name) : name(std::move(_name)) {}
			~Module() = default;


			EVO_NODISCARD auto getName() const -> std::string_view { return this->name; }


			// Only supports constants
			EVO_NODISCARD auto getExprType(const Expr& expr) const -> Type;


			///////////////////////////////////
			// function

			EVO_NODISCARD auto createFunction(
				std::string&& func_name,
				evo::SmallVector<Parameter>&& parameters,
				CallingConvention callingConvention,
				Linkage linkage,
				Type returnType
			) -> Function::ID {
				#if defined(PCIT_CONFIG_DEBUG)
					this->check_param_names(parameters);
					this->check_global_name_reusue(func_name);
				#endif

				return this->functions.emplace_back(
					*this,
					FunctionDecl(std::move(func_name), std::move(parameters), callingConvention, linkage, returnType)
				);
			}

			EVO_NODISCARD auto getFunction(Function::ID id) const -> const Function& {
				return this->functions[id];
			}

			EVO_NODISCARD auto getFunction(Function::ID id) -> Function& {
				return this->functions[id];
			}



			using FunctionIter = core::StepAlloc<Function, Function::ID>::Iter;
			using FunctionsConstIter = core::StepAlloc<Function, Function::ID>::ConstIter;

			EVO_NODISCARD auto getFunctionIter() -> core::IterRange<FunctionIter> {
				return core::IterRange<FunctionIter>(this->functions.begin(), this->functions.end());
			}

			EVO_NODISCARD auto getFunctionIter() const -> core::IterRange<FunctionsConstIter> {
				return core::IterRange<FunctionsConstIter>(this->functions.cbegin(), this->functions.cend());
			}

			EVO_NODISCARD auto getFunctionsConstIter() const -> core::IterRange<FunctionsConstIter> {
				return core::IterRange<FunctionsConstIter>(this->functions.cbegin(), this->functions.cend());
			}


			///////////////////////////////////
			// function declaration

			EVO_NODISCARD auto createFunctionDecl(
				std::string&& func_name,
				evo::SmallVector<Parameter>&& parameters,
				CallingConvention callingConvention,
				Linkage linkage,
				Type returnType
			) -> FunctionDecl::ID {
				#if defined(PCIT_CONFIG_DEBUG)
					this->check_param_names(parameters);
					this->check_global_name_reusue(func_name);
				#endif

				return this->function_decls.emplace_back(
					std::move(func_name), std::move(parameters), callingConvention, linkage, returnType
				);
			}

			EVO_NODISCARD auto getFunctionDecl(FunctionDecl::ID id) const -> const FunctionDecl& {
				return this->function_decls[id];
			}

			EVO_NODISCARD auto getFunctionDecl(FunctionDecl::ID id) -> FunctionDecl& {
				return this->function_decls[id];
			}


			using FunctionDeclIter = core::StepAlloc<FunctionDecl, FunctionDecl::ID>::Iter;
			using FunctionDeclsConstIter = core::StepAlloc<FunctionDecl, FunctionDecl::ID>::ConstIter;

			EVO_NODISCARD auto getFunctionDeclIter() -> core::IterRange<FunctionDeclIter> {
				return core::IterRange<FunctionDeclIter>(
					this->function_decls.begin(), this->function_decls.end()
				);
			}

			EVO_NODISCARD auto getFunctionDeclIter() const -> core::IterRange<FunctionDeclsConstIter> {
				return core::IterRange<FunctionDeclsConstIter>(
					this->function_decls.cbegin(), this->function_decls.cend()
				);
			}

			EVO_NODISCARD auto getFunctionDeclsConstIter() const -> core::IterRange<FunctionDeclsConstIter> {
				return core::IterRange<FunctionDeclsConstIter>(
					this->function_decls.cbegin(), this->function_decls.cend()
				);
			}


			///////////////////////////////////
			// global

			EVO_NODISCARD auto createGlobalVar(
				std::string&& global_name,
				Type type,
				Linkage linkage,
				evo::Variant<Expr, GlobalVar::Zeroinit, GlobalVar::Uninit> value,
				bool isConstant,
				bool isExternal
			) -> GlobalVar::ID {
				#if defined(EVO_CONFIG_DEBUG)
					if(value.is<Expr>()){
						evo::debugAssert(type == this->getExprType(value.as<Expr>()), "Type and value must match");
						evo::debugAssert(value.as<Expr>().isConstant(), "Global can only have a constant value");
					}
					this->check_global_name_reusue(global_name);
				#endif

				return this->global_vars.emplace_back(
					std::move(global_name), type, linkage, value, isConstant, isExternal
				);
			}

			EVO_NODISCARD auto getGlobalVar(GlobalVar::ID id) const -> const GlobalVar& {
				return this->global_vars[id];
			}

			EVO_NODISCARD auto getGlobalVar(GlobalVar::ID id) -> GlobalVar& {
				return this->global_vars[id];
			}


			using GlobalVarIter = core::StepAlloc<GlobalVar, GlobalVar::ID>::Iter;
			using GlobalVarsConstIter = core::StepAlloc<GlobalVar, GlobalVar::ID>::ConstIter;

			EVO_NODISCARD auto getGlobalVarIter() -> core::IterRange<GlobalVarIter> {
				return core::IterRange<GlobalVarIter>(this->global_vars.begin(), this->global_vars.end());
			}

			EVO_NODISCARD auto getGlobalVarIter() const -> core::IterRange<GlobalVarsConstIter> {
				return core::IterRange<GlobalVarsConstIter>(this->global_vars.cbegin(), this->global_vars.cend());
			}

			EVO_NODISCARD auto getGlobalVarsConstIter() const -> core::IterRange<GlobalVarsConstIter> {
				return core::IterRange<GlobalVarsConstIter>(this->global_vars.cbegin(), this->global_vars.cend());
			}




			///////////////////////////////////
			// types

			EVO_NODISCARD static auto createTypeVoid() -> Type { return Type(Type::Kind::Void); }

			EVO_NODISCARD static auto createTypePtr() -> Type { return Type(Type::Kind::Ptr); }

			EVO_NODISCARD static auto createTypeSigned(uint32_t width) -> Type {
				evo::debugAssert(
					width != 0 && width < 1 << 23,
					"Invalid width for a signed ({})", width
				);
				return Type(Type::Kind::Signed, width);
			}

			EVO_NODISCARD static auto createTypeUnsigned(uint32_t width) -> Type {
				evo::debugAssert(
					width != 0 && width < 1 << 23,
					"Invalid width for an unsigned ({})", width
				);
				return Type(Type::Kind::Unsigned, width);
			}

			EVO_NODISCARD static auto createTypeFloat(uint32_t width) -> Type {
				evo::debugAssert(
					width == 16 || width == 32 || width == 64 || width == 80 || width == 128,
					"Invalid width for a float ({})", width
				);
				return Type(Type::Kind::Float, width);
			}

			EVO_NODISCARD static auto createTypeBFloat() -> Type { return Type(Type::Kind::BFloat); }



			EVO_NODISCARD auto createTypeArray(auto&&... args) -> Type {
				const uint32_t array_type_index = this->array_types.emplace_back(std::forward<decltype(args)>(args)...);
				return Type(Type::Kind::Array, array_type_index);
			}

			EVO_NODISCARD auto getTypeArray(const Type& arr_type) const -> const ArrayType& {
				evo::debugAssert(arr_type.getKind() == Type::Kind::Array, "Not an array");
				return this->array_types[arr_type.number];
			}



			EVO_NODISCARD auto createTypeStruct(
				std::string&& struct_name, evo::SmallVector<Type> members, bool is_packed
			) -> Type {
				#if defined(PCIT_CONFIG_DEBUG)
					this->check_global_name_reusue(struct_name);
				#endif

				const uint32_t struct_type_index = this->struct_types.emplace_back(
					std::move(struct_name), members, is_packed
				);
				return Type(Type::Kind::Struct, struct_type_index);
			}

			EVO_NODISCARD auto getTypeStruct(const Type& struct_type) const -> const StructType& {
				evo::debugAssert(struct_type.getKind() == Type::Kind::Struct, "Not a struct");
				return this->struct_types[struct_type.number];
			}


			using StructTypeIter = core::LinearStepAlloc<StructType, uint32_t>::Iter;
			using StructTypeConstIter = core::LinearStepAlloc<StructType, uint32_t>::ConstIter;

			EVO_NODISCARD auto getStructTypeIter() -> core::IterRange<StructTypeIter> {
				return core::IterRange<StructTypeIter>(
					this->struct_types.begin(), this->struct_types.end()
				);
			}

			EVO_NODISCARD auto getStructTypeIter() const -> core::IterRange<StructTypeConstIter> {
				return core::IterRange<StructTypeConstIter>(
					this->struct_types.cbegin(), this->struct_types.cend()
				);
			}

			EVO_NODISCARD auto getStructTypeConstIter() const -> core::IterRange<StructTypeConstIter> {
				return core::IterRange<StructTypeConstIter>(
					this->struct_types.cbegin(), this->struct_types.cend()
				);
			}



			EVO_NODISCARD auto createFunctionType(auto&&... args) -> Type {
				const uint32_t array_type_index = this->func_types.emplace_back(std::forward<decltype(args)>(args)...);
				return Type(Type::Kind::Function, array_type_index);
			}

			EVO_NODISCARD auto getFunctionType(const Type& func_type) const -> const FunctionType& {
				evo::debugAssert(func_type.getKind() == Type::Kind::Function, "Not an function");
				return this->func_types[func_type.number];
			}

		private:
			#if defined(PCIT_CONFIG_DEBUG)
				auto check_param_names(evo::ArrayProxy<Parameter> params) const -> void;

				auto check_global_name_reusue(std::string_view global_name) const -> void;
			#endif
	
		private:
			std::string name;

			core::StepAlloc<Function, Function::ID> functions{};
			core::StepAlloc<FunctionDecl, FunctionDecl::ID> function_decls{};
			core::StepAlloc<GlobalVar, GlobalVar::ID> global_vars{};

			core::StepAlloc<BasicBlock, BasicBlock::ID> basic_blocks{};
			core::StepAlloc<Number, uint32_t> numbers{};

			core::LinearStepAlloc<ArrayType, uint32_t> array_types{};
			core::LinearStepAlloc<StructType, uint32_t> struct_types{};
			core::LinearStepAlloc<FunctionType, uint32_t> func_types{};

			// exprs
			core::StepAlloc<Call, uint32_t> calls{};
			core::StepAlloc<CallVoid, uint32_t> call_voids{};
			core::StepAlloc<Ret, uint32_t> rets{};
			core::StepAlloc<Add, uint32_t> adds{};

			friend class ReaderAgent;
			friend class Agent;
	};


}


