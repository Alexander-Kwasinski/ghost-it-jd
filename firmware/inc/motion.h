#pragma once

void motion_init();

// called when "SCAN IT!" is called. selects and stores a random ghost target angle
void game_begin_scan();

// read the sensor and update the estimated rotation
void motion_update();

//return the player's current rotation relative to the starting orientation.
float motion_get_relative_heading_deg();

//takes the players current heading and calculates how close they are to the ghost
// returning a percentage from 0 to 100
int motion_scan_proximity_percent(float playerHeadingDeg);

//checks whether the player is close enough to the ghost to count as finding it
bool game_scan_target_found(float playerHeadingDeg);

// stops scanning after ending the "scan it" state
void motion_end_scan();