/* Deep sleep wake up example

 ����16M  14M���ڴ洢 2M����APP��OTA

 SPI-flash���������£� ǰ36Kϵͳʹ�÷���������0x8000��

nvs,      data, nvs,     0x9000,  0x4000,   16K

otadata,  data, ota,     0xd000,  0x2000,   8K

phy_init, data, phy,     0xf000,  0x1000,   4K

ota_0,    0,    ota_0,   0x10000,  1M,      1M  --Ҫ��֤��ַ��0x10000������Է���1M

ota_1,    0,    ota_1,   0x110000, 0xF0000   960K

�� 2M ����14M���ڴ洢

*/



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "driver/rtc_io.h"
#include "soc/sens_periph.h"
#include "soc/rtc.h"
/********************************/
//update van pull_ring
#include  "spiff.h"
/********************************/


void moding_sleep()
{
	ESP_LOGI("MODING_SLEEP", "esp_sleep_enable_timer_wakeup  START1");
	vTaskDelay(1*1000 / portTICK_PERIOD_MS);
	ESP_LOGI("MODING_SLEEP", "esp_sleep_enable_timer_wakeup  START2");
	esp_sleep_enable_timer_wakeup(5*1000000);
	esp_deep_sleep_start();
}
