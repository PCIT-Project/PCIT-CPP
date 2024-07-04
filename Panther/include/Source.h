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

namespace pcit::panther{


	class Source{
		public:
			using ID = SourceID;
			using Location = SourceLocation;

		public:
			Source(Source&& rhs) noexcept : id(rhs.id), location(std::move(rhs.location)), data(std::move(rhs.data)) {};
			Source(const Source&) = delete;

			~Source() = default;

			
			EVO_NODISCARD auto getID() const noexcept -> ID { return this->id; };
			EVO_NODISCARD auto getData() const noexcept -> const std::string& { return this->data; };

			EVO_NODISCARD auto locationIsPath() const noexcept -> bool;
			EVO_NODISCARD auto locationIsString() const noexcept -> bool;

			EVO_NODISCARD auto getLocationPath() const noexcept -> const fs::path&;
			EVO_NODISCARD auto getLocationString() const noexcept -> const std::string&;
			EVO_NODISCARD auto getLocationAsString() const noexcept -> std::string;

			EVO_NODISCARD auto getTokenBuffer() const noexcept -> const TokenBuffer& { return this->token_buffer; };
			EVO_NODISCARD auto getASTBuffer() const noexcept -> const ASTBuffer& { return this->ast_buffer; };
			

		private:
			Source(ID src_id, const std::string& loc, const std::string& data_str) noexcept
				: id(src_id), location(loc), data(data_str) {};

			Source(ID src_id, const std::string& loc, std::string&& data_str) noexcept
				: id(src_id), location(loc), data(std::move(data_str)) {};

			Source(ID src_id, std::string&& loc, const std::string& data_str) noexcept
				: id(src_id), location(std::move(loc)), data(data_str) {};

			Source(ID src_id, std::string&& loc, std::string&& data_str) noexcept
				: id(src_id), location(std::move(loc)), data(std::move(data_str)) {};


			Source(ID src_id, const fs::path& loc, const std::string& data_str) noexcept
				: id(src_id), location(loc), data(data_str) {};

			Source(ID src_id, const fs::path& loc, std::string&& data_str) noexcept
				: id(src_id), location(loc), data(std::move(data_str)) {};

			Source(ID src_id, fs::path&& loc, const std::string& data_str) noexcept
				: id(src_id), location(std::move(loc)), data(data_str) {};

			Source(ID src_id, fs::path&& loc, std::string&& data_str) noexcept
				: id(src_id), location(std::move(loc)), data(std::move(data_str)) {};
	
		private:
			ID id;
			evo::Variant<fs::path, std::string> location;
			std::string data;

			TokenBuffer token_buffer{};
			ASTBuffer ast_buffer{};

			friend class SourceManager;
			friend class Context;
			friend class Parser;
	};

};