#include "stdio.h"
#include "stdlib.h"
#include "pico/stdlib.h"
#include "stdio-task/stdio-task.h"
#include "protocol-task.h"
#include "led-task/led-task.h"
#include "ili9341-driver.h"
#include "bme280-driver.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "ili9341-display.h"
#include "ili9341-font.h"
#include "bme280-task/bme280-task.h"

#define DEVICE_NAME "my-pico-device"
#define DEVICE_VRSN "v0.6.1"

static ili9341_display_t ili9341_display = {0};

#define GUI_SCREEN_W 320
#define GUI_SCREEN_H 240

#define HISTORY_SECONDS 60
#define DEFAULT_MEASURE_PERIOD_MS 1000

typedef struct
{
    float temperature;
    float pressure_hpa;
    float humidity;
    bool valid;
} atmosphere_measurement_t;

static atmosphere_measurement_t current_measurement = {0};

static float temp_history[HISTORY_SECONDS] = {0};
static float hum_history[HISTORY_SECONDS] = {0};
static float press_history[HISTORY_SECONDS] = {0};

static uint32_t history_count = 0;
static uint32_t history_pos = 0;

static uint32_t measure_period_ms = DEFAULT_MEASURE_PERIOD_MS;
static uint64_t measure_ts_us = 0;

static bool measurements_enabled = true;
static bool gui_initialized = false;

static void gui_draw(void);

#define ILI9341_PIN_MISO 4
#define ILI9341_PIN_CS 10
#define ILI9341_PIN_SCK 6
#define ILI9341_PIN_MOSI 7
#define ILI9341_PIN_DC 8
#define ILI9341_PIN_RESET 9
// #define PIN_LED -> 3.3V

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
    measurements_enabled = true;
    bme280_task_set_state(BME280_TASK_STATE_RUN);
    printf("measurements started\n");
}

void tm_stop_callback(const char* args)
{
    measurements_enabled = false;
    bme280_task_set_state(BME280_TASK_STATE_IDLE);
    printf("measurements stopped\n");

    // gui_task_handle больше не перерисовывает экран, поэтому показываем STOP сразу
    if (gui_initialized)
    {
        gui_draw();
    }
}

void tm_period_callback(const char* args)
{
    uint32_t period_ms = 0;
    int result = sscanf(args, "%u", &period_ms);

    if (result != 1 || period_ms < 500)
    {
        printf("invalid period. usage: tm_period 1000\n");
        printf("minimum period is 500 ms\n");
        return;
    }

    measure_period_ms = period_ms;
    printf("measurement period set to %lu ms\n", (unsigned long)measure_period_ms);
}

void disp_screen_callback(const char* args)
{
	uint32_t c = 0;
	int result = sscanf(args, "%x", &c);
	
	uint16_t color = COLOR_BLACK;
	
	if (result)
	{
		color = RGB888_2_RGB565(c);
	}
	
	ili9341_fill_screen(&ili9341_display, color);
}

void disp_px_callback(const char* args)
{
    uint32_t x, y, c;
    int result = sscanf(args, "%u %u %x", &x, &y, &c);
    
    if (result != 3)
    {
        printf("invalid arguments. usage: disp_px x y color_in_hex\n");
        return;
    }
    
    uint16_t color = RGB888_2_RGB565(c);
    
    ili9341_draw_pixel(&ili9341_display, x, y, color);
}

void disp_line_callback(const char* args)
{
    uint32_t x0, y0, x1, y1, c;
    int result = sscanf(args, "%u %u %u %u %x", &x0, &y0, &x1, &y1, &c);
    
    if (result != 5)
    {
        printf("invalid arguments. usage: disp_line x0 y0 x1 y1 color_in_hex\n");
        return;
    }
    
    uint16_t color = RGB888_2_RGB565(c);
    
    ili9341_draw_line(&ili9341_display, x0, y0, x1, y1, color);
}

void disp_rect_callback(const char* args)
{
    uint32_t x, y, w, h, c;
    int result = sscanf(args, "%u %u %u %u %x", &x, &y, &w, &h, &c);
    
    if (result != 5)
    {
        printf("invalid arguments. usage: disp_rect x y width height color_in_hex\n");
        return;
    }
    
    uint16_t color = RGB888_2_RGB565(c);
    
    ili9341_draw_rect(&ili9341_display, x, y, w, h, color);
}

void disp_frect_callback(const char* args)
{
    uint32_t x, y, w, h, c;
    int result = sscanf(args, "%u %u %u %u %x", &x, &y, &w, &h, &c);
    
    if (result != 5)
    {
        printf("invalid arguments. usage: disp_frect x y width height color_in_hex\n");
        return;
    }
    
    uint16_t color = RGB888_2_RGB565(c);
    
    ili9341_draw_filled_rect(&ili9341_display, x, y, w, h, color);
}

void disp_text_callback(const char* args)
{
    uint32_t x, y, c;
    char text[64];
    int result = sscanf(args, "%u %u %x %[^\n]", &x, &y, &c, text);
    
    if (result != 4)
    {
        printf("invalid arguments. usage: disp_text x y color_in_hex text\n");
        return;
    }
    
    uint16_t color = RGB888_2_RGB565(c);
    
    ili9341_draw_text(&ili9341_display, x, y, text, &jetbrains_font, color, COLOR_BLACK);
}

static uint16_t temperature_color(float t)
{
    if (t < 18.0f)
    {
        return COLOR_CYAN;
    }
    else if (t <= 26.0f)
    {
        return COLOR_GREEN;
    }
    else
    {
        return COLOR_RED;
    }
}

static uint16_t humidity_color(float h)
{
    if (h < 30.0f)
    {
        return COLOR_YELLOW;
    }
    else if (h <= 60.0f)
    {
        return COLOR_GREEN;
    }
    else
    {
        return COLOR_CYAN;
    }
}

static uint16_t pressure_color(float p)
{
    if (p < 990.0f)
    {
        return COLOR_YELLOW;
    }
    else if (p <= 1030.0f)
    {
        return COLOR_GREEN;
    }
    else
    {
        return COLOR_RED;
    }
}

static float clamp_float(float x, float min_value, float max_value)
{
    if (x < min_value)
    {
        return min_value;
    }

    if (x > max_value)
    {
        return max_value;
    }

    return x;
}

static void gui_draw_bar(
    uint16_t x,
    uint16_t y,
    uint16_t w,
    uint16_t h,
    float value,
    float min_value,
    float max_value,
    uint16_t color
)
{
    value = clamp_float(value, min_value, max_value);

    float k = (value - min_value) / (max_value - min_value);
    uint16_t filled_w = (uint16_t)(k * w);

    ili9341_draw_rect(&ili9341_display, x, y, w, h, COLOR_WHITE);

    if (filled_w > 2)
    {
        ili9341_draw_filled_rect(
            &ili9341_display,
            x + 1,
            y + 1,
            filled_w - 2,
            h - 2,
            color
        );
    }

    if (filled_w < w - 2)
    {
        ili9341_draw_filled_rect(
            &ili9341_display,
            x + 1 + filled_w,
            y + 1,
            w - filled_w - 2,
            h - 2,
            COLOR_BLACK
        );
    }
}

static void gui_draw_temperature_graph(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    ili9341_draw_rect(&ili9341_display, x, y, w, h, COLOR_WHITE);

    if (history_count < 2)
    {
        ili9341_draw_text(
            &ili9341_display,
            x + 10,
            y + 20,
            "waiting data...",
            &jetbrains_font,
            COLOR_WHITE,
            COLOR_BLACK
        );
        return;
    }

    float min_t = 10.0f;
    float max_t = 40.0f;

    uint32_t points = history_count;

    if (points > HISTORY_SECONDS)
    {
        points = HISTORY_SECONDS;
    }

    for (uint32_t i = 1; i < points; i++)
    {
        uint32_t idx0 = (history_pos + HISTORY_SECONDS - points + i - 1) % HISTORY_SECONDS;
        uint32_t idx1 = (history_pos + HISTORY_SECONDS - points + i) % HISTORY_SECONDS;

        float t0 = clamp_float(temp_history[idx0], min_t, max_t);
        float t1 = clamp_float(temp_history[idx1], min_t, max_t);

        uint16_t x0 = x + 1 + (uint16_t)((i - 1) * (w - 2) / (points - 1));
        uint16_t x1 = x + 1 + (uint16_t)(i * (w - 2) / (points - 1));

        uint16_t y0 = y + h - 2 - (uint16_t)((t0 - min_t) * (h - 4) / (max_t - min_t));
        uint16_t y1 = y + h - 2 - (uint16_t)((t1 - min_t) * (h - 4) / (max_t - min_t));

        ili9341_draw_line(&ili9341_display, x0, y0, x1, y1, COLOR_YELLOW);
    }
}

static void gui_draw(void)
{
    char buffer[64];

    ili9341_fill_screen(&ili9341_display, COLOR_BLACK);

    ili9341_draw_text(
        &ili9341_display,
        10,
        8,
        "06-device atmosphere meter",
        &jetbrains_font,
        COLOR_CYAN,
        COLOR_BLACK
    );

    ili9341_draw_line(&ili9341_display, 0, 28, 319, 28, COLOR_WHITE);

    if (!current_measurement.valid)
    {
        ili9341_draw_text(
            &ili9341_display,
            10,
            50,
            "No BME280 data yet",
            &jetbrains_font,
            COLOR_RED,
            COLOR_BLACK
        );
        return;
    }

    uint16_t t_color = temperature_color(current_measurement.temperature);
    uint16_t h_color = humidity_color(current_measurement.humidity);
    uint16_t p_color = pressure_color(current_measurement.pressure_hpa);

    snprintf(buffer, sizeof(buffer), "Temp:  %.2f C", current_measurement.temperature);
    ili9341_draw_text(&ili9341_display, 10, 40, buffer, &jetbrains_font, t_color, COLOR_BLACK);
    gui_draw_bar(150, 40, 150, 12, current_measurement.temperature, 0.0f, 40.0f, t_color);

    snprintf(buffer, sizeof(buffer), "Hum:   %.2f %%", current_measurement.humidity);
    ili9341_draw_text(&ili9341_display, 10, 62, buffer, &jetbrains_font, h_color, COLOR_BLACK);
    gui_draw_bar(150, 62, 150, 12, current_measurement.humidity, 0.0f, 100.0f, h_color);

    snprintf(buffer, sizeof(buffer), "Press: %.2f hPa", current_measurement.pressure_hpa);
    ili9341_draw_text(&ili9341_display, 10, 84, buffer, &jetbrains_font, p_color, COLOR_BLACK);
    gui_draw_bar(150, 84, 150, 12, current_measurement.pressure_hpa, 950.0f, 1050.0f, p_color);

    snprintf(buffer, sizeof(buffer), "Period: %lu ms", (unsigned long)measure_period_ms);
    ili9341_draw_text(&ili9341_display, 10, 108, buffer, &jetbrains_font, COLOR_WHITE, COLOR_BLACK);

    if (measurements_enabled)
    {
        ili9341_draw_text(&ili9341_display, 210, 108, "RUN", &jetbrains_font, COLOR_GREEN, COLOR_BLACK);
    }
    else
    {
        ili9341_draw_text(&ili9341_display, 210, 108, "STOP", &jetbrains_font, COLOR_RED, COLOR_BLACK);
    }

    ili9341_draw_text(
        &ili9341_display,
        10,
        132,
        "Temperature graph, last 60 samples",
        &jetbrains_font,
        COLOR_WHITE,
        COLOR_BLACK
    );

    gui_draw_temperature_graph(10, 152, 300, 75);
}

static void gui_measure_once(void)
{
    float temperature = bme280_get_temperature();
    float pressure_hpa = bme280_get_pressure() / 100.0f;
    float humidity = bme280_get_humidity();

    current_measurement.temperature = temperature;
    current_measurement.pressure_hpa = pressure_hpa;
    current_measurement.humidity = humidity;
    current_measurement.valid = true;

    temp_history[history_pos] = temperature;
    hum_history[history_pos] = humidity;
    press_history[history_pos] = pressure_hpa;

    history_pos = (history_pos + 1) % HISTORY_SECONDS;

    if (history_count < HISTORY_SECONDS)
    {
        history_count++;
    }
}

static void gui_task_handle(void)
{
    if (!measurements_enabled)
    {
        return;
    }

    uint64_t now_us = time_us_64();

    if (now_us - measure_ts_us >= measure_period_ms * 1000ULL)
    {
        measure_ts_us = now_us;

        gui_measure_once();
        gui_draw();
    }
}

api_t device_api[] =
{
    {"help", help_callback, "print this help message"}, 
	{"version", version_callback, "get device name and firmware version"},
    {"led_on", led_on_callback, "turn on the LED"},
    {"led_off", led_off_callback, "turn off the LED"},
    {"led_blink", led_blink_callback, "set the LED to blink"},
    {"led_period", led_blink_set_period_ms_callback, "set the LED blink period in milliseconds"},
    {"mem", mem_callback, "dump memory at specified hex address, usage: mem 0x3FF00000"},
    {"wmem", wmem_callback, "write value to memory at specified hex address, usage: wmem 0x3FF00000 0x12345678"},
    {"disp_screen", disp_screen_callback, "color the whole screen with specified RGB888 color in hex, usage: disp_screen 0xFF0000 (red), disp_screen 0x00FF00 (green), disp_screen 0x0000FF (blue), default is black: disp_screen"},
    {"disp_px", disp_px_callback, "draw a pixel at specified coordinates with specified RGB888 color in hex, usage: disp_px 100 100 0xFF0000"},
    {"disp_line", disp_line_callback, "draw a line between two points with specified RGB888 color in hex, usage: disp_line 10 10 100 100 0xFF0000"},
    {"disp_rect", disp_rect_callback, "draw a rectangle at specified coordinates with specified RGB888 color in hex, usage: disp_rect 10 10 100 100 0xFF0000"},
    {"disp_frect", disp_frect_callback, "draw a filled rectangle at specified coordinates with specified RGB888 color in hex, usage: disp_frect 10 10 100 100 0xFF0000"},
    {"disp_text", disp_text_callback, "draw text at specified coordinates with specified RGB888 color in hex, usage: disp_text 10 10 0xFF0000 Hello, World!"},
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
    {"tm_period", tm_period_callback, "set measurement period in ms, usage: tm_period 1000"},
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

void rp2040_spi_write(const uint8_t *data, uint32_t size)
{
	spi_write_blocking(spi0, data, size);
}

void rp2040_spi_read(uint8_t *buffer, uint32_t length)
{
	spi_read_blocking(spi0, 0, buffer, length);
}

void rp2040_gpio_cs_write(bool level)
{
	gpio_put(ILI9341_PIN_CS, level);
}

void rp2040_gpio_dc_write(bool level)
{
    gpio_put(ILI9341_PIN_DC, level);
}

void rp2040_gpio_reset_write(bool level)
{
    gpio_put(ILI9341_PIN_RESET, level);
}

void rp2040_delay_ms(uint32_t ms)
{
    sleep_ms(ms);
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

    spi_init(spi0, 10 * 1000 * 1000); // 10 MHz

    gpio_set_function(ILI9341_PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(ILI9341_PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(ILI9341_PIN_MOSI, GPIO_FUNC_SPI);

    gpio_init(ILI9341_PIN_CS);
    gpio_set_dir(ILI9341_PIN_CS, GPIO_OUT);
    gpio_init(ILI9341_PIN_DC);
    gpio_set_dir(ILI9341_PIN_DC, GPIO_OUT);
    gpio_init(ILI9341_PIN_RESET);
    gpio_set_dir(ILI9341_PIN_RESET, GPIO_OUT);

    gpio_put(ILI9341_PIN_CS, 1);
    gpio_put(ILI9341_PIN_DC, 0);
    gpio_put(ILI9341_PIN_RESET, 0);

    ili9341_hal_t ili9341_hal = {0};
    ili9341_hal.spi_write = rp2040_spi_write;
    ili9341_hal.spi_read = rp2040_spi_read;
    ili9341_hal.gpio_cs_write = rp2040_gpio_cs_write;
    ili9341_hal.gpio_dc_write = rp2040_gpio_dc_write;

    ili9341_hal.gpio_reset_write = rp2040_gpio_reset_write;
    ili9341_hal.delay_ms = rp2040_delay_ms;

    ili9341_init(&ili9341_display, &ili9341_hal);
    ili9341_set_rotation(&ili9341_display, ILI9341_ROTATION_90);
   ili9341_fill_screen(&ili9341_display, COLOR_BLACK);

    ili9341_draw_text(
        &ili9341_display,
        10,
        10,
        "06-device starting...",
        &jetbrains_font,
        COLOR_WHITE,
        COLOR_BLACK
    );

    sleep_ms(500);

    gui_measure_once();
    gui_draw();

    measure_ts_us = time_us_64();
    gui_initialized = true;

    while (1)
    {   
        led_task_handle();
        protocol_task_handle(stdio_task_handle());
        bme280_task_handle();  
        
        gui_task_handle(); 
        
    }   
}
