#include "hardware/adc.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "stdio.h"
#include "adc-task.h"

const uint ADC_PIN = 26;
const uint ADC_CHANNEL = 0;
const uint ADC_TEMP_CHANNEL = 4;
const uint64_t ADC_TASK_PERIOD_US = 100000; 

adc_state_t ADC_STATE = ADC_TASK_STATE_IDLE;
uint64_t adc_task_ts;


void adc_task_init()
{
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_set_temp_sensor_enabled(true);
}

float adc_task_read_voltage()
{
    adc_select_input(ADC_CHANNEL);
    uint16_t voltage_counts = adc_read();
    float voltage = (float)voltage_counts * 3.3f / 4095.0f; // 12-bit ADC
    return voltage;
}

float adc_task_read_temperature()
{
    adc_select_input(ADC_TEMP_CHANNEL);
    uint16_t temp_counts = adc_read();
    float temp_V = (float)temp_counts * 3.3f / 4095.0f;
    float temp_C = 27.0f - (temp_V - 0.706f) / 0.001721f;
    return temp_C;
}

void adc_task_set_state(adc_state_t state)
{
    ADC_STATE = state;
    if (state == ADC_TASK_STATE_RUN)
    {
        adc_task_ts = time_us_64();
    }
}

void adc_task_handle()
{
    if (ADC_STATE == ADC_TASK_STATE_IDLE)
        return;
    uint64_t now = time_us_64();
    if (now - adc_task_ts >= ADC_TASK_PERIOD_US)
    {
        adc_task_ts = now;
        float voltage_V = adc_task_read_voltage();
        float temp_C = adc_task_read_temperature();

        printf("%f %f\n", voltage_V, temp_C);
        
    }
}