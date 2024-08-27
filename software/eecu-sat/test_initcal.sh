#!/bin/bash

cp -f ../qa/240527_calib.ini ../qa/240527-calib/
make && ./eecu-sat --input '../qa/240527-calib/analog_[0-9]*.bin' --output-format calibrate_linear_3p:calib_file=../qa/240527-calib/240527_calib.ini --output /dev/null
cat ../qa/240527-calib/240527_calib.ini

