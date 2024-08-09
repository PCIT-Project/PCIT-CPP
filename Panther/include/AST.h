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

#include "./Token.h"

// forward declaration
namespace pcit::panther{
	class ASTBuffer;
}

namespace pcit::panther::AST{

	enum class Kind{
		None, // only allowed for OptionalNode

		VarDecl,
		FuncDecl,
		AliasDecl,

		Return,

		Block,
		FuncCall,
		TemplatePack,
		TemplatedExpr,
		
		Prefix,
		Infix,
		Postfix,

		MultiAssign,

		Type,

		AttributeBlock,
		Attribute,

		BuiltinType,
		Ident,
		Intrinsic,
		Literal,
		Uninit,
		This,
		
		Discard,
	};


	// access the internal values of this through ASTBuffer
	//    some funcs are static, but if not get the ASTBuffer from the respective Source
	class Node{
		public:
			constexpr Node(Kind _kind, Token::ID token_id) : kind(_kind), _value{.token_id = token_id} {}
			constexpr Node(Kind _kind, uint32_t node_index) : kind(_kind), _value{.node_index = node_index} {}

			constexpr Node(const Node& rhs) = default;

			EVO_NODISCARD constexpr auto getKind() const -> Kind { return this->kind; }

		
		private:
			Kind kind;

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
			node->kind = Kind::None;
		}

		static constexpr auto has_value(const Node& node) -> bool {
			return node.getKind() != Kind::None;
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
		bool isDef;
		Token::ID ident;
		std::optional<Node> type;
		Node attributeBlock;
		std::optional<Node> value;
	};

	struct FuncDecl{
		struct Param{
			enum class Kind{
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
		Node type;
	};


	struct Return{
		Token::ID keyword;
		std::optional<Node> label;
		std::optional<Node> value;
	};

	struct Block{
		std::optional<Node> label;
		evo::SmallVector<Node> stmts;
	};

	struct FuncCall{
		struct Arg{
			std::optional<Node> explicitIdent;
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
		evo::SmallVector<Node> assigns;
		Node value;
	};


	struct Type{
		struct Qualifier{
			bool isPtr;
			bool isReadOnly;
			bool isOptional;
		};

		Node base;
		evo::SmallVector<Qualifier> qualifiers;
	};


	struct AttributeBlock{
		struct Attribute{
			Token::ID attribute;
			std::optional<Node> arg;
		};

		evo::SmallVector<Attribute> attributes;
	};

}

