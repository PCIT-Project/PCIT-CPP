////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>

#include "./misc.h"
#include "./forward_decl_ids.h"
#include "./Expr.h"

#include <iterator>

namespace pcit::pir{


	class BasicBlock{
		public:
			// for lookup in Module
			using ID = BasicBlockID;

		public:
			BasicBlock(std::string&& _name) : name(std::move(_name)) {
				evo::debugAssert(isStandardName(this->name), "Not valid name for basic block ({})", this->name);
			}
			~BasicBlock() = default;

			EVO_NODISCARD auto getName() const -> const std::string& { return this->name; }

			EVO_NODISCARD auto isTerminated() const -> bool {
				if(this->exprs.empty()){ return false; }
				return this->exprs.back().isTerminator();
			}


			//////////////////////////////////////////////////////////////////////
			// iterators

			using Iter                 = evo::SmallVector<Expr>::iterator;
			using ConstIter            = evo::SmallVector<Expr>::const_iterator;
			using ReverseIter          = evo::SmallVector<Expr>::reverse_iterator;
			using ConstReverseIterator = evo::SmallVector<Expr>::const_reverse_iterator;

			EVO_NODISCARD auto begin()        -> Iter      { return this->exprs.begin();  };
			EVO_NODISCARD auto begin()  const -> ConstIter { return this->exprs.begin();  };
			EVO_NODISCARD auto cbegin() const -> ConstIter { return this->exprs.cbegin(); };

			EVO_NODISCARD auto end()        -> Iter      { return this->exprs.end();  };
			EVO_NODISCARD auto end()  const -> ConstIter { return this->exprs.end();  };
			EVO_NODISCARD auto cend() const -> ConstIter { return this->exprs.cend(); };


			EVO_NODISCARD auto rbegin()        -> ReverseIter          { return this->exprs.rbegin();  };
			EVO_NODISCARD auto rbegin()  const -> ConstReverseIterator { return this->exprs.rbegin();  };
			EVO_NODISCARD auto crbegin() const -> ConstReverseIterator { return this->exprs.crbegin(); };

			EVO_NODISCARD auto rend()        -> ReverseIter          { return this->exprs.rend();  };
			EVO_NODISCARD auto rend()  const -> ConstReverseIterator { return this->exprs.rend();  };
			EVO_NODISCARD auto crend() const -> ConstReverseIterator { return this->exprs.crend(); };


			EVO_NODISCARD auto front() const -> const Expr& { return this->exprs.front(); }
			EVO_NODISCARD auto front()       ->       Expr& { return this->exprs.front(); }

			EVO_NODISCARD auto back() const -> const Expr& { return this->exprs.back(); }
			EVO_NODISCARD auto back()       ->       Expr& { return this->exprs.back(); }


			EVO_NODISCARD auto size() const -> size_t { return this->exprs.size(); }

			EVO_NODISCARD auto operator[](size_t i) const -> const Expr& { return this->exprs[i]; }
			EVO_NODISCARD auto operator[](size_t i)       ->       Expr& { return this->exprs[i]; }

		private:
			auto append(Expr&& expr) -> void {
				evo::debugAssert(expr.isStmt(), "Must append stmt");
				evo::debugAssert(this->isTerminated() == false, "Basic block already terminated");

				this->exprs.emplace_back(std::move(expr));
			}

			auto append(const Expr& expr) -> void {
				evo::debugAssert(expr.isStmt(), "Must append stmt");
				evo::debugAssert(this->isTerminated() == false, "Basic block already terminated");

				this->exprs.emplace_back(expr);
			}


			auto insert(Expr&& expr, size_t index) -> void {
				evo::debugAssert(expr.isStmt(), "Must append stmt");
				
				this->exprs.insert(this->index_to_iter(index + 1), std::move(expr));
			}

			auto insert(const Expr& expr, size_t index) -> void {
				evo::debugAssert(expr.isStmt(), "Must append stmt");
				
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

			bool is_terminated;

			friend class Agent;
	};


}


