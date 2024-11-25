#include "../mcc.h"
#include "../winc/m2m/m2m_wifi.h"
#include "../winc/socket/socket.h"
#include "../winc/common/winc_defines.h"
#include "../winc/driver/winc_adapter.h"
#include "../mqtt/mqtt_core/mqtt_core.h"
#include "../winc/m2m/m2m_types.h"
#include "../config/mqtt_config.h"
#include <stdlib.h> // Include for atof
#include <avr/io.h>
#include "clock_config.h"
#include <util/delay.h>
#define F_CPU 10000000UL  // Assuming a 16 MHz clock, adjust if necessary

#include <stdint.h>

#define CFG_MQTT_CONN_TIMEOUT  15 //ET THE SUBSCRIBING RATE
#define CONN_SSID CFG_MAIN_WLAN_SSID
#define CONN_AUTH CFG_MAIN_WLAN_AUTH
#define CONN_PASSWORD CFG_MAIN_WLAN_PSK
 // Define clock speed (example: 10 MHz)

// Define LCD port and pin connections
#define LCD_PORT_A PORTA
#define LCD_DDR_A  PORTA.DIR
#define LCD_PORT_C PORTC
#define LCD_DDR_C  PORTC.DIR
#define LCD_PORT_D PORTD
#define LCD_DDR_D  PORTD.DIR
#define RS PIN2_bm
#define E  PIN3_bm
#define D4 PIN0_bm
#define D5 PIN1_bm
#define D6 PIN6_bm
#define D7 PIN4_bm


typedef enum {
    APP_STATE_INIT,
    APP_STATE_STA_CONNECTING,
    APP_STATE_STA_CONNECTED,
    APP_STATE_WAIT_FOR_DNS,
    APP_STATE_TLS_START,
    APP_STATE_TLS_CONNECTING,
    SUBSCRIBING,
    PUBLISHING,
    PROCESS_LAST_PACKET,
    APP_STATE_ERROR,
    APP_STATE_STOPPED
} appStates_e;

static const char mqttPublishTopic[] = CFG_PUBTOPIC;
const char mqttPublishMsg[] = "OTHERTOPIC";
static const char mqttSubscribeTopicsList[NUM_TOPICS_SUBSCRIBE][SUBSCRIBE_TOPIC_SIZE] = {
    {CFG_SUBTOPIC}
};

static appStates_e appState = APP_STATE_INIT;
static uint32_t serverIPAddress = 0;
static uint8_t recvBuffer[256];
static bool timeReceived = false;
static bool appMQTTPublishTimeoutOccured = false;

static void app_buildMQTTConnectPacket(void);
static void app_buildPublishPacket(void);
static uint32_t appCheckMQTTPublishTimeout();
timerStruct_t appMQTTPublishTimer = {appCheckMQTTPublishTimeout, NULL};

mqttHeaderFlags last_received_header;
uint8_t t[] = "mchp/iot/events";
uint8_t only_once = 0;
     
volatile uint8_t last_received_topic[PUBLISH_TOPIC_SIZE];
volatile uint8_t last_received_message[PAYLOAD_SIZE];
volatile char lcd_payload[PAYLOAD_SIZE];

void create_MQTT_subscribe_packet(char *topic) {
    mqttSubscribePacket subscribe_packet;
    memset(&subscribe_packet, 0, sizeof(mqttSubscribePacket));
    subscribe_packet.subscribePayload->topic = topic;
    subscribe_packet.subscribePayload->topicLength = strlen(subscribe_packet.subscribePayload->topic);
    subscribe_packet.subscribePayload->requestedQoS = 0;

    subscribe_packet.packetIdentifierLSB = 1;
    subscribe_packet.packetIdentifierMSB = 0;

    MQTT_CreateSubscribePacket(&subscribe_packet);
}

void equalize_arrays(uint8_t *array1, uint8_t *array2) {
    while (*array2 != '\0') {
        *array1 = *array2;
        array2++;
        array1++;
    }
    while (*array1 != '\0') {
        *array1 = '\0';
        array1++;
    }
}

void lcd_send(unsigned char value, unsigned char mode);
void lcd_init(void);
void lcd_string(char *str);
void lcd_set_cursor(uint8_t col, uint8_t row);
void lcd_clear(void);

void lcd_send(unsigned char value, unsigned char mode) {
    // Set RS for command (0) or data (1)
    if (mode == 0) {
        LCD_PORT_A.OUTCLR = RS; // command  #change
    } else {
        LCD_PORT_A.OUTSET = RS; // data     //change
    }

    // Send higher nibble
    LCD_PORT_C.OUTCLR = D4 | D5;
    LCD_PORT_D.OUTCLR = D6 | D7;

    LCD_PORT_C.OUTSET = ((value & 0x10) >> 4) * D4 | ((value & 0x20) >> 5) * D5;
    LCD_PORT_D.OUTSET = ((value & 0x40) >> 6) * D6 | ((value & 0x80) >> 7) * D7;

    LCD_PORT_A.OUTSET = E; // This tells the LCD that data is ready to be read from the data pins.
    _delay_us(1);
    LCD_PORT_A.OUTCLR = E; // This tells the LCD to latch the data.
    _delay_us(200);

    // Send lower nibble
    LCD_PORT_C.OUTCLR = D4 | D5;
    LCD_PORT_D.OUTCLR = D6 | D7;

    LCD_PORT_C.OUTSET = ((value & 0x01) >> 0) * D4 | ((value & 0x02) >> 1) * D5;
    LCD_PORT_D.OUTSET = ((value & 0x04) >> 2) * D6 | ((value & 0x08) >> 3) * D7;

    LCD_PORT_A.OUTSET = E;
    _delay_us(1);
    LCD_PORT_A.OUTCLR = E;
    _delay_ms(2);
}

void lcd_init(void) {
    _delay_ms(20); // Wait for LCD to power up

    lcd_send(0x02, 0); // Initialize LCD in 4-bit mode      //changed
    lcd_send(0x28, 0); // 2 lines, 5x8 matrix
    lcd_send(0x0C, 0); // Display on, cursor off
    lcd_send(0x06, 0); // Increment cursor
    lcd_send(0x01, 0); // Clear display
    _delay_ms(2); // Wait for the LCD to process the clear command
}

void lcd_string(char *str) {
    while (*str) {
        lcd_send(*str++, 1);
    }
}

void lcd_clear(void) {
    lcd_send(0x01, 0); // Clear display command         //change
    _delay_ms(2);      // Wait for the LCD to process the clear command
}
void lcd_set_cursor(uint8_t col, uint8_t row) {
    if (row == 0) {
        lcd_send((col & 0x0F) | 0x80, 0);
    } else {
        lcd_send((col & 0x0F) | 0xC0, 0);       //change
    }
}

void MQTT_publish_handler_cb(uint8_t *topic, uint8_t *payload) { 
    printf("Received MQTT Message - Topic: %s, Payload: %s\n", topic, payload);
    equalize_arrays(last_received_topic, topic);
    equalize_arrays(last_received_message, payload);

    // Store the received message
    strcpy(lcd_payload, (char*)payload);      //meow
     // Clear the LCD and display the new payload value
    lcd_send(0x01, 0);
    lcd_set_cursor(3,0);
    lcd_string(lcd_payload);
      
      
    // Blink the External LED on PD4
    PORTD.OUTSET = PIN4_bm;
    _delay_ms(500);
    PORTD.OUTCLR = PIN4_bm;
    _delay_ms(500);

    //printf("Received Message: %s\n", payload); 
    PORTD.OUT = 0X1F; 
    _delay_ms(350);
    PORTD.OUT = 0; //turn second LED on (BLUE)
    _delay_ms(500);
    PORTD.OUT = 0X07; 
    _delay_ms(350);
    PORTD.OUT = 0; //turn second LED on (Green)
    _delay_ms(500);
       PORTD.OUT = 0X13; 
    _delay_ms(350);
    PORTD.OUT = 0; //turn second LED on (YELLOW)
    _delay_ms(500);
        PORTD.OUT = 0X01; 
    _delay_ms(350);
    PORTD.OUT = 0; //turn second LED on (RED)
    _delay_ms(500);
       PORTD.OUT = 0X00; 
    _delay_ms(350);
    PORTD.OUT = 0; //turn second LED on (Green)
    _delay_ms(500);
    
     
    
}

static uint32_t appCheckMQTTPublishTimeout() {
    appMQTTPublishTimeoutOccured = true;
    return ((CFG_MQTT_CONN_TIMEOUT - 1) * SECONDS);
}

static void app_buildMQTTConnectPacket(void) {
    mqttConnectPacket appConnectPacket;
    memset(&appConnectPacket, 0, sizeof(mqttConnectPacket));

    appConnectPacket.connectVariableHeader.connectFlagsByte.All = 0x02;
    appConnectPacket.connectVariableHeader.keepAliveTimer = CFG_MQTT_CONN_TIMEOUT;
    appConnectPacket.clientID = "sub_client";

    MQTT_CreateConnectPacket(&appConnectPacket);
}

static void app_buildPublishPacket(void) {
    mqttPublishPacket appPublishPacket;
    memset(&appPublishPacket, 0, sizeof(mqttPublishPacket));

    appPublishPacket.topic = mqttPublishTopic;
    appPublishPacket.payload = mqttPublishMsg;

    appPublishPacket.publishHeaderFlags.duplicate = 0;
    appPublishPacket.publishHeaderFlags.qos = CFG_QOS;
    appPublishPacket.publishHeaderFlags.retain = 0;

    appPublishPacket.packetIdentifierLSB = 10;
    appPublishPacket.packetIdentifierMSB = 0;

    appPublishPacket.payloadLength = strlen(appPublishPacket.payload);

    MQTT_CreatePublishPacket(&appPublishPacket);
}

static void changeState(appStates_e newState) {
    if (newState != appState) {
        appState = newState;
    }
}

static void wifiHandler(uint8_t u8MsgType, void *pvMsg) {
    tstrM2mWifiStateChanged *pstrWifiState = (tstrM2mWifiStateChanged *) pvMsg;

    switch (u8MsgType) {
        case M2M_WIFI_RESP_CON_STATE_CHANGED:
            if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED) {
                // Add your custom indication for successful AP connection
            }
            break;

        case M2M_WIFI_REQ_DHCP_CONF:
            if (appState == APP_STATE_STA_CONNECTING) {
                changeState(APP_STATE_STA_CONNECTED);
            }
            break;

        case M2M_WIFI_RESP_GET_SYS_TIME:
            timeReceived = true;
            break;

        default:
            break;
    }
}

static void icmpReplyHandler(uint32_t u32IPAddr, uint32_t u32RTT, uint8_t u8ErrorCode) {
    // Add your custom ICMP data handler
}

static void dnsHandler(uint8_t *pu8DomainName, uint32_t u32ServerIP) {
    if (u32ServerIP) {
        if (appState == APP_STATE_WAIT_FOR_DNS) {
            changeState(APP_STATE_TLS_START);
            m2m_ping_req(u32ServerIP, 0, icmpReplyHandler);
        }
    }

    serverIPAddress = u32ServerIP;
}

static void socketHandler(SOCKET sock, uint8_t u8Msg, void *pvMsg) {
    switch (u8Msg) {
        case SOCKET_MSG_CONNECT:
        {
            mqttContext* mqttConnnectionInfo = MQTT_GetClientConnectionInfo();
            tstrSocketConnectMsg *pstrConnect = (tstrSocketConnectMsg *) pvMsg;

            if (pstrConnect && pstrConnect->s8Error >= 0) {
                changeState(SUBSCRIBING);
                recv(sock, recvBuffer, sizeof(recvBuffer), 0);
            } else {
                changeState(APP_STATE_ERROR);
                *mqttConnnectionInfo->tcpClientSocket = -1;
                close(sock);
            }
            break;
        }

        case SOCKET_MSG_RECV:
        {
            mqttContext* mqttConnnectionInfo = MQTT_GetClientConnectionInfo();
            tstrSocketRecvMsg *pstrRecv = (tstrSocketRecvMsg *) pvMsg;

            if (pstrRecv && pstrRecv->s16BufferSize > 0) {
                MQTT_GetReceivedData(pstrRecv->pu8Buffer, pstrRecv->s16BufferSize);
            } else {
                changeState(APP_STATE_ERROR);
                *mqttConnnectionInfo->tcpClientSocket = -1;
                close(sock);
            }
            break;
        }

        case SOCKET_MSG_SEND:
        {
            recv(sock, recvBuffer, sizeof(recvBuffer), 0);
            break;
        }
    }
}

void app_mqttExampleInit(void) {
    tstrWifiInitParam param;
    winc_adapter_init();

    param.pfAppWifiCb = wifiHandler;
    if (M2M_SUCCESS != m2m_wifi_init(&param)) {
        // Add your custom error handler
    }

    MQTT_ClientInitialise();
    app_buildMQTTConnectPacket();
    timeout_create(&appMQTTPublishTimer, ((CFG_MQTT_CONN_TIMEOUT - 1) * SECONDS));

    m2m_wifi_set_sleep_mode(M2M_PS_DEEP_AUTOMATIC, 1);
}

void app_mqttScheduler(uint8_t* message_pointer1) {
    MQTT_SetPublishReceptionCallback(MQTT_publish_handler_cb);
    mqttContext* mqttConnnectionInfo = MQTT_GetClientConnectionInfo();
    timeout_callNextCallback();

    switch (appState) {
        case APP_STATE_INIT:
        {
            tstrNetworkId strNetworkId;
            tstrAuthPsk strAuthPsk;

            strNetworkId.pu8Bssid = NULL;
            strNetworkId.pu8Ssid = (uint8_t*) CONN_SSID;
            strNetworkId.u8SsidLen = strlen(CONN_SSID);
            strNetworkId.enuChannel = M2M_WIFI_CH_ALL;

            strAuthPsk.pu8Psk = NULL;
            strAuthPsk.pu8Passphrase = (uint8_t*) CONN_PASSWORD;
            strAuthPsk.u8PassphraseLen = strlen((const char*) CONN_PASSWORD);

            if (M2M_SUCCESS != m2m_wifi_connect((char *) CONN_SSID, sizeof (CONN_SSID), CONN_AUTH, (void *) CONN_PASSWORD, M2M_WIFI_CH_ALL)) {
                changeState(APP_STATE_ERROR);
                break;
            }

            changeState(APP_STATE_STA_CONNECTING);
            break;
        }

        case APP_STATE_STA_CONNECTING:
        {
            break;
        }

        case APP_STATE_STA_CONNECTED:
        {
            socketInit();
            registerSocketCallback(socketHandler, dnsHandler);
            changeState(APP_STATE_TLS_START);
            break;
        }

        case APP_STATE_WAIT_FOR_DNS:
        {
            break;
        }

        case APP_STATE_TLS_START:
        {
            struct sockaddr_in addr;

            if (*mqttConnnectionInfo->tcpClientSocket != -1) {
                close(*mqttConnnectionInfo->tcpClientSocket);
            }

            if (*mqttConnnectionInfo->tcpClientSocket = socket(AF_INET, SOCK_STREAM, 0)) {
                changeState(APP_STATE_ERROR);
                break;
            }
            addr.sin_family = AF_INET;
            addr.sin_port = _htons(CFG_MQTT_PORT);
            addr.sin_addr.s_addr = _htonl(CFG_MQTT_SERVERIPv4_HEX);

            if (connect(*mqttConnnectionInfo->tcpClientSocket, (struct sockaddr *) &addr, sizeof (struct sockaddr_in)) < 0) {
                close(*mqttConnnectionInfo->tcpClientSocket);
                *mqttConnnectionInfo->tcpClientSocket = -1;
                changeState(APP_STATE_ERROR);
                break;
            }
            changeState(APP_STATE_TLS_CONNECTING);
            break;
        }

        case APP_STATE_TLS_CONNECTING:
        {
            break;
        }

        case SUBSCRIBING:
        {
            if (appMQTTPublishTimeoutOccured == true) {
                only_once++;
                if (only_once == 3) {
                    create_MQTT_subscribe_packet(t);
                    MQTT_TransmissionHandler(mqttConnnectionInfo);
                    only_once = 4;
                }
            }

        }

        case PUBLISHING:
        {
            if (appMQTTPublishTimeoutOccured == true) {
                appMQTTPublishTimeoutOccured = false;
                app_buildPublishPacket();
            }

            MQTT_ReceptionHandler(mqttConnnectionInfo);
            MQTT_TransmissionHandler(mqttConnnectionInfo);
            changeState(PROCESS_LAST_PACKET);
            break;
        }

        case PROCESS_LAST_PACKET:
        {
            mqttConnnectionInfo = MQTT_GetClientConnectionInfo();
            MQTT_ReceptionHandler(mqttConnnectionInfo);
            last_received_header = MQTT_GetLastReceivedPacketHeader();
            changeState(SUBSCRIBING);
            break;
        }

        case APP_STATE_ERROR:
        {
            m2m_wifi_deinit(NULL);
            timeout_delete(&appMQTTPublishTimer);
            changeState(APP_STATE_STOPPED);
            break;
        }

        case APP_STATE_STOPPED:
        {
            changeState(APP_STATE_INIT);
            break;
        }
    }

    m2m_wifi_handle_events(NULL);
}
