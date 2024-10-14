#!/bin/sh

# environment variables received by this script from caller
# 
# ${sample_dir}  - directory from where to get the data files
# ${wrapper}     - an external binary that will indirectly call the unit test - like valgrind or strace

echo 'b28a5e76afd755d37b82a02d99551b83a2feb29ee414f91e497c4eeda205c815  calib.ini' > manifest

cat "${sample_dir}"/calib_globals.ini > calib.ini 

${wrapper} ./eecu-sat --input "${sample_dir}/analog_[0-9]*.bin" --output-format calibrate_linear_3p:calib_file=./calib.ini --output /dev/null
ret=$?

sha256sum --quiet -c manifest
ret=$(($? + ret))

[ "${ret}" != 0 ] && diff -u "${sample_dir}"/calib_reference.ini calib.ini

exit "${ret}"
