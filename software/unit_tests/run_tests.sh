#!/bin/sh

# the working directory while running this script must either be 
# $project/software/unit_tests or $project/software/eecu-sat

export prefix=$(dirname $(realpath $0))
export sample_dir="${prefix}/sample_files"
tmp_dir='/tmp/eecpu-sat'
log_file="${tmp_dir}/log"
keep_output='false'

GOOD=$'\e[32;01m'
WARN=$'\e[33;01m'
BAD=$'\e[31;01m'
HILITE=$'\e[36;01m'
BRACKET=$'\e[34;01m'
NORMAL=$'\e[0m'
ENDCOL=$'\e[A\e['$(( COLS - 8 ))'C'

ebegin() {
    echo -e " ${GOOD}*${NORMAL} $*"
}

eend() {
    retval="${1:-0}"
    efunc="${2:-eerror}"

    if [ ${retval} == "0" ]; then
            msg="${BRACKET}[ ${GOOD}ok${BRACKET} ]${NORMAL}"
    else
            msg="${BRACKET}[ ${BAD}!!${BRACKET} ]${NORMAL}"
    fi
    echo -e "${ENDCOL} ${msg}"
}

tests="ut_calibration_init ut_calibration ut_output_analog ut_output_srzip ut_output_srzip_metadata_import ut_trigger"

run_test() {
    ebegin "     ${1}"
    export test_tmp_dir="${tmp_dir}/${1}"
    rm -rf "${test_tmp_dir}"
    mkdir -p "${test_tmp_dir}"
    ln -s "${bin_file}" "${test_tmp_dir}/eecu-sat"
    cd "${test_tmp_dir}" 1>/dev/null 2>&1
    if [ "$2" == "--verbose" ]; then
        bash "${prefix}/${1}/test.sh"
        ret=$?
    else
        bash "${prefix}/${1}/test.sh" 1>>"${log_file}" 2>&1
        ret=$?
        eend "${ret}"
    fi
    cd - 1>/dev/null 2>&1
    "${keep_output}" || {
        [ "${ret}" = 0 ] && rm -rf "${test_tmp_dir}"
    }
    return "${ret}"
}

pwd | grep -q 'software/eecu-sat$' &&
    bin_file=$(realpath ./eecu-sat)

pwd | grep -q 'software/unit_tests$' &&
    bin_file=$(realpath ../eecu-sat/eecu-sat)

[ -z "${bin_file}" ] && {
    echo "the current working directory must either be 'PROJ/software/eecu-sat' or 'PROJ/software/unit-tests'"
    exit 1
}

[ ! -e "${bin_file}" ] && {
    echo "cannot find eecu-sat binary in ${bin_file}, did you compile the project?"
    exit 1
}

rm -rf /tmp/eecpu-sat
mkdir -p "${tmp_dir}"

[ ! -e "${sample_dir}" ] && {
    mkdir -p "${sample_dir}"
    unzip "${prefix}/sample_files.zip" -d "${sample_dir}" >/dev/null
}

if [ "$1" == "--valgrind" ]; then 
    export wrapper='valgrind --leak-check=full '
    for test in ${tests}; do
        run_test "${test}" --verbose
    done

else
    error_detected=false
    for test in ${tests}; do
        run_test "${test}"
        [ $? != 0 ] && error_detected=true
    done

    ${error_detected} && {
        cat "${log_file}"
        exit 1
    }
fi

exit 0
