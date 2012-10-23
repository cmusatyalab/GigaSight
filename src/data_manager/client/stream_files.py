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

RESOURCE = "/api/dm/stream/"

class CloudletClientError(Exception):
    pass

def get_files(cloudlet_url, resource_url=RESOURCE):
    end_point = urlparse("%s%s" % (cloudlet_url, resource_url))
    params = urllib.urlencode({})
    headers = {"Content-type":"application/json"}
    end_string = "%s" % (end_point[2])
    # HTTP response
    conn = httplib.HTTPConnection(end_point[1])
    conn.request("GET", end_string, params, headers)
    data = conn.getresponse().read()
    response_list = json.loads(data).get('objects', list())
    stream_list = list()
    for item in response_list:
        stream_list.append(item.get("path"))

    conn.close()
    return stream_list


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
    if len(sys.argv) != 2:
        sys.stderr.write("program [Cloudlet URL]\n")
        sys.exit(1)

    sys.stdout.write("Connecting to %s\n" % sys.argv[1])
    stream_paths = get_files(sys.argv[1])

    import pprint
    pprint.pprint(stream_paths)

