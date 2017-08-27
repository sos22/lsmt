#! /bin/bash

set -e

make -s tests/order
if ! diff <(tests/order) tests/order.expected
then
    echo "FAILED"
    false
else
    echo "PASSED"
fi

