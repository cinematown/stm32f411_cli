#include "hw.h"
#include "driver/button.h"

void hwInit(void)
{
    ledInit();
    uartInit();
    cliInit();
    buttonInit();
    //uartOpen(0, 9600);
}