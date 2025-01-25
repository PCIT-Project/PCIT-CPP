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

#include "../../include/Context.h"
#include "../../include/deps/deps.h"
#include "../../include/deps/DepsBuffer.h"

namespace pcit::panther{


	class DependencyAnalysis{
		public:
			DependencyAnalysis(Context& _context, Source::ID source_id) : 
				context(_context), source(this->context.getSourceManager()[source_id]) {}
			~DependencyAnalysis() = default;

			EVO_NODISCARD auto analyze() -> bool;

		private:
			template<bool IS_GLOBAL>
			EVO_NODISCARD auto analyze_symbol_decl_ident(
				const AST::Node& node,
				std::optional<deps::Node::ID> parent,
				evo::SmallVector<deps::Node::ID>* when_cond_parent_deps_list
			) -> bool;

			template<bool IS_GLOBAL>
			EVO_NODISCARD auto analyze_symbol_when_cond(
				const AST::Node& node,
				std::optional<deps::Node::ID> parent,
				evo::SmallVector<deps::Node::ID>* when_cond_parent_deps_list
			) -> bool;

			EVO_NODISCARD auto analyze_symbol_deps(deps::Node::ID deps_node_id) -> bool;

			using DepsSet = std::unordered_set<deps::Node::ID, deps::Node::IDHash>; // dependencies

			struct DepsInfo{
				DepsSet decls;
				DepsSet defs;
				deps::Node::ValueStage value_stage;

				DepsInfo() : decls(), defs(), value_stage(deps::Node::ValueStage::Comptime) {}

				DepsInfo(const DepsSet& _decls, const DepsSet& _defs, deps::Node::ValueStage vs)
					: decls(_decls), defs(_defs), value_stage(vs) {}

				DepsInfo(DepsSet&& _decls, const DepsSet& _defs, deps::Node::ValueStage vs)
					: decls(std::move(_decls)), defs(_defs), value_stage(vs) {}

				DepsInfo(const DepsSet& _decls, DepsSet&& _defs, deps::Node::ValueStage vs)
					: decls(_decls), defs(std::move(_defs)), value_stage(vs) {}

				DepsInfo(DepsSet&& _decls, DepsSet&& _defs, deps::Node::ValueStage vs)
					: decls(std::move(_decls)), defs(std::move(_defs)), value_stage(vs) {}


				auto mightBeComptime() const -> bool {
					return this->value_stage == deps::Node::ValueStage::Comptime 
						|| this->value_stage == deps::Node::ValueStage::Unknown;
				}

				auto add_to_decls(const DepsSet& deps) -> void {
					this->decls.reserve(this->decls.size() + deps.size());
					for(const deps::Node::ID& dep : deps){
						this->decls.emplace(dep);
					}
				}

				auto add_to_defs(const DepsSet& deps) -> void {
					this->defs.reserve(this->defs.size() + deps.size());
					for(const deps::Node::ID& dep : deps){
						this->defs.emplace(dep);
					}
				}


				auto merge_and_export() -> DepsSet&& {
					this->add_to_decls(this->defs);
					return std::move(this->decls);
				}

				auto operator+=(const DepsInfo& rhs) -> DepsInfo&;
			};


			EVO_NODISCARD auto analyze_stmt(const AST::Node& stmt) -> evo::Result<DepsInfo>;

			EVO_NODISCARD auto analyze_type_deps(const AST::Type& ast_type) -> evo::Result<DepsSet>;

			EVO_NODISCARD auto analyze_expr(const AST::Node& node) -> evo::Result<DepsInfo>;
			EVO_NODISCARD auto analyze_attributes_deps(const AST::AttributeBlock& attribute_block)
				-> evo::Result<DepsSet>;
			EVO_NODISCARD auto analyze_ident_deps(const Token::ID& token_id) -> DepsInfo;

			EVO_NODISCARD auto create_node(
				const AST::Node& ast_node, deps::Node::ValueStage value_stage, deps::Node::ExtraData extra_data
			) -> deps::Node::ID;

			EVO_NODISCARD auto create_node(const AST::Node& ast_node, deps::Node::ValueStage value_stage) 
			-> deps::Node::ID {
				return this->create_node(ast_node, value_stage, deps::Node::NoExtraData());
			}

			auto get_scope() -> DepsBuffer::Scope&;
			auto get_current_scope_level() -> DepsBuffer::ScopeLevel&;
			auto enter_scope() -> void;
			auto enter_scope(const auto& object_scope) -> void;
			auto leave_scope() -> void;

			auto add_symbol(std::string_view ident, deps::Node::ID deps_node_id) -> void;

			// returns false if it already existed
			auto add_global_symbol(std::string_view ident, Token::ID location, bool is_func, bool is_in_when_cond)
				-> bool;

			auto add_deps(DepsSet& target, const DepsSet& deps) const -> bool;
			auto add_deps(DepsSet& target, const deps::Node::ID& dep) const -> bool;

			auto emit_fatal(Diagnostic::Code code, Diagnostic::Location::None, auto&&... args) -> void {
				this->context.emitError(code, Diagnostic::Location::NONE, std::forward<decltype(args)>(args)...);
				this->errored = true;	
			}

			auto emit_fatal(Diagnostic::Code code, auto location, auto&&... args) -> void {
				this->context.emitError(
					code, Diagnostic::Location::get(location, this->source), std::forward<decltype(args)>(args)...
				);
				this->errored = true;	
			}


			auto emit_error(Diagnostic::Code code, Diagnostic::Location::None, auto&&... args) -> void {
				this->context.emitError(code, Diagnostic::Location::NONE, std::forward<decltype(args)>(args)...);
				this->errored = true;	
			}

			auto emit_error(Diagnostic::Code code, auto location, auto&&... args) -> void {
				this->context.emitError(
					code, Diagnostic::Location::get(location, this->source), std::forward<decltype(args)>(args)...
				);
				this->errored = true;	
			}

		private:
			Context& context;
			Source& source;

			std::stack<DepsBuffer::Scope::ID> scopes{};

			bool errored = false;
	};


	EVO_NODISCARD inline auto analyzeDependencies(Context& context, Source::ID source_id) -> bool {
		auto dependency_analyzer = DependencyAnalysis(context, source_id);
		return dependency_analyzer.analyze();
	}


}
