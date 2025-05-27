#ifndef SERVO_DRIVER_H
#define SERVO_DRIVER_H

// Your declarations go here



#include <stdint.h>
#include "main.h"

//IK2C handler
extern I2C_HandleTypeDef hi2c1;


//Device register-address
#define MODE1        0x00
#define PRESCALE     0xFE



//Servo-driver object
typedef struct{
    //All the pins +setup stuff
	uint8_t i2c_adr; //I2C address of device (0x40 by default)
    float servo_angles[16]; //Array of servo-angles (chans 1-16)
    float servo_angle_offsets[16]; //Array of servo-offsets (for calibration) (chans 1-16)
    uint16_t chan_en_mask; //16-bit mask for enabled servo-chans
} servoDriverObj;




//Servo-driver is represented in object form to allow multiple chained devices (16-boards < )
servoDriverObj *init_servo_driver(uint8_t i2c_adr);


//Write a register in driver
void servo_driver_write_reg(servoDriverObj *driver_pt, uint8_t reg, uint8_t data);


//Enable specified servo-channels (will automatically actuate angle)
void servo_driver_en_chans(servoDriverObj *driver_pt, uint8_t chan_index_l, uint8_t chan_index_h);


//Set angle of servo on specified channel
void servo_driver_set_angle(servoDriverObj *driver_pt, uint8_t chan_index, float angle);


//Returns the angle-state of a specified servo
float servo_driver_query_angle(servoDriverObj *servo_pt, uint8_t chan_index);


#endif // MY_HEADER_H

