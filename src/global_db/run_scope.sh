#!/bin/bash
cd gigasight
./manage.py runserver 0.0.0.0:9001 > /dev/null 2>&1 &
