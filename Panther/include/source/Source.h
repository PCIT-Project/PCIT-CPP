////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <filesystem>

#include <Evo.h>
#include <PCIT_core.h>

#include "./source_data.h"
#include "../tokens/TokenBuffer.h"
#include "../AST/AST.h"
#include "../AST/ASTBuffer.h"
#include "../deps/DepsBuffer.h"
#include "../sema/SemaBuffer.h"


namespace pcit::panther{


	class Source{
		public:
			using ID = SourceID;
			using Location = SourceLocation;

			struct CompilationConfig{
				struct ID : public core::UniqueID<uint32_t, struct ID> {
					using core::UniqueID<uint32_t, ID>::UniqueID;
				};

				std::filesystem::path basePath;
			};

		public:
			EVO_NODISCARD auto getID() const -> ID { return this->id; }
			EVO_NODISCARD auto getPath() const -> const std::filesystem::path& { return this->path; }
			EVO_NODISCARD auto getData() const -> const std::string& { return this->data; }
			EVO_NODISCARD auto getCompilationConfigID() const -> CompilationConfig::ID {
				return this->compilation_config_id;
			}


			EVO_NODISCARD auto getTokenBuffer() const -> const TokenBuffer& { return this->token_buffer; }
			EVO_NODISCARD auto getASTBuffer() const -> const ASTBuffer& { return this->ast_buffer; }
			

			Source(const Source&) = delete;

		private:
			Source(
				std::filesystem::path&& _path, std::string&& data_str, CompilationConfig::ID comp_config_id
			) : id(ID(0)), path(std::move(_path)), data(std::move(data_str)), compilation_config_id(comp_config_id) {}
	
		private:
			ID id;
			std::filesystem::path path;
			std::string data;
			CompilationConfig::ID compilation_config_id;

			TokenBuffer token_buffer{};
			ASTBuffer ast_buffer{};

			std::optional<DepsBuffer::Scope::ID> deps_scope_id{};
			evo::SmallVector<deps::Node::ID> deps_node_ids{}; // TODO: needed here? Or move it to DependencyAnalysis?

			std::optional<SemaBuffer::Scope::ID> sema_scope_id{};


			// TODO: make atomic?
			struct GlobalDeclIdentInfo{
				std::optional<Token::ID> declared_location; // nullopt if declared inside a when conditonal 
				                                            //   and didn't analyze semantic declaration yet
				bool is_func;

				std::optional<Token::ID> first_sub_scope_location{};
				core::SpinLock lock{};
			};

			core::LinearStepAlloc<GlobalDeclIdentInfo, uint32_t> global_decl_ident_infos{};
			std::unordered_map<std::string_view, uint32_t> global_decl_ident_infos_map{};


			using SemaID = evo::Variant<
			sema::Var::ID, sema::Func::ID, BaseType::Typedef::ID, BaseType::Alias::ID, sema::Struct::ID
			>;
			std::unordered_map<AST::Node, SemaID> symbol_map{};
			mutable core::SpinLock symbol_map_lock{};


			friend class SourceManager;
			friend class Context;
			friend core::LinearStepAlloc;
			friend class Tokenizer;
			friend class Parser;
			friend class DependencyAnalysis;
			friend class SemanticAnalyzer;
	};

	
}