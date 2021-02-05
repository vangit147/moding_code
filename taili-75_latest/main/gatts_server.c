/*
   ����˷��͹㲥����������ֵ�Ķ���֪ͨ���Է���0x55,0x02,0x55
   SPI_flash��Ҫ�Ȳ�����д�� 4096 ҳ
   4000��¼ͼƬ������4001-4096�洢·��
*/
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "sdkconfig.h"
#include "freertos/queue.h"
#include "gatts_server.h"
#include "esp_wifi.h"
#include "esp_err.h"
#include "esp_spiffs.h"
#include "calendar.h"

static const char *gatts_tag = "gatts";
extern uint32_t voltage;
esp_vfs_spiffs_conf_t conf;
extern void ble_senddata(unsigned char *data);

int write_id = 0;
uint16_t data_conn_id = 0xffff;
esp_gatt_if_t data_gatts_if = 0xff;



char wifi_ssid_first[32];
char wifi_pssd_first[64];
unsigned char device_info[26] = {0x00, 0x34, 0x01, 0x01, 0x11, 0x22, 0x0A, 0x01, 0x00,0x00,0x00,0x00,0x00,0x00};


//profile �����¼�
static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

#define GATTS_SERVICE_UUID_TEST_A 0x00FF
#define GATTS_CHAR_UUID_TEST_A 0xFF01
#define GATTS_DESCR_UUID_TEST_A 0x3333
#define GATTS_NUM_HANDLE_TEST_A 4

//�����豸����
#define TEST_DEVICE_NAME "Mdaink"
#define TEST_MANUFACTURER_DATA_LEN 17
#define GATTS_DEMO_CHAR_VAL_LEN_MAX 0x40
#define PREPARE_BUF_MAX_SIZE 1024

extern unsigned char receive_buffer[512]; //���ݽ��ջ�����
// ������ʼֵ  ������ʼֵ�����Ƿǿն����������ȱ���ʼ�մ����㣬�����ջ�����ش���
static uint8_t char1_str[] = {0x11, 0x22, 0x33};
static esp_gatt_char_prop_t a_property = 0;

static esp_attr_value_t gatts_demo_char1_val =
    {
        .attr_max_len = GATTS_DEMO_CHAR_VAL_LEN_MAX,
        .attr_len = sizeof(char1_str),
        .attr_value = char1_str,
};

static uint8_t adv_config_done = 0;
#define adv_config_flag (1 << 0)
#define scan_rsp_config_flag (1 << 1)

#ifdef CONFIG_SET_RAW_ADV_DATA
static uint8_t raw_adv_data[] = {
    0x02, 0x01, 0x06,
    0x02, 0x0a, 0xeb, 0x03, 0x03, 0xab, 0xcd};
static uint8_t raw_scan_rsp_data[] = {
    0x0f, 0x09, 0x45, 0x53, 0x50, 0x5f, 0x47, 0x41, 0x54, 0x54, 0x53, 0x5f, 0x44,
    0x45, 0x4d, 0x4f};
#else

static uint8_t adv_service_uuid128[32] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb,
    0x34,
    0x9b,
    0x5f,
    0x80,
    0x00,
    0x00,
    0x80,
    0x00,
    0x10,
    0x00,
    0x00,
    0xEE,
    0x00,
    0x00,
    0x00,
    //second uuid, 32bit, [12], [13], [14], [15] is the value
    0xfb,
    0x34,
    0x9b,
    0x5f,
    0x80,
    0x00,
    0x00,
    0x80,
    0x00,
    0x10,
    0x00,
    0x00,
    0xFF,
    0x00,
    0x00,
    0x00,
};

//adv ��Ч���������԰���31�������ֽ�
//static uint8_t test_manufacturer[TEST_MANUFACTURER_DATA_LEN] =  {0x12, 0x23, 0x45, 0x56};
//adv data
esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,    //��adv_data����Ϊɨ����Ӧ
    .include_name = true,     //adv_data �Ƿ�����豸����
    .include_txpower = false, //�Ƿ�������͹���
    .min_interval = 0x0006,   //������Сʱ���� Time = min_interval * 1.25 msec
    .max_interval = 0x0010,   //��������¼����, Time = max_interval * 1.25 msec
    .appearance = 0x00,
	//    .manufacturer_len = 14,                                               //���������ݳ���
	//update van
	.manufacturer_len = 26,                                               //���������ݳ���
	/*********************************************/
    .p_manufacturer_data = device_info,                                   //���������ݵ�
    .service_data_len = 0,                                                //�������ݳ���
    .p_service_data = NULL,                                               //��������
    .service_uuid_len = sizeof(adv_service_uuid128),                      //����UUID����
    .p_service_uuid = adv_service_uuid128,                                //�����UUID
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT), //����ģʽ�ı��
};
// ɨ��ָ�����
static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp = true,
    .include_name = true,
    .include_txpower = true,
    //.min_interval = 0x0006,
    //.max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0,       //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

#endif /* CONFIG_SET_RAW_ADV_DATA */

esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    //.peer_addr            =
    //.peer_addr_type       =
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
struct gatts_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
        .gatts_cb = gatts_profile_a_event_handler,
        .gatts_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

typedef struct
{
    uint8_t *prepare_buf;
    int prepare_len;
} prepare_type_env_t;

static prepare_type_env_t a_prepare_write_env;
//static prepare_type_env_t b_prepare_write_env;

void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);
void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event)
    {
#ifdef CONFIG_SET_RAW_ADV_DATA
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
        adv_config_done &= (~adv_config_flag);
        if (adv_config_done == 0)
        {
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
        adv_config_done &= (~scan_rsp_config_flag);
        if (adv_config_done == 0)
        {
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
#else
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~adv_config_flag);
        if (adv_config_done == 0)
        {
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~scan_rsp_config_flag);
        if (adv_config_done == 0)
        {
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
#endif
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        //advertising start complete event to indicate advertising start successfully or failed
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(gatts_tag, "Advertising start failed\n");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(gatts_tag, "Advertising stop failed\n");
        }
        else
        {
            ESP_LOGI(gatts_tag, "Stop adv successfully\n");
        }
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
        ESP_LOGI(gatts_tag, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                 param->update_conn_params.status,
                 param->update_conn_params.min_int,
                 param->update_conn_params.max_int,
                 param->update_conn_params.conn_int,
                 param->update_conn_params.latency,
                 param->update_conn_params.timeout);
        break;
    default:
        break;
    }
}

void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    esp_gatt_status_t status = ESP_GATT_OK;
    if (param->write.need_rsp)
    {
        if (param->write.is_prep)
        {
            if (prepare_write_env->prepare_buf == NULL)
            {
                prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE * sizeof(uint8_t));
                prepare_write_env->prepare_len = 0;
                if (prepare_write_env->prepare_buf == NULL)
                {
                    ESP_LOGE(gatts_tag, "Gatt_server prep no mem\n");
                    status = ESP_GATT_NO_RESOURCES;
                }
            }
            else
            {
                if (param->write.offset > PREPARE_BUF_MAX_SIZE)
                {
                    status = ESP_GATT_INVALID_OFFSET;
                }
                else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE)
                {
                    status = ESP_GATT_INVALID_ATTR_LEN;
                }
            }
            esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
            gatt_rsp->attr_value.len = param->write.len;
            gatt_rsp->attr_value.handle = param->write.handle;
            gatt_rsp->attr_value.offset = param->write.offset;
            gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
            esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
            if (response_err != ESP_OK)
            {
                ESP_LOGE(gatts_tag, "Send response error\n");
            }
            free(gatt_rsp);
            if (status != ESP_GATT_OK)
            {
                return;
            }
            memcpy(prepare_write_env->prepare_buf + param->write.offset,
                   param->write.value,
                   param->write.len);
            prepare_write_env->prepare_len += param->write.len;
        }
        else
        {
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, NULL);
        }
    }
}

void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC)
    {
        esp_log_buffer_hex(gatts_tag, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
        printf("come here\n");
        if (prepare_write_env->prepare_buf[0] == 0x55 && prepare_write_env->prepare_buf[1] == 0xfa && prepare_write_env->prepare_buf[prepare_write_env->prepare_len - 1] == 0x55 && prepare_write_env->prepare_buf[prepare_write_env->prepare_len - 2] == 0xfa)
        {
            if (prepare_write_env->prepare_buf[3] == 0x01)
            {
				wifi_config_t wifi_config;
				bzero(&wifi_config, sizeof(wifi_config_t));
				int j = 4 + prepare_write_env->prepare_buf[4] + 1;
				memset(wifi_ssid_first,0,sizeof(wifi_ssid_first));
				memset(wifi_pssd_first,0,sizeof(wifi_pssd_first));
				memcpy(wifi_ssid_first, prepare_write_env->prepare_buf + 5, prepare_write_env->prepare_buf[4]);
				memcpy(wifi_pssd_first, prepare_write_env->prepare_buf + 5 + prepare_write_env->prepare_buf[4] + 1, prepare_write_env->prepare_buf[j]);
				ESP_LOGW(gatts_tag,"wifi_ssid_first_receive:%s", wifi_ssid_first);
				ESP_LOGW(gatts_tag,"wifi_pssd_first_receive:%s", wifi_pssd_first);

				if(strcmp((char *)wifi_config.sta.ssid,wifi_ssid_first)&&strcmp((char *)wifi_config.sta.password,wifi_pssd_first)&&isconnected==0)
				{
					strcpy((char *)wifi_config.sta.ssid,wifi_ssid_first);
					strcpy((char *)wifi_config.sta.password,wifi_pssd_first);
					strcpy(current_data.wifi_ssid,(char *)wifi_config.sta.ssid);
					strcpy(current_data.wifi_pssd,(char *)wifi_config.sta.password);
					updated_data_to_flash();
					ESP_LOGW(gatts_tag,"current_data.wifi_ssid:%s", current_data.wifi_ssid);
					ESP_LOGW(gatts_tag,"current_data.wifi_pssd:%s", current_data.wifi_pssd);
					ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
					esp_wifi_disconnect();
					esp_wifi_connect();
					vTaskDelay(5000 / portTICK_PERIOD_MS);
				}
				ble_senddata(prepare_write_env->prepare_buf);
				if(isconnected==1)
				{
					wifi_config_page=1;
					getdeviceinfo();
					esp_ble_gap_config_adv_data(&adv_data);
					check_wifi_httpdownload_pic('0');
					sleep_for_next_wakeup();
				}
				else
				{
					sleep_for_next_wakeup();
				}
            }
        }
    }
    else
    {
        ESP_LOGI(gatts_tag, "ESP_GATT_PREP_WRITE_CANCEL");
    }
    if (prepare_write_env->prepare_buf)
    {
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }
    prepare_write_env->prepare_len = 0;
}

static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(gatts_tag, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
        gl_profile_tab[PROFILE_A_APP_ID].service_id.is_primary = true;
        gl_profile_tab[PROFILE_A_APP_ID].service_id.id.inst_id = 0x00;
        gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_TEST_A;


        esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(TEST_DEVICE_NAME); //�����豸����
        if (set_dev_name_ret)
        {
            ESP_LOGE(gatts_tag, "set device name failed, error code = %x", set_dev_name_ret);
        }
#ifdef CONFIG_SET_RAW_ADV_DATA
        esp_err_t raw_adv_ret = esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
        if (raw_adv_ret)
        {
            ESP_LOGE(gatts_tag, "config raw adv data failed, error code = %x ", raw_adv_ret);
        }
        adv_config_done |= adv_config_flag;
        esp_err_t raw_scan_ret = esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data, sizeof(raw_scan_rsp_data));
        if (raw_scan_ret)
        {
            ESP_LOGE(gatts_tag, "config raw scan rsp data failed, error code = %x", raw_scan_ret);
        }
        adv_config_done |= scan_rsp_config_flag;
#else
        getdeviceinfo();
        //���ù㲥����
        esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
        if (ret)
        {
            ESP_LOGE(gatts_tag, "config adv data failed, error code = %x", ret);
        }
        adv_config_done |= adv_config_flag;
        //����ɨ����Ӧ����
        ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
        if (ret)
        {
            ESP_LOGE(gatts_tag, "config scan response data failed, error code = %x", ret);
        }
        adv_config_done |= scan_rsp_config_flag;

#endif
        esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_A_APP_ID].service_id, GATTS_NUM_HANDLE_TEST_A);
        break;
    case ESP_GATTS_READ_EVT:
    {
        ESP_LOGI(gatts_tag, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d\n", param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
		getdeviceinfo();
        rsp.attr_value.handle = param->read.handle;
        rsp.attr_value.len = 14;
//        rsp.attr_value.value[0] = 0x55;
//        rsp.attr_value.value[1] = 0x02;
//        rsp.attr_value.value[2] = 0x55;
        for(uint8_t t=0;t<14;t++)
        {
             rsp.attr_value.value[t] = device_info[t];
        }
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                    ESP_GATT_OK, &rsp);
        break;
    }
    case ESP_GATTS_WRITE_EVT:
    {
        ESP_LOGI(gatts_tag, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d\n", param->write.conn_id, param->write.trans_id, param->write.handle);
        write_id = param->write.handle;
        esp_log_buffer_hex("receive data", receive_buffer, strlen((char *)receive_buffer));
        printf("run  here \n");
        if (!param->write.is_prep)
        {
            ESP_LOGI(gatts_tag, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
            if (param->write.len == 2 && (param->write.value[0] == 0x01) && param->write.value[2] == 0x00)
            {
            	ESP_LOGW(gatts_tag,"notidy data......");
            }
            else
            {
                if (param->write.value[param->write.len - 1] != 0x55)
                {
                    strcat((char *)receive_buffer, (char *)param->write.value);
                }
                else
                {
                    uint16_t len=param->write.len;
                    for(int l=0;l<len;l++)
                    {
                    	receive_buffer[l]=*(param->write.value+l);
                    }

                    esp_log_buffer_hex("receive data", receive_buffer, len);
                    int i=len;
                    ESP_LOGI(gatts_tag, "receive_buffer len= %d", i);
                    /********************************************/

                    if (receive_buffer[0] == 0x55 && receive_buffer[1] == 0xfa && receive_buffer[i - 1] == 0x55 && receive_buffer[i - 2] == 0xfa)
                    {
                        if (receive_buffer[3] == 0x01)
                        {
                        	wifi_config_t wifi_config;
                        	bzero(&wifi_config, sizeof(wifi_config_t));
							int j = 4 + receive_buffer[4] + 1;
							memset(wifi_ssid_first,0,sizeof(wifi_ssid_first));
							memset(wifi_pssd_first,0,sizeof(wifi_pssd_first));
							memcpy(wifi_ssid_first, receive_buffer + 5, receive_buffer[4]);
							memcpy(wifi_pssd_first, receive_buffer + 5 + receive_buffer[4] + 1, receive_buffer[j]);
							ESP_LOGW(gatts_tag,"wifi_ssid_first_receive:%s", wifi_ssid_first);
							ESP_LOGW(gatts_tag,"wifi_pssd_first_receive:%s", wifi_pssd_first);

							if(strcmp((char *)wifi_config.sta.ssid,wifi_ssid_first)&&strcmp((char *)wifi_config.sta.password,wifi_pssd_first)&&isconnected==0)
							{
								strcpy((char *)wifi_config.sta.ssid,wifi_ssid_first);
								strcpy((char *)wifi_config.sta.password,wifi_pssd_first);
								strcpy(current_data.wifi_ssid,(char *)wifi_config.sta.ssid);
								strcpy(current_data.wifi_pssd,(char *)wifi_config.sta.password);
								updated_data_to_flash();
								ESP_LOGW(gatts_tag,"current_data.wifi_ssid:%s", current_data.wifi_ssid);
								ESP_LOGW(gatts_tag,"current_data.wifi_pssd:%s", current_data.wifi_pssd);
								ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
								esp_wifi_disconnect();
								esp_wifi_connect();
								vTaskDelay(5000 / portTICK_PERIOD_MS);
							}
							ble_senddata(receive_buffer);
							if(isconnected==1)
							{
								wifi_config_page=1;
								getdeviceinfo();
								esp_ble_gap_config_adv_data(&adv_data);
								check_wifi_httpdownload_pic('0');
								sleep_for_next_wakeup();
							}
							else
							{
								sleep_for_next_wakeup();
							}

                        }
                    }
                }
            }
            //���յ������ݷ���ȥ
            // esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_A_APP_ID].char_handle,
            //  param->write.len, param->write.value, false);
            if (gl_profile_tab[PROFILE_A_APP_ID].descr_handle == param->write.handle && param->write.len == 2)
            {
                uint16_t descr_value = param->write.value[1] << 8 | param->write.value[0];
                if (descr_value == 0x0001)
                {
                    if (a_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY)
                    {
                        ESP_LOGI(gatts_tag, "notify enable");
                        memset((char *)receive_buffer, 0, 512);
                        //֪ͨ��Ϣ���͵�����
                        // uint8_t notify_data[3] = {0x55, 0x02, 0x55};
                        getdeviceinfo();
                        // for (unsigned char i = 0; i < 14; i++)
                        // {
                        //     rsp.attr_value.value[i] = device_info[i];
                        // }
                        //for (int i = 0; i < sizeof(notify_data); ++i)
                        //{
                        //    notify_data[i] = i%0xff;
                        // }
                        //the size of notify_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_A_APP_ID].char_handle,
                                                    14, device_info, false);
                    }
                }
                else if (descr_value == 0x0002)
                {
                    if (a_property & ESP_GATT_CHAR_PROP_BIT_INDICATE)
                    {
                        ESP_LOGI(gatts_tag, "indicate enable");
                        uint8_t indicate_data[15];
                        for (int i = 0; i < sizeof(indicate_data); ++i)
                        {
                            indicate_data[i] = i % 0xff;
                        }
                        //the size of indicate_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_A_APP_ID].char_handle,
                                                    sizeof(indicate_data), indicate_data, true);
                    }
                }
                else if (descr_value == 0x0000)
                {
                    ESP_LOGI(gatts_tag, "notify/indicate disable ");
                }
                else
                {
                    ESP_LOGE(gatts_tag, "unknown descr value");
                    esp_log_buffer_hex(gatts_tag, param->write.value, param->write.len);
                }
            }
        }
        example_write_event_env(gatts_if, &a_prepare_write_env, param);
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
        ESP_LOGI(gatts_tag, "ESP_GATTS_EXEC_WRITE_EVT");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        example_exec_write_event_env(&a_prepare_write_env, param);
        break;
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(gatts_tag, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
        break;
    case ESP_GATTS_UNREG_EVT:
        break;
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(gatts_tag, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
        gl_profile_tab[PROFILE_A_APP_ID].service_handle = param->create.service_handle;
        gl_profile_tab[PROFILE_A_APP_ID].char_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_A_APP_ID].char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_TEST_A;

        esp_ble_gatts_start_service(gl_profile_tab[PROFILE_A_APP_ID].service_handle);
        a_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
        //����ֵ���ԣ��ɶ���д��֪ͨ
        esp_err_t add_char_ret = esp_ble_gatts_add_char(gl_profile_tab[PROFILE_A_APP_ID].service_handle, &gl_profile_tab[PROFILE_A_APP_ID].char_uuid,
                                                        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                                        a_property,
                                                        &gatts_demo_char1_val, NULL);
        if (add_char_ret)
        {
            ESP_LOGE(gatts_tag, "add char failed, error code =%x", add_char_ret);
        }
        break;
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
        break;
    case ESP_GATTS_ADD_CHAR_EVT:
    {
        uint16_t length = 0;
        const uint8_t *prf_char;

        ESP_LOGI(gatts_tag, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
                 param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
        gl_profile_tab[PROFILE_A_APP_ID].char_handle = param->add_char.attr_handle;
        gl_profile_tab[PROFILE_A_APP_ID].descr_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_A_APP_ID].descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
        esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(param->add_char.attr_handle, &length, &prf_char);
        if (get_attr_ret == ESP_FAIL)
        {
            ESP_LOGE(gatts_tag, "ILLEGAL HANDLE");
        }
        ESP_LOGI(gatts_tag, "the gatts demo char length = %x\n", length);
        for (int i = 0; i < length; i++)
        {
            ESP_LOGI(gatts_tag, "prf_char[%x] =%x\n", i, prf_char[i]);
        }
        esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_A_APP_ID].service_handle, &gl_profile_tab[PROFILE_A_APP_ID].descr_uuid,
                                                               ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, NULL, NULL);
        if (add_descr_ret)
        {
            ESP_LOGE(gatts_tag, "add char descr failed, error code =%x", add_descr_ret);
        }
        break;
    }
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        gl_profile_tab[PROFILE_A_APP_ID].descr_handle = param->add_char_descr.attr_handle;
        ESP_LOGI(gatts_tag, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                 param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
        break;
    case ESP_GATTS_DELETE_EVT:
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(gatts_tag, "SERVICE_START_EVT, status %d, service_handle %d\n",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_STOP_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT:
    {
        data_gatts_if = gatts_if;
        data_conn_id = param->connect.conn_id;
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
        conn_params.latency = 0;
        conn_params.max_int = 0x20; // max_int = 0x20*1.25ms = 40ms
        conn_params.min_int = 0x10; // min_int = 0x10*1.25ms = 20ms
        conn_params.timeout = 400;  // timeout = 400*10ms = 4000ms
        ESP_LOGI(gatts_tag, "ESP_GATTS_CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:",
                 param->connect.conn_id,
                 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
        gl_profile_tab[PROFILE_A_APP_ID].conn_id = param->connect.conn_id;
        //start sent the update connection parameters to the peer device.
        esp_ble_gap_update_conn_params(&conn_params);
        //esp_ble_gap_stop_advertising();
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(gatts_tag, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x", param->disconnect.reason);
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(gatts_tag, "ESP_GATTS_CONF_EVT, status %d attr_handle %d", param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK)
        {
            esp_log_buffer_hex(gatts_tag, param->conf.value, param->conf.len);
        }
        break;
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    default:
        break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT)
    {
        if (param->reg.status == ESP_GATT_OK)
        {
            gl_profile_tab[param->reg.app_id].gatts_if = gatts_if;
        }
        else
        {
            ESP_LOGI(gatts_tag, "Reg app failed, app_id %04x, status %d\n",
                     param->reg.app_id,
                     param->reg.status);
            return;
        }
    }
    /* If the gatts_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do
    {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++)
        {
            if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                gatts_if == gl_profile_tab[idx].gatts_if)
            {
                if (gl_profile_tab[idx].gatts_cb)
                {
                    gl_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

void GattServers_Init(void)
{
    esp_err_t ret;
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    //����һ��esp_bt_controller_config_t��BT_CONTROLLER_INIT_CONFIG_DEFAULT()
    //�����ɵ�Ĭ������������BT���������ýṹ����ʼ��BT������
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    //��ʼ������������
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret)
    {
        ESP_LOGE(gatts_tag, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    /*ʹ����������������������BLEģʽ
    ֧����������ģʽ��
    ESP_BT_MODE_IDLE����������ģʽ
    ESP_BT_MODE_BLE��BLEģʽ
    ESP_BT_MODE_CLASSIC_BT��BT����ģʽ
    ESP_BT_MODE_BTDM��˫ģʽ��BLE + BT Classic��
   */
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret)
    {
        ESP_LOGE(gatts_tag, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    //��ʼ��������ջ
    ret = esp_bluedroid_init();
    if (ret)
    {
        ESP_LOGE(gatts_tag, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    //���ö�ջ
    ret = esp_bluedroid_enable();
    if (ret)
    {
        ESP_LOGE(gatts_tag, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    //ע��GATTS�¼��������
    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret)
    {
        ESP_LOGE(gatts_tag, "gatts register error, error code = %x", ret);
        return;
    }
    //ע��GAT�¼��������
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret)
    {
        ESP_LOGE(gatts_tag, "gap register error, error code = %x", ret);
        return;
    }
    //ʹ��Ӧ�ó���IDע��Ӧ�ó��������ļ�Ӧ�ó���ID���û�����ı�ţ����ڱ�ʶÿ�������ļ���
    //���������Ӧ�ó��������ļ�������һ̨�����������С�
    ret = esp_ble_gatts_app_register(PROFILE_A_APP_ID);
    if (ret)
    {
        ESP_LOGE(gatts_tag, "gatts app register error, error code = %x", ret);
        return;
    }
    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret)
    {
        ESP_LOGE(gatts_tag, "set local  MTU failed, error code = %x", local_mtu_ret);
    }
}

void getdeviceinfo(void)
{
	analysis_data();
	device_info[0] = (unsigned char)((voltage & 0xff00) >> 8);
	device_info[1] = (unsigned char)(voltage & 0xff);
	device_info[2] = isconnected;
	device_info[3] = current_data.pic_number;
	device_info[4] = (unsigned char)((current_data.remain_space & 0xff000000) >> 8 >> 8 >> 8);
    device_info[5] = (unsigned char)((current_data.remain_space & 0xff0000) >> 8 >> 8);
    device_info[6] = (unsigned char)((current_data.remain_space & 0xff00) >> 8);
    device_info[7] = (unsigned char)(current_data.remain_space & 0xff);
    device_info[8] = wifi_config_page;
//    for(unsigned char i=0;i<15;i++)
//    {
//    	ESP_LOGW(gatts_tag,"device[%d]=%x	",i,device_info[i]);
//    }
    printf("\n");
}


