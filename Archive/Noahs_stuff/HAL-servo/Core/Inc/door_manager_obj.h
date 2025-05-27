#ifndef DOOR_MANAGER_OBJ_H
#define DOOR_MANAGER_OBJ_H

#include "door_obj.h"
#include "servo_driver_obj.h"
#include <stdint.h>
#include <stdlib.h>

typedef struct{

	doorObj **door_objs; //Series of door-objs in management
	uint8_t door_count; //Number of door-objs
	uint8_t curr_enslaved_i; //Index of currently enslaved door-obj

} doorManagerObj;


// Initializes a door manager object
doorManagerObj *init_door_manager(uint8_t door_count, float *master_angle_pt, servoDriverObj *driver_pt, uint8_t driver_i_offset);

// Sets the currently enslaved door in the manager
void door_manager_set_enslaved(doorManagerObj *door_manager_pt, uint8_t door_i);

// Sets the specified door to a stable state at a given angle
void door_manager_set_angle(doorManagerObj *door_manager_pt, uint8_t door_i, float angle);

// Updates the state of all doors managed by the door manager
void door_manager_update(doorManagerObj *door_manager_pt);



#endif
