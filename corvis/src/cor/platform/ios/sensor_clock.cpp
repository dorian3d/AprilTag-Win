//
//  sensor_clock.cpp
//  RC3DK
//
//  Created by Eagle Jones on 3/23/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#include "../sensor_clock.h"
#include <mach/mach_time.h>

//  Modeled after LLVM sources at https://llvm.org/svn/llvm-project/libcxx/trunk/src/chrono.cpp
//  Implement ourselves because we can't guarantee that libc++ steady_clock will always match sensor clock.

//   mach_absolute_time() * MachInfo.numer / MachInfo.denom is the number of
//   nanoseconds since the computer booted up.  MachInfo.numer and MachInfo.denom
//   are run time constants supplied by the OS.  This clock has no relationship
//   to the Gregorian calendar.  Its main use is as a high resolution timer.

// MachInfo.numer / MachInfo.denom is often 1 on the latest equipment.  Specialize
//   for that case as an optimization.

static sensor_clock::rep sensor_simplified()
{
    return static_cast<sensor_clock::rep>(mach_absolute_time());
}

static double compute_sensor_factor()
{
    mach_timebase_info_data_t MachInfo;
    mach_timebase_info(&MachInfo);
    return static_cast<double>(MachInfo.numer) / MachInfo.denom;
}

static sensor_clock::rep sensor_full()
{
    static const double factor = compute_sensor_factor();
    return static_cast<sensor_clock::rep>(mach_absolute_time() * factor);
}

typedef sensor_clock::rep (*FP)();

static FP init_sensor_clock()
{
    mach_timebase_info_data_t MachInfo;
    mach_timebase_info(&MachInfo);
    if (MachInfo.numer == MachInfo.denom)
        return &sensor_simplified;
    return &sensor_full;
}

sensor_clock::time_point sensor_clock::now() noexcept
{
    static FP fp = init_sensor_clock();
    return time_point(duration(fp()));
}
