#ifndef _TIMER_H_
#define _TIMER_H_

#include <Windows.h>
#include <stdint.h>
class Timer
{
public:
	Timer();
	~Timer();
	void Update();
	float GetDeltaTime() const;
	uint64_t GetTimeStamp() const;


private:
	double _secondsPerCount;
	double _deltaTime;

	__int64 _prevTime;
	__int64 _currTime;


};

#endif

