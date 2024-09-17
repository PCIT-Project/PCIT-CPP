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
			Parser(Context& _context, Source::ID source_id) :
				context(_context),
				source(this->context.getSourceManager().getSource(source_id)),
				reader(this->source.getTokenBuffer())
				{}

			~Parser() = default;


			EVO_NODISCARD auto parse() -> bool;


		private:
			class Result{
				public:
					enum class Code{
						Success,
						WrongType,
						Error,
					};

				public:
					Result(Code res_code) : result_code(res_code) {}
					Result(AST::Node val) : result_code(Code::Success), node(val) {}

					// Result(const Result& rhs) = default;

					~Result() = default;

					EVO_NODISCARD auto code() const -> Code { return this->result_code; }
					EVO_NODISCARD auto value() const -> const AST::Node& {
						evo::debugAssert(
							this->result_code == Code::Success,
							"Attempted to get value from result that has no value"
						);
						return this->node;
					}
			
				private:
					Code result_code;

					union { // hack to allow for the node to be unitialized
						evo::byte dummy_data[1];
						AST::Node node;
					};
			};
			static_assert(std::is_trivially_copyable_v<Result>, "Result is not trivially copyable");


			EVO_NODISCARD auto parse_stmt() -> Result;

			template<AST::VarDecl::Kind VAR_DECL_KIND>
			EVO_NODISCARD auto parse_var_decl() -> Result;

			EVO_NODISCARD auto parse_func_decl() -> Result;
			EVO_NODISCARD auto parse_alias_decl() -> Result;
			EVO_NODISCARD auto parse_return() -> Result;
			EVO_NODISCARD auto parse_unreachable() -> Result;

			template<bool IS_WHEN>
			EVO_NODISCARD auto parse_conditional() -> Result;

			EVO_NODISCARD auto parse_assignment() -> Result;


			enum class BlockLabelRequirement{
				Required,
				NotAllowed,
				Optional,
			};
			// TODO: make label_requirement template?
			EVO_NODISCARD auto parse_block(BlockLabelRequirement label_requirement) -> Result;

			
			enum class TypeKind{
				Explicit,
				Expr,
				TemplateArg,
			};
			template<TypeKind KIND>
			EVO_NODISCARD auto parse_type() -> Result;

			EVO_NODISCARD auto parse_expr() -> Result;
			EVO_NODISCARD auto parse_sub_expr() -> Result;
			EVO_NODISCARD auto parse_infix_expr() -> Result;
			EVO_NODISCARD auto parse_infix_expr_impl(AST::Node lhs, int prec_level) -> Result;
			EVO_NODISCARD auto parse_prefix_expr() -> Result;

			enum class IsTypeTerm{
				Yes,
				YesAs, // for the [as] operator
				No,
				Maybe,
			};
			template<IsTypeTerm IS_TYPE_TERM>
			EVO_NODISCARD auto parse_term() -> Result; 
			EVO_NODISCARD auto parse_term_stmt() -> Result;
			EVO_NODISCARD auto parse_encapsulated_expr() -> Result;
			EVO_NODISCARD auto parse_atom() -> Result;

			EVO_NODISCARD auto parse_attribute_block() -> Result;
			EVO_NODISCARD auto parse_ident() -> Result;
			EVO_NODISCARD auto parse_intrinsic() -> Result;
			EVO_NODISCARD auto parse_literal() -> Result;
			EVO_NODISCARD auto parse_uninit() -> Result;
			EVO_NODISCARD auto parse_zeroinit() -> Result;
			EVO_NODISCARD auto parse_this() -> Result;


			EVO_NODISCARD auto parse_template_pack() -> Result;

			EVO_NODISCARD auto parse_func_params() -> evo::Result<evo::SmallVector<AST::FuncDecl::Param>>;
			EVO_NODISCARD auto parse_func_returns() -> evo::Result<evo::SmallVector<AST::FuncDecl::Return>>;


			///////////////////////////////////
			// checking

			auto expected_but_got(
				std::string_view location_str, Token::ID token, evo::SmallVector<Diagnostic::Info>&& infos = {}
			) -> void;

			EVO_NODISCARD auto check_result_fail(const Result& result, std::string_view location_str) -> bool;

			EVO_NODISCARD auto assert_token_fail(Token::Kind kind) -> bool;
			EVO_NODISCARD auto expect_token_fail(Token::Kind kind, std::string_view location_str) -> bool;


		private:
			Context& context;
			Source& source;


			///////////////////////////////////
			// token reader

			class TokenReader{
				public:
					TokenReader(const TokenBuffer& token_buffer) : buffer(token_buffer) {}
					~TokenReader() = default;

					EVO_NODISCARD auto at_end() const -> uint32_t {
						return this->cursor >= this->buffer.size();
					}

					EVO_NODISCARD auto peek(ptrdiff_t offset = 0) const -> Token::ID {
						const ptrdiff_t peek_location = ptrdiff_t(this->cursor) + offset;

						evo::debugAssert(peek_location >= 0, "cannot peek past beginning of buffer");
						evo::debugAssert(
							size_t(peek_location) < this->buffer.size(), "cannot peek past beginning of buffer"
						);

						return Token::ID(uint32_t(peek_location));
					}

					auto skip() -> void {
						evo::debugAssert(this->at_end() == false, "already at end");

						this->cursor += 1;
					}

					EVO_NODISCARD auto next() -> Token::ID {
						EVO_DEFER([&](){ this->skip(); });
						return this->peek();
					}


					EVO_NODISCARD auto get(Token::ID id) const -> const Token& {
						return this->buffer[id];
					}

					EVO_NODISCARD auto operator[](Token::ID id) const -> const Token& {
						return this->get(id);
					}

					EVO_NODISCARD auto go_back(Token::ID id) -> void {
						evo::debugAssert(id.get() < this->cursor, "id is not before current location");
						this->cursor = id.get();
					}

			
				private:
					const TokenBuffer& buffer;
					uint32_t cursor = 0;
			};

			TokenReader reader;


	};


}
