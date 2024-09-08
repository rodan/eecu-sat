#!/bin/bash

# environment variables received by this script from caller
# 
# ${sample_dir}  - directory from where to get the data files

cat << EOF > manifest
d3831112906a08305965ed01365006164219fc8cf09a6ce3700252d72a8ee08e  analog-1-1-1
878e281a518bd9da1274667728e1a57fbc1c1a8b455788b6b1401095c77afa03  analog-1-10-1
75e98c0f6ef6a2535440d82f16b4396dcbb1291cefb9349841cce91fe8b029bf  analog-1-11-1
1cf3eb30d2a71325eaf69afab445825d14530713900e5709c31c495e98d325de  analog-1-12-1
6d750ddeb61cf9b0e6b88128637b1289c5e5162b1b5661b34a2ce0784b905c7a  analog-1-13-1
86c1378d2c70cdb66e6cab9fe9d50d2cb61080391b1b96b8ec2f573c122f3ddd  analog-1-14-1
7a967aaab9abb7acdf7a6ea0f5c51d14b31ebc594753f34088254cdb0840764e  analog-1-15-1
e5f546fdfc8f7496c880e7f639e0bd67b34181e608e53e215dab411267029fd6  analog-1-16-1
d43107970e8fe5a4c666806b73cadf5e5668f10c9c1f056d257a46228c0f5772  analog-1-2-1
5423ae5dd76d061aa1c4ffbef39bcc8b394e57f3a13854ab12dee4f25bb8f02b  analog-1-3-1
b98c601a21f8b839c8eb45397ca81c9a54eb18a03c5d1e72f65e1eb2263a0882  analog-1-4-1
e0880629674ce5be791e1b9916d7d880fd38c88648441cca51a571f8e91a92e9  analog-1-5-1
294da3dd938958f0848d550715f87b9339df1d094601f91a1867749d68e5a69e  analog-1-6-1
6decbd7315d17b17995d16d18f139c9339add5af021c50506791e4463bf8cb74  analog-1-7-1
3b05f42d13230db877f4e5b8692ca9e24e9e03c4550fa5d7b9dd8353c04c3fff  analog-1-8-1
e1f19948fa609f538bd2f26fe94ff576311cd3edd487f43f81d23efab08faa5c  analog-1-9-1
6d03de2af8da5f4eedaa59bce29dad0c58e0ba2e2252a600dd343a04a321a90c  metadata
d4735e3a265e16eee03f59718b9b5d03019c07d8b6c51f90da3a666eec13ab35  version
EOF

./eecu-sat --input "${sample_dir}/analog_[0-9]*.bin" --output ./calibrated.sr --output-format "srzip:metadata_file=${sample_dir}/metadata_16ch" --transform-module "calibrate_linear_3p:calib_file=${sample_dir}/calib_reference.ini"
ret=$?

unzip calibrated.sr

sha256sum -c manifest
ret=$(($? + ret))

exit "${ret}"
