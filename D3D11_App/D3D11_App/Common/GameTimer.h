#pragma once

class GameTimer
{
public:
	GameTimer();

	float TotalTime() const;			// in seconds
	float DeltaTime() const;			// in seconds

	void Reset();							// Call before message loop
	void Start();							// Call when unpaused
	void Stop();							// Call when paused
	void Tick();							// Call every frame

private:
	double seconds_per_count_;
	double delta_time_;

	__int64 base_time_;				// time point
	__int64 paused_time_;			// time segment: sum of each puasing
	__int64 stop_time_;				// time point
	__int64 prev_time_;				// time point
	__int64 curr_time_;					// time point

	bool is_stopped_;
};