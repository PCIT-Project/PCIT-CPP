////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>

namespace pcit::core{


	class Printer{
		private:
			enum class Mode{
				CONSOLE_COLOR,
				CONSOLE,
				STRING,

				UNINIT, // to facilitate move
			};

			Printer(Mode _mode);

		public:
			static EVO_NODISCARD auto createConsole(bool with_color) -> Printer {
				if(with_color){
					return Printer(Mode::CONSOLE_COLOR);
				}else{
					return Printer(Mode::CONSOLE);
				}
			}

			static EVO_NODISCARD auto createConsole() -> Printer {
				return createConsole(platformSupportsColor() == DetectResult::YES);
			}

			static EVO_NODISCARD auto createString() -> Printer {
				return Printer(Mode::STRING);
			}


			~Printer();

			Printer(const Printer& rhs);
			Printer(Printer&& rhs);


			enum class DetectResult{
				YES,
				NO,
				UNKNOWN,
			};
			EVO_NODISCARD static auto platformSupportsColor() -> DetectResult;


			EVO_NODISCARD auto isPrintingColor() const -> bool { return this->mode == Mode::CONSOLE_COLOR; }
			EVO_NODISCARD auto isPrintingToConsle() const -> bool {
				return this->mode == Mode::CONSOLE_COLOR || this->mode == Mode::CONSOLE;
			}

			EVO_NODISCARD auto isPrintingString() const -> bool { return this->mode == Mode::STRING; }


			EVO_NODISCARD auto getString() const -> std::string {
				evo::debugAssert(this->isPrintingString(), "not printing a string");
				return this->string;
			}


			//////////////////////////////////////////////////////////////////////
			// printing


			auto print(std::string_view str) -> void;

			template<class... Args>
			auto print(std::format_string<Args...> fmt, Args&&... args) -> void {
				this->print(std::format(fmt, std::forward<decltype(args)>(args)...));
			}


			auto println() -> void { this->print("\n"); }

			auto println(std::string_view str) -> void;

			template<class... Args>
			auto println(std::format_string<Args...> fmt, Args&&... args) -> void {
				this->println(std::format(fmt, std::forward<decltype(args)>(args)...));
			}


			///////////////////////////////////
			// print Fatal

			auto printFatal(std::string_view str) -> void;

			template<class... Args>
			auto printFatal(std::format_string<Args...> fmt, Args&&... args) -> void {
				this->printFatal(std::format(fmt, std::forward<decltype(args)>(args)...));
			}

			auto printlnFatal(std::string_view str) -> void {
				this->printFatal(std::format("{}\n", str));
			}

			template<class... Args>
			auto printlnFatal(std::format_string<Args...> fmt, Args&&... args) -> void {
				this->printlnFatal(std::format(fmt, std::forward<decltype(args)>(args)...));
			}


			///////////////////////////////////
			// print Error

			auto printError(std::string_view str) -> void;

			template<class... Args>
			auto printError(std::format_string<Args...> fmt, Args&&... args) -> void {
				this->printError(std::format(fmt, std::forward<decltype(args)>(args)...));
			}

			auto printlnError(std::string_view str) -> void {
				this->printError(std::format("{}\n", str));
			}

			template<class... Args>
			auto printlnError(std::format_string<Args...> fmt, Args&&... args) -> void {
				this->printlnError(std::format(fmt, std::forward<decltype(args)>(args)...));
			}


			///////////////////////////////////
			// print Warning

			auto printWarning(std::string_view str) -> void;

			template<class... Args>
			auto printWarning(std::format_string<Args...> fmt, Args&&... args) -> void {
				this->printWarning(std::format(fmt, std::forward<decltype(args)>(args)...));
			}

			auto printlnWarning(std::string_view str) -> void {
				this->printWarning(std::format("{}\n", str));
			}

			template<class... Args>
			auto printlnWarning(std::format_string<Args...> fmt, Args&&... args) -> void {
				this->printlnWarning(std::format(fmt, std::forward<decltype(args)>(args)...));
			}


			///////////////////////////////////
			// print Info

			auto printInfo(std::string_view str) -> void;

			template<class... Args>
			auto printInfo(std::format_string<Args...> fmt, Args&&... args) -> void {
				this->printInfo(std::format(fmt, std::forward<decltype(args)>(args)...));
			}

			auto printlnInfo(std::string_view str) -> void {
				this->printInfo(std::format("{}\n", str));
			}

			template<class... Args>
			auto printlnInfo(std::format_string<Args...> fmt, Args&&... args) -> void {
				this->printlnInfo(std::format(fmt, std::forward<decltype(args)>(args)...));
			}


			///////////////////////////////////
			// print Success

			auto printSuccess(std::string_view str) -> void;

			template<class... Args>
			auto printSuccess(std::format_string<Args...> fmt, Args&&... args) -> void {
				this->printSuccess(std::format(fmt, std::forward<decltype(args)>(args)...));
			}

			auto printlnSuccess(std::string_view str) -> void {
				this->printSuccess(std::format("{}\n", str));
			}

			template<class... Args>
			auto printlnSuccess(std::format_string<Args...> fmt, Args&&... args) -> void {
				this->printlnSuccess(std::format(fmt, std::forward<decltype(args)>(args)...));
			}



			///////////////////////////////////
			// print Red

			auto printRed(std::string_view str) -> void;

			template<class... Args>
			auto printRed(std::format_string<Args...> fmt, Args&&... args) -> void {
				this->printRed(std::format(fmt, std::forward<decltype(args)>(args)...));
			}

			auto printlnRed(std::string_view str) -> void {
				this->printRed(std::format("{}\n", str));
			}

			template<class... Args>
			auto printlnRed(std::format_string<Args...> fmt, Args&&... args) -> void {
				this->printlnRed(std::format(fmt, std::forward<decltype(args)>(args)...));
			}


			///////////////////////////////////
			// print Yellow

			auto printYellow(std::string_view str) -> void;

			template<class... Args>
			auto printYellow(std::format_string<Args...> fmt, Args&&... args) -> void {
				this->printYellow(std::format(fmt, std::forward<decltype(args)>(args)...));
			}

			auto printlnYellow(std::string_view str) -> void {
				this->printYellow(std::format("{}\n", str));
			}

			template<class... Args>
			auto printlnYellow(std::format_string<Args...> fmt, Args&&... args) -> void {
				this->printlnYellow(std::format(fmt, std::forward<decltype(args)>(args)...));
			}


			///////////////////////////////////
			// print Green

			auto printGreen(std::string_view str) -> void;

			template<class... Args>
			auto printGreen(std::format_string<Args...> fmt, Args&&... args) -> void {
				this->printGreen(std::format(fmt, std::forward<decltype(args)>(args)...));
			}

			auto printlnGreen(std::string_view str) -> void {
				this->printGreen(std::format("{}\n", str));
			}

			template<class... Args>
			auto printlnGreen(std::format_string<Args...> fmt, Args&&... args) -> void {
				this->printlnGreen(std::format(fmt, std::forward<decltype(args)>(args)...));
			}


			///////////////////////////////////
			// print Blue

			auto printBlue(std::string_view str) -> void;

			template<class... Args>
			auto printBlue(std::format_string<Args...> fmt, Args&&... args) -> void {
				this->printBlue(std::format(fmt, std::forward<decltype(args)>(args)...));
			}

			auto printlnBlue(std::string_view str) -> void {
				this->printBlue(std::format("{}\n", str));
			}

			template<class... Args>
			auto printlnBlue(std::format_string<Args...> fmt, Args&&... args) -> void {
				this->printlnBlue(std::format(fmt, std::forward<decltype(args)>(args)...));
			}


			///////////////////////////////////
			// print Cyan

			auto printCyan(std::string_view str) -> void;

			template<class... Args>
			auto printCyan(std::format_string<Args...> fmt, Args&&... args) -> void {
				this->printCyan(std::format(fmt, std::forward<decltype(args)>(args)...));
			}

			auto printlnCyan(std::string_view str) -> void {
				this->printCyan(std::format("{}\n", str));
			}

			template<class... Args>
			auto printlnCyan(std::format_string<Args...> fmt, Args&&... args) -> void {
				this->printlnCyan(std::format(fmt, std::forward<decltype(args)>(args)...));
			}


			///////////////////////////////////
			// print Magenta

			auto printMagenta(std::string_view str) -> void;

			template<class... Args>
			auto printMagenta(std::format_string<Args...> fmt, Args&&... args) -> void {
				this->printMagenta(std::format(fmt, std::forward<decltype(args)>(args)...));
			}

			auto printlnMagenta(std::string_view str) -> void {
				this->printMagenta(std::format("{}\n", str));
			}

			template<class... Args>
			auto printlnMagenta(std::format_string<Args...> fmt, Args&&... args) -> void {
				this->printlnMagenta(std::format(fmt, std::forward<decltype(args)>(args)...));
			}


			///////////////////////////////////
			// print Gray

			auto printGray(std::string_view str) -> void;

			template<class... Args>
			auto printGray(std::format_string<Args...> fmt, Args&&... args) -> void {
				this->printGray(std::format(fmt, std::forward<decltype(args)>(args)...));
			}

			auto printlnGray(std::string_view str) -> void {
				this->printGray(std::format("{}\n", str));
			}

			template<class... Args>
			auto printlnGray(std::format_string<Args...> fmt, Args&&... args) -> void {
				this->printlnGray(std::format(fmt, std::forward<decltype(args)>(args)...));
			}

			
		private:
			Mode mode;
			std::string string;
	};


}