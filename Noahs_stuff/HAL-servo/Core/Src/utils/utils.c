#include "utils.h"

float map_range(float val, float in_min, float in_max, float out_min, float out_max){
	return out_min + (val - in_min)/(in_max - in_min) * (out_max - out_min);
}

//Utility functions
float clamp(float val, float min, float max){    return val < min ? min : (max < val ? max : val);
}
float bezier1D(float t, float P0, float P3, float K) {
    //Handle-offset A as a proportion K of the range

	//Clamp t within legal-range
	t = clamp(t, 0.0, 1.0);

    float A = (P3 - P0)*K;

    //Calculate the inner-handle positions
    float P1 = P0 + A;
    float P2 = P3 - A;

    //Calculate bezier-value at position t
    float u = 1.0 - t;
    return
        u*u*u * P0 +
        3 * u*u * t * P1 +
        3 * u * t*t * P2 +
        t*t*t * P3;
}
