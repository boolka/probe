# Probe

## Purpose

Simplest possible solution to check service availability.
Can be used in docker `HEALTHCHECK` instruction (check `docker/Dockerfile.test` folder for example).

## Usage

Usage: probe [...OPTIONS] [HOST]

Options:
  - -s, --service - service to connect to
  - -p, --port - port to connect to
  - -r, --retry - retry count
  - -t, --timeout - timeout between retryings
  - -h, --help - this help message
  - -v, --version - current application version

Examples:
  - `probe --port=80 1.2.3.4`
  - `probe --service=http localhost`
  - `probe --port=8080 localhost`
  - `probe --timeout=3 --retry=5 --service=https example.com`

## Description

This program uses GNU Name Service Switch (NSS) mechanics to determine the `HOST` to resolve.

The first thing probe is doing is trying to determine the host. When it is unavailable then it failures.
After this probe will try to connect to host that many times that `retry` & `timeout` tells.
On success probe will return `0` and `1` on failure.

Don't use the `0.0.0.0` address.

## Requirements

- libc

## TODO

- Check all addresses in `struct hostent`.
- Only `tcp` protocol for now is available.
