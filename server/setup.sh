#!/bin/bash

# sets up and installs everything
npm --prefix "./print_server" run setup &&
npm --prefix "./web_server" install &&
pip3 install -r ./page_server/requirements.txt