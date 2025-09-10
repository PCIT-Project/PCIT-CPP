////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <filesystem>

#include <Evo.h>

#include "./Type.h"


namespace pcit::clangint{


	struct MacroExpr{
		enum class Kind : uint32_t {
			NONE,
			BOOL,
			INTEGER,
			FLOAT,
			IDENT,
		};

		struct Integer{
			uint64_t value;
			std::optional<Type> type; // nullopt if fluid
		};

		struct Float{
			float64_t value;
			std::optional<Type> type; // nullopt if fluid
		};


		EVO_NODISCARD auto kind() const -> Kind { return this->_kind; }

		private:
			MacroExpr(Kind macro_kind, uint32_t id) : _kind(macro_kind), value{.id = id} {}
			MacroExpr(Kind macro_kind, bool boolean) : _kind(macro_kind), value{.boolean = boolean} {}

			MacroExpr(Kind macro_kind) : _kind(macro_kind) {
				evo::debugAssert(macro_kind == Kind::NONE, "Incorrect kind for no value");
			}

		private:
			Kind _kind;

			union Value{
				uint32_t id;
				bool boolean;
			} value;


			friend class MacroExprBuffer;
	};

	static_assert(sizeof(MacroExpr) <= sizeof(uint64_t), "Unexpected size for MacroExpr");


	class MacroExprBuffer{
		public:
			MacroExprBuffer() = default;
			~MacroExprBuffer() = default;


			//////////////////
			// none

			EVO_NODISCARD static auto createNone() -> MacroExpr {
				return MacroExpr(MacroExpr::Kind::NONE);
			}


			//////////////////
			// bool

			EVO_NODISCARD static auto createBool(bool value) -> MacroExpr {
				return MacroExpr(MacroExpr::Kind::BOOL, value);
			}
			EVO_NODISCARD static auto getBool(MacroExpr expr) -> bool {
				evo::debugAssert(expr.kind() == MacroExpr::Kind::BOOL, "Not MacroExpr::Kind::BOOL");
				return expr.value.boolean;
			}


			//////////////////
			// integer

			EVO_NODISCARD auto createInteger(auto&&... args) -> MacroExpr {
				const uint32_t id = uint32_t(this->integers.size());
				this->integers.emplace_back(std::forward<decltype(args)>(args)...);
				return MacroExpr(MacroExpr::Kind::INTEGER, id);
			}

			EVO_NODISCARD auto getInteger(MacroExpr expr) const -> const MacroExpr::Integer& {
				evo::debugAssert(expr.kind() == MacroExpr::Kind::INTEGER, "Not MacroExpr::Kind::INTEGER");
				return this->integers[expr.value.id];
			}


			//////////////////
			// float

			EVO_NODISCARD auto createFloat(auto&&... args) -> MacroExpr {
				const uint32_t id = uint32_t(this->floats.size());
				this->floats.emplace_back(std::forward<decltype(args)>(args)...);
				return MacroExpr(MacroExpr::Kind::FLOAT, id);
			}

			EVO_NODISCARD auto getFloat(MacroExpr expr) const -> const MacroExpr::Float& {
				evo::debugAssert(expr.kind() == MacroExpr::Kind::FLOAT, "Not MacroExpr::Kind::FLOAT");
				return this->floats[expr.value.id];
			}


			//////////////////
			// ident

			EVO_NODISCARD auto createIdent(std::string&& str) -> MacroExpr {
				const uint32_t id = uint32_t(this->idents.size());
				this->idents.emplace_back(std::move(str));
				return MacroExpr(MacroExpr::Kind::IDENT, id);
			}

			EVO_NODISCARD auto getIdent(MacroExpr expr) const -> const std::string& {
				evo::debugAssert(expr.kind() == MacroExpr::Kind::IDENT, "Not MacroExpr::Kind::IDENT");
				return this->idents[expr.value.id];
			}



		private:
			evo::StepVector<MacroExpr::Integer> integers{};
			evo::StepVector<MacroExpr::Float> floats{};
			evo::StepVector<std::string> idents{};
	};



}