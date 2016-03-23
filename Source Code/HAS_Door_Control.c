#include"libraries/stdconfig.h"
#include"libraries/delay.h"
#include<stdio.h>
#include"libraries/eeprom.h"

/*
 * A password based security system has been implemented using a 4x4 keypad
 * Password can be changed by the user
 * Password length has been set to 4 digits in the code & cannot be changed by the user
 * LEDs are used to indicate door actuator status & key press
 */
//rows
#define r1 RA4
#define r2 RA5
#define r3 RE0
#define r4 RE1
//columns
#define c1 RA0
#define c2 RA1
#define c3 RA2
#define c4 RA3

#include"libraries/keypad_4x4.h"

//lcd pins; required for the library
#define lcd_data PORTD
#define rs RC2
#define en RC1

#include"libraries/lcd_16x2.h"

#define key_press_indicator RB0     //key press indicator LED
#define change_request RB1    //password change request button
#define door_control RB2    //door lock actuator

//password length
#define password_length 4

void sys_init(void);
void correct_password(void);
void wrong_password(void);
void password_entry_complete_check(void);
void password_check(void);
void password_change(void);

unsigned char key=0;
unsigned char change_request_flag=0;                    //password change request flag
unsigned char button_press_count=0;
unsigned char password_entered[password_length]={0, 0, 0, 0};
unsigned char password[password_length];
void main()
{
    //initialize all ports & functions
    sys_init();
    while(1)
    {
        //scan & get password
        key=keyscan();
        if((button_press_count<password_length) && (key!=0))
        {
            password_entered[button_press_count]=key;              //save entered digit
            button_press_count++;
            //strobe LED to indicate genuine key press
            key_press_indicator=1;
            Delay_ms(250);
            key_press_indicator=0;
            key=0;
        }
        password_entry_complete_check();          //verify entered password & send appropriate messages
        password_change();          //check for password change request & send appropriate messages
    }
}
//Overall system initialize function
void sys_init(void)
{
    //rows are output & columns are input
    ADCON1|=0x0F;
    TRISA=0x0F;
    TRISE&=0xFC;
    TRISC=0xF9;                  //lcd control pins
    TRISD=0x00;                  //lcd data bus
    TRISB=0xFA;       //button, LED and door lock actuator

    //initilize LEDs
    key_press_indicator=0;
    door_control=0;       //door is locked by default

    lcd_init();                  //initialize LCD
    printf("     Enter");
    lcd_new_msg_second_line();
    printf("    Password:");

    eeprom_init();            //initialize eeprom

    //read password from eeprom
    for(unsigned char i=0; i<password_length; i++)
    {
        password[i]=eeprom_rd(i);
    }
}
//password entry complete check function
void password_entry_complete_check(void)
{
    if(button_press_count>=password_length)
    {
        password_check();                          //verify password entered
        button_press_count=0;
        if(change_request_flag==0)
        {
            lcd_clear();
            lcd_new_msg_first_line();
            printf("     Enter");
            lcd_new_msg_second_line();
            printf("    Password:");
        }
    }
}
//password verification function
void password_check(void)
{
    unsigned char flag;
    for(unsigned char i=0; i<password_length; i++)
    {
        //password must not be checked if new password is being entered
        if(change_request_flag!=2)
        {
            if(password_entered[i]==password[i])
            {
                flag=1;
            }
            else
            {
                flag=0;
            }
            //confirm new password
            if(change_request_flag==3)
            {
                if(password_entered[i]==password[i])
                {
                    flag=1;
                }
                else
                {
                    flag=0;
                }
            }
        }
        //replace old password buffer by new password
        else
        {
            flag=1;
            password[i]=password_entered[i];
        }
        //if wrong password is given
        if(flag==0)
        {
            wrong_password();
            break;
        }
    }
    //if correct password is given
    if(flag==1)
    {
        correct_password();
    }
}
//password right function
void correct_password(void)
{
    /*
     *change_request_flag=0 -> normal operation
     *change_request_flag=1 -> password change request; old password entered
     *change_request_flag=2 -> new password entered
     *change_request_flag=3 -> new password confirmed
     */

    //transmit correct password message as required
    if(change_request_flag!=2)
    {
        lcd_clear();
        lcd_new_msg_first_line();
        printf("    Correct");
        lcd_new_msg_second_line();
        printf("    Password");
        Delay_ms(2000);
    }

    switch(change_request_flag)
    {
        //welcome_message sent only if password change request was not given & user is simply trying to enter
        case 0:
            lcd_clear();
            lcd_new_msg_first_line();
            printf("    Welcome");
            lcd_new_msg_second_line();
            printf("      Home");
            //unlock the door
            door_control=1;
            Delay_ms(5000);
            door_control=0;
            break;
        //ask for new password
        case 1:
            lcd_clear();
            lcd_new_msg_first_line();
            printf("   Enter New");
            lcd_new_msg_second_line();
            printf("   Password:");
            change_request_flag++;
            break;

        case 2:
            //send message to confirm new password
            lcd_clear();
            lcd_new_msg_first_line();
            printf("  Confirm New");
            lcd_new_msg_second_line();
            printf("   Password:");
            change_request_flag++;
            break;

        //new password entering process is finished; reset request flag & remove old password completely from the system
        case 3:
            for(unsigned char i=0; i<password_length; i++)
            {
                eeprom_wr(i, password[i]);
            }
            change_request_flag=0;
            break;
    }
}
///password wrong function
void wrong_password(void)
{
    lcd_clear();
    lcd_new_msg_first_line();
    printf("   Incorrect");
    lcd_new_msg_second_line();
    printf("    Password");
    Delay_ms(2000);

    switch(change_request_flag)
    {
        //if old password was entered wrong
        case 1:
            lcd_clear();
            lcd_new_msg_first_line();
            printf("   Enter Old");
            lcd_new_msg_second_line();
            printf("   Password:");
            Delay_ms(2000);
            break;
        //if new password was typed wrong while confirming then ask to confirm again
        case 3:
            lcd_clear();
            lcd_new_msg_first_line();
            printf("  Confirm New");
            lcd_new_msg_second_line();
            printf("   Password:");
            break;
    }
}
//Password change function
void password_change(void)
{
    if(change_request==0)
    {
        Delay_ms(20);
        if(change_request==0)
        {
            //if password change request is received
            if(change_request_flag==0)
            {
                change_request_flag=1;
                lcd_clear();
                lcd_new_msg_first_line();
                printf("   Enter Old");
                lcd_new_msg_second_line();
                printf("   Password:");
            }
            //password change request can be cancelled by press the password change button again
            else
            {
                change_request_flag=0;
                //read password from eeprom
                for(unsigned char i=0; i<password_length; i++)
                {
                    password[i]=eeprom_rd(i);
                }
                lcd_clear();
                lcd_new_msg_first_line();
                printf("     Enter");
                lcd_new_msg_second_line();
                printf("    Password:");
            }
            button_press_count=0;
            Delay_ms(250);
        }
    }
}
