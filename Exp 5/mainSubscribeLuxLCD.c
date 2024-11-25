#include "mcc_generated_files/mcc.h"
#include <avr/io.h>
#include "mcc_generated_files/examples/mqtt_example.h"
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

int main(void) { 

    // Initialize LCD
    SYSTEM_Initialize(); 
    app_mqttExampleInit();
    
    LCD_DDR_A |= RS | E;
    LCD_DDR_C |= D4 | D5;
    LCD_DDR_D |= D6 | D7;
    lcd_init();

     // Initialize MQTT client
    while (1) {
     
        app_mqttScheduler();    // Run MQTT client scheduler
    }
    return 0;
}
