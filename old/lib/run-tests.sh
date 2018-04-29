#!/bin/bash

set -euo pipefail

cd "$(dirname "$0")"

export PYTHONPATH="$PWD"

for test in test/*Test.py; do
	printf -- "\x1b[1m * %s\x1b[0m\n" "$(basename "$test")"
	python3 "$test"
	printf -- "\n"
done
