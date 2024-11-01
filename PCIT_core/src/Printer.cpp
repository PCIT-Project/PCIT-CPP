//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


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
			rhs.mode = Mode::Uninit;
		#endif
	}




	auto Printer::platformSupportsColor() -> DetectResult {
		#if defined(EVO_PLATFORM_WINDOWS)
			return DetectResult::Yes;

		#elif defined(EVO_PLATFORM_LINUX)
			if(std::getenv("TERM") == nullptr){
				return DetectResult::Yes;
			}else{
				return DetectResult::No;
			}

		#else
			return DetectResult::Unknown;
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
		}else{
			evo::println(str);
		}
	}
	

	auto Printer::printFatal(std::string_view str) -> void {
		if(this->isPrintingColor()){
			evo::styleConsole::fatal();
			this->print(str);
			evo::styleConsole::reset();
		}else{
			this->print(str);
		}
	}
	
	auto Printer::printError(std::string_view str) -> void {
		if(this->isPrintingColor()){
			evo::styleConsole::error();
			this->print(str);
			evo::styleConsole::reset();
		}else{
			this->print(str);
		}
	}
	
	auto Printer::printWarning(std::string_view str) -> void {
		if(this->isPrintingColor()){
			evo::styleConsole::warning();
			this->print(str);
			evo::styleConsole::reset();
		}else{
			this->print(str);
		}
	}
	
	auto Printer::printInfo(std::string_view str) -> void {
		if(this->isPrintingColor()){
			evo::styleConsole::info();
			this->print(str);
			evo::styleConsole::reset();
		}else{
			this->print(str);
		}
	}

	auto Printer::printSuccess(std::string_view str) -> void {
		if(this->isPrintingColor()){
			evo::styleConsole::success();
			this->print(str);
			evo::styleConsole::reset();
		}else{
			this->print(str);
		}
	}
	

	auto Printer::printRed(std::string_view str) -> void {
		if(this->isPrintingColor()){
			evo::styleConsole::text::red();
			this->print(str);
			evo::styleConsole::reset();
		}else{
			this->print(str);
		}
	}
	
	auto Printer::printYellow(std::string_view str) -> void {
		if(this->isPrintingColor()){
			evo::styleConsole::text::yellow();
			this->print(str);
			evo::styleConsole::reset();
		}else{
			this->print(str);
		}
	}
	
	auto Printer::printGreen(std::string_view str) -> void {
		if(this->isPrintingColor()){
			evo::styleConsole::text::green();
			this->print(str);
			evo::styleConsole::reset();
		}else{
			this->print(str);
		}
	}
	
	auto Printer::printBlue(std::string_view str) -> void {
		if(this->isPrintingColor()){
			evo::styleConsole::text::blue();
			this->print(str);
			evo::styleConsole::reset();
		}else{
			this->print(str);
		}
	}
	
	auto Printer::printCyan(std::string_view str) -> void {
		if(this->isPrintingColor()){
			evo::styleConsole::text::cyan();
			this->print(str);
			evo::styleConsole::reset();
		}else{
			this->print(str);
		}
	}
	
	auto Printer::printMagenta(std::string_view str) -> void {
		if(this->isPrintingColor()){
			evo::styleConsole::text::magenta();
			this->print(str);
			evo::styleConsole::reset();
		}else{
			this->print(str);
		}
	}
	
	auto Printer::printGray(std::string_view str) -> void {
		if(this->isPrintingColor()){
			evo::styleConsole::text::gray();
			this->print(str);
			evo::styleConsole::reset();
		}else{
			this->print(str);
		}
	}
	





	
}