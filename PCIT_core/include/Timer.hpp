////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <chrono>
// #include <Evo.hpp>


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

			[[nodiscard]] auto getTotal() const -> TimePoint {
				return TimePoint(TimePointDuration(this->time.load()));
			}

			[[nodiscard]] auto getTotalAsDuration() const -> TimePointDuration {
				return TimePointDuration(this->time.load());
			}
			[[nodiscard]] auto getTotalAsRep() const -> TimePointRep {
				return this->time.load();
			}


			[[nodiscard]] static auto getNow() -> TimePoint {
				return std::chrono::time_point_cast<ChronoDuration>(std::chrono::steady_clock::now());
			}


			class Runner{
				public:
					Runner(std::atomic<Timer::TimePointRep>& target_time)
						: start_time(Timer::getNow()), _target_time(&target_time) {}

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
						this->_target_time->fetch_add((end - this->start_time).count());

						#if defined(PCIT_CONFIG_DEBUG)
							this->running = false;
						#endif
					}

			
				private:
					Timer::TimePoint start_time;
					std::atomic<Timer::TimePointRep>* _target_time;

					#if defined(PCIT_CONFIG_DEBUG)
						bool running = true;
					#endif
			};

			[[nodiscard]] auto start() -> Runner { return Runner(this->time); }


	
		private:
			std::atomic<TimePointRep> time{};
			static_assert(
				std::atomic<TimePointRep>::is_always_lock_free, "Expected Timer::TimePointRep to be lock-free"
			);
	};



	using TimerNS = Timer<std::chrono::nanoseconds>;
	using TimerMS = Timer<std::chrono::milliseconds>;
	
}
