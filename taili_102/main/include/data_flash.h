#ifndef _DATA_FLASH_H_
#define	_DATA_FLASH_H_
struct my_data
{
	unsigned char check_head[2];
	char pic_number;
	char low_power;
	char pic_name[20];
	char wifi_ssid[40];
	char wifi_pssd[40];
	char server_add_get_down_pic[50];
	char server_add_tell_down_ok[50];
	char server_add_tell_dele_ok[50];
	char server_add_to_downlo_pic[50];
	char network_wrong;
	char config_wifi;
	unsigned char check_tail[2];
	long time_stamp;
	long picture_time_stamp;
	long remain_space;
};
struct my_data current_data;

#endif
