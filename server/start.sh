#!/bin/bash

# Starts all servers (print & page server)

# Page server (python)
python3 ./page_server/server.py &
# Print server (node)
node ./print_server/index.js
