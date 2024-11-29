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
			Module(
				std::string&& _name, core::OS _os, core::Architecture _arch
			) : name(std::move(_name)), os(_os), arch(_arch) {}
			~Module() = default;

			EVO_NODISCARD auto getName() const -> std::string_view { return this->name; }
			EVO_NODISCARD auto getOS() const -> core::OS { return this->os; }
			EVO_NODISCARD auto getArchitecture() const -> core::Architecture { return this->arch; }



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
			// global values

			EVO_NODISCARD auto createGlobalString(std::string&& string) -> GlobalVar::String::ID {
				const Type str_type = this->createArrayType(this->createSignedType(8), string.size() + 1);
				return this->global_strings.emplace_back(std::move(string), str_type);
			}


			EVO_NODISCARD auto getGlobalString(GlobalVar::String::ID id) const -> const GlobalVar::String& {
				return this->global_strings[id];
			}



			EVO_NODISCARD auto createGlobalArray(
				Type element_type, std::vector<GlobalVar::Value> values
			) -> GlobalVar::Array::ID {
				#if defined(PCIT_CONFIG_DEBUG)
					for(const GlobalVar::Value& value : values){
						value.visit([&](const auto& element) -> void {
							using ValueT = std::decay_t<decltype(element)>;

							if constexpr(std::is_same<ValueT, Expr>()){
								evo::debugAssert(element.isConstant(), "Array element must be a constant");
								this->check_expr_type_match(element_type, element);

							}else if constexpr(std::is_same<ValueT, GlobalVar::Zeroinit>()){
								// Do nothing...

							}else if constexpr(std::is_same<ValueT, GlobalVar::Uninit>()){
								// Do nothing...

							}else if constexpr(std::is_same<ValueT, GlobalVar::String::ID>()){
								evo::debugAssert(
									element_type == this->getGlobalString(element).type,
									"Array element must match type"
								);

							}else if constexpr(std::is_same<ValueT, GlobalVar::Array::ID>()){
								evo::debugAssert(
									element_type == this->getGlobalArray(element).type,
									"Array element must match type"
								);

							}else if constexpr(std::is_same<ValueT, GlobalVar::Struct::ID>()){
								evo::debugAssert(
									element_type == this->getGlobalStruct(element).type,
									"Array element must match type"
								);

							}else{
								static_assert(false, "Unknown Global value kind");
							}
						});
					}
				#endif

				const Type array_type = this->createArrayType(element_type, values.size());
				return this->global_arrays.emplace_back(array_type, std::move(values));
			}


			EVO_NODISCARD auto getGlobalArray(GlobalVar::Array::ID id) const -> const GlobalVar::Array& {
				return this->global_arrays[id];
			}




			EVO_NODISCARD auto createGlobalStruct(
				Type type, std::vector<GlobalVar::Value> values
			) -> GlobalVar::Struct::ID {
				#if defined(PCIT_CONFIG_DEBUG)
					const StructType& struct_type = this->getStructType(type);

					for(size_t i = 0; const GlobalVar::Value& value : values){
						EVO_DEFER([&](){ i += 1; });

						const Type& member_type = struct_type.members[i];

			 			value.visit([&](const auto& member_value) -> void {
			 				using MemberValueT = std::decay_t<decltype(member_value)>;

			 				if constexpr(std::is_same<MemberValueT, Expr>()){
			 					this->check_expr_type_match(member_type, member_value);

			 				}else if constexpr(std::is_same<MemberValueT, GlobalVar::Zeroinit>()){
			 					// Do nothing...

			 				}else if constexpr(std::is_same<MemberValueT, GlobalVar::Uninit>()){
			 					// Do nothing...

			 				}else if constexpr(std::is_same<MemberValueT, GlobalVar::String::ID>()){
			 					evo::debugAssert(
			 						member_type == this->getGlobalString(member_value).type,
			 						"Struct member value must match type"
			 					);
			 					
			 				}else if constexpr(std::is_same<MemberValueT, GlobalVar::Array::ID>()){
			 					evo::debugAssert(
			 						member_type == this->getGlobalArray(member_value).type,
			 						"Struct member value must match type"
			 					);
			 					

			 				}else if constexpr(std::is_same<MemberValueT, GlobalVar::Struct::ID>()){
			 					evo::debugAssert(
			 						member_type == this->getGlobalStruct(member_value).type,
			 						"Struct member value must match type"
			 					);

			 				}else{
			 					static_assert(false, "Unknown Global value kind");
			 				}
			 			});
					}
				#endif

				return this->global_structs.emplace_back(type, std::move(values));
			}


			EVO_NODISCARD auto getGlobalStruct(GlobalVar::Struct::ID id) const -> const GlobalVar::Struct& {
				return this->global_structs[id];
			}


			///////////////////////////////////
			// global

			EVO_NODISCARD auto createGlobalVar(
				std::string&& global_name,
				Type type,
				Linkage linkage,
				GlobalVar::Value value,
				bool isConstant
			) -> GlobalVar::ID {
				#if defined(EVO_CONFIG_DEBUG)
					value.visit([&](const auto& member_value) -> void {
						using MemberValueT = std::decay_t<decltype(member_value)>;

						if constexpr(std::is_same<MemberValueT, Expr>()){
							this->check_expr_type_match(type, member_value);

						}else if constexpr(std::is_same<MemberValueT, GlobalVar::Zeroinit>()){
							// Do nothing...

						}else if constexpr(std::is_same<MemberValueT, GlobalVar::Uninit>()){
							// Do nothing...

						}else if constexpr(std::is_same<MemberValueT, GlobalVar::String::ID>()){
							evo::debugAssert(
								type == this->getGlobalString(member_value).type,
								"Global variable value must match type"
							);
							
						}else if constexpr(std::is_same<MemberValueT, GlobalVar::Array::ID>()){
							evo::debugAssert(
								type == this->getGlobalArray(member_value).type,
								"Global variable value must match type"
							);
							

						}else if constexpr(std::is_same<MemberValueT, GlobalVar::Struct::ID>()){
							evo::debugAssert(
								type == this->getGlobalStruct(member_value).type,
								"Global variable value must match type"
							);

						}else{
							static_assert(false, "Unknown Global value kind");
						}
					});
						
					this->check_global_name_reusue(global_name);
				#endif

				return this->global_vars.emplace_back(std::move(global_name), type, linkage, value, isConstant);
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

			EVO_NODISCARD static auto createVoidType() -> Type { return Type(Type::Kind::Void); }

			EVO_NODISCARD static auto createPtrType() -> Type { return Type(Type::Kind::Ptr); }

			EVO_NODISCARD static auto createSignedType(uint32_t width) -> Type {
				evo::debugAssert(
					width > 1 && width < 1 << 23,
					"Invalid width for a signed ({})", width
				);
				return Type(Type::Kind::Signed, width);
			}

			EVO_NODISCARD static auto createUnsignedType(uint32_t width) -> Type {
				evo::debugAssert(
					width != 0 && width < 1 << 23,
					"Invalid width for an unsigned ({})", width
				);
				return Type(Type::Kind::Unsigned, width);
			}

			EVO_NODISCARD static auto createBoolType() -> Type { return Type(Type::Kind::Bool); }


			EVO_NODISCARD static auto createFloatType(uint32_t width) -> Type {
				evo::debugAssert(
					width == 16 || width == 32 || width == 64 || width == 80 || width == 128,
					"Invalid width for a float ({})", width
				);
				return Type(Type::Kind::Float, width);
			}

			EVO_NODISCARD static auto createBFloatType() -> Type { return Type(Type::Kind::BFloat); }



			EVO_NODISCARD auto createArrayType(auto&&... args) -> Type {
				const uint32_t array_type_index = this->array_types.emplace_back(std::forward<decltype(args)>(args)...);
				return Type(Type::Kind::Array, array_type_index);
			}

			EVO_NODISCARD auto getArrayType(const Type& arr_type) const -> const ArrayType& {
				evo::debugAssert(arr_type.getKind() == Type::Kind::Array, "Not an array");
				return this->array_types[arr_type.number];
			}



			EVO_NODISCARD auto createStructType(
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

			EVO_NODISCARD auto getStructType(const Type& struct_type) const -> const StructType& {
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


			///////////////////////////////////
			// type traits

			EVO_NODISCARD auto sizeOfPtr() const -> size_t;
			EVO_NODISCARD auto alignmentOfPtr() const -> size_t;
			EVO_NODISCARD auto sizeOfGeneralRegister() const -> size_t;

			EVO_NODISCARD auto getSize(const Type& type, bool packed = false) const -> size_t;
			EVO_NODISCARD auto getAlignment(const Type& type) const -> size_t;



		private:
			#if defined(PCIT_CONFIG_DEBUG)
				auto check_param_names(evo::ArrayProxy<Parameter> params) const -> void;

				auto check_global_name_reusue(std::string_view global_name) const -> void;

				auto check_expr_type_match(Type type, const Expr& expr) const -> void;
			#endif
	
		private:
			std::string name;
			core::OS os;
			core::Architecture arch;

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
			core::StepAlloc<Load, uint32_t> loads{};
			core::StepAlloc<Store, uint32_t> stores{};
			core::StepAlloc<Add, uint32_t> adds{};
			core::StepAlloc<AddWrap, uint32_t> add_wraps{};

			// global values
			core::StepAlloc<GlobalVar::String, GlobalVar::String::ID> global_strings{};
			core::StepAlloc<GlobalVar::Array, GlobalVar::Array::ID> global_arrays{};
			core::StepAlloc<GlobalVar::Struct, GlobalVar::Struct::ID> global_structs{};


			friend class ReaderAgent;
			friend class Agent;
	};


}


