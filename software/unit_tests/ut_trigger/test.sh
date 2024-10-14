#!/bin/sh

# environment variables received by this script from caller
# 
# ${sample_dir}  - directory from where to get the data files
# ${wrapper}     - an external binary that will indirectly call the unit test - like valgrind or strace

cat << EOF > manifest
027dca4d099834c1454b4b85440e03fa269bf5dcb37188ad027c58a54a639c16  analog-1-1-1
43444d5312cb5334bceec9c83586ed3490c548f1eb4fca9c45cbee26e0dd10aa  analog-1-10-1
32e0712859477f980ba2a747907a1a0ba90e6cb54e8cd7aa0706aa91c2dbd78e  analog-1-11-1
0356f174431e15d8a14afee04487248389bddfd318d6d2dd7173ef4d0e1f1770  analog-1-12-1
771432747f694b4c4abd654e3eb8950fadb6749d3a364a15c6a6cd6742690246  analog-1-13-1
814a1a5647d2000ee08b2f0e4aac174fe70be567ebe2c305d656428f9412aec2  analog-1-14-1
b4ec6e16dde43d77a1118cfa101978ed91804f4d767a3cd12c20fa00cb1ce18a  analog-1-15-1
ed52f39f8f6a0c506d87781a699c7191238a237a5e0fdeb0ac5d52c55d9f4d11  analog-1-16-1
4d1299f2959963475a801d45f2738d989f00e6c48ce0d8b8027a3f3bebdb2118  analog-1-2-1
528b6d43fb0dba246cbe725e7eaf7b242f84fc774f8fbc05621ce7565a73fb2f  analog-1-3-1
88d1ff8d6b8978a827b4e12395dfe05f7fb4ee430c1d47fd78b3290cc21b436f  analog-1-4-1
d3f6c9999c2951bce49ed6971b0ccb64cccf916164111f25d6a69b2f860310b0  analog-1-5-1
ea4bf439a5021f7dbfa8dd03bdc74efaa06c09808f2563c701b20cd5a6ba0c10  analog-1-6-1
3858b99d7da7add5fb81b909674b35de32d2a8d6faf193e4872389c4018be264  analog-1-7-1
c527040b287e13a298f79551c1512779a76ee512b9ae2d971b816871863901d5  analog-1-8-1
6d4ee8ba78026dddce5d5531882bcebfeaf9668343380eea7ce1f9cb6c69cbdb  analog-1-9-1
6d03de2af8da5f4eedaa59bce29dad0c58e0ba2e2252a600dd343a04a321a90c  metadata
d4735e3a265e16eee03f59718b9b5d03019c07d8b6c51f90da3a666eec13ab35  version
EOF

${wrapper} ./eecu-sat -i "${sample_dir}//analog_[0-9]*.bin" -t "ch=analog_0.bin:type=o:level=3.00:name=jeff:nth=3:b=1000:a=1000" -o ./out.sr --output-format "srzip:metadata_file=${sample_dir}/metadata_16ch"
ret=$?

unzip -q ./out.sr
sha256sum --quiet -c manifest
ret=$(($? + ret))

exit "${ret}"
