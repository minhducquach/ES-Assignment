//#define HEARTBEAT 1
//#define DATA 2
//#define RESPONSE 3

//void printData(char *data);
//void heartbeatPackage(uint8_t ID, int status, int floor, int trash, int signal, char* data);
//void dataPackage(uint8_t ID, int capacity, char* data);
//void responsePackage(uint8_t ID, int type, char* data);
//void getData(const char* logName, char *data, char* json);
//int getType(const char* logName, char *data);

#define HEARTBEAT 1
#define DATA 2
#define RESPONSE 3

void responsePackage(uint8_t ID, int type, uint8_t data[]);
void getData(const char* logName, uint8_t data[], char* json);
int getType(const char* logName, uint8_t data[]);
void dataPackage(uint8_t ID, int capacity, uint8_t data[]);



