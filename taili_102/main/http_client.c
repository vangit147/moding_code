#include "http_client.h"

int  picname_times=0;
char low_power_picname[20]="lowvol.bin";
char network_wrong_picname[20]="networkerr.bin";
char config_wifi_picname[20]="qcode.bin";

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(http_tag, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(http_tag, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(http_tag, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(http_tag, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(http_tag, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(http_tag, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(http_tag, "HTTP_EVENT_DISCONNECTED");
        break;
    }
    return ESP_OK;
}

int http_test_task(char *dpwn_url)
{
	int data_read_size = 0, bytes_read = 0;

    char *buffer = malloc(sector_size + 1);

    if (buffer == NULL)
    {
        ESP_LOGE(http_tag, "Cannot malloc http receive buffer");
        return 0;
    }

    esp_http_client_config_t config = {
    	.url = dpwn_url,
        .event_handler = _http_event_handler,
    };

    wakeup_cause=*(strstr(config.url,"action")+7);
    ESP_LOGW(http_tag,"config.url=%s",config.url);
    ESP_LOGW(http_tag,"wakeup_cause=%c",wakeup_cause);

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err;
    unsigned char failed_times=3;
    while(failed_times)
    {
    	if ((err = esp_http_client_open(client, 0)) != ESP_OK)
		{
			ESP_LOGE(http_tag, "Failed to open HTTP connection: %s", esp_err_to_name(err));
			failed_times--;
			if(failed_times==0)
			{
				ESP_LOGE(http_tag, "Failed (tried 3 times) to open HTTP connection");
				free(buffer);
				if(current_data.network_wrong_state==0)
				{
					display_picture_temp(1,low_network_wifi_picture_page);
				}
				else
				{
					ESP_LOGE(http_tag,"flash no low_network_wifi_picture_page to display");
				}
				ESP_LOGW(http_tag,"sleep");
				esp_deep_sleep_start();
				return 0;
			}
		}
		else
		{
			break;
		}

    }


    ESP_LOGE(http_tag, "start_download");

    char *q=buffer;
    int  read_len=-1;
    int  content_length,i=0;

    content_length = esp_http_client_fetch_headers(client);
    data_read_size = esp_http_client_get_content_length(client);
    ESP_LOGW(http_tag,"content_length(read from https)=%d",content_length);
    ESP_LOGW(http_tag,"data_read_size(read from https)=%d",data_read_size);

    while(1)
    {
    	if(read_len==0)
    	{
    		break;
    	}
        read_len = esp_http_client_read(client,q++,1);
        if(read_len==1)
        {
        	i++;
        }
    }

    content_length=i;
    ESP_LOGW(http_tag,"content_length(read from https)=%d",content_length);

    if (content_length <= 0)
    {
    	ESP_LOGE(http_tag, "Error read data");
    	ESP_LOGW(http_tag,"sleep");
    	esp_deep_sleep_start();
    }

    if (content_length <= sector_size)
	{
		buffer[content_length] = 0;
		unsigned char len_url=strlen(current_data.server_add_to_downlo_pic);
		char download_temp[len_url];
		strncpy(download_temp,config.url,len_url);
		if(strcmp(download_temp,current_data.server_add_to_downlo_pic)!=0)
		{
			cJSON_data(buffer);
		}
	}
    else
    {
      	memset(PICNAME,0,20);
      	memcpy(PICNAME,&config.url[strlen(current_data.server_add_to_downlo_pic)-1],12);
      	search_dis_pic();
		memset(buffer,0,sector_size+1);
		if(current_data.low_power_state==0||current_data.network_wrong_state==0||current_data.config_wifi_state==0)
		{
			while (data_read_size > 0)
			{
				ESP_LOGE(http_tag, "read %d", bytes_read);
				read_len = esp_http_client_read(client, buffer, sector_size);
				spi_flash_write((low_network_wifi_picture_page + picture_cap * picture_index) * 4096 + bytes_read * sector_size, buffer, read_len);
				if(bytes_read==37)
				{
					 spi_flash_write((low_network_wifi_picture_page + picture_cap * picture_index) * 4096 + bytes_read * (sector_size/2), buffer, read_len);
				}
				bytes_read++;
				data_read_size -= read_len;
			}
			ESP_LOGI(http_tag, "read_times = %d", bytes_read);
			ESP_LOGE(http_tag, "wrote low_network_wifi_picture_page_data to %d ",low_network_wifi_picture_page);
		}
		else
		{
			while (data_read_size > 0)
			{
				ESP_LOGE(http_tag, "read %d", bytes_read);
				read_len = esp_http_client_read(client, buffer, sector_size);
				display_picture(buffer);
				bytes_read++;
				data_read_size -= read_len;
			}
			ESP_LOGI(http_tag, "read_times = %d", bytes_read);
			ESP_LOGI(http_tag, "displayed picture %s ", current_data.pic_name_current);
		}
    }

    ESP_LOGI(http_tag, "HTTP Stream reader Status = %d, content_length = %d",esp_http_client_get_status_code(client),esp_http_client_get_content_length(client));
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    free(buffer);
    ESP_LOGE(http_tag, "close http(s) cleanup client free buffer done");
    return 1;
}


void search_dis_pic()
{
	char time_string[20];

  	p=localtime(&time_now);
	int tm_year=p->tm_year+1900;
	int tm_mon=p->tm_mon+1;
	int tm_mday=p->tm_mday;
	int date=tm_year*10000+tm_mon*100+tm_mday;

  	if(strcmp(PICNAME,low_power_picname)==0)
  	{
  		picture_index=0;
  		current_data.low_power_state=0;
  	}
  	else if(strcmp(PICNAME,network_wrong_picname)==0)
	{
		picture_index=1;
		current_data.network_wrong_state=0;
	}
  	else if(strcmp(PICNAME,config_wifi_picname)==0)
  	{
  		picture_index=2;
  		current_data.config_wifi_state=0;
  	}
  	else
	{
  		if(wakeup_cause=='1')
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
		if(wakeup_cause=='2')
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

  		if(strcmp(PICNAME,time_string)==0)
  		{
			if(wakeup_cause=='0')
			{
				if(strcmp(PICNAME,current_data.pic_name_current)!=0)
				{
					picname_times=0;
					strcpy(current_data.pic_name_current,PICNAME);
				}
				else
				{
					ESP_LOGW(http_tag,"udpated picture : %s %d times in one day",PICNAME,picname_times++);
				}
			}
			else
			{
				strcpy(current_data.pic_name,PICNAME);
			}
  		}
  		else
		{
			ESP_LOGE(http_tag,"not the picture what will display, something wrong ,check your picture_time on your picture or check your picname from server");
			ESP_LOGE(timer_tag,"sleep");
			esp_deep_sleep_start();
		}
	}
  	updated_esp_time();
  	updated_data_to_flash();
 }


//    	if(flag==0)
//    	{
//			unsigned char i;
//    		char temp_name[20];
//			for (i = 0; i < current_data.pic_number; i++)
//			{
//				spi_flash_read(info_pic_name + i * 20, temp_name, 20);
//				if (strcmp((char *)temp_name, current_data.pic_name) == 0)
//				{
//					ESP_LOGW(http_tag, "find same name picture");
//					picture_index = i;
//					break;
//				}
//			}
//			if(i==current_data.pic_number)
//			{
//				picture_index=current_data.pic_number;
//				sf_WriteBuffer((uint8_t *)picname, info_pic_name + current_data.pic_number * 20, 20);
//				current_data.pic_number++;
//			}
//			picture_page_index=picture_page;
//    	}
//    	else
//    	{
//    		picture_page_index=low_network_wifi_picture_page;
//    		ESP_LOGE(http_tag, "start_earse flash size");
//			spi_flash_erase_range((picture_page_index + picture_cap * picture_index) * 4096, picture_cap * 4096); //Çå³ýflashÄÚ´æ
//			ESP_LOGE(http_tag, "earse flash end");
//    		sf_WriteBuffer((uint8_t *)picname, info_pic_name_for_err + picture_index * 20, 20);
//    	}
//    	udpated_data_to_flash();
