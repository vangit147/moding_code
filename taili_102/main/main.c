//spiffsgen.py 0xA00000 spiffs_image spiffs.bin esptool.py --chip esp32 --port COM5 write_flash -z 0x410000 spiffs.bin

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "sdkconfig.h"
#include "wifi.h"
#include "esp_wifi.h"
#include "esp_spiffs.h"
#include "driver/rtc_io.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "data_flash.h"

#include "aenc.h"


static esp_adc_cal_characteristics_t  *adc_chars;
uint32_t adc_reading=0;
uint32_t voltage=0;


extern int write_id;
extern int time_count;
extern SFLASH_T g_tSF;
extern unsigned char total;
extern unsigned int rec_data;
extern uint16_t data_conn_id;
extern esp_gatt_if_t data_gatts_if;
extern esp_ble_adv_params_t adv_params;
extern struct gatts_profile_inst gl_profile_tab[PROFILE_NUM];


int EXECDOWNLOAD_BIT=BIT3;
int READ_BIT = BIT2;
int CLEAR_BIT = BIT1;
int DOWNLOAD_BIT = BIT0;
unsigned char picture_num; //图片数量
unsigned char receive_buffer[512];

#define REC_LEN 300 //发送数据的消息队列的数量
#define MESSAGE_Q_NUM 1
#define EVENTGROUP_TASK_PRIO 20  //任务优先级
#define EVENTGROUP_STK_SIZE 4096 * 2  //任务堆栈大小

EventBits_t EventValue;
QueueHandle_t Message_Queue; //信息队列句柄
TaskHandle_t EventGroupTask_Handler;  //任务句柄
EventGroupHandle_t EventGroupHandler; //事件标志组句柄

void ble_senddata(unsigned char *data);  //函数声明
void eventgroup_task(void *pvParameters);  //任务函数

extern char download_url[200],file_name[32],url_state;

extern void e_init(void);//10.2  台历
extern esp_c_t  CN_Init(void);//5.65  拉环
extern void Timer_Config(void);


//主函数入口相关内容初始化
void app_main()
{
    g_tSF.TotalSize = 16 * 1024 * 1024; /* 总容量 = 16M */
    g_tSF.PageSize = 4 * 1024;          /* 页面大小 = 4K */

    ESP_LOGW(my_tag,"init NVS");
    esp_err_t ret;
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGW(my_tag,"device MAC address");
    uint8_t mac[6];
	esp_read_mac(mac, ESP_MAC_BT);
	for(int i=0;i<6;i++){
		device_info[8+i]=mac[i];
	}

	for(int i=0;i<6;i++)
	{
		ESP_LOGW(my_tag,"mac=%x",device_info[8+i]);
	}




//	 spi_flash_erase_sector(composite_picture_page);
//	 spi_flash_erase_sector(info_page+1);
//	 spi_flash_erase_sector(info_page);

	ESP_LOGW(my_tag,"wifi init_sta");
	wifi_init_sta();

//	ESP_LOGW(my_tag,"init timer");
//	Timer_Config();

//	ESP_LOGW(my_tag, "e_init");
//	e_init();
//	ESP_LOGW(my_tag, "CN_Init");
//	CN_Init();




	ESP_LOGW(my_tag, "read_write_init");
	read_write_init();

	analysis_data();

	ESP_LOGW(my_tag,"Get device information");
	getdeviceinfo();

	ESP_LOGW(my_tag,"gattserver(ble) init");
	GattServers_Init();

	ESP_LOGW(my_tag,"esp_ble_gap_config_adv_data");
	esp_ble_gap_config_adv_data(&adv_data);



	ESP_LOGW(my_tag,"get voltage");
	adc1_config_width(3);
	adc1_config_channel_atten(6,3);
	adc_chars = calloc(1,sizeof(esp_adc_cal_characteristics_t));
	esp_adc_cal_characterize(1,3,3,1100,adc_chars);
	voltage = esp_adc_cal_raw_to_voltage(adc1_get_raw(6),adc_chars)*2;
	voltage=voltage/10;
	ESP_LOGW(my_tag,"Voltage:%d",voltage);
	vTaskDelay(1000/portTICK_RATE_MS);

	if(voltage<=220)
	{
//		display_picture_temp(0,low_network_wifi_picture_page);
		ncolor_display(0,0x44);//red
		ESP_LOGW(my_tag,"low voltage ---> deep sleep start");
		esp_deep_sleep_start();
	}
	else
	{
		ESP_LOGW(my_tag,"find wakeup_cause");
//		find_wakeup_cause();
		check_wifi_httpdowload_pic('1');
//		ncolor_display(0,0x44);
	}

	EventGroupHandler = xEventGroupCreate(); //创建事件标志组
	//创建事件标志组处理任务
	xTaskCreate((TaskFunction_t)eventgroup_task,
				(const char *)"eventgroup_task",
				(uint16_t)EVENTGROUP_STK_SIZE,
				(void *)NULL,
				(UBaseType_t)EVENTGROUP_TASK_PRIO,
				(TaskHandle_t *)&EventGroupTask_Handler);
	return;
}



static unsigned int total_num = 0;
static unsigned int current_num = 0;
char send_buf[20];
unsigned char downloading_url[200];
int execDownloading=0;//0空闲 1读取中 2下载中

//事件标志组处理任务
void eventgroup_task(void *pvParameters)
{
	char cmp_name[20];
    while (1)
    {
        if (EventGroupHandler != NULL)
        {
            //等待事件组中的相应事件位
            EventValue = xEventGroupWaitBits((EventGroupHandle_t)EventGroupHandler,
                                             (EventBits_t)DOWNLOAD_BIT | CLEAR_BIT | READ_BIT | EXECDOWNLOAD_BIT,
                                             (BaseType_t)pdTRUE,
                                             (BaseType_t)pdFALSE,
                                             (TickType_t)portMAX_DELAY);
            printf("EventGroupvalue:%d\r\n", EventValue);
            if(EventValue & EXECDOWNLOAD_BIT){
            	if(execDownloading==0){
            		execDownloading=1;
            		char cmp_url[200];
					char temp_url[200];
					char itype;
					for(int i=0;i<2;i++){
						unsigned int start_pos=(info_page-1) * 4096 + i * 200;
						printf("EXECDOWNLOAD_BIT-1:: start pos=%d\n",start_pos);
						spi_flash_read(start_pos, cmp_url, 200);
						itype=cmp_url[0];
						strcpy(temp_url, ((char*)(cmp_url)+1));
						int ret = memcmp(temp_url, "https://", 5);
						printf("EXECDOWNLOAD_BIT-1:: ret=%d\n",ret);
						if(ret==0){
							printf("EXECDOWNLOAD_BIT-1:: cmp_url=%s  , temp_url=%s,   itype=%c\n", cmp_url,temp_url,itype);
							if(itype=='0'){
								printf("EXECDOWNLOAD_BIT-1:: exec http_test_task\n");
								int result=http_test_task(temp_url);
								if(result==1){
									printf("EXECDOWNLOAD_BIT-1:: update start\n");
									cmp_url[0]='1';
									sf_WriteBuffer((uint8_t *)cmp_url, start_pos, 200);
									printf("EXECDOWNLOAD_BIT-1:: update end\n");
								}
							}
						}
					}
					execDownloading=0;
            	}
            }
            //wifi连接成功准备下载图片
            else if (EventValue & DOWNLOAD_BIT)
            {
            	url_state=2;
            	printf("main download handler");
                //1280页也就是5M出开始写
                //下载图片前的工作
//                spi_flash_read(info_page * 4096, &picture_num, sizeof(unsigned char));
//                if (picture_num == 0xff) //没有图片
//                {
//                    picture_num = 0;
//                }

            	analysis_data();
                ESP_LOGE("main", "picture_num=%d", picture_num);

                unsigned char i;
                for (i = 0; i < current_data.pic_number; i++)
                {
					spi_flash_read(info_pic_name + i * 20, cmp_name, 20);
					if (strcmp((char *)cmp_name, current_data.pic_name) == 0)
					{
                        ESP_LOGE("main", "find same name picture");
                        picture_num = i;
                        break;
                    }
                }
                ESP_LOGE("main", "picture_num=%d", picture_num);
                if (i > picture_num) //没有找到相同的文件
                {
                    picture_num++;
                    ESP_LOGE("main", "not find same name picture");
                    ESP_LOGE("main", "current picture_num=%d", picture_num);
                    sf_WriteBuffer(&picture_num, info_page * 4096, sizeof(unsigned char));         //写入文件个数
                    sf_WriteBuffer((uint8_t *)file_name, info_page * 4096 + picture_num * 50, 40); //存储文件名称长度40
                }
                ESP_LOGI("http task", "read file_name= %s", file_name);
                ESP_LOGE("http task", "closr timer");


                //add begin
                char cmp_url[200];
                char temp_url[200];
                char itype=0;

				const char ch = '_';
				char *r;
				r = strrchr(download_url, ch);
                char index=*(r+2);
                unsigned int start_pos=(info_page-1) * 4096 + (index-49) * 200;
                spi_flash_read(start_pos, cmp_url, 200);
                strcpy(temp_url, ((char*)(cmp_url)+1));
                printf("DW:: start pos=%d\n",start_pos);
                int ret = memcmp(temp_url, "https://", 5);
                int urlIsExists=0;
                if(ret==0){
                	printf("DW:: cmp_url=%s  , temp_url=%s,   itype=%c,  index=%c\n", cmp_url,temp_url,itype,index);
                	if (strcmp(temp_url, download_url) == 0){//url存在
                		printf("DW:: cmp_url is extis\n");
                		urlIsExists=1;
                	}
                }
                if(urlIsExists<1){
                	memset(cmp_url,0,sizeof(cmp_url));
                	cmp_url[0]='0';
                	strcat(cmp_url, download_url);
                	sf_WriteBuffer((uint8_t *)cmp_url, start_pos, 200);
                	printf("DW:: write cmp_url:: cmp_url=%s \n", cmp_url);
                }

                //add end

                //update begin
				getdeviceinfo();
				esp_ble_gap_config_adv_data(&adv_data);
				xEventGroupSetBits(EventGroupHandler, EXECDOWNLOAD_BIT);
				esp_timer_start_periodic(periodic_timer, 90000 * 1000); //设置定时器的定时周期为1s
                //// esp_timer_stop(periodic_timer);
//                http_test_task(download_url, file_name);
//                ESP_LOGE("http task", "start timer");
//                getdeviceinfo();
//                esp_ble_gap_config_adv_data(&adv_data);
//                esp_timer_start_periodic(periodic_timer, 90000 * 1000); //设置定时器的定时周期为1s

				//update end
            }
            else if (EventValue & CLEAR_BIT)
            {
                //清空所有图片
                esp_wifi_stop();
                ESP_LOGE("http task", "clear picture");
                spi_flash_erase_sector(info_page);
                getdeviceinfo();
                esp_ble_gap_config_adv_data(&adv_data);
                esp_wifi_start();
                esp_timer_start_periodic(periodic_timer, 90000 * 1000); //设置定时器的定时周期为1s
            }
            else if (EventValue & READ_BIT)
            {
                //清空所有图片
                esp_wifi_stop();
                spi_flash_read(info_page * 4096, &picture_num, sizeof(unsigned char));
                ESP_LOGI("read task", "picture_num=%d", picture_num);
                int size;
                unsigned int bytes_read1 = 0;
                for (char i = 1; i <= picture_num; i++)
                {
                    spi_flash_read(info_page * 4096 + i * 50 + 41, &size, sizeof(int));
                    ESP_LOGI("read task", "size=%d", size);
                    spi_flash_read(info_page * 4096 + i * 50, file_name, 40);
                    ESP_LOGI("read task", "filename=%s", file_name);
                    ESP_LOGI("read task", "read data:");
                    while (size > 0)
                    {
                        spi_flash_read((picture_page + picture_cap * i) * 4096 + bytes_read1 * 512, receive_buffer, 512);
                        ESP_LOGI("read task", "add=%d", (picture_page + picture_cap * i) * 4096 + bytes_read1 * 512);
                        bytes_read1++;
                        size -= 512;
                        esp_log_buffer_hex("receive data", receive_buffer, 512);
                    }
                    ESP_LOGI("read task", "bytes_read=%d", bytes_read1);
                }
                esp_wifi_start();
                esp_timer_start_periodic(periodic_timer, 90000 * 1000); //设置定时器的定时周期为1s
            }
        }
        else
        {
            vTaskDelay(10 / portTICK_RATE_MS); //延时10ms，也就是10个时钟节拍
        }
    }
}
// ble 一次只能发送20个字节，分包进行数据发送
void ble_senddata(unsigned char *data)
{
    int len = 0;
    //len = strlen((char *)data);
    len = data[2] + 4;
    printf("len=%d\r\n", len);
    if (len <= 20)
    {
        printf("GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d", data_gatts_if, data_conn_id, write_id);
        esp_ble_gatts_send_indicate(data_gatts_if, data_conn_id, write_id,
                                    len, (uint8_t *)data, false);
    }
    else if (len > 20)
    {
        if (len % 20 == 0)
        {
            total_num = len / 20;
        }
        else
        {
            total_num = len / 20 + 1;
        }
        current_num = 1;
        while (current_num <= total_num)
        {
            if (current_num < total_num)
            {
                memcpy(send_buf, receive_buffer + (current_num - 1) * 20, 20);
                esp_ble_gatts_send_indicate(data_gatts_if, data_conn_id, write_id,
                                            20, (uint8_t *)send_buf, false);
            }
            else if (current_num == total_num)
            {
                memset(send_buf, 0, 20);
                // printf("remin_data=%d\r\n", (len - (current_num - 1) * 20));
                memcpy(send_buf, receive_buffer + (current_num - 1) * 20, (len - (current_num - 1) * 20));
                esp_ble_gatts_send_indicate(data_gatts_if, data_conn_id, write_id,
                                            (len - (current_num - 1) * 20), (uint8_t *)send_buf, false);
            }
            current_num++;
        }
    }
}
