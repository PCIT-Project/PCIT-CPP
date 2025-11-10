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

#include "../tokens/Token.h"

// forward declaration
namespace pcit::panther{
	class ASTBuffer;
}

namespace pcit::panther::AST{

	enum class Kind : uint32_t {
		NONE, // don't use! only here to allow optimization of std::optional<Node>

		VAR_DEF,
		FUNC_DEF,
		DELETED_SPECIAL_METHOD,
		ALIAS_DEF,
		DISTINCT_ALIAS_DEF,
		STRUCT_DEF,
		INTERFACE_DEF,
		INTERFACE_IMPL,
		UNION_DEF,
		ENUM_DEF,

		RETURN,
		ERROR,
		UNREACHABLE,
		CONDITIONAL,
		WHEN_CONDITIONAL,
		WHILE,
		DEFER,
		BREAK,
		CONTINUE,
		DELETE,

		BLOCK,
		FUNC_CALL,
		INDEXER,
		TEMPLATE_PACK,
		TEMPLATED_EXPR,
		
		PREFIX,
		INFIX,
		POSTFIX,

		MULTI_ASSIGN,

		NEW,
		ARRAY_INIT_NEW,
		DESIGNATED_INIT_NEW,
		TRY_ELSE,

		ARRAY_TYPE,
		POLY_INTERFACE_REF_TYPE,
		TYPE,
		TYPEID_CONVERTER,

		ATTRIBUTE_BLOCK,
		ATTRIBUTE,

		DEDUCER,
		PRIMITIVE_TYPE,
		IDENT,
		INTRINSIC,
		LITERAL,
		UNINIT,
		ZEROINIT,
		THIS,
		
		DISCARD,
	};


	// access the internal values of this through ASTBuffer
	//    some funcs are static, but if not get the ASTBuffer from the respective Source (gotten through SourceManager)
	class Node{
		public:
			constexpr Node(Kind node_kind, Token::ID token_id) : _kind(node_kind), _value{.token_id = token_id} {}
			constexpr Node(Kind node_kind, uint32_t node_index) : _kind(node_kind), _value{.node_index = node_index} {}

			constexpr Node(const Node& rhs) = default;

			EVO_NODISCARD constexpr auto kind() const -> Kind { return this->_kind; }


			auto operator==(const Node& rhs) const -> bool {
				return this->_kind == rhs._kind && this->_value.node_index == rhs._value.node_index;
			}

		
		private:
			Kind _kind;

			union Value{
				evo::byte dummy[1];

				Token::ID token_id;
				uint32_t node_index; // used by ASTBuffer to get the data
			} _value;


			friend ASTBuffer;
			friend struct core::OptionalInterface<Node>;
			friend std::hash<Node>;
	};


	static_assert(sizeof(Node) == 8, "sizeof AST::Node is different than expected");
	static_assert(std::is_trivially_copyable_v<Node>, "AST::Node is not trivially copyable");

}



namespace pcit::core{
	
	template<>
	struct OptionalInterface<panther::AST::Node>{
		static constexpr auto init(panther::AST::Node* node) -> void {
			node->_kind = panther::AST::Kind::NONE;
		}

		static constexpr auto has_value(const panther::AST::Node& node) -> bool {
			return node.kind() != panther::AST::Kind::NONE;
		}
	};

}


namespace std{
	
	template<>
	class optional<pcit::panther::AST::Node> : public pcit::core::Optional<pcit::panther::AST::Node>{
		public:
			using pcit::core::Optional<pcit::panther::AST::Node>::Optional;
			using pcit::core::Optional<pcit::panther::AST::Node>::operator=;
	};

}


namespace pcit::panther::AST{
	
	struct VarDef{
		enum class Kind : uint8_t {
			VAR,
			CONST,
			DEF,
		};

		Kind kind;
		Token::ID ident;
		std::optional<Node> type;
		Node attributeBlock;
		std::optional<Node> value;
	};

	struct FuncDef{
		struct Param{
			enum class Kind : uint8_t {
				READ,
				MUT,
				IN,
			};

			Node name; // either identifier or `this`
			std::optional<Node> type; // no type given if ident is `this`
			Kind kind;
			Node attributeBlock;
			std::optional<Node> defaultValue;
		};

		struct Return{
			std::optional<Token::ID> ident;
			Node type;
		};

		Token::ID name; // either identifier or operator
		std::optional<Node> templatePack;
		evo::SmallVector<Param> params;
		Node attributeBlock;
		evo::SmallVector<Return> returns;
		evo::SmallVector<Return> errorReturns;
		std::optional<Node> block; // only nullopt if is an interface method with no default implementation
	};

	struct DeletedSpecialMethod{
		Token::ID memberToken;
	};

	struct AliasDef{
		Token::ID ident;
		Node attributeBlock;
		Node type;
	};

	struct DistinctAliasDef{
		Token::ID ident;
		Node attributeBlock;
		Node type;
	};


	struct StructDef{
		Token::ID ident;
		std::optional<Node> templatePack;
		Node attributeBlock;
		Node block;
	};

	struct UnionDef{
		struct Field{
			Token::ID ident;
			Node type;
		};

		Token::ID ident;
		Node attributeBlock;
		evo::SmallVector<Field> fields;
		evo::SmallVector<Node> statements;
	};

	struct EnumDef{
		struct Enumerator{
			Token::ID ident;
			std::optional<Node> value;
		};

		Token::ID ident;
		std::optional<Node> underlyingType;
		Node attributeBlock;
		evo::SmallVector<Enumerator> enumerators;
		evo::SmallVector<Node> statements;
	};


	struct InterfaceDef{
		Token::ID ident;
		Node attributeBlock;
		evo::SmallVector<Node> methods; // Nodes are all FuncDef
	};


	struct InterfaceImpl{
		struct Method{
			Token::ID method;
			evo::Variant<Token::ID, Node> value;
		};

		Node target;
		evo::SmallVector<Method> methods;
	};


	struct Return{
		Token::ID keyword;
		std::optional<Node> label;
		evo::Variant<std::monostate, Node, Token::ID> value; // std::monostate == return; Token::ID == return...;
	};

	struct Error{
		Token::ID keyword;
		evo::Variant<std::monostate, Node, Token::ID> value; // std::monostate == error; Token::ID == error...;
	};

	struct Break{
		Token::ID keyword;
		std::optional<Token::ID> label;
	};

	struct Continue{
		Token::ID keyword;
		std::optional<Token::ID> label;
	};

	struct Delete{
		Token::ID keyword;
		Node value;
	};

	struct Conditional{
		Token::ID keyword;
		Node cond;
		Node thenBlock;
		std::optional<Node> elseBlock;  // either `Block` or `Conditional`
	};

	struct WhenConditional{
		Token::ID keyword;
		Node cond;
		Node thenBlock;
		std::optional<Node> elseBlock; // either `Block` or `WhenConditional`
	};

	struct While{
		Token::ID keyword;
		Node cond;
		Node block;
	};

	struct Defer{
		Token::ID keyword;
		Node block;
	};

	struct Block{
		struct Output{
			std::optional<Token::ID> ident;
			Node typeID;
		};

		Token::ID openBrace;
		Token::ID closeBrace;
		std::optional<Token::ID> label;
		evo::SmallVector<Output> outputs; // only used if `.label` has value
		evo::SmallVector<Node> statements;
	};

	struct FuncCall{
		struct Arg{
			std::optional<Token::ID> label;
			Node value;
		};

		Node target;
		evo::SmallVector<Arg> args;
	};

	struct Indexer{
		Node target;
		evo::SmallVector<Node> indices;
		Token::ID openBracket;
	};

	struct TemplatePack{
		struct Param{
			Token::ID ident;
			Node type;
			std::optional<Node> defaultValue;
		};

		evo::SmallVector<Param> params;
	};

	struct TemplatedExpr{
		Node base;
		evo::SmallVector<Node> args;
	};

	struct Prefix{
		Token::ID opTokenID;
		Node rhs;
	};

	struct Infix{
		Node lhs;
		Token::ID opTokenID;
		Node rhs;	
	};

	struct Postfix{
		Node lhs;
		Token::ID opTokenID;
	};

	struct MultiAssign{
		Token::ID openBracketLocation;
		evo::SmallVector<Node> assigns;
		Node value;
	};

	struct New{
		Token::ID keyword;
		Node type;
		evo::SmallVector<FuncCall::Arg> args;
	};

	struct ArrayInitNew{
		Token::ID keyword;
		Node type;
		evo::SmallVector<Node> values;
	};

	struct DesignatedInitNew{
		struct MemberInit{
			Token::ID ident;
			Node expr;
		};

		Token::ID keyword;
		Node type;
		evo::SmallVector<MemberInit> memberInits;
	};

	struct TryElse{
		Node attemptExpr;
		Node exceptExpr;
		evo::SmallVector<Token::ID> exceptParams;
		Token::ID elseTokenID;
	};


	struct ArrayType{
		Token::ID openBracket;
		Node elemType;
		evo::SmallVector<std::optional<Node>> dimensions; // element is nullopt if dimension is ptr
		std::optional<Node> terminator;
		std::optional<bool> refIsMut; // only has value if is array ref
	};

	struct PolyInterfaceRefType{
		Node interface;
		bool isMut;
	};


	struct Type{
		struct Qualifier{
			bool isPtr: 1;
			bool isMut: 1;
			bool isUninit: 1;
			bool isOptional: 1;

			Qualifier(bool is_ptr, bool is_mut, bool is_uninit, bool is_optional)
				: isPtr(is_ptr), isMut(is_mut), isUninit(is_uninit), isOptional(is_optional) {
				evo::debugAssert(is_ptr || is_optional, "must be pointer xor optional");
				evo::debugAssert(is_mut || is_ptr, "mut must be a pointer");
				evo::debugAssert(is_uninit == false || is_ptr, "uninit must be a pointer");
			}

			EVO_NODISCARD auto operator==(const Qualifier& rhs) const -> bool {
				return (std::bit_cast<uint8_t>(*this) & 0b1111) == (std::bit_cast<uint8_t>(rhs) & 0b1111);
			}
		};
		static_assert(sizeof(Qualifier) == 1, "sizeof(AST::Type::Qualifier) != 1");

		Node base;
		evo::SmallVector<Qualifier> qualifiers;
	};

	struct TypeIDConverter{ // example: type(@getTypeID<{Int}>())
		Token::ID keyword;
		Node expr;
	};


	struct AttributeBlock{
		struct Attribute{
			Token::ID attribute;
			evo::StaticVector<Node, 2> args;
		};

		evo::SmallVector<Attribute> attributes;
	};

}



namespace std{
	
	template<>
	struct hash<pcit::panther::AST::Node>{
		auto operator()(const pcit::panther::AST::Node& node) const -> size_t {
			return evo::hashCombine(
				std::hash<uint32_t>{}(evo::to_underlying(node.kind())), std::hash<uint32_t>{}(node._value.node_index)
			);
		}
	};

}
