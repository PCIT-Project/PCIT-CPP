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

#include "./ASG.h"


namespace pcit::panther{

	class ASGBuffer{
		public:
			ASGBuffer() = default;
			~ASGBuffer() = default;

			///////////////////////////////////
			// funcs

			EVO_NODISCARD auto createFunc(auto&&... args) -> ASG::Func::ID {
				const auto created_id = ASG::Func::ID(uint32_t(this->funcs.size()));
				this->funcs.emplace_back(std::forward<decltype(args)>(args)...);
				return created_id;
			}

			EVO_NODISCARD auto getFunc(ASG::Func::ID id) const -> const ASG::Func& { return this->funcs[id.get()]; }


			EVO_NODISCARD auto getFuncs() const -> core::IterRange<ASG::Func::ID::Iterator> {
				return core::IterRange<ASG::Func::ID::Iterator>(
					ASG::Func::ID::Iterator(ASG::Func::ID(0)),
					ASG::Func::ID::Iterator(ASG::Func::ID(uint32_t(this->funcs.size())))
				);
			};

			EVO_NODISCARD auto numFuncs() const -> size_t { return this->funcs.size(); }


			///////////////////////////////////
			// templated func calls

			EVO_NODISCARD auto createTemplatedFunc(auto&&... args) -> ASG::TemplatedFunc::ID {
				const auto created_id = ASG::TemplatedFunc::ID(uint32_t(this->templated_funcs.size()));
				this->templated_funcs.emplace_back(
					std::make_unique<ASG::TemplatedFunc>(std::forward<decltype(args)>(args)...)
				);
				return created_id;
			}

			EVO_NODISCARD auto getTemplatedFunc(ASG::TemplatedFunc::ID id) const -> const ASG::TemplatedFunc& {
				return *this->templated_funcs[id.get()];
			}


			///////////////////////////////////
			// vars

			EVO_NODISCARD auto createVar(auto&&... args) -> ASG::Var::ID {
				const auto created_id = ASG::Var::ID(uint32_t(this->vars.size()));
				this->vars.emplace_back(std::forward<decltype(args)>(args)...);
				return created_id;
			}

			EVO_NODISCARD auto getVar(ASG::Var::ID id) const -> const ASG::Var& { return this->vars[id.get()]; }

			EVO_NODISCARD auto getVars() const -> core::IterRange<ASG::Var::ID::Iterator> {
				return core::IterRange<ASG::Var::ID::Iterator>(
					ASG::Var::ID::Iterator(ASG::Var::ID(0)),
					ASG::Var::ID::Iterator(ASG::Var::ID(uint32_t(this->vars.size())))
				);
			};

			EVO_NODISCARD auto numVars() const -> size_t { return this->vars.size(); }


			///////////////////////////////////
			// func calls

			EVO_NODISCARD auto createFuncCall(auto&&... args) -> ASG::FuncCall::ID {
				const auto created_id = ASG::FuncCall::ID(uint32_t(this->func_calls.size()));
				this->func_calls.emplace_back(std::forward<decltype(args)>(args)...);
				return created_id;
			}

			EVO_NODISCARD auto getFuncCall(ASG::FuncCall::ID id) const -> const ASG::FuncCall& {
				return this->func_calls[id.get()];
			}


			///////////////////////////////////
			// ints

			EVO_NODISCARD auto createLiteralInt(uint64_t integer, std::optional<TypeInfo::ID> type_info_id)
			-> ASG::LiteralInt::ID {
				const auto created_id = ASG::LiteralInt::ID(uint32_t(this->literal_ints.size()));
				this->literal_ints.emplace_back(integer, type_info_id);
				return created_id;
			}

			EVO_NODISCARD auto getLiteralInt(ASG::LiteralInt::ID id) const -> const ASG::LiteralInt& {
				return this->literal_ints[id.get()];
			}


			///////////////////////////////////
			// floats

			EVO_NODISCARD auto createLiteralFloat(float64_t floating_point, std::optional<TypeInfo::ID> type_info_id)
			-> ASG::LiteralFloat::ID {
				const auto created_id = ASG::LiteralFloat::ID(uint32_t(this->literal_floats.size()));
				this->literal_floats.emplace_back(floating_point, type_info_id);
				return created_id;
			}

			EVO_NODISCARD auto getLiteralFloat(ASG::LiteralFloat::ID id) const -> const ASG::LiteralFloat& {
				return this->literal_floats[id.get()];
			}


			///////////////////////////////////
			// bools

			EVO_NODISCARD auto createLiteralBool(bool boolean) -> ASG::LiteralBool::ID {
				const auto created_id = ASG::LiteralBool::ID(uint32_t(this->literal_bools.size()));
				this->literal_bools.emplace_back(boolean);
				return created_id;
			}

			EVO_NODISCARD auto getLiteralBool(ASG::LiteralBool::ID id) const -> const ASG::LiteralBool& {
				return this->literal_bools[id.get()];
			}


			///////////////////////////////////
			// chars

			EVO_NODISCARD auto createLiteralChar(char characcter) -> ASG::LiteralChar::ID {
				const auto created_id = ASG::LiteralChar::ID(uint32_t(this->literal_chars.size()));
				this->literal_chars.emplace_back(characcter);
				return created_id;
			}

			EVO_NODISCARD auto getLiteralChar(ASG::LiteralChar::ID id) const -> const ASG::LiteralChar& {
				return this->literal_chars[id.get()];
			}

	
		private:
			evo::SmallVector<ASG::Func> funcs{};
			evo::SmallVector<std::unique_ptr<ASG::TemplatedFunc>> templated_funcs{};
			evo::SmallVector<ASG::Var> vars{};

			evo::SmallVector<ASG::FuncCall> func_calls{};

			evo::SmallVector<ASG::LiteralInt> literal_ints{};
			evo::SmallVector<ASG::LiteralFloat> literal_floats{};
			evo::SmallVector<ASG::LiteralBool> literal_bools{}; // switch to bool for std::vector<bool> optimization?
			evo::SmallVector<ASG::LiteralChar> literal_chars{};

			friend class SemanticAnalyzer;
	};


}
