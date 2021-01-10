//spiffsgen.py 0xA00000 spiffs_image spiffs.bin esptool.py --chip esp32 --port COM5 write_flash -z 0x410000 spiffs.bin

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_timer.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "sdkconfig.h"
#include "time.h"
#include "gatts_server.h"
#include "wifi.h"
#include "esp_wifi.h"
#include "spiff.h"
#include "http_client.h"
#include "esp_spiffs.h"
#include "esp_timer.h"
#include "driver/rtc_io.h"
#include "esp_log.h"
#include "data_flash.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "cJSON.h"

//pullring
#include "aenc.h"
//end

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
extern esp_ble_adv_data_t adv_data;
extern esp_ble_adv_params_t adv_params;
extern esp_timer_handle_t periodic_timer;
extern struct gatts_profile_inst gl_profile_tab[PROFILE_NUM];
//extern char download_url[100], file_name[32];

int EXECDOWNLOAD_BIT=BIT3;
int READ_BIT = BIT2;
int CLEAR_BIT = BIT1;
int DOWNLOAD_BIT = BIT0;
unsigned char picture_num; //图片数量
unsigned char receive_buffer[512];

//宏定义
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


extern unsigned char device_info[26];
extern char download_url[200],  download_url_failed[200],file_name[32],url_state;

int url_length;
unsigned char failed_bit=0;
unsigned char failed_times=0;


extern void e_init(void);
extern void display_picture(int display_index,int picture_page_index);

//pullring
extern esp_c_t  CN_Init(void);
extern void ncolor_display(uint16_t index,unsigned char pic_data4);
//end

#define my_tag "desk_calender"
#define timer_tag "timer"
#define sector_size 4096
struct timeval stime;
struct tm *p;
time_t time_now;
extern unsigned char isconnected;
unsigned char wifi_config_page=0;
char default_wifi_ssid[40]="moding_wifi";
char default_wifi_pssd[40]="modingtech.com";
long default_time_stamp=1609430400;// 2021/1/1  00:00:00
void read_write_init();
void analysis_data();
void init_default_data();
void inttostring(long value, char * output);
void ucharconnectchar(char a[],unsigned char b[]);
void search_pic(esp_sleep_source_t wakeup_cause);
void check_wifi_httpdowload_pic(char pic_name[20],int wakeup_cause);
void download_composite(cJSON * item);
//void cJSON_data(char *json_str);
void cJSON_data();
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

//    cJSON_data();


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

//	ESP_LOGW(my_tag, "e_init");
//	e_init();
	ESP_LOGW(my_tag, "CN_Init");
	CN_Init();

	ESP_LOGW(my_tag, "read_write_init");
	read_write_init();

	analysis_data();

	ESP_LOGW(my_tag,"Get device information");
	getdeviceinfo();

	ESP_LOGW(my_tag,"gattserver(ble) init");
	GattServers_Init();

	ESP_LOGW(my_tag,"esp_ble_gap_config_adv_data");
	esp_ble_gap_config_adv_data(&adv_data);

	ESP_LOGW(my_tag,"wifi init_sta");
	wifi_init_sta();



//	ESP_LOGW(my_tag,"init timer");
//	Timer_Config();



	ESP_LOGW(my_tag,"get voltage");
	adc1_config_width(3);
	adc1_config_channel_atten(6,3);
	adc_chars = calloc(1,sizeof(esp_adc_cal_characteristics_t));
	esp_adc_cal_characterize(1,3,3,1100,adc_chars);
	voltage = esp_adc_cal_raw_to_voltage(adc1_get_raw(6),adc_chars)*2;
	voltage=100*voltage;
	ESP_LOGW(my_tag,"Voltage:%d",voltage);
	vTaskDelay(1000/portTICK_RATE_MS);

	if(voltage<=220)
	{
		ESP_LOGW(my_tag,"display low voltage picture");
//		display_picture(0,low_network_wifi_picture_page);
		ncolor_display(0,0x44);//red
	}
	else
	{
		ESP_LOGW(my_tag,"find wakeup_cause and print it");

		//使用定时器每次醒来执行不同的任务
//		esp_sleep_wakeup_cause_t my_cause=esp_sleep_get_wakeup_cause();
//		ESP_LOGW(my_tag,"my_cause=%d",my_cause);
//		unsigned char task;
//		switch (esp_sleep_get_wakeup_cause())
//		{
//			case ESP_SLEEP_WAKEUP_UNDEFINED:
//				printf("now wakeup\n");
//				ESP_LOGW(my_tag,"init timer");
//				Timer_Config();
//				break;
//			case ESP_SLEEP_WAKEUP_EXT1:
//				spi_flash_read(composite_picture_page*4096+sizeof(unsigned char),&task, sizeof(unsigned char));
//				printf("task=%x\n",task);
//				if(task==0xff)
//				{
//					printf("first task\n");
//					Timer_Config();
//				}
//				if(task==0x11)
//				{
//					printf("second task\n");
//					Timer_Config();
//				}
//				if(task==0x22)
//				{
//					printf("third task\n");
//					Timer_Config();
//				}
//				break;
//			case ESP_SLEEP_WAKEUP_ALL:
//				printf("here\n");
//				break;
//			default:
//				ESP_LOGW(my_tag,"not sleep");
//				break;
//		}


		switch (esp_sleep_get_wakeup_cause())
		{
			case ESP_SLEEP_WAKEUP_EXT1:
				ESP_LOGW(my_tag,"ESP_SLEEP_WAKEUP_EXT1(3)= %d",esp_sleep_get_wakeup_cause());
				ESP_LOGW(my_tag,"wakeup by search next page ");
				search_pic(ESP_SLEEP_WAKEUP_EXT1);//3
				break;
			case ESP_SLEEP_WAKEUP_EXT0:
				ESP_LOGW(my_tag,"ESP_SLEEP_WAKEUP_EXT0(2) = %d",esp_sleep_get_wakeup_cause());
				ESP_LOGW(my_tag,"wakeup by search prev page");
				search_pic(ESP_SLEEP_WAKEUP_TIMER);//2
				break;
			case ESP_SLEEP_WAKEUP_TIMER:
				ESP_LOGW(my_tag,"ESP_SLEEP_WAKEUP_TIMER(4) = %d",esp_sleep_get_wakeup_cause());
				ESP_LOGW(my_tag,"wakup by timer auto_update");//4
				break;
			case ESP_SLEEP_WAKEUP_UNDEFINED:
				ESP_LOGW(my_tag,"ESP_SLEEP_WAKEUP_UNDEFINE(0) = %d",esp_sleep_get_wakeup_cause());
				ESP_LOGW(my_tag,"wakup by manual update");//0
				search_pic(ESP_SLEEP_WAKEUP_UNDEFINED);
				break;
			default:
				ESP_LOGW(my_tag,"not sleep");
				break;
		}

		esp_sleep_enable_ext0_wakeup(0ULL<<0x00, 0);
		esp_sleep_enable_ext1_wakeup(1ULL<<0x19, 0);
	}
    const int ext_wakeup_pin_1 = 25;
    const uint64_t ext_wakeup_pin_1_mask = 1ULL << ext_wakeup_pin_1;
    esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_1_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
    esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_1_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
	EventGroupHandler = xEventGroupCreate(); //创建事件标志组
	//创建事件标志组处理任务
	xTaskCreate((TaskFunction_t)eventgroup_task,
				(const char *)"eventgroup_task",
				(uint16_t)EVENTGROUP_STK_SIZE,
				(void *)NULL,
				(UBaseType_t)EVENTGROUP_TASK_PRIO,
				(TaskHandle_t *)&EventGroupTask_Handler);
//*/
	return;
}
    //test desk_calendar display picture
//    ESP_LOGW(my_tag,"test desk_calendar display picture");
//    int display_index=5;
//	char display_flag[20];
//	unsigned int start_pos=(info_page-1) * 4096;
//	spi_flash_read(start_pos, display_flag, 20);
//	if(display_flag[0]==0x55 && display_flag[1]==0xfa && display_flag[2]<13 && display_flag[18]==0xfa && display_flag[19]==0x55)
//	{
//		display_index=display_flag[2];
//	}
//	else
//	{
//		display_flag[0]=0x55;
//		display_flag[1]=0xfa;
//		display_flag[18]=0xfa;
//		display_flag[19]=0x55;
//		display_index=5;
//	}
//	int up=0;
//	switch (esp_sleep_get_wakeup_cause())
//	{
//		case ESP_SLEEP_WAKEUP_EXT1:
//			ESP_LOGW(my_tag,"ESP_SLEEP_WAKEUP_EXT1(3) = %d",esp_sleep_get_wakeup_cause());
//			up=1;
//			break;
//		case ESP_SLEEP_WAKEUP_EXT0:
//			ESP_LOGW(my_tag,"ESP_SLEEP_WAKEUP_EXT0(2) = %d",esp_sleep_get_wakeup_cause());
//			up=2;
//			break;
//		case ESP_SLEEP_WAKEUP_UNDEFINED:
//			up=2;
//			ESP_LOGW(my_tag,"ESP_SLEEP_WAKEUP_UNDEFINED(0) = %d",esp_sleep_get_wakeup_cause());
//			break;
//		default:
//			ESP_LOGW(my_tag,"not sleep");
//			break;
//	}
//	if(up==1)
//	{
//		display_index++;
//	}
//	else if(up==2)
//	{
//		display_index--;
//	}
//	if(display_index==13)
//	{
//		display_index=1;
//	}
//	else if(display_index==0)
//	{
//		display_index=12;
//	}
//	printf("enter display_index=%d\r\n",display_index);
//	display_picture(display_index);
//	display_flag[2]=display_index;
//	sf_WriteBuffer((uint8_t *)display_flag, start_pos, 20);
//	esp_sleep_enable_ext0_wakeup(0ULL<<0x00, 0);
//	esp_sleep_enable_ext1_wakeup(1ULL<<0x19, 0);
//	ESP_LOGW(my_tag,"enter sleep");
//	esp_deep_sleep_start();
	//test end

void read_write_init()
{
	struct my_data data;
	memset(&data,0,sizeof(data));
	spi_flash_read(info_page*sector_size,&data,sizeof(data));
	if(data.check_head[0]==0x55&&data.check_head[1]==0x56&&data.check_tail[0]==0x55&&data.check_tail[1]==0x56)
	{
		 ESP_LOGW(my_tag, "Start Interpreting data");
	}
	else
	{
		memset(&data,0,sizeof(data));
		spi_flash_read(info_page_backup*sector_size,&data,sizeof(data));
		if(data.check_head[0]==0x55&&data.check_head[1]==0x56&&data.check_tail[0]==0x55&&data.check_tail[1]==0x56)
		{
			spi_flash_write(info_page*sector_size,&data,sizeof(data));
			ESP_LOGW(my_tag,"Data recovery complete");
			read_write_init();
		}
		else
		{
			init_default_data();
			read_write_init();
		}
	}
}
void init_default_data()
{
	ESP_LOGW(my_tag,"Assign the system default value to an array");
	struct my_data default_data;
	struct timeval stime;
	memset(&default_data,0,sizeof(default_data));
	default_data.check_head[0]=0x55;
	default_data.check_head[1]=0x56;
	default_data.pic_number=0;
	default_data.low_power=0;
	strcpy(default_data.pic_name,"20210101.bin");
	strcpy(default_data.pic_name_current,"20210101.bin");
	strcpy(default_data.wifi_ssid,default_wifi_ssid);
	strcpy(default_data.wifi_pssd,default_wifi_pssd);
	strcpy(default_data.server_add_get_down_pic,"https://aink.net/devices/download/pic/");
	strcpy(default_data.server_add_tell_down_ok,"https://aink.net/devices/finished/pic/");
	strcpy(default_data.server_add_tell_dele_ok,"https://aink.net/devices/deleted/pic/");
	strcpy(default_data.server_add_to_downlo_pic,"https://aink.net/devices/resources/");
	default_data.network_wrong=0;
	default_data.config_wifi=0;
	default_data.check_tail[0]=0x55;
	default_data.check_tail[1]=0x56;
	default_data.time_stamp=default_time_stamp;
	stime.tv_sec = 	default_data.time_stamp;
	settimeofday(&stime,NULL);
	default_data.picture_time_stamp=default_time_stamp;
	default_data.remain_space=default_time_stamp;
	spi_flash_erase_sector(info_page);
	spi_flash_write(info_page*sector_size,&default_data,sizeof(default_data));
	spi_flash_write(info_page_backup*sector_size,&default_data,sizeof(default_data));
	ESP_LOGW(my_tag,"write default data to array done and write flash 1280 1281 done");
}
void analysis_data()
{
//	ESP_LOGW(my_tag,"sizeof(default_data)=%d",sizeof(data));
	spi_flash_read(info_page*sector_size,&current_data,sizeof(current_data));
	ESP_LOGW(my_tag,"current_data.check_head[0]=%x",current_data.check_head[0]);
	ESP_LOGW(my_tag,"current_data.check_head[1]=%x",current_data.check_head[1]);
	ESP_LOGW(my_tag,"current_data.time_stamp=%ld",current_data.time_stamp);
	gettimeofday(&stime,NULL);
	time_now=stime.tv_sec;
	current_data.time_stamp=stime.tv_sec;
//	p=localtime(&time_now);
//	ESP_LOGW(my_tag,"now it`s :%s",asctime(p));
	ESP_LOGW(my_tag,"current_data.picture_time_stamp=%ld",current_data.picture_time_stamp);
	ESP_LOGW(my_tag,"current_data.pic_name=%s",current_data.pic_name);
	ESP_LOGW(my_tag,"current_data.wifi_ssid=%s",current_data.wifi_ssid);
	ESP_LOGW(my_tag,"current_data.wifi_pssd=%s",current_data.wifi_pssd);
	ESP_LOGW(my_tag,"current_data.remain_space=%ld",current_data.remain_space);
	ESP_LOGW(my_tag,"current_data.pic_number=%d",current_data.pic_number);
	ESP_LOGW(my_tag,"current_data.server_add_get_down_pic=%s",current_data.server_add_get_down_pic);
	ESP_LOGW(my_tag,"current_data.server_add_tell_down_ok=%s",current_data.server_add_tell_down_ok);
	ESP_LOGW(my_tag,"current_data.server_add_tell_dele_ok=%s",current_data.server_add_tell_dele_ok);
	ESP_LOGW(my_tag,"current_data.server_add_to_downlo_pic=%s",current_data.server_add_to_downlo_pic);
	ESP_LOGW(my_tag,"current_data.check_tail[0]=%x",current_data.check_head[0]);
	ESP_LOGW(my_tag,"current_data.check_tail[1]=%x",current_data.check_head[1]);
	ESP_LOGW(my_tag,"get device nformation");
//	unsigned char temp_data;
//	for(int i=0;i<4096;i++)
//	{
//		 spi_flash_read(info_page*sector_size+i,&temp_data,1);
//		 if(temp_data==0xff)
//		 {
//			 break;
//		 }
//		 ESP_LOGI(my_tag,"temp_data %d =%x", i+1,temp_data);
//	}
}
void search_pic(esp_sleep_source_t wakeup_cause)
{
	unsigned char i;
	char temp_name[20];
	for (i = 0; i < current_data.pic_number; i++)
	{
		spi_flash_read(info_pic_name + i * 20, temp_name, 20);
		if (strcmp((char *)temp_name, current_data.pic_name) == 0)
		{
			break;
		}
	}
	switch (esp_sleep_get_wakeup_cause())
	{
		case ESP_SLEEP_WAKEUP_EXT1:
			ESP_LOGW(my_tag,"ESP_SLEEP_WAKEUP_EXT1(3)= %d",esp_sleep_get_wakeup_cause());
			ESP_LOGW(my_tag,"wakeup by search next page ");
			if(i+1<current_data.pic_number)
			{
				ESP_LOGW(my_tag,"Search next page success");
				ESP_LOGW(my_tag,"now ,display it");
				spi_flash_read(info_pic_name + (i+1) * 20, temp_name, 20);
				display_picture(i+1,picture_page);
				check_wifi_httpdowload_pic(temp_name,2);//下载此页(temp_name)的下一页但不显示
			}
			else
			{
				ESP_LOGW(my_tag,"Search  next page  failed");
				ESP_LOGW(my_tag,"now , download next page ");
				check_wifi_httpdowload_pic(current_data.pic_name,2);//下载此页的下一页并显示
			}
			break;
		case ESP_SLEEP_WAKEUP_EXT0:
			ESP_LOGW(my_tag,"ESP_SLEEP_WAKEUP_EXT0(2) = %d",esp_sleep_get_wakeup_cause());
			ESP_LOGW(my_tag,"wakeup by search prev page");
			if(i>0)
			{
				ESP_LOGW(my_tag,"Search prev  page success");
				ESP_LOGW(my_tag,"now ,display it");
				display_picture(i-1,picture_page);
				spi_flash_read(info_pic_name + (i-1) * 20, temp_name, 20);
				check_wifi_httpdowload_pic(temp_name,1);//下载此页(temp_name)的上一页但不显示
			}
			else
			{
				check_wifi_httpdowload_pic(current_data.pic_name,1);//下载上一页并显示
			}
			break;
		case ESP_SLEEP_WAKEUP_TIMER:
			ESP_LOGW(my_tag,"ESP_SLEEP_WAKEUP_TIMER(4) = %d",esp_sleep_get_wakeup_cause());
			ESP_LOGW(my_tag,"wakup by timer auto_update");//4
			if(i+1<current_data.pic_number)
			{
				ESP_LOGW(my_tag,"Search next page success");
				ESP_LOGW(my_tag,"now ,display it");
				spi_flash_read(info_pic_name + (i+1) * 20, temp_name, 20);
				display_picture(i+1,picture_page);
				check_wifi_httpdowload_pic(temp_name,0);//下载此页(temp_name)的下一页但显示
			}
			else
			{
				ESP_LOGW(my_tag,"Search  next page  failed");
				ESP_LOGW(my_tag,"now , download next page ");
				check_wifi_httpdowload_pic(current_data.pic_name,0);//下载此页的下一页并显示
			}
			break;
		case ESP_SLEEP_WAKEUP_UNDEFINED:
			ESP_LOGW(my_tag,"ESP_SLEEP_WAKEUP_UNDEFINE(0) = %d",esp_sleep_get_wakeup_cause());
			ESP_LOGW(my_tag,"wakup by manual update");//0
			check_wifi_httpdowload_pic(current_data.pic_name,3);//自动更新
			break;
		default:
			ESP_LOGW(my_tag,"not sleep");
			break;
	}
}

//void string2int(const char * string)
//{
//    int value = 0;
//    int index = 0;
//    for(;string[index] >= '0' && string[index] <= '9'; index ++)
//    {
//        value = value * 10 + string[index] - '0';
//    }
//    return value;
//}

void inttostring(long value, char * output)
{
    int index = 0;
    if(value == 0)
    {
        output[0] = value + '0';
    }
    else
    {
        while(value)
        {
            output[index] = value % 10 + '0';
            index ++;
            value /= 10;
        }
    }
}

void ucharconnectchar(char a[],unsigned char b[])
{
	unsigned char i=0,j=0;
	while(a[i++]==0)
	{
		break;
	}
	for(;i<200;i++)
	{
		a[i]=b[j];
		j++;
		if(b[j]==0)
		{
			break;
		}
	}
}

void check_wifi_httpdowload_pic(char pic_name[20],int wakeup_cause)
{
	ESP_LOGW(my_tag,"isconnected=%d",isconnected);
	if(isconnected)
	{
		for(unsigned char i=0;i<50;i++)
		{
			download_url[i]=current_data.server_add_get_down_pic[i];
		}
		ucharconnectchar(download_url,&device_info[8]);
		strcat(download_url,"?");
		strcat(download_url,"picname=");
		strcat(download_url,current_data.pic_name);
		strcat(download_url,"&");
		strcat(download_url,"picsize=");
		char temp_pic_number[1];
		inttostring((int)current_data.pic_number,temp_pic_number);
		strcat(download_url,temp_pic_number);
		strcat(download_url,"&");
		strcat(download_url,"voltage=");
		char temp_voltage[2];
		inttostring((int)voltage,temp_voltage);
		strcat(download_url,temp_pic_number);
		strcat(download_url,"&");
		strcat(download_url,"action=");
		char temp_wakeup_caues[1];
		inttostring((int)wakeup_cause,temp_wakeup_caues);
		strcat(download_url,temp_wakeup_caues);
		strcat(download_url,"&");
		strcat(download_url,"timestamp=");
		unsigned char i=1;
		while(current_data.time_stamp/10)
		{
			i++;
		}
		char temp_time_stamp[i];
		inttostring(current_data.time_stamp,temp_time_stamp);
		strcat(download_url,temp_time_stamp);
		ESP_LOGW(my_tag,"Request server(%s) try to get download_picture_name",download_url);
		http_test_task(download_url);
	}
	else
	{
		if(strcmp(current_data.wifi_ssid,default_wifi_ssid)==0&&strcmp(current_data.wifi_pssd,default_wifi_pssd)==0)
		{
//			display_picture(2,low_network_wifi_picture_page);
			ncolor_display(0,0x55);//yellow
			wifi_config_page=1;
			getdeviceinfo();
			esp_ble_gap_config_adv_data(&adv_data);
		}
		else
		{
//			display_picture(1,low_network_wifi_picture_page);
			ncolor_display(0,0x33);//blue
		}
		esp_timer_start_periodic(periodic_timer, 15*1000 * 1000);//三分钟后睡眠
	}
}

//void cJSON_data(char *json_str)
void cJSON_data()
{
//	{\"picname\":\"20201222.bin\"},{\"picname\":\"20201223.bin\"},{\"picname\":\"20201224.bin\"}
	struct timeval stime;
	char * json_str = "{\"timestame\":56325142,\"deletepic\":\"20201011.bin\",\"downloadlist\":[], \"lowvol\":\"lowvol.bin\", \"networkerr\":\"networkerr.bin\", \"qcode\":\"xx\"}";
	cJSON * root = NULL;
	cJSON * item = NULL;
	cJSON * item_temp = NULL;
	char temp_name[20];
	root = cJSON_Parse(json_str);
	if (!root)
	{
		printf("Error before: [%s]\n",cJSON_GetErrorPtr());
	}
	else
	{
		item = cJSON_GetObjectItem(root, "succ");
		if(item!=NULL)
		{
			if(strcmp(item->valuestring,"ok")==0)
			{
				failed_bit=1;
				failed_times=0;
			}
			else
			{
				if(failed_times>2)
				{
					failed_bit=1;
					failed_times=0;
					ESP_LOGW(my_tag,"picture download failed (more than twice)");
					display_picture(1,low_network_wifi_picture_page);
					ESP_LOGW(my_tag,"go to sleep");
					esp_deep_sleep_start();
				}
			}
		}
		else
		{
//			printf("%s\n", "cJSON(format):");
//			printf("%s\n", cJSON_Print(root));

			printf("%s\n", "get timestame cJSON object:");
			item = cJSON_GetObjectItem(root, "timestame");
			stime.tv_sec=item->valueint;
			settimeofday(&stime,NULL);
			printf("%s\n", cJSON_Print(item));
			ESP_LOGW(my_tag,"%s:%d, and set ESP32 time done!!!!!", item->string,item->valueint);

			printf("%s\n", "get lowvol cJSON object :");
			item = cJSON_GetObjectItem(root, "lowvol");
			printf("%s\n", cJSON_Print(item));
			ESP_LOGW(my_tag,"%s:%s", item->string,item->valuestring);
			download_composite(item);


			printf("%s\n", "get networkerr cJSON object :");
			item = cJSON_GetObjectItem(root, "networkerr");
			printf("%s\n", cJSON_Print(item));
			ESP_LOGW(my_tag,"%s:%s", item->string,item->valuestring);
			download_composite(item);

			printf("%s\n", "get qcode cJSON object :");
			item = cJSON_GetObjectItem(root, "qcode");
			printf("%s\n", cJSON_Print(item));
			ESP_LOGW(my_tag,"%s:%s", item->string,item->valuestring);
			download_composite(item);

			printf("%s\n", "get deletepic cJSON object:");
			item = cJSON_GetObjectItem(root, "deletepic");
			printf("%s\n", cJSON_Print(item));
			ESP_LOGW(my_tag,"%s:%s", item->string,item->valuestring);
			for (unsigned char i = 0; i < current_data.pic_number; i++)
			{
				spi_flash_read(info_pic_name + i * 20, temp_name, 20);
				if (strcmp((char *)temp_name, item->valuestring) == 0)
				{
					ESP_LOGW(my_tag, "find same name picture,now delete it");
					spi_flash_read(info_page*4096,&current_data,sizeof(current_data));
					current_data.pic_number--;
					spi_flash_write(info_page*4096,&current_data,sizeof(current_data));
					temp_name[8]='*';
					sf_WriteBuffer((uint8_t *)temp_name,info_pic_name + i * 20,20);
					break;
				}
			}
			download_composite(item);

			printf("%s\n", "get downloadlist cJSON object:");
			item_temp = cJSON_GetObjectItem(root, "downloadlist");
			ESP_LOGW(my_tag,"%s", cJSON_Print(item_temp));

			printf("%s\n", "get picname cJSON object:");
			if( NULL != item_temp )
			{
				cJSON *client_list  = item_temp->child;
				if( NULL == client_list )
				{
					ESP_LOGW(my_tag,"picname is NULL,no need to download picture(s)");
					ESP_LOGW(my_tag,"go to sleep");
					esp_deep_sleep_start();
				}
				else
				{
					while( client_list != NULL )
					{
						failed_times++;
						download_composite(item);
						if(failed_bit==1)
						{
							client_list = client_list->next;
						}
					}
					ESP_LOGW(my_tag,"all pictures downloaded");
					ESP_LOGW(my_tag,"go to sleep");
					esp_deep_sleep_start();
				}
			}
			else
			{
				ESP_LOGW(my_tag,"no downloadlist string");
				ESP_LOGW(my_tag,"go to sleep");
				esp_deep_sleep_start();
			}
		}
	}
}

void download_composite(cJSON * item)
{
	memset(download_url,0,200);
	char * temp_string="deletepic";
	if(item->string==temp_string)
	{
		for(unsigned char i=0;i<50;i++)
		{
			download_url[i]=current_data.server_add_tell_dele_ok[i];
		}
	}
	else
	{
		for(unsigned char i=0;i<50;i++)
		{
			download_url[i]=current_data.server_add_to_downlo_pic[50];
		}
		ucharconnectchar(download_url,&device_info[8]);
		strcat(download_url,"/");
		strcat(download_url,item->valuestring);
		http_test_task(download_url);

		memset(download_url,0,200);
		for(unsigned char i=0;i<50;i++)
		{
			download_url[i]=current_data.server_add_tell_down_ok[i];
		}
	}
	ucharconnectchar(download_url,&device_info[8]);
	strcat(download_url,"?");
	strcat(download_url,"picname=");
	strcat(download_url,item->valuestring);
	strcat(download_url,"&");
	strcat(download_url,"timestamp=");
	unsigned char i=1;
	while(current_data.time_stamp/10)
	{
		i++;
	}
	char temp_time_stamp[i];
	inttostring(current_data.time_stamp,temp_time_stamp);
	strcat(download_url,temp_time_stamp);
	http_test_task(download_url);
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
