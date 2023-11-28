#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "uart.h"

static const int RX_BUF_SIZE = RX_BUFFER;

void uart_config(void) {
	const uart_config_t uart_config = {
		.baud_rate = 9600,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.source_clk = UART_SCLK_DEFAULT,
	};
	uart_driver_install(UART1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
	uart_param_config(UART1, &uart_config);
	uart_set_pin(UART1, UART1_TXD_PIN, UART1_RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

	uart_driver_install(UART2, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
	uart_param_config(UART2, &uart_config);
	uart_set_pin(UART2, UART2_TXD_PIN, UART2_RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

int sendData_UART1(const char* logName, uint8_t data[]) {
	const int len = strlen((char*) data);
	const int txBytes = uart_write_bytes(UART1, data, len);
	ESP_LOGI(logName, "Send data to UART");
	return txBytes;
}

void receiveData_UART1(const char* logName, uint8_t* data, int *flag) {
	const int rxBytes = uart_read_bytes(UART1, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
	if (rxBytes > 0) {
		*flag = 1;
		data[rxBytes] = 0;
		ESP_LOGI(logName, "Read %d bytes: '%s'", rxBytes, data);
		ESP_LOG_BUFFER_HEXDUMP(logName, data, rxBytes, ESP_LOG_INFO);
	}
	else *flag = 0;
}

int sendData_UART2(const char* logName, uint8_t data[]) {
	const int len = strlen((char*) data);
	const int txBytes = uart_write_bytes(UART2, data, len);
	ESP_LOGI(logName, "Send data to UART");
	return txBytes;
}

void receiveData_UART2(const char* logName, uint8_t* data, int *flag) {
	const int rxBytes = uart_read_bytes(UART2, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
	if (rxBytes > 0) {
		*flag = 1;
		data[rxBytes] = 0;
		ESP_LOGI(logName, "Read %d bytes: '%s'", rxBytes, data);
		ESP_LOG_BUFFER_HEXDUMP(logName, data, rxBytes, ESP_LOG_INFO);
	}
	else *flag = 0;
}
