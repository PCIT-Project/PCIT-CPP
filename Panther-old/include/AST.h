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

#include "./Token.h"

// forward declaration
namespace pcit::panther{
	class ASTBuffer;
}

namespace pcit::panther::AST{

	enum class Kind : uint32_t {
		None, // don't use! only here to allow optimization of std::optional<Node>

		VarDecl,
		FuncDecl,
		AliasDecl,
		TypedefDecl,
		StructDecl,

		Return,
		Conditional,
		WhenConditional,
		While,
		Unreachable,

		Block,
		FuncCall,
		TemplatePack,
		TemplatedExpr,
		
		Prefix,
		Infix,
		Postfix,

		MultiAssign,

		New,

		Type,
		TypeIDConverter,

		AttributeBlock,
		Attribute,

		PrimitiveType,
		Ident,
		Intrinsic,
		Literal,
		Uninit,
		Zeroinit,
		This,
		
		Discard,
	};


	// access the internal values of this through ASTBuffer
	//    some funcs are static, but if not get the ASTBuffer from the respective Source
	class Node{
		public:
			constexpr Node(Kind node_kind, Token::ID token_id) : _kind(node_kind), _value{.token_id = token_id} {}
			constexpr Node(Kind node_kind, uint32_t node_index) : _kind(node_kind), _value{.node_index = node_index} {}

			constexpr Node(const Node& rhs) = default;

			EVO_NODISCARD constexpr auto kind() const -> Kind { return this->_kind; }

		
		private:
			Kind _kind;

			union Value{
				evo::byte dummy[1];

				Token::ID token_id;
				uint32_t node_index; // used by ASTBuffer to get the data
			} _value;


			friend ASTBuffer;
			friend std::optional<Node>;
			friend struct NodeOptInterface;
	};

	struct NodeOptInterface{
		static constexpr auto init(Node* node) -> void {
			node->_kind = Kind::None;
		}

		static constexpr auto has_value(const Node& node) -> bool {
			return node.kind() != Kind::None;
		}
	};


	static_assert(sizeof(Node) == 8, "sizeof AST::Node is different than expected");
	static_assert(std::is_trivially_copyable_v<Node>, "AST::Node is not trivially copyable");

}



namespace std{
	
	template<>
	class optional<pcit::panther::AST::Node> 
		: public pcit::core::Optional<pcit::panther::AST::Node, pcit::panther::AST::NodeOptInterface>{

		public:
			using pcit::core::Optional<pcit::panther::AST::Node, pcit::panther::AST::NodeOptInterface>::Optional;
			using pcit::core::Optional<pcit::panther::AST::Node, pcit::panther::AST::NodeOptInterface>::operator=;
	};

}


namespace pcit::panther::AST{
	
	struct VarDecl{
		enum class Kind : uint8_t {
			Var,
			Const,
			Def,
		};
		Kind kind;
		Token::ID ident;
		std::optional<Node> type;
		Node attributeBlock;
		std::optional<Node> value;
	};

	struct FuncDecl{
		struct Param{
			enum class Kind : uint8_t {
				Read,
				Mut,
				In,
			};

			Node name;
			std::optional<Node> type; // no type given if name is `this`
			Kind kind;
			Node attributeBlock;
		};

		struct Return{
			std::optional<Token::ID> ident;
			Node type;
		};

		Node name;
		std::optional<Node> templatePack;
		evo::SmallVector<Param> params;
		Node attributeBlock;
		evo::SmallVector<Return> returns;
		Node block;
	};

	struct AliasDecl{
		Token::ID ident;
		Node attributeBlock;
		Node type;
	};

	struct TypedefDecl{
		Token::ID ident;
		Node attributeBlock;
		Node type;
	};


	struct StructDecl{
		Node name;
		std::optional<Node> templatePack;
		Node attributeBlock;
		Node block;
	};


	struct Return{
		Token::ID keyword;
		std::optional<Node> label;
		evo::Variant<std::monostate, Node, Token::ID> value; // std::monostate == return; Token::ID == return...;
	};

	struct Conditional{
		Token::ID keyword;
		Node cond;
		Node thenBlock;
		std::optional<Node> elseBlock;
	};

	struct WhenConditional{
		Token::ID keyword;
		Node cond;
		Node thenBlock;
		std::optional<Node> elseBlock;
	};

	struct While{
		Token::ID keyword;
		Node cond;
		Node block;
	};

	struct Block{
		Token::ID openBrace;
		std::optional<Node> label;
		std::optional<Node> labelExplicitType; // only if .label is not nullopt
		evo::SmallVector<Node> stmts;
	};

	struct FuncCall{
		struct Arg{
			std::optional<Token::ID> label;
			Node value;
		};

		Node target;
		evo::SmallVector<Arg> args;
	};

	struct TemplatePack{
		struct Param{
			Token::ID ident;
			Node type;
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
		Node type;
		evo::SmallVector<FuncCall::Arg> args;
	};


	struct Type{
		struct Qualifier{
			bool isPtr: 1;
			bool isReadOnly: 1;
			bool isOptional: 1;

			EVO_NODISCARD auto operator==(const Qualifier& rhs) const -> bool {
				return (std::bit_cast<uint8_t>(*this) & 0b111) == (std::bit_cast<uint8_t>(rhs) & 0b111);
			}
		};
		static_assert(sizeof(Qualifier) == 1, "sizeof(AST::Type::Qualifier) != 1");

		Node base;
		evo::SmallVector<Qualifier> qualifiers;
	};

	struct TypeIDConverter{ // example: Type(@getTypeID<{Int}>())
		Node expr;
	};


	struct AttributeBlock{
		struct Attribute{
			Token::ID attribute;
			evo::SmallVector<Node, 2> args;
		};

		evo::SmallVector<Attribute> attributes;
	};

}

