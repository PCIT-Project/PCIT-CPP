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

#include "./Source.hpp"
#include "./CFamilySource.hpp"
#include "./BuiltinModule.hpp"



namespace pcit::pir{
	class Module;
}


namespace pcit::panther{


	class SourceManager{
		public:
			enum class CreatePackageFailReason{
				PATH_NOT_ABSOLUTE,
				PATH_DOESNT_EXIST,
				PATH_NOT_DIRECTORY,
				INVALID_NAME,
				INVALID_OPTION_NAME,
			};

		public:
			SourceManager() = default;
			~SourceManager() = default;


			[[nodiscard]] auto createPackage(Source::Package&& package)
			-> evo::Expected<Source::Package::ID, CreatePackageFailReason> {
				if(package.basePath.is_absolute() == false){
					return evo::Unexpected(CreatePackageFailReason::PATH_NOT_ABSOLUTE);
				}

				if(evo::fs::exists(package.basePath.string()) == false){
					return evo::Unexpected(CreatePackageFailReason::PATH_DOESNT_EXIST);
				}

				if(std::filesystem::is_directory(package.basePath) == false){
					return evo::Unexpected(CreatePackageFailReason::PATH_NOT_DIRECTORY);
				}

				for(char character : package.name){
					if(evo::isAlphaNumeric(character) == false && character != '_' && character != '.'){
						return evo::Unexpected(CreatePackageFailReason::INVALID_NAME);
					}
				}

				for(const auto& [name, value] : package.options){
					if(name.empty()){ return evo::Unexpected(CreatePackageFailReason::INVALID_OPTION_NAME); }

					if(evo::isLetter(name[0]) == false && name[0] != '_'){
						return evo::Unexpected(CreatePackageFailReason::INVALID_OPTION_NAME);
					}

					for(size_t i = 1; i < name.size(); i+=1){
						if(evo::isAlphaNumeric(name[i]) == false && name[i] != '_'){
							return evo::Unexpected(CreatePackageFailReason::INVALID_OPTION_NAME);
						}
					}
				}

				return this->priv.packages.emplace_back(std::move(package));
			}

			[[nodiscard]] auto createPackage(const Source::Package& package)
			-> evo::Expected<Source::Package::ID, CreatePackageFailReason> {
				return this->createPackage(evo::copy(package));
			}


			[[nodiscard]] auto getPackage(Source::Package::ID id) const 
			-> const Source::Package& {
				return this->priv.packages[id];
			}


			[[nodiscard]] auto operator[](Source::ID id) const -> const Source& {
				const auto lock = std::lock_guard(this->priv.sources_lock);
				return this->priv.sources[id];
			}
			[[nodiscard]] auto operator[](Source::ID id) -> Source& {
				const auto lock = std::lock_guard(this->priv.sources_lock);
				return this->priv.sources[id];
			}

			[[nodiscard]] auto operator[](CFamilySource::ID id) const -> const CFamilySource& {
				const auto lock = std::lock_guard(this->priv.c_family_sources_lock);
				return this->priv.c_family_sources[id];
			}
			[[nodiscard]] auto operator[](CFamilySource::ID id) -> CFamilySource& {
				const auto lock = std::lock_guard(this->priv.c_family_sources_lock);
				return this->priv.c_family_sources[id];
			}

			[[nodiscard]] auto operator[](BuiltinModule::ID id) const -> const BuiltinModule& {
				return this->priv.builtin_modules[size_t(evo::to_underlying(id))];
			}
			[[nodiscard]] auto operator[](BuiltinModule::ID id) -> BuiltinModule& {
				return this->priv.builtin_modules[size_t(evo::to_underlying(id))];
			}



			[[nodiscard]] auto getSourceIDRange() const -> evo::IterRange<Source::ID::Iterator> {
				const auto lock = std::lock_guard(this->priv.sources_lock);
				return evo::IterRange<Source::ID::Iterator>(
					Source::ID::Iterator(Source::ID(0)),
					Source::ID::Iterator(Source::ID(uint32_t(this->priv.sources.size())))
				);
			}


			[[nodiscard]] auto size() const -> size_t {
				const auto lock = std::lock_guard(this->priv.sources_lock);
				return this->priv.sources.size();
			}


			[[nodiscard]] auto lookupSpecialNameSourceID(std::string_view path) const -> std::optional<Source::ID>;
			[[nodiscard]] auto lookupSourceID(std::string_view path) const -> std::optional<Source::ID>;
			[[nodiscard]] auto lookupCFamilySourceID(std::string_view path) const -> std::optional<CFamilySource::ID>;

			struct GottenCFamilySourceID{
				CFamilySource::ID id;
				bool created;
			};
			[[nodiscard]] auto getOrCreateCFamilySourceID(
				std::filesystem::path&& path, bool is_cpp, pir::Module* pir_module // nullptr if not adding debug info
			) -> GottenCFamilySourceID;
			[[nodiscard]] auto getOrCreateCFamilySourceID(
				std::string_view path, bool is_cpp, pir::Module* pir_module // nullptr if not adding debug info
			) -> GottenCFamilySourceID {
				return this->getOrCreateCFamilySourceID(std::filesystem::path(path), is_cpp, pir_module);
			}



			[[nodiscard]] auto getCFamilySourceIDRange() const -> evo::IterRange<CFamilySource::ID::Iterator> {
				const auto lock = std::lock_guard(this->priv.c_family_sources_lock);
				return evo::IterRange<CFamilySource::ID::Iterator>(
					CFamilySource::ID::Iterator(CFamilySource::ID(0)),
					CFamilySource::ID::Iterator(CFamilySource::ID(uint32_t(this->priv.c_family_sources.size())))
				);
			}

			

		private:
			auto create_source(
				std::filesystem::path&& path, std::string&& data_str, Source::Package::ID comp_config_id
			) -> Source::ID {
				const auto lock = std::lock_guard(this->priv.sources_lock);

				const Source::ID new_source_id = this->priv.sources.emplace_back(
					std::move(path), std::move(data_str), comp_config_id
				);

				this->priv.sources[new_source_id].id = new_source_id;

				return new_source_id;
			}

			auto create_c_family_source(std::filesystem::path&& path, std::string&& data_str, bool is_cpp)
			-> CFamilySource::ID {
				const auto lock = std::lock_guard(this->priv.c_family_sources_lock);

				const CFamilySource::ID new_source_id = this->priv.c_family_sources.emplace_back(
					std::move(path), std::move(data_str), is_cpp, std::nullopt
				);

				this->priv.c_family_sources[new_source_id].id = new_source_id;

				return new_source_id;
			}


			auto create_c_family_source_with_debug_info(
				std::filesystem::path&& path, std::string&& data_str, bool is_cpp, pir::Module& pir_module
			) -> CFamilySource::ID;


			auto add_special_name_path(std::string_view name, const std::filesystem::path& path) -> void {
				this->priv.special_name_paths.emplace(name, path);
			}


			[[nodiscard]] auto emplace_source_package_config(auto&&... args) -> Source::Package::ID {
				return this->priv.packages.emplace_back(std::forward<decltype(args)>(args)...);
			}


	
		private:
			// To prevent context from accessing private members while allowing access to private methods
			struct /* priv */ {
				private:
					core::LinearStepAlloc<Source, Source::ID, 0> sources{};
					mutable evo::SpinLock sources_lock{};

					core::LinearStepAlloc<CFamilySource, CFamilySource::ID, 0> c_family_sources{};
					mutable evo::SpinLock c_family_sources_lock{};

					std::array<BuiltinModule, 3> builtin_modules{};

					core::LinearStepAlloc<Source::Package, Source::Package::ID> packages{};

					std::unordered_map<std::string_view, std::filesystem::path> special_name_paths{};

					friend SourceManager;
			} priv;


			friend class Context;
	};

	
}