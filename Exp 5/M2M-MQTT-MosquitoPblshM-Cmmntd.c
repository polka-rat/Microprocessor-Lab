#include "../mcc.h" // only publishes, "subscribe" part commented create_MQTT_subscribe_packet() line 330
#include "../winc/m2m/m2m_wifi.h" 
#include "../winc/socket/socket.h"
#include "../winc/common/winc_defines.h"
#include "../winc/driver/winc_adapter.h"
#include "../mqtt/mqtt_core/mqtt_core.h"
#include "../winc/m2m/m2m_types.h"
#include "../config/mqtt_config.h"

#include "clock_config.h"
#include <util/delay.h>

#define CONN_SSID CFG_MAIN_WLAN_SSID
#define CONN_AUTH CFG_MAIN_WLAN_AUTH
#define CONN_PASSWORD CFG_MAIN_WLAN_PSK

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
//const char mqttPublishMsg[] = "mchp payload";
char mqttPublishMsg[] = "mchp payload";
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
uint8_t t[] = "subscribing/topic"; // related to subscribing by publish node 
uint8_t only_once = 0;

volatile uint8_t last_received_topic[PUBLISH_TOPIC_SIZE];
volatile uint8_t last_received_message[PAYLOAD_SIZE];

void create_MQTT_subscribe_packet(char *topic) {
    mqttSubscribePacket subscribe_packet;
    //fill the whole packet with 0:
    memset(&subscribe_packet, 0, sizeof (mqttSubscribePacket));
    subscribe_packet.subscribePayload->topic = topic;
    subscribe_packet.subscribePayload->topicLength = strlen(subscribe_packet.subscribePayload->topic);
    subscribe_packet.subscribePayload->requestedQoS = 0;

    subscribe_packet.packetIdentifierLSB = 1;
    subscribe_packet.packetIdentifierMSB = 0;

    MQTT_CreateSubscribePacket(&subscribe_packet);
} // need not be there in publish node EE2016IITM

void equalize_arrays(uint8_t *array1, uint8_t *array2) {
    while (*array2 != '\0') //While string does not equal Null character.
    {
        *array1 = *array2; //equalize message to be sent with desired message
        array2++; // increment pointer
        array1++;
    } //if array1 was longer than array, we need to delete the strings that remain
    while (*array1 != '\0') { //check if there are extra strings in array1   
        *array1 = '\0'; //delete extra string
        array1++; //go for the next one
    }
}

void MQTT_publish_handler_cb(uint8_t *topic, uint8_t *payload) {// called in scheduler
    // after receiption in subscribe node, further action: "call back" fn. EE2016IITM
    equalize_arrays(last_received_topic, topic); 
    equalize_arrays(last_received_message, payload);
    
    // printf("Received Message: %s\n", payload); // entered in subscribe node
    // PORTD.OUT = PORTD.OUT | 0xFB; turn second LED off (Green) Remove
}   // this is commented in publish node

static uint32_t appCheckMQTTPublishTimeout() { // not called anywhere? EE2016IITM
    appMQTTPublishTimeoutOccured = true; // Mark that timer has executed
    return ((CFG_MQTT_CONN_TIMEOUT - 1) * SECONDS); // interpublish interval
}

static void app_buildMQTTConnectPacket(void) {// called by app_mqttExampleInit()
    mqttConnectPacket appConnectPacket; // communication part 

    memset(&appConnectPacket, 0, sizeof (mqttConnectPacket));

    appConnectPacket.connectVariableHeader.connectFlagsByte.All = 0x02;
    // Packets need to be sent to the server every 10s.
    appConnectPacket.connectVariableHeader.keepAliveTimer = CFG_MQTT_CONN_TIMEOUT;
    appConnectPacket.clientID = "PublishBoardA";

    MQTT_CreateConnectPacket(&appConnectPacket);
}   // This function creates communication packets in both subscribe & publish nodes 

static void app_buildPublishPacket(void) { // called by app_mqttScheduler()
    mqttPublishPacket appPublishPacket;

    memset(&appPublishPacket, 0, sizeof (mqttPublishPacket));

    appPublishPacket.topic = mqttPublishTopic;
    appPublishPacket.payload = mqttPublishMsg;

    // Fixed header
    appPublishPacket.publishHeaderFlags.duplicate = 0;
    appPublishPacket.publishHeaderFlags.qos = CFG_QOS;
    appPublishPacket.publishHeaderFlags.retain = 0;

    // Variable header
    appPublishPacket.packetIdentifierLSB = 10;
    appPublishPacket.packetIdentifierMSB = 0;

    appPublishPacket.payloadLength = strlen(appPublishPacket.payload);

    MQTT_CreatePublishPacket(&appPublishPacket);
}   // this creates the publish packets in publish & subscribe nodes

static void changeState(appStates_e newState) {
    if (newState != appState) { // whatever state currently, switch to
        appState = newState; //  specified - newstate - state
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
} // important ingredient of communication 

static void icmpReplyHandler(uint32_t u32IPAddr, uint32_t u32RTT, uint8_t u8ErrorCode) {
    // Add your custom ICMP data handler
} 

static void dnsHandler(uint8_t *pu8DomainName, uint32_t u32ServerIP) {
    if (u32ServerIP) { // called by scheduler EE2016IITM
        if (appState == APP_STATE_WAIT_FOR_DNS) {
            changeState(APP_STATE_TLS_START); // switching among states EE2016IITM
            m2m_ping_req(u32ServerIP, 0, icmpReplyHandler);
        }
    }

    serverIPAddress = u32ServerIP;
}

static void socketHandler(SOCKET sock, uint8_t u8Msg, void *pvMsg) {
    switch (u8Msg) { // called by scheduler() EE2016IITM
        case SOCKET_MSG_CONNECT:
        {
            mqttContext* mqttConnnectionInfo = MQTT_GetClientConnectionInfo();
            tstrSocketConnectMsg *pstrConnect = (tstrSocketConnectMsg *) pvMsg;

            if (pstrConnect && pstrConnect->s8Error >= 0) {
                changeState(SUBSCRIBING); //  
             // immediately after connection establishment?? EE2016IITM
                recv(sock, recvBuffer, sizeof (recvBuffer), 0);
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
            // fllwng Loop Imprtnt EE2016IITM
            if (pstrRecv && pstrRecv->s16BufferSize > 0) { 
                MQTT_GetReceivedData(pstrRecv->pu8Buffer, pstrRecv->s16BufferSize);
            } else { 
                changeState(APP_STATE_ERROR);

                *mqttConnnectionInfo->tcpClientSocket = -1;
                close(sock);
            }
            break;
        }// I guess MSG_CONNECT: just check the connection, if OK, receive (after subscrptn)
         // MSG_RECV --> subscrptn, MSG_SEND --> publish EE2016IITM
        case SOCKET_MSG_SEND:
        {
            recv(sock, recvBuffer, sizeof (recvBuffer), 0);
            break; // recv has to be replaced by publish?? EE2016IITM
        }   // something not correct here EE2016IITM
    }
}

void app_mqttExampleInit(void) { // why is it, an example? EE2016IITM
    tstrWifiInitParam param;

    winc_adapter_init();

    param.pfAppWifiCb = wifiHandler; // wifiHandler is a fn?? EE2016IITM
    if (M2M_SUCCESS != m2m_wifi_init(&param)) {
        // Add your custom error handler
    }

    MQTT_ClientInitialise();
    app_buildMQTTConnectPacket();
    timeout_create(&appMQTTPublishTimer, ((CFG_MQTT_CONN_TIMEOUT - 1) * SECONDS));

    m2m_wifi_set_sleep_mode(M2M_PS_DEEP_AUTOMATIC, 1);
}

void app_mqttScheduler(uint8_t* message_pointer1) {
    MQTT_SetPublishReceptionCallback(MQTT_publish_handler_cb); // not defined?? EE2016IITM 
    mqttContext* mqttConnnectionInfo = MQTT_GetClientConnectionInfo(); // ND EE2016IITM 
    equalize_arrays(mqttPublishMsg, message_pointer1); 
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
            socketInit(); // socket Handler is-itn't a function?? EE2016IITM
            registerSocketCallback(socketHandler, dnsHandler); // here
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
            if (appMQTTPublishTimeoutOccured == true) //checks if timer was completed
            {
                only_once++; //counts how many times the timer was completed
                if (only_once == 3) { //if timer was completed 3 times, subscribe
                    // create_MQTT_subscribe_packet(t); // in publish, no subscription IITM 
                    MQTT_TransmissionHandler(mqttConnnectionInfo); // EE2016IITM
                    only_once = 4; //to avoid only_once overflow and reset to be 3 again
                }
            }
            changeState(PUBLISHING);
        }

        case PUBLISHING:
        {
            if (appMQTTPublishTimeoutOccured == true) {
                appMQTTPublishTimeoutOccured = false;
                app_buildPublishPacket(); // build the packet for the first message
            }

            MQTT_ReceptionHandler(mqttConnnectionInfo); // Not Defnd EE2016IITM
            MQTT_TransmissionHandler(mqttConnnectionInfo); // Not Defnd EE2016IITM
            changeState(PROCESS_LAST_PACKET);
            break;
        }

        case PROCESS_LAST_PACKET:
        {   // may be we can remove next 3 lines
            mqttConnnectionInfo = MQTT_GetClientConnectionInfo(); //actualize MQTT connection information
            MQTT_ReceptionHandler(mqttConnnectionInfo); //detects received packet and processes it
            last_received_header = MQTT_GetLastReceivedPacketHeader(); //save the value of the header
            // header of last rcvd packet in subscribe node EE2016IITM
            changeState(SUBSCRIBING); // receiving itself says it is in SUBSCRIBING state?
            break; // Why then changing to SUBSCRIBING? EE2016IITM
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




