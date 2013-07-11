#!/bin/bash

echo "Make sure global server is running. Otherwise, updater will cause an error: [Enter]"
read user_input

# run local server
cd project
./manage.py runserver 0.0.0.0:5000 > /dev/null 2>&1 &
cd -

# run sync updater
cd client
./update_to_global.py http://monsoon.elijah.cs.cmu.edu:5000 10 > /dev/null 2>&1 &

disown
