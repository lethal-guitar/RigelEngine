#!/bin/bash

CLANG_FORMAT="clang-format-11"

if ! which $CLANG_FORMAT >/dev/null 2>&1; then
  CLANG_FORMAT="clang-format"
fi

if ! which $CLANG_FORMAT >/dev/null 2>&1; then
  echo "error: clang-format not found, make sure it's in the PATH"
  exit 1
fi

if ! $CLANG_FORMAT --version | grep "11\.\([0-9]\+\)\.\([0-9]\+\)" >/dev/null 2>&1; then
  echo "error: wrong version of clang-format found, version 11.x.x needed"
  exit 1
fi

STAGED_FILES="$(git diff --name-only --staged)"
REPO_ROOT="$(git rev-parse --show-toplevel)"

for file in $STAGED_FILES; do
  if [[ "$file" =~ \.[chi]pp$ ]]; then
    $CLANG_FORMAT -i "$REPO_ROOT/$file"
  fi
done
