////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <stack>
#include <unordered_set>

#include <Evo.hpp>
#include <PCIT_core.hpp>
#include <PIR.hpp>

#include "../../include/sema/sema.hpp"
#include "../../include/TypeManager.hpp"



namespace pcit::panther{
	
	struct SemaToPIRDataVTableID{
		BaseType::Interface::ID interface_id;
		TypeInfo::ID impl_id;

		[[nodiscard]] auto operator==(const SemaToPIRDataVTableID&) const -> bool = default;
	};

}



namespace std{


	template<>
	struct hash<pcit::panther::SemaToPIRDataVTableID>{
		auto operator()(const pcit::panther::SemaToPIRDataVTableID& key) const noexcept -> size_t {
			return evo::hashCombine(
				std::hash<uint32_t>{}(key.interface_id.get()), std::hash<pcit::panther::TypeInfo::ID>{}(key.impl_id)
			);
		};
	};

	
}



namespace pcit::panther{


	class SemaToPIRData{
		public:
			struct Config{
				bool includeDebugInfo;
				bool useReadableNames;
				bool checkedMath;

				bool useDebugUnreachables;
			};
			
			struct FuncInfo{
				struct Param{
					std::optional<pir::Type> reference_type;
					std::optional<uint32_t> in_param_index;

					[[nodiscard]] auto is_copy() const -> bool { return this->reference_type.has_value() == false; }
				};

				struct ReturnParam{
					pir::Expr expr;
					pir::Type reference_type;
				};

				evo::SmallVector<evo::Variant<std::monostate, pir::Function::ID, pir::ExternalFunction::ID>> pir_ids;
				pir::Type return_type;
				bool isImplicitRVO;
				bool isNoReturn;
				evo::SmallVector<Param> params;
				evo::SmallVector<ReturnParam> return_params; // only used if they are out params
				std::optional<pir::Expr> error_return_param;
				std::optional<pir::Type> error_return_type;
			};


			struct JITBuildFuncs{
				pir::ExternalFunction::ID build_set_num_threads      = pir::ExternalFunction::ID::dummy();
				pir::ExternalFunction::ID build_set_output           = pir::ExternalFunction::ID::dummy();
				pir::ExternalFunction::ID build_set_add_debug_info   = pir::ExternalFunction::ID::dummy();
				pir::ExternalFunction::ID build_set_std_lib_package  = pir::ExternalFunction::ID::dummy();
				pir::ExternalFunction::ID build_create_package       = pir::ExternalFunction::ID::dummy();
				pir::ExternalFunction::ID build_add_source_file      = pir::ExternalFunction::ID::dummy();
				pir::ExternalFunction::ID build_add_source_directory = pir::ExternalFunction::ID::dummy();
				pir::ExternalFunction::ID build_add_c_header_file    = pir::ExternalFunction::ID::dummy();
				pir::ExternalFunction::ID build_add_cpp_header_file  = pir::ExternalFunction::ID::dummy();
			};

			using VTableID = SemaToPIRDataVTableID;


			struct InterfaceInfo{
				evo::SmallVector<std::optional<pir::Type>> error_return_types;
			};

			struct PIRType{
				pir::Type pir_type;
				std::optional<pir::meta::StructType::ID> meta_type_id;
			};

		public:
			SemaToPIRData(Config&& _config) : config(_config) {}
			~SemaToPIRData() = default;

			[[nodiscard]] auto getConfig() const -> const Config& { return this->config; }

			[[nodiscard]] auto getInterfacePtrType(pir::Module& module, class SourceManager& source_manager)
				-> const PIRType&;

			[[nodiscard]] auto getArrayRefType(
				pir::Module& module,
				class Context& context,
				BaseType::ArrayRef::ID array_ref_id,
				const std::function<pir::meta::Type(TypeInfo::ID)>& get_data_ptr_meta_type
			) -> const PIRType&;
			[[nodiscard]] auto getArrayRefType(
				pir::Module& module,
				class Context& context,
				TypeInfo::ID array_ref_id, 
				const std::function<pir::meta::Type(TypeInfo::ID)>& get_data_ptr_meta_type
			) -> const PIRType&;


			[[nodiscard]] auto lookupGlobalVar(pir::GlobalVar::ID id) const -> std::optional<sema::GlobalVar::ID>;
			[[nodiscard]] auto lookupGlobalString(pir::GlobalVar::ID id) const -> std::optional<sema::StringValue::ID>;
			[[nodiscard]] auto lookupVTable(pir::GlobalVar::ID id) const -> std::optional<VTableID>;
			[[nodiscard]] auto lookupSingleMethodVTable(pir::Function::ID id) const -> std::optional<VTableID>;



			//////////////////
			// JIT build funcs

			[[nodiscard]] auto getJITBuildFuncs() const -> const JITBuildFuncs& {
				return this->jit_build_funcs;
			}

			auto createJITBuildFuncDecls(pir::Module& module) -> void;




		private:
			auto create_struct(BaseType::Struct::ID struct_id, pir::Type pir_id) -> void {
				const auto lock = std::scoped_lock(this->structs_lock);
				const auto emplace_result = this->structs.emplace(struct_id, pir_id);
				evo::debugAssert(emplace_result.second, "This struct id was already added to PIR lower");
			}

			[[nodiscard]] auto get_struct(BaseType::Struct::ID struct_id) -> pir::Type {
				const auto lock = std::scoped_lock(this->structs_lock);
				evo::debugAssert(this->structs.contains(struct_id), "Doesn't have this struct");
				return this->structs.at(struct_id);
			}

			[[nodiscard]] auto has_struct(BaseType::Struct::ID struct_id) -> bool {
				const auto lock = std::scoped_lock(this->structs_lock);
				return this->structs.contains(struct_id);
			}



			auto create_union(BaseType::Union::ID union_id, pir::Type pir_id) -> void {
				const auto lock = std::scoped_lock(this->unions_lock);
				const auto emplace_result = this->unions.emplace(union_id, pir_id);
				evo::debugAssert(emplace_result.second, "This union id was already added to PIR lower");
			}

			[[nodiscard]] auto get_union(BaseType::Union::ID union_id) -> pir::Type {
				const auto lock = std::scoped_lock(this->unions_lock);
				evo::debugAssert(this->unions.contains(union_id), "Doesn't have this union");
				return this->unions.at(union_id);
			}

			[[nodiscard]] auto has_union(BaseType::Union::ID union_id) -> bool {
				const auto lock = std::scoped_lock(this->unions_lock);
				return this->unions.contains(union_id);
			}



			auto create_global_var(sema::GlobalVar::ID global_var_id, pir::GlobalVar::ID pir_id) -> void {
				const auto lock = std::scoped_lock(this->global_vars_lock);
				const auto emplace_result = this->global_vars.emplace(global_var_id, pir_id);
				this->reverse_global_vars.emplace(pir_id, global_var_id);
				evo::debugAssert(emplace_result.second, "This global var id was already added to PIR lower");
			}

			[[nodiscard]] auto get_global_var(sema::GlobalVar::ID global_var_id) -> pir::GlobalVar::ID {
				const auto lock = std::scoped_lock(this->global_vars_lock);
				evo::debugAssert(this->global_vars.contains(global_var_id), "Doesn't have this global var");
				return this->global_vars.at(global_var_id);
			}



			auto create_global_string(sema::StringValue::ID global_string_id, pir::GlobalVar::ID pir_id) -> void {
				const auto lock = std::scoped_lock(this->global_strings_lock);
				this->global_strings.emplace(global_string_id, pir_id);
				this->reverse_global_strings.emplace(pir_id, global_string_id);
			}

			[[nodiscard]] auto get_global_string(sema::StringValue::ID string_value_id) -> pir::GlobalVar::ID {
				const auto lock = std::scoped_lock(this->global_vars_lock);
				evo::debugAssert(this->global_strings.contains(string_value_id), "Doesn't have this global string");
				return this->global_strings.at(string_value_id);
			}



			auto create_func(sema::Func::ID func_id, auto&&... func_info_args) -> void {
				const auto lock = std::scoped_lock(this->funcs_lock);
				const auto emplace_result = this->funcs.emplace(
					func_id,
					&this->funcs_info_alloc.emplace_back(
						std::forward<decltype(func_info_args)>(func_info_args)...
					)
				);
				evo::debugAssert(emplace_result.second, "This func id was already added to PIR lower");
			}

			[[nodiscard]] auto get_func(sema::Func::ID func_id) -> FuncInfo& {
				const auto lock = std::scoped_lock(this->funcs_lock);
				evo::debugAssert(this->funcs.contains(func_id), "Doesn't have this func");
				return *this->funcs.at(func_id);
			}


			// returns true if added
			auto add_extern_func_if_needed(const std::string& func_name) -> bool {
				const auto lock = std::scoped_lock(this->extern_funcs_lock);
				const auto emplace_result = this->extern_funcs.emplace(func_name);
				return emplace_result.second;
			}
			auto add_extern_func_if_needed(std::string&& func_name) -> bool {
				const auto lock = std::scoped_lock(this->extern_funcs_lock);
				const auto emplace_result = this->extern_funcs.emplace(std::move(func_name));
				return emplace_result.second;
			}



			auto create_vtable(VTableID vtable_id, pir::GlobalVar::ID pir_id) -> void {
				const auto lock = std::scoped_lock(this->vtables_lock);
				const auto emplace_result = this->vtables.emplace(vtable_id, pir_id);
				evo::debugAssert(emplace_result.second, "This vtable id was already added to PIR lower");
				this->reverse_vtables.emplace(pir_id, vtable_id);
			}

			[[nodiscard]] auto get_vtable(VTableID vtable_id) -> pir::GlobalVar::ID {
				const auto lock = std::scoped_lock(this->vtables_lock);
				evo::debugAssert(this->vtables.contains(vtable_id), "Doesn't have this vtable");
				return this->vtables.at(vtable_id);
			}



			auto create_single_method_vtable(VTableID single_method_vtable_id, pir::Function::ID pir_id) -> void {
				const auto lock = std::scoped_lock(this->single_method_vtables_lock);
				const auto emplace_result = this->single_method_vtables.emplace(single_method_vtable_id, pir_id);
				evo::debugAssert(emplace_result.second, "This single_method_vtable id was already added to PIR lower");
				this->reverse_single_method_vtables.emplace(pir_id, single_method_vtable_id);
			}

			[[nodiscard]] auto get_single_method_vtable(VTableID single_method_vtable_id) -> pir::Function::ID {
				const auto lock = std::scoped_lock(this->single_method_vtables_lock);
				evo::debugAssert(
					this->single_method_vtables.contains(single_method_vtable_id),
					"Doesn't have this single-method vtable"
				);
				return this->single_method_vtables.at(single_method_vtable_id);
			}



			auto create_interface(BaseType::Interface::ID interface_id, auto&&... interface_info_args) -> void {
				const auto lock = std::scoped_lock(this->interfaces_lock);
				const auto emplace_result = this->interfaces.emplace(
					interface_id,
					&this->interfaces_info_alloc.emplace_back(
						std::forward<decltype(interface_info_args)>(interface_info_args)...
					)
				);
				evo::debugAssert(emplace_result.second, "This interface id was already added to PIR lower");
			}

			[[nodiscard]] auto get_interface(BaseType::Interface::ID interface_id) -> InterfaceInfo& {
				const auto lock = std::scoped_lock(this->interfaces_lock);
				evo::debugAssert(this->interfaces.contains(interface_id), "Doesn't have this interface");
				return *this->interfaces.at(interface_id);
			}


			auto get_or_create_meta_basic_type(
				TypeInfo::ID id, pir::Module& module, std::string&& type_name, pir::Type pir_type
			) -> pir::meta::BasicType::ID {
				const auto value_handle = this->meta_basic_types.get(id);
				if(value_handle.needsToBeSet() == false){ return value_handle.getValue(); }

				return value_handle.emplaceValue(
					module.createMetaBasicType(std::string(type_name), std::string(type_name), pir_type)
				);
			}


			[[nodiscard]] auto get_or_create_meta_pointer_qualified_type(
				TypeInfo::ID id,
				pir::Module& module,
				std::string&& type_name,
				std::optional<pir::meta::Type> qualee_type,
				pir::meta::QualifiedType::Qualifier qualifier
			) -> pir::meta::QualifiedType::ID {
				evo::debugAssert(
					qualifier == pir::meta::QualifiedType::Qualifier::POINTER
						|| qualifier == pir::meta::QualifiedType::Qualifier::MUT_POINTER,
					"Must be a pointer qualifier"
				);

				const auto value_handle = this->meta_pointer_qualified_types.get(id);
				if(value_handle.needsToBeSet() == false){ return value_handle.getValue(); }

				return value_handle.emplaceValue(
					module.createMetaQualifiedType(
						std::string(type_name), std::string(type_name), qualee_type, qualifier
					)
				);
			}


			[[nodiscard]] auto get_or_create_meta_reference_qualified_type(
				TypeInfo::ID id,
				pir::Module& module,
				std::string&& type_name,
				pir::meta::Type qualee_type,
				pir::meta::QualifiedType::Qualifier qualifier
			) -> pir::meta::QualifiedType::ID {
				evo::debugAssert(
					qualifier == pir::meta::QualifiedType::Qualifier::REFERENCE
						|| qualifier == pir::meta::QualifiedType::Qualifier::MUT_REFERENCE,
					"Must be a reference qualifier"
				);

				const auto value_handle = this->meta_reference_qualified_types.get(id);
				if(value_handle.needsToBeSet() == false){ return value_handle.getValue(); }

				return value_handle.emplaceValue(
					module.createMetaQualifiedType(
						std::string(type_name), std::string(type_name), qualee_type, qualifier
					)
				);
			}




			[[nodiscard]] auto get_or_create_meta_array_type(
				BaseType::Array::ID id,
				pir::Module& module,
				const TypeManager& type_manager,
				const Context& context,
				pir::Type array_type,
				pir::meta::Type element_type,
				evo::SmallVector<uint64_t>&& dimensions
			) -> pir::meta::ArrayType::ID {
				const auto value_handle = this->meta_array_types.get(id);
				if(value_handle.needsToBeSet() == false){ return value_handle.getValue(); }

				return value_handle.emplaceValue(
					module.createMetaArrayType(
						type_manager.printType(BaseType::ID(id), context),
						array_type,
						element_type,
						std::move(dimensions)
					)
				);
			}



			auto create_meta_enum(BaseType::Enum::ID enum_id, pir::meta::EnumType::ID enum_type_id) -> void {
				const auto lock = std::scoped_lock(this->meta_enum_types_lock);
				this->meta_enum_types.emplace(enum_id, enum_type_id);
			}

			[[nodiscard]] auto get_meta_enum(BaseType::Enum::ID enum_id) const -> pir::meta::EnumType::ID {
				const auto lock = std::scoped_lock(this->meta_enum_types_lock);
				return this->meta_enum_types.at(enum_id);
			}

			[[nodiscard]] auto has_meta_enum(BaseType::Enum::ID enum_id) const -> bool {
				const auto lock = std::scoped_lock(this->meta_enum_types_lock);
				return this->meta_enum_types.contains(enum_id);
			}






			[[nodiscard]] auto get_string_literal_id() -> uint64_t {
				return this->num_string_literals.fetch_add(1);
			}

			[[nodiscard]] auto get_byte_array_id() -> uint64_t {
				return this->num_byte_arrays.fetch_add(1);
			}

	
		private:
			Config config;


			std::optional<PIRType> interface_ptr_type = std::nullopt;
			mutable evo::SpinLock interface_ptr_type_lock{};	

			core::MapAlloc<BaseType::ArrayRef::ID, PIRType> array_ref_type_infos{};

			std::unordered_map<BaseType::Struct::ID, pir::Type> structs{};
			mutable evo::SpinLock structs_lock{};

			std::unordered_map<BaseType::Union::ID, pir::Type> unions{};
			mutable evo::SpinLock unions_lock{};

			std::unordered_map<sema::GlobalVar::ID, pir::GlobalVar::ID> global_vars{};
			std::unordered_map<pir::GlobalVar::ID, sema::GlobalVar::ID> reverse_global_vars{};
			mutable evo::SpinLock global_vars_lock{};

			std::unordered_map<sema::StringValue::ID, pir::GlobalVar::ID> global_strings{};
			std::unordered_map<pir::GlobalVar::ID, sema::StringValue::ID> reverse_global_strings{};
			mutable evo::SpinLock global_strings_lock{};

			evo::StepVector<FuncInfo> funcs_info_alloc{};
			std::unordered_map<sema::Func::ID, FuncInfo*> funcs{};
			mutable evo::SpinLock funcs_lock{};

			std::unordered_set<std::string> extern_funcs{};
			mutable evo::SpinLock extern_funcs_lock{};

			JITBuildFuncs jit_build_funcs{};

			std::unordered_map<VTableID, pir::GlobalVar::ID> vtables{};
			std::unordered_map<pir::GlobalVar::ID, VTableID> reverse_vtables{};
			mutable evo::SpinLock vtables_lock{};

			std::unordered_map<VTableID, pir::Function::ID> single_method_vtables{};
			std::unordered_map<pir::Function::ID, VTableID> reverse_single_method_vtables{};
			mutable evo::SpinLock single_method_vtables_lock{};

			evo::StepVector<InterfaceInfo> interfaces_info_alloc{};
			std::unordered_map<BaseType::Interface::ID, InterfaceInfo*> interfaces{};
			mutable evo::SpinLock interfaces_lock{};

			std::unordered_map<const TypeInfo*, pir::Type> optional_types{};
			mutable evo::SpinLock optional_types_lock{};

			std::atomic<uint64_t> num_string_literals = 0;
			std::atomic<uint64_t> num_byte_arrays = 0;


			core::MapAlloc<TypeInfo::ID, pir::meta::BasicType::ID> meta_basic_types{};
			core::MapAlloc<TypeInfo::ID, pir::meta::QualifiedType::ID> meta_pointer_qualified_types{};
			core::MapAlloc<TypeInfo::ID, pir::meta::QualifiedType::ID> meta_reference_qualified_types{};
			core::MapAlloc<BaseType::Array::ID, pir::meta::ArrayType::ID> meta_array_types{};

			std::unordered_map<BaseType::Enum::ID, pir::meta::EnumType::ID> meta_enum_types{};
			mutable evo::SpinLock meta_enum_types_lock{};


			friend class SemaToPIR;
	};


}

