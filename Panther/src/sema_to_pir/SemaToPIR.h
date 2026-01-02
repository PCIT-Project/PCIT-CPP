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
#include "./SemaToPIRData.h"



namespace pcit::panther{

	struct ManagedLifetimeErrorParam{
		uint32_t index;

		EVO_NODISCARD auto operator==(const ManagedLifetimeErrorParam&) const -> bool = default;
	};

}


namespace std{
	
	template<>
	struct hash<pcit::panther::ManagedLifetimeErrorParam>{
		auto operator()(pcit::panther::ManagedLifetimeErrorParam ret_param) const noexcept -> size_t {
			return std::hash<uint32_t>{}(ret_param.index);
		};
	};

}





namespace pcit::panther{


	class SemaToPIR{
		public:
			using Data = SemaToPIRData;

		public:
			SemaToPIR(class Context& _context, pir::Module& _module, Data& _data)
				: context(_context), module(_module), agent(_module), data(_data) {}
			~SemaToPIR() = default;

			auto lower() -> void;


			auto lowerStruct(BaseType::Struct::ID struct_id) -> pir::Type;
			// not thread-safe
			auto lowerStructAndDependencies(BaseType::Struct::ID struct_id) -> pir::Type; 

			auto lowerUnion(BaseType::Union::ID union_id) -> pir::Type;
			// not thread-safe
			auto lowerUnionAndDependencies(BaseType::Union::ID union_id) -> pir::Type;

			auto lowerGlobalDecl(sema::GlobalVar::ID global_var_id) -> std::optional<pir::GlobalVar::ID>;
			auto lowerGlobalDef(sema::GlobalVar::ID global_var_id) -> void;
			EVO_NODISCARD auto lowerFuncDeclConstexpr(sema::Func::ID func_id) -> pir::Function::ID;
			auto lowerFuncDecl(sema::Func::ID func_id) -> void;
			auto lowerFuncDef(sema::Func::ID func_id) -> void;

			auto lowerInterface(BaseType::Interface::ID interface_id) -> void;

			auto lowerInterfaceVTable(
				BaseType::Interface::ID interface_id,
				TypeInfo::ID type_id,
				const evo::SmallVector<sema::Func::ID>& funcs
			) -> void;
			auto lowerInterfaceVTableConstexpr(
				BaseType::Interface::ID interface_id,
				TypeInfo::ID type_id,
				const evo::SmallVector<sema::Func::ID>& funcs
			) -> void;
			

			auto createJITEntry(sema::Func::ID target_entry_func) -> pir::Function::ID;
			auto createConsoleExecutableEntry(sema::Func::ID target_entry_func) -> pir::Function::ID;
			auto createWindowedExecutableEntry(sema::Func::ID target_entry_func) -> pir::Function::ID;


			EVO_NODISCARD auto createFuncJITInterface(sema::Func::ID func_id, pir::Function::ID pir_func_id)
				-> pir::Function::ID;


		private:
			template<bool MAY_LOWER_DEPENDENCY> // not thread-safe if true
			EVO_NODISCARD auto lower_struct(BaseType::Struct::ID struct_id) -> pir::Type;

			template<bool MAY_LOWER_DEPENDENCY> // not thread-safe if true
			EVO_NODISCARD auto lower_union(BaseType::Union::ID union_id) -> pir::Type;

			// see definition for explanation
			auto lower_func_decl(sema::Func::ID func_id) -> std::optional<pir::Function::ID>;

			template<bool IS_CONSTEXPR>
			auto lower_interface_vtable_impl(
				BaseType::Interface::ID interface_id,
				TypeInfo::ID type_id,
				const evo::SmallVector<sema::Func::ID>& funcs
			) -> void;

			auto lower_stmt(const sema::Stmt& stmt) -> void;

			EVO_NODISCARD auto get_expr_register(const sema::Expr expr) -> pir::Expr;
			EVO_NODISCARD auto get_expr_pointer(const sema::Expr expr) -> pir::Expr;
			EVO_NODISCARD auto get_expr_store(const sema::Expr expr, evo::ArrayProxy<pir::Expr> store_locations)
				-> void;
			EVO_NODISCARD auto get_expr_discard(const sema::Expr expr) -> void;

			enum class GetExprMode{
				REGISTER,
				POINTER,
				STORE,
				DISCARD,
			};

			template<GetExprMode MODE>
			auto get_expr_impl(const sema::Expr expr, evo::ArrayProxy<pir::Expr> store_locations)
				-> std::optional<pir::Expr>;


			template<GetExprMode MODE>
			auto default_new_expr(
				TypeInfo::ID expr_type_id, bool is_initialization, evo::ArrayProxy<pir::Expr> store_location
			) -> std::optional<pir::Expr>;

			auto delete_expr(const sema::Expr& expr, TypeInfo::ID expr_type_id) -> void;
			auto delete_expr(pir::Expr expr, TypeInfo::ID expr_type_id) -> void; // expr must be pointer to value


			template<GetExprMode MODE>
			auto expr_copy(
				const sema::Expr& expr,
				TypeInfo::ID expr_type_id,
				bool is_initialization,
				evo::ArrayProxy<pir::Expr> store_locations
			) -> std::optional<pir::Expr>;

			template<GetExprMode MODE>
			auto expr_copy(
				pir::Expr expr,
				TypeInfo::ID expr_type_id,
				bool is_initialization,
				evo::ArrayProxy<pir::Expr> store_locations
			) -> std::optional<pir::Expr>;



			template<GetExprMode MODE>
			auto expr_move(
				const sema::Expr& expr,
				TypeInfo::ID expr_type_id,
				bool is_initialization,
				evo::ArrayProxy<pir::Expr> store_locations
			) -> std::optional<pir::Expr>;

			template<GetExprMode MODE>
			auto expr_move(
				pir::Expr expr,
				TypeInfo::ID expr_type_id,
				bool is_initialization,
				evo::ArrayProxy<pir::Expr> store_locations
			) -> std::optional<pir::Expr>;


			template<GetExprMode MODE>
			auto expr_cmp(
				TypeInfo::ID expr_type_id,
				const sema::Expr& lhs,
				const sema::Expr& rhs,
				bool is_equal,
				evo::ArrayProxy<pir::Expr> store_locations
			) -> std::optional<pir::Expr>;

			template<GetExprMode MODE>
			auto expr_cmp(
				TypeInfo::ID expr_type_id,
				pir::Expr lhs,
				pir::Expr rhs,
				bool is_equal,
				evo::ArrayProxy<pir::Expr> store_locations
			) -> std::optional<pir::Expr>;



			EVO_NODISCARD auto calc_in_param_bitmap(
				const BaseType::Function& target_func, evo::ArrayProxy<sema::Expr> args
			) const -> uint32_t;



			auto iterate_array(
				const BaseType::Array& array_type, std::string_view op_name, std::function<void(pir::Expr)> body_func
			) -> void;


			EVO_NODISCARD auto create_call(
				evo::Variant<std::monostate, pir::Function::ID, pir::ExternalFunction::ID> func_id,
				evo::SmallVector<pir::Expr>&& args,
				std::string&& name = ""
			) -> pir::Expr;

			auto create_call_void(
				evo::Variant<std::monostate, pir::Function::ID, pir::ExternalFunction::ID> func_id,
				evo::SmallVector<pir::Expr>&& args
			) -> void;


			template<GetExprMode MODE>
			auto intrinsic_func_call_expr(
				const sema::FuncCall& func_call, evo::ArrayProxy<pir::Expr> store_locations
			) -> std::optional<pir::Expr>;

			template<GetExprMode MODE>
			auto template_intrinsic_func_call_expr(
				const sema::FuncCall& func_call, evo::ArrayProxy<pir::Expr> store_locations
			) -> std::optional<pir::Expr>;

			auto intrinsic_func_call(const sema::FuncCall& func_call) -> void;


			auto create_fatal() -> void;


			EVO_NODISCARD auto get_global_var_value(const sema::Expr expr) -> pir::GlobalVar::Value;

			template<bool MAY_LOWER_DEPENDENCY>
			EVO_NODISCARD auto get_type(TypeInfo::VoidableID voidable_type_id) -> pir::Type;
			template<bool MAY_LOWER_DEPENDENCY>
			EVO_NODISCARD auto get_type(TypeInfo::ID type_id) -> pir::Type;
			template<bool MAY_LOWER_DEPENDENCY>
			EVO_NODISCARD auto get_type(BaseType::ID base_type_id) -> pir::Type;

			EVO_NODISCARD auto mangle_name(BaseType::Struct::ID struct_id) const -> std::string;
			EVO_NODISCARD auto mangle_name(BaseType::Union::ID union_id) const -> std::string;
			EVO_NODISCARD auto mangle_name(BaseType::Interface::ID interface_id) const -> std::string;

			template<bool PIR_STMT_NAME_SAFE = false>
			EVO_NODISCARD auto mangle_name(sema::GlobalVar::ID global_var_id) const -> std::string;

			template<bool PIR_STMT_NAME_SAFE = false>
			EVO_NODISCARD auto mangle_name(sema::Func::ID func_id) const -> std::string;

			template<bool PIR_STMT_NAME_SAFE = false>
			EVO_NODISCARD auto mangle_name(sema::Func::ID func_id, Token::Kind op_kind) const -> std::string;


			EVO_NODISCARD auto get_parent_name(
				std::optional<EncapsulatingSymbolID> parent_id,
				evo::Variant<SourceID, ClangSourceID, BuiltinModuleID> source_id
			) const -> std::string;

			EVO_NODISCARD auto get_parent_name(
				std::optional<EncapsulatingSymbolID> parent_id,
				evo::Variant<SourceID, ClangSourceID> source_id
			) const -> std::string {
				if(source_id.is<SourceID>()){
					return this->get_parent_name(parent_id, source_id.as<SourceID>());
				}else{
					return this->get_parent_name(parent_id, source_id.as<ClangSourceID>());
				}
			}

			EVO_NODISCARD auto get_parent_name(
				std::optional<EncapsulatingSymbolID> parent_id,
				evo::Variant<SourceID, BuiltinModuleID> source_id
			) const -> std::string {
				if(source_id.is<SourceID>()){
					return this->get_parent_name(parent_id, source_id.as<SourceID>());
				}else{
					return this->get_parent_name(parent_id, source_id.as<BuiltinModuleID>());
				}
			}

			EVO_NODISCARD auto get_parent_name(
				std::optional<EncapsulatingSymbolID> parent_id, SourceID source_id
			) const -> std::string {
				return this->get_parent_name(
					parent_id, evo::Variant<SourceID, ClangSourceID, BuiltinModuleID>(source_id)
				);
			}

			EVO_NODISCARD auto get_parent_name(
				std::optional<EncapsulatingSymbolID> parent_id, ClangSourceID source_id
			) const -> std::string {
				return this->get_parent_name(
					parent_id, evo::Variant<SourceID, ClangSourceID, BuiltinModuleID>(source_id)
				);
			}

			EVO_NODISCARD auto get_parent_name(
				std::optional<EncapsulatingSymbolID> parent_id, BuiltinModuleID source_id
			) const -> std::string {
				return this->get_parent_name(
					parent_id, evo::Variant<SourceID, ClangSourceID, BuiltinModuleID>(source_id)
				);
			}




			// Note on naming: use a '.' prefix if the operation is an intermediate (not returned by some sema expr)
			EVO_NODISCARD auto name(std::string_view str) const -> std::string;
			template<class... Args>
			EVO_NODISCARD auto name(std::format_string<Args...> fmt, Args&&... args) const -> std::string;


			struct AutoDeleteTarget{
				pir::Expr expr;
				TypeInfo::ID typeID;
			};

			using ManagedLifetimeTarget = evo::Variant<pir::Expr, ManagedLifetimeErrorParam>;

			struct AutoDeleteManagedLifetimeTarget{
				ManagedLifetimeTarget expr;
				TypeInfo::ID typeID;
			};

			struct DeferItem{
				struct Targets{
					bool on_scope_end;
					bool on_return; // includes implicit return
					bool on_error;
					bool on_continue;
					bool on_break;
				};

				evo::Variant<
					sema::Defer::ID, std::function<void()>, AutoDeleteTarget, AutoDeleteManagedLifetimeTarget
				> defer_item;
				Targets targets;
			};

			struct ScopeLevel{
				using IsAlive = bool;

				std::string_view label; // empty if no label
				evo::SmallVector<pir::Expr> label_output_locations; // empty if none
				std::optional<pir::BasicBlock::ID> begin_block;
				std::optional<pir::BasicBlock::ID> end_block;
				bool is_loop;
				evo::SmallVector<DeferItem> defers{};
				std::unordered_map<ManagedLifetimeTarget, IsAlive> value_states;

				ScopeLevel() : label(), label_output_locations(), end_block(), is_loop(false) {}
				ScopeLevel(
					std::string_view _label,
					evo::SmallVector<pir::Expr>&& _label_output_locations,
					std::optional<pir::BasicBlock::ID> _begin_block,
					pir::BasicBlock::ID _end_block,
					bool _is_loop
				) : 
					label(_label),
					label_output_locations(std::move(_label_output_locations)),
					begin_block(_begin_block),					
					end_block(_end_block),
					is_loop(_is_loop)
				{
					evo::debugAssert(!this->is_loop || this->begin_block.has_value(), "loop must have begin block");
				}
			};

			auto push_scope_level(auto&&... scope_level_args) -> void;
			auto pop_scope_level() -> void;
			EVO_NODISCARD auto get_current_scope_level() -> ScopeLevel&;

			auto add_auto_delete_target(pir::Expr expr, TypeInfo::ID type_id) -> void;


			enum class DeferTarget{
				SCOPE_END,
				RETURN, // includes implicit return
				ERROR,
				CONTINUE,
				BREAK,
			};

			template<DeferTarget TARGET>
			auto output_defers_for_scope_level(const ScopeLevel& scope_level) -> void;

	
		private:
			class Context& context;
			pir::Module& module;
			pir::Agent agent;

			class Source* current_source = nullptr;
			const Data::FuncInfo* current_func_info = nullptr;
			const BaseType::Function* current_func_type = nullptr;

			std::unordered_map<sema::Expr, pir::Expr> local_func_exprs{};
			evo::SmallVector<ScopeLevel> scope_levels{}; // TODO(PERF): use stack?

			uint32_t in_param_bitmap = 0; // 0 bit means move, 1 bit means copy

			evo::SmallVector<AutoDeleteTarget> end_of_stmt_deletes{};

			Data& data;
	};


}
