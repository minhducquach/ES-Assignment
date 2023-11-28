#include "../../envi/include/env.h"
#include "esp_http_client.h"
#include "mqtt_client.h"
#include "esp_event.h"
#include "esp_log.h"
//#include "uart.h"
#include "data_package.h"

struct params {
	uint8_t data[8];
	char json[100];
};

void thingsboard_login(void);
void create_thingsboard_device(int id);
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
void mqtt_app_start(int binID);
