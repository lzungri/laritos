#include <log.h>

#include <core.h>
#include <time/time.h>
#include <time/timeconv.h>
#include <component/timer.h>
#include <component/component.h>

int rtc_gettime(time_t *t) {
    component_t *c;
    for_each_filtered_component(c, c->type == COMP_TYPE_RTC) {
        timer_comp_t *timer = (timer_comp_t *) c;
        t->ns = 0;
        return timer->ops.get_value(timer, &t->secs);
    }
    return -1;
}

int rtc_get_localtime_calendar(calendar_t *c) {
    time_t time = { 0 };
    if (rtc_gettime(&time) < 0) {
        return -1;
    }
    return epoch_to_localtime_calendar(time.secs, c);
}

int set_timezone(timezone_t t, bool daylight) {
    _laritos.timeinfo.tz = t;
    _laritos.timeinfo.dst = daylight;
    return 0;
}

int get_localtime_offset(void) {
    int secs = _laritos.timeinfo.tz;
    secs += _laritos.timeinfo.dst ? 1 : 0;
    secs *= SECS_PER_HOUR;
    return secs;
}
