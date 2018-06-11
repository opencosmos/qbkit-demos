#!/bin/bash

set -eu

cd "$(dirname "$0")"

declare -r base_dev='/var/tmp/serial_chat'
declare -r a_dev="${base_dev}_a.dev"
declare -r b_dev="${base_dev}_b.dev"

declare -r base_url='ipc:///var/tmp/serial_chat'
declare -r a_rx_url="${base_url}_a_rx.zmq"
declare -r a_tx_url="${base_url}_a_tx.zmq"
declare -r b_rx_url="${base_url}_b_rx.zmq"
declare -r b_tx_url="${base_url}_b_tx.zmq"

while read line; do
	tmux splitw -vfl 6 sh -xc "$line || { echo 'Failed' && read tmp ; }"
	sleep 0.2
done < <(
cat <<EOF
    socat PTY,raw,echo=0,link="$a_dev" PTY,raw,echo=0,link="$b_dev"
    ./bin/bridge --device="$a_dev" --rx_url="$a_rx_url" --tx_url="$a_tx_url"
    ./bin/bridge --device="$b_dev" --rx_url="$b_rx_url" --tx_url="$b_tx_url"
    ./bin/chat --rx_url="$a_rx_url" --tx_url="$a_tx_url" --username=deadpool
    ./bin/chat --rx_url="$b_rx_url" --tx_url="$b_tx_url" --username=cable
EOF
)
