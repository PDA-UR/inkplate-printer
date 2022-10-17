#! /bin/bash

# Generate a self-signed certificate for the API server.
# This is used for testing purposes only.

openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -sha256 -days 365