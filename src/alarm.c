#include <string.h>
#include <init.h>
#include "calendar.h"
#include "clock.h"
#include "motor.h"
#include "state.h"
#include "alarm.h"

#define PULSE_TIME_US (250*1000)
#define PULSE_TIMES 3
#define PULSE_INTERVAL K_SECONDS(5)

struct k_work alarm_work;
static alarm_struct_t alarm_time;
static bool alarm_enabled = 0;

extern struct g_state state;

struct k_timer alarm_timer;
static void alarm_timer_callback(struct k_timer *timer_id)
{
	k_work_submit(&alarm_work);
}
K_TIMER_DEFINE(alarm_timer, alarm_timer_callback, NULL);

static void ring(struct k_work *item)
{
	ARG_UNUSED(item);
	motor_pulse_single(PULSE_TIME_US, PULSE_TIMES);
}

void alarm_set(alarm_struct_t * p_alarm_time)
{
	memcpy(&alarm_time, p_alarm_time, sizeof(alarm_time));
}

void alarm_get(alarm_struct_t * p_alarm_time)
{
	memcpy(p_alarm_time, &alarm_time, sizeof(alarm_time));
}

bool alarm_check(void)
{
	if (!alarm_enabled)
		return false;

	if (!(cal_get_weekday(NULL) & alarm_time.days))
		return false;

	time_struct_t *p_time = clock_get_time_p();
	if ((p_time->hours == alarm_time.time.hours &&
	     (p_time->minutes ==
	      alarm_time.time.minutes + alarm_time.snooze_minutes))) {
		/* Show alarm screen */
		state.next = 1;
		state.pgm_state = PGM_STATE_ALARM_RING;
		/* Ring alarm (motor) */
		k_work_submit(&alarm_work);
		k_timer_start(&alarm_timer, PULSE_INTERVAL, PULSE_INTERVAL);
		return true;
	}

	return false;
}

void alarm_enable(bool enable)
{
	alarm_enabled = enable;
}

bool alarm_is_enabled(void)
{
	return alarm_enabled;
}

void alarm_snooze(uint8_t minutes) {
	/* Porbably should handle rollover n stuff, oh well */
	alarm_time.snooze_minutes += minutes;
	/* Cut the power to the motor */
	k_timer_stop(&alarm_timer);
}

void alarm_stop() {
	alarm_time.snooze_minutes = 0;
	/* Cut the power to the motor */
	k_timer_stop(&alarm_timer);
}

static int alarm_init(const struct device *dev) {
	k_work_init(&alarm_work, ring);

	return 0;
}
SYS_INIT(alarm_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
