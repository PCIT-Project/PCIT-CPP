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


	class SemaToPIR{
		public:
			class Data{
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
						evo::SmallVector<pir::Expr> error_return_params;
					};

				public:
					Data(Config&& _config) : config(_config) {}
					~Data() = default;

					EVO_NODISCARD auto getConfig() const -> const Config& { return this->config; }

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
			};

		public:
			SemaToPIR(class Context& _context, pir::Module& _module, Data& _data)
				: context(_context), module(_module), agent(_module), data(_data) {}
			~SemaToPIR() = default;

			auto lower() -> void;

			auto lowerStruct(const BaseType::Struct::ID struct_id) -> void;
			auto lowerGlobal(const sema::GlobalVar::ID global_var_id) -> void;
			auto lowerFuncDecl(const sema::Func::ID func_id) -> pir::Function::ID;
			auto lowerFuncDef(const sema::Func::ID func_id) -> void;


		private:
			auto lower_stmt(const sema::Stmt& stmt) -> void;

			EVO_NODISCARD auto get_expr_register(const sema::Expr expr) -> pir::Expr;
			EVO_NODISCARD auto get_expr_pointer(const sema::Expr expr) -> pir::Expr;
			EVO_NODISCARD auto get_expr_store(const sema::Expr expr, evo::ArrayProxy<pir::Expr> store_locations)
				-> void;

			enum class GetExprMode{
				Register,
				Pointer,
				Store,
			};

			template<GetExprMode MODE>
			auto get_expr_impl(const sema::Expr expr, evo::ArrayProxy<pir::Expr> store_locations)
				-> std::optional<pir::Expr>;



			EVO_NODISCARD auto get_global_var_value(const sema::Expr expr) -> pir::GlobalVar::Value;

			EVO_NODISCARD auto get_type(const TypeInfo::VoidableID voidable_type_id) -> pir::Type;
			EVO_NODISCARD auto get_type(const TypeInfo::ID type_id) -> pir::Type;
			EVO_NODISCARD auto get_type(const TypeInfo& type_info) -> pir::Type;
			EVO_NODISCARD auto get_type(const BaseType::ID base_type_id) -> pir::Type;

			EVO_NODISCARD auto mangle_name(const BaseType::Struct::ID struct_id) const -> std::string;
			EVO_NODISCARD auto mangle_name(const sema::GlobalVar::ID global_var_id) const -> std::string;
			EVO_NODISCARD auto mangle_name(const sema::Func::ID func_id) const -> std::string;


			EVO_NODISCARD auto name(std::string_view str) const -> std::string;
			
			template<class... Args>
			EVO_NODISCARD auto name(std::format_string<Args...> fmt, Args&&... args) const -> std::string;





	
		private:
			class Context& context;
			pir::Module& module;
			pir::Agent agent;

			class Source* current_source = nullptr;

			Data& data;
	};


}
