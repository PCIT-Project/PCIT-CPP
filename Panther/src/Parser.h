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
			Parser(Context& _context, Source::ID source_id) noexcept :
				context(_context),
				source(this->context.getSourceManager().getSource(source_id)),
				reader(this->source.getTokenBuffer())
				{};

			~Parser() = default;


			EVO_NODISCARD auto parse() noexcept -> bool;


		private:
			class Result{
				public:
					enum class Code{
						Success,
						WrongType,
						Error,
					};

				public:
					Result(Code res_code) noexcept : result_code(res_code) {};
					Result(AST::Node val) noexcept : result_code(Code::Success), node(val) {};

					Result(const Result& rhs) noexcept = default;

					~Result() = default;

					EVO_NODISCARD auto code() const noexcept -> Code { return this->result_code; };
					EVO_NODISCARD auto value() const noexcept -> const AST::Node& {
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
			static_assert(std::is_trivially_copyable_v<Result>, "Result is not trivially copyable");


			EVO_NODISCARD auto parse_stmt() noexcept -> Result;

			EVO_NODISCARD auto parse_var_decl() noexcept -> Result;
			EVO_NODISCARD auto parse_func_decl() noexcept -> Result;
			EVO_NODISCARD auto parse_assignment() noexcept -> Result;

			EVO_NODISCARD auto parse_block() noexcept -> Result;
			EVO_NODISCARD auto parse_type(bool is_expr = false) noexcept -> Result;

			EVO_NODISCARD auto parse_expr() noexcept -> Result;
			EVO_NODISCARD auto parse_sub_expr() noexcept -> Result;
			EVO_NODISCARD auto parse_infix_expr() noexcept -> Result;
			EVO_NODISCARD auto parse_infix_expr_impl(AST::Node lhs, int prec_level) noexcept -> Result;
			EVO_NODISCARD auto parse_prefix_expr() noexcept -> Result;

			enum class IsTypeTerm{
				Yes,
				No,
				Maybe,
			};
			EVO_NODISCARD auto parse_term(IsTypeTerm is_type_term = IsTypeTerm::No) noexcept -> Result;
			EVO_NODISCARD auto parse_paren_expr() noexcept -> Result;
			EVO_NODISCARD auto parse_atom() noexcept -> Result;

			EVO_NODISCARD auto parse_ident() noexcept -> Result;
			EVO_NODISCARD auto parse_intrinsic() noexcept -> Result;
			EVO_NODISCARD auto parse_literal() noexcept -> Result;
			EVO_NODISCARD auto parse_uninit() noexcept -> Result;

			///////////////////////////////////
			// checking

			auto expected_but_got(
				std::string_view location_str, const Token& token, evo::SmallVector<Diagnostic::Info>&& infos = {}
			) noexcept -> void;

			EVO_NODISCARD auto check_result_fail(const Result& result, std::string_view location_str) noexcept -> bool;

			EVO_NODISCARD auto assert_token_fail(Token::Kind kind) noexcept -> bool;
			EVO_NODISCARD auto expect_token_fail(Token::Kind kind, std::string_view location_str) noexcept -> bool;


		private:
			Context& context;
			Source& source;


			///////////////////////////////////
			// token reader

			class TokenReader{
				public:
					TokenReader(const TokenBuffer& token_buffer) noexcept : buffer(token_buffer) {};
					~TokenReader() = default;

					EVO_NODISCARD auto at_end() const noexcept -> uint32_t {
						return this->cursor >= this->buffer.size();
					};

					EVO_NODISCARD auto peek(ptrdiff_t offset = 0) const noexcept -> Token::ID {
						const ptrdiff_t peek_location = ptrdiff_t(this->cursor) + offset;

						evo::debugAssert(peek_location >= 0, "cannot peek past beginning of buffer");
						evo::debugAssert(
							size_t(peek_location) < this->buffer.size(), "cannot peek past beginning of buffer"
						);

						return Token::ID(uint32_t(peek_location));
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

					EVO_NODISCARD auto go_back(Token::ID id) noexcept -> void {
						evo::debugAssert(id.get() < this->cursor, "id is not before current location");
						this->cursor = id.get();
					};

			
				private:
					const TokenBuffer& buffer;
					uint32_t cursor = 0;
			};

			TokenReader reader;


	};


};
