#!/bin/bash

set -eu

declare -r base_dev='/var/tmp/serial_chat'
declare -r a_dev="${base_dev}_a.dev"
declare -r b_dev="${base_dev}_b.dev"

declare -r base_url='ipc:///var/tmp/serial_chat'
declare -r a_server_url="${base_url}_a_rx.zmq"
declare -r a_client_url="${base_url}_a_tx.zmq"
declare -r b_server_url="${base_url}_b_rx.zmq"
declare -r b_client_url="${base_url}_b_tx.zmq"

while read line; do
	tmux splitw -vfl 6 sh -xc "$line"
	sleep 0.2
done < <(
cat <<EOF
    socat PTY,raw,echo=0,link="$a_dev" PTY,raw,echo=0,link="$b_dev"
    ./bin/bridge --device="$a_dev" --server_url="$a_server_url" --client_url="$a_client_url"
    ./bin/bridge --device="$b_dev" --server_url="$b_server_url" --client_url="$b_client_url"
    ./bin/chat --server_url="$a_server_url" --client_url="$a_client_url" --username=deadpool
    ./bin/chat --server_url="$b_server_url" --client_url="$b_client_url" --username=cable
EOF
)
