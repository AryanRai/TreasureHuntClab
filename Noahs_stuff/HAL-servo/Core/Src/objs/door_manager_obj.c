#include "door_manager_obj.h"



/*
typedef struct{

	doorObj **door_objs; //Series of door-objs in management
	uint8_t door_count; //Number of door-objs
	uint8_t curr_enslaved_i; //Index of currently enslaved door-obj

} doorManagerObj;
*/


//Init door-managing object
doorManagerObj *init_door_manager(uint8_t door_count, float *master_angle_pt, servoDriverObj *driver_pt, uint8_t driver_i_offset){
	//Allocate memory for door-manager
	doorManagerObj *door_manager_pt = malloc(sizeof(doorManagerObj));

	door_manager_pt->door_objs = malloc(sizeof(doorObj *) * door_count);

	//Number of doors under management
	door_manager_pt->door_count = door_count;
	//Initially none of the doors are enslaved (STABLE 0.0 default)
	door_manager_pt->curr_enslaved_i = -1;

	//Enable appropriate channels on servo-driver (driver_i_offset is base index)
	servo_driver_en_chans(driver_pt, driver_i_offset, driver_i_offset + door_count-1);

	//Populate door-objs
	for (uint8_t i=0; i<door_count; i++){
		//Note: driver_i_offset is the starting index of servos in driver (ascending series)
		door_manager_pt->door_objs[i] = init_door(master_angle_pt, driver_pt, i + driver_i_offset);
	}


	return door_manager_pt;
}


//Change the currently enslaved door
void door_manager_set_enslaved(doorManagerObj *door_manager_pt, uint8_t door_i){
	//Check if queried index is within legal-range
	if (0 <= door_i && door_i < door_manager_pt->door_count){

		//Set previously enslaved door to stable-state (only if necessary)
		if (door_manager_pt->curr_enslaved_i != (uint8_t) -1){
			door_manager_pt->door_objs[ door_manager_pt->curr_enslaved_i ]->state = STABLE;
		}

		//Set specified door's index to enslaved-state
		door_manager_pt->door_objs[ door_i ]->state = ENSLAVED;

		//Update index of currently-enslaved door
		door_manager_pt->curr_enslaved_i = door_i;

	}


}

//Set door at specified index to stable state (at a given angle)
void door_manager_set_angle(doorManagerObj *door_manager_pt, uint8_t door_i, float angle){
	//Check if queried index is within legal-range
	if (0 <= door_i && door_i < door_manager_pt->door_count){
		if (0.0f <= angle && angle <= 80.0f){
			door_set_angle_stable(door_manager_pt->door_objs[ door_i ], angle);
		}
	}
}


void door_manager_reset(doorManagerObj *door_manager_pt){
	//Set all doors to stable 0.0
	for (uint8_t i=0; i<door_manager_pt->door_count; i++){
		door_set_angle_stable(door_manager_pt->door_objs[ i ], 0.0f);
	}

	door_manager_pt->curr_enslaved_i = -1;

}


//Update (and actuate) the state of each door under management
void door_manager_update(doorManagerObj *door_manager_pt){
	for (uint8_t i=0; i<door_manager_pt->door_count; i++){
		door_update(door_manager_pt->door_objs[i]);
	}
}
