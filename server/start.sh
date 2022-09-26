#!/bin/bash

# Starts all servers (print & page server)
# flag --dev enables dev mode

# 1. free up ports

. parse_yml.sh
eval "$(parse_yaml config.yml "config_")"
USED_PORTS=("$config_PRINT_SERVER_port" "$config_PAGE_SERVER_port" "$config_WEB_SERVER_port")

# kill all processes that use the ports
for port in "${USED_PORTS[@]}"
do
    # if process is running
    if lsof -Pi :"$port" -sTCP:LISTEN -t >/dev/null ; then
        # get pid
        pid=$(lsof -t -i:"$port")
        # notify
        echo "Killing process $pid to free port $port"
        # kill process
        kill -9 "$pid"
    fi
done

# 2. start servers

if [ "$1" == "--dev" ]; then
    echo "Starting in DEV mode"
    # Page server (python)
    python3 ./page_server/server.py &
    # Print server (node)
    node ./print_server/index.js &
    # Web server (node + vite)
    npm --prefix "./web_server" run dev
else
    echo "Starting in PROD mode"
    python3 ./page_server/server.py &
    npm --prefix "./print_server" run start &
    npm --prefix "./web_server" run start
fi