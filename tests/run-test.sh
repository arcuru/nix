#!/usr/bin/env bash

# To be run from meson

TESTS_ENVIRONMENT=("TEST_NAME=${test%.*}" 'NIX_REMOTE=')

: ${BASH:=/usr/bin/env bash}

# init test
cd ../tests && env "${TESTS_ENVIRONMENT[@]}" $BASH -e init.sh 2>/dev/null > /dev/null

# run test
cd $(dirname $1) && env "${TESTS_ENVIRONMENT[@]}" $BASH -e $(basename $1)
