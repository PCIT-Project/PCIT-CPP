//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>

#include "./forward_decl_ids.h"
#include "./Expr.h"

namespace pcit::pir{


	class BasicBlock{
		public:
			// for lookup in Module
			using ID = BasicBlockID;

		public:
			BasicBlock(std::string&& _name) : name(std::move(_name)) {}
			~BasicBlock() = default;

			EVO_NODISCARD auto getName() const -> std::string_view { return this->name; }

			EVO_NODISCARD auto isTerminated() const -> bool {
				if(this->exprs.empty()){ return false; }
				return this->exprs.back().isTerminator();
			}


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
	
		private:
			std::string name;

			evo::SmallVector<Expr> exprs{};

			bool is_terminated;
	};


}


