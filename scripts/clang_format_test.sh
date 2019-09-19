#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

if [ ! -f ".clang-format" ]; then
    echo ".clang-format file not found!"
    exit 1
fi

content=$(pwd)
echo "$content"

cp -r "$content" ../test_format

pushd ../test_format
find . -type f -name "*.hpp" -or -name "*.cpp" -or -name "*.h" -or -name "*.c" \
    -exec sh -c "
for f do
    if ! git check-ignore -q \"$f\"; then
       clang-format -i -style=file $f
    fi
done
" find-sh {} +
ret=$?

popd

compare_result=$(diff -uqrNa $content test_format)

if [ -z "$compare_result" ] && [ $ret -eq 0 ]; then
    echo -e "${GREEN}All source code in commit are properly formatted.${NC}"
    rm -rf test_format
    exit 0
else
    echo -e "${RED}Found formatting errors!${NC}"
    echo $compare_result
    rm -rf test_format
    exit 1
fi
