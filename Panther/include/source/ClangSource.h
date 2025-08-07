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
#include "../TypeManager.h"
#include "../sema/sema.h"


namespace pcit::panther{


	class ClangSource{
		public:
			using ID = ClangSourceID;
			using Location = ClangSourceLocation;
			using DeclInfoID = ClangSourceDeclInfoID;
			using DeclInfo = ClangSourceDeclInfo;

			using Symbol = evo::Variant<BaseType::ID>;

			struct SymbolInfo{
				Symbol symbol;
				ID sourceID;
			};


		public:
			EVO_NODISCARD auto getID() const -> ID { return this->id; }
			EVO_NODISCARD auto getPath() const -> const std::filesystem::path& { return this->path; }
			EVO_NODISCARD auto getData() const -> const std::string& { return this->data; }
			EVO_NODISCARD auto isCPP() const -> bool { return this->is_cpp; }


			EVO_NODISCARD auto createDeclInfo(std::string name, uint32_t line, uint32_t collumn) -> DeclInfoID {
				return this->decl_infos.emplace_back(name, line, line, collumn, collumn);
			}


			EVO_NODISCARD auto getDeclInfo(DeclInfoID decl_info_id) const -> DeclInfo {
				const SavedDeclInfo& saved_decl_info = this->decl_infos[decl_info_id];

				return DeclInfo(
					Location(
						this->id,
						saved_decl_info.line_start,
						saved_decl_info.line_end,
						saved_decl_info.collumn_end,
						saved_decl_info.collumn_end
					),
					std::string_view(saved_decl_info.name)
				);
			}



			auto addSymbol(std::string_view symbol_name, Symbol&& symbol, ID source_id) -> void {
				const auto lock = std::scoped_lock(this->symbol_map_lock);
				const auto symbol_name_view = std::string_view(this->symbol_names.emplace_back(symbol_name));
				this->symbol_map.emplace(symbol_name_view, SymbolInfo(std::move(symbol), source_id));
			}

			auto addSymbol(std::string_view symbol_name, const Symbol& symbol, ID source_id) -> void {
				const auto lock = std::scoped_lock(this->symbol_map_lock);
				const auto symbol_name_view = std::string_view(this->symbol_names.emplace_back(symbol_name));
				this->symbol_map.emplace(symbol_name_view, SymbolInfo(symbol, source_id));
			}


			auto addSymbol(std::string&& symbol_name, Symbol&& symbol, ID source_id) -> void {
				const auto lock = std::scoped_lock(this->symbol_map_lock);
				const auto symbol_name_view = std::string_view(this->symbol_names.emplace_back(std::move(symbol_name)));
				this->symbol_map.emplace(symbol_name_view, SymbolInfo(std::move(symbol), source_id));
			}

			auto addSymbol(std::string&& symbol_name, const Symbol& symbol, ID source_id) -> void {
				const auto lock = std::scoped_lock(this->symbol_map_lock);
				const auto symbol_name_view = std::string_view(this->symbol_names.emplace_back(std::move(symbol_name)));
				this->symbol_map.emplace(symbol_name_view, SymbolInfo(symbol, source_id));
			}




			EVO_NODISCARD auto getSymbol(std::string_view symbol_name) const -> std::optional<SymbolInfo> {
				const auto lock = std::scoped_lock(this->symbol_map_lock);

				const auto find = this->symbol_map.find(symbol_name);
				if(find != this->symbol_map.end()){ return find->second; }

				return std::nullopt;
			}



			auto setSymbolMapComplete() -> void { this->symboL_map_complete = true; }
			EVO_NODISCARD auto isSymboLMapComplete() const -> bool { return this->symboL_map_complete; }


			ClangSource(const ClangSource&) = delete;

		private:
			ClangSource(std::filesystem::path&& _path, std::string&& data_str, bool _is_cpp)
				: id(ID(0)), path(std::move(_path)), data(std::move(data_str)), is_cpp(_is_cpp) {}


			struct SavedDeclInfo{
				std::string name;
				uint32_t line_start;
				uint32_t line_end;
				uint32_t collumn_start;
				uint32_t collumn_end;
			};


		private:
			ID id;
			std::filesystem::path path;
			std::string data;
			bool is_cpp;

			core::SyncLinearStepAlloc<SavedDeclInfo, DeclInfoID> decl_infos{};

			evo::StepVector<std::string> symbol_names{};
			std::unordered_map<std::string_view, SymbolInfo> symbol_map{};
			mutable core::SpinLock symbol_map_lock{};

			std::atomic<bool> symboL_map_complete = false;

			friend class SourceManager;
			friend core::LinearStepAlloc;
	};

	
}