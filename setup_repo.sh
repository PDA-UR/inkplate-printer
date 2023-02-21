#!/bin/bash

# 1. create symlink webapp -> server to serve the built webapp

# check if dist folder exists, rm if it does
echo "Removing existing webbapp dist folder"
rm -rf clients/webapp/dist

THIS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
SYMLINK_TARGET="$THIS_DIR/servers/api_server/public"

echo "Creating symlink webapp dist -> $SYMLINK_TARGET"
ln -s "$SYMLINK_TARGET" clients/webapp/dist