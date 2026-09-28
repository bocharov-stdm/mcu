#include "stdio.h"
#include "stdlib.h"
#include "pico/stdlib.h"
#include "stdio-task/stdio-task.h"
#include "protocol-task/protocol-task.h"
#include "led-task/led-task.h"
#include "adc-task/adc-task.h"

#define DEVICE_NAME "my-pico-device"
#define DEVICE_VRSN "v0.0.1"

uint32_t mem(uint32_t addr)
{
    return *(volatile uint32_t*)addr;
}

void wmem(uint32_t addr, uint32_t data)
{
    *(volatile uint32_t*)addr = data;
}

void version_callback(const char* args)
{
	printf("device name: '%s', firmware version: %s\n", DEVICE_NAME, DEVICE_VRSN);
}

void led_on_callback(const char* args)
{
    led_task_set_state(LED_STATE_ON);
}

void led_off_callback(const char* args)
{
    led_task_set_state(LED_STATE_OFF);
}  

void led_blink_callback(const char* args)
{
    led_task_set_state(LED_STATE_BLINK);
}

void led_blink_set_period_ms_callback(const char* args)
{
    uint32_t period_ms = 0;
    sscanf(args, "%u", &period_ms);
    if (period_ms == 0)
    {
        printf("invalid period_ms value: '%s'\n", args);
        return;
    }
    led_task_set_blink_period(period_ms);
}

void help_callback(const char* args);

void mem_callback(const char* args)
{
    uint32_t addr;
    sscanf(args, "%x", &addr);
    printf("0x%08lx\n", (unsigned long)mem(addr));
}

void wmem_callback(const char* args)
{
    uint32_t addr, value;
    sscanf(args, "%x %x", &addr, &value);

    if (addr == 0 || addr % 4 != 0)
    {
        printf("invalid address\n");
        return;
    }
    wmem(addr, value);
}

void get_adc_callback(const char* args)
{
    float voltage_V = adc_task_read_voltage();
    printf("%f\n", voltage_V);
}

void get_temp_callback(const char* args)
{
    float temp_C = adc_task_read_temperature();
    printf("%f\n", temp_C);
}

void tm_start_callback(const char* args)
{
    adc_task_set_state(ADC_TASK_STATE_RUN);
}

void tm_stop_callback(const char* args)
{
    adc_task_set_state(ADC_TASK_STATE_IDLE);
}

api_t device_api[] =
{
    {"help", help_callback, "print this help message"}, 
	{"version", version_callback, "get device name and firmware version"},
    {"led_on", led_on_callback, "turn on the LED"},
    {"led_off", led_off_callback, "turn off the LED"},
    {"led_blink", led_blink_callback, "set the LED to blink"},
    {"set_period", led_blink_set_period_ms_callback, "set the LED blink period in milliseconds"},
    {"mem", mem_callback, "dump memory at specified hex address, usage: mem 0x3FF00000"},
    {"wmem", wmem_callback, "write value to memory at specified hex address, usage: wmem 0x3FF00000 0x12345678"},
    {"get_adc", get_adc_callback, "read voltage from ADC and print it"},
    {"get_temp", get_temp_callback, "read temperature from internal sensor and print it"},
    {"tm_start", tm_start_callback, "start the temperature monitoring task"},
    {"tm_stop", tm_stop_callback, "stop the temperature monitoring task"},
    {NULL, NULL, NULL},
};

void help_callback(const char* args)
{
    printf("available commands:\n");
    for (int i = 0; device_api[i].command_name != NULL; i++)
    {
        printf("Комманда  %s: %s\n", device_api[i].command_name, device_api[i].command_help);
    }
}

int main(){
    led_task_init();

    stdio_init_all();

    stdio_task_init();

    adc_task_init();

    protocol_task_init(device_api);

    while (1)
    {   
        led_task_handle();
        protocol_task_handle(stdio_task_handle());
        adc_task_handle();
    }   
}
