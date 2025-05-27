#ifndef DOOR_OBJ_H
#define DOOR_OBJ_H

#include <stdint.h>
#include "servo_driver_obj.h"

#define EASING_STRENGTH 0.3
#define STEP_COUNT 100

// Door state enum
enum motion_state { STABLE, AUTO_EASING, ENSLAVED };

// Door object struct
typedef struct {
    servoDriverObj *driver_pt;
    uint8_t servo_index;

    enum motion_state state;
    float *parent_angle_pt;
    float stable_angle;

    float start_angle;
    float end_angle;
    float anim_t;
} doorObj;

// Function declarations
doorObj *init_door(float *parent_angle_pt, servoDriverObj *driver_pt, uint8_t servo_index);
void door_set_angle_stable(doorObj *door_pt, float angle);
void door_calib_servo(doorObj *door_pt);
void door_update(doorObj *door_pt);

#endif
