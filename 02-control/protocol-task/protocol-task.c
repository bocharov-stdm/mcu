#include "protocol-task.h"
#include "stdio.h"
#include "string.h"

typedef int bool;
#define true 1
#define false 0

static api_t* api = {0};
static int commands_count = 0;

void protocol_task_init(api_t* device_api)
{
    if (!device_api) return;
    api = device_api;

    int i = 0;
    while (device_api[i].command_name != NULL)
    {
        i++;
    }

    commands_count = i;
}


void protocol_task_handle(char* command_string)
{
    const char* command_name = command_string;
    const char* command_args = NULL;

    char* space_symbol = strchr(command_string, ' ');
    if (space_symbol)
    {
	    *space_symbol = '\0';
	    command_args = space_symbol + 1;
    }
    else
    {
	    command_args = "";
    }

    printf("Command: %s, arguments: %s\n", command_name, command_args);

    bool found = false;
    for (int i=0; i < commands_count; i++)
    {
        if (strcmp(command_name, api[i].command_name) == 0)
        {
            api[i].command_callback(command_args);
            found = true;
        }
    }
    if (!found)
    {
        printf("Unknown command: %s\n", command_name);
    }
}
