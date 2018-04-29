#!/bin/bash

set -euo pipefail

cd "$(dirname "$0")"

export PYTHONPATH="$PWD/lib:$PWD"

declare -r name="${1:-}"

shift

if [ -z "$name" ]; then
	echo "Known demos:"
	echo ""
	printf -- " * %s\n\n" "$(ls -d */ | sed -e 's/\/$//' | grep -vwE 'lib|__pycache__' )"
	exit 1
fi

python3 "$name/Main.py" "$@"
