//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
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


	Printer::Printer() {
		std::construct_at(this, Printer::platformSupportsColor() == DetectResult::Yes);
	}

	Printer::Printer(bool should_print_color) : print_color(should_print_color) {
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



	Printer::Printer(const Printer& rhs) : print_color(rhs.print_color) {
		#if defined(EVO_PLATFORM_WINDOWS)
			if(this->isPrintingColor()){
				win_color_console.num_color_created += 1;
			}
		#endif
	}

	Printer::Printer(Printer&& rhs) : print_color(rhs.print_color) {
		#if defined(EVO_PLATFORM_WINDOWS)
			rhs.print_color = false;
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
		evo::print(str);
	}
	

	auto Printer::printFatal(std::string_view str) -> void {
		if(this->isPrintingColor()){
			evo::styleConsole::fatal();
			evo::print(str);
			evo::styleConsole::reset();
		}else{
			evo::print(str);
		}
	}
	
	auto Printer::printError(std::string_view str) -> void {
		if(this->isPrintingColor()){
			evo::styleConsole::error();
			evo::print(str);
			evo::styleConsole::reset();
		}else{
			evo::print(str);
		}
	}
	
	auto Printer::printWarning(std::string_view str) -> void {
		if(this->isPrintingColor()){
			evo::styleConsole::warning();
			evo::print(str);
			evo::styleConsole::reset();
		}else{
			evo::print(str);
		}
	}
	
	auto Printer::printInfo(std::string_view str) -> void {
		if(this->isPrintingColor()){
			evo::styleConsole::info();
			evo::print(str);
			evo::styleConsole::reset();
		}else{
			evo::print(str);
		}
	}

	auto Printer::printSuccess(std::string_view str) -> void {
		if(this->isPrintingColor()){
			evo::styleConsole::success();
			evo::print(str);
			evo::styleConsole::reset();
		}else{
			evo::print(str);
		}
	}
	

	auto Printer::printRed(std::string_view str) -> void {
		if(this->isPrintingColor()){
			evo::styleConsole::text::red();
			evo::print(str);
			evo::styleConsole::reset();
		}else{
			evo::print(str);
		}
	}
	
	auto Printer::printYellow(std::string_view str) -> void {
		if(this->isPrintingColor()){
			evo::styleConsole::text::yellow();
			evo::print(str);
			evo::styleConsole::reset();
		}else{
			evo::print(str);
		}
	}
	
	auto Printer::printGreen(std::string_view str) -> void {
		if(this->isPrintingColor()){
			evo::styleConsole::text::green();
			evo::print(str);
			evo::styleConsole::reset();
		}else{
			evo::print(str);
		}
	}
	
	auto Printer::printBlue(std::string_view str) -> void {
		if(this->isPrintingColor()){
			evo::styleConsole::text::blue();
			evo::print(str);
			evo::styleConsole::reset();
		}else{
			evo::print(str);
		}
	}
	
	auto Printer::printCyan(std::string_view str) -> void {
		if(this->isPrintingColor()){
			evo::styleConsole::text::cyan();
			evo::print(str);
			evo::styleConsole::reset();
		}else{
			evo::print(str);
		}
	}
	
	auto Printer::printMagenta(std::string_view str) -> void {
		if(this->isPrintingColor()){
			evo::styleConsole::text::magenta();
			evo::print(str);
			evo::styleConsole::reset();
		}else{
			evo::print(str);
		}
	}
	
	auto Printer::printGray(std::string_view str) -> void {
		if(this->isPrintingColor()){
			evo::styleConsole::text::gray();
			evo::print(str);
			evo::styleConsole::reset();
		}else{
			evo::print(str);
		}
	}
	





	
}