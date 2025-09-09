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

#include "./Source.h"
#include "./ClangSource.h"
#include "./BuiltinModule.h"


namespace pcit::panther{


	class SourceManager{
		public:
			SourceManager() = default;
			~SourceManager() = default;


			EVO_NODISCARD auto createSourceProjectConfig(Source::ProjectConfig&& src_comp_config)
			-> Source::ProjectConfig::ID {
				evo::debugAssert(src_comp_config.basePath.is_absolute(), "Base path must be absolute");
				return this->priv.source_project_configs.emplace_back(std::move(src_comp_config));
			}

			EVO_NODISCARD auto createSourceProjectConfig(const Source::ProjectConfig& src_comp_config)
			-> Source::ProjectConfig::ID {
				evo::debugAssert(src_comp_config.basePath.is_absolute(), "Base path must be absolute");
				return this->priv.source_project_configs.emplace_back(src_comp_config);
			}

			EVO_NODISCARD auto getSourceProjectConfig(Source::ProjectConfig::ID id) const 
			-> const Source::ProjectConfig& {
				return this->priv.source_project_configs[id];
			}


			EVO_NODISCARD auto operator[](Source::ID id) const -> const Source& { return this->priv.sources[id]; }
			EVO_NODISCARD auto operator[](Source::ID id)       ->       Source& { return this->priv.sources[id]; }

			EVO_NODISCARD auto operator[](ClangSource::ID id) const -> const ClangSource& {
				return this->priv.clang_sources[id];
			}
			EVO_NODISCARD auto operator[](ClangSource::ID id) -> ClangSource& {
				return this->priv.clang_sources[id];
			}

			EVO_NODISCARD auto operator[](BuiltinModule::ID id) const -> const BuiltinModule& {
				return this->priv.builtin_modules[size_t(evo::to_underlying(id))];
			}
			EVO_NODISCARD auto operator[](BuiltinModule::ID id) -> BuiltinModule& {
				return this->priv.builtin_modules[size_t(evo::to_underlying(id))];
			}



			EVO_NODISCARD auto getSourceIDRange() const -> evo::IterRange<Source::ID::Iterator> {
				const auto lock = std::lock_guard(this->priv.sources_lock);
				return evo::IterRange<Source::ID::Iterator>(
					Source::ID::Iterator(Source::ID(0)),
					Source::ID::Iterator(Source::ID(uint32_t(this->priv.sources.size())))
				);
			}


			EVO_NODISCARD auto size() const -> size_t {
				const auto lock = std::lock_guard(this->priv.sources_lock);
				return this->priv.sources.size();
			}


			EVO_NODISCARD auto lookupSpecialNameSourceID(std::string_view path) const -> std::optional<Source::ID>;
			EVO_NODISCARD auto lookupSourceID(std::string_view path) const -> std::optional<Source::ID>;
			EVO_NODISCARD auto lookupClangSourceID(std::string_view path) const -> std::optional<ClangSource::ID>;

			struct GottenClangSourceID{
				ClangSource::ID id;
				bool created;
			};
			EVO_NODISCARD auto getOrCreateClangSourceID(std::filesystem::path&& path, bool is_cpp, bool is_header)
				-> GottenClangSourceID;
			EVO_NODISCARD auto getOrCreateClangSourceID(std::string_view path, bool is_cpp, bool is_header)
			-> GottenClangSourceID {
				return this->getOrCreateClangSourceID(std::filesystem::path(path), is_cpp, is_header);
			}



			EVO_NODISCARD auto getClangSourceIDRange() const -> evo::IterRange<ClangSource::ID::Iterator> {
				const auto lock = std::lock_guard(this->priv.clang_sources_lock);
				return evo::IterRange<ClangSource::ID::Iterator>(
					ClangSource::ID::Iterator(ClangSource::ID(0)),
					ClangSource::ID::Iterator(ClangSource::ID(uint32_t(this->priv.clang_sources.size())))
				);
			}

			

		private:
			auto create_source(
				std::filesystem::path&& path, std::string&& data_str, Source::ProjectConfig::ID comp_config_id
			) -> Source::ID {
				const auto lock = std::lock_guard(this->priv.sources_lock);

				const Source::ID new_source_id = this->priv.sources.emplace_back(
					std::move(path), std::move(data_str), comp_config_id
				);

				this->operator[](new_source_id).id = new_source_id;

				return new_source_id;
			}

			auto create_clang_source(std::filesystem::path&& path, std::string&& data_str, bool is_cpp, bool is_header)
			-> ClangSource::ID {
				const auto lock = std::lock_guard(this->priv.clang_sources_lock);

				const ClangSource::ID new_source_id = this->priv.clang_sources.emplace_back(
					std::move(path), std::move(data_str), is_cpp, is_header
				);

				this->operator[](new_source_id).id = new_source_id;

				return new_source_id;
			}

			auto add_special_name_path(std::string_view name, const std::filesystem::path& path) -> void {
				this->priv.special_name_paths.emplace(name, path);
			}


			EVO_NODISCARD auto emplace_source_project_config(auto&&... args) -> Source::ProjectConfig::ID {
				return this->priv.source_project_configs.emplace_back(std::forward<decltype(args)>(args)...);
			}
	
		private:
			// To prevent context from accessing private members while allowing access to private methods
			struct /* priv */ {
				private:
					core::LinearStepAlloc<Source, Source::ID, 0> sources{};
					mutable core::SpinLock sources_lock{};

					core::LinearStepAlloc<ClangSource, ClangSource::ID, 0> clang_sources{};
					mutable core::SpinLock clang_sources_lock{};

					std::array<BuiltinModule, 2> builtin_modules{};

					using CompConfig = Source::ProjectConfig;
					core::LinearStepAlloc<CompConfig, CompConfig::ID> source_project_configs{};

					std::unordered_map<std::string_view, std::filesystem::path> special_name_paths{};

					friend SourceManager;
			} priv;


			friend class Context;
	};

	
}