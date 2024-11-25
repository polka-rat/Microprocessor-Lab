#include "mcc_generated_files/mcc.h"
#include <avr/io.h>
#include <math.h>
#include <stdio.h>
#include <util/delay.h>

void ADC_init(void) {
    ADC0.CTRLC = ADC_REFSEL_AVCC_gc; // Set the reference voltage (AVcc)
    ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_10BIT_gc; // Enable the ADC     //change
    ADC0.MUXPOS = ADC_MUXPOS_AIN5_gc; // Select the ADC channel (AIN5 on PD5)
}

uint16_t ADC_read(void) {
    ADC0.COMMAND = ADC_STCONV_bm; // Start conversion
    while (!(ADC0.INTFLAGS & ADC_RESRDY_bm)); // Wait for conversion to complete
    ADC0.INTFLAGS = ADC_RESRDY_bm; // Clear the interrupt flag
    return ADC0.RES; // Return the result       //change
}

float ADC_to_voltage(uint16_t adc_value, float vref) {
    return (adc_value * vref) / 1023.0; /* 10 bit ADC */    //change
}

float voltage_to_illuminance(float voltage) {
    /* Exponential relationship mapping for the light sensor
     Given on User guide: 10 �A@20lux and 50 �A@100lux, using a 10 k resistor
     I = V / R -> 10 �A = 20lux -> 10e-6 = V / //10k -> V = 0.1V at 20lux
     I = V / R -> 50 �A = 100lux -> 50e-6 = V / 10k -> V = 0.5V at 100lux
    
    Assuming the relation: lux = a * exp(b * voltage)
    Using points (0.1, 20) and (0.5, 100):
     20 = a * exp(b * 0.1)
     100 = a * exp(b * 0.5)*/

    float a = 20.0 / exp(0.1 * 2.485); // Derived a
    float b = 2.485; // Derived b from the two points

    return a * exp(b * voltage);
}



int main(void) {
    SYSTEM_Initialize();
    app_mqttExampleInit();
    ADC_init();

    char hello_message[] = "Hello World!";
    char lux_message[20];

    while (1) {
        uint16_t adc_value = ADC_read();
        float voltage = ADC_to_voltage(adc_value, 3.3);
        float lux = voltage_to_illuminance(voltage);

        // Format the lux message to send via MQTT
        sprintf(lux_message, "Lux: %.2f", lux);
        
        // Send the lux value message
        uint8_t lux_mqtt_message[20];
        for (int i = 0; i < sizeof(lux_message); i++) {
            lux_mqtt_message[i] = (uint8_t)lux_message[i];
        }
        app_mqttScheduler(lux_mqtt_message);

    }
}
