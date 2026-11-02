#pragma once

void motion_init();

// called when "SCAN IT!" is called. selects and stores a random ghost target angle
void motion_begin_scan();

// read the sensor and update the estimated rotation
void motion_update();

bool motion_is_ready();

// returns true if a scan is currently active
bool motion_is_scanning();

// used so that the display knows which way to skew the proximity meter
float motion_scan_direction(float ghostAngleDeg);

//checks whether the player is close enough to the ghost to count as finding it
bool motion_target_found(float ghostAngleDeg);

// stops scanning after ending the "scan it" state
void motion_end_scan();