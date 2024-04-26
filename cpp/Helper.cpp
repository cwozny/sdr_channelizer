#include "Helper.h"

#include <ctime>
#include <cstdio>

void getFilenameStr(const std::chrono::system_clock::time_point now, char* filenameStr, const int filenameLength)
{
  // Convert current time from chrono to time_t which goes down to second precision
  const std::time_t tt = std::chrono::system_clock::to_time_t(now);
  // Convert back to chrono so that we have a current time rounded to seconds
  const std::chrono::system_clock::time_point nowSec = std::chrono::system_clock::from_time_t(tt);
  tm utc_tm = *gmtime(&tt);

  std::uint16_t year  = utc_tm.tm_year + 1900;
  std::uint8_t month  = utc_tm.tm_mon + 1;
  std::uint8_t day    = utc_tm.tm_mday;
  std::uint8_t hour   = utc_tm.tm_hour;
  std::uint8_t minute = utc_tm.tm_min;
  std::uint8_t second = utc_tm.tm_sec;
  std::uint16_t millisecond = (now-nowSec)/std::chrono::milliseconds(1);

  snprintf(filenameStr, filenameLength, "%04d_%02d_%02d_%02d_%02d_%02d_%03d.iq", year, month, day, hour, minute, second, millisecond);
}
