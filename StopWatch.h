/* 
 * File:   StopWatch.h
 * Author: KjellKod
 * From: https://github.com/KjellKod/StopWatch
 * 
 * Created on 2014-02-07 
 */


#pragma once
#include <boost/chrono/chrono.hpp>
using namespace boost::chrono ;

class StopWatch {
public:
   typedef steady_clock clock;
   typedef microseconds microseconds;
   typedef milliseconds milliseconds;
   typedef seconds seconds;

   StopWatch();
   StopWatch(const StopWatch&);
   StopWatch& operator=(const StopWatch& rhs);

   uint64_t ElapsedUs() const;
   uint64_t ElapsedMs() const;
   uint64_t ElapsedSec() const;

   steady_clock::time_point Restart();

private:
   clock::time_point mStart;
};

