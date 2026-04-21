////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.hpp>

#include "./misc.hpp"
#include "./forward_decl_ids.hpp"
#include "./Expr.hpp"

#include <iterator>

namespace pcit::pir{


	class BasicBlock{
		public:
			// for lookup in Module
			using ID = BasicBlockID;

		public:
			// should only be used by InstrHandler as it sets ID as well
			BasicBlock(std::string&& _name) : name(std::move(_name)){
				evo::debugAssert(isStandardName(this->name), "Not valid name for basic block ({})", this->name);
			}
			~BasicBlock() = default;

			[[nodiscard]] auto getName() const -> const std::string& { return this->name; }
			[[nodiscard]] auto getID() const -> ID { return this->id; }

			[[nodiscard]] auto isTerminated() const -> bool {
				if(this->exprs.empty()){ return false; }
				return this->exprs.back().isTerminator();
			}


			//////////////////////////////////////////////////////////////////////
			// iterators

			using Iter                 = evo::SmallVector<Expr>::iterator;
			using ConstIter            = evo::SmallVector<Expr>::const_iterator;
			using ReverseIter          = evo::SmallVector<Expr>::reverse_iterator;
			using ConstReverseIterator = evo::SmallVector<Expr>::const_reverse_iterator;

			[[nodiscard]] auto begin()        -> Iter      { return this->exprs.begin();  };
			[[nodiscard]] auto begin()  const -> ConstIter { return this->exprs.begin();  };
			[[nodiscard]] auto cbegin() const -> ConstIter { return this->exprs.cbegin(); };

			[[nodiscard]] auto end()        -> Iter      { return this->exprs.end();  };
			[[nodiscard]] auto end()  const -> ConstIter { return this->exprs.end();  };
			[[nodiscard]] auto cend() const -> ConstIter { return this->exprs.cend(); };


			[[nodiscard]] auto rbegin()        -> ReverseIter          { return this->exprs.rbegin();  };
			[[nodiscard]] auto rbegin()  const -> ConstReverseIterator { return this->exprs.rbegin();  };
			[[nodiscard]] auto crbegin() const -> ConstReverseIterator { return this->exprs.crbegin(); };

			[[nodiscard]] auto rend()        -> ReverseIter          { return this->exprs.rend();  };
			[[nodiscard]] auto rend()  const -> ConstReverseIterator { return this->exprs.rend();  };
			[[nodiscard]] auto crend() const -> ConstReverseIterator { return this->exprs.crend(); };


			[[nodiscard]] auto front() const -> const Expr& { return this->exprs.front(); }
			[[nodiscard]] auto front()       ->       Expr& { return this->exprs.front(); }

			[[nodiscard]] auto back() const -> const Expr& { return this->exprs.back(); }
			[[nodiscard]] auto back()       ->       Expr& { return this->exprs.back(); }


			[[nodiscard]] auto size() const -> size_t { return this->exprs.size(); }

			// Not intended to be called directly, better to be called by InstrHandler::deleteBodyOfTargetBasicBlock
			// 	as it will delete all exprs
			auto clear() -> void { this->exprs.clear(); }

			[[nodiscard]] auto operator[](size_t i) const -> const Expr& { return this->exprs[i]; }
			[[nodiscard]] auto operator[](size_t i)       ->       Expr& { return this->exprs[i]; }

		private:
			auto append(Expr&& expr) -> void {
				evo::debugAssert(expr.isStmt(), "Can only append stmt");
				evo::debugAssert(this->isTerminated() == false, "Basic block already terminated");

				this->exprs.emplace_back(std::move(expr));
			}

			auto append(const Expr& expr) -> void {
				evo::debugAssert(expr.isStmt(), "Can only append stmt");
				evo::debugAssert(this->isTerminated() == false, "Basic block already terminated");

				this->exprs.emplace_back(expr);
			}


			auto insert(Expr&& expr, size_t index) -> void {
				evo::debugAssert(expr.isStmt(), "Can only insert stmt");
				
				this->exprs.insert(this->index_to_iter(index + 1), std::move(expr));
			}

			auto insert(const Expr& expr, size_t index) -> void {
				evo::debugAssert(expr.isStmt(), "Can only insert stmt");
				
				this->exprs.insert(this->index_to_iter(index + 1), expr);
			}


			auto remove(size_t index) -> void {
				this->exprs.erase(this->index_to_iter(index));
			}


			auto index_to_iter(size_t index) -> Iter {
				Iter iter = this->begin();
				std::advance(iter, ptrdiff_t(index));
				return iter;
			}
	
		private:
			std::string name;
			evo::SmallVector<Expr> exprs{};
			ID id;
			bool is_terminated;

			friend class InstrHandler;
	};


}


