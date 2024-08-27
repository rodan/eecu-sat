#!/bin/bash

#make && ./eecu-sat --input '../qa/analog_[0-9]*.bin' --output /tmp/analog_ --output-format analog --transform-module calibrate_linear_3p:calib_file=../qa/240527-calib/240527_calib.ini
make && ./eecu-sat --input '../qa/240527-calib/analog_[0-9]*.bin' --output /tmp/calibrated.sr --output-format srzip:metadata_file=../qa/240527-calib/metadata --transform-module calibrate_linear_3p:calib_file=../qa/240527-calib/240527_calib.ini

