////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <filesystem>

#include <Evo.hpp>
#include <PCIT_core.hpp>

#include "./source_data.hpp"
#include "../tokens/TokenBuffer.hpp"
#include "../AST/AST.hpp"
#include "../AST/ASTBuffer.hpp"
#include "../../src/symbol_proc/SymbolProc.hpp"
#include "../../src/sema/ScopeManager.hpp"
#include "../../../PIR/include/meta.hpp"


namespace pcit::panther{


	class Source{
		public:
			using ID = SourceID;
			using Location = SourceLocation;

			struct Package{
				struct ID : public core::UniqueID<uint32_t, struct ID> {
					using core::UniqueID<uint32_t, ID>::UniqueID;
				};

				std::filesystem::path basePath;
				std::string name;

				struct Warns{
					bool methodCallOnNonMethod        = true;
					bool memberTypeByValueAccessor    = true;
					bool deleteMovedFromExpr          = true;
					bool deleteTriviallyDeletableType = true;
					bool comptimeIfCond               = true;
					bool alreadyUnsafe                = true;
					bool experimentalF80              = true;

					static auto all() -> Warns { return Warns(); };
				} warn;
			};

		public:
			[[nodiscard]] auto getID() const -> ID { return this->id; }
			[[nodiscard]] auto getPath() const -> const std::filesystem::path& { return this->path; }
			[[nodiscard]] auto getData() const -> const std::string& { return this->data; }
			[[nodiscard]] auto getPackageID() const -> Package::ID { return this->packagage_id; }

			[[nodiscard]] auto getTokenBuffer() const -> const TokenBuffer& { return this->token_buffer; }
			[[nodiscard]] auto getASTBuffer() const -> const ASTBuffer& { return this->ast_buffer; }

			[[nodiscard]] auto getPIRMetaFileID() const -> std::optional<pir::meta::File::ID> {
				return this->pir_file_id;
			}
			

			Source(const Source&) = delete;

		private:
			Source(
				std::filesystem::path&& _path, std::string&& data_str, Package::ID pgk_id
			) : id(ID(0)), path(std::move(_path)), data(std::move(data_str)), packagage_id(pgk_id) {}

	
		private:
			ID id;
			std::filesystem::path path;
			std::string data;
			Package::ID packagage_id;

			TokenBuffer token_buffer{};
			ASTBuffer ast_buffer{};

			bool is_ready_for_sema = false;

			std::optional<sema::ScopeManager::Scope::ID> sema_scope_id{};
			SymbolProc::Namespace global_symbol_procs{};

			std::optional<pir::meta::File::ID> pir_file_id;

			friend class SourceManager;
			friend class Context;
			friend core::LinearStepAlloc;
			friend class Tokenizer;
			friend class Parser;
			friend class SymbolProcBuilder;
			friend class SemanticAnalyzer;
	};

	
}