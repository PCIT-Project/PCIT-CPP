//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once

#include <shared_mutex>

#include <PCIT_core.h>

#include "./ASG_IDs.h"
#include "./source_data.h"
#include "./AST.h"
#include "./TypeManager.h"


namespace pcit::panther{

	
	class ScopeManager{
		public:
			class ScopeLevel{
				public:
					struct ID : public core::UniqueID<uint32_t, struct ID> {
						using core::UniqueID<uint32_t, ID>::UniqueID;
					};

					// using TemplateExpr = evo::Variant<uint64_t, float64_t, bool, char>;

				public:
					ScopeLevel() = default;
					~ScopeLevel() = default;

					auto addSubScope() -> void;
					auto setSubScopeTerminated() -> void;
					auto setTerminated() -> void;
					EVO_NODISCARD auto isTerminated() const -> bool;
					EVO_NODISCARD auto isNotTerminated() const -> bool;

					EVO_NODISCARD auto lookupFunc(std::string_view ident) const -> std::optional<ASG::FuncID>;
					auto addFunc(std::string_view ident, ASG::FuncID id) -> void;

					EVO_NODISCARD auto lookupTemplatedFunc(std::string_view ident) const 
						-> std::optional<ASG::TemplatedFuncID>;
					auto addTemplatedFunc(std::string_view ident, ASG::TemplatedFuncID id) -> void;

					EVO_NODISCARD auto lookupVar(std::string_view ident) const -> std::optional<ASG::VarID>;
					auto addVar(std::string_view ident, ASG::VarID id) -> void;

					EVO_NODISCARD auto lookupImport(std::string_view ident) const -> std::optional<SourceID>;
					EVO_NODISCARD auto getImportLocation(std::string_view ident) const -> Token::ID;
					auto addImport(std::string_view ident, SourceID id, Token::ID location) -> void;

				private:
					std::unordered_map<std::string_view, ASG::FuncID> funcs{};
					std::unordered_map<std::string_view, ASG::TemplatedFuncID> templated_funcs{};
					std::unordered_map<std::string_view, ASG::VarID> vars{};
					std::unordered_map<std::string_view, SourceID> imports{};
					std::unordered_map<std::string_view, Token::ID> import_locations{};
					evo::uint num_sub_scopes_not_terminated = 0;
					bool is_terminated = false;
					bool has_sub_scopes = false;
			};


			class Scope{ // not thread-safe
				public:
					using ObjectScope = evo::Variant<std::monostate, ASG::FuncID>;

				public:
					Scope() = default;
					~Scope() = default;

					///////////////////////////////////
					// scope level

					auto pushScopeLevel(ScopeLevel::ID id) -> void;
					auto pushScopeLevel(ScopeLevel::ID id, ASG::FuncID func_id) -> void;
					auto popScopeLevel() -> void;

					EVO_NODISCARD auto getCurrentScopeLevel() const -> ScopeLevel::ID {
						return this->scope_levels.back();
					}

					// note: these are purposely backwards

					EVO_NODISCARD auto begin() -> evo::SmallVector<ScopeLevel::ID>::reverse_iterator {
						return this->scope_levels.rbegin();
					}

					EVO_NODISCARD auto begin() const -> evo::SmallVector<ScopeLevel::ID>::const_reverse_iterator {
						return this->scope_levels.rbegin();
					}

					EVO_NODISCARD auto end() -> evo::SmallVector<ScopeLevel::ID>::reverse_iterator {
						return this->scope_levels.rend();
					}

					EVO_NODISCARD auto end() const -> evo::SmallVector<ScopeLevel::ID>::const_reverse_iterator {
						return this->scope_levels.rend();
					}

					EVO_NODISCARD auto size() const -> size_t {
						return this->scope_levels.size();
					}


					///////////////////////////////////
					// object scope

					EVO_NODISCARD auto inObjectScope() const -> bool { return !this->object_scopes.empty(); }
					EVO_NODISCARD auto getCurrentObjectScope() const -> const ObjectScope& {
						evo::debugAssert(this->inObjectScope(), "not in object scope");
						return this->object_scopes.back().obj_scope;
					}

					EVO_NODISCARD auto getCurrentObjectScopeIndex() const -> uint32_t {
						if(this->object_scopes.empty()){ return 0; }
						return this->object_scopes.back().scope_level_index - 1;
					}

					// must be popped manually
					// be careful - only use when declaring things like params
					EVO_NODISCARD auto pushFakeObjectScope() -> void {
						this->object_scopes.emplace_back(std::monostate(), uint32_t(this->scope_levels.size()) + 1);
					}

					EVO_NODISCARD auto popFakeObjectScope() -> void {
						evo::debugAssert(
							this->getCurrentObjectScope().is<std::monostate>(), "not in a fake object scope"
						);
						this->object_scopes.pop_back();
					}

				private:
					struct ObjectScopeData{
						ObjectScope obj_scope;
						uint32_t scope_level_index;
					};

					// TODO: use a stack?
					evo::SmallVector<ScopeLevel::ID> scope_levels{};
					evo::SmallVector<ObjectScopeData> object_scopes{};
			};
		
		public:
			ScopeManager() = default;
			~ScopeManager();

			EVO_NODISCARD auto operator[](ScopeLevel::ID id) const -> const ScopeLevel& {
				const auto lock = std::shared_lock(this->mutex);
				return *this->scope_levels[id.get()];
			}

			EVO_NODISCARD auto operator[](ScopeLevel::ID id) -> ScopeLevel& {
				const auto lock = std::shared_lock(this->mutex);
				return *this->scope_levels[id.get()];
			}


			EVO_NODISCARD auto createScopeLevel() -> ScopeLevel::ID;
	
		private:
			std::vector<ScopeLevel*> scope_levels{};
			mutable std::shared_mutex mutex{};
	};

	// just to help SemanticAnalyzer keep track of the global declarations
	class GlobalScope{
		public:
			struct Func{
				const AST::FuncDecl& ast_func;
				ASG::FuncID asg_func;
			};

			struct Var{
				const AST::VarDecl& ast_var;
				ASG::VarID asg_var;
			};

		public:
			GlobalScope() = default;
			~GlobalScope() = default;

			auto addFunc(const AST::FuncDecl& ast_func, ASG::FuncID asg_func) -> void;
			EVO_NODISCARD auto getFuncs() const -> evo::ArrayProxy<Func>;

			auto addVar(const AST::VarDecl& ast_var, ASG::VarID asg_var) -> void;
			EVO_NODISCARD auto getVars() const -> evo::ArrayProxy<Var>;
	
		private:
			evo::SmallVector<Func> funcs{};
			evo::SmallVector<Var> vars{};
	};
	
	
}