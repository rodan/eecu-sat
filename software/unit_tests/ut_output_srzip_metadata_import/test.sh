#!/bin/bash

# environment variables received by this script from caller
# 
# ${sample_dir}  - directory from where to get the data files
# ${wrapper}     - an external binary that will indirectly call the unit test - like valgrind or strace

cat << EOF > manifest
8fc15478236654fa105b4cdc71a26440b8eb3dfcfd97a252c8c0f81a3f79d788  analog-1-1-1
ff1ca7fd59c6af3b7552fde2df846a0d209b63ac67006ad198c74552e0fd1bff  analog-1-10-1
bf1247a380046ed1f8f8279672ffb09013d625f1028d51ce1c4776af60f245e4  analog-1-11-1
2756c364fd60153431567c4348814c80c925f0eb671688af909259cb6fc4c941  analog-1-12-1
75a09e2a01247af4970951762be034e8b1087db9be8ef1f2d9b5727b2b768006  analog-1-13-1
cda149b549d10cbfed8746d925ee7f872803e4fe6c6b8af7afa009f22f6371b5  analog-1-14-1
07b90d031b4fac873ebe7d366041283b43cb47eff2a6232e1d49c3464ae9a558  analog-1-15-1
e243e401d73107b3dcd2f2efffde4e71f92f186bb4cd341075d7015775693b7c  analog-1-16-1
f30a232645ac61c09c4cc9a06f8bc3d3c75161766c83f09b494c34ca6fcfb6d0  analog-1-2-1
508017fcd0579dbf2b3ac009bc64d52d61b45e5f67f89329debc40bb1949439c  analog-1-3-1
0d3ad7c9aab07c673cc4384052acf95aa08b47ca57d386fa22ce37c92fe3c9d7  analog-1-4-1
2956af1a0fa7e01e20a94dbd48ac39cf5a06fcf778cb75eb4154aa2b271f477c  analog-1-5-1
fe521359e19f186c0a76af896a48527c22bf634e30e93bd00dcf2f50eed706f2  analog-1-6-1
e7f47dd47ad28a11ccd2f0fb5af2250850798eb018c239534c576dd8ae0d3c4c  analog-1-7-1
12624a3e9630f47096d052053d376381bd2c9ee7a473c6144031f194fa23303a  analog-1-8-1
b1af4470cb1ab58adcc0182e7f5d8ab02d4084ccc6e890a888cbf8baa2c441c7  analog-1-9-1
6d03de2af8da5f4eedaa59bce29dad0c58e0ba2e2252a600dd343a04a321a90c  metadata
d4735e3a265e16eee03f59718b9b5d03019c07d8b6c51f90da3a666eec13ab35  version
EOF

${wrapper} ./eecu-sat --input "${sample_dir}/analog_[0-9]*.bin" --output ./out.sr --output-format "srzip:metadata_file=${sample_dir}/metadata_16ch"
ret=$?

unzip -q out.sr

sha256sum --quiet -c manifest
ret=$(($? + ret))

exit "${ret}"
