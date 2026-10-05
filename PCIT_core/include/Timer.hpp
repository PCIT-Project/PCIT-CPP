////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <chrono>
#include <Evo.hpp>


namespace pcit::core{

	template<class ChronoDuration>
	class Timer{
		public:
			Timer() = default;
			~Timer() = default;

			Timer(const Timer&) = delete;


			using TimePoint = std::chrono::time_point<std::chrono::steady_clock, ChronoDuration>;
			using TimePointDuration = typename TimePoint::duration;
			using TimePointRep = typename TimePoint::rep;


			///////////////////////////////////
			// these functions assume no timers are running

			[[nodiscard]] auto getTotal() const -> TimePoint {
				return TimePoint(TimePointDuration(this->getTotalAsRep()));
			}

			[[nodiscard]] auto getTotalAsDuration() const -> TimePointDuration {
				return TimePointDuration(this->getTotalAsRep());
			}

			[[nodiscard]] auto getTotalAsRep() const -> TimePointRep {
				TimePointRep total = 0;

				for(const TimePointRep& time : this->times){
					total += time;
				}

				return total;
			}



			[[nodiscard]] auto getMax() const -> TimePoint {
				return TimePoint(TimePointDuration(this->getMaxAsRep()));
			}

			[[nodiscard]] auto getMaxAsDuration() const -> TimePointDuration {
				return TimePointDuration(this->getMaxAsRep());
			}

			[[nodiscard]] auto getMaxAsRep() const -> TimePointRep {
				TimePointRep max = 0;

				for(const TimePointRep& time : this->times){
					max = std::max(max, time);
				}

				return max;
			}


			// these functions assume no timers are running
			///////////////////////////////////



			[[nodiscard]] static auto getNow() -> TimePoint {
				return std::chrono::time_point_cast<ChronoDuration>(std::chrono::steady_clock::now());
			}


			class Runner{
				public:
					Runner(Timer::TimePointRep& target_time)
						: start_time(Timer::getNow()), _target_time(target_time) {}

					#if defined(PCIT_CONFIG_DEBUG)
						~Runner(){ evo::debugAssert(this->running == false, "Timer wasn't stopped"); }
					#else
						~Runner() = default;
					#endif

					Runner(const Runner&) = delete;

					#if defined(PCIT_CONFIG_DEBUG)
						Runner(Runner&& rhs) : 
							start_time(rhs.start_time),
							_target_time(rhs._target_time),
							running(std::exchange(rhs.running, false))
						{}
					#else
						Runner(Runner&& rhs) : start_time(rhs.start_time), _target_time(rhs._target_time) {}
					#endif

					auto operator=(Runner&& rhs) -> Runner& {
						std::construct_at(this, std::move(rhs));
						return *this;
					}


					auto stop() -> void {
						const Timer::TimePoint end = Timer::getNow();
						this->_target_time += (end - this->start_time).count();

						#if defined(PCIT_CONFIG_DEBUG)
							this->running = false;
						#endif
					}

			
				private:
					Timer::TimePoint start_time;
					Timer::TimePointRep& _target_time;

					#if defined(PCIT_CONFIG_DEBUG)
						bool running = true;
					#endif
			};

			[[nodiscard]] auto start() -> Runner {
				const std::thread::id this_thread_id = std::this_thread::get_id();

				TimePointRep* time_point_rep = [&]() -> TimePointRep* {
					const auto lock = std::scoped_lock(this->times_map_lock);

					const auto time_point_find = this->times_map.find(this_thread_id);
					if(time_point_find != this->times_map.end()){
						return time_point_find->second;

					}else{
						TimePointRep* created_time_point_rep = &this->times.emplace_back(0);
						this->times_map.emplace(this_thread_id, created_time_point_rep);
						return created_time_point_rep;
					}
				}();

				return Runner(*time_point_rep);
			}


	
		private:
			evo::StepVector<TimePointRep> times{};

			std::unordered_map<std::thread::id, TimePointRep*> times_map{};
			evo::SpinLock times_map_lock{};
	};



	using TimerNS = Timer<std::chrono::nanoseconds>;
	using TimerMS = Timer<std::chrono::milliseconds>;
	
}
