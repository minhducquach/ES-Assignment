/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_wifi.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "mqtt_client.h"
#include "esp_http_client.h"
#include "data_package.h"
#include "uart.h"
#include "thingsboard.h"
#include "aes.h"

#include "../components/envi/include/env.h"

#define EXAMPLE_ESP_WIFI_SSID      WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      WIFI_PASS
#define EXAMPLE_ESP_MAXIMUM_RETRY  100

static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *WIFI_TAG = "wifi station";
extern const char *UART_TAG;
extern const char *MQTT_TAG;

static int s_retry_num = 0;
extern esp_mqtt_client_handle_t mqtt_client;

int trackMQTT = 0;

TaskHandle_t taskHandler1 = NULL;
TaskHandle_t taskHandler2 = NULL;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(WIFI_TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(WIFI_TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(WIFI_TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(WIFI_TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(WIFI_TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
        return ESP_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(WIFI_TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
        return ESP_FAIL;
    } else {
        ESP_LOGE(WIFI_TAG, "UNEXPECTED EVENT");
        return ESP_FAIL;
    }
}

//void handleUART1(struct params* taskParams){
//	while (1){
//		int flag = 0;
//		receiveData_UART1(UART_TAG, taskParams->data, &flag);
////		flag = 1;
//		if (flag == 1){
//			uint8_t id = (uint8_t) taskParams->data[1];
//			if (!trackMQTT[0]){
//				create_thingsboard_device(id);
//				trackMQTT[0] = 1;
//				mqtt_app_start(id);
//			}
//			esp_mqtt_client_register_event(mqtt_client[0], ESP_EVENT_ANY_ID, mqtt_event_handler, (void*) taskParams);
//			uint8_t* response = (uint8_t*) malloc(8);
//			responsePackage(id, getType(UART_TAG, taskParams->data), response);
//			sendData_UART1(UART_TAG, response);
//		}
//	}
//}

void handleUART2(struct params* taskParams){
	// Config UART pins
	uart_config();
	// Data-received flag
	int flag = 0;
	while (1){
		// 1. Receive Data from UART
		receiveData_UART2(UART_TAG, taskParams->data, &flag);
		if (flag == 1){
			uint8_t id = (uint8_t) taskParams->data[1];
			if (!trackMQTT){
				create_thingsboard_device(id);
				mqtt_app_start(id);
				trackMQTT = 1;
			}
			int msg_id;
			getData(UART_TAG, taskParams->data, taskParams->json);
			ESP_LOGI(MQTT_TAG, "MQTT_EVENT_CONNECTED");
			char* enc_msg = AES_encrypt((uint8_t*)taskParams->json);
			printf("Json: %s\n", taskParams->json);
			char* thingsboard_msg = malloc(strlen(enc_msg) + 20);
			sprintf(thingsboard_msg, "{\"text\": \"%s\"}", enc_msg);
			msg_id = esp_mqtt_client_publish(mqtt_client, "v1/devices/me/telemetry", thingsboard_msg, 0, 1, 0);
			ESP_LOGI(MQTT_TAG, "sent publish successful, msg_id=%d, data=%s", msg_id, thingsboard_msg);
			free(thingsboard_msg);
		}
	}
}

void app_main(void)
{
    //Initialize NVS & connect to wifi
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(WIFI_TAG, "ESP_WIFI_MODE_STA");
    while (wifi_init_sta() != ESP_OK){
    	continue;
    }
//    struct params* taskParams_1 = malloc(sizeof(struct params));
//    dataPackage(1, 50, taskParams_1->data);
    struct params* taskParams_2 = malloc(sizeof(struct params));
//    dataPackage(2, 60, taskParams_2->data);
//	xTaskCreate(&handleUART1, "Receive UART Device 1", 4096, (void*) taskParams_1, 1, &taskHandler1);
	xTaskCreate(&handleUART2, "Receive UART Device 2", 4096, (void*) taskParams_2, 1, &taskHandler2);
}
