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

#include "./Function.hpp"
#include "./GlobalVar.hpp"
#include "./meta.hpp"



namespace pcit::pir{


	class Module{
		public:
			Module(
				std::string&& _name, core::Target _target
			) : name(std::move(_name)), target(_target) {}
			~Module() = default;

			Module(const Module&) = delete;
			Module(Module&&) noexcept = default;


			[[nodiscard]] auto getName() const -> std::string_view { return this->name; }
			[[nodiscard]] auto getTarget() const -> const core::Target& { return this->target; }


			///////////////////////////////////
			// function

			[[nodiscard]] auto createFunction(
				std::string&& func_name,
				evo::SmallVector<Parameter>&& parameters,
				CallingConvention calling_convension,
				Linkage linkage,
				Type return_type,
				bool is_no_return = false,
				std::optional<meta::Function::ID> meta_id = std::nullopt
			) -> Function::ID {
				#if defined(PCIT_CONFIG_DEBUG)
					this->check_param_names(parameters);
					this->check_global_name_reuse(func_name);
				#endif

				evo::debugAssert(
					is_no_return == false || return_type.kind() == Type::Kind::VOID,
					"`#noReturn` can only be on a function that returns `Void`"
				);

				return this->functions.emplace_back(
					*this,
					ExternalFunction(
						std::move(func_name),
						std::move(parameters),
						calling_convension,
						linkage,
						return_type,
						is_no_return,
						meta_id
					)
				);
			}

			[[nodiscard]] auto getFunction(Function::ID id) const -> const Function& {
				return this->functions[id];
			}

			[[nodiscard]] auto getFunction(Function::ID id) -> Function& {
				return this->functions[id];
			}

			
			auto deleteBodyOfFunction(Function::ID id) -> void;


			using FunctionIter = core::StepAlloc<Function, Function::ID>::Iter;
			using FunctionConstIter = core::StepAlloc<Function, Function::ID>::ConstIter;

			[[nodiscard]] auto getFunctionIter() -> evo::IterRange<FunctionIter> {
				return evo::IterRange<FunctionIter>(this->functions.begin(), this->functions.end());
			}

			[[nodiscard]] auto getFunctionIter() const -> evo::IterRange<FunctionConstIter> {
				return evo::IterRange<FunctionConstIter>(this->functions.cbegin(), this->functions.cend());
			}

			[[nodiscard]] auto getFunctionConstIter() const -> evo::IterRange<FunctionConstIter> {
				return evo::IterRange<FunctionConstIter>(this->functions.cbegin(), this->functions.cend());
			}


			///////////////////////////////////
			// function declaration

			[[nodiscard]] auto createExternalFunction(
				std::string&& func_name,
				evo::SmallVector<Parameter>&& parameters,
				CallingConvention calling_convension,
				Linkage linkage,
				Type return_type,
				bool is_no_return = false,
				std::optional<meta::Function::ID> meta_id = std::nullopt
			) -> ExternalFunction::ID {
				#if defined(PCIT_CONFIG_DEBUG)
					this->check_param_names(parameters);
					this->check_global_name_reuse(func_name);
				#endif

				evo::debugAssert(
					is_no_return == false || return_type.kind() == Type::Kind::VOID,
					"`#noReturn` can only be on a function that returns `Void`"
				);

				return this->external_funcs.emplace_back(
					std::move(func_name),
					std::move(parameters),
					calling_convension,
					linkage,
					return_type,
					is_no_return,
					meta_id
				);
			}

			[[nodiscard]] auto getExternalFunction(ExternalFunction::ID id) const -> const ExternalFunction& {
				return this->external_funcs[id];
			}

			[[nodiscard]] auto getExternalFunction(ExternalFunction::ID id) -> ExternalFunction& {
				return this->external_funcs[id];
			}


			using ExternalFunctionIter = core::StepAlloc<ExternalFunction, ExternalFunction::ID>::Iter;
			using ExternalFunctionConstIter = core::StepAlloc<ExternalFunction, ExternalFunction::ID>::ConstIter;

			[[nodiscard]] auto getExternalFunctionIter() -> evo::IterRange<ExternalFunctionIter> {
				return evo::IterRange<ExternalFunctionIter>(
					this->external_funcs.begin(), this->external_funcs.end()
				);
			}

			[[nodiscard]] auto getExternalFunctionIter() const -> evo::IterRange<ExternalFunctionConstIter> {
				return evo::IterRange<ExternalFunctionConstIter>(
					this->external_funcs.cbegin(), this->external_funcs.cend()
				);
			}

			[[nodiscard]] auto getExternalFunctionConstIter() const -> evo::IterRange<ExternalFunctionConstIter> {
				return evo::IterRange<ExternalFunctionConstIter>(
					this->external_funcs.cbegin(), this->external_funcs.cend()
				);
			}


			///////////////////////////////////
			// global values

			[[nodiscard]] auto createGlobalString(std::string&& string) -> GlobalVar::String::ID {
				const Type str_type = this->getOrCreateArrayType(this->createSignedType(8), string.size());
				return this->global_strings.emplace_back(std::move(string), str_type);
			}


			[[nodiscard]] auto getGlobalString(GlobalVar::String::ID id) const -> const GlobalVar::String& {
				return this->global_strings[id];
			}


			//////////////////
			// global array

			[[nodiscard]] auto createGlobalArray(
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
									element_type == this->getGlobalString(element).type, "Array element must match type"
								);

							}else if constexpr(std::is_same<ValueT, GlobalVar::Array::ID>()){
								evo::debugAssert(
									element_type == this->getGlobalArray(element).type, "Array element must match type"
								);

							}else if constexpr(std::is_same<ValueT, GlobalVar::ByteArray::ID>()){
								evo::debugAssert(
									element_type == this->getGlobalByteArray(element).type,
									"Array element must match type"
								);

							}else if constexpr(std::is_same<ValueT, GlobalVar::Struct::ID>()){
								evo::debugAssert(
									element_type == this->getGlobalStruct(element).type, "Array element must match type"
								);

							}else{
								static_assert(false, "Unknown Global value kind");
							}
						});
					}
				#endif

				const Type array_type = this->getOrCreateArrayType(element_type, values.size());
				return this->global_arrays.emplace_back(array_type, std::move(values));
			}


			[[nodiscard]] auto getGlobalArray(GlobalVar::Array::ID id) const -> const GlobalVar::Array& {
				return this->global_arrays[id];
			}


			//////////////////
			// global byte array

			[[nodiscard]] auto createGlobalByteArray(evo::SmallVector<std::byte>&& bytes) -> GlobalVar::ByteArray::ID {
				const Type array_type = this->getOrCreateArrayType(this->createUnsignedType(8), bytes.size());
				return this->global_byte_arrays.emplace_back(array_type, std::move(bytes));
			}

			[[nodiscard]] auto createGlobalByteArray(evo::ArrayProxy<std::byte> bytes) -> GlobalVar::ByteArray::ID {
				return this->createGlobalByteArray(evo::SmallVector<std::byte>(bytes.begin(), bytes.end()));
			}

			[[nodiscard]] auto getGlobalByteArray(GlobalVar::ByteArray::ID id) const -> const GlobalVar::ByteArray& {
				return this->global_byte_arrays[id];
			}


			//////////////////
			// global struct

			[[nodiscard]] auto createGlobalStruct(
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
			 						member_type == this->getGlobalString(member_value).type,
			 						"Struct member value must match type"
			 					);
			 					
			 				}else if constexpr(std::is_same<MemberValueT, GlobalVar::Array::ID>()){
			 					evo::debugAssert(
			 						member_type == this->getGlobalArray(member_value).type,
			 						"Struct member value must match type"
			 					);

			 				}else if constexpr(std::is_same<MemberValueT, GlobalVar::ByteArray::ID>()){
			 					evo::debugAssert(
			 						member_type == this->getGlobalByteArray(member_value).type,
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


			[[nodiscard]] auto getGlobalStruct(GlobalVar::Struct::ID id) const -> const GlobalVar::Struct& {
				return this->global_structs[id];
			}


			///////////////////////////////////
			// global

			[[nodiscard]] auto createGlobalVar(
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
							evo::debugAssert(member_value.isConstant(), "Global variable value must be constant");

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

						}else if constexpr(std::is_same<MemberValueT, GlobalVar::ByteArray::ID>()){
							evo::debugAssert(
								type == this->getGlobalByteArray(member_value).type,
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
						
					this->check_global_name_reuse(global_name);
				#endif

				return this->global_vars.emplace_back(std::move(global_name), type, linkage, value, isConstant);
			}


			[[nodiscard]] auto getGlobalVar(GlobalVar::ID id) const -> const GlobalVar& {
				return this->global_vars[id];
			}

			[[nodiscard]] auto getGlobalVar(GlobalVar::ID id) -> GlobalVar& {
				return this->global_vars[id];
			}


			using GlobalVarIter = core::StepAlloc<GlobalVar, GlobalVar::ID>::Iter;
			using GlobalVarConstIter = core::StepAlloc<GlobalVar, GlobalVar::ID>::ConstIter;

			[[nodiscard]] auto getGlobalVarIter() -> evo::IterRange<GlobalVarIter> {
				return evo::IterRange<GlobalVarIter>(this->global_vars.begin(), this->global_vars.end());
			}

			[[nodiscard]] auto getGlobalVarIter() const -> evo::IterRange<GlobalVarConstIter> {
				return evo::IterRange<GlobalVarConstIter>(this->global_vars.cbegin(), this->global_vars.cend());
			}

			[[nodiscard]] auto getGlobalVarConstIter() const -> evo::IterRange<GlobalVarConstIter> {
				return evo::IterRange<GlobalVarConstIter>(this->global_vars.cbegin(), this->global_vars.cend());
			}



			///////////////////////////////////
			// types

			[[nodiscard]] static auto createVoidType() -> Type { return Type(Type::Kind::VOID); }

			[[nodiscard]] static auto createPtrType() -> Type { return Type(Type::Kind::PTR); }

			[[nodiscard]] static auto createUnsignedType(uint32_t width) -> Type {
				evo::debugAssert(width != 0 && width < 1 << 23, "Invalid width for an integer ({})", width);
				return Type(Type::Kind::UNSIGNED, width);
			}

			[[nodiscard]] static auto createSignedType(uint32_t width) -> Type {
				evo::debugAssert(width != 0 && width < 1 << 23, "Invalid width for an integer ({})", width);
				return Type(Type::Kind::SIGNED, width);
			}

			[[nodiscard]] static auto createBoolType() -> Type { return Type(Type::Kind::BOOL); }


			[[nodiscard]] static auto createFloatType(uint32_t width) -> Type {
				evo::debugAssert(
					width == 16 || width == 32 || width == 64 || width == 80 || width == 128,
					"Invalid width for a float ({})", width
				);
				return Type(Type::Kind::FLOAT, width);
			}


			[[nodiscard]] auto getOrCreateArrayType(Type elem_type, uint64_t length) -> Type {
				const auto lock = std::scoped_lock(array_types_lock);

				// TODO(FUTURE): lookup with a hash map
				for(uint32_t i = 0; const ArrayType& array_type : this->array_types){
					EVO_DEFER([&](){ i += 1; });

					if(array_type.elemType == elem_type && array_type.length == length){
						return Type(Type::Kind::ARRAY, i);
					}
				}

				const uint32_t array_type_index = this->array_types.emplace_back(elem_type, length);
				return Type(Type::Kind::ARRAY, array_type_index);
			}

			[[nodiscard]] auto getArrayType(const Type& arr_type) const -> const ArrayType& {
				evo::debugAssert(arr_type.kind() == Type::Kind::ARRAY, "Not an array");

				const auto lock = std::scoped_lock(array_types_lock);
				return this->array_types[arr_type.number];
			}



			[[nodiscard]] auto createStructType(
				std::string&& struct_name, evo::SmallVector<Type>&& members, bool is_packed
			) -> Type {
				#if defined(PCIT_CONFIG_DEBUG)
					this->check_global_name_reuse(struct_name);
				#endif
				evo::debugAssert(members.empty() == false, "Cannot create a struct type with no members");

				const uint32_t struct_type_index = this->struct_types.emplace_back(
					std::move(struct_name), std::move(members), is_packed
				);
				return Type(Type::Kind::STRUCT, struct_type_index);
			}

			[[nodiscard]] auto getStructType(const Type& struct_type) const -> const StructType& {
				evo::debugAssert(struct_type.kind() == Type::Kind::STRUCT, "Not a struct");
				return this->struct_types[struct_type.number];
			}


			using StructTypeIter = core::SyncLinearStepAlloc<StructType, uint32_t>::Iter;
			using StructTypeConstIter = core::SyncLinearStepAlloc<StructType, uint32_t>::ConstIter;

			[[nodiscard]] auto getStructTypeIter() -> evo::IterRange<StructTypeIter> {
				return evo::IterRange<StructTypeIter>(
					this->struct_types.begin(), this->struct_types.end()
				);
			}

			[[nodiscard]] auto getStructTypeIter() const -> evo::IterRange<StructTypeConstIter> {
				return evo::IterRange<StructTypeConstIter>(
					this->struct_types.cbegin(), this->struct_types.cend()
				);
			}

			[[nodiscard]] auto getStructTypeConstIter() const -> evo::IterRange<StructTypeConstIter> {
				return evo::IterRange<StructTypeConstIter>(
					this->struct_types.cbegin(), this->struct_types.cend()
				);
			}



			[[nodiscard]] auto getOrCreateFunctionType(
				evo::SmallVector<Type>&& parameters, CallingConvention calling_convension, Type return_type
			) -> Type {
				const auto lock = std::scoped_lock(func_types_lock);

				// TODO(FUTURE): lookup with hash map
				for(uint32_t i = 0; const FunctionType& func_type : this->func_types){
					if(
						func_type.parameters == parameters
						&& func_type.callingConvention == calling_convension
						&& func_type.returnType == return_type
					){
						return Type(Type::Kind::FUNCTION, i);
					}
				
					i += 1;
				}

				const uint32_t func_type_index = this->func_types.emplace_back(
					std::move(parameters), calling_convension, return_type
				);
				return Type(Type::Kind::FUNCTION, func_type_index);
			}

			[[nodiscard]] auto getFunctionType(const Type& func_type) const -> const FunctionType& {
				evo::debugAssert(func_type.kind() == Type::Kind::FUNCTION, "Not an function");

				const auto lock = std::scoped_lock(func_types_lock);
				return this->func_types[func_type.number];
			}



			///////////////////////////////////
			// type traits

			[[nodiscard]] auto sizeOfPtr() const -> size_t;
			[[nodiscard]] auto alignmentOfPtr() const -> size_t;
			[[nodiscard]] auto sizeOfGeneralRegister() const -> size_t;
			[[nodiscard]] auto maxAlignmentOfPrimitive() const -> size_t;

			[[nodiscard]] auto numBytes(const Type& type, bool include_padding = true) const -> size_t;
			[[nodiscard]] auto getAlignment(const Type& type) const -> size_t;



			//////////////////////////////////////////////////////////////////////
			// meta

			///////////////////////////////////
			// meta files

			[[nodiscard]] auto createMetaFile(
				std::string&& meta_name,
				std::string&& file_path,
				meta::Language language,
				std::string&& producer_name
			) -> meta::File::ID {
				#if defined(PCIT_CONFIG_DEBUG)
					this->check_meta_name_reuse(meta_name);
				#endif

				return this->meta_files.emplace_back(
					std::move(meta_name), std::move(file_path), language, std::move(producer_name)
				);
			}

			[[nodiscard]] auto getMetaFile(meta::File::ID id) const -> const meta::File& {
				return this->meta_files[id];
			}


			using MetaFileIter = core::SyncLinearStepAlloc<meta::File, meta::FileID>::Iter;
			using MetaFileConstIter = core::SyncLinearStepAlloc<meta::File, meta::FileID>::ConstIter;

			[[nodiscard]] auto getMetaFileIter() -> evo::IterRange<MetaFileIter> {
				return evo::IterRange<MetaFileIter>(this->meta_files.begin(), this->meta_files.end());
			}

			[[nodiscard]] auto getMetaFileIter() const -> evo::IterRange<MetaFileConstIter> {
				return evo::IterRange<MetaFileConstIter>(this->meta_files.cbegin(), this->meta_files.cend());
			}

			[[nodiscard]] auto getMetaFileConstIter() const -> evo::IterRange<MetaFileConstIter> {
				return evo::IterRange<MetaFileConstIter>(this->meta_files.cbegin(), this->meta_files.cend());
			}


			///////////////////////////////////
			// meta types

			[[nodiscard]] auto createMetaBasicType(
				std::string&& meta_name, std::string&& type_name, Type underlying_type
			) -> meta::BasicType::ID {
				#if defined(PCIT_CONFIG_DEBUG)
					this->check_meta_name_reuse(meta_name);
				#endif

				evo::debugAssert(
					underlying_type.kind() == Type::Kind::UNSIGNED
						|| underlying_type.kind() == Type::Kind::SIGNED
						|| underlying_type.kind() == Type::Kind::BOOL
						|| underlying_type.kind() == Type::Kind::FLOAT,
					"Invalid underlying type"
				);

				return this->meta_basic_types.emplace_back(std::move(meta_name), std::move(type_name), underlying_type);
			}

			[[nodiscard]] auto getMetaBasicType(meta::BasicType::ID id) const -> const meta::BasicType& {
				return this->meta_basic_types[id];
			}


			using MetaBasicTypeIter = core::SyncLinearStepAlloc<meta::BasicType, meta::BasicTypeID>::Iter;
			using MetaBasicTypeConstIter = core::SyncLinearStepAlloc<meta::BasicType, meta::BasicTypeID>::ConstIter;

			[[nodiscard]] auto getMetaBasicTypeIter() -> evo::IterRange<MetaBasicTypeIter> {
				return evo::IterRange<MetaBasicTypeIter>(this->meta_basic_types.begin(), this->meta_basic_types.end());
			}

			[[nodiscard]] auto getMetaBasicTypeIter() const -> evo::IterRange<MetaBasicTypeConstIter> {
				return evo::IterRange<MetaBasicTypeConstIter>(
					this->meta_basic_types.cbegin(), this->meta_basic_types.cend()
				);
			}

			[[nodiscard]] auto getMetaBasicTypeConstIter() const -> evo::IterRange<MetaBasicTypeConstIter> {
				return evo::IterRange<MetaBasicTypeConstIter>(
					this->meta_basic_types.cbegin(), this->meta_basic_types.cend()
				);
			}


			///////////////////////////////////
			// meta qualified types

			[[nodiscard]] auto createMetaQualifiedType(
				std::string&& meta_name,
				std::string&& type_name,
				std::optional<meta::Type> qualee_type, // nullptr if should be RawPtr or void*
				meta::QualifiedType::Qualifier qualifier
			) -> meta::QualifiedType::ID {
				#if defined(PCIT_CONFIG_DEBUG)
					this->check_meta_name_reuse(meta_name);
				#endif

				return this->meta_qualified_types.emplace_back(
					std::move(meta_name), std::move(type_name), qualee_type, qualifier
				);
			}

			[[nodiscard]] auto getMetaQualifiedType(meta::QualifiedType::ID id) const -> const meta::QualifiedType& {
				return this->meta_qualified_types[id];
			}


			using MetaQualifiedTypeIter = core::SyncLinearStepAlloc<meta::QualifiedType, meta::QualifiedTypeID>::Iter;
			using MetaQualifiedTypeConstIter =
				core::SyncLinearStepAlloc<meta::QualifiedType, meta::QualifiedTypeID>::ConstIter;

			[[nodiscard]] auto getMetaQualifiedTypeIter() -> evo::IterRange<MetaQualifiedTypeIter> {
				return evo::IterRange<MetaQualifiedTypeIter>(
					this->meta_qualified_types.begin(), this->meta_qualified_types.end()
				);
			}

			[[nodiscard]] auto getMetaQualifiedTypeIter() const -> evo::IterRange<MetaQualifiedTypeConstIter> {
				return evo::IterRange<MetaQualifiedTypeConstIter>(
					this->meta_qualified_types.cbegin(), this->meta_qualified_types.cend()
				);
			}

			[[nodiscard]] auto getMetaQualifiedTypeConstIter() const -> evo::IterRange<MetaQualifiedTypeConstIter> {
				return evo::IterRange<MetaQualifiedTypeConstIter>(
					this->meta_qualified_types.cbegin(), this->meta_qualified_types.cend()
				);
			}


			///////////////////////////////////
			// meta struct types

			[[nodiscard]] auto createMetaStructType(
				Type struct_type,
				std::string&& meta_name,
				std::string&& struct_name,
				evo::SmallVector<meta::StructType::Member>&& members,
				meta::FileID file_id,
				meta::Scope scope_where_defined,
				uint32_t line_number
			) -> meta::StructType::ID {
				evo::debugAssert(struct_type.kind() == Type::Kind::STRUCT, "not struct type");
				#if defined(PCIT_CONFIG_DEBUG)
					this->check_meta_name_reuse(meta_name);
				#endif
				
				const meta::StructType::ID created_type_id = this->meta_struct_types.emplace_back(
					std::move(meta_name),
					struct_type,
					std::move(struct_name),
					std::move(members),
					file_id,
					scope_where_defined,
					line_number
				);


				{
					const auto lock = std::scoped_lock(this->meta_struct_type_lookup_lock);
					this->meta_struct_type_lookup.emplace(struct_type, created_type_id);
				}

				return created_type_id;
			}

			[[nodiscard]] auto getMetaStructType(meta::StructType::ID id) const -> const meta::StructType& {
				return this->meta_struct_types[id];
			}


			[[nodiscard]] auto lookupMetaStructType(Type struct_type) const -> std::optional<meta::StructType::ID> {
				evo::debugAssert(struct_type.kind() == Type::Kind::STRUCT, "not struct type");

				const auto lock = std::scoped_lock(this->meta_struct_type_lookup_lock);

				const auto find = this->meta_struct_type_lookup.find(struct_type);
				if(find != this->meta_struct_type_lookup.end()){ return find->second; }
				return std::nullopt;
			}



			using MetaStructTypeIter = core::SyncLinearStepAlloc<meta::StructType, meta::StructTypeID>::Iter;
			using MetaStructTypeConstIter = core::SyncLinearStepAlloc<meta::StructType, meta::StructTypeID>::ConstIter;

			[[nodiscard]] auto getMetaStructTypeIter() -> evo::IterRange<MetaStructTypeIter> {
				return evo::IterRange<MetaStructTypeIter>(
					this->meta_struct_types.begin(), this->meta_struct_types.end()
				);
			}

			[[nodiscard]] auto getMetaStructTypeIter() const -> evo::IterRange<MetaStructTypeConstIter> {
				return evo::IterRange<MetaStructTypeConstIter>(
					this->meta_struct_types.cbegin(), this->meta_struct_types.cend()
				);
			}

			[[nodiscard]] auto getMetaStructTypeConstIter() const -> evo::IterRange<MetaStructTypeConstIter> {
				return evo::IterRange<MetaStructTypeConstIter>(
					this->meta_struct_types.cbegin(), this->meta_struct_types.cend()
				);
			}



			///////////////////////////////////
			// meta array types

			[[nodiscard]] auto createMetaArrayType(
				std::string&& meta_name,
				pir::Type array_type,
				meta::Type element_type,
				evo::SmallVector<uint64_t>&& dimensions
			) -> meta::ArrayType::ID {
				#if defined(PCIT_CONFIG_DEBUG)
					this->check_meta_name_reuse(meta_name);
				#endif
				
				return this->meta_array_types.emplace_back(
					std::move(meta_name), array_type, element_type, std::move(dimensions)
				);
			}

			[[nodiscard]] auto getMetaArrayType(meta::ArrayType::ID id) const -> const meta::ArrayType& {
				return this->meta_array_types[id];
			}



			using MetaArrayTypeIter = core::SyncLinearStepAlloc<meta::ArrayType, meta::ArrayTypeID>::Iter;
			using MetaArrayTypeConstIter = core::SyncLinearStepAlloc<meta::ArrayType, meta::ArrayTypeID>::ConstIter;

			[[nodiscard]] auto getMetaArrayTypeIter() -> evo::IterRange<MetaArrayTypeIter> {
				return evo::IterRange<MetaArrayTypeIter>(
					this->meta_array_types.begin(), this->meta_array_types.end()
				);
			}

			[[nodiscard]] auto getMetaArrayTypeIter() const -> evo::IterRange<MetaArrayTypeConstIter> {
				return evo::IterRange<MetaArrayTypeConstIter>(
					this->meta_array_types.cbegin(), this->meta_array_types.cend()
				);
			}

			[[nodiscard]] auto getMetaArrayTypeConstIter() const -> evo::IterRange<MetaArrayTypeConstIter> {
				return evo::IterRange<MetaArrayTypeConstIter>(
					this->meta_array_types.cbegin(), this->meta_array_types.cend()
				);
			}



			//////////////////////////////////
			// meta enum types

			[[nodiscard]] auto createMetaEnumType(
				std::string&& meta_name,
				std::string&& enum_name,
				meta::Type underlying_type,
				evo::SmallVector<meta::EnumType::Enumerator>&& enumerators,
				meta::FileID file_id,
				meta::Scope scope_where_defined,
				uint32_t line_number
			) -> meta::EnumType::ID {
				evo::debugAssert(underlying_type.is<meta::BasicType::ID>(), "Enum underlying type must be integral");
				evo::debugAssert(
					this->getMetaBasicType(underlying_type.as<meta::BasicType::ID>()).underlyingType.isIntegral(),
					"Enum underlying type must be integral"
				);
				#if defined(PCIT_CONFIG_DEBUG)
					this->check_meta_name_reuse(meta_name);
				#endif
				
				return this->meta_enum_types.emplace_back(
					std::move(meta_name),
					std::move(enum_name),
					underlying_type,
					std::move(enumerators),
					file_id,
					scope_where_defined,
					line_number
				);
			}

			[[nodiscard]] auto getMetaEnumType(meta::EnumType::ID id) const -> const meta::EnumType& {
				return this->meta_enum_types[id];
			}



			using MetaEnumTypeIter = core::SyncLinearStepAlloc<meta::EnumType, meta::EnumTypeID>::Iter;
			using MetaEnumTypeConstIter = core::SyncLinearStepAlloc<meta::EnumType, meta::EnumTypeID>::ConstIter;

			[[nodiscard]] auto getMetaEnumTypeIter() -> evo::IterRange<MetaEnumTypeIter> {
				return evo::IterRange<MetaEnumTypeIter>(
					this->meta_enum_types.begin(), this->meta_enum_types.end()
				);
			}

			[[nodiscard]] auto getMetaEnumTypeIter() const -> evo::IterRange<MetaEnumTypeConstIter> {
				return evo::IterRange<MetaEnumTypeConstIter>(
					this->meta_enum_types.cbegin(), this->meta_enum_types.cend()
				);
			}

			[[nodiscard]] auto getMetaEnumTypeConstIter() const -> evo::IterRange<MetaEnumTypeConstIter> {
				return evo::IterRange<MetaEnumTypeConstIter>(
					this->meta_enum_types.cbegin(), this->meta_enum_types.cend()
				);
			}



			///////////////////////////////////
			// meta functions

			[[nodiscard]] auto createMetaFunction(
				std::string&& meta_name,
				std::string&& unmangled_name,
				std::optional<meta::Type> return_meta_type, // nullopt if `Void`
				evo::SmallVector<meta::Type>&& param_meta_types,
				meta::File::ID file_id,
				meta::Scope scope_where_defined,
				uint32_t line_number
			) -> meta::Function::ID {
				#if defined(PCIT_CONFIG_DEBUG)
					this->check_meta_name_reuse(meta_name);
				#endif

				return this->meta_functions.emplace_back(
					std::move(meta_name),
					std::move(unmangled_name),
					return_meta_type,
					std::move(param_meta_types),
					file_id,
					scope_where_defined,
					line_number
				);
			}

			[[nodiscard]] auto getMetaFunction(meta::Function::ID id) const -> const meta::Function& {
				return this->meta_functions[id];
			}


			using MetaFunctionIter = core::SyncLinearStepAlloc<meta::Function, meta::FunctionID>::Iter;
			using MetaFunctionConstIter = core::SyncLinearStepAlloc<meta::Function, meta::FunctionID>::ConstIter;

			[[nodiscard]] auto getMetaFunctionIter() -> evo::IterRange<MetaFunctionIter> {
				return evo::IterRange<MetaFunctionIter>(this->meta_functions.begin(), this->meta_functions.end());
			}

			[[nodiscard]] auto getMetaFunctionIter() const -> evo::IterRange<MetaFunctionConstIter> {
				return evo::IterRange<MetaFunctionConstIter>(
					this->meta_functions.cbegin(), this->meta_functions.cend()
				);
			}

			[[nodiscard]] auto getMetaFunctionConstIter() const -> evo::IterRange<MetaFunctionConstIter> {
				return evo::IterRange<MetaFunctionConstIter>(
					this->meta_functions.cbegin(), this->meta_functions.cend()
				);
			}


		private:
			#if defined(PCIT_CONFIG_DEBUG)
				auto check_param_names(evo::ArrayProxy<Parameter> params) const -> void;

				auto check_global_name_reuse(std::string_view global_name) const -> void;
				auto check_meta_name_reuse(std::string_view meta_name) const -> void;

				auto check_expr_type_match(Type type, const Expr& expr) const -> void;
			#endif
	
		private:
			std::string name;
			core::Target target;

			core::StepAlloc<Function, Function::ID> functions{};
			core::StepAlloc<ExternalFunction, ExternalFunction::ID> external_funcs{};
			core::StepAlloc<GlobalVar, GlobalVar::ID> global_vars{};

			core::StepAlloc<BasicBlock, BasicBlock::ID> basic_blocks{};
			core::StepAlloc<Number, uint32_t> numbers{};


			core::LinearStepAlloc<ArrayType, uint32_t> array_types{};
			mutable evo::SpinLock array_types_lock{};

			core::SyncLinearStepAlloc<StructType, uint32_t> struct_types{};

			core::LinearStepAlloc<FunctionType, uint32_t> func_types{};
			mutable evo::SpinLock func_types_lock{};


			// exprs
			core::StepAlloc<Call, uint32_t> calls{};
			core::StepAlloc<CallVoid, uint32_t> call_voids{};
			core::StepAlloc<CallNoReturn, uint32_t> call_no_returns{};
			core::StepAlloc<Abort, uint32_t> aborts{};
			core::StepAlloc<Breakpoint, uint32_t> breakpoints{};
			core::StepAlloc<Ret, uint32_t> rets{};
			core::StepAlloc<Jump, uint32_t> jumps{};
			core::StepAlloc<Branch, uint32_t> branches{};
			core::StepAlloc<Phi, uint32_t> phis{};
			core::StepAlloc<Switch, uint32_t> switches{};
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
			core::StepAlloc<ByteSwap, uint32_t> byte_swaps{};
			core::StepAlloc<CtPop, uint32_t> ctpops{};
			core::StepAlloc<CTLZ, uint32_t> ctlzs{};
			core::StepAlloc<CTTZ, uint32_t> cttzs{};

			core::StepAlloc<CmpXchg, uint32_t> cmpxchgs{};
			core::StepAlloc<AtomicRMW, uint32_t> atomic_rmws{};

			core::StepAlloc<MetaLocalVar, uint32_t> meta_local_vars{};


			// global values
			core::StepAlloc<GlobalVar::String, GlobalVar::String::ID> global_strings{};
			core::StepAlloc<GlobalVar::Array, GlobalVar::Array::ID> global_arrays{};
			core::StepAlloc<GlobalVar::ByteArray, GlobalVar::ByteArray::ID> global_byte_arrays{};
			core::StepAlloc<GlobalVar::Struct, GlobalVar::Struct::ID> global_structs{};


			// meta
			core::SyncLinearStepAlloc<meta::File, meta::File::ID> meta_files{};
			core::SyncLinearStepAlloc<meta::BasicType, meta::BasicType::ID> meta_basic_types{};
			core::SyncLinearStepAlloc<meta::QualifiedType, meta::QualifiedType::ID> meta_qualified_types{};
			core::SyncLinearStepAlloc<meta::StructType, meta::StructType::ID> meta_struct_types{};
			core::SyncLinearStepAlloc<meta::ArrayType, meta::ArrayType::ID> meta_array_types{};
			core::SyncLinearStepAlloc<meta::EnumType, meta::EnumType::ID> meta_enum_types{};
			core::SyncLinearStepAlloc<meta::Function, meta::Function::ID> meta_functions{};

			std::unordered_map<Type, meta::StructType::ID> meta_struct_type_lookup{};
			mutable evo::SpinLock meta_struct_type_lookup_lock{};


			friend class InstrReader;
			friend class InstrHandler;
	};


}


