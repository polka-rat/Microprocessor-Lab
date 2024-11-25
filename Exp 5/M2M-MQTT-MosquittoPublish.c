#include "../mcc.h"
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
#define CFG_MQTT_CONN_TIMEOUT 15

typedef enum
{
    APP_STATE_INIT,
    APP_STATE_STA_CONNECTING,
    APP_STATE_STA_CONNECTED,
    APP_STATE_WAIT_FOR_DNS,
    APP_STATE_TLS_START,
    APP_STATE_TLS_CONNECTING,
    APP_STATE_TLS_CONNECTED,
    APP_STATE_ERROR,
    APP_STATE_STOPPED
} appStates_e;


static const char mqttPublishTopic[] = CFG_PUBTOPIC;
//const char mqttPublishMsg[] = "mchp payload";
char mqttPublishMsg[] = "mchp payload";
static const char mqttSubscribeTopicsList[NUM_TOPICS_SUBSCRIBE][SUBSCRIBE_TOPIC_SIZE] = {{CFG_SUBTOPIC}};
static appStates_e appState = APP_STATE_INIT;
static uint32_t serverIPAddress = 0;
static uint8_t recvBuffer[256];
static bool timeReceived = false;
static bool appMQTTPublishTimeoutOccured = false;


static void app_buildMQTTConnectPacket(void);
static void app_buildPublishPacket(void);
static uint32_t appCheckMQTTPublishTimeout();
timerStruct_t appMQTTPublishTimer = {appCheckMQTTPublishTimeout, NULL};

void equalize_arrays(uint8_t *array1 ,uint8_t *array2){
    while (*array2 != '\0')            //While string does not equal Null character.
    {
    *array1 = *array2;    //equalize message to be sent with desired message
    array2++;                         // increment pointer
    array1++;
    }                       //if array1 was longer than array, we need to delete the strings that remain
    while (*array1 != '\0'){ //check if there are extra strings in array1   
        *array1='\0';        //delete extra string
        array1++;            //go for the next one
    }
}


static uint32_t appCheckMQTTPublishTimeout()
{
    appMQTTPublishTimeoutOccured = true; // Mark that timer has executed
    return ((CFG_MQTT_CONN_TIMEOUT - 1) * SECONDS);
}

static void app_buildMQTTConnectPacket(void)
{
    mqttConnectPacket appConnectPacket;

    memset(&appConnectPacket, 0, sizeof(mqttConnectPacket));

    appConnectPacket.connectVariableHeader.connectFlagsByte.All = 0x02;
    // Packets need to be sent to the server every 10s.
    appConnectPacket.connectVariableHeader.keepAliveTimer = CFG_MQTT_CONN_TIMEOUT;
    appConnectPacket.clientID = "pub_client";

    MQTT_CreateConnectPacket(&appConnectPacket);
}


static void app_buildPublishPacket(void)
{
    mqttPublishPacket appPublishPacket;
    
    memset(&appPublishPacket, 0, sizeof(mqttPublishPacket));   
      
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
}

static void changeState(appStates_e newState)
{
    if (newState != appState)
    {
        appState = newState;
    }
}

static void wifiHandler(uint8_t u8MsgType, void *pvMsg)
{
    tstrM2mWifiStateChanged *pstrWifiState = (tstrM2mWifiStateChanged *)pvMsg;
    
    switch (u8MsgType)
    {
        case M2M_WIFI_RESP_CON_STATE_CHANGED:
            if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED) 
            {
                // Add your custom indication for successful AP connection
            }
            break;
            
        case M2M_WIFI_REQ_DHCP_CONF:
            if (appState == APP_STATE_STA_CONNECTING)
            {
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

static void icmpReplyHandler(uint32_t u32IPAddr, uint32_t u32RTT, uint8_t u8ErrorCode)
{
    // Add your custom ICMP data handler
}

static void dnsHandler(uint8_t *pu8DomainName, uint32_t u32ServerIP)
{
    if (u32ServerIP)
    {
        if (appState == APP_STATE_WAIT_FOR_DNS)
        {
            changeState(APP_STATE_TLS_START);
            m2m_ping_req(u32ServerIP, 0, icmpReplyHandler);
        }
    }

    serverIPAddress = u32ServerIP;
}


static void socketHandler(SOCKET sock, uint8_t u8Msg, void *pvMsg)
{
    switch (u8Msg)
    {
        case SOCKET_MSG_CONNECT:
        {
            mqttContext* mqttConnnectionInfo = MQTT_GetClientConnectionInfo();
            tstrSocketConnectMsg *pstrConnect = (tstrSocketConnectMsg *)pvMsg;

            if (pstrConnect && pstrConnect->s8Error >= 0)
            {
                changeState(APP_STATE_TLS_CONNECTED);

                recv(sock, recvBuffer, sizeof(recvBuffer), 0);
            }
            else
            {
                changeState(APP_STATE_ERROR);

                *mqttConnnectionInfo->tcpClientSocket = -1;
                close(sock);
            }

            break;
        }

        case SOCKET_MSG_RECV:
        {
            mqttContext* mqttConnnectionInfo = MQTT_GetClientConnectionInfo();
            tstrSocketRecvMsg *pstrRecv = (tstrSocketRecvMsg *)pvMsg;

            if (pstrRecv && pstrRecv->s16BufferSize > 0)
            {                
                MQTT_GetReceivedData(pstrRecv->pu8Buffer, pstrRecv->s16BufferSize);
            }
            else
            {
                changeState(APP_STATE_ERROR);
                
                *mqttConnnectionInfo->tcpClientSocket = -1;
                close(sock);
            }
            break;
        }

        case SOCKET_MSG_SEND:
        {
            break;
        }
    }
}


void app_mqttExampleInit(void)
{
    tstrWifiInitParam param;
    
    winc_adapter_init();
    
    param.pfAppWifiCb = wifiHandler;
    if (M2M_SUCCESS != m2m_wifi_init(&param))
    {
        // Add your custom error handler
    }
    
    MQTT_ClientInitialise();
    app_buildMQTTConnectPacket();   
    timeout_create(&appMQTTPublishTimer, ((CFG_MQTT_CONN_TIMEOUT - 1) * SECONDS));
    
    m2m_wifi_set_sleep_mode(M2M_PS_DEEP_AUTOMATIC, 1);
}

//void app_mqttScheduler(void)
void app_mqttScheduler(uint8_t* message_pointer1)
{
    mqttContext* mqttConnnectionInfo = MQTT_GetClientConnectionInfo();
    equalize_arrays(mqttPublishMsg,message_pointer1);    
    timeout_callNextCallback();
    
    switch(appState)
    {
        case APP_STATE_INIT:
        {
            tstrNetworkId   strNetworkId;
            tstrAuthPsk     strAuthPsk;

            strNetworkId.pu8Bssid   = NULL;
            strNetworkId.pu8Ssid    = (uint8_t*)CONN_SSID;
            strNetworkId.u8SsidLen  = strlen(CONN_SSID);
            strNetworkId.enuChannel = M2M_WIFI_CH_ALL;

            strAuthPsk.pu8Psk          = NULL;
            strAuthPsk.pu8Passphrase   = (uint8_t*)CONN_PASSWORD;
            strAuthPsk.u8PassphraseLen = strlen((const char*)CONN_PASSWORD);


            if(M2M_SUCCESS != m2m_wifi_connect((char *)CONN_SSID, sizeof(CONN_SSID), CONN_AUTH, (void *)CONN_PASSWORD, M2M_WIFI_CH_ALL))
            {
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

            if (*mqttConnnectionInfo->tcpClientSocket != -1)
            {
                close(*mqttConnnectionInfo->tcpClientSocket);
            }

            if(*mqttConnnectionInfo->tcpClientSocket = socket(AF_INET, SOCK_STREAM, 0))
            {
                changeState(APP_STATE_ERROR);
                break;
            }
            addr.sin_family      = AF_INET;
            addr.sin_port        = _htons(CFG_MQTT_PORT);
            addr.sin_addr.s_addr = _htonl(CFG_MQTT_SERVERIPv4_HEX);

            if (connect(*mqttConnnectionInfo->tcpClientSocket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
            {
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

        case APP_STATE_TLS_CONNECTED:
        {
            if(appMQTTPublishTimeoutOccured == true)
            {
                appMQTTPublishTimeoutOccured = false;
                app_buildPublishPacket();    
            }

            MQTT_ReceptionHandler(mqttConnnectionInfo);
            MQTT_TransmissionHandler(mqttConnnectionInfo);
        }
        break;

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

