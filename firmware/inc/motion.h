#pragma once

void motion_init();

// called when "SCAN IT!" is called. selects and stores a random ghost target angle
void motion_begin_scan();

// read the sensor and update the estimated rotation
void motion_update();

//checks whether the player is close enough to the ghost to count as finding it
bool motion_scan_target_found(float ghostAngleDeg);

// stops scanning after ending the "scan it" state
void motion_end_scan();