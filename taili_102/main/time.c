//深度睡眠时电流是6uA,蓝牙wifi初始化之后电流是81.7ua

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "sdkconfig.h"
//#include "sys/time.h"
#include "spiff.h"
#include "esp_spi_flash.h"
#include "data_flash.h"
#include "time.h"
esp_timer_handle_t periodic_timer;
extern struct timeval stime;
extern struct tm *p;
extern time_t time_now;


#define timer_tag "timer"



//extern void analysis_data();
//gettimeofday(&stime,NULL);
//	analysis_data();
//	current_data.time_stamp=stime.tv_sec;
//	spi_flash_write(info_page*4096,&current_data,sizeof(current_data));
//	spi_flash_write(info_page_backup*4096,&current_data,sizeof(current_data));
//	time_now=stime.tv_sec;
//	p=localtime(&time_now);
//	printf("%d/%d/%d",(1900+p->tm_year),(1+p->tm_mon),p->tm_mday);
//	printf("%d  %d:%d:%d\n",p->tm_wday,(p->tm_hour+8),p->tm_min,p->tm_sec);


//定时器回调函数
static void periodic_timer_callback(void *arg)
{
    esp_timer_stop(periodic_timer);
//    unsigned char task;
//    spi_flash_read(composite_picture_page*4096+sizeof(unsigned char),&task, sizeof(unsigned char));
//    printf("task=%x\n",task);
//    if(task==0xff)
//    {
//    	task=0x11;
//    	sf_WriteBuffer((uint8_t *)&task, composite_picture_page*4096+sizeof(unsigned char), 1);
//    }
//    else if(task==0x11)
//    {
//    	task=0x22;
//    	sf_WriteBuffer((uint8_t *)&task, composite_picture_page*4096+sizeof(unsigned char), 1);
//    }
//    else
//    {
//    	task=0xff;
//    	sf_WriteBuffer((uint8_t *)&task, composite_picture_page*4096+sizeof(unsigned char), 1);
//    }
    ESP_LOGE(timer_tag,"sleep");
    printf("start deep sleep: %lld us\r\n", esp_timer_get_time());
    esp_deep_sleep_start();
}


//创建周期定时器
void Timer_Config(void)
{
	const esp_timer_create_args_t periodic_timer_args = {
	        .callback = &periodic_timer_callback,
	        .name = "periodic"};
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(20*1000*1000));
	esp_timer_start_periodic(periodic_timer, 20*1000 * 1000);
}

