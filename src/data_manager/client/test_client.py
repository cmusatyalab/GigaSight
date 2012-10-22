#!/usr/bin/env python
#
# Elijah: Cloudlet Infrastructure for Mobile Computing
# Copyright (C) 2011-2012 Carnegie Mellon University
# Author: Kiryong Ha (krha@cmu.edu)
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of version 2 of the GNU General Public License as published
# by the Free Software Foundation.  A copy of the GNU General Public License
# should have been distributed along with this program in the file
# LICENSE.GPL.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
from urlparse import urlparse
import httplib
import json
import sys
import urllib

class CloudletClientError(Exception):
    pass

def get_test(end_point):
    params = urllib.urlencode({})
    headers = {"Content-type":"application/json"}
    end_string = "%s" % (end_point[2])
    print end_point[2]
    print end_point[1]
    # HTTP response
    conn = httplib.HTTPConnection(end_point[1])
    conn.request("GET", end_string, params, headers)
    response = conn.getresponse()
    data = response.read()
    dd = json.loads(data)
    print json.dumps(dd, indent=2)
    conn.close()


def request_new_server(end_point):
    # other data
    s = {
            "user_id":"1"
        }
    params = json.dumps(s)
    headers = {"Content-type":"application/json" }

    conn = httplib.HTTPConnection(end_point[1])
    conn.request("POST", "%s" % end_point[2], params, headers)
    print "request new segment: %s" % (end_point[2])
    response = conn.getresponse()
    data = response.read()
    dd = json.loads(data)
    conn.close()

    print json.dumps(dd, indent=2)


if __name__ == "__main__":
    #get_test(urlparse("http://monsoon.elijah.cs.cmu.edu:5000/api/dm/segment/"))
    request_new_server(urlparse("http://monsoon.elijah.cs.cmu.edu:5000/api/dm/segment/"))

