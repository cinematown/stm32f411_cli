#include "log.h"
#include "cli.h"

static uint8_t runtime_log_level = LOG_LEVEL_INFO;


void cliLog(uint8_t argc, char** argv)
{
    //log [get]/[set] [0~5]
    if(argc==2 && !strcmp(argv[1], "get")){
        cliPrintf("Current Log Level: %d\r\n", runtime_log_level);

    }else if(argc==3 && !strcmp(argv[1], "set")){
        int level = atoi(argv[2]);
        if(0 <= level && level <= 5){
            logSetLevel(level);
            cliPrintf("Log Level Set to %d\r\n", level);
        }else{
            cliPrintf("Invalid level(0~5)\r\n");
        }
    }else{
        cliPrintf("0: FATAL, 1:ERROR, 2:WARN, 3:INFO, 4:DEBUG, 5:VERBOSE\r\n");
        cliPrintf("Usage: log get\r\n");
        cliPrintf("Usage: log set [0~5]\r\n");
    }
}

bool logInit()
{
    return true;
}

void logSetLevel(uint8_t Level)
{
    runtime_log_level=Level;
}

uint8_t logGetLevel(void)
{
    return runtime_log_level;
}

//public
uint8_t logGetRuntimeLevel(void)
{
    return runtime_log_level; 
}

void logPrintf(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    int len;

    va_start(args, fmt);
    len = vsnprintf(buf, 256, fmt, args);
    va_end(args);

    uartWrite(0, (uint8_t*)buf, len);
}

