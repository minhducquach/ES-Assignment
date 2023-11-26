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
#include "esp_event.h"
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
#include "esp_log.h"
#include "data_package.h"
#include "uart.h"
#include "thingsboard.h"

#define EXAMPLE_ESP_WIFI_SSID      "AndroidAP"
#define EXAMPLE_ESP_WIFI_PASS      "minhduco19"
#define EXAMPLE_ESP_MAXIMUM_RETRY  100

static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *WIFI_TAG = "wifi station";
static const char *MQTT_TAG = "MQTT_TCP";
static const char *UART_TAG = "UART_TASK";

static int s_retry_num = 0;
static esp_mqtt_client_handle_t mqtt_client[2];
//char* response;

struct params {
	char data[8];
	char json[100];
};

int trackMQTT[2] = {0, 0};

TaskHandle_t taskHandler1 = NULL;
TaskHandle_t taskHandler2 = NULL;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(MQTT_TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
	struct params* data = (struct params*) handler_args;
	getData(UART_TAG, data->data, data->json);
	int type = getType(UART_TAG, data->data);
//	ESP_LOGD(MQTT_TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
//    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
    	int msg_id;
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_CONNECTED");
        if (type == HEARTBEAT){
        	msg_id = esp_mqtt_client_publish(client, "v1/devices/me/attributes", data->json, 0, 1, 0);
        }
        else {
        	msg_id = esp_mqtt_client_publish(client, "v1/devices/me/telemetry", data->json, 0, 1, 0);
        }
        ESP_LOGI(MQTT_TAG, "sent publish successful, msg_id=%d, data=%s", msg_id, data->json);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(MQTT_TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(MQTT_TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start(int binID)
{
	char* binName = (char*) malloc(8);
	sprintf(binName, "bin_00%d", binID);
	printf("%s\n", binName);
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://34.124.201.130",
		.broker.address.port = 1883,
		.credentials.username = binName
    };
#if CONFIG_BROKER_URL_FROM_STDIN
    char line[128];

    if (strcmp(mqtt_cfg.broker.address.uri, "FROM_STDIN") == 0) {
        int count = 0;
        printf("Please enter url of mqtt broker\n");
        while (count < 128) {
            int c = fgetc(stdin);
            if (c == '\n') {
                line[count] = '\0';
                break;
            } else if (c > 0 && c < 127) {
                line[count] = c;
                ++count;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        mqtt_cfg.broker.address.uri = line;
        printf("Broker url: %s\n", line);
    } else {
        ESP_LOGE(MQTT_TAG, "Configuration mismatch: wrong broker url");
        abort();
    }
#endif /* CONFIG_BROKER_URL_FROM_STDIN */
    mqtt_client[binID-1] = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(mqtt_client[binID-1]);
}

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

void handleUART1(struct params* taskParams){
	while (1){
		int flag = 0;
//		receiveData_UART1(UART_TAG, taskParams->data, &flag);
		flag = 1;
		if (flag == 1){
			int id = (int) taskParams->data[1];
			if (!trackMQTT[0]){
				create_thingsboard_device(id);
				trackMQTT[0] = 1;
				mqtt_app_start(id);
			}
			esp_mqtt_client_register_event(mqtt_client[0], ESP_EVENT_ANY_ID, mqtt_event_handler, (void*) taskParams);
			char* response = (char*) malloc(8);
			responsePackage(id, RESPONSE, response);
			sendData_UART1(UART_TAG, response);
		}
		vTaskDelay(pdMS_TO_TICKS(60000));
	}
}

void handleUART2(struct params* taskParams){
	while (1){
		int flag = 0;
//		receiveData_UART1(UART_TAG, taskParams->data, &flag);
		flag = 1;
		if (flag == 1){
			int id = (int) taskParams->data[1];
			if (!trackMQTT[1]){
				create_thingsboard_device(id);
				mqtt_app_start(id);
				trackMQTT[1] = 1;
			}
			esp_mqtt_client_register_event(mqtt_client[1], ESP_EVENT_ANY_ID, mqtt_event_handler, (void*) taskParams);
			char* response = (char*) malloc(8);
			responsePackage(id, RESPONSE, response);
			sendData_UART1(UART_TAG, response);
		}
		vTaskDelay(pdMS_TO_TICKS(60000));
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
    struct params* taskParams_1 = malloc(sizeof(struct params));
    dataPackage(1, 50, taskParams_1->data);
    struct params* taskParams_2 = malloc(sizeof(struct params));
    dataPackage(2, 60, taskParams_2->data);
	xTaskCreate(&handleUART1, "Receive UART Device 1", 4096, (void*) taskParams_1, 1, &taskHandler1);
	xTaskCreate(&handleUART2, "Receive UART Device 2", 4096, (void*) taskParams_2, 1, &taskHandler2);
}
