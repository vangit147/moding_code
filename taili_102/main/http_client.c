#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "string.h"
#include "freertos/event_groups.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "spiff.h"
#include "data_flash.h"
#include "cJSON.h"
#include "esp_sleep.h"
#include "time.h"

char wakeup_cause=-1;
extern struct timeval stime;
extern struct tm *p;
extern time_t time_now;

extern int time_count;
extern FILE *writefile;
extern unsigned char picture_num; //图片数量
extern void cJSON_data(char *json_str);
extern void display_picture(char *buffer);
extern void display_picture_temp(int display_index,int picture_page_index);
static const char *TAG = "http_client";
#define my_tag "desk_calender"
#define MAX_HTTP_RECV_BUFFER 4096

//update van
char epd_data[4096]; //该数组最大可以设置为55928
extern int url_length;

extern void analysis_data();
extern void inttostring(long value, char * output);
/**************************************************/
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    }
    return ESP_OK;
}

int http_test_task(char *dpwn_url)
{
	int data_read_size = 0, bytes_read = 0;
	char picname[20];
	char temp_name[20];
	char low_power_picname[20]="lowvol.bin";
	char network_wrong_picname[20]="networkerr.bin";
	char config_wifi_picname[20]="qcode.bin";
	unsigned char picture_index;
	int picture_page_index;
	unsigned char flag=0;
    char *buffer = malloc(MAX_HTTP_RECV_BUFFER + 1);

    if (buffer == NULL)
    {
        ESP_LOGE(TAG, "Cannot malloc http receive buffer");
        return 0;
    }

    analysis_data();
    spi_flash_read(info_page*4096,&current_data,sizeof(current_data));

    esp_http_client_config_t config = {
    	.url = dpwn_url,
        .event_handler = _http_event_handler,
    };

    wakeup_cause=*(strstr(config.url,"action")+7);

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err;
    unsigned char failed_times=3;
    while(failed_times)
    {
    	if ((err = esp_http_client_open(client, 0)) != ESP_OK)
		{
			ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
			failed_times--;
			if(failed_times==0)
			{
				ESP_LOGW(my_tag, "Failed (tried 3 times) to open HTTP connection");
				free(buffer);
				display_picture_temp(1,low_network_wifi_picture_page);
				ESP_LOGW(my_tag,"sleep");
				esp_deep_sleep_start();
				return 0;
			}
		}
		else
		{
			break;
		}
    }

    ESP_LOGW(my_tag,"config.url=%s",config.url);
    int content_length = esp_http_client_fetch_headers(client);
    int total_read_len = 0, read_len;
    data_read_size = esp_http_client_get_content_length(client);


    ESP_LOGE(TAG, "start_earse flash size");
    spi_flash_erase_range((picture_page + picture_cap * current_data.pic_number) * 4096, picture_cap * 4096); //清除flash内存
    ESP_LOGE(TAG, "earse flash end");

    ESP_LOGE(TAG, "start_download");
    ESP_LOGW(my_tag,"content_length=%d",content_length);
    ESP_LOGW(my_tag,"data_read_size=%d",data_read_size);
    if (total_read_len < content_length && content_length <= MAX_HTTP_RECV_BUFFER)
    {
        read_len = esp_http_client_read(client, buffer, content_length);
        if (read_len <= 0)
        {
            ESP_LOGE(TAG, "Error read data\n");
        }
        buffer[read_len] = 0;
        ESP_LOGD(TAG, "read_len = %d\n", read_len);
        //analysis data from buffer by cJSON
	   unsigned char len_url=strlen(current_data.server_add_to_downlo_pic);
	   char download_temp[len_url];
	   strncpy(download_temp,config.url,len_url);
	   if(strcmp(download_temp,current_data.server_add_to_downlo_pic)!=0&&bytes_read==0)
	   {
		   cJSON_data(buffer);
	   }
    }
    else
    {
    	memset(picname,0,20);
    	memcpy(picname,&config.url[strlen(current_data.server_add_to_downlo_pic)-1],12);

    	if(strcmp(picname,low_power_picname)==0)
    	{
    		picture_index=0;
    		flag++;
    	}
		if(strcmp(picname,network_wrong_picname)==0)
		{
			picture_index=1;
			flag++;
		}
    	if(strcmp(picname,config_wifi_picname)==0)
    	{
    		picture_index=2;
    		flag++;
    	}

    	if(flag==0)
    	{
//			unsigned char i;
//			for (i = 0; i < current_data.pic_number; i++)
//			{
//				spi_flash_read(info_pic_name + i * 20, temp_name, 20);
//				if (strcmp((char *)temp_name, current_data.pic_name) == 0)
//				{
//					ESP_LOGW(my_tag, "find same name picture");
//					picture_index = i;
//					break;
//				}
//			}
//			if(i==current_data.pic_number)
//			{
				picture_index=current_data.pic_number;
				sf_WriteBuffer((uint8_t *)picname, info_pic_name + current_data.pic_number * 20, 20);
				current_data.pic_number++;
				spi_flash_write(info_page*4096,&current_data,sizeof(current_data));
//			}
			picture_page_index=picture_page;
    	}
    	else
    	{
        	picture_page_index=low_network_wifi_picture_page;
    		sf_WriteBuffer((uint8_t *)picname, info_pic_name_for_err + picture_index * 20, 20);
    	}

    	p=localtime(&time_now);
		int tm_year=p->tm_year+1900;
		int tm_mon=p->tm_mon+1;
		int tm_mday=p->tm_mday;
		char time_string[20];
		int date=tm_year*10000+tm_mon*100+tm_mday;
		if(wakeup_cause==1)
		{
			if(tm_mon==1)
			{
				tm_mon=12;
				tm_year--;
			}
			else
			{
				tm_mon--;
			}
			tm_mday=1;
		}
		if(wakeup_cause==2)
		{
			if(tm_mon==12)
			{
				tm_mon=1;
				tm_year++;
			}
			else
			{
				tm_mon++;
			}
			tm_mday=1;
		}
		date=tm_year*10000+tm_mon*100+tm_mday;
		inttostring(date,time_string);
		strcat(time_string,".bin");

        memset(buffer,0,MAX_HTTP_RECV_BUFFER+1);
        while (data_read_size > 0)
        {
            ESP_LOGE(TAG, "read %d", bytes_read);
            read_len = esp_http_client_read(client, buffer, MAX_HTTP_RECV_BUFFER);
            if(flag==1)
            {
				spi_flash_write((picture_page_index + picture_cap * picture_index) * 4096 + bytes_read * MAX_HTTP_RECV_BUFFER, buffer, read_len);
				if(bytes_read==37)
				{
					 spi_flash_write((picture_page_index + picture_cap * picture_index) * 4096 + bytes_read * (MAX_HTTP_RECV_BUFFER/2), buffer, read_len);
				}
            }
            else
            {
            	if(strcmp(picname,time_string)==0)
				{
					display_picture(buffer);
				}
            }
            bytes_read++;
            data_read_size -= read_len;
        }
    }


    ESP_LOGE(TAG, "write end");
    ESP_LOGI(TAG, "read_times = %d", bytes_read);
    ESP_LOGI(TAG, "HTTP Stream reader Status = %d, content_length = %d",esp_http_client_get_status_code(client),esp_http_client_get_content_length(client));
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    free(buffer);
    ESP_LOGE(TAG, "free buffer end");
    return 1;
}
