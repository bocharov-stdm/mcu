#include "led-task.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"


const uint LED_PIN = 25;
uint32_t LED_BLINK_PERIOD_US = 500000;

uint64_t led_ts;
led_state_t led_state;

void led_task_init()
{
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    led_state = LED_STATE_OFF;
    led_ts = time_us_64();
}

void led_task_handle()
{
    uint64_t now = time_us_64();

    switch (led_state)
    {
    case LED_STATE_OFF:
        gpio_put(LED_PIN, 0);
        break;
    case LED_STATE_ON:
        gpio_put(LED_PIN, 1);
        break;
    case LED_STATE_BLINK:
        if (now - led_ts >= LED_BLINK_PERIOD_US)
        {
            gpio_put(LED_PIN, !gpio_get(LED_PIN));
            led_ts = now;
        }
        break;
    default:
        break;
    }
}

void led_task_set_state(led_state_t state)
{
    led_state = state;
}

void led_task_set_blink_period(uint32_t period_ms)
{
    LED_BLINK_PERIOD_US = period_ms * 1000 / 2;
}

