//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>
#include <PCIT_core.h>

#include "./source_data.h"


namespace pcit::panther{


	class Token{
		public:
			struct ID : public core::UniqueComparableID<uint32_t, ID> { // ID lookup in TokenBuffer
				public:
					using core::UniqueComparableID<uint32_t, ID>::UniqueComparableID;
					using Iterator = IteratorImpl<ID>;
			};

			enum class Kind{
				None,

				Ident,
				Intrinsic,
				Attribute,

				// literals
				LiteralInt,
				LiteralFloat,
				LiteralBool,
				LiteralString,
				LiteralChar,

				// types
				TypeVoid,
				TypeType,
				TypeInt,

				// keywords
				KeywordVar,
				KeywordFunc,

				// operators
				Equal, // =
				RightArrow, // ->
				LessThan, // <
				GreaterThan, // >

				// punctuation
				OpenParen, // (
				CloseParen, // )
				OpenBracket, // [
				CloseBracket, // ]
				OpenBrace, // {
				CloseBrace, // }
				Comma, // ,
				SemiColon, // ;
				Colon, // :
				Pipe, // |
			};
			using enum class Kind;

			struct Location{
				uint32_t lineStart;
				uint32_t lineEnd;
				uint32_t collumnStart;
				uint32_t collumnEnd;
			};

		public:
			Token(Kind _kind, Location _location) noexcept : kind(_kind), location(_location), value(false) {};

			Token(Kind _kind, Location _location, bool val) noexcept
				: kind(_kind), location(_location), value{.boolean = val} {};

			Token(Kind _kind, Location _location, uint64_t val) noexcept
				: kind(_kind), location(_location), value{.integer = val} {};

			Token(Kind _kind, Location _location, float64_t val) noexcept
				: kind(_kind), location(_location), value{.floating_point = val} {};

			Token(Kind _kind, Location _location, std::string_view val) noexcept
				: kind(_kind), location(_location), value{.string = val} {};

			~Token() = default;


			EVO_NODISCARD auto getKind() const noexcept -> const Kind& { return this->kind; };

			EVO_NODISCARD auto getLocation() const noexcept -> const Location& { return this->location; };

			EVO_NODISCARD auto getSourceLocation(SourceID source_id) const noexcept -> SourceLocation {
				return SourceLocation(
					source_id,
					this->location.lineStart,
					this->location.lineEnd,
					this->location.collumnStart,
					this->location.collumnEnd
				);
			};


			EVO_NODISCARD auto getBool() const noexcept -> bool {
				evo::debugAssert(this->kind == Kind::LiteralBool, "Token does not have a bool value");
				return this->value.boolean;
			};

			EVO_NODISCARD auto getInt() const noexcept -> uint64_t {
				evo::debugAssert(this->kind == Kind::LiteralInt, "Token does not have a integer value");
				return this->value.integer;
			};

			EVO_NODISCARD auto getFloat() const noexcept -> float64_t {
				evo::debugAssert(this->kind == Kind::LiteralFloat, "Token does not have a float value");
				return this->value.floating_point;
			};

			EVO_NODISCARD auto getString() const noexcept -> std::string_view {
				evo::debugAssert(
					this->kind == Kind::LiteralString || this->kind == Kind::LiteralChar ||
					this->kind == Kind::Ident || this->kind == Kind::Intrinsic || this->kind == Kind::Attribute,
					"Token does not have a string value"
				);
				return this->value.string;
			};
			


			// TODO: hash-map optimization?
			EVO_NODISCARD static constexpr auto lookupKind(std::string_view op_str) noexcept -> Kind {
				// length 2
				if(op_str == "->"){ return Kind::RightArrow; }

				// length 1
				if(op_str == "="){ return Kind::Equal; }
				if(op_str == "<"){ return Kind::LessThan; }
				if(op_str == ">"){ return Kind::GreaterThan; }
				if(op_str == "("){ return Kind::OpenParen; }
				if(op_str == ")"){ return Kind::CloseParen; }
				if(op_str == "["){ return Kind::OpenBracket; }
				if(op_str == "]"){ return Kind::CloseBracket; }
				if(op_str == "{"){ return Kind::OpenBrace; }
				if(op_str == "}"){ return Kind::CloseBrace; }
				if(op_str == ","){ return Kind::Comma; }
				if(op_str == ";"){ return Kind::SemiColon; }
				if(op_str == ":"){ return Kind::Colon; }
				if(op_str == "|"){ return Kind::Pipe; }

				evo::debugFatalBreak("Uknown or unsupported kind ({})", op_str);
			};


			EVO_NODISCARD static auto printKind(Kind kind) noexcept -> std::string_view {
				switch(kind){
					break; case Kind::None:          return "None";
					break; case Kind::Ident:         return "Ident";
					break; case Kind::Intrinsic:     return "Intrinsic";
					break; case Kind::Attribute:     return "Attribute";

					// literals
					break; case Kind::LiteralInt:    return "LiteralInt";
					break; case Kind::LiteralFloat:  return "LiteralFloat";
					break; case Kind::LiteralBool:   return "LiteralBool";
					break; case Kind::LiteralString: return "LiteralString";
					break; case Kind::LiteralChar:   return "LiteralChar";

					// types
					break; case Kind::TypeVoid:      return "Void";
					break; case Kind::TypeType:      return "Type";
					break; case Kind::TypeInt:       return "Int";

					// keywords
					break; case Kind::KeywordVar:    return "var";
					break; case Kind::KeywordFunc:   return "func";

					// operators
					break; case Kind::Equal:         return "=";
					break; case Kind::RightArrow:    return "->";
					break; case Kind::LessThan:      return "<";
					break; case Kind::GreaterThan:   return ">";

					// punctuation
					break; case Kind::OpenParen:     return "(";
					break; case Kind::CloseParen:    return ")";
					break; case Kind::OpenBracket:   return "[";
					break; case Kind::CloseBracket:  return "]";
					break; case Kind::OpenBrace:     return "{";
					break; case Kind::CloseBrace:    return "}";
					break; case Kind::Comma:         return ",";
					break; case Kind::SemiColon:     return ";";
					break; case Kind::Colon:         return ":";
					break; case Kind::Pipe:          return "|";
				};

				evo::debugFatalBreak("Unknown or unsupported token kind ({})", evo::to_underlying(kind));
			};


		private:
			Kind kind;
			Location location;

			union {
				bool boolean;
				uint64_t integer;
				float64_t floating_point;
				std::string_view string;
			} value;
			
	};


};


template<>
struct std::formatter<pcit::panther::Token::Kind> : std::formatter<std::string_view> {
    auto format(const pcit::panther::Token::Kind& kind, std::format_context& ctx) {
        return std::formatter<std::string_view>::format(pcit::panther::Token::printKind(kind), ctx);
    };
};


