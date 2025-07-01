////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <stack>

#include <Evo.h>
#include <PCIT_core.h>

#include "../../include/Context.h"
#include "../../include/AST/AST.h"
#include "../../include/source/source_data.h"
#include "./TokenReader.h"


namespace pcit::panther{


	class Parser{
		public:
			Parser(Context& _context, Source::ID source_id) :
				context(_context),
				source(this->context.getSourceManager()[source_id]),
				reader(this->source.getTokenBuffer())
				{}

			~Parser() = default;


			EVO_NODISCARD auto parse() -> evo::Result<>;


		private:
			class Result{
				public:
					enum class Code{
						SUCCESS,
						WRONG_TYPE,
						ERROR,
					};

				public:
					Result(Code res_code) : result_code(res_code) {
						evo::debugAssert(this->result_code != Code::SUCCESS, "Incorrect constructor for this code");
					}
					Result(AST::Node val) : result_code(Code::SUCCESS), node(val) {}

					// Result(const Result& rhs) = default;

					~Result() = default;

					EVO_NODISCARD auto code() const -> Code { return this->result_code; }
					EVO_NODISCARD auto value() const -> const AST::Node& {
						evo::debugAssert(
							this->result_code == Code::SUCCESS,
							"Attempted to get value from result that has no value"
						);
						return this->node;
					}
			
				private:
					Code result_code;

					struct DummyData{ evo::byte dummy_data[1]; };

					union { // hack to allow for the node to be unitialized
						DummyData dummy_data;
						AST::Node node;
					};
			};
			static_assert(std::is_trivially_copyable_v<Result>, "Result is not trivially copyable");


			EVO_NODISCARD auto parse_stmt() -> Result;

			template<AST::VarDecl::Kind VAR_DECL_KIND>
			EVO_NODISCARD auto parse_var_decl() -> Result;

			template<bool MUST_HAVE_BODY>
			EVO_NODISCARD auto parse_func_decl() -> Result;

			EVO_NODISCARD auto parse_alias_decl() -> Result;
			EVO_NODISCARD auto parse_type_decl() -> Result;
			EVO_NODISCARD auto parse_struct_decl(const AST::Node& ident, const AST::Node& attrs_pre_equals) -> Result;
			EVO_NODISCARD auto parse_interface_decl() -> Result;
			EVO_NODISCARD auto parse_interface_impl() -> Result;
			EVO_NODISCARD auto parse_return() -> Result;
			EVO_NODISCARD auto parse_error() -> Result;
			EVO_NODISCARD auto parse_unreachable() -> Result;
			template<bool IS_WHEN> EVO_NODISCARD auto parse_conditional() -> Result;
			EVO_NODISCARD auto parse_while() -> Result;
			template<bool IS_ERROR_DEFER> EVO_NODISCARD auto parse_defer() -> Result;

			EVO_NODISCARD auto parse_assignment() -> Result;


			enum class BlockLabelRequirement{
				REQUIRED,
				NOT_ALLOWED,
				OPTIONAL,
			};
			// TODO(PERF): make label_requirement template?
			EVO_NODISCARD auto parse_block(BlockLabelRequirement label_requirement) -> Result;

			
			enum class TypeKind{
				EXPLICIT,
				EXPLICIT_MAYBE_DEDUCER,
				EXPR, // For use with `as`
				TEMPLATE_ARG,
			};
			template<TypeKind KIND>
			EVO_NODISCARD auto parse_type() -> Result;

			EVO_NODISCARD auto parse_expr() -> Result;
			EVO_NODISCARD auto parse_sub_expr() -> Result;
			EVO_NODISCARD auto parse_infix_expr() -> Result;
			EVO_NODISCARD auto parse_infix_expr_impl(AST::Node lhs, int prec_level) -> Result;
			EVO_NODISCARD auto parse_prefix_expr() -> Result;
			EVO_NODISCARD auto parse_new_expr() -> Result;
			EVO_NODISCARD auto parse_try_expr() -> Result;

			enum class TermKind{
				EXPLICIT_TYPE,
				AS_TYPE,
				EXPR,
				TEMPLATE_ARG,
			};
			template<TermKind TERM_KIND>
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
			EVO_NODISCARD auto parse_func_error_returns() -> evo::Result<evo::SmallVector<AST::FuncDecl::Return>>;
			EVO_NODISCARD auto parse_func_call_args() -> evo::Result<evo::SmallVector<AST::FuncCall::Arg>>;
			EVO_NODISCARD auto parse_struct_init() -> evo::Result<evo::SmallVector<AST::StructInitNew::MemberInit>>;


			///////////////////////////////////
			// checking

			auto expected_but_got(
				std::string_view expected_str, Token::ID got_token, evo::SmallVector<Diagnostic::Info>&& infos = {}
			) -> void;

			EVO_NODISCARD auto check_result_fail(const Result& result, std::string_view location_str) -> bool;

			EVO_NODISCARD auto assert_token_fail(Token::Kind kind) -> bool;
			EVO_NODISCARD auto expect_token_fail(Token::Kind kind, std::string_view location_str) -> bool;


		private:
			Context& context;
			Source& source;

			TokenReader reader;
	};


	EVO_NODISCARD inline auto parse(Context& context, Source::ID source_id) -> evo::Result<> {
		auto parser = Parser(context, source_id);
		return parser.parse();
	}


}
