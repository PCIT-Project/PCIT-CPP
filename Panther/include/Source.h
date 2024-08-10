//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>
#include <PCIT_core.h>

#include <filesystem>
namespace fs = std::filesystem;

#include "./source_data.h"
#include "./TokenBuffer.h"
#include "./ASTBuffer.h"
#include "./ASGBuffer.h"
#include "../src/sema/ScopeManager.h"


namespace pcit::panther{

	namespace sema{
		class SemanticAnalyzer;
	}


	class Source{
		public:
			using ID = SourceID;
			using Location = SourceLocation;

		public:
			Source(Source&& rhs) : id(rhs.id), location(std::move(rhs.location)), data(std::move(rhs.data)) {}

			Source(const Source&) = delete;
			auto operator=(const Source&) = delete;

			~Source() = default;

			
			EVO_NODISCARD auto getID() const -> ID { return this->id; }
			EVO_NODISCARD auto getData() const -> const std::string& { return this->data; }

			EVO_NODISCARD auto locationIsPath() const -> bool;
			EVO_NODISCARD auto locationIsString() const -> bool;

			EVO_NODISCARD auto getLocationPath() const -> const fs::path&;
			EVO_NODISCARD auto getLocationString() const -> const std::string&;
			EVO_NODISCARD auto getLocationAsString() const -> std::string;

			EVO_NODISCARD auto getTokenBuffer() const -> const TokenBuffer& { return this->token_buffer; }
			EVO_NODISCARD auto getASTBuffer() const -> const ASTBuffer& { return this->ast_buffer; }
			EVO_NODISCARD auto getASGBuffer() const -> const ASGBuffer& { return this->asg_buffer; }


		private:
			Source(ID src_id, const std::string& loc, const std::string& data_str)
				: id(src_id), location(loc), data(data_str) {}

			Source(ID src_id, const std::string& loc, std::string&& data_str)
				: id(src_id), location(loc), data(std::move(data_str)) {}

			Source(ID src_id, std::string&& loc, const std::string& data_str)
				: id(src_id), location(std::move(loc)), data(data_str) {}

			Source(ID src_id, std::string&& loc, std::string&& data_str)
				: id(src_id), location(std::move(loc)), data(std::move(data_str)) {}


			Source(ID src_id, const fs::path& loc, const std::string& data_str)
				: id(src_id), location(loc), data(data_str) {}

			Source(ID src_id, const fs::path& loc, std::string&& data_str)
				: id(src_id), location(loc), data(std::move(data_str)) {}

			Source(ID src_id, fs::path&& loc, const std::string& data_str)
				: id(src_id), location(std::move(loc)), data(data_str) {}

			Source(ID src_id, fs::path&& loc, std::string&& data_str)
				: id(src_id), location(std::move(loc)), data(std::move(data_str)) {}
	
		private:
			ID id;
			evo::Variant<fs::path, std::string> location;
			std::string data;

			TokenBuffer token_buffer{};
			ASTBuffer ast_buffer{};
			ASGBuffer asg_buffer{};

			sema::ScopeManager::ScopeLevel::ID global_scope_level = sema::ScopeManager::ScopeLevel::ID::dummy();
			sema::GlobalScope global_scope{};

			friend class SourceManager;
			friend class Context;
			friend class Tokenizer;
			friend class Parser;
			friend sema::SemanticAnalyzer;
	};

}