# AnyPrint - Print Server

This directory contains the code for the print server.

The print server provides the following functionality:

- a printer server in the network, that can be configured via CUPS

## Setup

- `npm install` to install dependencies
- `npm run start` to start the server

## Notes

- whenever a page is printed, the server will try to store the ps file in a queue in `$HOME/.inkplate-printer/print_queue`
  - if the queue directory exists, it is open
  - if the queue directory does not exist, it closed and the printer will not print anything
- the [API server](../api_server/) opens/closes the queue
- the [API server](../api_server/) listens to new files in the queue directory and notify clients when new pages appear
