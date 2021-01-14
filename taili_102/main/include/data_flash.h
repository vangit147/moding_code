#ifndef _DATA_FLASH_H_
#define	_DATA_FLASH_H_

#include "common.h"



#define AND '&'
#define QUES '?'

extern unsigned char isconnected;
extern esp_ble_adv_data_t adv_data;
extern unsigned char device_info[26];
extern esp_timer_handle_t periodic_timer;

void find_wakeup_cause();

void read_write_init();

void init_default_data();

void analysis_data();

void updated_esp_time();

void updated_data_to_flash();

void check_wifi_httpdowload_pic(char wakeup_cause);

void download_composite(cJSON * item);

void charconnectuchar(char a[],unsigned char b[]);


#endif
