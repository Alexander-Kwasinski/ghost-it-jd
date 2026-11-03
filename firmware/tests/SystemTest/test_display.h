#pragma once

#include "test_inputs.h"
#include "test_motion.h"

bool testDisplayBegin();
void testDisplayUpdate(const MotionReading &motion,
                       const InputSnapshot &inputs,
                       bool motionInitialized,
                       bool displayInitialized,
                       bool sdReady,
                       bool audioPlaying,
                       const char *trackName,
                       unsigned long sdBytesRead);
