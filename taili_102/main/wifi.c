#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "wifi.h"
#include "esp_err.h"
#include "esp_blufi_api.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_log.h"
#include "gatts_server.h"
#include "calendar.h"
#define wifi_tag	"wifi"
extern esp_ble_adv_data_t adv_data;
unsigned char isconnected = 0;
/* ϵͳ�¼�ѭ�������� */
// static esp_err_t example_net_event_handler(void *ctx, system_event_t *event)
// {
//     wifi_mode_t mode;
//     switch (event->event_id)
//     {
//     case SYSTEM_EVENT_STA_START:
//         esp_wifi_connect();
//         break;
//     case SYSTEM_EVENT_STA_GOT_IP:
//     {
//         printf("GOT_IP:WIFI connectted\r\n");
//         // esp_wifi_get_mode(&mode);
//         break;
//     }
//     case SYSTEM_EVENT_STA_CONNECTED:
//         printf("connected WIFI connectted\r\n");
//         break;
//     case SYSTEM_EVENT_STA_DISCONNECTED:
//         /* This is a workaround as ESP32 WiFi libs don't currently
//            auto-reassociate. */
//         printf("WIFI disconnectted\r\n");
//         break;
//     default:
//         break;
//     }
//     return ESP_OK;
// }
static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
	static int retry_num = 0; /* ��¼wifi�������� */
	/* ϵͳ�¼�ΪWiFi�¼� */
    if (event_base == WIFI_EVENT)
    {
        if (event_id == WIFI_EVENT_STA_START) /* �¼�idΪSTA��ʼ */
        {
        	esp_err_t  err_code;
        	err_code=esp_wifi_connect();
        	ESP_LOGW(wifi_tag,"wifi_connect_err_code=%d\n",err_code);
        	/********************************/
        }
        else if (event_id == WIFI_EVENT_STA_DISCONNECTED) /* �¼�idΪʧȥSTA���� */
        {
        	esp_err_t  err_code;
        	err_code=esp_wifi_connect();
        	ESP_LOGW(wifi_tag,"wifi_connect_code=%d",err_code);
        	/*****************************/
        	isconnected = 0;
        	retry_num++;
        	ESP_LOGW(wifi_tag,"retry to connect to the AP %d times. ", retry_num);
            if (retry_num > 10) /* WiFi������������10 ��ʱ*/
            {
                retry_num = 0;
                ESP_LOGW(wifi_tag,"WIFI CONNECTED FAIL ....RECONNCET.....");
            }
        }
    }
    /* ϵͳ�¼�Ϊip��ַ�¼������¼�idΪ�ɹ���ȡip��ַ */
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data; /* ��ȡIP��ַ��Ϣ*/
        ESP_LOGW(wifi_tag,"WIFI CONNECTED SUCCESS.....");
        ESP_LOGW(wifi_tag,"got IP:%d.%d.%d.%d", IP2STR(&event->ip_info.ip)); /* ��ӡip��ַ*/
        /* WiFi������������ */
        retry_num = 0;
        isconnected = 1;
        getdeviceinfo();
        esp_ble_gap_config_adv_data(&adv_data);
    }
}
void wifi_init_sta(void)
{
    tcpip_adapter_init(); //��ʼ��TCP/IPЭ��ջ
    /* ����Ĭ���¼�ѭ��,*/
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // ESP_ERROR_CHECK(esp_event_loop_init(example_net_event_handler, NULL));
    //upate van
    /*�������д�����������ʼ��WIFI���� */
    /**************************/
    /* ʹ��WIFI_INIT_CONFIG_DEFAULT() ����ȡһ��Ĭ�ϵ�wifi���ò����ṹ�����*/
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    /* ����cfg������ʼ��wifi��������Ҫ����Դ */
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    //����֮ǰ�洢��wifi ssid��pwd
    esp_wifi_restore();
    //update van
    // ����STA ģʽ������wifi�����Լ�����
    // ʹ��ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config))������������Ч
	/**************************/
//     wifi_config_t wifi_config = {
//         .sta = {
////             .ssid = current_data.wifi_ssid,
////             .password = current_data.wifi_pssd},
//			  .ssid = "moding_wifi",
//			             .password = "modingtech.com"},
//     };
//     ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    /* ���¼��������ע�ᵽϵͳĬ���¼�ѭ�����ֱ���WiFi�¼���IP��ַ�¼���smartconfig�¼� */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
    //update van
    //���õ�ǰ����ģʽ����ʹ��esp_wifi_get_mode()��ȡ��ǰ����ģʽ
    //esp���ɴ���APģʽ��Ҳ�ɴ���STAģʽ��Ҳ��STA+APģʽ
    /*****************************/
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    //update van
    /*����wifi���ܣ�ʹ���ǵ�����ȫ����Ч*/
	/****************************************/
    ESP_ERROR_CHECK(esp_wifi_start());
    //esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
//    esp_wifi_set_auto_connect();

    ESP_LOGW(wifi_tag,"wifi_init_sta finished. \n");
}
