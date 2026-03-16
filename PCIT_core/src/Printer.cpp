////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/Printer.h"


#if defined(EVO_PLATFORM_WINDOWS)
	#if !defined(WIN32_LEAN_AND_MEAN)
		#define WIN32_LEAN_AND_MEAN
	#endif

	#if !defined(NOCOMM)
		#define NOCOMM
	#endif

	#if !defined(NOMINMAX)
		#define NOMINMAX
	#endif
	
	#include <windows.h>

	// This is for managing setting console mode in windows (and only doing it once)
	static struct /* win_color_console */ {
		uint32_t starting_mode;
		std::atomic<size_t> num_color_created = 0;
		bool mode_change_needed = false;
	} win_color_console;
#endif


namespace pcit::core{


	namespace styleConsole{
		static constexpr auto reset()  -> std::string_view { return "\033[0m"; };

		namespace text{
			static constexpr auto white()    -> std::string_view { return "\033[37m"; };
			static constexpr auto gray()     -> std::string_view { return "\033[90m"; };
			static constexpr auto black()    -> std::string_view { return "\033[30m"; };

			static constexpr auto red()      -> std::string_view { return "\033[31m"; };
			static constexpr auto yellow()   -> std::string_view { return "\033[33m"; };
			static constexpr auto green()    -> std::string_view { return "\033[32m"; };
			static constexpr auto cyan()     -> std::string_view { return "\033[36m"; };
			static constexpr auto blue()     -> std::string_view { return "\033[34m"; };
			static constexpr auto magenta()  -> std::string_view { return "\033[35m"; };
		};

		namespace background{
			static constexpr auto white()    -> std::string_view { return "\033[47m"; };
			static constexpr auto gray()     -> std::string_view { return "\033[100m"; };
			static constexpr auto black()    -> std::string_view { return "\033[40m"; };

			static constexpr auto red()      -> std::string_view { return "\033[41m"; };
			static constexpr auto yellow()   -> std::string_view { return "\033[43m"; };
			static constexpr auto green()    -> std::string_view { return "\033[42m"; };
			static constexpr auto cyan()     -> std::string_view { return "\033[46m"; };
			static constexpr auto blue()     -> std::string_view { return "\033[44m"; };
			static constexpr auto magenta()  -> std::string_view { return "\033[45m"; };
		};
	}



	Printer::Printer(Mode _mode) : mode(_mode) {
		#if defined(EVO_PLATFORM_WINDOWS)
			if(this->isPrintingColor() && std::atomic_fetch_add(&win_color_console.num_color_created, 1) == 0){
				const auto win_console_handle = ::GetStdHandle(STD_OUTPUT_HANDLE);

				DWORD win_console_set_mode;
				::GetConsoleMode(win_console_handle, &win_console_set_mode);
				win_color_console.starting_mode = win_console_set_mode;

				if((win_color_console.starting_mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) == 0){
					win_console_set_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
				}

				if((win_color_console.starting_mode & ENABLE_PROCESSED_OUTPUT) == 0){
					win_console_set_mode |= ENABLE_PROCESSED_OUTPUT;
				}

				if(win_console_set_mode != win_color_console.starting_mode){
					win_color_console.mode_change_needed = true;
					::SetConsoleMode(win_console_handle, win_console_set_mode);
				}
			}
		#endif
	}


	Printer::~Printer() {
		#if defined(EVO_PLATFORM_WINDOWS)
			if(
				this->isPrintingColor() && 
				win_color_console.mode_change_needed && 
				std::atomic_fetch_sub(&win_color_console.num_color_created, 1) == 1
			){
				::SetConsoleMode(::GetStdHandle(STD_OUTPUT_HANDLE), win_color_console.starting_mode);
				win_color_console.mode_change_needed = false;
			}
		#endif	
	}



	Printer::Printer(const Printer& rhs) : mode(rhs.mode) {
		#if defined(EVO_PLATFORM_WINDOWS)
			if(this->isPrintingColor()){
				win_color_console.num_color_created += 1;
			}
		#endif
	}

	Printer::Printer(Printer&& rhs) : mode(rhs.mode) {
		#if defined(EVO_PLATFORM_WINDOWS)
			rhs.mode = Mode::UNINIT;
		#endif
	}




	auto Printer::platformSupportsColor() -> DetectResult {
		#if defined(EVO_PLATFORM_WINDOWS)
			return DetectResult::YES;

		#elif defined(EVO_PLATFORM_LINUX)
			if(std::getenv("TERM") == nullptr){
				return DetectResult::YES;
			}else{
				return DetectResult::NO;
			}

		#else
			return DetectResult::UNKNOWN;
		#endif
	}



	auto Printer::print(std::string_view str) -> void {
		if(this->isPrintingString()){
			this->string += str;
		}else{
			evo::print(str);
		}
	}

	auto Printer::println(std::string_view str) -> void {
		if(this->isPrintingString()){
			this->string += str;
			this->string += '\n';
		}else{
			evo::println(str);
		}
	}
	

	auto Printer::printFatal(std::string_view str) -> void {
		if(this->isPrintingColor()){
			this->print(styleConsole::text::white());
			this->print(styleConsole::background::red());
			this->print(str);
			this->print(styleConsole::reset());

		}else{
			this->print(str);
		}
	}
	
	auto Printer::printError(std::string_view str) -> void {
		if(this->isPrintingColor()){
			this->print(styleConsole::text::red());
			this->print(str);
			this->print(styleConsole::reset());
		}else{
			this->print(str);
		}
	}
	
	auto Printer::printWarning(std::string_view str) -> void {
		if(this->isPrintingColor()){
			this->print(styleConsole::text::yellow());
			this->print(str);
			this->print(styleConsole::reset());
		}else{
			this->print(str);
		}
	}
	
	auto Printer::printInfo(std::string_view str) -> void {
		if(this->isPrintingColor()){
			this->print(styleConsole::text::cyan());
			this->print(str);
			this->print(styleConsole::reset());
		}else{
			this->print(str);
		}
	}

	auto Printer::printSuccess(std::string_view str) -> void {
		if(this->isPrintingColor()){
			this->print(styleConsole::text::green());
			this->print(str);
			this->print(styleConsole::reset());
		}else{
			this->print(str);
		}
	}
	

	auto Printer::printRed(std::string_view str) -> void {
		if(this->isPrintingColor()){
			this->print(styleConsole::text::red());
			this->print(str);
			this->print(styleConsole::reset());
		}else{
			this->print(str);
		}
	}
	
	auto Printer::printYellow(std::string_view str) -> void {
		if(this->isPrintingColor()){
			this->print(styleConsole::text::yellow());
			this->print(str);
			this->print(styleConsole::reset());
		}else{
			this->print(str);
		}
	}
	
	auto Printer::printGreen(std::string_view str) -> void {
		if(this->isPrintingColor()){
			this->print(styleConsole::text::green());
			this->print(str);
			this->print(styleConsole::reset());
		}else{
			this->print(str);
		}
	}
	
	auto Printer::printBlue(std::string_view str) -> void {
		if(this->isPrintingColor()){
			this->print(styleConsole::text::blue());
			this->print(str);
			this->print(styleConsole::reset());
		}else{
			this->print(str);
		}
	}
	
	auto Printer::printCyan(std::string_view str) -> void {
		if(this->isPrintingColor()){
			this->print(styleConsole::text::cyan());
			this->print(str);
			this->print(styleConsole::reset());
		}else{
			this->print(str);
		}
	}
	
	auto Printer::printMagenta(std::string_view str) -> void {
		if(this->isPrintingColor()){
			this->print(styleConsole::text::magenta());
			this->print(str);
			this->print(styleConsole::reset());
		}else{
			this->print(str);
		}
	}
	
	auto Printer::printGray(std::string_view str) -> void {
		if(this->isPrintingColor()){
			this->print(styleConsole::text::gray());
			this->print(str);
			this->print(styleConsole::reset());
		}else{
			this->print(str);
		}
	}
	





	
}