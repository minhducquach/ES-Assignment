#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "thingsboard.h"
#include "esp_http_client.h"

char* response;

esp_err_t client_event_post_handler(esp_http_client_event_handle_t evt)
{
	switch (evt->event_id) {
		case HTTP_EVENT_ON_DATA:
//			vTaskDelay(2000/portTICK_PERIOD_MS);
//			printf("HTTP_EVENT_ON_DATA: %.*s\n", evt->data_len, (char *)evt->data);
			response = malloc(strlen((char *)evt->data)*sizeof(char));
			sprintf(response, "%s", (char *)evt->data);
			break;
		default:
			break;
	}
	return ESP_OK;
}


void create_thingsboard_device(int id)
{
    esp_http_client_config_t config_post = {
        .url = "http://34.124.201.130:8080/api/device-with-credentials",
        .method = HTTP_METHOD_POST,
        .cert_pem = NULL,
        .event_handler = client_event_post_handler,
		.buffer_size = 2048,
		.buffer_size_tx = 2048,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config_post);

    char  *post_data = (char*) malloc(400);
    sprintf(post_data, "{\"device\":{\"name\":\"BIN_00%d\",\"label\":\"\",\"deviceProfileId\":{\"entityType\":\"DEVICE_PROFILE\",\"id\":\"fbd5d1e0-881c-11ee-a89d-070a45cea09a\"},\"additionalInfo\":{\"gateway\":false,\"overwriteActivityTime\":false,\"description\":\"\"},\"customerId\":null},\"credentials\":{\"credentialsType\":\"ACCESS_TOKEN\",\"credentialsId\":\"bin_00%d\",\"credentialsValue\":null}}", id, id);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "X-Authorization", "Bearer eyJhbGciOiJIUzUxMiJ9.eyJzdWIiOiJkdWNxdWFjaEBjaGlja2RldnMuY29tIiwidXNlcklkIjoiN2MyNTQzMzAtODgxZC0xMWVlLWE4OWQtMDcwYTQ1Y2VhMDlhIiwic2NvcGVzIjpbIlRFTkFOVF9BRE1JTiJdLCJzZXNzaW9uSWQiOiI2NmU5OGY0My00OWFkLTQzODMtODhiNy0xMzUxY2I0YTI1YTUiLCJpc3MiOiJ0aGluZ3Nib2FyZC5pbyIsImlhdCI6MTcwMDk4Mzk4OCwiZXhwIjoxNzAwOTkyOTg4LCJmaXJzdE5hbWUiOiJEdWMiLCJsYXN0TmFtZSI6IlF1YWNoIE1pbmgiLCJlbmFibGVkIjp0cnVlLCJpc1B1YmxpYyI6ZmFsc2UsInRlbmFudElkIjoiZmJkNGU3ODAtODgxYy0xMWVlLWE4OWQtMDcwYTQ1Y2VhMDlhIiwiY3VzdG9tZXJJZCI6IjEzODE0MDAwLTFkZDItMTFiMi04MDgwLTgwODA4MDgwODA4MCJ9.XI7Sny2ck69qIHT5OBIJ6bdIGGwzhsGavB2g1USzFOApN9UFWcb-MPEM-wLSZAip6NU0zO_kDQ1HVi3HOnkQWg");

    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}

void thingsboard_login()
{
    esp_http_client_config_t config_post = {
        .url = "http://34.124.201.130:8080/api/auth/login",
        .method = HTTP_METHOD_POST,
        .cert_pem = NULL,
        .event_handler = client_event_post_handler,
		.buffer_size_tx = 2048,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config_post);

    char  *post_data = "{\"username\":\"ducquach@chickdevs.com\",\"password\":\"chickdevs\"}";
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client, "Content-Type", "application/json");
//    esp_http_client_set_header(client, "X-Authorization", "Bearer eyJhbGciOiJIUzUxMiJ9.eyJzdWIiOiJkdWNxdWFjaEBjaGlja2RldnMuY29tIiwidXNlcklkIjoiN2MyNTQzMzAtODgxZC0xMWVlLWE4OWQtMDcwYTQ1Y2VhMDlhIiwic2NvcGVzIjpbIlRFTkFOVF9BRE1JTiJdLCJzZXNzaW9uSWQiOiIyOTdiYzUzMS0yNzEzLTRjMTUtOGNkYy05Y2MyOGEwODk3NzUiLCJpc3MiOiJ0aGluZ3Nib2FyZC5pbyIsImlhdCI6MTcwMDgxMjI0NywiZXhwIjoxNzAwODIxMjQ3LCJmaXJzdE5hbWUiOiJEdWMiLCJsYXN0TmFtZSI6IlF1YWNoIE1pbmgiLCJlbmFibGVkIjp0cnVlLCJpc1B1YmxpYyI6ZmFsc2UsInRlbmFudElkIjoiZmJkNGU3ODAtODgxYy0xMWVlLWE4OWQtMDcwYTQ1Y2VhMDlhIiwiY3VzdG9tZXJJZCI6IjEzODE0MDAwLTFkZDItMTFiMi04MDgwLTgwODA4MDgwODA4MCJ9.3afg-RTMAMFunqDIdLimwP8GLCToqj-TDvv2UWzMZmWTc6qYdyA0mYovei6yeAu8P97sy5kevwIWqaEWzxOeJw");

    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}
