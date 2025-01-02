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

#include "./Source.h"


namespace pcit::panther{


	class SourceManager{
		public:
			SourceManager() = default;
			~SourceManager() = default;


			EVO_NODISCARD auto createSourceCompilationConfig(Source::CompilationConfig&& src_comp_config)
			-> Source::CompilationConfig::ID {
				evo::debugAssert(src_comp_config.basePath.is_absolute(), "Base path must be absolute");
				return this->priv.source_compilation_configs.emplace_back(std::move(src_comp_config));
			}

			EVO_NODISCARD auto createSourceCompilationConfig(const Source::CompilationConfig& src_comp_config)
			-> Source::CompilationConfig::ID {
				evo::debugAssert(src_comp_config.basePath.is_absolute(), "Base path must be absolute");
				return this->priv.source_compilation_configs.emplace_back(src_comp_config);
			}

			EVO_NODISCARD auto getSourceCompilationConfig(Source::CompilationConfig::ID id) const 
			-> const Source::CompilationConfig& {
				return this->priv.source_compilation_configs[id];
			}


			EVO_NODISCARD auto operator[](Source::ID id) const -> const Source& { return this->priv.sources[id]; }
			EVO_NODISCARD auto operator[](Source::ID id)       ->       Source& { return this->priv.sources[id]; }


			EVO_NODISCARD auto begin() const -> Source::ID::Iterator {
				return Source::ID::Iterator(Source::ID(0));
			}

			EVO_NODISCARD auto end() const -> Source::ID::Iterator {
				const auto lock = std::lock_guard(this->priv.sources_lock);
				return Source::ID::Iterator(Source::ID(uint32_t(this->priv.sources.size())));
			}

			EVO_NODISCARD auto size() const -> size_t {
				const auto lock = std::lock_guard(this->priv.sources_lock);
				return this->priv.sources.size();
			}


		private:
			auto create_source(
				std::filesystem::path&& path, std::string&& data_str, Source::CompilationConfig::ID comp_config_id
			) -> Source::ID {
				const auto lock = std::lock_guard(this->priv.sources_lock);

				const Source::ID new_source_id = this->priv.sources.emplace_back(
					std::move(path), std::move(data_str), comp_config_id
				);

				this->operator[](new_source_id).id = new_source_id;

				return new_source_id;
			}

			auto add_special_name_path(std::string_view name, const std::filesystem::path& path) -> void {
				this->priv.special_name_paths.emplace(name, path);
			}


			EVO_NODISCARD auto emplace_source_compilation_config(auto&&... args) -> Source::CompilationConfig::ID {
				return this->priv.source_compilation_configs.emplace_back(std::forward<decltype(args)>(args)...);
			}
	
		private:
			// To prevent context from accessing private members while allowing access to private methods
			struct /* priv */ {
				private:
					core::LinearStepAlloc<Source, Source::ID, 0> sources{};
					mutable core::SpinLock sources_lock{};

					using CompConfig = Source::CompilationConfig;
					core::LinearStepAlloc<CompConfig, CompConfig::ID> source_compilation_configs{};
					std::unordered_map<std::string_view, std::filesystem::path> special_name_paths{};

					friend SourceManager;
			} priv;


			friend class Context;
	};

	
}