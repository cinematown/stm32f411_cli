#include "hw.h"

void hwInit(void)
{
    ledInit();
    uartInit();
    cliInit();
    //uartOpen(0, 9600);
}