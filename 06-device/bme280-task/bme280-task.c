#include "pico/stdlib.h"
#include "stdio.h"
#include "bme280-task.h"
#include "bme280-driver.h"

// период вывода телеметрии
const uint64_t BME280_TASK_PERIOD_US = 100000;

static bme280_state_t STATE = BME280_TASK_STATE_IDLE;
static uint64_t task_ts;

void bme280_task_set_state(bme280_state_t state)
{
    STATE = state;
    if (state == BME280_TASK_STATE_RUN)
    {
        task_ts = time_us_64();
    }
}

void bme280_task_handle()
{
    if (STATE == BME280_TASK_STATE_IDLE)
        return;

    uint64_t now = time_us_64();

    if (now - task_ts >= BME280_TASK_PERIOD_US)
    {
        task_ts = now;

        float temp = bme280_get_temperature();
        float pressure = bme280_get_pressure();
        float humidity = bme280_get_humidity();

        // формат строки: "температура давление влажность" — его читает Python-скрипт
        printf("%f %f %f\n", temp, pressure, humidity);
    }
}