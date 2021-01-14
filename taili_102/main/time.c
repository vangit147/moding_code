//���˯��ʱ������6uA,����wifi��ʼ��֮�������81.7ua
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_sleep.h"

esp_timer_handle_t periodic_timer;
#define timer_tag "timer"

//��ʱ���ص�����
static void periodic_timer_callback(void *arg)
{
    esp_timer_stop(periodic_timer);
    ESP_LOGE(timer_tag,"deep sleep");
    ESP_LOGE(timer_tag,"start deep sleep: %lld us", esp_timer_get_time());
    esp_deep_sleep_start();
}


//�������ڶ�ʱ��
void Timer_Config(void)
{
	const esp_timer_create_args_t periodic_timer_args = {
	        .callback = &periodic_timer_callback,
	        .name = "periodic"};
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(20*1000*1000));
	esp_timer_start_periodic(periodic_timer, 20*1000 * 1000);
}

