#include <log.h>

#include <time/time.h>
#include <core.h>
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
