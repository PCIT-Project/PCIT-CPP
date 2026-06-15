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
#include "../TypeManager.hpp"
#include "../sema/sema.hpp"
#include "../../../PIR/include/meta.hpp"


namespace pcit::panther{


	class CFamilySource{
		public:
			using ID = CFamilySourceID;
			using Location = CFamilySourceLocation;
			using DeclInfoID = CFamilySourceDeclInfoID;
			using DeclInfo = CFamilySourceDeclInfo;

			using Symbol = evo::Variant<BaseType::ID, sema::Func::ID, sema::GlobalVar::ID>;

			struct SymbolInfo{
				Symbol symbol;
				ID sourceID;
				bool isPub;
			};


		public:
			CFamilySource(const CFamilySource&) = delete;


			///////////////////////////////////
			// info getters

			[[nodiscard]] auto getID() const -> ID { return this->id; }
			[[nodiscard]] auto getPath() const -> const std::filesystem::path& { return this->path; }
			[[nodiscard]] auto getSystemIncludeDirectories() const -> evo::ArrayProxy<std::string> {
				return this->system_include_directories;
			}
			[[nodiscard]] auto getIncludeDirectories() const -> evo::ArrayProxy<std::string> {
				return this->include_directories;
			}
			[[nodiscard]] auto getData() const -> const std::string& { return this->data; }
			[[nodiscard]] auto isCPP() const -> bool { return this->is_cpp; }
			[[nodiscard]] auto isImportedByPthr() const -> bool { return this->is_imported_by_pthr; }
			
			[[nodiscard]] auto getPIRMetaFileID() const -> std::optional<pir::meta::File::ID> {
				return this->pir_file_id;
			}


			///////////////////////////////////
			// decl info

			[[nodiscard]] auto createDeclInfo(std::string name, uint32_t line, uint32_t collumn) -> DeclInfoID {
				return this->decl_infos.emplace_back(name, line, line, collumn, collumn);
			}


			[[nodiscard]] auto getDeclInfo(DeclInfoID decl_info_id) const -> DeclInfo {
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


			///////////////////////////////////
			// imported symbols

			auto addImportedSymbol(std::string&& symbol_name, Symbol symbol, ID source_id, bool is_pub) -> void {
				evo::debugAssert(this->isSymbolImportComplete() == false, "symbol import was already completed");

				const auto symbol_name_view = std::string_view(this->names.emplace_back(std::move(symbol_name)));
				this->imported_symbols.emplace(symbol_name_view, SymbolInfo(symbol, source_id, is_pub));
			}

			auto addImportedSymbol(std::string_view symbol_name, Symbol symbol, ID source_id, bool is_pub) -> void {
				return this->addImportedSymbol(std::string(symbol_name), symbol, source_id, is_pub);
			}



			[[nodiscard]] auto getImportedSymbol(std::string_view symbol_name) const -> std::optional<SymbolInfo> {
				const auto find = this->imported_symbols.find(symbol_name);
				if(find != this->imported_symbols.end()){ return find->second; }

				return std::nullopt;
			}


			auto setSymbolImportComplete() -> void { this->symbol_import_complete = true; }
			[[nodiscard]] auto isSymbolImportComplete() const -> bool { return this->symbol_import_complete; }


			auto addInlinedFuncName(std::string_view name) -> void {
				this->inlined_func_names.emplace_back(std::string(name));
			}
			auto addInlinedFuncName(std::string&& name) -> void {
				this->inlined_func_names.emplace_back(std::move(name));
			}

			[[nodiscard]] auto getInlinedFuncNames() const 
			-> evo::IterRange<evo::StepVector<std::string>::const_iterator> {
				return evo::IterRange<evo::StepVector<std::string>::const_iterator>(
					this->inlined_func_names.begin(), this->inlined_func_names.end()
				);
			}


			///////////////////////////////////
			// source symbols

			using SymbolCreator = std::function<Symbol()>;

			[[nodiscard]] auto getOrCreateSourceSymbol(std::string&& symbol_name, const SymbolCreator& symbol_creator)
			-> Symbol {
				const auto lock = std::scoped_lock(this->source_symbols_lock);

				const auto find = this->source_symbols.find(symbol_name);
				if(find != this->source_symbols.end()){ return find->second; }

				const auto symbol_name_view = std::string_view(this->names.emplace_back(std::move(symbol_name)));
				const Symbol created_symbol = symbol_creator();
				this->source_symbols.emplace(symbol_name_view, created_symbol);
				return created_symbol;
			}

			[[nodiscard]] auto getOrCreateSourceSymbol(
				std::string_view symbol_name, const SymbolCreator& symbol_creator
			) -> Symbol {
				return this->getOrCreateSourceSymbol(std::string(symbol_name), symbol_creator);
			}


			///////////////////////////////////
			// defines

			auto addDefine(std::string&& define_name, std::optional<DeclInfoID> decl_info_id) -> void {
				const auto define_name_view = std::string_view(this->names.emplace_back(std::move(define_name)));
				this->defines.emplace(define_name_view, decl_info_id);
			}

			auto addDefine(std::string_view define_name, std::optional<DeclInfoID> decl_info_id) -> void {
				this->addDefine(std::string(define_name), decl_info_id);
			}

			[[nodiscard]] auto getDefine(std::string_view define_name) const
			-> std::optional<std::optional<DeclInfoID>> {
				const auto find = this->defines.find(define_name);
				if(find != this->defines.end()){ return find->second; }
				return std::nullopt;
			}


		private:
			CFamilySource(
				std::filesystem::path&& _path,
				evo::SmallVector<std::string>&& _system_include_directories,
				evo::SmallVector<std::string>&& _include_directories,
				std::string&& data_str,
				bool _is_cpp,
				bool _is_imported_by_pthr,
				std::optional<pir::meta::FileID> _pir_file_id
			) :
				id(ID(0)),
				path(std::move(_path)),
				system_include_directories(std::move(_system_include_directories)),
				include_directories(std::move(_include_directories)),
				data(std::move(data_str)),
				is_cpp(_is_cpp),
				is_imported_by_pthr(_is_imported_by_pthr),
				pir_file_id(_pir_file_id)
			{}


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
			evo::SmallVector<std::string> system_include_directories;
			evo::SmallVector<std::string> include_directories;
			std::string data;
			bool is_cpp;
			bool is_imported_by_pthr;
			std::optional<pir::meta::FileID> pir_file_id;

			core::SyncLinearStepAlloc<SavedDeclInfo, DeclInfoID> decl_infos{};

			evo::StepVector<std::string> names{};
			std::unordered_map<std::string_view, SymbolInfo> imported_symbols{};
			std::unordered_map<std::string_view, Symbol> source_symbols{};
			mutable evo::SpinLock source_symbols_lock{};

			std::unordered_map<std::string_view, std::optional<DeclInfoID>> defines{};

			evo::StepVector<std::string> inlined_func_names{};

			std::atomic<bool> symbol_import_complete = false;

			friend class SourceManager;
			friend core::LinearStepAlloc;
	};

	
}