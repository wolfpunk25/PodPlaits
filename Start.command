#!/bin/zsh
cd -- "${0:A:h}"
/usr/bin/python3 bridge.py
read -r '?Press Return to close.'
