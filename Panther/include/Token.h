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

				
				///////////////////////////////////
				// literals

				LiteralInt,
				LiteralFloat,
				LiteralBool,
				LiteralString,
				LiteralChar,

				
				///////////////////////////////////
				// types

				TypeVoid,
				TypeType,
				TypeThis,

				TypeInt,
				TypeISize,
				TypeI_N,

				TypeUInt,
				TypeUSize,
				TypeUI_N,

				TypeF16,
				TypeF32,
				TypeF64,
				TypeF80,
				TypeF128,

				TypeByte,
				TypeBool,
				TypeChar,
				TypeRawPtr,

				// C compatibility
				TypeCShort,
				TypeCUShort,
				TypeCInt,
				TypeCUInt,
				TypeCLong,
				TypeCULong,
				TypeCLongLong,
				TypeCULongLong,
				TypeCLongDouble,
				
				///////////////////////////////////
				// keywords

				KeywordVar,
				KeywordFunc,

				KeywordNull,
				KeywordUninit,
				KeywordThis,

				KeywordRead,
				KeywordMut,
				KeywordIn,
				
				// KeywordAddr,
				KeywordCopy,
				KeywordMove,
				KeywordAs,


				///////////////////////////////////
				// operators

				RightArrow, // ->

				// assignment
				Assign, // =
				AssignAdd, // +=
				AssignAddWrap, // +@=
				AssignAddSat, // +|=
				AssignSub, // -=
				AssignSubWrap, // -@=
				AssignSubSat, // -|=
				AssignMul, // *=
				AssignMulWrap, // *@=
				AssignMulSat, // *|=
				AssignDiv, // /=
				AssignMod, // %=
				AssignShiftLeft, // <<=
				AssignShiftLeftSat, // <<|=
				AssignShiftRight, // >>=
				AssignBitwiseAnd, // &=
				AssignBitwiseOr, // |=
				AssignBitwiseXOr, // ^=


				// arithmetic
				Plus, // +
				AddWrap, // +@
				AddSat, // +|
				Minus, // -
				SubWrap, // -@
				SubSat, // -|
				Asterisk, // *
				MulWrap, // *@
				MulSat, // *|
				ForwardSlash, // /
				Mod, // %

				// comparative
				Equal, // ==
				NotEqual, // !=
				LessThan, // <
				LessThanEqual, // <=
				GreaterThan, // >
				GreaterThanEqual, // >=

				// logical
				Not, // !
				And, // &&
				Or, // ||

				// bitwise
				ShiftLeft, // <<
				ShiftLeftSat, // <<|
				ShiftRight, // >>
				BitwiseAnd, // &
				BitwiseOr, // |
				BitwiseXOr, // ^
				BitwiseNot, // ~

				// accessors
				Accessor, // .
				Dereference, // .*
				Unwrap, // .?
				
				///////////////////////////////////
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
				QuestionMark, // ?
			};

			struct Location{
				uint32_t lineStart;
				uint32_t lineEnd;
				uint16_t collumnStart;
				uint16_t collumnEnd;
			};

		public:
			Token(Kind _kind, Location _location) noexcept : kind(_kind), location(_location), value(false) {};

			Token(Kind _kind, Location _location, bool val) noexcept
				: kind(_kind), location(_location), value{.boolean = val} {};

			Token(Kind _kind, Location _location, uint64_t val) noexcept
				: kind(_kind), location(_location), value{.integer = val} {};

			Token(Kind _kind, Location _location, float64_t val) noexcept
				: kind(_kind), location(_location), value{.floating_point = val} {};

			Token(Kind _kind, Location _location, const class Source& data, std::string_view val) noexcept;

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

			EVO_NODISCARD auto getBitWidth() const noexcept -> uint64_t {
				evo::debugAssert(
					this->kind == Kind::TypeI_N || this->kind == Kind::TypeUI_N,
					"Token does not have a bit-width value"
				);
				return this->value.integer;
			};

			EVO_NODISCARD auto getFloat() const noexcept -> float64_t {
				evo::debugAssert(this->kind == Kind::LiteralFloat, "Token does not have a float value");
				return this->value.floating_point;
			};

			EVO_NODISCARD auto getString(const class Source& source) const noexcept -> std::string_view;
			



			// TODO: hash-map optimization?
			EVO_NODISCARD static consteval auto lookupKind(std::string_view op_str) noexcept -> Kind {
				// length 4
				if(op_str == "<<|="){ return Kind::AssignShiftLeftSat; }

				// length 3
				if(op_str == "<<|"){ return Kind::ShiftLeftSat; }
				if(op_str == "+@="){ return Kind::AssignAddWrap; }
				if(op_str == "+|="){ return Kind::AssignAddSat; }
				if(op_str == "-@="){ return Kind::AssignSubWrap; }
				if(op_str == "-|="){ return Kind::AssignSubSat; }
				if(op_str == "*@="){ return Kind::AssignMulWrap; }
				if(op_str == "*|="){ return Kind::AssignMulSat; }
				if(op_str == "<<="){ return Kind::AssignShiftLeft; }
				if(op_str == ">>="){ return Kind::AssignShiftRight; }


				// length 2
				if(op_str == "->"){ return Kind::RightArrow; }

				if(op_str == "+="){ return Kind::AssignAdd; }
				if(op_str == "-="){ return Kind::AssignSub; }
				if(op_str == "*="){ return Kind::AssignMul; }
				if(op_str == "/="){ return Kind::AssignDiv; }
				if(op_str == "%="){ return Kind::AssignMod; }
				if(op_str == "&="){ return Kind::AssignBitwiseAnd; }
				if(op_str == "|="){ return Kind::AssignBitwiseOr; }
				if(op_str == "^="){ return Kind::AssignBitwiseXOr; }

				if(op_str == "+@"){ return Kind::AddWrap; }
				if(op_str == "+|"){ return Kind::AddSat; }
				if(op_str == "-@"){ return Kind::SubWrap; }
				if(op_str == "-|"){ return Kind::SubSat; }
				if(op_str == "*@"){ return Kind::MulWrap; }
				if(op_str == "*|"){ return Kind::MulSat; }

				if(op_str == "=="){ return Kind::Equal; }
				if(op_str == "!="){ return Kind::NotEqual; }
				if(op_str == "<="){ return Kind::LessThanEqual; }
				if(op_str == ">="){ return Kind::GreaterThanEqual; }

				if(op_str == "&&"){ return Kind::And; }
				if(op_str == "||"){ return Kind::Or; }

				if(op_str == "<<"){ return Kind::ShiftLeft; }
				if(op_str == ">>"){ return Kind::ShiftRight; }

				if(op_str == ".*"){ return Kind::Dereference; }
				if(op_str == ".?"){ return Kind::Unwrap; }


				// length 1
				if(op_str == "="){ return Kind::Assign; }

				if(op_str == "+"){ return Kind::Plus; }
				if(op_str == "-"){ return Kind::Minus; }
				if(op_str == "*"){ return Kind::Asterisk; }
				if(op_str == "/"){ return Kind::ForwardSlash; }
				if(op_str == "%"){ return Kind::Mod; }

				if(op_str == "<"){ return Kind::LessThan; }
				if(op_str == ">"){ return Kind::GreaterThan; }

				if(op_str == "!"){ return Kind::Not; }

				if(op_str == "&"){ return Kind::BitwiseAnd; }
				if(op_str == "|"){ return Kind::BitwiseOr; }
				if(op_str == "^"){ return Kind::BitwiseXOr; }
				if(op_str == "~"){ return Kind::BitwiseNot; }

				if(op_str == "."){ return Kind::Accessor; }

				if(op_str == "("){ return Kind::OpenParen; }
				if(op_str == ")"){ return Kind::CloseParen; }
				if(op_str == "["){ return Kind::OpenBracket; }
				if(op_str == "]"){ return Kind::CloseBracket; }
				if(op_str == "{"){ return Kind::OpenBrace; }
				if(op_str == "}"){ return Kind::CloseBrace; }
				if(op_str == ","){ return Kind::Comma; }
				if(op_str == ";"){ return Kind::SemiColon; }
				if(op_str == ":"){ return Kind::Colon; }
				if(op_str == "?"){ return Kind::QuestionMark; }

				evo::debugFatalBreak("Unknown or unsupported kind ({})", op_str);
			};


			EVO_NODISCARD static auto printKind(Kind kind) noexcept -> std::string_view {
				switch(kind){
					break; case Kind::None:               return "None";

					break; case Kind::Ident:              return "Ident";
					break; case Kind::Intrinsic:          return "Intrinsic";
					break; case Kind::Attribute:          return "Attribute";


					///////////////////////////////////
					// literals

					break; case Kind::LiteralInt:         return "LiteralInt";
					break; case Kind::LiteralFloat:       return "LiteralFloat";
					break; case Kind::LiteralBool:        return "LiteralBool";
					break; case Kind::LiteralString:      return "LiteralString";
					break; case Kind::LiteralChar:        return "LiteralChar";



					///////////////////////////////////
					// types

					break; case Kind::TypeVoid: return "Void";
					break; case Kind::TypeType: return "Type";
					break; case Kind::TypeThis: return "This";

					break; case Kind::TypeInt: return "Int";
					break; case Kind::TypeISize: return "ISize";
					break; case Kind::TypeI_N: return "I_n";

					break; case Kind::TypeUInt: return "UInt";
					break; case Kind::TypeUSize: return "USize";
					break; case Kind::TypeUI_N: return "UI_n";

					break; case Kind::TypeF16: return "F16";
					break; case Kind::TypeF32: return "F32";
					break; case Kind::TypeF64: return "F64";
					break; case Kind::TypeF80: return "F80";
					break; case Kind::TypeF128: return "F128";

					break; case Kind::TypeByte: return "Byte";
					break; case Kind::TypeBool: return "Bool";
					break; case Kind::TypeChar: return "Char";
					break; case Kind::TypeRawPtr: return "RawPtr";

					// C compatibility
					break; case Kind::TypeCShort: return "CShort";
					break; case Kind::TypeCUShort: return "CUShort";
					break; case Kind::TypeCInt: return "CInt";
					break; case Kind::TypeCUInt: return "CUInt";
					break; case Kind::TypeCLong: return "CLong";
					break; case Kind::TypeCULong: return "CULong";
					break; case Kind::TypeCLongLong: return "CLongLong";
					break; case Kind::TypeCULongLong: return "CULongLong";
					break; case Kind::TypeCLongDouble: return "CLongDouble";


					///////////////////////////////////
					// keywords

					break; case Kind::KeywordVar:         return "var";
					break; case Kind::KeywordFunc:        return "func";

					break; case Kind::KeywordNull:        return "null";
					break; case Kind::KeywordUninit:      return "uninit";
					break; case Kind::KeywordThis:        return "this";

					break; case Kind::KeywordRead:        return "read";
					break; case Kind::KeywordMut:         return "mut";
					break; case Kind::KeywordIn:          return "in";

					// break; case Kind::KeywordAddr:     return "addr";
					break; case Kind::KeywordCopy:        return "copy";
					break; case Kind::KeywordMove:        return "move";
					break; case Kind::KeywordAs:          return "as";


					///////////////////////////////////
					// operators

					break; case Kind::RightArrow:         return "->";

					// assignment
					break; case Kind::Assign:             return "=";
					break; case Kind::AssignAdd:          return "+=";
					break; case Kind::AssignAddWrap:      return "+@=";
					break; case Kind::AssignAddSat:       return "+|=";
					break; case Kind::AssignSub:          return "-=";
					break; case Kind::AssignSubWrap:      return "-@=";
					break; case Kind::AssignSubSat:       return "-|=";
					break; case Kind::AssignMul:          return "*=";
					break; case Kind::AssignMulWrap:      return "*@=";
					break; case Kind::AssignMulSat:       return "*|=";
					break; case Kind::AssignDiv:          return "/=";
					break; case Kind::AssignMod:          return "%=";
					break; case Kind::AssignShiftLeft:    return "<<=";
					break; case Kind::AssignShiftLeftSat: return "<<|=";
					break; case Kind::AssignShiftRight:   return ">>=";
					break; case Kind::AssignBitwiseAnd:   return "&=";
					break; case Kind::AssignBitwiseOr:    return "|=";
					break; case Kind::AssignBitwiseXOr:   return "^=";

					// arithmetic
					break; case Kind::Plus:               return "+";
					break; case Kind::AddWrap:            return "+@";
					break; case Kind::AddSat:             return "+|";
					break; case Kind::Minus:              return "-";
					break; case Kind::SubWrap:            return "-@";
					break; case Kind::SubSat:             return "-|";
					break; case Kind::Asterisk:           return "*";
					break; case Kind::MulWrap:            return "*@";
					break; case Kind::MulSat:             return "*|";
					break; case Kind::ForwardSlash:       return "/";
					break; case Kind::Mod:                return "%";

					// logical
					break; case Kind::Equal:              return "==";
					break; case Kind::NotEqual:           return "!=";
					break; case Kind::LessThan:           return "<";
					break; case Kind::LessThanEqual:      return "<=";
					break; case Kind::GreaterThan:        return ">";
					break; case Kind::GreaterThanEqual:   return ">=";

					// logical
					break; case Kind::Not:                return "!";
					break; case Kind::And:                return "&&";
					break; case Kind::Or:                 return "||";
					
					// bitwise
					break; case Kind::ShiftLeft:          return "<<";
					break; case Kind::ShiftLeftSat:       return "<<|";
					break; case Kind::ShiftRight:         return ">>";
					break; case Kind::BitwiseAnd:         return "&";
					break; case Kind::BitwiseOr:          return "|";
					break; case Kind::BitwiseXOr:         return "^";
					break; case Kind::BitwiseNot:         return "~";

					// Accessors
					break; case Kind::Accessor:           return ".";
					break; case Kind::Dereference:        return ".*";
					break; case Kind::Unwrap:             return ".?";


					///////////////////////////////////
					// punctuation

					break; case Kind::OpenParen:          return "(";
					break; case Kind::CloseParen:         return ")";
					break; case Kind::OpenBracket:        return "[";
					break; case Kind::CloseBracket:       return "]";
					break; case Kind::OpenBrace:          return "{";
					break; case Kind::CloseBrace:         return "}";
					break; case Kind::Comma:              return ",";
					break; case Kind::SemiColon:          return ";";
					break; case Kind::Colon:              return ":";
					break; case Kind::QuestionMark:       return "?";
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

				struct /* string */ {
					uint32_t index;
					uint32_t length;
				} string;
			} value;
	};



	static_assert(sizeof(Token) == 24);

};


template<>
struct std::formatter<pcit::panther::Token::Kind> : std::formatter<std::string_view> {
    auto format(const pcit::panther::Token::Kind& kind, std::format_context& ctx) {
        return std::formatter<std::string_view>::format(pcit::panther::Token::printKind(kind), ctx);
    };
};


