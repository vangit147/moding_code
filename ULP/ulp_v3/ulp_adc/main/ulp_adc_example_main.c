/* 
   x,z轴分方向的，所以定义两个阈值
   变量说明 :
   阈值需要在初始化ULP的时候赋值
   p_ax    x正轴阈值
   m_ax    x负轴阈值 
   sysrun_times:ulp运行的次数
   moving_times：运动的次数
   move_flags：当前是否运动的标志位
   在ulp中设置了唤醒主CPU后不停止工作，ULP会一直在运行
*/

#include <stdio.h>
#include <string.h>
#include "esp_sleep.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "driver/adc.h"
#include "driver/dac.h"
#include "esp32/ulp.h"
#include "ulp_main.h"
#include "time.h"
#include "bleinit.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"

extern esp_timer_handle_t periodic_timer;
int BLE_BIT = BIT0;
int SCREEN_BIT = BIT1;
int BLECON_BIT = BIT2;
int BLEDIS_BIT = BIT3;
int TIMER_BIT = BIT4;
int STARTSCREEN = BIT5;

EventBits_t EventValue;
//任务优先级
#define EVENTGROUP_TASK_PRIO 10
//任务堆栈大小
#define EVENTGROUP_STK_SIZE 4096 * 2
//任务句柄
TaskHandle_t EventGroupTask_Handler;
EventGroupHandle_t EventGroupHandler; //事件标志组句柄
//任务函数
void eventgroup_task(void *pvParameters);

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[] asm("_binary_ulp_main_bin_end");
const gpio_num_t GPIO_SCL = GPIO_NUM_32;
const gpio_num_t GPIO_SDA = GPIO_NUM_33;
const gpio_num_t gpio_led = GPIO_NUM_2;
/* This function is called once after power-on reset, to load ULP program into
 * RTC memory and configure the ADC.
 */
static void init_ulp_program();

/* This function is called every time before going into deep sleep.
 * It starts the ULP program and resets measurement counter.

 */
static void start_ulp_program();
short aacx, aacy, aacz; //加速度传感器原始数据
float AccX, AccY, AccZ;
void app_main()
{
    unsigned short temp;
    esp_err_t ret;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
//    ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());
    //esp_sleep_enable_timer_wakeup(1000000);
//    ulp_set_wakeup_period(0, 1000 * 1000);   //ulp 运行周期1s
    EventGroupHandler = xEventGroupCreate(); //创建事件标志组                                             //创建事件标志组处理任务
    xTaskCreate((TaskFunction_t)eventgroup_task,
                (const char *)"eventgroup_task",
                (uint16_t)EVENTGROUP_STK_SIZE,
                (void *)NULL,
                (UBaseType_t)EVENTGROUP_TASK_PRIO,
                (TaskHandle_t *)&EventGroupTask_Handler);

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_TIMER) //定时器唤醒
    {
        printf("timer wakeup\n");
        aacx = ((unsigned short)ulp_ac << 8) | ulp_acl;
        aacz = ((unsigned short)ulp_az << 8) | ulp_azl;
        AccX = aacx / 16384.0; // X-axis value
        AccZ = aacz / 16384.0;
        printf("the aacx is %d \t", aacx);
        printf("the aacz is %d  \t", aacz);
        printf("the aacx is %lf    \t", AccX);
        printf("the aacz is %lf    \n", AccZ);
        temp = ulp_sysrun_times;
        printf("moving_times=%x,sysrun_times=%x,move_flags=%x\r\n", ulp_moving_times, ulp_sysrun_times, ulp_move_flags);
        printf("sysruntimes=%x\r\n", ulp_sysrun_times);
        // esp_deep_sleep_start();
        xEventGroupSetBits(EventGroupHandler, STARTSCREEN); //创建刷屏任务事件标志组
        return;
    }
    else if (cause == ESP_SLEEP_WAKEUP_ULP)
    {
        //10S内有三次检测到动就说明有动产生，打印相关变量信息

        aacx = ((unsigned short)ulp_ac << 8) | ulp_acl;
        aacz = ((unsigned short)ulp_az << 8) | ulp_azl;
        AccX = aacx / 16384.0; // X-axis value
        AccZ = aacz / 16384.0;
        printf("the aacx is %d \t", aacx);
        printf("the aacz is %d  \t", aacz);
        printf("the aacx is %lf    \t", AccX);
        printf("the aacz is %lf    \n", AccZ);
        printf("compare_times=%d\r\n", ulp_compare_times);
        temp = ulp_sysrun_times;
        printf("moving_times=%x,sysrun_times=%x,move_flags=%x\r\n", ulp_moving_times, ulp_sysrun_times, ulp_move_flags);
        printf("sysruntimes=%x\r\n", ulp_sysrun_times);
        //esp_deep_sleep_start();
        xEventGroupSetBits(EventGroupHandler, STARTSCREEN); //创建刷屏任务事件标志组
        return;
    }
    init_ulp_program();
    start_ulp_program();
    ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());
    esp_deep_sleep_start();
}

static void init_ulp_program()
{
    esp_err_t err = ulp_load_binary(0, ulp_main_bin_start,
                                    (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
    ESP_ERROR_CHECK(err);
    rtc_gpio_init(gpio_led);
    rtc_gpio_set_direction(gpio_led, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_init(GPIO_SCL);
    rtc_gpio_set_direction(GPIO_SCL, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_init(GPIO_SDA);
    rtc_gpio_set_direction(GPIO_SDA, RTC_GPIO_MODE_INPUT_ONLY);
    ulp_p_ax = 1000;
    ulp_p_az = 10000;
    ulp_m_ax = 50;
    ulp_m_az = 0;
    ulp_compare_times = 20; //设置ulp的比较运行次数
    /* Set ULP wake up period to 20ms */
    ulp_set_wakeup_period(0, 1000 * 1000);
}

static void start_ulp_program()
{
    /* Reset sample counter */

    /* Start the program */
    esp_err_t err = ulp_run(&ulp_entry - RTC_SLOW_MEM);
    ESP_ERROR_CHECK(err);
}
//事件标志组处理任务
void eventgroup_task(void *pvParameters)
{
    while (1)
    {

        if (EventGroupHandler != NULL)
        {
            //等待事件组中的相应事件位
            EventValue = xEventGroupWaitBits((EventGroupHandle_t)EventGroupHandler,
                                             (EventBits_t)BLE_BIT | BLECON_BIT | BLEDIS_BIT | STARTSCREEN | TIMER_BIT,
                                             (BaseType_t)pdTRUE,
                                             (BaseType_t)pdFALSE,
                                             (TickType_t)portMAX_DELAY);
            //  printf("EventGroupvalue:%d\r\n", EventValue);

            if (EventValue & STARTSCREEN) //刷屏任务
            {
                printf("start screen.......\r\n");
                vTaskDelay(3000 / portTICK_RATE_MS);
                printf("end screen.......\r\n");
                printf("ulp_move_flags=%x\r\n", (unsigned short)ulp_move_flags); //ulp的值转换为short类型进行判断去除干扰位
                if ((unsigned short)ulp_move_flags == 1)                         //说明还在与运动
                {
                    printf("setting timerwakerup\r\n");
                    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(10000000)); //打开定时器开始周期性执行任务，并设置定时器深睡唤醒
                }
                else
                {
                    printf("not move\r\n");
                }
                printf("enter deepsleep\r\n");
                esp_deep_sleep_start();
            }
        }
        else
        {
            vTaskDelay(10 / portTICK_RATE_MS); //延时10ms，也就是10个时钟节拍
        }
    }
}
