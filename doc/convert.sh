eecu-sat --input "s3/analog_[0-9]*.bin" -t "ch=analog_0.bin:type=u:level=7.00:name=jeff:nth=1:b=50000:a=500000" --transform-module "calibrate_linear_3p:calib_file=calib_reference.ini" --output /tmp/cal/analog_ --output-format analog

eecu-sat --input "s1/analog_[0-9]*.bin" -t "ch=analog_0.bin:type=u:level=7.00:name=jeff:nth=1:b=50000:a=500000" --transform-module "calibrate_linear_3p:calib_file=calib_reference.ini" --output /tmp/cal/analog_ --output-format analog:channel_offset=16

eecu-sat --input "s2/analog_[0-9]*.bin" -t "ch=analog_8.bin:type=u:level=7.00:name=jeff:nth=1:b=50000:a=500000" --transform-module "calibrate_linear_3p:calib_file=calib_reference.ini" --output /tmp/cal/analog_ --output-format analog:channel_offset=30

eecu-sat --input "cal/analog_[0-9]*.bin" --output cal/start.sr --output-format "srzip:metadata_file=/home/prodan/_work/linux/eecu-sat/doc/metadata"

