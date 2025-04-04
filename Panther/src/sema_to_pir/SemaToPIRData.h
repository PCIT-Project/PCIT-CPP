////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <stack>

#include <Evo.h>
#include <PCIT_core.h>
#include <PIR.h>

#include "../../include/sema/sema.h"


namespace pcit::panther{


	class SemaToPIRData{
		public:
			struct Config{
				bool useReadableNames;
				bool checkedMath;
				bool isJIT;
				bool addSourceLocations;
			};
			
			struct FuncInfo{
				pir::Function::ID pir_id;
				pir::Type return_type;
				evo::SmallVector<bool> arg_is_copy;
				evo::SmallVector<pir::Expr> return_params; // only used if they are out params
				std::optional<pir::Expr> error_return_param;
				std::optional<pir::Type> error_return_type;
			};


			struct JITInterfaceFuncs{
				pir::ExternalFunction::ID return_generic_int  = pir::ExternalFunction::ID::dummy();
				pir::ExternalFunction::ID return_generic_bool = pir::ExternalFunction::ID::dummy();
				pir::ExternalFunction::ID return_generic_f16  = pir::ExternalFunction::ID::dummy();
				pir::ExternalFunction::ID return_generic_bf16 = pir::ExternalFunction::ID::dummy();
				pir::ExternalFunction::ID return_generic_f32  = pir::ExternalFunction::ID::dummy();
				pir::ExternalFunction::ID return_generic_f64  = pir::ExternalFunction::ID::dummy();
				pir::ExternalFunction::ID return_generic_f80  = pir::ExternalFunction::ID::dummy();
				pir::ExternalFunction::ID return_generic_f128 = pir::ExternalFunction::ID::dummy();
				pir::ExternalFunction::ID return_generic_char = pir::ExternalFunction::ID::dummy();
			};

		public:
			SemaToPIRData(Config&& _config) : config(_config) {}
			~SemaToPIRData() = default;

			EVO_NODISCARD auto getConfig() const -> const Config& { return this->config; }

			EVO_NODISCARD auto getJITInterfaceFuncs() const -> const JITInterfaceFuncs& {
				return this->jit_interface_funcs;
			}

			EVO_NODISCARD auto getJITInterfaceFuncsArray() const -> evo::ArrayProxy<pir::ExternalFunction::ID> {
				return evo::ArrayProxy<pir::ExternalFunction::ID>(
					reinterpret_cast<const pir::ExternalFunction::ID*>(&this->jit_interface_funcs),
					sizeof(JITInterfaceFuncs) / sizeof(pir::ExternalFunction::ID)
				);
			}





			auto createNeededJITInterfaceFuncDecls(pir::Module& module) -> void;


			auto create_struct(const BaseType::Struct::ID struct_id, pir::Type pir_id) -> void {
				const auto lock = std::scoped_lock(this->structs_lock);
				const auto emplace_result = this->structs.emplace(struct_id.get(), pir_id);
				evo::debugAssert(emplace_result.second, "This struct id was already added to PIR lower");
			}


			auto create_global_var(const sema::GlobalVar::ID global_var_id, pir::GlobalVar::ID pir_id) -> void {
				const auto lock = std::scoped_lock(this->global_vars_lock);
				const auto emplace_result = this->global_vars.emplace(global_var_id.get(), pir_id);
				evo::debugAssert(emplace_result.second, "This global var id was already added to PIR lower");
			}


			auto create_func(const sema::Func::ID func_id, auto&&... func_info_args) -> void {
				const auto lock = std::scoped_lock(this->funcs_lock);
				const auto emplace_result = this->funcs.emplace(
					func_id.get(),
					&this->funcs_info_alloc.emplace_back(
						std::forward<decltype(func_info_args)>(func_info_args)...
					)
				);
				evo::debugAssert(emplace_result.second, "This func id was already added to PIR lower");
			}



			EVO_NODISCARD auto get_struct(const BaseType::Struct::ID struct_id) -> pir::Type {
				const auto lock = std::scoped_lock(this->structs_lock);
				return this->structs.at(struct_id.get());
			}

			EVO_NODISCARD auto get_global_var(const sema::GlobalVar::ID global_var_id) -> pir::GlobalVar::ID {
				const auto lock = std::scoped_lock(this->global_vars_lock);
				return this->global_vars.at(global_var_id.get());
			}

			EVO_NODISCARD auto get_func(const sema::Func::ID func_id) -> FuncInfo& {
				const auto lock = std::scoped_lock(this->funcs_lock);
				return *this->funcs.at(func_id.get());
			}
	
		private:
			Config config;

			std::unordered_map<uint32_t, pir::Type> structs{};
			mutable core::SpinLock structs_lock{};

			std::unordered_map<uint32_t, pir::GlobalVar::ID> global_vars{};
			mutable core::SpinLock global_vars_lock{};

			core::StepVector<FuncInfo> funcs_info_alloc{};
			std::unordered_map<uint32_t, FuncInfo*> funcs{};
			mutable core::SpinLock funcs_lock{};

			JITInterfaceFuncs jit_interface_funcs{};
	};


}
