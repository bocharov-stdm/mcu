#include "stdio.h"
#include "stdlib.h"
#include "pico/stdlib.h"
#include "stdio-task/stdio-task.h"
#include "protocol-task.h"
#include "led-task/led-task.h"
#include "ili9341-driver.h"
#include "hardware/spi.h"
#include "ili9341-display.h"
#include "ili9341-font.h"

#define DEVICE_NAME "my-pico-device"
#define DEVICE_VRSN "v0.0.1"

static ili9341_display_t ili9341_display = {0};

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
    {"disp_screen", disp_screen_callback, "color the whole screen with specified RGB888 color in hex, usage: disp_screen 0xFF0000 (red), disp_screen 0x00FF00 (green), disp_screen 0x0000FF (blue), default is black: disp_screen"},
    {"disp_px", disp_px_callback, "draw a pixel at specified coordinates with specified RGB888 color in hex, usage: disp_px 100 100 0xFF0000"},
    {"disp_line", disp_line_callback, "draw a line between two points with specified RGB888 color in hex, usage: disp_line 10 10 100 100 0xFF0000"},
    {"disp_rect", disp_rect_callback, "draw a rectangle at specified coordinates with specified RGB888 color in hex, usage: disp_rect 10 10 100 100 0xFF0000"},
    {"disp_frect", disp_frect_callback, "draw a filled rectangle at specified coordinates with specified RGB888 color in hex, usage: disp_frect 10 10 100 100 0xFF0000"},
    {"disp_text", disp_text_callback, "draw text at specified coordinates with specified RGB888 color in hex, usage: disp_text 10 10 0xFF0000 Hello, World!"},
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

int main(){
    led_task_init();

    stdio_init_all();

    stdio_task_init();

    protocol_task_init(device_api);

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
    sleep_ms(300);
    ili9341_draw_filled_rect(&ili9341_display, 10, 10, 100, 60, COLOR_RED);
    ili9341_draw_filled_rect(&ili9341_display, 120, 10, 100, 60, COLOR_GREEN);
    ili9341_draw_filled_rect(&ili9341_display, 230, 10, 80, 60, COLOR_BLUE);
    ili9341_draw_rect(&ili9341_display, 10, 90, 300, 80, COLOR_WHITE);
    ili9341_draw_line(&ili9341_display, 0, 0, 319, 239, COLOR_YELLOW);
    ili9341_draw_line(&ili9341_display, 319, 0, 0, 239, COLOR_CYAN);
    ili9341_draw_text(&ili9341_display, 20, 100, "Hello, ILI9341!", &jetbrains_font, COLOR_WHITE, COLOR_BLACK);
    ili9341_draw_text(&ili9341_display, 20, 116, "RP2040 / Pico SDK", &jetbrains_font, COLOR_YELLOW, COLOR_BLACK);
    sleep_ms(300);

    while (1)
    {   
        led_task_handle();
        protocol_task_handle(stdio_task_handle());

        
    }   
}
