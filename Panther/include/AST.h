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


	template<bool IsOptional>
	class NodeImpl{
		public:
			NodeImpl(Kind _kind, Token::ID token_id) : kind(_kind), _value{.token_id = token_id} {}
			NodeImpl(Kind _kind, uint32_t node_index) : kind(_kind), _value{.node_index = node_index} {}

			NodeImpl(const NodeImpl<false>& rhs) requires(IsOptional)
				: kind(rhs.kind), _value{.node_index = rhs._value.node_index}{}

			NodeImpl(const NodeImpl<IsOptional>&) = default;


			NodeImpl() requires(IsOptional) : kind(Kind::None), _value{} {}
			NodeImpl(std::nullopt_t) requires(IsOptional) : kind(Kind::None), _value{} {}



			EVO_NODISCARD auto getKind() const -> Kind { return this->kind; }


			EVO_NODISCARD auto hasValue() const -> bool {
				return this->kind != Kind::None;
			}

			EVO_NODISCARD auto value() const -> NodeImpl<false> requires(IsOptional) {
				evo::debugAssert(this->hasValue(), "optional node does not have value");
				return *(NodeImpl<false>*)this;
			}

		
		private:
			Kind kind;

			union Value{
				evo::byte dummy[1];

				Token::ID token_id;
				uint32_t node_index; // used by ASTBuffer to get the data
			} _value;

			static_assert(sizeof(Value) == 4);


			friend ASTBuffer;
			friend NodeImpl<false>;
			friend NodeImpl<true>;
	};

	using Node = NodeImpl<false>;
	using NodeOptional = NodeImpl<true>;

	static_assert(sizeof(Node) == 8, "sizeof AST::Node is different than expected");
	static_assert(sizeof(NodeOptional) == 8, "sizeof AST::NodeOptional is different than expected");
	static_assert(std::is_trivially_copyable_v<Node>, "AST::Node is not trivially copyable");
	static_assert(std::is_trivially_copyable_v<NodeOptional>, "AST::NodeOptional is not trivially copyable");


	struct VarDecl{
		bool isDef;
		Node ident;
		NodeOptional type;
		Node attributeBlock;
		NodeOptional value;
	};

	struct FuncDecl{
		struct Param{
			enum class Kind{
				Read,
				Mut,
				In,
			};

			Node ident;
			NodeOptional type;
			Kind kind;
			Node attributeBlock;
		};

		struct Return{
			NodeOptional ident;
			Node type;
		};

		Node ident;
		NodeOptional templatePack;
		evo::SmallVector<Param> params;
		Node attributeBlock;
		evo::SmallVector<Return> returns;
		Node block;
	};

	struct AliasDecl{
		Node ident;
		Node type;
	};


	struct Return{
		Token::ID keyword;
		NodeOptional label;
		NodeOptional value;
	};

	struct Block{
		NodeOptional label;
		evo::SmallVector<Node> stmts;
	};

	struct FuncCall{
		struct Arg{
			NodeOptional explicitIdent;
			Node value;
		};

		Node target;
		evo::SmallVector<Arg> args;
	};

	struct TemplatePack{
		struct Param{
			Node ident;
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
			Node name;
			NodeOptional arg;
		};

		evo::SmallVector<Attribute> attributes;
	};

}

