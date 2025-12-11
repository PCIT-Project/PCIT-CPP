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
				using core::UniqueID<uint32_t, ID>::UniqueID;
				using core::UniqueID<uint32_t, ID>::operator==;
			};

			enum class Kind : uint32_t {
				NONE,

				IDENT,
				INTRINSIC,
				ATTRIBUTE,
				DEDUCER,
				ANONYMOUS_DEDUCER,

				
				///////////////////////////////////
				// literals

				LITERAL_INT,
				LITERAL_FLOAT,
				LITERAL_BOOL,
				LITERAL_STRING,
				LITERAL_CHAR,

				
				///////////////////////////////////
				// types

				TYPE_VOID,
				TYPE_TYPE,
				TYPE_THIS,

				TYPE_INT,
				TYPE_ISIZE,
				TYPE_I_N,

				TYPE_UINT,
				TYPE_USIZE,
				TYPE_UI_N,

				TYPE_F16,
				TYPE_BF16,
				TYPE_F32,
				TYPE_F64,
				TYPE_F80,
				TYPE_F128,

				TYPE_BYTE,
				TYPE_BOOL,
				TYPE_CHAR,
				TYPE_RAWPTR,
				TYPE_TYPEID,

				// C compatibility
				TYPE_C_WCHAR,
				TYPE_C_SHORT,
				TYPE_C_USHORT,
				TYPE_C_INT,
				TYPE_C_UINT,
				TYPE_C_LONG,
				TYPE_C_ULONG,
				TYPE_C_LONG_LONG,
				TYPE_C_ULONG_LONG,
				TYPE_C_LONG_DOUBLE,

				
				///////////////////////////////////
				// keywords

				KEYWORD_VAR,
				KEYWORD_CONST,
				KEYWORD_DEF,
				KEYWORD_FUNC,
				KEYWORD_TYPE,
				KEYWORD_STRUCT,
				KEYWORD_INTERFACE,
				KEYWORD_IMPL,
				KEYWORD_UNION,
				KEYWORD_ENUM,

				KEYWORD_RETURN,
				KEYWORD_ERROR,
				KEYWORD_UNREACHABLE,
				KEYWORD_BREAK,
				KEYWORD_CONTINUE,

				KEYWORD_NULL,
				KEYWORD_UNINIT,
				KEYWORD_ZEROINIT,
				KEYWORD_THIS,

				KEYWORD_READ,
				KEYWORD_MUT,
				KEYWORD_IN,
				
				KEYWORD_COPY,
				KEYWORD_MOVE,
				KEYWORD_FORWARD,
				KEYWORD_NEW,
				KEYWORD_DELETE,
				KEYWORD_AS,

				KEYWORD_IF,
				KEYWORD_ELSE,
				KEYWORD_WHEN,
				KEYWORD_WHILE,
				KEYWORD_FOR,
				KEYWORD_DEFER,
				KEYWORD_ERROR_DEFER,

				KEYWORD_TRY,


				///////////////////////////////////
				// operators

				RIGHT_ARROW,            // ->
				UNDERSCORE,             // _
				ELLIPSIS,               // ...
				MUT_PTR,                // *mut


				// assignment
				ASSIGN,                // =
				ASSIGN_ADD,            // +=
				ASSIGN_ADD_WRAP,       // +%=
				ASSIGN_ADD_SAT,        // +|=
				ASSIGN_SUB,            // -=
				ASSIGN_SUB_WRAP,       // -%=
				ASSIGN_SUB_SAT,        // -|=
				ASSIGN_MUL,            // *=
				ASSIGN_MUL_WRAP,       // *%=
				ASSIGN_MUL_SAT,        // *|=
				ASSIGN_DIV,            // /=
				ASSIGN_MOD,            // %=
				ASSIGN_SHIFT_LEFT,     // <<=
				ASSIGN_SHIFT_LEFT_SAT, // <<|=
				ASSIGN_SHIFT_RIGHT,    // >>=
				ASSIGN_BITWISE_AND,    // &=
				ASSIGN_BITWISE_OR,     // |=
				ASSIGN_BITWISE_XOR,    // ^=


				// arithmetic
				PLUS,          // +
				ADD_WRAP,      // +%
				ADD_SAT,       // +|
				MINUS,         // -
				SUB_WRAP,      // -%
				SUB_SAT,       // -|
				ASTERISK,      // *
				MUL_WRAP,      // *%
				MUL_SAT,       // *|
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
				ACCESSOR,    // .
				DEREFERENCE, // .*
				UNWRAP,      // .?

				// Template
				OPEN_TEMPLATE,  // <{
				CLOSE_TEMPLATE, // }>
				
				///////////////////////////////////
				// punctuation

				OPEN_PAREN,           // (
				CLOSE_PAREN,          // )
				OPEN_BRACKET,         // [
				CLOSE_BRACKET,        // ]
				OPEN_BRACE,           // {
				CLOSE_BRACE,          // }
				COMMA,                // ,
				SEMICOLON,            // ;
				COLON,                // :
				QUESTION_MARK,        // ?
				DOUBLE_FORWARD_SLASH, // //
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
			Token(Kind token_kind, evo::float64_t val) : _kind(token_kind) { this->get_value<evo::float64_t>() = val; }

			Token(Kind token_kind, std::string_view val) : _kind(token_kind) {
				SmallStringView& value_str = this->get_value<SmallStringView>();
				value_str.ptr = val.data();
				value_str.size = uint32_t(val.size());
			}

			~Token() = default;


			EVO_NODISCARD auto kind() const -> const Kind& { return this->_kind; }


			EVO_NODISCARD auto getBool() const -> bool {
				evo::debugAssert(this->_kind == Kind::LITERAL_BOOL, "Token does not have a bool value");
				return this->get_value<bool>();
			}

			EVO_NODISCARD auto getChar() const -> char {
				evo::debugAssert(this->_kind == Kind::LITERAL_CHAR, "Token does not have a char value");
				return this->get_value<char>();
			}

			EVO_NODISCARD auto getInt() const -> uint64_t {
				evo::debugAssert(this->_kind == Kind::LITERAL_INT, "Token does not have a integer value");
				return this->get_value<uint64_t>();
			}

			EVO_NODISCARD auto getBitWidth() const -> uint32_t {
				evo::debugAssert(
					this->_kind == Kind::TYPE_I_N || this->_kind == Kind::TYPE_UI_N,
					"Token does not have a bit-width value"
				);
				return uint32_t(this->get_value<uint64_t>());
			}

			EVO_NODISCARD auto getFloat() const -> evo::float64_t {
				evo::debugAssert(this->_kind == Kind::LITERAL_FLOAT, "Token does not have a float value");
				return this->get_value<evo::float64_t>();
			}

			EVO_NODISCARD auto getString() const -> std::string_view {
				evo::debugAssert(
					this->_kind == Kind::LITERAL_STRING 
					|| this->_kind == Kind::IDENT 
					|| this->_kind == Kind::INTRINSIC
					|| this->_kind == Kind::ATTRIBUTE
					|| this->_kind == Kind::DEDUCER,
					"Token does not have a string value"
				);

				const SmallStringView& value_str = this->get_value<SmallStringView>();
				return std::string_view(value_str.ptr, value_str.size);
			};
			



			EVO_NODISCARD static consteval auto lookupKind(std::string_view op_str) -> Kind {
				// length 4
				if(op_str == "<<|="){ return Kind::ASSIGN_SHIFT_LEFT_SAT; }
				if(op_str == "*mut"){ return Kind::MUT_PTR; }

				// length 3
				if(op_str == "..."){ return Kind::ELLIPSIS; }

				if(op_str == "<<|"){ return Kind::SHIFT_LEFT_SAT; }
				if(op_str == "+%="){ return Kind::ASSIGN_ADD_WRAP; }
				if(op_str == "+|="){ return Kind::ASSIGN_ADD_SAT; }
				if(op_str == "-%="){ return Kind::ASSIGN_SUB_WRAP; }
				if(op_str == "-|="){ return Kind::ASSIGN_SUB_SAT; }
				if(op_str == "*%="){ return Kind::ASSIGN_MUL_WRAP; }
				if(op_str == "*|="){ return Kind::ASSIGN_MUL_SAT; }
				if(op_str == "<<="){ return Kind::ASSIGN_SHIFT_LEFT; }
				if(op_str == ">>="){ return Kind::ASSIGN_SHIFT_RIGHT; }


				// length 2
				if(op_str == "$$"){ return Kind::ANONYMOUS_DEDUCER; }
				if(op_str == "->"){ return Kind::RIGHT_ARROW; }

				if(op_str == "+="){ return Kind::ASSIGN_ADD; }
				if(op_str == "-="){ return Kind::ASSIGN_SUB; }
				if(op_str == "*="){ return Kind::ASSIGN_MUL; }
				if(op_str == "/="){ return Kind::ASSIGN_DIV; }
				if(op_str == "%="){ return Kind::ASSIGN_MOD; }
				if(op_str == "&="){ return Kind::ASSIGN_BITWISE_AND; }
				if(op_str == "|="){ return Kind::ASSIGN_BITWISE_OR; }
				if(op_str == "^="){ return Kind::ASSIGN_BITWISE_XOR; }

				if(op_str == "+%"){ return Kind::ADD_WRAP; }
				if(op_str == "+|"){ return Kind::ADD_SAT; }
				if(op_str == "-%"){ return Kind::SUB_WRAP; }
				if(op_str == "-|"){ return Kind::SUB_SAT; }
				if(op_str == "*%"){ return Kind::MUL_WRAP; }
				if(op_str == "*|"){ return Kind::MUL_SAT; }

				if(op_str == "=="){ return Kind::EQUAL; }
				if(op_str == "!="){ return Kind::NOT_EQUAL; }
				if(op_str == "<="){ return Kind::LESS_THAN_EQUAL; }
				if(op_str == ">="){ return Kind::GREATER_THAN_EQUAL; }

				if(op_str == "&&"){ return Kind::AND; }
				if(op_str == "||"){ return Kind::OR; }

				if(op_str == "<<"){ return Kind::SHIFT_LEFT; }
				if(op_str == ">>"){ return Kind::SHIFT_RIGHT; }

				if(op_str == ".*"){ return Kind::DEREFERENCE; }
				if(op_str == ".?"){ return Kind::UNWRAP; }

				if(op_str == "<{"){ return Kind::OPEN_TEMPLATE; }
				if(op_str == "}>"){ return Kind::CLOSE_TEMPLATE; }

				if(op_str == "//"){ return Kind::DOUBLE_FORWARD_SLASH; }



				// length 1
				if(op_str == "="){ return Kind::ASSIGN; }

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

				if(op_str == "_"){ return Kind::UNDERSCORE; }

				if(op_str == "("){ return Kind::OPEN_PAREN; }
				if(op_str == ")"){ return Kind::CLOSE_PAREN; }
				if(op_str == "["){ return Kind::OPEN_BRACKET; }
				if(op_str == "]"){ return Kind::CLOSE_BRACKET; }
				if(op_str == "{"){ return Kind::OPEN_BRACE; }
				if(op_str == "}"){ return Kind::CLOSE_BRACE; }
				if(op_str == ","){ return Kind::COMMA; }
				if(op_str == ";"){ return Kind::SEMICOLON; }
				if(op_str == ":"){ return Kind::COLON; }
				if(op_str == "?"){ return Kind::QUESTION_MARK; }

				evo::debugFatalBreak("Unknown or unsupported kind ({})", op_str);
			}


			EVO_NODISCARD static auto printKind(Kind kind) -> std::string_view {
				switch(kind){
					break; case Kind::NONE:                   return "{{NONE}}";

					break; case Kind::IDENT:                  return "IDENT";
					break; case Kind::INTRINSIC:              return "INTRINSIC";
					break; case Kind::ATTRIBUTE:              return "ATTRIBUTE";
					break; case Kind::DEDUCER:                return "DEDUCER";
					break; case Kind::ANONYMOUS_DEDUCER:      return "$$";


					///////////////////////////////////
					// literals

					break; case Kind::LITERAL_INT:            return "LITERAL_INT";
					break; case Kind::LITERAL_FLOAT:          return "LITERAL_FLOAT";
					break; case Kind::LITERAL_BOOL:           return "LITERAL_BOOL";
					break; case Kind::LITERAL_STRING:         return "LITERAL_STRING";
					break; case Kind::LITERAL_CHAR:           return "LITERAL_CHAR";



					///////////////////////////////////
					// types

					break; case Kind::TYPE_VOID:              return "Void";
					break; case Kind::TYPE_TYPE:              return "Type";
					break; case Kind::TYPE_THIS:              return "This";

					break; case Kind::TYPE_INT:               return "Int";
					break; case Kind::TYPE_ISIZE:             return "ISize";
					break; case Kind::TYPE_I_N:               return "I{n}";

					break; case Kind::TYPE_UINT:              return "UInt";
					break; case Kind::TYPE_USIZE:             return "USize";
					break; case Kind::TYPE_UI_N:              return "UI{n}";

					break; case Kind::TYPE_F16:               return "F16";
					break; case Kind::TYPE_BF16:              return "BF16";
					break; case Kind::TYPE_F32:               return "F32";
					break; case Kind::TYPE_F64:               return "F64";
					break; case Kind::TYPE_F80:               return "F80";
					break; case Kind::TYPE_F128:              return "F128";

					break; case Kind::TYPE_BYTE:              return "Byte";
					break; case Kind::TYPE_BOOL:              return "Bool";
					break; case Kind::TYPE_CHAR:              return "Char";
					break; case Kind::TYPE_RAWPTR:            return "RawPtr";
					break; case Kind::TYPE_TYPEID:            return "TypeID";

					// C compatibility
					break; case Kind::TYPE_C_WCHAR:           return "CWChar";
					break; case Kind::TYPE_C_SHORT:           return "CShort";
					break; case Kind::TYPE_C_USHORT:          return "CUShort";
					break; case Kind::TYPE_C_INT:             return "CInt";
					break; case Kind::TYPE_C_UINT:            return "CUInt";
					break; case Kind::TYPE_C_LONG:            return "CLong";
					break; case Kind::TYPE_C_ULONG:           return "CULong";
					break; case Kind::TYPE_C_LONG_LONG:       return "CLongLong";
					break; case Kind::TYPE_C_ULONG_LONG:      return "CULongLong";
					break; case Kind::TYPE_C_LONG_DOUBLE:     return "CLongDouble";


					///////////////////////////////////
					// keywords

					break; case Kind::KEYWORD_VAR:            return "var";
					break; case Kind::KEYWORD_CONST:          return "const";
					break; case Kind::KEYWORD_DEF:            return "def";
					break; case Kind::KEYWORD_FUNC:           return "func";
					break; case Kind::KEYWORD_TYPE:           return "type";
					break; case Kind::KEYWORD_STRUCT:         return "struct";
					break; case Kind::KEYWORD_INTERFACE:      return "interface";
					break; case Kind::KEYWORD_IMPL:           return "impl";
					break; case Kind::KEYWORD_UNION:          return "union";
					break; case Kind::KEYWORD_ENUM:           return "enum";

					break; case Kind::KEYWORD_RETURN:         return "return ";
					break; case Kind::KEYWORD_ERROR:          return "error";
					break; case Kind::KEYWORD_UNREACHABLE:    return "unreachable";
					break; case Kind::KEYWORD_BREAK:          return "break";
					break; case Kind::KEYWORD_CONTINUE:       return "continue";

					break; case Kind::KEYWORD_NULL:           return "null";
					break; case Kind::KEYWORD_UNINIT:         return "uninit";
					break; case Kind::KEYWORD_ZEROINIT:       return "zeroinit";
					break; case Kind::KEYWORD_THIS:           return "this";

					break; case Kind::KEYWORD_READ:           return "read";
					break; case Kind::KEYWORD_MUT:            return "mut";
					break; case Kind::KEYWORD_IN:             return "in";

					break; case Kind::KEYWORD_COPY:           return "copy";
					break; case Kind::KEYWORD_MOVE:           return "move";
					break; case Kind::KEYWORD_FORWARD:        return "forward";
					break; case Kind::KEYWORD_NEW:            return "new";
					break; case Kind::KEYWORD_DELETE:         return "delete";
					break; case Kind::KEYWORD_AS:             return "as";

					break; case Kind::KEYWORD_IF:             return "if";
					break; case Kind::KEYWORD_ELSE:           return "else";
					break; case Kind::KEYWORD_WHEN:           return "when";
					break; case Kind::KEYWORD_WHILE:          return "while";
					break; case Kind::KEYWORD_FOR:            return "for";
					break; case Kind::KEYWORD_DEFER:          return "defer";
					break; case Kind::KEYWORD_ERROR_DEFER:    return "errorDefer";

					break; case Kind::KEYWORD_TRY:            return "try";



					///////////////////////////////////
					// operators

					break; case Kind::RIGHT_ARROW:            return "->";
					break; case Kind::UNDERSCORE:             return "_";
					break; case Kind::ELLIPSIS:               return "...";
					break; case Kind::MUT_PTR:                return "*mut";


					// assignment
					break; case Kind::ASSIGN:                 return "=";
					break; case Kind::ASSIGN_ADD:             return "+=";
					break; case Kind::ASSIGN_ADD_WRAP:        return "+%=";
					break; case Kind::ASSIGN_ADD_SAT:         return "+|=";
					break; case Kind::ASSIGN_SUB:             return "-=";
					break; case Kind::ASSIGN_SUB_WRAP:        return "-%=";
					break; case Kind::ASSIGN_SUB_SAT:         return "-|=";
					break; case Kind::ASSIGN_MUL:             return "*=";
					break; case Kind::ASSIGN_MUL_WRAP:        return "*%=";
					break; case Kind::ASSIGN_MUL_SAT:         return "*|=";
					break; case Kind::ASSIGN_DIV:             return "/=";
					break; case Kind::ASSIGN_MOD:             return "%=";
					break; case Kind::ASSIGN_SHIFT_LEFT:      return "<<=";
					break; case Kind::ASSIGN_SHIFT_LEFT_SAT:  return "<<|=";
					break; case Kind::ASSIGN_SHIFT_RIGHT:     return ">>=";
					break; case Kind::ASSIGN_BITWISE_AND:     return "&=";
					break; case Kind::ASSIGN_BITWISE_OR:      return "|=";
					break; case Kind::ASSIGN_BITWISE_XOR:     return "^=";

					// arithmetic
					break; case Kind::PLUS:                   return "+";
					break; case Kind::ADD_WRAP:               return "+%";
					break; case Kind::ADD_SAT:                return "+|";
					break; case Kind::MINUS:                  return "-";
					break; case Kind::SUB_WRAP:               return "-%";
					break; case Kind::SUB_SAT:                return "-|";
					break; case Kind::ASTERISK:               return "*";
					break; case Kind::MUL_WRAP:               return "*%";
					break; case Kind::MUL_SAT:                return "*|";
					break; case Kind::FORWARD_SLASH:          return "/";
					break; case Kind::MOD:                    return "%";

					// logical
					break; case Kind::EQUAL:                  return "==";
					break; case Kind::NOT_EQUAL:              return "!=";
					break; case Kind::LESS_THAN:              return "<";
					break; case Kind::LESS_THAN_EQUAL:        return "<=";
					break; case Kind::GREATER_THAN:           return ">";
					break; case Kind::GREATER_THAN_EQUAL:     return ">=";

					// logical
					break; case Kind::NOT:                    return "!";
					break; case Kind::AND:                    return "&&";
					break; case Kind::OR:                     return "||";
					
					// bitwise
					break; case Kind::SHIFT_LEFT:             return "<<";
					break; case Kind::SHIFT_LEFT_SAT:         return "<<|";
					break; case Kind::SHIFT_RIGHT:            return ">>";
					break; case Kind::BITWISE_AND:            return "&";
					break; case Kind::BITWISE_OR:             return "|";
					break; case Kind::BITWISE_XOR:            return "^";
					break; case Kind::BITWISE_NOT:            return "~";

					// Accessors
					break; case Kind::ACCESSOR:               return ".";
					break; case Kind::DEREFERENCE:            return ".*";
					break; case Kind::UNWRAP:                 return ".?";

					// templates
					break; case Kind::OPEN_TEMPLATE:          return "<{";
					break; case Kind::CLOSE_TEMPLATE:         return "}>";



					///////////////////////////////////
					// punctuation

					break; case Kind::OPEN_PAREN:             return "(";
					break; case Kind::CLOSE_PAREN:            return ")";
					break; case Kind::OPEN_BRACKET:           return "[";
					break; case Kind::CLOSE_BRACKET:          return "]";
					break; case Kind::OPEN_BRACE:             return "{";
					break; case Kind::CLOSE_BRACE:            return "}";
					break; case Kind::COMMA:                  return ",";
					break; case Kind::SEMICOLON:              return ";";
					break; case Kind::COLON:                  return ":";
					break; case Kind::QUESTION_MARK:          return "?";
					break; case Kind::DOUBLE_FORWARD_SLASH:   return "//";
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

			template<> EVO_NODISCARD auto get_value<evo::float64_t>() const -> const evo::float64_t& {
				return *(evo::float64_t*)&this->value;
			}
			template<> EVO_NODISCARD auto get_value<evo::float64_t>()       ->       evo::float64_t& {
				return *(evo::float64_t*)&this->value;
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

}



namespace pcit::core{
	
	template<>
	struct OptionalInterface<panther::Token::ID>{
		static constexpr auto init(panther::Token::ID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::Token::ID& id) -> bool {
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
	struct hash<pcit::panther::Token::ID> {
	    EVO_NODISCARD auto operator()(pcit::panther::Token::ID id) const -> size_t {
	    	return std::hash<uint32_t>{}(id.get());
	    }
	};

	template<>
	class optional<pcit::panther::Token::ID> 
		: public pcit::core::Optional<pcit::panther::Token::ID>{

		public:
			using pcit::core::Optional<pcit::panther::Token::ID>::Optional;
			using pcit::core::Optional<pcit::panther::Token::ID>::operator=;
	};

}



