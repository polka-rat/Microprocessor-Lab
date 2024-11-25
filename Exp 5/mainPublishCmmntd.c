#include "mcc_generated_files/mcc.h"

int main(void) {
    SYSTEM_Initialize();
    app_mqttExampleInit();

    uint8_t message[] = "Hello";
    while (1) {// what is the topic? message is "Hello"
        app_mqttScheduler(message);
    }
}
// corresponding to subscribe code, how is asked to subscribe
// by inputting the topic? For publish board comment the 
// line within subscribing loop, so that it would not subscribe
// topic is default - AVR-IoT/iot/events. Publish node (Tx) would 
// publish in the above topic & subscribe node (Rx), also would
// subscribe to the same topic. Point is the subscribe node would 
// publish in some other topic which we dont care. 
// Part (A): Display the received message "hello" in the LCD provided.
// Part (B): Display the received realtime light intensity data in
// same LCD  
