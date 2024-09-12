#!/bin/bash

# environment variables received by this script from caller
# 
# ${sample_dir}  - directory from where to get the data files
# ${wrapper}     - an external binary that will indirectly call the unit test - like valgrind or strace

cat << EOF > manifest
e170f2eb5400938f96c7cefad670e97f34f4e15726f481f79c70123ff41a83a1  analog_1.bin
81a726045dad041ba88dccc81f2ee8bfb2c3f9a9b02de33e04afcebc40ec0012  analog_10.bin
d67664fe8b97821aa4824eb2eb7a118aa33739a3e9b82c980d547aa5ef738e4e  analog_11.bin
e0007cf549c0bd3a90fd500e8a033b8135f8407b11c561db8965338d1118558a  analog_12.bin
ab26453c238cf7c1aa7d392c17b883ebb78ae74bf01a1162b8523c4d87c70063  analog_13.bin
ec86d2597a3921d989ea93a9c86004ebeb5a292e35cbe360552081b7f545ada4  analog_14.bin
14bb7c9fa256fec1dd78246b0c2841d0557d0ff181afed74332dad42c26c5318  analog_15.bin
056fc43a7cc2fdd5d90b255f3f85d4a31c5c7dba7779690f1dc49d67b4fbe977  analog_16.bin
2862e302efef2f00c757479792e6c0af46e704a35175a3c40b4432cb73f16afd  analog_2.bin
85fe8a0bd906c5d9bbe18ba0b6d07a161f7bec4d0314d810dbbdccc86be9bca1  analog_3.bin
963f501b2ff7a89912e93f2a1731fb04d0591dcb5f08e71545c4df9ec68667ec  analog_4.bin
2abac5b4fdfe2774adeb94881d7226cfc118142ad414033e0b2725658d32aaef  analog_5.bin
6e53f4b5e220e6ce23e903277fa93fc0c6929ca343bfa296bdf0baf8a2693af4  analog_6.bin
c879dcae79937750f90439d048123abbb55b0feef2b9e052a9745179457491c9  analog_7.bin
be34c19c24ad0108ecd40ac7b73c137217b226d96005cabb78b8d43df6b65b54  analog_8.bin
c5de76ac06393dfc3fd4b5832f752c30a0db2924e162c6964177141a646d6b2f  analog_9.bin
EOF

${wrapper} ./eecu-sat --input "${sample_dir}/analog_[0-9]*.bin" --output ./analog_ --output-format analog
ret=$?

sha256sum --quiet -c manifest
ret=$(($? + ret))

exit "${ret}"
