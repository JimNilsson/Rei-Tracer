#include "Timer.h"

Timer::Timer()
{
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	_secondsPerCount = 1.0 / (double)countsPerSec;

	_deltaTime = 0;
	_prevTime = 0;
	_currTime = 0;
}

Timer::~Timer()
{
}

void Timer::Update()
{
	__int64 curTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&curTime);
	_currTime = curTime;
	_deltaTime = (_currTime - _prevTime) * _secondsPerCount;
	_prevTime = _currTime;


}

float Timer::GetDeltaTime() const
{
	return (float)_deltaTime;
}

uint64_t Timer::GetTimeStamp() const
{
	uint64_t timestamp;
	QueryPerformanceCounter((LARGE_INTEGER*)&timestamp);
	return timestamp;
}

