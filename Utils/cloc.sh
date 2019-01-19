#!/bin/sh

time find ../Core -type f -not -wholename "*3rdparty*" | ./cloc.py $@
