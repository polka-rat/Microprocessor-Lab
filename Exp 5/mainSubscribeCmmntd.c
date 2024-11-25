#include "mcc_generated_files/mcc.h" // main file for subscribe Board B

int main(void) {
    SYSTEM_Initialize();
    app_mqttExampleInit();

    uint8_t message[] = "SubscribeToEE2016IITM";
    while (1) {// Topic is EE2016IITM? message is "Subscribe To EE2016IITM"
        app_mqttScheduler(message); // the above topic would not be monitored
    }
}
// Publish Board A (Tx) would publish in default topic & subscribe Board B (Rx), also would
// subscribe to the same topic. Simultaneously, the subscribe node B would also publish in 
// some other topic (EE2016IITM), which we dont monitor. The main code, says the Board B 
// publishes "Subscribe to EE2016IITM". Topic is "EE2016IITM".  
