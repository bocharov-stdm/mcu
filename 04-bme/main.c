#include "stdio.h"
#include "stdlib.h"
#include "pico/stdlib.h"
#include "stdio-task/stdio-task.h"
#include "protocol-task.h"
#include "bme280-driver.h"
#include "led-task/led-task.h"
#include "hardware/i2c.h"
#include "bme280-task/bme280-task.h"

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

void read_regs_callback(const char* args)
{
    uint32_t start_reg_address, N;
    sscanf(args, "%x %x", &start_reg_address, &N);

    if (N > 0xFF)
    {
        printf("invalid length\n");
        return;
    }

    if (start_reg_address > 0xFF)
    {
        printf("invalid register range\n");
        return;
    }   

    uint8_t buffer[256] = {0};
    bme280_read_regs(start_reg_address, buffer, N);
    printf("read data:");
    for (int i = 0; i < N; i++)
    {
        printf("bme280 register [0x%X] = 0x%X\n", start_reg_address + i, buffer[i]);
    }
    printf("\n");
}

void write_regs_callback(const char* args)
{
    uint32_t reg_address, value;
    sscanf(args, "%x %x", &reg_address, &value);

    if (reg_address > 0xFF)
    {
        printf("invalid register address\n");
        return;
    }

    if (value > 0xFF)
    {
        printf("invalid value\n");
        return;
    }
    bme280_write_reg(reg_address, value);
    printf("wrote 0x%X to bme280 register [0x%X]\n", value, reg_address);
}

void bme280_read_temp_callback(const char* args)
{
    uint32_t temp_raw = bme280_read_temp_raw();
    printf("bme280 raw temperature: %lu\n", (unsigned long)temp_raw);
}

void bme280_read_pressure_callback(const char* args)
{
    uint32_t pressure_raw = bme280_read_pressure_raw();
    printf("bme280 raw pressure: %lu\n", (unsigned long)pressure_raw);
}

void bme280_read_humidity_callback(const char* args)
{
    uint16_t humidity_raw = bme280_read_humidity_raw();
    printf("bme280 raw humidity: %u\n", humidity_raw);
}

void bme280_read_temp_CIE_callback(const char* args)
{
    float temp_CIE = bme280_get_temperature();
    printf("bme280 temperature: %.2f °C\n", temp_CIE);
}

void bme280_read_pressure_hPa_callback(const char* args)
{
    float pressure_hPa = bme280_get_pressure() / 100.0;
    printf("bme280 pressure: %.2f hPa\n", pressure_hPa);
}

void bme280_read_humidity_percent_callback(const char* args)
{
    float humidity_percent = bme280_get_humidity();
    printf("bme280 humidity: %.2f %%\n", humidity_percent);
}

void tm_start_callback(const char* args)
{
    bme280_task_set_state(BME280_TASK_STATE_RUN);
}

void tm_stop_callback(const char* args)
{
    bme280_task_set_state(BME280_TASK_STATE_IDLE);
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
    {"read_regs", read_regs_callback, "read register value, usage: read_reg 0x10 0x05"},
    {"write_reg", write_regs_callback, "write register value, usage: write_reg 0x10 0x05"},
    {"temp_raw", bme280_read_temp_callback, "read raw temperature value"},
    {"pres_raw", bme280_read_pressure_callback, "read raw pressure value"},
    {"hum_raw", bme280_read_humidity_callback, "read raw humidity value"},
    {"temp", bme280_read_temp_CIE_callback, "read temperature in °C"},
    {"pres", bme280_read_pressure_hPa_callback, "read pressure in hPa"},
    {"hum", bme280_read_humidity_percent_callback, "read humidity in %"},
    {"tm_start", tm_start_callback, "start temperature measurement"},
    {"tm_stop", tm_stop_callback, "stop temperature measurement"},

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

void rp2040_i2c_read(uint8_t* buffer, uint16_t length)
{
	i2c_read_timeout_us(i2c1, 0x76, buffer, length, false, 100000);
}

void rp2040_i2c_write(uint8_t* data, uint16_t size)
{
	i2c_write_timeout_us(i2c1, 0x76, data, size, false, 100000);
}



int main(){
    led_task_init();

    stdio_init_all();

    stdio_task_init();

    protocol_task_init(device_api);

    i2c_init(i2c1, 100000);

    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);   

    bme280_init(rp2040_i2c_read, rp2040_i2c_write);

    while (1)
    {   
        led_task_handle();
        protocol_task_handle(stdio_task_handle());
        bme280_task_handle();
        
    }   
}
