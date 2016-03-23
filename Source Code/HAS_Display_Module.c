#include"libraries/stdconfig.h"
#include"libraries/delay.h"
#include"libraries/adc.h"
#include"libraries/spi.h"
#include<stdio.h>

//lcd pins; required for the library
#define lcd_data PORTD
#define rs RC2
#define en RC1

#include"libraries/lcd_16x2.h"

//required for the nRF24L01+ library
#define ss RB4         //slave select pin
#define ce RB3         //activate signal for nRF24L01+
#define new_data !RB2   //IRQ pin on nRF24L01+

#include"libraries/nrf24.h"

#define temp_thres 55

#define buzzer RC0

#define rx_addr '2'         //module address

void interrupt isr(void);
void interrupt_init(void);
void sys_init(void);
unsigned int temp_acquire(void);
void data_sort(void);
void disp_time(void);
void disp_weather(void);

unsigned int temp=0;   //temp sensor data
unsigned char weather[6]="CLEAR ";
unsigned char time[6]={18, 39, 1, 15, 2, 15};

void main()
{
    sys_init();            //initialize system
    while(1)
    {
        temp=temp_acquire();           //get temperature data
        while(temp>temp_thres)      //possible fire
        {
            buzzer=1;                  //sound the alarm
        }
        buzzer=0;
        //display stuff on LCD
        disp_time();            //display time
        Delay_ms(10000);
        disp_weather();         //display weather
        Delay_ms(10000);
    }
}
//system initialize function
void sys_init(void)
{
    TRISB&=0xE7;                            //ss & ce are output, RB2 is input
    ss=1;
    ce=1;
    TRISA=0xFF;                 //RA0 is analog input
    TRISC=0xF8;                  //lcd control pins
    buzzer=0;                   //fire alarm buzzer
    TRISD=0x00;                  //lcd data bus
    lcd_init();                  //initialize LCD
    adc_init();               //initialize ADC
    interrupt_init();         //initialize interrupts
    SPI_master_init(0x00, SLOW);          //initialize MSSP module for SPI communication
    nrf24_init();          //initialize nRF24L01+
}
//interrupt initialize function
void interrupt_init(void)
{
    IPEN=1;             //enable interrupt priority
    GIEH=1;             //enable high priority interrupts
    GIEL=1;             //enable low priority interrupts
    INTEDG2=0;        //INT2 on falling edge
    INT2IE=1;     //enable INT2
    INT2IF=0;         //clear interrupt flag
}
//high priority ISR function
void interrupt isr(void)
{
    nrf24_data_rx();       //get received data
    data_sort();          //process received data
}
//received data processing function
void data_sort(void)
{
    if(rx_data[0]==rx_addr)          //data received
    {
        switch(rx_data[1])
        {
            case 'T':          //time data received
                time[0]=rx_data[2];      //hours
                time[1]=rx_data[3];      //minutes
                time[2]=rx_data[4];      //day of the week
                time[3]=rx_data[5];      //day of the month
                time[4]=rx_data[6];      //month
                time[5]=rx_data[7];      //year
                break;

            case 'W':                 //weather data received
                weather[0]=rx_data[2];
                weather[1]=rx_data[3];
                weather[2]=rx_data[4];
                weather[3]=rx_data[5];
                weather[4]=rx_data[6];
                weather[5]=rx_data[7];
                break;

            default:                  //invalid data received
                asm("NOP");
        }
    }
}
//LM35 data processing function
unsigned int temp_acquire(void)
{
    float l;
    l=adc_acquire(CHANNEL_0);      //get data for processing
    l=(l*500.0)/1023.0;
    unsigned int temp_val=l;
    return temp_val;
}
//time display function
void disp_time(void)
{
    lcd_clear();
    //compensate for some glitches due to single digits
    if(time[0]<=9 && time[1]>9)
    {
        printf("     0%d:%d", time[0], time[1]);
    }
    else if(time[0]>9 && time[1]<=9)
    {
        printf("     %d:0%d", time[0], time[1]);
    }
    else if(time[0]<=9 && time[1]<=9)
    {
        printf("     0%d:0%d", time[0], time[1]);
    }
    else
    {
        printf("     %d:%d", time[0], time[1]);
    }
    lcd_new_msg_second_line();
    switch(time[2])
    {
        case 1:
            printf("Sun, ");
            break;

        case 2:
            printf("Mon, ");
            break;

        case 3:
            printf("Tue, ");
            break;

        case 4:
            printf("Wed, ");
            break;

        case 5:
            printf("Thu, ");
            break;

        case 6:
            printf("Fri, ");
            break;

        case 7:
            printf("Sat, ");
            break;

        default:
            asm("NOP");
    }
    switch(time[4])
    {
        case 1:
            printf("Jan ");
            break;

        case 2:
            printf("Feb ");
            break;

        case 3:
            printf("Mar ");
            break;

        case 4:
            printf("Apr ");
            break;

        case 5:
            printf("May ");
            break;

        case 6:
            printf("Jun ");
            break;

        case 7:
            printf("Jul ");
            break;

        case 8:
            printf("Aug ");
            break;

        case 9:
            printf("Sep ");
            break;

        case 10:
            printf("Oct ");
            break;

        case 11:
            printf("Nov ");
            break;

        case 12:
            printf("Dec ");
            break;

        default:
            asm("NOP");
    }
    printf("%d,", time[3]);
    printf("20%d", time[5]);
}
//weather display function
void disp_weather(void)
{
    temp=temp_acquire();       //get temp data
    lcd_clear();
    printf("   Temp: %d C", temp);

    //custom character for Celsius symbol
    lcd_cmd_transmit(0x40);            //go to CGRAM location 0
    //send custom character data; CGRAM location increments by 1 automatically with each write
    lcd_data_transmit(0x06);
    lcd_data_transmit(0x09);
    lcd_data_transmit(0x09);
    lcd_data_transmit(0x06);
    lcd_data_transmit(0x00);
    lcd_data_transmit(0x00);
    lcd_data_transmit(0x00);
    lcd_data_transmit(0x00);
    //the above code makes one custom character; we can make & store 7 more like this in the LCD at any given time
    //switch back to DDRAM
    lcd_cmd_transmit(0x8B);
    lcd_cmd_transmit(0x06);
    //display custom character at location 0x40; sending 0x01 displays character at location 0x41 & so on
    lcd_data_transmit(0x00);

    lcd_new_msg_second_line();
    printf("Weather: ");
    for(unsigned char i=0; i<6; i++)
    {
        lcd_data_transmit(weather[i]);
    }
}