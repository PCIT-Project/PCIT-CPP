////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./args.hpp"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

#include "./printing.hpp"


namespace pthr{


	auto print_help(core::Printer& printer) -> void {
		pthr::print_logo(printer);
		pthr::print_version(printer);

		printer.print("\nUsage: \033[36mpthr\033[0m [action] {target file}? [options]\n");

		printer.print(R"(
Actions:
    build           run the build system (if no target file is given, defaults to "./build.pthr")
    script          run as a script (if no target file is given, defaults to "./script.pthr")
    version         prints the current version
    help            prints the help page

Options:
    -noColor        Disables color printing
    -noStdLib       Doesn't include the standard library
    -v=VALUE        Sets the verbosity level
        none        (default)
        some
        full

    -workDir {PATH} Sets the working directory (if not given, defaults to current)
    
)");

	}


	auto parse_args(std::string_view action, evo::ArrayProxy<std::string_view> args)
	-> evo::Result<pthr::CmdArgsConfig> {
		const std::filesystem::path executable_path = [&]() -> std::filesystem::path {
			#if defined(EVO_PLATFORM_WINDOWS)
				return core::windows::getExecutablePath();
			#else
				return core::linux::getExecutablePath();
			#endif	
		}();

		auto cmd_args_config = pthr::CmdArgsConfig(executable_path.parent_path());


		if(action == "build"){
			cmd_args_config.action = pthr::CmdArgsConfig::Action::BUILD;

			if(args.empty() == false && args[0][0] != '-'){
				cmd_args_config.file = args[0];
			}

		}else if(action == "script"){
			cmd_args_config.action = pthr::CmdArgsConfig::Action::SCRIPT;

			if(args.empty() == false && args[0][0] != '-'){
				cmd_args_config.file = args[0];
			}

		}else if(action == "help"){
			cmd_args_config.action = pthr::CmdArgsConfig::Action::HELP;

		}else if(action == "version"){
			cmd_args_config.action = pthr::CmdArgsConfig::Action::VERSION;

		}else{
			cmd_args_config.printError("Unknown action \"{}\"", action);
			return evo::resultError;
		}


		struct Arg{
			enum class Kind{
				SINGLE,
				ATTACHED_VALUE,
				SEPARATE_VALUE,
			};

			using Func = evo::Result<>(*)(pthr::CmdArgsConfig&, std::string_view);

			Kind kind;
			Func func;
		};

		auto arg_map = std::unordered_map<std::string_view, Arg>{
			std::pair<std::string_view, Arg>{
				"noStdLib",
				Arg(
					Arg::Kind::SINGLE,
					[](pthr::CmdArgsConfig& cmd_args_config, [[maybe_unused]] std::string_view value_str)
					-> evo::Result<> {
						cmd_args_config.use_std_lib = false;
						return evo::Result<>();
					}
				)
			},
			std::pair<std::string_view, Arg>{
				"noColor",
				Arg(
					Arg::Kind::SINGLE,
					[](pthr::CmdArgsConfig& cmd_args_config, [[maybe_unused]] std::string_view value_str)
					-> evo::Result<> {
						cmd_args_config.print_color = false;
						return evo::Result<>();
					}
				)
			},
			std::pair<std::string_view, Arg>{
				"v",
				Arg(
					Arg::Kind::ATTACHED_VALUE,
					[](pthr::CmdArgsConfig& cmd_args_config, std::string_view value_str) -> evo::Result<> {
						if(value_str == "full"){
							cmd_args_config.verbosity = pthr::CmdArgsConfig::Verbosity::FULL;

						}else if(value_str == "some"){
							cmd_args_config.verbosity = pthr::CmdArgsConfig::Verbosity::SOME;

						}else if(value_str == "none"){
							cmd_args_config.verbosity = pthr::CmdArgsConfig::Verbosity::NONE;

						}else{
							cmd_args_config.printError("Unknown verbosity argument value \"{}\"", value_str);
							return evo::resultError;
						}

						return evo::Result<>();
					}
				)
			},
			std::pair<std::string_view, Arg>{
				"workDir",
				Arg(
					Arg::Kind::SEPARATE_VALUE,
					[](pthr::CmdArgsConfig& cmd_args_config, std::string_view value_str) -> evo::Result<> {
						cmd_args_config.workingDirectory = std::filesystem::path(value_str);
						return evo::Result<>();
					}
				)
			},
		};

			
		for(size_t i = size_t(cmd_args_config.file.has_value()); i < args.size(); i+=1){
			const std::string_view arg_str = args[i];

			if(arg_str[0] != '-'){
				cmd_args_config.printError("Unknown argument \"{}\" (all arguments begin with a `-`)", arg_str);
				return evo::resultError;
			}

			size_t end_index = 1;
			while(end_index < arg_str.size()){
				if(arg_str[end_index] == '='){
					break;
				}
				end_index += 1;
			}

			

			const auto find_arg = arg_map.find(arg_str.substr(1, end_index - 1));

			if(find_arg == arg_map.end()){
				cmd_args_config.printError("Unknown argument \"-{}\"", arg_str.substr(1, end_index - 1));
				return evo::resultError;
			}

			const Arg& arg = find_arg->second;

			switch(arg.kind){
				case Arg::Kind::SINGLE: {
					if(end_index != arg_str.size()){
						cmd_args_config.printError("This argument cannot have a value");
						return evo::resultError;
					}

					if(arg.func(cmd_args_config, std::string_view()).isError()){ return evo::resultError; }
				} break;

				case Arg::Kind::ATTACHED_VALUE: {
					if(end_index == arg_str.size()){
						cmd_args_config.printError("This argument requires an attached value ({}={{VALUE}})", arg_str);
						return evo::resultError;
					}

					if(end_index + 1 == arg_str.size()){
						cmd_args_config.printError(
							"This argument requires a value after the `=` ({}{{VALUE}})", arg_str
						);
						return evo::resultError;
					}

					if(arg.func(cmd_args_config, arg_str.substr(end_index + 1)).isError()){ return evo::resultError; }
				} break;

				case Arg::Kind::SEPARATE_VALUE: {
					if(end_index != arg_str.size()){
						cmd_args_config.printError("This argument must have a separate value ({} {{VALUE}})", arg_str);
						return evo::resultError;
					}

					if(i + 1 == args.size()){
						cmd_args_config.printError("This argument must have a separate value ({} {{VALUE}})", arg_str);
						return evo::resultError;	
					}

					if(arg.func(cmd_args_config, args[i + 1]).isError()){ return evo::resultError; }
					i += 1;
				} break;
			}
		}

		return cmd_args_config;
	}

	
}
