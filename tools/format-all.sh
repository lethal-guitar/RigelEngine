#!/bin/bash

CLANG_FORMAT="clang-format-11"

if ! which $CLANG_FORMAT > /dev/null 2>&1; then
    CLANG_FORMAT="clang-format"
fi

if ! which $CLANG_FORMAT > /dev/null 2>&1; then
    echo "error: clang-format not found, make sure it's in the PATH"
    exit 1
fi

if ! $CLANG_FORMAT --version | grep "11\.0\.0" > /dev/null 2>&1; then
    echo "error: wrong version of clang-format found, version 11.0.0 needed"
    exit 1
fi

find "$(pwd)/benchmark" "$(pwd)/src" "$(pwd)/test" -iname *.\[chi\]pp -exec $CLANG_FORMAT -i {} +
