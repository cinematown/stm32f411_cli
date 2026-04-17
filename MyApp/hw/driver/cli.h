#ifndef __HW_DRIVER_CLI_H__
#define __HW_DRIVER_CLI_H__

#include "hw_def.h"
#include "def.h"
#include "log.h"
#include "uart.h"
#include "ap.h"

typedef void (*cli_callback_t)(void);
void cliSetCtrlCHandler(cli_callback_t handler);

void cliInit();
void cliPrintf(const char *fmt, ...);

void cliParseArgs(char* Line_buf);
bool cliAdd(const char* cmd_str, void(*cmd_func)(uint8_t argc, char* argv[]));
void cliRunCommand();
void cliMain();


#endif