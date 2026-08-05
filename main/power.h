/*
 * power.h — Power management (battery voltage, charging state, low-battery alerts)
 *
 * ADC is shared: GPIO34 is both battery voltage divider AND button A.
 * Since the divider is 9.1k/2.4k and flows into ADC1_CH6, sampling the
 * battery is non-disruptive as long as button A is not held at the same time.
 * Readings are averaged to mitigate noise.
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>

#define POWER_BATTERY_DIVIDER  4.791f   /* V_bat = V_adc * 4.791 */
#define POWER_BATTERY_FULL_V   4.20f    /* Li-ion full charge */
#define POWER_BATTERY_LOW_V    3.30f    /* low-battery threshold */
#define POWER_BATTERY_CRIT_V   3.10f    /* critical shutdown threshold */

/* Battery state (visual) */
typedef enum {
    POWER_BAT_UNKNOWN = 0,
    POWER_BAT_NORMAL,
    POWER_BAT_LOW,
    POWER_BAT_CRITICAL,
} power_bat_state_t;

void power_init(void);
float power_read_voltage(void);             /* raw, unfiltered */
float power_read_voltage_filtered(void);    /* moving-average filtered */
uint8_t power_read_percent(void);           /* 0..100 */
power_bat_state_t power_read_state(void);
void power_task(void *arg);                 /* FreeRTOS task body: periodic refresh */