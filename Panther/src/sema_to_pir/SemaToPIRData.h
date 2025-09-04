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

#include <Evo.h>
#include <PCIT_core.h>
#include <PIR.h>

#include "../../include/sema/sema.h"



namespace pcit::panther{
	
	struct SemaToPIRDataVTableID{
		BaseType::Interface::ID interface_id;
		BaseType::ID impl_id;

		EVO_NODISCARD auto operator==(const SemaToPIRDataVTableID&) const -> bool = default;
	};

}



namespace std{


	template<>
	struct hash<pcit::panther::SemaToPIRDataVTableID>{
		auto operator()(const pcit::panther::SemaToPIRDataVTableID& key) const noexcept -> size_t {
			return evo::hashCombine(
				std::hash<uint32_t>{}(key.interface_id.get()), std::hash<pcit::panther::BaseType::ID>{}(key.impl_id)
			);
		};
	};

	
}



namespace pcit::panther{


	class SemaToPIRData{
		public:
			struct Config{
				bool useReadableNames;
				bool checkedMath;
				bool isJIT;
				bool addSourceLocations;

				bool useDebugUnreachables;
			};
			
			struct FuncInfo{
				struct Param{
					std::optional<pir::Type> reference_type;
					std::optional<uint32_t> in_param_index;

					EVO_NODISCARD auto is_copy() const -> bool { return this->reference_type.has_value() == false; }
				};

				evo::SmallVector<evo::Variant<pir::Function::ID, pir::ExternalFunction::ID>> pir_ids;
				pir::Type return_type;
				evo::SmallVector<Param> params;
				evo::SmallVector<pir::Expr> return_params; // only used if they are out params
				std::optional<pir::Expr> error_return_param;
				std::optional<pir::Type> error_return_type;
			};


			struct JITBuildFuncs{
				pir::ExternalFunction::ID build_set_num_threads = pir::ExternalFunction::ID::dummy();
				pir::ExternalFunction::ID build_set_output      = pir::ExternalFunction::ID::dummy();
				pir::ExternalFunction::ID build_set_use_std_lib = pir::ExternalFunction::ID::dummy();
			};

			using VTableID = SemaToPIRDataVTableID;

		public:
			SemaToPIRData(Config&& _config) : config(_config) {}
			~SemaToPIRData() = default;

			EVO_NODISCARD auto getConfig() const -> const Config& { return this->config; }

			auto getInterfacePtrType(pir::Module& module) -> pir::Type;
			auto getArrayRefType(pir::Module& module, unsigned num_dimensions) -> pir::Type;



			//////////////////
			// JIT build funcs

			EVO_NODISCARD auto getJITBuildFuncs() const -> const JITBuildFuncs& {
				return this->jit_build_funcs;
			}

			auto createJITBuildFuncDecls(pir::Module& module) -> void;




		private:	
			auto create_struct(BaseType::Struct::ID struct_id, pir::Type pir_id) -> void {
				const auto lock = std::scoped_lock(this->structs_lock);
				const auto emplace_result = this->structs.emplace(struct_id, pir_id);
				evo::debugAssert(emplace_result.second, "This struct id was already added to PIR lower");
			}

			auto create_union(BaseType::Union::ID union_id, pir::Type pir_id) -> void {
				const auto lock = std::scoped_lock(this->unions_lock);
				const auto emplace_result = this->unions.emplace(union_id, pir_id);
				evo::debugAssert(emplace_result.second, "This union id was already added to PIR lower");
			}


			auto create_global_var(sema::GlobalVar::ID global_var_id, pir::GlobalVar::ID pir_id) -> void {
				const auto lock = std::scoped_lock(this->global_vars_lock);
				const auto emplace_result = this->global_vars.emplace(global_var_id, pir_id);
				evo::debugAssert(emplace_result.second, "This global var id was already added to PIR lower");
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
			}



			EVO_NODISCARD auto get_struct(BaseType::Struct::ID struct_id) -> pir::Type {
				const auto lock = std::scoped_lock(this->structs_lock);
				evo::debugAssert(this->structs.contains(struct_id), "Doesn't have this struct");
				return this->structs.at(struct_id);
			}

			EVO_NODISCARD auto get_union(BaseType::Union::ID union_id) -> pir::Type {
				const auto lock = std::scoped_lock(this->unions_lock);
				evo::debugAssert(this->unions.contains(union_id), "Doesn't have this union");
				return this->unions.at(union_id);
			}

			EVO_NODISCARD auto get_global_var(sema::GlobalVar::ID global_var_id) -> pir::GlobalVar::ID {
				const auto lock = std::scoped_lock(this->global_vars_lock);
				evo::debugAssert(this->global_vars.contains(global_var_id), "Doesn't have this global var");
				return this->global_vars.at(global_var_id);
			}

			EVO_NODISCARD auto get_func(sema::Func::ID func_id) -> FuncInfo& {
				const auto lock = std::scoped_lock(this->funcs_lock);
				evo::debugAssert(this->funcs.contains(func_id), "Doesn't have this func");
				return *this->funcs.at(func_id);
			}

			EVO_NODISCARD auto get_vtable(VTableID vtable_id) -> pir::GlobalVar::ID {
				const auto lock = std::scoped_lock(this->vtables_lock);
				evo::debugAssert(this->vtables.contains(vtable_id), "Doesn't have this vtable");
				return this->vtables.at(vtable_id);
			}



			EVO_NODISCARD auto has_struct(BaseType::Struct::ID struct_id) -> bool {
				const auto lock = std::scoped_lock(this->structs_lock);
				return this->structs.contains(struct_id);
			}

			EVO_NODISCARD auto has_union(BaseType::Union::ID union_id) -> bool {
				const auto lock = std::scoped_lock(this->unions_lock);
				return this->unions.contains(union_id);
			}


			EVO_NODISCARD auto get_string_literal_id() -> uint64_t {
				return this->num_string_literals.fetch_add(1);
			}

	
		private:
			Config config;


			std::optional<pir::Type> interface_ptr_type = std::nullopt;
			mutable core::SpinLock interface_ptr_type_lock{};	

			evo::SmallVector<std::optional<pir::Type>> array_ref_type{};
			mutable core::SpinLock array_ref_type_lock{};	


			std::unordered_map<BaseType::Struct::ID, pir::Type> structs{};
			mutable core::SpinLock structs_lock{};

			std::unordered_map<BaseType::Union::ID, pir::Type> unions{};
			mutable core::SpinLock unions_lock{};

			std::unordered_map<sema::GlobalVar::ID, pir::GlobalVar::ID> global_vars{};
			mutable core::SpinLock global_vars_lock{};

			evo::StepVector<FuncInfo> funcs_info_alloc{};
			std::unordered_map<sema::Func::ID, FuncInfo*> funcs{};
			mutable core::SpinLock funcs_lock{};

			std::unordered_set<std::string> extern_funcs{};
			mutable core::SpinLock extern_funcs_lock{};

			JITBuildFuncs jit_build_funcs{};

			std::unordered_map<VTableID, pir::GlobalVar::ID> vtables{};
			mutable core::SpinLock vtables_lock{};

			std::unordered_map<const TypeInfo*, pir::Type> optional_types{};
			mutable core::SpinLock optional_types_lock{};

			std::atomic<uint64_t> num_string_literals = 0;

			friend class SemaToPIR;
	};


}

