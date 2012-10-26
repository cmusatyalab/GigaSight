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
import pprint
import datetime
import time
from datetime import datetime
from dateutil import parser

STREAM_RESOURCE = "/api/dm/stream/"
TAG_RESOURCE = "/api/dm/tag/"
SEGMENT_RESOURCE = "/api/dm/segment/"

GLOBAL_CLOUDLET_RESOURCE = "/api/gm/cloudlet/"
GLOBAL_SEGMENT_RESOURCE = "/api/gm/segment/"

class CloudletClientError(Exception):
    pass

def get_files(cloudlet_url, resource_url):
    sys.stdout.write("Connecting to %s%s\n" % (cloudlet_url, resource_url))
    end_point = urlparse("%s%s" % (cloudlet_url, resource_url))
    params = urllib.urlencode({})
    headers = {"Content-type":"application/json"}
    end_string = "%s" % (end_point[2])
    # HTTP response
    conn = httplib.HTTPConnection(end_point[1])
    conn.request("GET", end_string, params, headers)
    data = conn.getresponse().read()
    response_list = json.loads(data).get('objects', list())
    conn.close()
    return response_list


def convert_to_dict(response_list):
    def resource_to_segid(resource_url):
        segment_id = [a.strip() for a in resource_url.split("/") if a != ''][-1]
        return segment_id

    ret_dic = dict([(resource_to_segid(seg['segment']), seg) for seg in response_list])
    return ret_dic


def get_segment(cloudlet_url, resource_url):
    def resource_to_segid(resource_url):
        segment_id = [a.strip() for a in resource_url.split("/") if a != ''][-1]
        return segment_id

    sys.stdout.write("Connecting to %s%s\n" % (cloudlet_url, resource_url))
    end_point = urlparse("%s%s" % (cloudlet_url, resource_url))
    params = urllib.urlencode({})
    headers = {"Content-type":"application/json"}
    end_string = "%s" % (end_point[2])
    # HTTP response
    conn = httplib.HTTPConnection(end_point[1])
    conn.request("GET", end_string, params, headers)
    data = conn.getresponse().read()
    response_list = json.loads(data).get('objects', list())
    ret_dic = dict([(resource_to_segid(seg['seg_id']), seg)  for seg in response_list])
    conn.close()
    return ret_dic

def post(global_url, resource_url, json_string):
    print("Posting to %s%s" % (global_url, resource_url))
    end_point = urlparse("%s%s" % (global_url, resource_url))

    params = json.dumps(json_string)
    headers = {"Content-type":"application/json" }

    conn = httplib.HTTPConnection(end_point[1])
    conn.request("POST", "%s" % end_point[2], params, headers)
    response = conn.getresponse()
    data = response.read()
    dd = json.loads(data)
    conn.close()

    return dd


def put(global_url, resource_url, json_string):
    print("Updating to %s%s" % (global_url, resource_url))
    end_point = urlparse("%s%s" % (global_url, resource_url))

    params = json.dumps(json_string)
    headers = {"Content-type":"application/json" }

    conn = httplib.HTTPConnection(end_point[1])
    conn.request("PUT", "%s" % end_point[2], params, headers)
    response = conn.getresponse()
    data = response.read()
    dd = json.loads(data)
    conn.close()

    return dd


def filter_out(resource):
    ret_list = []
    filter_key = ['tag', 'start_time', 'length', 'location', 'start_time']
    for segment_id, values in resource.iteritems():
        filter_item = dict()
        mod_time = parser.parse(values['mod_time'])
        filter_item['seg_id'] = segment_id
        filter_item['mod_time'] = values['mod_time']
        for item in filter_key:
            if values.get(item) != None:
                filter_item[item] = values.get(item)

        ret_list.append(filter_item)
    return ret_list

def update_to_cloud(cloudlet_url):
    global SEGMENT_RESOURCE
    global STREAM_RESOURCE
    global TAG_RESOURCE
    CLOUD_URL = "http://monsoon.elijah.cs.cmu.edu:9000"
    CLOUD_CLOUDLET = "/api/gm/cloudlet/"
    CLOUD_SEGMENT = "/api/gm/segment/"

    # get cloudlet information
    segment_url = "%s" % (SEGMENT_RESOURCE)
    tag_url = "%s" % TAG_RESOURCE
    stream_url = "%s?status=%s" % (STREAM_RESOURCE, 'FIN')
    segment_resource = get_segment(cloudlet_url, segment_url)
    stream_resource = convert_to_dict(get_files(cloudlet_url, stream_url))
    tag_resource = convert_to_dict(get_files(cloudlet_url, tag_url))
    tag_resource.update(segment_resource)
    tag_resource.update(stream_resource)
    tag_filtered = filter_out(tag_resource)

    # get cloud information
    cloud_cloudlet = get_files(CLOUD_URL, CLOUD_CLOUDLET)
    cloudlet_dic = dict([(seg['url_prefix'], seg)  for seg in cloud_cloudlet])
    cloud_segment = get_files(CLOUD_URL, CLOUD_SEGMENT)
    cloud_seg_dic = dict([(seg['seg_id'], seg)  for seg in cloud_segment])
    if cloudlet_dic.get(cloudlet_url) == None:
        ret_dict = post(CLOUD_URL, CLOUD_CLOUDLET, {'ipaddr':'0.0.0.0', 'url_prefix':cloudlet_url})
        cloudlet_resource_uri = ret_dict.get("resource_uri", None)
    else:
        cloudlet_resource_uri = cloudlet_dic[cloudlet_url]['resource_uri']

    for tag_item in tag_filtered:
        if tag_item.get('tag'):
            tag_item['tag_list'] = tag_item.get('tag')

        if cloud_seg_dic.get(tag_item['seg_id'], None) == None:
            # POST for new segment
            tag_item['cloudlet'] = cloudlet_resource_uri
            if not tag_item.get('lengh'):
                tag_item['length'] = 0
            if not tag_item.get('location'):
                tag_item['location'] = ''
            if not tag_item.get('start_time'):
                tag_item['start_time'] = u'2012-10-25T20:24:39+00:00'
            ret_dict = post(CLOUD_URL, CLOUD_SEGMENT, tag_item)
        else:
            segment_resource_uri = cloud_seg_dic.get(tag_item['seg_id'])['resource_uri']
            tag_item['cloudlet'] = cloudlet_resource_uri
            ret_dict = put(CLOUD_URL, segment_resource_uri, tag_item)
            '''
            pprint.pprint(tag_item)
            print "put"
            sys.exit(1)
            '''

    #pprint.pprint(tag_filtered)
    #pprint.pprint(stream_resource)

if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.stderr.write("program [Cloudlet URL] [interval in sec]\n")
        sys.exit(1)

    cloudlet_url = sys.argv[1]
    interval = int(sys.argv[2])

    while True:
        print "Update starting at %s" % datetime.now()
        update_to_cloud(cloudlet_url)
        time.sleep(interval)
        print ""


