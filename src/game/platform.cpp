#include "platform.h"

namespace lf {

// Howard Hinnant's civil-days algorithms, offset so that 2000-01-01 == day 0.
static int32_t daysFromCivil(int y, unsigned m, unsigned d) {
  y -= m <= 2;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = (unsigned)(y - era * 400);
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + (int)doe - 719468;
}

uint32_t toEpoch(const DateTime& dt) {
  static const int32_t base = daysFromCivil(2000, 1, 1);
  int32_t days = daysFromCivil(dt.year, dt.month, dt.day) - base;
  if (days < 0) days = 0;
  return (uint32_t)days * 86400u + dt.hour * 3600u + dt.minute * 60u + dt.second;
}

DateTime fromEpoch(uint32_t e) {
  static const int32_t base = daysFromCivil(2000, 1, 1);
  int32_t z = (int32_t)(e / 86400u) + base + 719468;
  uint32_t sod = e % 86400u;
  const int era = (z >= 0 ? z : z - 146096) / 146097;
  const unsigned doe = (unsigned)(z - era * 146097);
  const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  int y = (int)yoe + era * 400;
  const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  const unsigned mp = (5 * doy + 2) / 153;
  const unsigned d = doy - (153 * mp + 2) / 5 + 1;
  const unsigned m = mp < 10 ? mp + 3 : mp - 9;
  y += m <= 2;
  DateTime dt;
  dt.year = (uint16_t)y;
  dt.month = (uint8_t)m;
  dt.day = (uint8_t)d;
  dt.hour = (uint8_t)(sod / 3600);
  dt.minute = (uint8_t)((sod % 3600) / 60);
  dt.second = (uint8_t)(sod % 60);
  return dt;
}

}  // namespace lf
