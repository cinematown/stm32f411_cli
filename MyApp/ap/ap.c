#include "ap.h"
#include "bsp.h"
#include "cli.h"
#include "cmsis_os.h"
#include "cmsis_os2.h"
#include "led.h"
#include "my_gpio.h"
#include "stm32f411xe.h"
#include "stm32f4xx_hal.h"
#include <ctype.h>

static uint32_t led_toggle_period = 0;
static uint32_t temp_read_period = 0;
void cliLed(uint8_t argc, char **argv)
{
    if (argc >= 2) {
        if (strcmp(argv[1], "on") == 0) {
            led_toggle_period = 0;
            ledOn();
            cliPrintf("LED ON\r\n");
        } else if (strcmp(argv[1], "off") == 0) {
            led_toggle_period = 0;
            ledOff();
            cliPrintf("LED OFF\r\n");
        } else if (strcmp(argv[1], "toggle") == 0) {
            if(argc == 3){
                led_toggle_period=atoi(argv[2]);
                if(led_toggle_period>0){
                    cliPrintf("LED Auto-Toggled!!\r\n");
                }else{
                    cliPrintf("Invalid Period\r\n");
                }
            }else{
                led_toggle_period = 0;
                ledToggle();
                cliPrintf("LED TOGGLE\r\n");
            }
            
        } else {
            cliPrintf("Invalid Command\r\n");
        }
    } else {
        cliPrintf("Usage: led [on|off|toggle]\r\n");
        cliPrintf("Usage: led toggle [period]\r\n");
    }
}

void cliInfo(uint8_t argc, char **argv)
{
  if (argc == 1) {
    uint32_t uid0 = HAL_GetUIDw0();
    uint32_t uid1 = HAL_GetUIDw1();
    uint32_t uid2 = HAL_GetUIDw2();
    // uint32_t rev = HAL_GetREVID();
    uint32_t dev = HAL_GetDEVID();

    cliPrintf("--------------------------------------------\r\n");
    cliPrintf(" HW Model    :   STM32F411\r\n");
    cliPrintf(" FW Version  :   v1.0.0\r\n");
    cliPrintf(" Build Date  :   %s %s\r\n", __DATE__, __TIME__);
    cliPrintf(" Serial Num  :   %08x-%08x-%08x\r\n", uid0, uid1, uid2);
    cliPrintf(" Device ID   :   %08x\r\n", dev);

    cliPrintf("--------------------------------------------\r\n");
  }

  if (argc == 2 || strcmp(argv[1], "uptime") == 0) {
    cliPrintf("System Uptime: %d ms \r\n", millis());
  } else {
    cliPrintf("Usage: info\r\n");
    cliPrintf("Usage: info [uptime]\r\n");
  }
}

void cliSys(uint8_t argc, char **argv)
{
  if (argc == 2 && strcmp(argv[1], "reset") == 0) {
    NVIC_SystemReset();
  } else {
    cliPrintf("Usage: sys [reset]\r\n");
  }
}

void cliGpio(uint8_t argc, char **argv)
{
    // argv[1] : "read" "write"
    // argb[2] : pin A5, B12
  if (argc >= 3) {
    char port_char = tolower(argv[2][0]);
    int pin_num = atoi(&argv[2][1]); //?

    uint8_t port_idx = port_char - 'a'; // switch문에 숫자로 전달

    if (strcmp(argv[1], "read") == 0) {
      int8_t state = gpioExtRead(port_idx, pin_num);
      if (state < 0)
        cliPrintf("Invalid Port or Pin\r\n");
      else
        cliPrintf("GPIO %c%d=%d\r\n", toupper(port_char), pin_num, state);

    } else if (strcmp(argv[1], "write") == 0 && argc == 4) {
      int val = atoi(argv[3]);
      if (gpioExtWrite(port_idx, pin_num, val) == true)
        cliPrintf("GPIO %c%d Set to %d\r\n", toupper(port_char), pin_num, val);
      else
        cliPrintf("Invalid Port or Pin\r\n");

    } else {
    }

  } else {
    cliPrintf("Usage: gpio read [a~h][0~15]\r\n");
    cliPrintf("Usage: gpio write [a~h][0~15] [0|1]\r\n");
  }
}

static bool isSafeAddress(uint32_t addr)
{
    //1. f411 flash
    if(0x08000000 <= addr && addr <= 0x0807FFFF) return true;
    //2. f411 ram
    if(0x20000000 <= addr && addr <= 0x20001FFF) return true;
    //3. system memory
    if(0x1FFF0000 <= addr && addr <= 0x1FFF7A1F) return true;
    //4. peripheral register
    if(0x40000000 <= addr && addr <= 0x4005FFFF) return true;

    return false;
}

void cliMd(uint8_t argc, char **argv)
{
  // md 0x8000-0000 32
  if (argc >= 2) {
    uint32_t addr = strtoul(argv[1], NULL, 16);
    uint32_t length = 16;

    if (argc >= 3) {
      length = strtoul(argv[2], NULL, 0);
    }

    for (uint32_t i = 0; i < length; i+=16) {
      cliPrintf("0x%08x : ", addr + i);
      for (uint32_t j = 0; j < 16; j++) {
        if(i+j < length){
            uint32_t target_addr = addr+i+j;
            if(isSafeAddress(target_addr)){
                uint8_t val = *(volatile uint8_t *)(target_addr);
                cliPrintf("%02x ", val);
            }else{
                cliPrintf("Not valid address!!\r\n");
                break;
            }
        }else{
            cliPrintf("   ");
        }
        
      }
      cliPrintf(" | ");
      for(uint32_t j=0; j <16; j++){
        if(i+j <length){
            uint32_t target_addr = addr+i+j;
            if(isSafeAddress(target_addr)){
                uint8_t val = *(volatile uint8_t *)(target_addr);
                if(val >= 0x20 && val <= 0x7E) cliPrintf("%c", val);
                else cliPrintf(".");
            }
        }
      }
      cliPrintf("\r\n");
    }

  } else {
    cliPrintf("Usage : md [add(hex)] [length]\r\n");
    cliPrintf("        md 80000000 32 \r\n");
  }
}

//button on/off == enable/disable
void cliButton(uint8_t argc, char **argv)
{
    if(argc == 2){
        if(strcmp(argv[1], "on") == 0){
            buttonEnable(true);
            cliPrintf("Button Interrupt Report: ON\r\n");
        }else if(strcmp(argv[1], "off") == 0){
            buttonEnable(false);
            cliPrintf("Button Interrupt Report: OFF\r\n");
        }
    }else{
        cliPrintf("Usage Button on|off\r\n");
        cliPrintf("Current Status: %s\r\n", buttonGetEnable()? "enable" : "disable");
    }
}

void cliTemp(uint8_t argc, char **argv)
{
  if(argc==1){
    temp_read_period = 0;
    cliPrintf("Temperature: %0.2f ^C\r\n", tempRead());
  }else if(argc==2){
    cliPrintf("Temperature Auto Read Start\r\n");
    temp_read_period = atoi(argv[1]);
  }else{
    cliPrintf("Usage: temp\r\n");
    cliPrintf("Usage: temp [period]\r\n");
  }
}

void ledSystemTask(void *argument)
{
  while (1){
    if(led_toggle_period > 0){
      ledToggle();
      osDelay(led_toggle_period);
      //vTaskDelay(1000); // HALdelay는 시스템 전체를 멈추고 싶을 때
    }else{
      osDelay(50); //cmsis 함수를 가급적 사용하려고
    }
  }
}

void tempSystemTask(void *argument)
{
  while(1){
    if(temp_read_period > 0){
      cliPrintf("Temperature: %0.2f ^C\r\n", tempRead());
      osDelay(temp_read_period);
    }else{
      osDelay(50);
    }
  }
}


void apInit(void) {
  hwInit();
  cliAdd("led", cliLed);
  cliAdd("info", cliInfo);
  cliAdd("sys", cliSys);
  cliAdd("gpio", cliGpio);
  cliAdd("md", cliMd);
  cliAdd("button", cliButton);
  cliAdd("temp", cliTemp);
}

void apMain(void) {
    //osThreadId_t ledSystemTaskHandle;
    // const osThreadAttr_t ledSystemTask_attributes = {
    //     .name = "ledSystemTask",
    //     .stack_size = 128 * 4,
    //     .priority = (osPriority_t) osPriorityNormal,
    // };
    // //ledSystemTaskHandle = osThreadNew(ledSystemTask, NULL, &ledSystemTask_attributes);
    // osThreadNew(ledSystemTask, NULL, &ledSystemTask_attributes);
    
    uartPrintf(0, "LED Task Started!! \r\n");
    
    while (1) {

    // HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    // ledOff();
    // HAL_Delay(1000);
    // ledOn();
    // HAL_Delay(1000);
    //------------------------------------------

    // uartWrite(0, (uint8_t *)"HELLOW", 7);

    // if(uartAvailable(0)>0){
    //     uint8_t ch = uartRead(0);

    //    uartPrintf(0, "%c", ch);
    //}
    // HAL_Delay(500);

        cliMain();
        osDelay(1); //task 시간을 양보
    }
}