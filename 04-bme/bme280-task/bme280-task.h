#pragma once

typedef enum
{
    BME280_TASK_STATE_IDLE = 0,
    BME280_TASK_STATE_RUN = 1,
} bme280_state_t;

void bme280_task_set_state(bme280_state_t state);
void bme280_task_handle();