// Copyright 2015 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef _WIN32
# error time_win32.c is only intended to be used with win32 based systems
#endif  // _WIN32

#ifdef __cplusplus
extern "C"
{
#endif

#include "rcutils/time.h"

#include <windows.h>

#include "./common.h"
#include "rcutils/allocator.h"
#include "rcutils/error_handling.h"

rcutils_ret_t
rcutils_system_time_now(rcutils_time_point_value_t * now)
{
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(
    now, RCUTILS_RET_INVALID_ARGUMENT, rcutils_get_default_allocator());
  FILETIME ft;
  GetSystemTimePreciseAsFileTime(&ft);
  LARGE_INTEGER li;
  li.LowPart = ft.dwLowDateTime;
  li.HighPart = ft.dwHighDateTime;
  // Adjust for January 1st, 1970, see:
  //   https://support.microsoft.com/en-us/kb/167296
  li.QuadPart -= 116444736000000000;
  // Convert to nanoseconds from 100's of nanoseconds.
  *now = li.QuadPart * 100;
  return RCUTILS_RET_OK;
}

rcutils_ret_t
rcutils_steady_time_now(rcutils_time_point_value_t * now)
{
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(
    now, RCUTILS_RET_INVALID_ARGUMENT, rcutils_get_default_allocator());
  LARGE_INTEGER cpu_frequency, performance_count;
  // These should not ever fail since XP is already end of life:
  // From https://msdn.microsoft.com/en-us/library/windows/desktop/ms644905(v=vs.85).aspx and
  //      https://msdn.microsoft.com/en-us/library/windows/desktop/ms644904(v=vs.85).aspx:
  // "On systems that run Windows XP or later, the function will always succeed and will
  //  thus never return zero."
  QueryPerformanceFrequency(&cpu_frequency);
  QueryPerformanceCounter(&performance_count);
  // Calculate nanoseconds and seconds separately because
  // otherwise overflow can happen in intermediate calculations
  // This conversion will overflow if the PC runs >292 years non-stop
  const rcutils_time_point_value_t whole_seconds =
    performance_count.QuadPart / cpu_frequency.QuadPart;
  const rcutils_time_point_value_t remainder_count =
    performance_count.QuadPart % cpu_frequency.QuadPart;
  const rcutils_time_point_value_t remainder_ns =
    RCUTILS_S_TO_NS(remainder_count) / cpu_frequency.QuadPart;
  const rcutils_time_point_value_t total_ns =
    RCUTILS_S_TO_NS(whole_seconds) + remainder_ns;
  *now = total_ns;
  return RCUTILS_RET_OK;
}

#ifdef __cplusplus
}
#endif
