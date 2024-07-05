//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once

#include <deque>

#include <Evo.h>
#include <PCIT_core.h>

#include "./Token.h"

// forward declaration
namespace pcit::panther{
	class ASTBuffer;
};

namespace pcit::panther::AST{

	enum class Kind{
		None, // only allowed for OptionalNode

		VarDecl,
		FuncDecl,

		Type,
		Block,

		BuiltinType,
		Ident,
		Literal,
	};


	template<bool IsOptional>
	class NodeImpl{
		public:
			NodeImpl(Kind _kind, Token::ID token_id) noexcept : kind(_kind), value{.token_id = token_id} {};
			NodeImpl(Kind _kind, uint32_t node_index) noexcept : kind(_kind), value{.node_index = node_index} {};

			NodeImpl(const NodeImpl<false>& rhs) noexcept requires(IsOptional)
				: kind(rhs.kind), value{.node_index = rhs.value.node_index}{};

			NodeImpl(const NodeImpl<IsOptional>&) = default;


			NodeImpl() noexcept requires(IsOptional) : kind(Kind::None), value{} {};
			NodeImpl(std::nullopt_t) noexcept requires(IsOptional) : kind(Kind::None), value{} {};



			EVO_NODISCARD auto getKind() const noexcept -> Kind { return this->kind; };


			EVO_NODISCARD auto hasValue() const noexcept -> bool {
				return this->kind != Kind::None;
			};

			EVO_NODISCARD auto getValue() const noexcept -> NodeImpl<false> requires(IsOptional) {
				evo::debugAssert(this->hasValue(), "optional node does not have value");
				return *(NodeImpl<false>*)this;
			};

		
		private:
			Kind kind;

			union Value{
				evo::byte dummy[1];

				Token::ID token_id;
				uint32_t node_index; // used by ASTBuffer to get the data
			} value;

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


};



namespace pcit::panther::AST{

	struct VarDecl{
		Node ident;
		NodeOptional type;
		NodeOptional value;
	};

	struct FuncDecl{
		Node ident;
		Node returnType;
		Node block;
	};


	struct Block{
		evo::SmallVector<Node> stmts;
	};


	struct Type{
		// struct Qualifier{
			// bool isPtr;
			// bool isReadOnly;
			// bool isOptional;
		// };

		Node base;
		// evo::SmallVector<Qualifier> qualifiers;
	};

};




