#include <Windows.h>
#include "GameTimer.h"

GameTimer::GameTimer()
	: seconds_per_count_(0.0)
	, delta_time_(-1.0)
	, base_time_(0)
	, paused_time_(0)
	, prev_time_(0)
	, curr_time_(0)
	, is_stopped_(false)
{
	__int64 counts_per_second;
	QueryPerformanceFrequency((LARGE_INTEGER*)&counts_per_second);
	seconds_per_count_ = 1.0 / counts_per_second;
}

// Return the total time elapsed since Reset() was called
// Not counting any time when the clock is stopped
float GameTimer::TotalTime() const
{
	if (is_stopped_)
	{
		return (float)((stop_time_ - base_time_ - paused_time_) * seconds_per_count_);
	}
	else
	{
		return (float)((curr_time_ - base_time_ - paused_time_) * seconds_per_count_);
	}
}

float GameTimer::DeltaTime() const
{
	return (float)delta_time_;
}

void GameTimer::Reset()
{
	delta_time_ = -1.0;

	__int64 curr_time;
	QueryPerformanceCounter((LARGE_INTEGER*)&curr_time);

	base_time_ = curr_time;
	paused_time_ = 0;
	stop_time_ = curr_time;
	prev_time_ = curr_time;
	curr_time_ = curr_time;
	is_stopped_ = false;
}

void GameTimer::Start()
{
	if (!is_stopped_)
		return;

	__int64 start_time;
	QueryPerformanceCounter((LARGE_INTEGER*)&start_time);

	paused_time_ += start_time - stop_time_;
	stop_time_ = 0;
	prev_time_ = start_time;
	curr_time_ = start_time;
	is_stopped_ = false;
}

void GameTimer::Stop()
{
	if (is_stopped_)
		return;

	__int64 stop_time;
	QueryPerformanceCounter((LARGE_INTEGER*)&stop_time);
	
	stop_time_ = stop_time;
	is_stopped_ = true;
}

void GameTimer::Tick()
{
	if (is_stopped_)
	{
		delta_time_ = 0.0;
		return;
	}

	__int64 curr_time;
	QueryPerformanceCounter((LARGE_INTEGER*)&curr_time);
	
	curr_time_ = curr_time;
	delta_time_ = (curr_time_ - prev_time_) * seconds_per_count_;
	prev_time_ = curr_time;

	if (delta_time_ < 0.0)
	{
		delta_time_ = 0.0;
	}
}




