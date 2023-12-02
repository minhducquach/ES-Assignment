#define UART1_TXD_PIN (GPIO_NUM_10)
#define UART1_RXD_PIN (GPIO_NUM_9)
#define UART2_TXD_PIN (GPIO_NUM_17)
#define UART2_RXD_PIN (GPIO_NUM_16)
#define UART1 UART_NUM_1
#define UART2 UART_NUM_2

#define RX_BUFFER 1024

void uart_config(void);
int sendData_UART1(const char* logName, uint8_t data[]);
void receiveData_UART1(const char* logName, uint8_t* data, int* flag);
int sendData_UART2(const char* logName, uint8_t data[]);
void receiveData_UART2(const char* logName, uint8_t* data, int* flag);
