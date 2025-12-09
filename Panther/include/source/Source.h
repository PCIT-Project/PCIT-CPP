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
#include "../../src/symbol_proc/SymbolProc.h"
#include "../../src/sema/ScopeManager.h"


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
					bool deleteMovedFromExpr          = true;
					bool deleteTriviallyDeletableType = true;
					bool constexprIfCond              = true;

					static auto all() -> Warns { return Warns(); };
				} warn;
			};

		public:
			EVO_NODISCARD auto getID() const -> ID { return this->id; }
			EVO_NODISCARD auto getPath() const -> const std::filesystem::path& { return this->path; }
			EVO_NODISCARD auto getData() const -> const std::string& { return this->data; }
			EVO_NODISCARD auto getPackageID() const -> Package::ID { return this->packagage_id; }


			EVO_NODISCARD auto getTokenBuffer() const -> const TokenBuffer& { return this->token_buffer; }
			EVO_NODISCARD auto getASTBuffer() const -> const ASTBuffer& { return this->ast_buffer; }
			

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

			friend class SourceManager;
			friend class Context;
			friend core::LinearStepAlloc;
			friend class Tokenizer;
			friend class Parser;
			friend class SymbolProcBuilder;
			friend class SemanticAnalyzer;
	};

	
}