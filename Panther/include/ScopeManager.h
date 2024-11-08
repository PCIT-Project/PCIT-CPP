//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once

#include <shared_mutex>

#include <PCIT_core.h>

#include "./ASG_IDs.h"
#include "./ASG_Stmt.h"
#include "./source_data.h"
#include "./AST.h"
#include "./TypeManager.h"


namespace pcit::panther{

	
	class ScopeManager{
		public:
			class Level{ // not thread-safe
				public:
					struct ID : public core::UniqueID<uint32_t, struct ID> {
						using core::UniqueID<uint32_t, ID>::UniqueID;
					};

					struct ImportInfo{
						SourceID sourceID;
						Token::ID tokenID;
					};

					using IdentID = evo::Variant<
						evo::SmallVector<ASG::FuncID>,
						ASG::TemplatedFuncID,
						ASG::VarID,
						ASG::ParamID,
						ASG::ReturnParamID,
						ImportInfo,
						BaseType::Alias::ID
					>;

				public:
					Level(ASG::StmtBlock* stmt_block = nullptr) : _stmt_block(stmt_block) {}
					~Level() = default;

					EVO_NODISCARD auto hasStmtBlock() const -> bool;
					EVO_NODISCARD auto stmtBlock() const -> const ASG::StmtBlock&;
					EVO_NODISCARD auto stmtBlock()       ->       ASG::StmtBlock&;

					auto addSubScope() -> void;
					auto setSubScopeTerminated() -> void;
					auto setTerminated() -> void;
					EVO_NODISCARD auto isTerminated() const -> bool;
					EVO_NODISCARD auto isNotTerminated() const -> bool;

					auto addFunc(std::string_view ident, ASG::FuncID id) -> void;
					auto addTemplatedFunc(std::string_view ident, ASG::TemplatedFuncID id) -> void;
					auto addVar(std::string_view ident, ASG::VarID id) -> void;
					auto addParam(std::string_view ident, ASG::ParamID id) -> void;
					auto addReturnParam(std::string_view ident, ASG::ReturnParamID id) -> void;
					auto addImport(std::string_view ident, SourceID id, Token::ID location) -> void;
					auto addAlias(std::string_view ident, BaseType::Alias::ID id) -> void;

					auto lookupIdent(std::string_view ident) const -> const IdentID*;

				private:
					std::unordered_map<std::string_view, IdentID> ids{};

					ASG::StmtBlock* _stmt_block;

					unsigned num_sub_scopes_not_terminated = 0;
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

					auto pushLevel(Level::ID id) -> void;
					auto pushLevel(Level::ID id, ASG::FuncID func_id) -> void;
					auto popLevel() -> void;

					EVO_NODISCARD auto getCurrentLevel() const -> Level::ID { return this->scope_levels.back(); }


					EVO_NODISCARD auto size() const -> size_t { return this->scope_levels.size(); }

					// note: these are purposely reverse iterators

					EVO_NODISCARD auto begin() -> evo::SmallVector<Level::ID>::reverse_iterator {
						return this->scope_levels.rbegin();
					}

					EVO_NODISCARD auto begin() const -> evo::SmallVector<Level::ID>::const_reverse_iterator {
						return this->scope_levels.rbegin();
					}

					EVO_NODISCARD auto end() -> evo::SmallVector<Level::ID>::reverse_iterator {
						return this->scope_levels.rend();
					}

					EVO_NODISCARD auto end() const -> evo::SmallVector<Level::ID>::const_reverse_iterator {
						return this->scope_levels.rend();
					}

					
					///////////////////////////////////
					// object scope

					// pushing / popping happens automatically with `pushLevel` / `popLevel`

					EVO_NODISCARD auto inObjectScope() const -> bool { return !this->object_scopes.empty(); }
					EVO_NODISCARD auto getCurrentObjectScope() const -> const ObjectScope& {
						evo::debugAssert(this->inObjectScope(), "not in object scope");
						return this->object_scopes.back().obj_scope;
					}

					EVO_NODISCARD auto getCurrentObjectScopeIndex() const -> uint32_t {
						if(this->object_scopes.empty()){ return 0; }
						return this->object_scopes.back().scope_level_index - 1;
					}

					EVO_NODISCARD auto inObjectMainScope() const -> bool {
						evo::debugAssert(this->object_scopes.size() >= 1, "not in object scope");
						return this->object_scopes.back().scope_level_index - 1 == this->size();
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
					evo::SmallVector<Level::ID> scope_levels{};
					evo::SmallVector<ObjectScopeData> object_scopes{};
			};
		
		public:
			ScopeManager() = default;
			~ScopeManager() = default;

			EVO_NODISCARD auto operator[](Level::ID id) const -> const Level& {
				const auto lock = std::shared_lock(this->mutex);
				return this->scope_levels[id];
			}

			EVO_NODISCARD auto operator[](Level::ID id) -> Level& {
				const auto lock = std::shared_lock(this->mutex);
				return this->scope_levels[id];
			}


			EVO_NODISCARD auto createLevel(ASG::StmtBlock* stmt_block = nullptr) -> Level::ID {
				const auto lock = std::unique_lock(this->mutex);
				return this->scope_levels.emplace_back(stmt_block);
			}
	
		private:
			core::LinearStepAlloc<Level, Level::ID> scope_levels{};
			mutable core::SpinLock mutex{};
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