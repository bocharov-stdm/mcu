#include "pico/stdlib.h"
#include "stdio.h"
#include "hardware/gpio.h"

#define DEVICE_NAME "pico p2040" 
#define DEVICE_VRSION "v1"  

const uint LED_PIN = 25;

int main()
{
    stdio_init_all();
    while (true)
    {
        char c = getchar();
        printf("Char is: %c , code: [%d]\n", c, c);
    }
}
