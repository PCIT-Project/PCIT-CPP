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
#include "../../include/DG/DG.h"
#include "../../include/DG/DGBuffer.h"

namespace pcit::panther{


	class DependencyAnalysis{
		public:
			DependencyAnalysis(Context& _context, Source::ID source_id) : 
				context(_context), source(this->context.getSourceManager()[source_id]) {}
			~DependencyAnalysis() = default;

			EVO_NODISCARD auto analyze() -> bool;

		private:
			EVO_NODISCARD auto analyze_global_decl_ident(
				const AST::Node& node,
				std::optional<DG::Node::ID> parent,
				evo::SmallVector<DG::Node::ID>* when_cond_parent_deps_list
			) -> bool;
			EVO_NODISCARD auto analyze_global_when_cond(
				const AST::Node& node,
				std::optional<DG::Node::ID> parent,
				evo::SmallVector<DG::Node::ID>* when_cond_parent_deps_list
			) -> bool;

			EVO_NODISCARD auto analyze_symbol_deps(DG::Node::ID dg_node_id) -> bool;

			using Deps = std::unordered_set<DG::Node::ID, DG::Node::IDHash>; // dependencies

			struct DepsInfo{
				Deps decls;
				Deps defs;
				DG::Node::UsageKind usage_kind;

				DepsInfo() : decls(), defs(), usage_kind(DG::Node::UsageKind::Comptime) {}

				DepsInfo(const Deps& _decls, const Deps& _defs, DG::Node::UsageKind uk)
					: decls(_decls), defs(_defs), usage_kind(uk) {}

				DepsInfo(Deps&& _decls, const Deps& _defs, DG::Node::UsageKind uk)
					: decls(std::move(_decls)), defs(_defs), usage_kind(uk) {}

				DepsInfo(const Deps& _decls, Deps&& _defs, DG::Node::UsageKind uk)
					: decls(_decls), defs(std::move(_defs)), usage_kind(uk) {}

				DepsInfo(Deps&& _decls, Deps&& _defs, DG::Node::UsageKind uk)
					: decls(std::move(_decls)), defs(std::move(_defs)), usage_kind(uk) {}


				auto mightBeComptime() const -> bool {
					return this->usage_kind == DG::Node::UsageKind::Comptime 
						|| this->usage_kind == DG::Node::UsageKind::Unknown;
				}

				auto add_to_decls(const Deps& deps) -> void {
					this->decls.reserve(this->decls.size() + deps.size());
					for(const DG::Node::ID& dep : deps){
						this->decls.emplace(dep);
					}
				}

				auto add_to_defs(const Deps& deps) -> void {
					this->defs.reserve(this->defs.size() + deps.size());
					for(const DG::Node::ID& dep : deps){
						this->defs.emplace(dep);
					}
				}


				auto merge_and_export() -> Deps&& {
					this->add_to_decls(this->defs);
					return std::move(this->decls);
				}

				auto operator+=(const DepsInfo& rhs) -> DepsInfo&;
			};


			EVO_NODISCARD auto analyze_stmt(const AST::Node& stmt) -> evo::Result<DepsInfo>;

			EVO_NODISCARD auto analyze_type_deps(const AST::Type& ast_type) -> evo::Result<Deps>;

			EVO_NODISCARD auto analyze_expr(const AST::Node& node) -> evo::Result<DepsInfo>;
			EVO_NODISCARD auto analyze_attributes_deps(const AST::AttributeBlock& attribute_block) -> evo::Result<Deps>;
			EVO_NODISCARD auto analyze_ident_deps(const Token::ID& token_id) -> DepsInfo;

			EVO_NODISCARD auto create_node(
				const AST::Node& ast_node, DG::Node::UsageKind usage_kind, DG::Node::ExtraData extra_data
			) -> DG::Node::ID;

			EVO_NODISCARD auto create_node(const AST::Node& ast_node, DG::Node::UsageKind usage_kind) -> DG::Node::ID {
				return this->create_node(ast_node, usage_kind, DG::Node::NoExtraData());
			}

			auto get_scope() -> DGBuffer::Scope&;
			auto get_current_scope_level() -> DGBuffer::ScopeLevel&;
			auto enter_scope() -> void;
			auto enter_scope(const auto& object_scope) -> void;
			auto leave_scope() -> void;

			auto add_symbol(std::string_view ident, DG::Node::ID dg_node_id) -> void;

			auto add_deps(Deps& target, const Deps& deps) const -> bool;
			auto add_deps(Deps& target, const DG::Node::ID& dep) const -> bool;

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

			std::stack<DGBuffer::Scope::ID> scopes{};

			bool errored = false;
	};


	EVO_NODISCARD inline auto analyzeDependencies(Context& context, Source::ID source_id) -> bool {
		auto dependency_analyzer = DependencyAnalysis(context, source_id);
		return dependency_analyzer.analyze();
	}


}
