#!/bin/sh

set -e

BASE_DIR="$(dirname `readlink -f ${0}`)"
SRC_DIR="$(readlink -f "${BASE_DIR}/..")"
BUILD_DIR=.
REGENERATE=

RED="\e[31m"
GREEN="\e[32m"
BLUE="\e[34m"
ERROR="[ ERROR   ]"
SUCCESS="[ SUCCESS ]"
INFO="[  INFO   ]"
DEBUG="[  DEBUG  ]"
RESET="\e[m"

function info()
{
    echo -e "${BLUE}${INFO}${RESET} ${@}"
}

die()
{
    echo -e "${RED}${ERROR} ${@}${RESET}"
    exit 1
}

pushd_silent()
{
    pushd "${1}" &>/dev/null || die "No directory: ${1}"
}

popd_silent()
{
    popd &>/dev/null || die "Cannot go back to previous dir!"
}

read_cache()
{
    if [ -f "${BUILD_DIR}/CMakeCache.txt" ]
    then
        while IFS= read -r line
        do
            case ${line} in
                "OPTIMIZE:"*)
                    OPTIMIZE="${line#*=}"
                    ;;
                "COVERAGE:"*)
                    COVERAGE="${line#*=}"
                    ;;
                "SANITIZE:"*)
                    SANITIZE="${line#*=}"
                    ;;
                "BUILD_TESTS:"*)
                    BUILD_TESTS="${line#*=}"
                    ;;
                *)
                    ;;
            esac
        done <<< $(cat ${BUILD_DIR}/CMakeCache.txt)
    fi
}

read_flags()
{
    local temp
    while [[ $# -gt 0 ]]; do
        case "${1}" in
            --optimize=*)
                temp="${1#*=}"
                if [[ "${temp}" != "${OPTIMIZE}" ]]
                then
                    REGENERATE="true"
                fi
                OPTIMIZE="${temp}"
                ;;
            --coverage=*)
                temp="${1#*=}"
                if [[ "${temp}" != "${COVERAGE}" ]]
                then
                    REGENERATE="true"
                fi
                COVERAGE="${temp}"
                ;;
            --sanitize=*)
                temp="${1#*=}"
                if [[ "${temp}" != "${SANITIZE}" ]]
                then
                    REGENERATE="true"
                fi
                SANITIZE="${temp}"
                ;;
            --build-tests=*)
                temp="${1#*=}"
                if [[ "${temp}" != "${BUILD_TESTS}" ]]
                then
                    REGENERATE="true"
                fi
                BUILD_TESTS="${temp}"
                ;;
            run|test)
                COMMAND="${1}"
                shift
                ARGS=${@}
                break
                ;;
            *)
                die "Unsupported flag/command: ${1}"
                ;;
        esac
        shift
    done
}

regenerate_cmake()
{
    if [ ! -d "${BUILD_DIR}" ] || [ ! -f "${BUILD_DIR}/build.ninja" ] || [ -n "${REGENERATE}" ]
    then
        cmake -DOPTIMIZE=${OPTIMIZE} -DSANITIZE=${SANITIZE} -DCOVERAGE=${COVERAGE} -DBUILD_TESTS=${BUILD_TESTS} -GNinja -B "${BUILD_DIR}" "${SRC_DIR}"
    fi
}

build_target()
{
    pushd_silent "${BUILD_DIR}"
    if [ -f build.ninja ]
    then
        local ts="$(date +%s)"
        ninja "${1}"
        info "Build stats:"
        ${BASE_DIR}/ninja_log_parse.py ".ninja_log" "${ts}"
    else
        die "CMake build was not generated"
    fi
    popd_silent
}

run_command()
{
    info "Running: ${@}"
    local command="${1}"
    shift
    "${command}" ${@}
}

[ -z "${1}" ] && die "No command given"

if [ "$(readlink -f "${BASE_DIR}/..")" == "$(readlink -f .)" ]
then
    BUILD_DIR="build"
fi

COMMAND=
ARGS=
OPTIMIZE="ON"
COVERAGE="OFF"
SANITIZE="OFF"
BUILD_TESTS="OFF"

read_cache
read_flags ${@}
regenerate_cmake

case "${COMMAND}" in
    run)
        shift
        build_target log-viewer
        run_command "${BUILD_DIR}/log-viewer" ${ARGS}
        ;;
    test)
        shift
        if [ "${COVERAGE}" == "ON" ]
        then
            build_target test
            ninja tests-cov-html
        else
            build_target test
            run_command "${BUILD_DIR}/test/test" ${ARGS}
        fi
        ;;
    *)
        die "Missing command: ${COMMAND}"
        ;;
esac
