//深度睡眠时电流是6uA,蓝牙wifi初始化之后电流是81.7ua
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "data_flash.h"

esp_timer_handle_t periodic_timer;
#define timer_tag "timer"

//定时器回调函数
static void periodic_timer_callback(void *arg)
{
    esp_timer_stop(periodic_timer);
    sleep_for_next_wakeup();
}


//创建周期定时器
void Timer_Config(void)
{
	const esp_timer_create_args_t periodic_timer_args = {
	        .callback = &periodic_timer_callback,
	        .name = "periodic"};
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));

	time_now=stime.tv_sec;
  	p=localtime(&time_now);
  	printf("p->tm_hour=%d p->tm_min=%d p->tm_sec=%d ",p->tm_hour,p->tm_min,p->tm_sec);
  	if(p->tm_hour==12&&p->tm_min==59&&p->tm_sec>=20&&p->tm_sec<=59)
  	{
  		ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(600*1000*1000));//10 minutes
  		ESP_LOGW(timer_tag,"After hours 10 minutes , it will be wakeup by timer");
  	}
  	else
  	{
  		int ti;
  		if(p->tm_hour<=12)
  		{
  			ti=12-p->tm_hour;
  			ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup((ti*60*60+(60-p->tm_min)*60+60-p->tm_sec)*1000*1000));
  			ESP_LOGW(timer_tag,"After %d hours %d minutes %d seconds , it will be wakeup by timer",ti,60-p->tm_min,60-p->tm_sec);
  		}
  		else
  		{
  			ti=36-p->tm_hour;
  			ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup((ti*60*60+(60-p->tm_min)*60+60-p->tm_sec)*1000*1000));
  			ESP_LOGW(timer_tag,"After %d hours %d minutes %d seconds , it will be wakeup by timer",ti,60-p->tm_min,60-p->tm_sec);
  		}
  	}
}

