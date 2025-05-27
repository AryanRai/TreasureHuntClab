
#include "door_obj.h"
#include "utils.h"

#include <stdlib.h>


extern const float STEP_SIZE = 1.0/STEP_COUNT;



//doorObj initialiser
doorObj *init_door(float *parent_angle_pt, servoDriverObj *driver_pt, uint8_t servo_index){

    //Allocate mem for door
    doorObj *door_pt = malloc(sizeof(doorObj));

    door_pt->driver_pt = driver_pt;
    //Index of servo-channel that controls door
    door_pt->servo_index = servo_index;

    //Init state
    door_pt->state = STABLE;

    door_pt->parent_angle_pt = parent_angle_pt;

    return door_pt;
}


//Set angle of door's servo (and state to stable)
void door_set_angle_stable(doorObj *door_pt, float angle){
    door_pt->state = STABLE;
    door_pt->stable_angle = angle;
    return;
}


//Calibrate/zero door's servo
void door_calib_servo(doorObj *door_pt){
    //FIX!!!!!!!
    return;
}


void door_update(doorObj *door_pt){
    switch (door_pt->state){
        //Can either be open or closed (or whatever stable-state is specified in door_pt->stable_state)
        case STABLE:
            //Check if servo needs closing (angle update to 0 deg)
            if (servo_driver_query_angle(door_pt->driver_pt, door_pt->servo_index) != door_pt->stable_angle){
                servo_driver_set_angle(door_pt->driver_pt, door_pt->servo_index, door_pt->stable_angle);
            }

            break;


        case ENSLAVED:
            //Update servo-angle to potentiometer-value (clamping value to rotation-range)
        	servo_driver_set_angle(door_pt->driver_pt, door_pt->servo_index,
                            clamp(*(door_pt->parent_angle_pt), 0.0, 90.0));

            break;


        case AUTO_EASING:
            //Use cubic bezier-interpolation for smooth/easing servo
        	servo_driver_set_angle(door_pt->driver_pt, door_pt->servo_index,
                            bezier1D(door_pt->anim_t,
                                    door_pt->start_angle,
                                    door_pt->end_angle, EASING_STRENGTH) );

            //Increment animation-position t (step++)
            door_pt->anim_t += STEP_SIZE;

            //Check if animation has been completed
            if (door_pt->anim_t >= 1.0){
                //Update state to stable (unchanging)
                door_set_angle_stable(door_pt, door_pt->end_angle);
            }
            break;
    }

    return;
}
