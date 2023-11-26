#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "esp_log.h"
#include "data_package.h"

void printData (char *data) {
	printf("0x");
	for (int i = 0; i < 8; i++)
		printf("%02X", (unsigned char)data[i]);
	printf("\n");
}

int checkSum(char *data) {
	uint16_t check_sum = (data[5] << 8) + data[6];
	uint16_t sum = data[1];
	for (int i = 2; i < 5; i++)
	        sum += data[i];
	if (sum == check_sum)
		return 1;
	return 0;
}

void heartbeatPackage(uint8_t ID, int status, int floor, int trash, int signal, char* data) {
	data[0] = 0x0A;
	data[7] = 0x0B;
	data[1] = ID;
	data[2] = 0xA0;
	if (status)
		data[2] += 0x01;
	data[3] = (floor << 4) + trash;
	data[4] = ((signal / 10) << 4) + (signal % 10);
	uint16_t check_sum = 0;
	for (int i = 1; i < 5; i++)
		check_sum += data[i];
	data[5] = check_sum >> 8;
	data[6] = check_sum;
}

void dataPackage(uint8_t ID, int capacity, char* data) {
	data[0] = 0x0A;
	data[7] = 0x0B;
	data[1] = ID;
	data[2] = 0xB0;
	data[3] = ((capacity / 10) << 4) + (capacity % 10);
	data[4] = 0x00;
	uint16_t check_sum = 0;
	for (int i = 1; i < 5; i++)
		check_sum += data[i];
	data[5] = check_sum >> 8;
	data[6] = check_sum;
}

void responsePackage(uint8_t ID, int type, char* data) {
	data[0] = 0x0A;
	data[7] = 0x0B;
	data[1] = ID;
	data[2] = 0xC4;
	data[3] = 0x4F;
	data[4] = 0xB0 + type;
	uint16_t check_sum = 0;
	for (int i = 1; i < 5; i++)
		check_sum += data[i];
	data[5] = check_sum >> 8;
	data[6] = check_sum;
}

int checkPackage(const char* logName, char* data) {
	if (data[0] != 0x0A || data[7] != 0x0B) {
		ESP_LOGW(logName, "Data is invalid!");
		return 0;
	}
	if (!checkSum(data)) {
		ESP_LOGW(logName, "Checksum fail! Data is incorrect!");
		return 0;
	}
	return 1;
}

int getType(const char* logName, char *data) {
	uint8_t command = data[2] >> 4;
	if (command == 0x0A)
		return HEARTBEAT;
	if (command == 0x0B)
		return DATA;
	if (command == 0x0C)
		return RESPONSE;
	ESP_LOGW(logName, "Data command is invalid!");
	return 0;
}

void getData(const char* logName, char *data, char* json) {
	if (checkPackage(logName, data) == 0) {
		return;
	}
	int package = getType(logName, data);
	if (package == HEARTBEAT) {
		int status = data[2] & 0x0f;
		int floor = (data[3] & 0xf0) >> 4;
		int trash = data[3] & 0x0f;
		int signal = ((data[4] & 0xf0) >> 4) * 10 + (data[4] & 0x0f);
		sprintf(json, "{status: %d, floor: %d, trash_id: %d, signal: %d}", status, floor, trash, signal);
	}
	else if (package == DATA) {
		int capacity = ((data[3] & 0xf0) >> 4) * 10 + (data[3] & 0x0f);
		sprintf(json, "{capacity: %d}", capacity);
	}
	else if (package == RESPONSE) {
		ESP_LOGI(logName, "Data package is Response package!");
	}
	else {
		ESP_LOGW(logName, "Can not identify data package!");
	}
}
