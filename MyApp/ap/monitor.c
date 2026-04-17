#include "monitor.h"
#include "cli.h"
#include "cmsis_os2.h"
#include <stdint.h>
#include <stdio.h>

//#define LOG_TAG "MON" //로그가 어느파일에서 발생됐는지

typedef enum{
    MONITOR_MODE_OFF = 0,
    MONITOR_MODE_BINARY,
    MONITOR_MODE_ASCII
} monitor_mode_t;

static monitor_packet_t  g_packet;
static osMutexId_t monitor_mtx=NULL;
static monitor_mode_t current_monitor_mode=MONITOR_MODE_OFF;
static uint32_t monitor_period = 0;
static monitor_sync_cb_t sync_cb_handler = NULL;


static uint8_t calcChecksum(uint8_t* data, uint32_t len)
{
    uint8_t sum=0;
    for(uint32_t i=0; i<len; i++){
        sum ^=data[i];
    }
    return sum;
}

static void cliMonitor(uint8_t argc, char** argv)
{
    if(argc>=2){
        
        if(!strcmp(argv[1], "on")){
            if(argc==3){
                monitorSetPeriod(atoi(argv[2]));
                sync_cb_handler();
            }else{
                monitorSetPeriod(1000);
            }
            current_monitor_mode=MONITOR_MODE_ASCII;
            LOG_INF("Monitoring Mode: ON (ASCII)");
            return;
        } else if (!strcmp(argv[1], "off")){
            current_monitor_mode=MONITOR_MODE_OFF;
            LOG_INF("Monitoring Mode: OFF (Text Mode Restored)");
            return;
        }
    }
    
    LOG_INF("Usage: monitor [on|off]");
    if(current_monitor_mode==MONITOR_MODE_ASCII)
        LOG_INF("Current Mode: ON (ASCII)");
    else
        LOG_INF("Current Mode: OFF");
}

uint32_t monitorGetPeriod(void)
{
    return monitor_period;
}

void monitorSetPeriod(uint32_t period)
{
    if(period > 0)
        monitor_period = period;
    else
        monitor_period = 1000;
}

void monitorSetSyncHandler(monitor_sync_cb_t handler){
    sync_cb_handler = handler;
}


void monitorInit()
{
    memset(&g_packet, 0, sizeof(g_packet));
    if(monitor_mtx==NULL) monitor_mtx=osMutexNew(NULL);
    cliAdd("mon", cliMonitor);
}

bool isMonitoringOn(void)
{
    return (current_monitor_mode != MONITOR_MODE_OFF);
}

void monitorUpdateValue(SensorID id, DataType type, void *p_val)
{
    if(monitor_mtx==NULL) return;

    osMutexAcquire(monitor_mtx, osWaitForever);

    //sensor id check
    int target_idx = -1;
    for(int i=0; i<g_packet.count; i++){
        if(g_packet.nodes[i].id==(uint8_t)id){
            target_idx=i;
            break;
        }
    }

    //new senser
    if(target_idx == -1){
        if(g_packet.count<MAX_SENSOR_NODES){
            target_idx=g_packet.count;
            g_packet.nodes[target_idx].id = (uint8_t)id;
            g_packet.count++;
        }else{
            osMutexRelease(monitor_mtx);
            return;
        }
    }

    //update
    g_packet.nodes[target_idx].type = (uint8_t)type;

    if(type==TYPE_UINT8 || type==TYPE_BOOL){
        g_packet.nodes[target_idx].value.u8_val[0] = *(uint8_t*)p_val;
    }else{
        memcpy(&g_packet.nodes[target_idx].value, p_val, 4);
    }

    osMutexRelease(monitor_mtx);
}

void monitorSendPacket()
{
    if(current_monitor_mode==MONITOR_MODE_OFF || monitor_mtx==NULL) return;
 
    osMutexAcquire(monitor_mtx, osWaitForever);

    char tx_buf[256] = {0};
    uint32_t len=0;

    if(current_monitor_mode==MONITOR_MODE_ASCII){
        //시작 문자 $
        len += snprintf(tx_buf+len, sizeof(tx_buf)-len, "$%d", g_packet.count);
        
        //구분자 노드
        for(int i=0; i<g_packet.count; i++){
            uint8_t id = g_packet.nodes[i].id;
            uint8_t type = g_packet.nodes[i].type;

            len += snprintf(tx_buf+len, sizeof(tx_buf)-len, ",");
            
            //id, type
            len += snprintf(tx_buf+len, sizeof(tx_buf)-len, "%d:%d:", id, type);

            //value
            if(type==TYPE_UINT8 || type==TYPE_BOOL){
                len += snprintf(tx_buf+len, sizeof(tx_buf)-len, "%u", g_packet.nodes[i].value.u8_val[0]);
            }else if(type==TYPE_INT32){
                len += snprintf(tx_buf+len, sizeof(tx_buf)-len, "%ld", g_packet.nodes[i].value.i_val);
            }else if(type==TYPE_FLOAT){
                len += snprintf(tx_buf+len, sizeof(tx_buf)-len, "%.2f", g_packet.nodes[i].value.f_val);
            }else{
                len += snprintf(tx_buf+len, sizeof(tx_buf)-len, "%lu", g_packet.nodes[i].value.u_val);
            }
        }
        //종료 문자 #
        len +=snprintf(tx_buf+len, sizeof(tx_buf), "#\r\n");
    }else{}

    uartWrite(0, (uint8_t *)tx_buf, len);

    osMutexRelease(monitor_mtx);

}