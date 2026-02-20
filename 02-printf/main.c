#include "pico/stdlib.h"
#include "stdio.h"
#include "hardware/gpio.h"

#define DEVICE_NAME "nosok_pico"
#define DEVICE_VRSN "v2.2.8"

const uint LED_PIN = 25;

int main()
{
    stdio_init_all();
    while (true)
    {
        printf("Device name: '%s'\n", DEVICE_NAME);
        sleep_ms(1000);
    }
}
