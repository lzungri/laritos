#pragma once

#include <time/time.h>

/**
 * Converts the calendar time to local broken-down time
 *
 * Note: This function was imported from Linux and adapted to laritos
 *
 * @param secs: the number of seconds elapsed since 00:00:00 on January 1, 1970,
 *              Coordinated Universal Time (UTC).
 * @return: 0 on success, <0 on error
 */
int epoch_to_utc_calendar(const uint64_t secs, calendar_t *c);
int epoch_to_localtime_calendar(const uint64_t secs, calendar_t *c);
