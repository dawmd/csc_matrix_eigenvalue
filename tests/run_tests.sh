#!/bin/bash

GREEN="\e[1;32m"
RED="\e[1;31m"
DEFAULT="\e[0m"

function test_ok {
    printf "${GREEN}OK${DEFAULT}\n"
}

function test_bad {
    printf "${RED}WRONG${DEFAULT}\n"
}

function run_tests {
    for test in $(ls *.in); do
        ./../build/csc < ${test}> output.out 2>/dev/null
        printf "${test}: "
        if (diff output.out ${test%in}out); then
            test_ok;
        else
            test_bad;
        fi
    done

    return 0
}

run_tests
