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
			Module(
				std::string&& _name, core::Platform _platform
			) : name(std::move(_name)), platform(_platform) {}
			~Module() = default;

			Module(const Module&) = delete;
			Module(Module&&) noexcept = default;


			EVO_NODISCARD auto getName() const -> std::string_view { return this->name; }
			EVO_NODISCARD auto getPlatform() const -> const core::Platform& { return this->platform; }


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
					ExternalFunction(
						std::move(func_name), std::move(parameters), callingConvention, linkage, returnType
					)
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

			EVO_NODISCARD auto createExternalFunction(
				std::string&& func_name,
				evo::SmallVector<Parameter>&& parameters,
				CallingConvention callingConvention,
				Linkage linkage,
				Type returnType
			) -> ExternalFunction::ID {
				#if defined(PCIT_CONFIG_DEBUG)
					this->check_param_names(parameters);
					this->check_global_name_reusue(func_name);
				#endif

				return this->external_funcs.emplace_back(
					std::move(func_name), std::move(parameters), callingConvention, linkage, returnType
				);
			}

			EVO_NODISCARD auto getExternalFunction(ExternalFunction::ID id) const -> const ExternalFunction& {
				return this->external_funcs[id];
			}

			EVO_NODISCARD auto getExternalFunction(ExternalFunction::ID id) -> ExternalFunction& {
				return this->external_funcs[id];
			}


			using ExternalFunctionIter = core::StepAlloc<ExternalFunction, ExternalFunction::ID>::Iter;
			using ExternalFunctionsConstIter = core::StepAlloc<ExternalFunction, ExternalFunction::ID>::ConstIter;

			EVO_NODISCARD auto getExternalFunctionIter() -> core::IterRange<ExternalFunctionIter> {
				return core::IterRange<ExternalFunctionIter>(
					this->external_funcs.begin(), this->external_funcs.end()
				);
			}

			EVO_NODISCARD auto getExternalFunctionIter() const -> core::IterRange<ExternalFunctionsConstIter> {
				return core::IterRange<ExternalFunctionsConstIter>(
					this->external_funcs.cbegin(), this->external_funcs.cend()
				);
			}

			EVO_NODISCARD auto getExternalFunctionsConstIter() const -> core::IterRange<ExternalFunctionsConstIter> {
				return core::IterRange<ExternalFunctionsConstIter>(
					this->external_funcs.cbegin(), this->external_funcs.cend()
				);
			}


			///////////////////////////////////
			// global values

			EVO_NODISCARD auto createGlobalString(std::string&& string) -> GlobalVar::String::ID {
				const Type str_type = this->createArrayType(this->createIntegerType(8), string.size() + 1);
				return this->global_strings.emplace_back(std::move(string), str_type);
			}


			EVO_NODISCARD auto getGlobalString(GlobalVar::String::ID id) const -> const GlobalVar::String& {
				return this->global_strings[id];
			}



			EVO_NODISCARD auto createGlobalArray(
				Type element_type, evo::SmallVector<GlobalVar::Value> values
			) -> GlobalVar::Array::ID {
				#if defined(PCIT_CONFIG_DEBUG)
					for(const GlobalVar::Value& value : values){
						value.visit([&](const auto& element) -> void {
							using ValueT = std::decay_t<decltype(element)>;

							if constexpr(std::is_same<ValueT, GlobalVar::NoValue>()){
								evo::debugAssert("Cannot have array element with no value");

							}else if constexpr(std::is_same<ValueT, Expr>()){
								evo::debugAssert(element.isConstant(), "Array element must be a constant");
								this->check_expr_type_match(element_type, element);

							}else if constexpr(std::is_same<ValueT, GlobalVar::Zeroinit>()){
								// Do nothing...

							}else if constexpr(std::is_same<ValueT, GlobalVar::Uninit>()){
								// Do nothing...

							}else if constexpr(std::is_same<ValueT, GlobalVar::String::ID>()){
								evo::debugAssert(
									this->typesEquivalent(element_type, this->getGlobalString(element).type),
									"Array element must match type"
								);

							}else if constexpr(std::is_same<ValueT, GlobalVar::Array::ID>()){
								evo::debugAssert(
									this->typesEquivalent(element_type, this->getGlobalArray(element).type),
									"Array element must match type"
								);

							}else if constexpr(std::is_same<ValueT, GlobalVar::Struct::ID>()){
								evo::debugAssert(
									this->typesEquivalent(element_type, this->getGlobalStruct(element).type),
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
				Type type, evo::SmallVector<GlobalVar::Value> values
			) -> GlobalVar::Struct::ID {
				#if defined(PCIT_CONFIG_DEBUG)
					const StructType& struct_type = this->getStructType(type);

					for(size_t i = 0; const GlobalVar::Value& value : values){
						EVO_DEFER([&](){ i += 1; });

						const Type& member_type = struct_type.members[i];

			 			value.visit([&](const auto& member_value) -> void {
			 				using MemberValueT = std::decay_t<decltype(member_value)>;

			 				if constexpr(std::is_same<MemberValueT, GlobalVar::NoValue>()){
			 					evo::debugAssert("Cannot have struct element with no value");

			 				}else if constexpr(std::is_same<MemberValueT, Expr>()){
			 					this->check_expr_type_match(member_type, member_value);

			 				}else if constexpr(std::is_same<MemberValueT, GlobalVar::Zeroinit>()){
			 					// Do nothing...

			 				}else if constexpr(std::is_same<MemberValueT, GlobalVar::Uninit>()){
			 					// Do nothing...

			 				}else if constexpr(std::is_same<MemberValueT, GlobalVar::String::ID>()){
			 					evo::debugAssert(
			 						this->typesEquivalent(member_type, this->getGlobalString(member_value).type),
			 						"Struct member value must match type"
			 					);
			 					
			 				}else if constexpr(std::is_same<MemberValueT, GlobalVar::Array::ID>()){
			 					evo::debugAssert(
			 						this->typesEquivalent(member_type, this->getGlobalArray(member_value).type),
			 						"Struct member value must match type"
			 					);
			 					

			 				}else if constexpr(std::is_same<MemberValueT, GlobalVar::Struct::ID>()){
			 					evo::debugAssert(
			 						this->typesEquivalent(member_type, this->getGlobalStruct(member_value).type),
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

						if constexpr(std::is_same<MemberValueT, GlobalVar::NoValue>()){
							// Do nothing...

						}else if constexpr(std::is_same<MemberValueT, Expr>()){
							this->check_expr_type_match(type, member_value);

						}else if constexpr(std::is_same<MemberValueT, GlobalVar::Zeroinit>()){
							// Do nothing...

						}else if constexpr(std::is_same<MemberValueT, GlobalVar::Uninit>()){
							// Do nothing...

						}else if constexpr(std::is_same<MemberValueT, GlobalVar::String::ID>()){
							evo::debugAssert(
								this->typesEquivalent(type, this->getGlobalString(member_value).type),
								"Global variable value must match type"
							);
							
						}else if constexpr(std::is_same<MemberValueT, GlobalVar::Array::ID>()){
							evo::debugAssert(
								this->typesEquivalent(type, this->getGlobalArray(member_value).type),
								"Global variable value must match type"
							);
							

						}else if constexpr(std::is_same<MemberValueT, GlobalVar::Struct::ID>()){
							evo::debugAssert(
								this->typesEquivalent(type, this->getGlobalStruct(member_value).type),
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

			EVO_NODISCARD static auto createVoidType() -> Type { return Type(Type::Kind::VOID); }

			EVO_NODISCARD static auto createPtrType() -> Type { return Type(Type::Kind::PTR); }

			EVO_NODISCARD static auto createIntegerType(uint32_t width) -> Type {
				evo::debugAssert(width != 0 && width < 1 << 23, "Invalid width for an integer ({})", width);
				return Type(Type::Kind::INTEGER, width);
			}

			EVO_NODISCARD static auto createBoolType() -> Type { return Type(Type::Kind::BOOL); }


			EVO_NODISCARD static auto createFloatType(uint32_t width) -> Type {
				evo::debugAssert(
					width == 16 || width == 32 || width == 64 || width == 80 || width == 128,
					"Invalid width for a float ({})", width
				);
				return Type(Type::Kind::FLOAT, width);
			}

			EVO_NODISCARD static auto createBFloatType() -> Type { return Type(Type::Kind::BFLOAT); }



			EVO_NODISCARD auto createArrayType(Type elem_type, uint64_t length) -> Type {
				const uint32_t array_type_index = this->array_types.emplace_back(elem_type, length);
				return Type(Type::Kind::ARRAY, array_type_index);
			}

			EVO_NODISCARD auto getArrayType(const Type& arr_type) const -> const ArrayType& {
				evo::debugAssert(arr_type.kind() == Type::Kind::ARRAY, "Not an array");
				return this->array_types[arr_type.number];
			}



			EVO_NODISCARD auto createStructType(
				std::string&& struct_name, evo::SmallVector<Type>&& members, bool is_packed
			) -> Type {
				#if defined(PCIT_CONFIG_DEBUG)
					this->check_global_name_reusue(struct_name);
				#endif
				evo::debugAssert(members.empty() == false, "Cannot create a struct type with no members");

				const uint32_t struct_type_index = this->struct_types.emplace_back(
					std::move(struct_name), std::move(members), is_packed
				);
				return Type(Type::Kind::STRUCT, struct_type_index);
			}

			EVO_NODISCARD auto getStructType(const Type& struct_type) const -> const StructType& {
				evo::debugAssert(struct_type.kind() == Type::Kind::STRUCT, "Not a struct");
				return this->struct_types[struct_type.number];
			}


			using StructTypeIter = core::SyncLinearStepAlloc<StructType, uint32_t>::Iter;
			using StructTypeConstIter = core::SyncLinearStepAlloc<StructType, uint32_t>::ConstIter;

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
				return Type(Type::Kind::FUNCTION, array_type_index);
			}

			EVO_NODISCARD auto getFunctionType(const Type& func_type) const -> const FunctionType& {
				evo::debugAssert(func_type.kind() == Type::Kind::FUNCTION, "Not an function");
				return this->func_types[func_type.number];
			}



			EVO_NODISCARD auto typesEquivalent(const Type& lhs, const Type& rhs) const -> bool {
				if(lhs.kind() != rhs.kind()){ return false; }

				switch(lhs.kind()){
					case Type::Kind::VOID:    return true;
					case Type::Kind::INTEGER: return lhs.getWidth() == rhs.getWidth();
					case Type::Kind::BOOL:    return true;
					case Type::Kind::FLOAT:   return lhs.getWidth() == rhs.getWidth();
					case Type::Kind::BFLOAT:  return true;
					case Type::Kind::PTR:     return true;

					case Type::Kind::ARRAY: {
						const ArrayType& lhs_array = this->getArrayType(lhs);
						const ArrayType& rhs_array = this->getArrayType(rhs);

						if(lhs_array.length != rhs_array.length){ return false; }
						return this->typesEquivalent(lhs_array.elemType, rhs_array.elemType);
					} break;

					case Type::Kind::STRUCT: {
						const StructType& lhs_struct = this->getStructType(lhs);
						const StructType& rhs_struct = this->getStructType(rhs);

						if(lhs_struct.isPacked != rhs_struct.isPacked){ return false; }
						if(lhs_struct.members.size() != rhs_struct.members.size()){ return false; }
						for(size_t i = 0; i < lhs_struct.members.size(); i+=1){
							if(this->typesEquivalent(lhs_struct.members[i], rhs_struct.members[i]) == false){
								return false;
							}
						}
						return true;
					} break;

					case Type::Kind::FUNCTION: {
						const FunctionType& lhs_func = this->getFunctionType(lhs);
						const FunctionType& rhs_func = this->getFunctionType(rhs);

						if(lhs_func.callingConvention != rhs_func.callingConvention){ return false; }
						if(lhs_func.parameters.size() != rhs_func.parameters.size()){ return false; }
						for(size_t i = 0; i < lhs_func.parameters.size(); i+=1){
							if(this->typesEquivalent(lhs_func.parameters[i], rhs_func.parameters[i]) == false){
								return false;
							}
						}
						return this->typesEquivalent(lhs_func.returnType, rhs_func.returnType);
					} break;
				}

				evo::unreachable();
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
			core::Platform platform;

			core::StepAlloc<Function, Function::ID> functions{};
			core::StepAlloc<ExternalFunction, ExternalFunction::ID> external_funcs{};
			core::StepAlloc<GlobalVar, GlobalVar::ID> global_vars{};

			core::StepAlloc<BasicBlock, BasicBlock::ID> basic_blocks{};
			core::StepAlloc<Number, uint32_t> numbers{};

			core::SyncLinearStepAlloc<ArrayType, uint32_t> array_types{};
			core::SyncLinearStepAlloc<StructType, uint32_t> struct_types{};
			core::SyncLinearStepAlloc<FunctionType, uint32_t> func_types{};

			// exprs
			core::StepAlloc<Call, uint32_t> calls{};
			core::StepAlloc<CallVoid, uint32_t> call_voids{};
			core::StepAlloc<Ret, uint32_t> rets{};
			core::StepAlloc<Branch, uint32_t> branches{};
			core::StepAlloc<Load, uint32_t> loads{};
			core::StepAlloc<Store, uint32_t> stores{};
			core::StepAlloc<CalcPtr, uint32_t> calc_ptrs{};
			core::StepAlloc<BitCast, uint32_t> bitcasts{};
			core::StepAlloc<Memcpy, uint32_t> memcpys{};
			core::StepAlloc<Memset, uint32_t> memsets{};
			core::StepAlloc<Trunc, uint32_t> truncs{};
			core::StepAlloc<FTrunc, uint32_t> ftruncs{};
			core::StepAlloc<SExt, uint32_t> sexts{};
			core::StepAlloc<ZExt, uint32_t> zexts{};
			core::StepAlloc<FExt, uint32_t> fexts{};
			core::StepAlloc<IToF, uint32_t> itofs{};
			core::StepAlloc<UIToF, uint32_t> uitofs{};
			core::StepAlloc<FToI, uint32_t> ftois{};
			core::StepAlloc<FToUI, uint32_t> ftouis{};

			core::StepAlloc<Add, uint32_t> adds{};
			core::StepAlloc<SAddWrap, uint32_t> sadd_wraps{};
			core::StepAlloc<UAddWrap, uint32_t> uadd_wraps{};
			core::StepAlloc<SAddSat, uint32_t> sadd_sats{};
			core::StepAlloc<UAddSat, uint32_t> uadd_sats{};
			core::StepAlloc<FAdd, uint32_t> fadds{};
			core::StepAlloc<Sub, uint32_t> subs{};
			core::StepAlloc<SSubWrap, uint32_t> ssub_wraps{};
			core::StepAlloc<USubWrap, uint32_t> usub_wraps{};
			core::StepAlloc<SSubSat, uint32_t> ssub_sats{};
			core::StepAlloc<USubSat, uint32_t> usub_sats{};
			core::StepAlloc<FSub, uint32_t> fsubs{};
			core::StepAlloc<Mul, uint32_t> muls{};
			core::StepAlloc<SMulWrap, uint32_t> smul_wraps{};
			core::StepAlloc<UMulWrap, uint32_t> umul_wraps{};
			core::StepAlloc<SMulSat, uint32_t> smul_sats{};
			core::StepAlloc<UMulSat, uint32_t> umul_sats{};
			core::StepAlloc<FMul, uint32_t> fmuls{};
			core::StepAlloc<SDiv, uint32_t> sdivs{};
			core::StepAlloc<UDiv, uint32_t> udivs{};
			core::StepAlloc<FDiv, uint32_t> fdivs{};
			core::StepAlloc<SRem, uint32_t> srems{};
			core::StepAlloc<URem, uint32_t> urems{};
			core::StepAlloc<FRem, uint32_t> frems{};
			core::StepAlloc<FNeg, uint32_t> fnegs{};

			core::StepAlloc<IEq, uint32_t> ieqs{};
			core::StepAlloc<FEq, uint32_t> feqs{};
			core::StepAlloc<INeq, uint32_t> ineqs{};
			core::StepAlloc<FNeq, uint32_t> fneqs{};
			core::StepAlloc<SLT, uint32_t> slts{};
			core::StepAlloc<ULT, uint32_t> ults{};
			core::StepAlloc<FLT, uint32_t> flts{};
			core::StepAlloc<SLTE, uint32_t> sltes{};
			core::StepAlloc<ULTE, uint32_t> ultes{};
			core::StepAlloc<FLTE, uint32_t> fltes{};
			core::StepAlloc<SGT, uint32_t> sgts{};
			core::StepAlloc<UGT, uint32_t> ugts{};
			core::StepAlloc<FGT, uint32_t> fgts{};
			core::StepAlloc<SGTE, uint32_t> sgtes{};
			core::StepAlloc<UGTE, uint32_t> ugtes{};
			core::StepAlloc<FGTE, uint32_t> fgtes{};

			core::StepAlloc<And, uint32_t> ands{};
			core::StepAlloc<Or, uint32_t> ors{};
			core::StepAlloc<Xor, uint32_t> xors{};
			core::StepAlloc<SHL, uint32_t> shls{};
			core::StepAlloc<SSHLSat, uint32_t> sshlsats{};
			core::StepAlloc<USHLSat, uint32_t> ushlsats{};
			core::StepAlloc<SSHR, uint32_t> sshrs{};
			core::StepAlloc<USHR, uint32_t> ushrs{};

			core::StepAlloc<BitReverse, uint32_t> bit_reverses{};
			core::StepAlloc<BSwap, uint32_t> bswaps{};
			core::StepAlloc<CtPop, uint32_t> ctpops{};
			core::StepAlloc<CTLZ, uint32_t> ctlzs{};
			core::StepAlloc<CTTZ, uint32_t> cttzs{};


			// global values
			core::StepAlloc<GlobalVar::String, GlobalVar::String::ID> global_strings{};
			core::StepAlloc<GlobalVar::Array, GlobalVar::Array::ID> global_arrays{};
			core::StepAlloc<GlobalVar::Struct, GlobalVar::Struct::ID> global_structs{};


			friend class ReaderAgent;
			friend class Agent;
	};


}


