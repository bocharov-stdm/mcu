#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"

#define DEVICE_NAME "my-pico-device"
#define DEVICE_VRSN "v0.0.1"

uint32_t global_variable = 0;
const uint32_t constant_variable = 42;

int main()
{
    stdio_init_all();

    while (true)
    {
        // 4) строковая константа
        printf("Hello World!\n");

        // 5) строка с аргументами
        printf("device name: '%s', firmware version: %s\n", DEVICE_NAME, DEVICE_VRSN);

        // 6) timestamp
        uint64_t timestamp = time_us_64();
        printf("system timestamp: %llu us\n", (unsigned long long)timestamp);

        printf("--------------------------------------------------------------\n");

        // 7) stack variable
        uint32_t stack_variable = 8888;
        printf("stack variable  | addr = 0x%X | value = %u\n",
               (unsigned int)(uintptr_t)&stack_variable,
               (unsigned int)*(&stack_variable));

        printf("stack variable  | addr = 0x%X | value = %X\n",
               (unsigned int)(uintptr_t)&stack_variable,
               (unsigned int)*(&stack_variable));

        printf("stack variable  | addr = 0x%X | value = 0x%X\n",
               (unsigned int)(uintptr_t)&stack_variable,
               (unsigned int)*(&stack_variable));

        // 8) global variable
        global_variable++;
        printf("global variable | addr = 0x%X | value = %u\n",
               (unsigned int)(uintptr_t)&global_variable,
               (unsigned int)*(&global_variable));

        // 9) heap variable (СПЕЦИАЛЬНО без free)
        uint32_t* heap_variable = (uint32_t*)malloc(sizeof(uint32_t));
        if (heap_variable)
        {
            *heap_variable = 5555;
            printf("heap variable   | addr = 0x%X | value = %u\n",
                   (unsigned int)(uintptr_t)heap_variable,
                   (unsigned int)*heap_variable);
        }
        else
        {
            printf("heap variable   | addr = 0x%X | value = %u (malloc failed)\n", 0u, 0u);
        }

        // 10) constant variable
        printf("constant var    | addr = 0x%X | value = %u\n",
               (unsigned int)(uintptr_t)&constant_variable,
               (unsigned int)*(&constant_variable));

        // 11) constant string
        printf("constant string | addr = 0x%X | value = 0x%X, [%s]\n",
               (unsigned int)(uintptr_t)DEVICE_NAME,
               (unsigned int)(*((uint32_t*)DEVICE_NAME)),
               DEVICE_NAME);

        // 12) reg chip id (чтение по адресу)
        printf("reg chip id     | addr = 0x%X | value = 0x%X\n",
               (unsigned int)0x40000000u,
               (unsigned int)(*((uint32_t*)0x40000000u)));

        // 13) var by addr (АДРЕС НУЖНО ПОДСТАВИТЬ ТВОЙ ИЗ ВЫВОДА!)
        // СНАЧАЛА посмотри, какой адрес у global_variable напечатался выше, и вставь сюда.
        // Пример из методички: 0x20002278
        printf("var by addr     | addr = 0x%X | value = %u\n",
               (unsigned int)0x20002278u,
               (unsigned int)(*((uint32_t*)0x20002278u)));

        // 14) main function addr + первые 4 байта
        printf("main function   | addr = 0x%X | value = 0x%X\n",
               (unsigned int)(uintptr_t)main,
               (unsigned int)(*((uint32_t*)(uintptr_t)main)));

        printf("--------------------------------------------------------------\n\n");

        // 15) delay
        sleep_ms(1000);
    }
}