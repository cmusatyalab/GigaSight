#!/bin/bash
cd project
./manage.py runserver 0.0.0.0:9000 > /dev/null 2>&1 &
