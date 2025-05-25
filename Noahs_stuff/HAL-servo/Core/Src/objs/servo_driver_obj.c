#include <stdlib.h>
#include <servo_driver_obj.h>
#include "main.h"
#include "utils.h"
#include "stm32f3xx_hal.h"



/*
//Servo object
typedef struct{
    //All the pins +setup stuff
    uint8_t i2c_adr
    float servo_angles[16]; //Array of servo-angles (chans 1-16)
    float servo_angle_offsets[16]; //Array of servo-offsets (for calibration) (chans 1-16)
    uint16_t chan_en_mask; //16-bit mask for enabled servo-chans
} servoDriverObj;
*/




//Servo-driver is represented in object form to allow multiple chained devices (16-boards < )
servoDriverObj *init_servo_driver(uint8_t i2c_adr){
    //Allocate obj-mem
    servoDriverObj *driver_pt = malloc(sizeof(servoDriverObj));

    //Asign devices I2C address (probably =0x40)
    driver_pt->i2c_adr = i2c_adr;


    //Configure device
	//For 50hz PWM output: prescale = 25MHz / (4096 × 50Hz) - 1 ≈ 121
	uint8_t prescale = 121;
	servo_driver_write_reg(driver_pt, MODE1, 0x10);       // Sleep
	servo_driver_write_reg(driver_pt, PRESCALE, prescale);
	servo_driver_write_reg(driver_pt, MODE1, 0xA1);       // Restart + auto-increment


    //Init empty enable-mask (no active servos)
    driver_pt->chan_en_mask = 0;

    //Populate servo-data (0 default)
    for (int i=0; i<16; i++){
        driver_pt->servo_angles[i] = 0;
        driver_pt->servo_angle_offsets[i] = 0;
    }

    return driver_pt;
}




//Write a register in driver
void servo_driver_write_reg(servoDriverObj *driver_pt, uint8_t reg, uint8_t data) {
    HAL_I2C_Mem_Write(&hi2c1, (driver_pt->i2c_adr << 1), reg, I2C_MEMADD_SIZE_8BIT, &data, 1, HAL_MAX_DELAY);
    return;
}



//Enable specified servo-channels (will automatically actuate angle)

//ADD DISABLE!!!!

void servo_driver_en_chans(servoDriverObj *driver_pt, uint8_t chan_index_l, uint8_t chan_index_h){
    //Set enable-bits for selected servo-chans
    for (int i = chan_index_l; i < chan_index_h + 1; i++){
        driver_pt->chan_en_mask |= (1 << i);
        //Actuate angle
        servo_driver_set_angle(driver_pt, i, driver_pt->servo_angles[i]);
    }

    return;
}





//Set angle of servo on specified channel
void servo_driver_set_angle(servoDriverObj *driver_pt, uint8_t chan_index, float angle){
	//Update servo-state
	driver_pt->servo_angles[chan_index] = angle;


	//Don't actuate the updated angle if the channel isn't enabled
	if ((driver_pt->chan_en_mask & (1 << chan_index)) == 0){
		return;
	}

	//Apply calibration offset
	float true_angle = 180.0f - (driver_pt->servo_angle_offsets[chan_index] + angle + 90.0f);

    //Map angle to tick count (0-180 deg  ->  150-500 ticks)
	uint16_t num_ticks = (uint16_t) map_range( clamp(true_angle, 0.0, 180.0), 0, 180, 500, 150);

	//Pointer to the lower 8-bits of tick-count
	uint8_t *tick_pt = (uint8_t *) &num_ticks;

	//Address of pwm_registers for specified channel
	uint8_t pwm_base_reg = 0x06 + chan_index*4;

	//Write lower and higher reg for tick-count
	servo_driver_write_reg(driver_pt, pwm_base_reg, 0x00);
	servo_driver_write_reg(driver_pt, pwm_base_reg + 1, 0x00);
	servo_driver_write_reg(driver_pt, pwm_base_reg + 2, *tick_pt);
	servo_driver_write_reg(driver_pt, pwm_base_reg + 3, *(tick_pt + 1));


	return;
}



//Returns the angle-state of a specified servo
float servo_driver_query_angle(servoDriverObj *driver_pt, uint8_t chan_index){
	return driver_pt->servo_angles[chan_index];
}






