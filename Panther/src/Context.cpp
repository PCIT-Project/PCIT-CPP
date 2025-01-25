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
#include "./deps/DependencyAnalysis.h"
#include "./sema/SemanticAnalyzer.h"

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


	auto Context::optimalNumThreads() -> unsigned {
		return unsigned(core::ThreadPool<Task>::optimalNumThreads());
	}




	//////////////////////////////////////////////////////////////////////
	// build targets

	// TODO: force shutdown when hit fail condition
	auto Context::tokenize() -> bool {
		const auto worker = [&](Task& task) -> bool {
			this->tokenize_impl(std::move(task.as<FileToLoad>().path), task.as<FileToLoad>().compilation_config_id);
			return true;
		};

		if(this->_config.isMultiThreaded()){
			auto local_work_manager = core::ThreadPool<Task>();

			auto tasks = evo::SmallVector<Task>();
			for(FileToLoad& file_to_load : this->files_to_load){
				tasks.emplace_back(std::move(file_to_load));
			}

			local_work_manager.startup(this->_config.numThreads);
			local_work_manager.work(std::move(tasks), worker);
			local_work_manager.waitUntilDoneWorking();
			local_work_manager.shutdown();

		}else{
			auto local_work_manager = core::SingleThreadedWorkQueue<Task>(worker);

			for(FileToLoad& file_to_load : this->files_to_load){
				local_work_manager.emplaceTask(std::move(file_to_load));
			}

			this->files_to_load.clear();

			local_work_manager.run();
		}

		return this->num_errors == 0;
	}



	auto Context::parse() -> bool {
		const auto worker = [&](Task& task) -> bool {
			this->parse_impl(std::move(task.as<FileToLoad>().path), task.as<FileToLoad>().compilation_config_id);
			return true;
		};

		if(this->_config.isMultiThreaded()){
			auto local_work_manager = core::ThreadPool<Task>();

			auto tasks = evo::SmallVector<Task>();
			for(FileToLoad& file_to_load : this->files_to_load){
				tasks.emplace_back(std::move(file_to_load));
			}

			local_work_manager.startup(this->_config.numThreads);
			local_work_manager.work(std::move(tasks), worker);
			local_work_manager.waitUntilDoneWorking();
			local_work_manager.shutdown();

		}else{
			auto local_work_manager = core::SingleThreadedWorkQueue<Task>(worker);

			for(FileToLoad& file_to_load : this->files_to_load){
				local_work_manager.emplaceTask(std::move(file_to_load));
			}

			this->files_to_load.clear();

			local_work_manager.run();
		}

		return this->num_errors == 0;
	}


	auto Context::analyzeDependencies() -> bool {
		const auto worker = [&](Task& task) -> bool {
			this->dependency_analysis_impl(
				std::move(task.as<FileToLoad>().path), task.as<FileToLoad>().compilation_config_id
			);
			return true;
		};

		if(this->_config.isMultiThreaded()){
			auto local_work_manager = core::ThreadPool<Task>();

			auto tasks = evo::SmallVector<Task>();
			for(FileToLoad& file_to_load : this->files_to_load){
				tasks.emplace_back(std::move(file_to_load));
			}

			local_work_manager.startup(this->_config.numThreads);
			local_work_manager.work(std::move(tasks), worker);
			local_work_manager.waitUntilDoneWorking();
			local_work_manager.shutdown();

		}else{
			auto local_work_manager = core::SingleThreadedWorkQueue<Task>(worker);

			for(FileToLoad& file_to_load : this->files_to_load){
				local_work_manager.emplaceTask(std::move(file_to_load));
			}

			this->files_to_load.clear();

			local_work_manager.run();
		}

		return this->num_errors == 0;
	}



	auto Context::analyzeSemantics() -> bool {
		if(this->analyzeDependencies() == false){ return false; }

		if(this->type_manager.primitivesInitialized() == false){
			this->type_manager.initPrimitives();
		}

		for(const Source::ID& source_id : this->source_manager){
			this->source_manager[source_id].sema_scope_id = this->sema_buffer.scope_manager.createScope();
			this->sema_buffer.scope_manager
				.getScope(*this->source_manager[source_id].sema_scope_id)
				.pushLevel(this->sema_buffer.scope_manager.createLevel());
		}

		const auto worker = [&](Task& task_variant) -> bool {
			task_variant.visit([&](auto& task) -> void {
				using TaskType = std::decay_t<decltype(task)>;

				if constexpr(std::is_same<TaskType, FileToLoad>()){
					evo::debugFatalBreak("Should never hit this task");

				}else if constexpr(std::is_same<TaskType, SemaDecl>()){
					analyze_semantics_decl(*this, task.deps_node_id);

				}else if constexpr(std::is_same<TaskType, SemaDef>()){
					analyze_semantics_def(*this, task.deps_node_id);

				}else if constexpr(std::is_same<TaskType, SemaDeclDef>()){
					analyze_semantics_decl_def(*this, task.deps_node_id);

				}else{
					static_assert(false, "Unsupported task type");
				}
			});

			return !this->hasHitFailCondition();
		};


		if(this->_config.isMultiThreaded()){
			auto& work_manager_inst = this->work_manager.emplace<core::ThreadQueue<Task>>(worker);

			for(uint32_t i = 0; deps::Node& deps_node : this->deps_buffer){
				if(deps_node.hasNoDeclDeps()){ // has decl deps
					deps_node.declSemaStatus = deps::Node::SemaStatus::InQueue;
					if(deps_node.hasNoDefDeps()){
						deps_node.defSemaStatus = deps::Node::SemaStatus::InQueue;
						work_manager_inst.addTask(SemaDeclDef(deps::Node::ID(i)));
					}else{
						work_manager_inst.addTask(SemaDecl(deps::Node::ID(i)));
					}
				}

				i += 1;
			}

			work_manager_inst.startup(this->_config.numThreads);
			while(this->getNumErrors() == 0 && this->deps_buffer.num_nodes_sema_status_not_done > 0){
				// TODO: different amount of yields?
				std::this_thread::yield();
				std::this_thread::yield();
				std::this_thread::yield();
			}
			work_manager_inst.waitUntilDoneWorking(); // probably not needed, but I think the safety is worth it
			work_manager_inst.shutdown();

		}else{
			auto& work_manager_inst = this->work_manager.emplace<core::SingleThreadedWorkQueue<Task>>(worker);

			for(uint32_t i = 0; deps::Node& deps_node : this->deps_buffer){
				if(deps_node.hasNoDeclDeps()){
					deps_node.declSemaStatus = deps::Node::SemaStatus::InQueue;
					if(deps_node.hasNoDefDeps()){
						deps_node.defSemaStatus = deps::Node::SemaStatus::InQueue;
						work_manager_inst.addTask(SemaDeclDef(deps::Node::ID(i)));
					}else{
						work_manager_inst.addTask(SemaDecl(deps::Node::ID(i)));
					}
				}

				i += 1;
			}

			work_manager_inst.run();
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
				Diagnostic::Location::NONE,
				std::format("File \"{}\" does not exist", path.string())
			);
			return evo::resultError;
		}

		evo::Result<std::string> file_data = evo::fs::readFile(path.string());
		if(file_data.isError()){
			this->emitError(
				Diagnostic::Code::MiscLoadFileFailed,
				Diagnostic::Location::NONE,
				std::format("Failed to load file: \"{}\"", path.string())	
			);
			return evo::resultError;
		}

		return this->source_manager.create_source(std::move(path), std::move(file_data.value()), compilation_config_id);
	}


	auto Context::tokenize_impl(std::filesystem::path&& path, Source::CompilationConfig::ID compilation_config_id)
	-> void {
		const evo::Result<Source::ID> new_source = this->load_source(std::move(path), compilation_config_id);
		if(new_source.isError()){ return; }

		this->trace("Loaded file: \"{}\"", this->source_manager[new_source.value()].getPath().string());

		if(panther::tokenize(*this, new_source.value()) == false){ return; }
		this->trace("Tokenized file: \"{}\"", this->source_manager[new_source.value()].getPath().string());
	}


	auto Context::parse_impl(std::filesystem::path&& path, Source::CompilationConfig::ID compilation_config_id)
	-> void {
		const evo::Result<Source::ID> new_source = this->load_source(std::move(path), compilation_config_id);
		if(new_source.isError()){ return; }

		this->trace("Loaded file: \"{}\"", this->source_manager[new_source.value()].getPath().string());

		if(panther::tokenize(*this, new_source.value()) == false){ return; }
		this->trace("Tokenized file: \"{}\"", this->source_manager[new_source.value()].getPath().string());

		if(panther::parse(*this, new_source.value()) == false){ return; }
		this->trace("Parsed file: \"{}\"", this->source_manager[new_source.value()].getPath().string());
	}

	auto Context::dependency_analysis_impl(
		std::filesystem::path&& path, Source::CompilationConfig::ID compilation_config_id
	) -> evo::Result<Source::ID> {
		const evo::Result<Source::ID> new_source = this->load_source(std::move(path), compilation_config_id);
		if(new_source.isError()){ return evo::resultError; }

		this->trace("Loaded file: \"{}\"", this->source_manager[new_source.value()].getPath().string());

		if(panther::tokenize(*this, new_source.value()) == false){ return evo::resultError; }
		this->trace("Tokenized file: \"{}\"", this->source_manager[new_source.value()].getPath().string());

		if(panther::parse(*this, new_source.value()) == false){ return evo::resultError; }
		this->trace("Parsed file: \"{}\"", this->source_manager[new_source.value()].getPath().string());

		if(panther::analyzeDependencies(*this, new_source.value()) == false){ return evo::resultError; }
		this->trace(
			"Analyzed dependencies of file: \"{}\"", this->source_manager[new_source.value()].getPath().string()
		);

		return new_source.value();
	}



	//////////////////////////////////////////////////////////////////////
	// misc


	auto Context::emit_diagnostic_impl(const Diagnostic& diagnostic) -> void {
		const auto lock_guard = std::lock_guard(this->diagnostic_callback_mutex);

		this->_diagnostic_callback(*this, diagnostic);
	}

	
}
