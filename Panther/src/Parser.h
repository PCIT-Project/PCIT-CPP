//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once

#include <stack>

#include <Evo.h>
#include <PCIT_core.h>

#include "../include/Context.h"
#include "../include/source_data.h"


namespace pcit::panther{


	class Parser{
		public:
			Parser(Context& _context, Source::ID _source_id) noexcept
				: context(_context),
				  source_id(_source_id),
				  reader(this->context.getSourceManager().getSource(this->source_id).getTokenBuffer())
				{};

			~Parser() = default;

			EVO_NODISCARD auto parse() noexcept -> void;

		public:
			///////////////////////////////////
			// result 

			class Result{
				public:
					enum class Code{
						None,
						Success,
						WrongType,
						Error,
					};

					using enum class Code;

				public:
					Result() noexcept : result_code(Code::None) {};
					Result(Code res_code) noexcept : result_code(res_code) {};
					Result(AST::Node val) noexcept : result_code(Code::Success), node(val) {};

					Result(const Result& rhs) noexcept : result_code(rhs.result_code), node(rhs.node) {};

					~Result() = default;

					EVO_NODISCARD inline auto code() const noexcept -> Code { return this->result_code; };
					EVO_NODISCARD inline auto value() const noexcept -> const AST::Node& {
						evo::debugAssert(
							this->result_code == Code::Success,
							"Attempted to get value from result that has no value"
						);
						return this->node;
					};
			
				private:
					Code result_code;

					union { // hack to allow for the node to be unitialized
						evo::byte dummy_data[1];
						AST::Node node;
					};
			};


			///////////////////////////////////
			// stack

			struct StackFrame{
				struct VarDecl{
					AST::Node ident;
					bool has_type = false;
					AST::NodeOptional type{};
					bool has_expr = false;
				};

				union Context{
					evo::byte dummy[1];

					VarDecl var_decl;

					Context() noexcept {};
					Context(VarDecl&& _var_decl) noexcept : var_decl(_var_decl) {};
				};

				using Call = void(*)(Parser&);
				Call call; // nullptr means that it should remove a context
			};

			// these functions may not be called directly
			auto _parse_stmt_selector() noexcept -> void;
			auto _parse_stmt_add_global() noexcept -> void;

			auto _parse_var_decl_type() noexcept -> void;
			auto _parse_var_decl_value() noexcept -> void;
			auto _parse_var_decl_end() noexcept -> void;


		private:
			auto execute_stack() noexcept -> void;


			///////////////////////////////////
			// parsing

			// these functions return through this->last_result and only one can be used per function as it requires
			// 	use of the dynamic stack
			auto parse_stmt_dispatch() noexcept -> void;
			auto parse_global_stmt_dispatch() noexcept -> void;

			auto parse_var_decl_dispatch() noexcept -> void;
			auto parse_type_dispatch() noexcept -> void;

			auto parse_expr_dispatch() noexcept -> void;

			auto parse_atom() noexcept -> Result;
			auto parse_literal() noexcept -> Result;
			auto parse_ident() noexcept -> Result;


			///////////////////////////////////
			// checking and messaging

			// returns true if caller should return
			EVO_NODISCARD auto expected_but_got(std::string_view expected, const Token& token) noexcept -> bool;
			EVO_NODISCARD auto check_result(const Result& result, std::string_view expected) noexcept -> bool;
			EVO_NODISCARD auto expect_token(Token::Kind kind, std::string_view location_str) noexcept -> bool;
			EVO_NODISCARD auto assert_token(Token::Kind kind, std::string_view location_str) noexcept -> bool;


			///////////////////////////////////
			// misc helpers
			
			EVO_NODISCARD auto getASTBuffer() noexcept -> ASTBuffer&;


		private:
			Context& context;
			Source::ID source_id;

			std::stack<StackFrame> stack{};
			std::stack<StackFrame::Context> frame_contexts{};

			Result last_result{};


			///////////////////////////////////
			// token reader

			class TokenReader{
				public:
					TokenReader(const TokenBuffer& token_buffer) noexcept : buffer(token_buffer) {};
					~TokenReader() = default;

					EVO_NODISCARD auto at_end() const noexcept -> uint32_t {
						return this->cursor >= this->buffer.size();
					};

					EVO_NODISCARD auto peek() const noexcept -> Token::ID {
						evo::debugAssert(this->at_end() == false, "already at end");

						return Token::ID(this->cursor);
					};

					auto skip() noexcept -> void {
						evo::debugAssert(this->at_end() == false, "already at end");

						this->cursor += 1;
					};

					EVO_NODISCARD auto next() noexcept -> Token::ID {
						EVO_DEFER([&](){ this->skip(); });
						return this->peek();
					};


					EVO_NODISCARD auto get(Token::ID id) const noexcept -> const Token& {
						return this->buffer[id];
					};

					EVO_NODISCARD auto operator[](Token::ID id) const noexcept -> const Token& {
						return this->get(id);
					};

			
				private:
					const TokenBuffer& buffer;
					uint32_t cursor = 0;
			};

			TokenReader reader;


	};


};
