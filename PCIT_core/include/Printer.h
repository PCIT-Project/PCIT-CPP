//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>

namespace pcit::core{


	class Printer{
		public:
			Printer() noexcept; // should print colors based on platformSupportsColor (unknown defaults to no color)
			Printer(bool should_print_color) noexcept;

			~Printer();

			Printer(const Printer& rhs) noexcept;
			Printer(Printer&& rhs) noexcept;


			enum class DetectResult{
				Yes,
				No,
				Unknown,
			};
			EVO_NODISCARD static auto platformSupportsColor() noexcept -> DetectResult;


			EVO_NODISCARD auto isPrintingColor() const noexcept -> bool { return this->print_color; };


			auto print(std::string_view str) const noexcept -> void;

			auto printFatal(std::string_view str) const noexcept -> void;
			auto printError(std::string_view str) const noexcept -> void;
			auto printWarning(std::string_view str) const noexcept -> void;
			auto printInfo(std::string_view str) const noexcept -> void;
			auto printSuccess(std::string_view str) const noexcept -> void;

			auto printRed(std::string_view str) const noexcept -> void;
			auto printYellow(std::string_view str) const noexcept -> void;
			auto printGreen(std::string_view str) const noexcept -> void;
			auto printBlue(std::string_view str) const noexcept -> void;
			auto printCyan(std::string_view str) const noexcept -> void;
			auto printMagenta(std::string_view str) const noexcept -> void;
			auto printGray(std::string_view str) const noexcept -> void;

			
		private:
			bool print_color;
	};


};