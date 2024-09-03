#!/bin/bash

# environment variables received by this script from caller
# 
# ${sample_dir}  - directory from where to get the data files

./eecu-sat -i "${sample_dir}//analog_[0-9]*.bin" -t "ch=analog_0.bin:type=o:level=3.00:name=jeff:nth=1" -o ./ff.sr --output-format "srzip:metadata_file=${sample_dir}/metadata_16ch"
ret=$?

exit "${ret}"
