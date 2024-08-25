#!/bin/bash

make && ./eecu-sat --input '../qa/240527-calib/analog_[0-9]*.bin' --calibration '../qa/240527-calib/240527_calib.ini' --output /tmp/

