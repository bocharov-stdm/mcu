#include "stdio-task.h"
#include "stdio.h"
#include "pico/stdlib.h"

#define COMMAND_BUF_LEN 128

char command[COMMAND_BUF_LEN] = {0};

int command_buf_idx;

void stdio_task_init()
{
	command_buf_idx = 0;
}

char* stdio_task_handle()
{
    int symbol = getchar_timeout_us(0);
    if (symbol == PICO_ERROR_TIMEOUT)
    {
        return NULL;
    }

    // Обработка backspace
    if (symbol == '\b' || symbol == 127) // 127 - также часто используется для backspace (DEL)
    {
        if (command_buf_idx > 0)
        {
            command_buf_idx--;
            // Опционально: визуальное отображение удаления
            putchar('\b');  // Возврат курсора
            putchar(' ');   // Затирание символа пробелом
            putchar('\b');  // Возврат курсора снова
        }
        return NULL;
    }

    putchar(symbol);

    // Обработка enter (конец строки)
    if (symbol == '\r' || symbol == '\n')
    {
        command[command_buf_idx] = '\0';
        command_buf_idx = 0;
        
        printf("\nreceived string: '%s'\n", command); // Добавлен \n для перевода строки

        return command;
    }

    // Проверка переполнения буфера
    if (command_buf_idx >= COMMAND_BUF_LEN - 1)
    {
        command_buf_idx = 0;
        return NULL;
    }

    // Добавление обычного символа
    command[command_buf_idx] = symbol;
    command_buf_idx++;
    
    return NULL;
}