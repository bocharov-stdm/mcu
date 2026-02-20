#include "pico/stdlib.h"
#include "stdio.h"
#include "hardware/gpio.h"

#define DEVICE_NAME "This name was promised 3000 years ago"
#define DEVICE_VRSN "V2"

const uint LED_PIN = 25;

int main()
{
    stdio_init_all();
    while (true)
    {
        printf("Device name: '%s', Version: '%s'\n", DEVICE_NAME, DEVICE_VRSN);
        sleep_ms(1000);
    }
}
