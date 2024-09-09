#!/bin/bash

SED_BIN='sed'
uname -s | grep -q 'FreeBSD' && SED_BIN='gsed'

build=$(($(grep BUILD version.h | "${SED_BIN}" 's|.*BUILD\s\([0-9]\{1,9\}\).*|\1|')+1))
"${SED_BIN}" -i "s|^.*BUILD.*$|#define BUILD ${build}|" version.h

commit=$(git log | grep -c '^commit')
"${SED_BIN}" -i "s|^.*COMMIT.*$|#define COMMIT ${commit}|" version.h

compile_date=$(date -u)

"${SED_BIN}" -i "s|^// compiled on.*|// compiled on ${compile_date}|" version.h

