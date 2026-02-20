#include "pico/stdlib.h"
#include "stdio.h"
#include "hardware/gpio.h"

#define DEVICE_NAME "nosok_pico"  # Поменяйте
#define DEVICE_VRSION "v2.2.8"  # Тем более поменяйте

const uint LED_PIN = 25;

int main()
{
    stdio_init_all();
    while (true)
    {
        char c = getchar();
        printf("Received char: %c [%d]\n", c, c);
    }
}
