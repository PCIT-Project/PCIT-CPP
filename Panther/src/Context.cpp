////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/Context.h"

#include "./tokens/Tokenizer.h"
#include "./AST/Parser.h"

namespace pcit::panther{


	//////////////////////////////////////////////////////////////////////
	// path helpers

	EVO_NODISCARD static auto path_exitsts(const std::filesystem::path& path) -> bool {
		auto ec = std::error_code();
		return std::filesystem::exists(path, ec) && (ec.value() == 0);
	}

	EVO_NODISCARD static auto path_is_pthr_file(const std::filesystem::path& path) -> bool {
		return path_exitsts(path) && path.extension() == ".pthr";
	}

	EVO_NODISCARD static auto normalize_path(const std::filesystem::path& path, const std::filesystem::path& base_path)
	-> std::filesystem::path {
		return (base_path / path).lexically_normal();
	}


	//////////////////////////////////////////////////////////////////////
	// misc

	Context::~Context(){
		if(this->thread_pool.isRunning()){
			this->thread_pool.shutdown();
		}
	}

	auto Context::optimalNumThreads() -> unsigned {
		return unsigned(core::ThreadPool<Task>::optimalNumThreads());
	}




	//////////////////////////////////////////////////////////////////////
	// build targets

	auto Context::tokenize() -> bool {
		auto worker = [&](Task& task) -> bool {
			const evo::Result<Source::ID> new_source = this->load_source(
				std::move(task.file_to_load.path), task.file_to_load.compilation_config_id
			);
			if(new_source.isError()){ return true; }

			this->trace("Loaded file: \"{}\"", this->source_manager[new_source.value()].getPath().string());

			if(panther::tokenize(*this, new_source.value()) == false){ return true; }

			this->trace("Tokenized file: \"{}\"", this->source_manager[new_source.value()].getPath().string());

			return true;
		};

		if(this->_config.isMultiThreaded()){
			this->work_manager.emplace<core::ThreadPool<Task>>();

			auto tasks = evo::SmallVector<Task>();
			for(FileToLoad& file_to_load : this->files_to_load){
				tasks.emplace_back(std::move(file_to_load));
			}

			this->work_manager.as<core::ThreadPool<Task>>().startup(this->_config.numThreads);
			this->work_manager.as<core::ThreadPool<Task>>().work(std::move(tasks), worker);
			this->work_manager.as<core::ThreadPool<Task>>().waitUntilDoneWorking();
			this->work_manager.as<core::ThreadPool<Task>>().shutdown();

		}else{
			this->work_manager.emplace<core::SingleThreadedWorkQueue<Task>>(worker);

			for(FileToLoad& file_to_load : this->files_to_load){
				this->work_manager.as<core::SingleThreadedWorkQueue<Task>>().emplaceTask(std::move(file_to_load));
			}

			this->files_to_load.clear();

			this->work_manager.as<core::SingleThreadedWorkQueue<Task>>().run();
		}

		return this->num_errors == 0;
	}



	auto Context::parse() -> bool {
		auto worker = [&](Task& task) -> bool {
			const evo::Result<Source::ID> new_source = this->load_source(
				std::move(task.file_to_load.path), task.file_to_load.compilation_config_id
			);
			if(new_source.isError()){ return true; }

			this->trace("Loaded file: \"{}\"", this->source_manager[new_source.value()].getPath().string());

			if(panther::tokenize(*this, new_source.value()) == false){ return true; }
			this->trace("Tokenized file: \"{}\"", this->source_manager[new_source.value()].getPath().string());

			if(panther::parse(*this, new_source.value()) == false){ return true; }
			this->trace("Parsed file: \"{}\"", this->source_manager[new_source.value()].getPath().string());

			return true;
		};

		if(this->_config.isMultiThreaded()){
			this->work_manager.emplace<core::ThreadPool<Task>>();

			auto tasks = evo::SmallVector<Task>();
			for(FileToLoad& file_to_load : this->files_to_load){
				tasks.emplace_back(std::move(file_to_load));
			}

			this->work_manager.as<core::ThreadPool<Task>>().startup(this->_config.numThreads);
			this->work_manager.as<core::ThreadPool<Task>>().work(std::move(tasks), worker);
			this->work_manager.as<core::ThreadPool<Task>>().waitUntilDoneWorking();
			this->work_manager.as<core::ThreadPool<Task>>().shutdown();

		}else{
			this->work_manager.emplace<core::SingleThreadedWorkQueue<Task>>(worker);

			for(FileToLoad& file_to_load : this->files_to_load){
				this->work_manager.as<core::SingleThreadedWorkQueue<Task>>().emplaceTask(std::move(file_to_load));
			}

			this->files_to_load.clear();

			this->work_manager.as<core::SingleThreadedWorkQueue<Task>>().run();
		}

		return this->num_errors == 0;
	}





	//////////////////////////////////////////////////////////////////////
	// adding sources

	auto Context::addSourceFile(const std::filesystem::path& path, Source::CompilationConfig::ID compilation_config_id)
	-> AddSourceResult {
		evo::debugAssert(this->mayAddSourceFile(), "Cannot add any source files");
		if(path_exitsts(path) == false){ return AddSourceResult::DoesntExist; }

		const Source::CompilationConfig& compilation_config =
			this->source_manager.getSourceCompilationConfig(compilation_config_id);
		this->files_to_load.emplace_back(
			normalize_path(path, compilation_config.basePath), compilation_config_id
		);

		return AddSourceResult::Success;
	}

	auto Context::addSourceDirectory(
		const std::filesystem::path& directory, Source::CompilationConfig::ID compilation_config_id
	) -> AddSourceResult {
		evo::debugAssert(this->mayAddSourceFile(), "Cannot add any source files");
		if(path_exitsts(directory) == false){ return AddSourceResult::DoesntExist; }
		if(std::filesystem::is_directory(directory) == false){ return AddSourceResult::NotDirectory; }

		const Source::CompilationConfig& compilation_config =
			this->source_manager.getSourceCompilationConfig(compilation_config_id);

		for(const std::filesystem::path& file_path : std::filesystem::directory_iterator(directory)){
			if(path_is_pthr_file(file_path)){
				this->files_to_load.emplace_back(
					normalize_path(file_path, compilation_config.basePath), compilation_config_id
				);
			}
		}

		return AddSourceResult::Success;
	}

	auto Context::addSourceDirectoryRecursive(
		const std::filesystem::path& directory, Source::CompilationConfig::ID compilation_config_id
	) -> AddSourceResult {
		evo::debugAssert(this->mayAddSourceFile(), "Cannot add any source files");
		if(path_exitsts(directory) == false){ return AddSourceResult::DoesntExist; }
		if(std::filesystem::is_directory(directory) == false){ return AddSourceResult::NotDirectory; }

		const Source::CompilationConfig& compilation_config =
			this->source_manager.getSourceCompilationConfig(compilation_config_id);

		for(const std::filesystem::path& file_path : std::filesystem::recursive_directory_iterator(directory)){
			if(path_is_pthr_file(file_path)){
				this->files_to_load.emplace_back(
					normalize_path(file_path, compilation_config.basePath), compilation_config_id
				);
			}
		}

		return AddSourceResult::Success;
	}

	auto Context::addStdLib(const std::filesystem::path& directory) -> AddSourceResult {
		evo::debugAssert(this->mayAddSourceFile(), "Cannot add any source files");
		evo::debugAssert(directory.is_absolute(), "std lib directory must be absolute");
		evo::debugAssert(this->added_std_lib == false, "already added std lib");

		if(path_exitsts(directory) == false){ return AddSourceResult::DoesntExist; }
		if(std::filesystem::is_directory(directory) == false){ return AddSourceResult::NotDirectory; }

		const Source::CompilationConfig::ID compilation_config_id = 
			this->source_manager.emplace_source_compilation_config(directory);
		const Source::CompilationConfig& compilation_config =
			this->source_manager.getSourceCompilationConfig(compilation_config_id);

		for(const std::filesystem::path& file_path : std::filesystem::recursive_directory_iterator(directory)){
			if(path_is_pthr_file(file_path)){
				if(file_path.stem() == "std"){  this->source_manager.add_special_name_path("std", file_path);  }
				if(file_path.stem() == "math"){ this->source_manager.add_special_name_path("math", file_path); }

				this->files_to_load.emplace_back(
					normalize_path(file_path, compilation_config.basePath), compilation_config_id
				);
			}
		}

		this->added_std_lib = true;

		return AddSourceResult::Success;
	}



	//////////////////////////////////////////////////////////////////////
	// task 

	EVO_NODISCARD auto Context::load_source(
		std::filesystem::path&& path, Source::CompilationConfig::ID compilation_config_id
	) -> evo::Result<Source::ID> {
		if(std::filesystem::exists(path) == false){
			this->emitError(
				Diagnostic::Code::MiscFileDoesNotExist,
				Diagnostic::noLocation,
				std::format("File \"{}\" does not exist", path.string())
			);
			return evo::resultError;
		}

		evo::Result<std::string> file_data = evo::fs::readFile(path.string());
		if(file_data.isError()){
			this->emitError(
				Diagnostic::Code::MiscLoadFileFailed,
				Diagnostic::noLocation,
				std::format("Failed to load file: \"{}\"", path.string())	
			);
			return evo::resultError;
		}

		return this->source_manager.create_source(std::move(path), std::move(file_data.value()), compilation_config_id);
	}



	//////////////////////////////////////////////////////////////////////
	// misc


	auto Context::emit_diagnostic_impl(const Diagnostic& diagnostic) -> void {
		const auto lock_guard = std::lock_guard(this->diagnostic_callback_mutex);

		this->_diagnostic_callback(*this, diagnostic);
	}

	
}
