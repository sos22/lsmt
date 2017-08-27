#! /bin/bash

set -e
set -o pipefail

make -s tests/order
if ! diff <(tests/order) tests/order.expected
then
    echo "FAILED"
    false
else
    echo "PASSED"
fi

