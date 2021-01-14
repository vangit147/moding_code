#include "data_flash.h"

char *deletepic="deletepic=";
char *picname="picname=";
char *picsize="picsize=";
char *Voltage="voltage=";
char *action="action=";
char *timestamp="timestamp=";
unsigned char wifi_config_page=0;
char default_wifi_ssid[40]="moding_wifi";
char default_wifi_pssd[40]="modingtech.com";
long default_time_stamp=1609430400;// 2021/1/1  00:00:00
char download_URL[200];
unsigned char failed_bit=0;
unsigned char failed_times=0;




void find_wakeup_cause()
{
	switch (esp_sleep_get_wakeup_cause())
	{
		case ESP_SLEEP_WAKEUP_EXT1:
			ESP_LOGW(my_tag,"ESP_SLEEP_WAKEUP_EXT1(3)= %d",esp_sleep_get_wakeup_cause());
			ESP_LOGW(my_tag,"wakeup by search next page ");
			ESP_LOGW(my_tag,"download next page and display it");
//			check_wifi_httpdowload_pic('2');

			break;
		case ESP_SLEEP_WAKEUP_EXT0:
			ESP_LOGW(my_tag,"ESP_SLEEP_WAKEUP_EXT0(2) = %d",esp_sleep_get_wakeup_cause());
			ESP_LOGW(my_tag,"wakeup by search prev page");
			ESP_LOGW(my_tag,"download prev page and display it");
//			check_wifi_httpdowload_pic('1');//下载上一页并显示

			break;
		case ESP_SLEEP_WAKEUP_TIMER:
			ESP_LOGW(my_tag,"ESP_SLEEP_WAKEUP_TIMER(4) = %d",esp_sleep_get_wakeup_cause());
			ESP_LOGW(my_tag,"wakup by timer auto_update");//4
//			check_wifi_httpdowload_pic('0');

			break;
		case ESP_SLEEP_WAKEUP_GPIO:
			ESP_LOGW(my_tag,"ESP_SLEEP_WAKEUP_UNDEFINE(0) = %d",esp_sleep_get_wakeup_cause());
			ESP_LOGW(my_tag,"wakup by manual update");//0
//			check_wifi_httpdowload_pic('1');//自动更新
			break;
		case ESP_SLEEP_WAKEUP_UNDEFINED:
			break;
		default:
			ESP_LOGW(my_tag,"other wakeup cause ----> deep sleep start");

			break;
	}


	esp_sleep_enable_gpio_wakeup();
	esp_sleep_enable_ext0_wakeup(0ULL<<0x00, 0);
	esp_sleep_enable_ext1_wakeup(1ULL<<0x19, 0);
	if(esp_sleep_get_wakeup_cause()==ESP_SLEEP_WAKEUP_GPIO)
	{
		ESP_LOGE(timer_tag,"light sleep");
		esp_light_sleep_start();
	}
	else
	{
		ESP_LOGE(timer_tag,"deep sleep");
		esp_light_sleep_start();
	}
}

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
	memset(&default_data,0,sizeof(default_data));

	default_data.check_head[0]=0x55;
	default_data.check_head[1]=0x56;
	default_data.pic_number=0;
	default_data.low_power_state=-1;
	strcpy(default_data.pic_name,"20210101.bin");
	strcpy(default_data.pic_name_current,"20210112.bin");
	strcpy(default_data.wifi_ssid,default_wifi_ssid);
	strcpy(default_data.wifi_pssd,default_wifi_pssd);
	strcpy(default_data.server_add_get_down_pic,"https://aink.net/devices/download/pic/");
	default_data.network_wrong_state=-1;
	default_data.config_wifi_state=-1;
	strcpy(default_data.server_add_tell_down_ok,"https://aink.net/devices/finished/pic/");
	strcpy(default_data.server_add_tell_dele_ok,"https://aink.net/devices/deleted/pic/");
	strcpy(default_data.server_add_to_downlo_pic,"https://aink.net/devices/resources/");
	default_data.check_tail[0]=0x55;
	default_data.check_tail[1]=0x56;
	default_data.time_stamp=default_time_stamp;
	stime.tv_sec = 	default_data.time_stamp;
	settimeofday(&stime,NULL);
	default_data.picture_time_stamp=default_time_stamp;
	default_data.remain_space=11534336-2*4*1024-picture_cap*(current_data.pic_number+current_data.low_power_state+current_data.config_wifi_state+current_data.network_wrong_state);

	spi_flash_erase_sector(info_page);
	spi_flash_erase_sector(info_page_backup);
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

void updated_esp_time()
{
	gettimeofday(&stime,NULL);
	time_now=stime.tv_sec;
	current_data.time_stamp=stime.tv_sec;
	p=localtime(&time_now);
	p->tm_hour=p->tm_hour+8;
	ESP_LOGW(my_tag,"now it`s :%s",asctime(p));
}

void updated_data_to_flash()
{
	spi_flash_erase_sector(info_page);
	spi_flash_erase_sector(info_page_backup);
	spi_flash_write(info_page*4096,&current_data,sizeof(current_data));
	spi_flash_write(info_page_backup*4096,&current_data,sizeof(current_data));
	ESP_LOGW(my_tag,"udpated data to flash 1280 1281 sector");
}

void check_wifi_httpdowload_pic(char wakeup_cause)
{
	ESP_LOGW(my_tag,"isconnected=%d",isconnected);
	if(isconnected)
	{
		memset(download_URL,'\0',200);
		strcat(download_URL,current_data.server_add_get_down_pic);
		charconnectuchar(download_URL,&device_info[8]);
		download_URL[strlen(download_URL)]=QUES;
		strcat(download_URL,picname);
		if(wakeup_cause==0)
		{
			strcat(download_URL,current_data.pic_name_current);
		}
		else
		{
			strcat(download_URL,current_data.pic_name);
		}
		download_URL[strlen(download_URL)]=AND;
		strcat(download_URL,picsize);
		char char_pic_number[2];
		inttostring((int)current_data.pic_number,char_pic_number);
		char_pic_number[1]='\0';
		download_URL[strlen(download_URL)]=char_pic_number[0];
		download_URL[strlen(download_URL)]=AND;
		strcat(download_URL,Voltage);
		char char_voltage[4];
		inttostring(328,char_voltage);
		char_voltage[3]='\0';
		strcat(download_URL,char_voltage);
		download_URL[strlen(download_URL)]=AND;
		strcat(download_URL,action);
		download_URL[strlen(download_URL)]=wakeup_cause;
		download_URL[strlen(download_URL)]=AND;
		strcat(download_URL,timestamp);
		char temp_time_stamp[50];
//		inttostring(1234567890,temp_time_stamp);
//		inttostring(current_data.time_stamp,temp_time_stamp);
		inttostring(default_time_stamp,temp_time_stamp);
		temp_time_stamp[10]='\0';
		strcat(download_URL,temp_time_stamp);

		ESP_LOGW(my_tag,"Request server try to get download_picture_name");
		ESP_LOGE(my_tag,"url=%s",download_URL);
		http_test_task(download_URL);
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

void cJSON_data(char *json_str)
{
//	{\"picname\":\"20201222.bin\"},{\"picname\":\"20201223.bin\"},{\"picname\":\"20201224.bin\"}
//	char * json_str = "{\"timestame\":56325142,\"deletepic\":\"20201011.bin\",\"downloadlist\":[], \"lowvol\":\"lowvol.bin\", \"networkerr\":\"networkerr.bin\", \"qcode\":\"xx\"}";
	struct timeval stime;
	cJSON * root = NULL;
	cJSON * item = NULL;
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
				failed_times=0;
				failed_bit=1;
			}
			else
			{
				if(failed_times>2)
				{
					failed_times=0;
					ESP_LOGW(my_tag,"picture download failed (more than twice)");
//					display_picture_temp(1,low_network_wifi_picture_page);
					ESP_LOGW(my_tag,"go to sleep");
					esp_deep_sleep_start();
				}
			}
		}
		else
		{
			printf("%s\n", "cJSON(format):");
			printf("%s\n", cJSON_Print(root));

			printf("%s\n", "get timestamp cJSON object:");
			item = cJSON_GetObjectItem(root, "timestamp");
			stime.tv_sec=item->valueint;
			settimeofday(&stime,NULL);
			printf("%s\n", cJSON_Print(item));
			printf("%ld\n", stime.tv_sec);
			ESP_LOGW(my_tag,"%s:%ld:%d, and set ESP32 time done!!!!!", item->string,item->valuelong,item->valueint);
			updated_esp_time();
			updated_data_to_flash();

			item = cJSON_GetObjectItem(root, "picname");
			if(item!=NULL)
			{
				while(failed_bit==0)
//				while(failed_times<=2)
				{
					printf("%s\n", "get picname cJSON object:");
					printf("%s\n", cJSON_Print(item));
					failed_times++;
					download_composite(item);
				}
			}
			else
			{
				ESP_LOGW(my_tag,"picname is NULL,no need to download picture(s)");
				ESP_LOGW(my_tag,"go to sleep");
				esp_deep_sleep_start();
			}

			item = cJSON_GetObjectItem(root, "lowvol");
			if(item!=NULL)
			{
				printf("%s\n", "get lowvol cJSON object :");
				printf("%s\n", cJSON_Print(item));
				ESP_LOGW(my_tag,"%s:%s", item->string,item->valuestring);
				download_composite(item);
			}

			item = cJSON_GetObjectItem(root, "networkerr");
			if(item!=NULL)
			{
				printf("%s\n", "get networkerr cJSON object :");
				printf("%s\n", cJSON_Print(item));
				ESP_LOGW(my_tag,"%s:%s", item->string,item->valuestring);
				download_composite(item);
			}

			item = cJSON_GetObjectItem(root, "qcode");
			if(item!=NULL)
			{
				printf("%s\n", "get qcode cJSON object :");
				printf("%s\n", cJSON_Print(item));
				ESP_LOGW(my_tag,"%s:%s", item->string,item->valuestring);
				download_composite(item);
			}

//			item = cJSON_GetObjectItem(root, "deletepic");
//			if(item!=NULL)
//			{
//				printf("%s\n", "get deletepic cJSON object:");
//				printf("%s\n", cJSON_Print(item));
//				ESP_LOGW(my_tag,"%s:%s", item->string,item->valuestring);
//				download_composite(item);
//			}
		}
	}
}

void download_composite(cJSON * item)
{
	memset(download_URL,0,200);
	printf("%s\n",item->string);
	printf("%s\n",item->valuestring);
	if(strcmp(item->string,"deletepic")==0)
	{
		ESP_LOGW(my_tag,"item->string=deletepic");
		strcat(download_URL,current_data.server_add_tell_dele_ok);

	}
	if(strcmp(item->string,"picname")==0||strcmp(item->string,"lowvol")||strcmp(item->string,"networkerr")||strcmp(item->string,"qcode"))
	{
		ESP_LOGW(my_tag,"item->string=picname");
		strcat(download_URL,current_data.server_add_to_downlo_pic);
		charconnectuchar(download_URL,&device_info[8]);
		download_URL[strlen(download_URL)]='/';
		strcat(download_URL,item->valuestring);
		ESP_LOGE(my_tag,"url=%s",download_URL);
		http_test_task(download_URL);

		memset(download_URL,0,200);
		strcat(download_URL,current_data.server_add_tell_down_ok);
	}
	charconnectuchar(download_URL,&device_info[8]);
	download_URL[strlen(download_URL)]=QUES;
	strcat(download_URL,picname);
	strcat(download_URL,item->valuestring);
	download_URL[strlen(download_URL)]=AND;
	strcat(download_URL,timestamp);
	char temp_time_stamp[11];
	inttostring(current_data.time_stamp,temp_time_stamp);
	temp_time_stamp[10]='\0';
	strcat(download_URL,temp_time_stamp);
	ESP_LOGE(my_tag,"url=%s",download_URL);
	http_test_task(download_URL);
}


void inttostring(long value, char * output)
{
    int index = 0;
    char temp;
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
    for(unsigned char i=0;i<index/2;i++)
    {
    	temp=output[i];
    	output[i]=output[index-i-1];
    	output[index-i-1]=temp;
    }
}

void charconnectuchar(char a[],unsigned char b[])
{
	unsigned char i=0,j=0;
	while(1)
	{
		if(a[i]=='\0')
		{
			printf("i=%d\n",i);
			break;
		}
		i++;
	}
	char arr[18];
	char *p=arr;
	arr[17]='\0';
	for(;j<6;j++)
	{
		sprintf(p,"%x",b[j]);
		p++;
		p++;
		if(j<5)
		{
			*p=':';
			p++;
		}
	}
	strcat(a,arr);
	printf("j=%d\n",j);
	for(j=0;j<18;j++)
	{
		printf("arr[%d]=%c\n",j,arr[j]);
	}
}



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

//	cJSON * item_temp = NULL;
//	char temp_name[20];
//			printf("%s\n", "get downloadlist cJSON object:");
//			item_temp = cJSON_GetObjectItem(root, "downloadlist");
//			ESP_LOGW(my_tag,"%s", cJSON_Print(item_temp));
//
//			printf("%s\n", "get picname cJSON object:");
//			if( NULL != item_temp )
//			{
//				cJSON *client_list  = item_temp->child;
//				if( NULL == client_list )
//				{
//					ESP_LOGW(my_tag,"picname is NULL,no need to download picture(s)");
//					ESP_LOGW(my_tag,"go to sleep");
//					esp_deep_sleep_start();
//				}
//				else
//				{
//					while( client_list != NULL )
//					{
//						failed_times++;
//						download_composite(item);
//						if(failed_bit==1)
//						{
//							client_list = client_list->next;
//						}
//					}
//					ESP_LOGW(my_tag,"all pictures downloaded");
//					ESP_LOGW(my_tag,"go to sleep");
//					esp_deep_sleep_start();
//				}
//			}
//			else
//			{
//				ESP_LOGW(my_tag,"no downloadlist string");
//				ESP_LOGW(my_tag,"go to sleep");
//				esp_deep_sleep_start();
//			}


//				 "deletepic"
//				for (unsigned char i = 0; i < current_data.pic_number; i++)
//				{
//					spi_flash_read(info_pic_name + i * 20, temp_name, 20);
//					if (strcmp((char *)temp_name, item->valuestring) == 0)
//					{
//						ESP_LOGW(my_tag, "find same name picture,now delete it");
//						spi_flash_read(info_page*4096,&current_data,sizeof(current_data));
//						current_data.pic_number--;
//						spi_flash_write(info_page*4096,&current_data,sizeof(current_data));
//						temp_name[8]='*';
//						sf_WriteBuffer((uint8_t *)temp_name,info_pic_name + i * 20,20);
//						break;
//					}
//				}


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
