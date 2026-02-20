#include "stdio.h"
#include "pico/stdlib.h"
#include "stdio-task/stdio-task.h"
#include "protocol-task/protocol-task.h"
#include "led-task/led-task.h"

#define DEVICE_NAME "Pico"
#define DEVICE_VRSN "1.0.0" // firmware version

#define RAM_START 0x20000000
#define RAM_END   0x20040000  // 256KB RAM
#define SCRATCH_X_START 0x20040000
#define SCRATCH_X_END   0x20041000  // 4KB
#define SCRATCH_Y_START 0x20041000
#define SCRATCH_Y_END   0x20042000  // 4KB

bool is_valid_address(uint32_t addr) {
    return (addr >= RAM_START && addr < RAM_END) ||
           (addr >= SCRATCH_X_START && addr < SCRATCH_X_END) ||
           (addr >= SCRATCH_Y_START && addr < SCRATCH_Y_END);
}

// Прототипы функций
void version_callback(const char* args);
void led_on_callback(const char* args);
void led_off_callback(const char* args);
void led_blink_callback(const char* args);
void led_blink_set_period_ms_callback(const char* args);
void help_callback(const char* args);
void mem_callback(const char* args);
void wmem_callback(const char* args);

uint32_t mem(uint32_t addr)
{
    return *(volatile uint32_t*)addr;
}

void wmem(uint32_t addr, uint32_t data)
{
    *(volatile uint32_t*)addr = data;
}

api_t device_api[] =
{
	{"version", version_callback, "get device name and firmware version"},
    {"on", led_on_callback, "led on"},
    {"off", led_off_callback, "led off"},
    {"blink", led_blink_callback, "led blink"},
    {"blink_set_period_ms", led_blink_set_period_ms_callback, "set blink period in ms"},
    {"help", help_callback, "print this help"},
    {"mem", mem_callback, "get data att address"},
    {"wmem", wmem_callback, "write data to address"},
	{NULL, NULL, NULL},
};


void version_callback(const char* args)
{
	printf("device name: '%s', firmware version: %s\n", DEVICE_NAME, DEVICE_VRSN);
}

void led_on_callback(const char* args)
{
    led_task_state_set(LED_STATE_ON);
}

void led_off_callback(const char* args)
{
    led_task_state_set(LED_STATE_OFF);
}

void led_blink_callback(const char* args)
{
    led_task_state_set(LED_STATE_BLINK);
}

void led_blink_set_period_ms_callback(const char* args)
{
    uint period_ms = 0;
    sscanf(args, "%u", &period_ms);
    if (period_ms <= 0)
    {
        printf("invalid period\n");
        return;
    }

    led_task_set_blink_period_ms(period_ms);
}

void help_callback(const char* args)
{
    for (int i = 0; device_api[i].command_name != NULL; i++)
    {
        printf("%s - %s\n", device_api[i].command_name, device_api[i].command_help);
    }
}

void mem_callback(const char* args)
{
    uint32_t addr;
    sscanf(args, "%u", &addr);
    if (addr == 0 || addr % 4 != 0)
    {
        printf("invalid address\n");
        return;
    }
    printf("%u\n", mem(addr));
}

void wmem_callback(const char* args)
{
    uint32_t addr, value;
    sscanf(args, "%u %u", &addr, &value);
    if (addr == 0 || addr % 4 != 0)
    {
        printf("invalid address\n");
        return;
    }
    wmem(addr, value);
}

int main() 
{
    stdio_init_all();
    protocol_task_init(device_api);
    stdio_task_init();
    // led_task_init();
    while (1)
    {
        char* command = stdio_task_handle();
        if (command != NULL)
        {
            protocol_task_handle(command);
        }
        led_task_handle();
    }
}

