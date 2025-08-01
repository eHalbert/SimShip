/// Copyright (C) 2012 - All Rights Reserved
/// All rights reserved. http://www.equals-forty-two.com
///
/// @brief Timer interface

#pragma once

#include <iomanip>
#include <iostream>
#include <Windows.h>

namespace eh
{
	class Timer
	{
	public:

		// 'running' is initially false.  A timer needs to be explicitly started
		// using 'start' or 'restart'
		Timer();

		/// Check if the timer is running
		bool isRunning() const { return mRunning; }

		/// Return the elapsed seconds
		double getTime() const;

		/// Get the elapsed time on the last batch (start/stop)
		double getElapsedTime() const;
		
		/// Init the timer
		void reset();

		/// Start the timer
		void start();

		/// Restart the timer
		void restart();

		/// Stop the timer
		void stop();

		/// Display the timer state.
		void check(std::ostream& os = std::cout);

	private:

		friend std::ostream& operator<<(std::ostream& os, const Timer& t);

		// Data members

		bool     mRunning;
		double   mAccumulatedTime; ///> Accumulated time in seconds

		LARGE_INTEGER  mFrequency;
		LARGE_INTEGER  mStartTime;    ///< Start time of the timer.
	};

	class DelayTimer : private Timer
	{
	public:

		DelayTimer();

		DelayTimer(double delay);

		void set(double delay);

		bool check() const;

	private:

		double mDelay;
	};
}

