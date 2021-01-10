#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "pic.h"
#include "spiff.h"
#include "esp_spi_flash.h"
#define sector_size 4096
#define my_tag "desk_calender"

#define u8    uint8_t
#define u16   uint16_t
#define u32   uint32_t
#define	 LINE_VLAUE   120
#define  COLUMN_VLAUE 640
#define MONO 1
#define RED  2
void D_MS(int M)
{	vTaskDelay(M / portTICK_RATE_MS);
}
typedef enum{
	FUNTION_S=0,
	FUNTION_E=1,
}c_t;
void DEV_OP(u8 value, u8 num)
{
	int tnum=num;
	gpio_set_level(value,tnum);
}

u8 DEV_GP(u8 value)
{
	u8 i;
	return (i=gpio_get_level(value));
}
void epd_read(void)
{
	while(DEV_GP(0x0d));
}
void er_init(void)
{
    gpio_config_t io_conf;
    io_conf.intr_type = 0;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = ((1ULL<<16)|\
	                                 (1ULL<<15)|(1ULL<<26) \
						             |(1ULL<<27)|(1ULL<<12)\
						    |(1ULL<<14));
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
    io_conf.intr_type = 0;
    io_conf.pin_bit_mask = (1ULL<<13);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
	DEV_OP(0x0f,FUNTION_E);
	DEV_OP(0x0c,FUNTION_S);
    D_MS(10);
    DEV_OP(0x1b,FUNTION_E);
}

void DEV_CO(uint8_t value)
{
	DEV_OP(0x0f, FUNTION_E);
	DEV_OP(0x0f, FUNTION_S);
	DEV_OP(0x1b, FUNTION_S);
    ets_delay_us(5);
    DEV_OP(0x0c, FUNTION_S);
    for (int i = 0; i < 8; i++){
        if ((value & 0x80) == 0)
        	DEV_OP(0x0e, FUNTION_S);
        else
        	DEV_OP(0x0e, FUNTION_E);
        DEV_OP(0x0c, FUNTION_E);
            ets_delay_us(5);
            DEV_OP(0x0c, FUNTION_S);
            value <<= 1;
    }
    ets_delay_us(5);
    DEV_OP(0x0f, FUNTION_E);
}
void DEV_DTO(uint8_t value)
{
	DEV_OP(0x0f, FUNTION_E);
	DEV_OP(0x0f, FUNTION_S);
	DEV_OP(0x1b, FUNTION_E);
    ets_delay_us(5);
    DEV_OP(0x0c, FUNTION_S);
    for (int i = 0; i < 8; i++)
    {
        if ((value & 0x80) == 0)
        	DEV_OP(0x0e, FUNTION_S);
        else
        	DEV_OP(0x0e, FUNTION_E);
        DEV_OP(0x0c, FUNTION_E);
            ets_delay_us(2);
            DEV_OP(0x0c, FUNTION_S);
            value <<= 1;
    }
    ets_delay_us(5);
    DEV_OP(0x0f, FUNTION_E);
}
void epd_rd(void)
{
	D_MS(100);
	DEV_OP(0x1a, FUNTION_S);
	D_MS(10);
	DEV_OP(0x1a, FUNTION_E);
	D_MS(10);
	epd_read();
	DEV_CO(0x12);
	epd_read();
}
void epd_lut(u8 Tsensor_mode)
{
	DEV_CO(0x18);
	DEV_DTO(0x80);
	DEV_CO(0x22);
	DEV_DTO(0xB1);
	DEV_CO(0x20);
	epd_read();
}
void e_init(void)
{
	er_init();
	epd_rd();
	DEV_CO(0x0c);
	DEV_DTO(0xAE);
	DEV_DTO(0xC7);
	DEV_DTO(0xC3);
	DEV_DTO(0xC0);
	DEV_DTO(0x80);
	DEV_CO(0x01);
	DEV_DTO(0x7f);
	DEV_DTO(0x02);
	DEV_DTO(0x00);
	DEV_CO(0x11);
	DEV_DTO(0x01);
	DEV_CO(0x44);
	DEV_DTO(0x00);
	DEV_DTO(0x00);
	DEV_DTO(0xbf);
	DEV_DTO(0x03);
	DEV_CO(0x45);
	DEV_DTO(0x7f);
	DEV_DTO(0x02);
	DEV_DTO(0x00);
	DEV_DTO(0x00);
	DEV_CO(0x3c);
	DEV_DTO(0x01);
}
void reflesh_disp(void)
{
	DEV_CO(0x22);
	DEV_DTO(0xC7);
	DEV_CO(0x20);
	epd_read();
	D_MS(100);
}
void ereLocation(void)
{
	DEV_CO(0x4E);
	DEV_DTO(0x00);
	DEV_DTO(0x00);
	DEV_CO(0x4F);
	DEV_DTO(0x7f);
	DEV_DTO(0x02);
	epd_read();
}

uint32_t pic_index_temp=0;
void pic_Load_Data_temp(int display_index,int picture_page_index)
{
	int len=19,i=0;
	char buffer[sector_size];
	int temp_total;
	if(pic_index_temp==76799)
	{
		ESP_LOGW(my_tag,"pic_index=%d",pic_index_temp);
		i=19;
		len=38;
	}
	for(;i<len;i++)
	{
		if(i==18)
		{
			 spi_flash_write((picture_page_index + picture_cap * display_index) * 4096 + i * ((sector_size*3)/4), buffer, (sector_size*3)/4);
			 temp_total=(sector_size*3)/4;
		}
		else if(i<18)
		{
			spi_flash_write((picture_page_index + picture_cap * display_index) * 4096 + i * sector_size, buffer, sector_size);
			temp_total=sector_size;
		}
		else if(i==19||(i>19&&i<37))
		{
			 spi_flash_write((picture_page_index + picture_cap * display_index) * 4096 + (i-1) * (sector_size/4), buffer, sector_size/4);
			 spi_flash_write((picture_page_index + picture_cap * display_index) * 4096 + i * ((sector_size*3)/4), &buffer[sector_size/4], (sector_size*3)/4);
			 temp_total=sector_size;
		}
		else
		{
			spi_flash_write((picture_page_index + picture_cap * display_index) * 4096 + (i-1) * (sector_size/4), buffer, sector_size/4);
			spi_flash_write((picture_page_index + picture_cap * display_index) * 4096 + i * (sector_size/2), &buffer[sector_size/4], (sector_size/2));
			temp_total=(sector_size*3)/4;
		}
		for(int j=0;j<temp_total;j++)
		{
			DEV_DTO(*(buffer+pic_index_temp));
			pic_index_temp++;
		}
	}
	if(pic_index_temp==153599)
	{
		ESP_LOGW(my_tag,"pic_index=%d",pic_index_temp);
	}
}
void  pic_dis_temp(int display_index,int picture_page_index)
{
	pic_index_temp=0;
	epd_lut(0);
	ereLocation();
	DEV_CO(0x24);//write RAM for black(0)/white (1)
	pic_Load_Data_temp(display_index,picture_page_index);

	ereLocation();
	DEV_CO(0x26);//write RAM for red(1)/white (0)
	pic_Load_Data_temp(display_index,picture_page_index);
}
void display_picture_temp(int display_index,int picture_page_index)
{
	pic_dis_temp(display_index,picture_page_index);
	reflesh_disp();
}

int pic_index=0;
int times=0;
void pic_Load_Data(char *buffer)
{
	if(times==18)
	{
		epd_lut(0);
		ereLocation();
		DEV_CO(0x24);//write RAM for black(0)/white (1)
		for(int j=0;j<3072;j++)
		{
			DEV_DTO(*(buffer+pic_index));
			pic_index++;
		}
		ESP_LOGW(my_tag,"pic_index=%d",pic_index);
		ereLocation();
		DEV_CO(0x26);//write RAM for red(1)/white (0)
		for(int j=0;j<1024;j++)
		{
			DEV_DTO(*(buffer+pic_index));
			pic_index++;
		}
	}
	else
	{
		for(int j=0;j<sizeof(buffer);j++)
		{
			DEV_DTO(*(buffer+pic_index));
			pic_index++;
		}
	}
	times++;
}
void  pic_dis(char *buffer)
{
	if(times<18)
	{
		epd_lut(0);
		ereLocation();
		DEV_CO(0x24);//write RAM for black(0)/white (1)
		pic_Load_Data(buffer);
	}
	else if(times>19)
	{
		ereLocation();
		DEV_CO(0x26);//write RAM for red(1)/white (0)
		pic_Load_Data(buffer);
	}
	else
	{
		pic_Load_Data(buffer);
	}
}
void display_picture(char *buffer)
{
	pic_dis(buffer);
	reflesh_disp();
}
