# AnyPrint - API Server

This directory contains the code for the API server.

The API server provides the following functionality:

- Websocket API for clients to communicate with the server
- REST API for clients to download pages
- Webserver for the webapp

## Setup

- `npm install` to install dependencies
- `npm run dev` to start the development server (restarts on file changes)
- `npm run start` to start the production server
- for development: generate a self-signed certificate with `npm run generate-cert`
  - the certificate is stored in `ssl/`

## Notes

- the `public/` directory is served by the webserver
  - the [Webapp](../../clients/webapp) dist directory is symlinked to `public/`, which means that whenever the webapp is built, the output will appear in it
- the file and folder structure in `routes/` is mirrored in the REST/Websocket API:
  - e.g. `routes/rest/example.js` is available at `/api/example`
    - (`/rest` is replaced with `/api`)
  - e.g. `routes/socket/example.js` is available at `example`
    - (`/ws` is removed)
