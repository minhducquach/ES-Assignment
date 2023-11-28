#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "thingsboard.h"
#include "aes.h"

const char *MQTT_TAG = "MQTT_TCP";
const char *UART_TAG = "UART_TASK";

esp_mqtt_client_handle_t mqtt_client[2];

char* response;

void remove_newlines(char* str) {
    int i, j;
    for (i = j = 0; str[i] != '\0'; i++)
        if (str[i] != '\n')
            str[j++] = str[i];
    str[j] = '\0';
}

void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(MQTT_TAG, "Last error %s: 0x%x", message, error_code);
    }
}

esp_err_t client_event_post_handler(esp_http_client_event_handle_t evt)
{
	switch (evt->event_id) {
		case HTTP_EVENT_ON_DATA:
			response = malloc(strlen((char *)evt->data)*sizeof(char));
			sprintf(response, "%s", (char *)evt->data);
			break;
		default:
			break;
	}
	return ESP_OK;
}

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
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
        char* enc_msg = AES_encrypt((uint8_t*)data->json);
        char* thingsboard_msg = malloc(strlen(enc_msg) + 20);
        sprintf(thingsboard_msg, "{\"text\": \"%s\"}", enc_msg);
        if (type == HEARTBEAT){
        	msg_id = esp_mqtt_client_publish(client, "v1/devices/me/attributes", thingsboard_msg, 0, 1, 0);
        }
        else {
        	msg_id = esp_mqtt_client_publish(client, "v1/devices/me/telemetry", thingsboard_msg, 0, 1, 0);
        }
        ESP_LOGI(MQTT_TAG, "sent publish successful, msg_id=%d, data=%s", msg_id, thingsboard_msg);
        free(thingsboard_msg);
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

void mqtt_app_start(int binID)
{
	char* broker_add = malloc(22);
	sprintf(broker_add, "mqtt://%s", IP_ADDRESS);
	char* binName = (char*) malloc(8);
	sprintf(binName, "bin_00%d", binID);
	printf("%s\n", binName);
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = broker_add,
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

void create_thingsboard_device(int id)
{
	char* url = malloc(55);
	sprintf(url, "http://%s:8080/api/device-with-credentials", IP_ADDRESS);
    esp_http_client_config_t config_post = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .cert_pem = NULL,
        .event_handler = client_event_post_handler,
		.buffer_size_tx = 2048,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config_post);

    char  *post_data = (char*) malloc(400);
    sprintf(post_data, "{\"device\":{\"name\":\"BIN_00%d\",\"label\":\"\",\"deviceProfileId\":{\"entityType\":\"DEVICE_PROFILE\",\"id\":\"fbd5d1e0-881c-11ee-a89d-070a45cea09a\"},\"additionalInfo\":{\"gateway\":false,\"overwriteActivityTime\":false,\"description\":\"\"},\"customerId\":null},\"credentials\":{\"credentialsType\":\"ACCESS_TOKEN\",\"credentialsId\":\"bin_00%d\",\"credentialsValue\":null}}", id, id);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "X-Authorization", AUTH_TOKEN);
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}

void thingsboard_login()
{
	char* url = malloc(42);
	sprintf(url, "http://%s:8080/api/auth/login", IP_ADDRESS);
    esp_http_client_config_t config_post = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .cert_pem = NULL,
        .event_handler = client_event_post_handler,
		.buffer_size = 2048,
		.buffer_size_tx = 2048,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config_post);


    char  *post_data = malloc(80);
    sprintf(post_data, "{\"username\":\"%s\",\"password\":\"%s\"}", USERNAME, PASSWORD);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client, "Content-Type", "application/json");
//    esp_http_client_set_header(client, "X-Authorization", "Bearer eyJhbGciOiJIUzUxMiJ9.eyJzdWIiOiJkdWNxdWFjaEBjaGlja2RldnMuY29tIiwidXNlcklkIjoiN2MyNTQzMzAtODgxZC0xMWVlLWE4OWQtMDcwYTQ1Y2VhMDlhIiwic2NvcGVzIjpbIlRFTkFOVF9BRE1JTiJdLCJzZXNzaW9uSWQiOiIyOTdiYzUzMS0yNzEzLTRjMTUtOGNkYy05Y2MyOGEwODk3NzUiLCJpc3MiOiJ0aGluZ3Nib2FyZC5pbyIsImlhdCI6MTcwMDgxMjI0NywiZXhwIjoxNzAwODIxMjQ3LCJmaXJzdE5hbWUiOiJEdWMiLCJsYXN0TmFtZSI6IlF1YWNoIE1pbmgiLCJlbmFibGVkIjp0cnVlLCJpc1B1YmxpYyI6ZmFsc2UsInRlbmFudElkIjoiZmJkNGU3ODAtODgxYy0xMWVlLWE4OWQtMDcwYTQ1Y2VhMDlhIiwiY3VzdG9tZXJJZCI6IjEzODE0MDAwLTFkZDItMTFiMi04MDgwLTgwODA4MDgwODA4MCJ9.3afg-RTMAMFunqDIdLimwP8GLCToqj-TDvv2UWzMZmWTc6qYdyA0mYovei6yeAu8P97sy5kevwIWqaEWzxOeJw");

    esp_http_client_perform(client);
//    printf("RESPONSE: %.*s\n", strlen(response), response);
    remove_newlines(response);
//    printf("RESPONSE_TMP: %.*s\n", strlen(response), response);
//    printf("%.*s", 10, response + 628);
    esp_http_client_cleanup(client);
}
