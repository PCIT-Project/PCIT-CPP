////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>

#include <deque>

namespace pcit::core{

	template<class TASK>
	class SingleThreadedWorkQueue{
		public:
			using WorkFunc = std::function<evo::Result<>(TASK&)>;

		public:
			SingleThreadedWorkQueue(WorkFunc&& work_func) : _work_func(std::move(work_func)) {}
			SingleThreadedWorkQueue(const WorkFunc& work_func) : _work_func(work_func) {}

			~SingleThreadedWorkQueue() = default;


			auto emplaceTask(auto&&... args) -> void {
				this->tasks.emplace_front(std::forward<decltype(args)>(args)...);
			}

			auto addTask(TASK&& task) -> void {
				this->tasks.emplace_front(std::move(task));
			}

			auto addTask(const TASK& task) -> void {
				this->tasks.emplace_front(task);
			}

			auto addTask(auto&&... args) -> void {
				this->tasks.emplace_front(std::forward<decltype(args)>(args)...);
			}

			auto run() -> evo::Result<> {
				while(this->tasks.empty() == false){
					TASK next_task = std::move(this->tasks.back());
					this->tasks.pop_back();
					const evo::Result<> work_res = this->_work_func(next_task);
					if(work_res.isError()){ return evo::resultError; }
				}

				return evo::Result<>();
			}


			auto forceClearQueue() -> void {
				this->tasks.clear();
			}

		private:
			std::deque<TASK> tasks{};

			WorkFunc _work_func;
	};


}


