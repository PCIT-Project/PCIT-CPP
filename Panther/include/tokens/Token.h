////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>
#include <PCIT_core.h>


namespace pcit::panther{


	class Token{
		public:
			struct ID : public core::UniqueID<uint32_t, ID> { // ID lookup in TokenBuffer
				public:
					using core::UniqueID<uint32_t, ID>::UniqueID;
			};

			enum class Kind : uint32_t {
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
				TypeBF16,
				TypeF32,
				TypeF64,
				TypeF80,
				TypeF128,

				TypeByte,
				TypeBool,
				TypeChar,
				TypeRawPtr,
				TypeTypeID,

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
				KeywordConst,
				KeywordDef,
				KeywordFunc,
				KeywordAlias,
				KeywordType,
				KeywordStruct,

				KeywordReturn,
				KeywordError,
				KeywordUnreachable,

				KeywordNull,
				KeywordUninit,
				KeywordZeroinit,
				KeywordThis,

				KeywordRead,
				KeywordMut,
				KeywordIn,
				
				KeywordCopy,
				KeywordMove,
				KeywordDestructiveMove,
				KeywordForward,
				KeywordNew,
				KeywordAs,

				KeywordIf,
				KeywordElse,
				KeywordWhen,
				KeywordWhile,


				///////////////////////////////////
				// operators

				RightArrow,        // ->
				Underscore,        // _
				Ellipsis,          // ...
				ReadOnlyAddressOf, // &|


				// assignment
				Assign,             // =
				AssignAdd,          // +=
				AssignAddWrap,      // +%=
				AssignAddSat,       // +|=
				AssignSub,          // -=
				AssignSubWrap,      // -%=
				AssignSubSat,       // -|=
				AssignMul,          // *=
				AssignMulWrap,      // *%=
				AssignMulSat,       // *|=
				AssignDiv,          // /=
				AssignMod,          // %=
				AssignShiftLeft,    // <<=
				AssignShiftLeftSat, // <<|=
				AssignShiftRight,   // >>=
				AssignBitwiseAnd,   // &=
				AssignBitwiseOr,    // |=
				AssignBitwiseXOr,   // ^=


				// arithmetic
				Plus,         // +
				AddWrap,      // +%
				AddSat,       // +|
				Minus,        // -
				SubWrap,      // -%
				SubSat,       // -|
				Asterisk,     // *
				MulWrap,      // *%
				MulSat,       // *|
				ForwardSlash, // /
				DoubleForwardSlash, // //
				Mod,          // %

				// comparative
				Equal,            // ==
				NotEqual,         // !=
				LessThan,         // <
				LessThanEqual,    // <=
				GreaterThan,      // >
				GreaterThanEqual, // >=

				// logical
				Not, // !
				And, // &&
				Or,  // ||

				// bitwise
				ShiftLeft,    // <<
				ShiftLeftSat, // <<|
				ShiftRight,   // >>
				BitwiseAnd,   // &
				BitwiseOr,    // |
				BitwiseXOr,   // ^
				BitwiseNot,   // ~

				// accessors
				Accessor,    // .
				Dereference, // .*
				Unwrap,      // .?

				// Template
				OpenTemplate,  // <{
				CloseTemplate, // }>
				
				///////////////////////////////////
				// punctuation

				OpenParen,    // (
				CloseParen,   // )
				OpenBracket,  // [
				CloseBracket, // ]
				OpenBrace,    // {
				CloseBrace,   // }
				Comma,        // ,
				SemiColon,    // ;
				Colon,        // :
				QuestionMark, // ?
			};

			struct Location{
				uint32_t lineStart;
				uint32_t lineEnd;
				uint32_t collumnStart;
				uint32_t collumnEnd;
			};


			struct SmallStringView{
				const char* ptr;
				uint32_t size;
			};

		public:
			Token(Kind token_kind) : _kind(token_kind) {}

			Token(Kind token_kind, bool val) : _kind(token_kind) { this->get_value<bool>() = val; }
			Token(Kind token_kind, char val) : _kind(token_kind) { this->get_value<char>() = val; }
			Token(Kind token_kind, uint64_t val) : _kind(token_kind) { this->get_value<uint64_t>() = val; }
			Token(Kind token_kind, float64_t val) : _kind(token_kind) { this->get_value<float64_t>() = val; }

			Token(Kind token_kind, std::string_view val) : _kind(token_kind) {
				SmallStringView& value_str = this->get_value<SmallStringView>();
				value_str.ptr = val.data();
				value_str.size = uint32_t(val.size());
			}

			~Token() = default;


			EVO_NODISCARD auto kind() const -> const Kind& { return this->_kind; }


			EVO_NODISCARD auto getBool() const -> bool {
				evo::debugAssert(this->_kind == Kind::LiteralBool, "Token does not have a bool value");
				return this->get_value<bool>();
			}

			EVO_NODISCARD auto getChar() const -> char {
				evo::debugAssert(this->_kind == Kind::LiteralChar, "Token does not have a char value");
				return this->get_value<char>();
			}

			EVO_NODISCARD auto getInt() const -> uint64_t {
				evo::debugAssert(this->_kind == Kind::LiteralInt, "Token does not have a integer value");
				return this->get_value<uint64_t>();
			}

			EVO_NODISCARD auto getBitWidth() const -> uint32_t {
				evo::debugAssert(
					this->_kind == Kind::TypeI_N || this->_kind == Kind::TypeUI_N,
					"Token does not have a bit-width value"
				);
				return uint32_t(this->get_value<uint64_t>());
			}

			EVO_NODISCARD auto getFloat() const -> float64_t {
				evo::debugAssert(this->_kind == Kind::LiteralFloat, "Token does not have a float value");
				return this->get_value<float64_t>();
			}

			EVO_NODISCARD auto getString() const -> std::string_view {
				evo::debugAssert(
					this->_kind == Kind::LiteralString || this->_kind == Kind::LiteralChar ||
					this->_kind == Kind::Ident || this->_kind == Kind::Intrinsic || this->_kind == Kind::Attribute,
					"Token does not have a string value"
				);

				const SmallStringView& value_str = this->get_value<SmallStringView>();
				return std::string_view(value_str.ptr, value_str.size);
			};
			



			EVO_NODISCARD static consteval auto lookupKind(std::string_view op_str) -> Kind {
				// length 4
				if(op_str == "<<|="){ return Kind::AssignShiftLeftSat; }

				// length 3
				if(op_str == "..."){ return Kind::Ellipsis; }

				if(op_str == "<<|"){ return Kind::ShiftLeftSat; }
				if(op_str == "+%="){ return Kind::AssignAddWrap; }
				if(op_str == "+|="){ return Kind::AssignAddSat; }
				if(op_str == "-%="){ return Kind::AssignSubWrap; }
				if(op_str == "-|="){ return Kind::AssignSubSat; }
				if(op_str == "*%="){ return Kind::AssignMulWrap; }
				if(op_str == "*|="){ return Kind::AssignMulSat; }
				if(op_str == "<<="){ return Kind::AssignShiftLeft; }
				if(op_str == ">>="){ return Kind::AssignShiftRight; }


				// length 2
				if(op_str == "->"){ return Kind::RightArrow; }
				if(op_str == "&|"){ return Kind::ReadOnlyAddressOf; }

				if(op_str == "+="){ return Kind::AssignAdd; }
				if(op_str == "-="){ return Kind::AssignSub; }
				if(op_str == "*="){ return Kind::AssignMul; }
				if(op_str == "/="){ return Kind::AssignDiv; }
				if(op_str == "%="){ return Kind::AssignMod; }
				if(op_str == "&="){ return Kind::AssignBitwiseAnd; }
				if(op_str == "|="){ return Kind::AssignBitwiseOr; }
				if(op_str == "^="){ return Kind::AssignBitwiseXOr; }

				if(op_str == "+%"){ return Kind::AddWrap; }
				if(op_str == "+|"){ return Kind::AddSat; }
				if(op_str == "-%"){ return Kind::SubWrap; }
				if(op_str == "-|"){ return Kind::SubSat; }
				if(op_str == "*%"){ return Kind::MulWrap; }
				if(op_str == "*|"){ return Kind::MulSat; }
				if(op_str == "//"){ return Kind::DoubleForwardSlash; }

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

				if(op_str == "<{"){ return Kind::OpenTemplate; }
				if(op_str == "}>"){ return Kind::CloseTemplate; }



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

				if(op_str == "_"){ return Kind::Underscore; }

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
			}


			EVO_NODISCARD static auto printKind(Kind kind) -> std::string_view {
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

					break; case Kind::TypeVoid:           return "Void";
					break; case Kind::TypeType:           return "Type";
					break; case Kind::TypeThis:           return "This";

					break; case Kind::TypeInt:            return "Int";
					break; case Kind::TypeISize:          return "ISize";
					break; case Kind::TypeI_N:            return "I{n}";

					break; case Kind::TypeUInt:           return "UInt";
					break; case Kind::TypeUSize:          return "USize";
					break; case Kind::TypeUI_N:           return "UI{n}";

					break; case Kind::TypeF16:            return "F16";
					break; case Kind::TypeBF16:           return "BF16";
					break; case Kind::TypeF32:            return "F32";
					break; case Kind::TypeF64:            return "F64";
					break; case Kind::TypeF80:            return "F80";
					break; case Kind::TypeF128:           return "F128";

					break; case Kind::TypeByte:           return "Byte";
					break; case Kind::TypeBool:           return "Bool";
					break; case Kind::TypeChar:           return "Char";
					break; case Kind::TypeRawPtr:         return "RawPtr";
					break; case Kind::TypeTypeID:         return "TypeID";

					// C compatibility
					break; case Kind::TypeCShort:         return "CShort";
					break; case Kind::TypeCUShort:        return "CUShort";
					break; case Kind::TypeCInt:           return "CInt";
					break; case Kind::TypeCUInt:          return "CUInt";
					break; case Kind::TypeCLong:          return "CLong";
					break; case Kind::TypeCULong:         return "CULong";
					break; case Kind::TypeCLongLong:      return "CLongLong";
					break; case Kind::TypeCULongLong:     return "CULongLong";
					break; case Kind::TypeCLongDouble:    return "CLongDouble";


					///////////////////////////////////
					// keywords

					break; case Kind::KeywordVar:         return "var";
					break; case Kind::KeywordConst:       return "const";
					break; case Kind::KeywordDef:         return "def";
					break; case Kind::KeywordFunc:        return "func";
					break; case Kind::KeywordAlias:       return "alias";
					break; case Kind::KeywordType:        return "type";
					break; case Kind::KeywordStruct:      return "struct";

					break; case Kind::KeywordReturn:      return "return";
					break; case Kind::KeywordError:       return "error";
					break; case Kind::KeywordUnreachable: return "unreachable";

					break; case Kind::KeywordNull:        return "null";
					break; case Kind::KeywordUninit:      return "uninit";
					break; case Kind::KeywordZeroinit:    return "zeroinit";
					break; case Kind::KeywordThis:        return "this";

					break; case Kind::KeywordRead:        return "read";
					break; case Kind::KeywordMut:         return "mut";
					break; case Kind::KeywordIn:          return "in";

					break; case Kind::KeywordCopy:        return "copy";
					break; case Kind::KeywordMove:        return "move";
					break; case Kind::KeywordNew:         return "new";
					break; case Kind::KeywordAs:          return "as";

					break; case Kind::KeywordIf:          return "if";
					break; case Kind::KeywordElse:        return "else";
					break; case Kind::KeywordWhen:        return "when";
					break; case Kind::KeywordWhile:       return "while";



					///////////////////////////////////
					// operators

					break; case Kind::RightArrow:         return "->";
					break; case Kind::Underscore:         return "_";
					break; case Kind::Ellipsis:           return "...";
					break; case Kind::ReadOnlyAddressOf:  return "&|";


					// assignment
					break; case Kind::Assign:             return "=";
					break; case Kind::AssignAdd:          return "+=";
					break; case Kind::AssignAddWrap:      return "+%=";
					break; case Kind::AssignAddSat:       return "+|=";
					break; case Kind::AssignSub:          return "-=";
					break; case Kind::AssignSubWrap:      return "-%=";
					break; case Kind::AssignSubSat:       return "-|=";
					break; case Kind::AssignMul:          return "*=";
					break; case Kind::AssignMulWrap:      return "*%=";
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
					break; case Kind::AddWrap:            return "+%";
					break; case Kind::AddSat:             return "+|";
					break; case Kind::Minus:              return "-";
					break; case Kind::SubWrap:            return "-%";
					break; case Kind::SubSat:             return "-|";
					break; case Kind::Asterisk:           return "*";
					break; case Kind::MulWrap:            return "*%";
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

					// templates
					break; case Kind::OpenTemplate:       return "<{";
					break; case Kind::CloseTemplate:      return "}>";



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
				}

				evo::debugFatalBreak("Unknown or unsupported token kind ({})", evo::to_underlying(kind));
			}


		private:
			template<class T> EVO_NODISCARD auto get_value() const -> const T&;
			template<class T> EVO_NODISCARD auto get_value()       ->       T&;

			template<> EVO_NODISCARD auto get_value<bool>() const -> const bool& { return *(bool*)&this->value; }
			template<> EVO_NODISCARD auto get_value<bool>()       ->       bool& { return *(bool*)&this->value; }

			template<> EVO_NODISCARD auto get_value<char>() const -> const char& { return *(char*)&this->value; }
			template<> EVO_NODISCARD auto get_value<char>()       ->       char& { return *(char*)&this->value; }

			template<> EVO_NODISCARD auto get_value<uint64_t>() const -> const uint64_t& {
				return *(uint64_t*)&this->value;
			}
			template<> EVO_NODISCARD auto get_value<uint64_t>()       ->       uint64_t& {
				return *(uint64_t*)&this->value;
			}

			template<> EVO_NODISCARD auto get_value<float64_t>() const -> const float64_t& {
				return *(float64_t*)&this->value;
			}
			template<> EVO_NODISCARD auto get_value<float64_t>()       ->       float64_t& {
				return *(float64_t*)&this->value;
			}

			template<> EVO_NODISCARD auto get_value<SmallStringView>() const -> const SmallStringView& {
				return *(SmallStringView*)&this->value;
			}
			template<> EVO_NODISCARD auto get_value<SmallStringView>()       ->       SmallStringView& {
				return *(SmallStringView*)&this->value;
			}


		private:
			Kind _kind;
			evo::byte value[12];
	};

	static_assert(sizeof(Token) == 16, "sizeof pcit::panther::Token is different than expected");
	static_assert(sizeof(Token::Location) == 16, "sizeof pcit::panther::Token::Location is different than expected");


	struct TokenIDOptInterface{
		static constexpr auto init(Token::ID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const Token::ID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

}


namespace std{

	template<>
	struct formatter<pcit::panther::Token::Kind> : formatter<string_view> {
		using Token = pcit::panther::Token;

	    auto format(const Token::Kind& kind, format_context& ctx) const -> format_context::iterator {
	        return formatter<string_view>::format(Token::printKind(kind), ctx);
	    }
	};


	template<>
	class optional<pcit::panther::Token::ID> 
		: public pcit::core::Optional<pcit::panther::Token::ID, pcit::panther::TokenIDOptInterface>{

		public:
			using pcit::core::Optional<pcit::panther::Token::ID, pcit::panther::TokenIDOptInterface>::Optional;
			using pcit::core::Optional<pcit::panther::Token::ID, pcit::panther::TokenIDOptInterface>::operator=;
	};

	
}



