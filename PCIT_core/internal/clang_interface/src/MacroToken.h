////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>




namespace pcit::clangint{

	

	class MacroToken{
		public:
			enum class Kind{
				IDENT,


				///////////////////////////////////
				// literals

				LITERAL_INT,
				LITERAL_FLOAT,
				LITERAL_BOOL,
				LITERAL_STRING,
				LITERAL_WIDE_STRING,
				LITERAL_CHAR,
				LITERAL_WIDE_CHAR,


				///////////////////////////////////
				// types

				TYPE_VOID,
				TYPE_SIGNED,
				TYPE_UNSIGNED,

				TYPE_I64,

				TYPE_BOOL,
				TYPE_CHAR,

				TYPE_FLOAT,
				TYPE_DOUBLE,

				TYPE_INT,
				TYPE_LONG,
				TYPE_SHORT,


				///////////////////////////////////
				// keywords

				KEYWORD_CONST,
				KEYWORD_SIZEOF,
				KEYWORD_NULLPTR,


				///////////////////////////////////
				// operator

				// arithmetic
				PLUS,          // +
				MINUS,         // -
				ASTERISK,      // *
				FORWARD_SLASH, // /
				MOD,           // %

				// comparative
				EQUAL,              // ==
				NOT_EQUAL,          // !=
				LESS_THAN,          // <
				LESS_THAN_EQUAL,    // <=
				GREATER_THAN,       // >
				GREATER_THAN_EQUAL, // >=

				// logical
				NOT, // !
				AND, // &&
				OR,  // ||

				// bitwise
				SHIFT_LEFT,     // <<
				SHIFT_LEFT_SAT, // <<|
				SHIFT_RIGHT,    // >>
				BITWISE_AND,    // &
				BITWISE_OR,     // |
				BITWISE_XOR,    // ^
				BITWISE_NOT,    // ~

				// accessors
				ACCESSOR,      // .
				TYPE_ACCESSOR, // ::


				///////////////////////////////////
				// punctuation

				OPEN_PAREN,    // (
				CLOSE_PAREN,   // )
				OPEN_BRACKET,  // [
				CLOSE_BRACKET, // ]
				OPEN_BRACE,    // {
				CLOSE_BRACE,   // }
				COLON,         // :
				QUESTION_MARK, // ?
			};


			struct IntValue{
				enum class Type{
					FLUID,
					C_UINT,
					C_LONG,
					C_ULONG,
					C_LONG_LONG,
					C_ULONG_LONG,
					ISIZE,
					USIZE,
				};

				uint64_t value;
				Type type;
			};


			struct FloatValue{
				enum class Type{
					FLUID,
					F16,
					BF16,
					F32,
					F64,
					// F128,
					C_LONG_DOUBLE,
				};

				float64_t value;
				Type type;
			};

		public:
			MacroToken(Kind token_kind) : _kind(token_kind), value{false} {}

			MacroToken(Kind token_kind, bool bool_value) : _kind(token_kind), value{.bool_value = bool_value} {}
			MacroToken(Kind token_kind, char char_value) : _kind(token_kind), value{.char_value = char_value} {}
			MacroToken(Kind token_kind, wchar_t wide_char_value)
				: _kind(token_kind), value{.wide_char_value = wide_char_value} {}
			MacroToken(Kind token_kind, IntValue int_value) : _kind(token_kind), value{.int_value = int_value} {}
			MacroToken(Kind token_kind, FloatValue float_value) : _kind(token_kind), value{.float_value = float_value}{}
			MacroToken(Kind token_kind, std::string_view string_value)
				: _kind(token_kind), value{.string_value = string_value} {}

			~MacroToken() = default;


			EVO_NODISCARD auto getBool() const -> bool {
				evo::debugAssert(this->_kind == Kind::LITERAL_BOOL, "MacroToken does not have a bool value");
				return this->value.bool_value;
			}

			EVO_NODISCARD auto getChar() const -> char {
				evo::debugAssert(this->_kind == Kind::LITERAL_CHAR, "MacroToken does not have a char value");
				return this->value.char_value;
			}

			EVO_NODISCARD auto getWideChar() const -> wchar_t {
				evo::debugAssert(this->_kind == Kind::LITERAL_WIDE_CHAR, "MacroToken does not have a wide char value");
				return this->value.wide_char_value;
			}

			EVO_NODISCARD auto getInt() const -> IntValue {
				evo::debugAssert(this->_kind == Kind::LITERAL_INT, "MacroToken does not have a integer value");
				return this->value.int_value;
			}

			EVO_NODISCARD auto getFloat() const -> FloatValue {
				evo::debugAssert(this->_kind == Kind::LITERAL_FLOAT, "MacroToken does not have a float value");
				return this->value.float_value;
			}

			EVO_NODISCARD auto getString() const -> std::string_view {
				evo::debugAssert(
					this->_kind == Kind::LITERAL_STRING || this->_kind == Kind::IDENT, 
					"MacroToken does not have a string value"
				);

				return this->value.string_value;
			};


			EVO_NODISCARD auto kind() const -> Kind { return this->_kind; }


			EVO_NODISCARD static consteval auto lookupKind(std::string_view op_str) -> Kind {
				// length 2
				if(op_str == "=="){ return Kind::EQUAL; }
				if(op_str == "!="){ return Kind::NOT_EQUAL; }
				if(op_str == "<="){ return Kind::LESS_THAN_EQUAL; }
				if(op_str == ">="){ return Kind::GREATER_THAN_EQUAL; }

				if(op_str == "&&"){ return Kind::AND; }
				if(op_str == "||"){ return Kind::OR; }

				if(op_str == "<<"){ return Kind::SHIFT_LEFT; }
				if(op_str == ">>"){ return Kind::SHIFT_RIGHT; }

				if(op_str == "::"){ return Kind::TYPE_ACCESSOR; }



				// length 1
				if(op_str == "+"){ return Kind::PLUS; }
				if(op_str == "-"){ return Kind::MINUS; }
				if(op_str == "*"){ return Kind::ASTERISK; }
				if(op_str == "/"){ return Kind::FORWARD_SLASH; }
				if(op_str == "%"){ return Kind::MOD; }

				if(op_str == "<"){ return Kind::LESS_THAN; }
				if(op_str == ">"){ return Kind::GREATER_THAN; }

				if(op_str == "!"){ return Kind::NOT; }

				if(op_str == "&"){ return Kind::BITWISE_AND; }
				if(op_str == "|"){ return Kind::BITWISE_OR; }
				if(op_str == "^"){ return Kind::BITWISE_XOR; }
				if(op_str == "~"){ return Kind::BITWISE_NOT; }

				if(op_str == "."){ return Kind::ACCESSOR; }

				if(op_str == "("){ return Kind::OPEN_PAREN; }
				if(op_str == ")"){ return Kind::CLOSE_PAREN; }
				if(op_str == "["){ return Kind::OPEN_BRACKET; }
				if(op_str == "]"){ return Kind::CLOSE_BRACKET; }
				if(op_str == "{"){ return Kind::OPEN_BRACE; }
				if(op_str == "}"){ return Kind::CLOSE_BRACE; }
				if(op_str == ":"){ return Kind::COLON; }
				if(op_str == "?"){ return Kind::QUESTION_MARK; }

				evo::debugFatalBreak("Unknown or unsupported kind ({})", op_str);
			}


		private:
			Kind _kind;

			union Value{
				bool bool_value;
				char char_value;
				wchar_t wide_char_value;
				IntValue int_value;
				FloatValue float_value;
				std::string_view string_value;
			} value;

	};


}