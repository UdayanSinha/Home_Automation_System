#include"libraries/stdconfig.h"
#include"libraries/delay.h"
#include"libraries/spi.h"

//required for the nRF24L01+ library
#define ss RB4         //slave select pin
#define ce RB3         //activate signal for nRF24L01+
#define new_data !RB2   //IRQ pin on nRF24L01+

#include"libraries/nrf24.h"

//primary relay board pins
#define RELAY_01 RD0
#define RELAY_02 RD1
#define RELAY_03 RD2
#define RELAY_04 RD3
#define RELAY_05 RD4
#define RELAY_06 RD5
#define RELAY_07 RD6
#define RELAY_08 RD7

//secondary relay board pins
#define RELAY_11 RA0
#define RELAY_12 RA1
#define RELAY_13 RA2
#define RELAY_14 RA3
#define RELAY_15 RA4
#define RELAY_16 RA5
#define RELAY_17 RE0
#define RELAY_18 RE1

#define PRI_BRD_EN !RC0              //primary board enable
#define SEC_BRD_EN !RC1              //secondary board enable

#define rx_addr '1'         //module address

void interrupt_init(void);
void interrupt isr(void);
void relay_control(void);
void sys_init(void);

void main()
{
    sys_init();            //initialize system
    while(1);
}
//system initialize function
void sys_init(void)
{
    TRISB&=0xE7;                            //ss & ce are output, RB2 is input
    ss=1;
    ce=1;
    interrupt_init();         //initialize interrupts
    SPI_master_init(0x00, SLOW);          //initialize MSSP module for SPI communication
    nrf24_init();          //initialize nRF24L01+

    //relay board pins are output
    ADCON1|=0x0F;
    TRISD&=0x00;
    TRISC&=0xFF;
    TRISE&=0xFC;
    TRISA&=0x00;

    //initialise relays
    RELAY_01=0;
    RELAY_02=0;
    RELAY_03=0;
    RELAY_04=0;
    RELAY_05=0;
    RELAY_06=0;
    RELAY_07=0;
    RELAY_08=0;

    RELAY_11=0;
    RELAY_12=0;
    RELAY_13=0;
    RELAY_14=0;
    RELAY_15=0;
    RELAY_16=0;
    RELAY_17=0;
    RELAY_18=0;
}
//Interrupt initialize function
void interrupt_init(void)
{
    IPEN=1;       //enable priority of interrupts
    //enable interrupts
    GIEH=1;
    GIEL=1;
    INTEDG2=0;        //INT2 on falling edge
    INT2IE=1;     //enable INT2
    INT2IF=0;         //clear interrupt flag
}
//ISR function
void interrupt isr(void)
{
    nrf24_data_rx();       //get received data
    relay_control();              //switch relays as required
}
//relay control function
void relay_control(void)
{
    if(rx_data[0]==rx_addr)                                  //data received
    {
        if((rx_data[1]=='P') && (PRI_BRD_EN==1))         //primary board
        {
            switch(rx_data[2])
            {
                case 0x01:
                    RELAY_01=rx_data[3];
                    break;

                case 0x02:
                    RELAY_02=rx_data[3];
                    break;

                case 0x03:
                    RELAY_03=rx_data[3];
                    break;

                case 0x04:
                    RELAY_04=rx_data[3];
                    break;

                case 0x05:
                    RELAY_05=rx_data[3];
                    break;

                case 0x06:
                    RELAY_06=rx_data[3];
                    break;

                case 0x07:
                    RELAY_07=rx_data[3];
                    break;

                case 0x08:
                    RELAY_08=rx_data[3];
                    break;

                default:
                    asm("NOP");                //invalid command
            }
        }
        else if((rx_data[1]=='S') && (SEC_BRD_EN==1))        //secondary board
        {
            switch(rx_data[2])
            {
                case 0x11:
                    RELAY_11=rx_data[3];
                    break;

                case 0x12:
                    RELAY_12=rx_data[3];
                    break;

                case 0x13:
                    RELAY_13=rx_data[3];
                    break;

                case 0x14:
                    RELAY_14=rx_data[3];
                    break;

                case 0x15:
                    RELAY_15=rx_data[3];
                    break;

                case 0x16:
                    RELAY_16=rx_data[3];
                    break;

                case 0x17:
                    RELAY_17=rx_data[3];
                    break;

                case 0x18:
                    RELAY_18=rx_data[3];
                    break;

                default:
                    asm("NOP");         //invalid command
            }
        }
    }

    if(PRI_BRD_EN==0)                //primary board not connected
    {
        RELAY_01=0;
        RELAY_02=0;
        RELAY_03=0;
        RELAY_04=0;
        RELAY_05=0;
        RELAY_06=0;
        RELAY_07=0;
        RELAY_08=0;
    }
    if(SEC_BRD_EN==0)                //secondary board not connected
    {
        RELAY_11=0;
        RELAY_12=0;
        RELAY_13=0;
        RELAY_14=0;
        RELAY_15=0;
        RELAY_16=0;
        RELAY_17=0;
        RELAY_18=0;
    }
}
