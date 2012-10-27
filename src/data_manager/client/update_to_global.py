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
import urllib2
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
    def resource_to_segid(resource_url):
        segment_id = [a.strip() for a in resource_url.split("/") if a != ''][-1]
        return segment_id

    sys.stdout.write("Connecting to %s%s\n" % (cloudlet_url, resource_url))
    end_point = urlparse("%s%s" % (cloudlet_url, resource_url))
    params = urllib.urlencode({})
    headers = {"Content-type":"application/json"}
    end_string = "%s?%s" % (end_point[2], end_point[4])
    # HTTP response
    conn = httplib.HTTPConnection(end_point[1])
    conn.request("GET", end_string, params, headers)
    data = conn.getresponse().read()
    response_list = json.loads(data).get('objects', list())
    for item in response_list:
        if item.get('segment'):
            item['seg_id'] = resource_to_segid(item.get('segment'))
            item['segment'] = None
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
    end_string = "%s?%s" % (end_point[2], end_point[4])
    # HTTP response
    conn = httplib.HTTPConnection(end_point[1])
    conn.request("GET", end_string, params, headers)
    data = conn.getresponse().read()
    response_list = json.loads(data).get('objects', list())
    #ret_dic = dict([(resource_to_segid(seg['seg_id']), seg)  for seg in response_list])
    conn.close()
    return response_list

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

def filter_item(items):
    ret_dict = dict()
    filter_key = ['seg_id', 'tag_list', 'start_time', 'length', 'location', \
            'start_time', 'cloudlet', 'mod_time']
    for key in items:
        if key in filter_key:
            ret_dict[key] = items.get(key)
    return ret_dict


def update_to_cloud(cloudlet_url, filter_date):
    global SEGMENT_RESOURCE
    global STREAM_RESOURCE
    global TAG_RESOURCE
    CLOUD_URL = "http://monsoon.elijah.cs.cmu.edu:9000"
    CLOUD_CLOUDLET = "/api/gm/cloudlet/"
    CLOUD_SEGMENT = "/api/gm/segment/"

    # get global information
    cloud_cloudlet = get_files(CLOUD_URL, CLOUD_CLOUDLET)
    cloudlet_dic = dict([(seg['url_prefix'], seg)  for seg in cloud_cloudlet])
    cloud_segment = get_files(CLOUD_URL, CLOUD_SEGMENT)
    cloud_seg_dic = dict([(seg['seg_id'], seg)  for seg in cloud_segment])
    if cloudlet_dic.get(cloudlet_url) == None:
        ret_dict = post(CLOUD_URL, CLOUD_CLOUDLET, {'ipaddr':'0.0.0.0', 'url_prefix':cloudlet_url})
        cloudlet_resource_uri = ret_dict.get("resource_uri", None)
    else:
        cloudlet_resource_uri = cloudlet_dic[cloudlet_url]['resource_uri']

    # get cloudlet information
    filter_date_string = filter_date.strftime("%Y-%m-%d %H:%M:%S").replace(" ", "%20")
    segment_url = "%s?mod_time__gte=%s" % (SEGMENT_RESOURCE, filter_date_string)
    tag_url = "%s?mod_time__gte=%s" % (TAG_RESOURCE, filter_date_string)
    stream_url = "%s?mod_time__gte=%s&status=FIN" % \
            (STREAM_RESOURCE, filter_date_string)
    segment_resource = get_segment(cloudlet_url, segment_url)

    #1. update segment update
    for segment_item in segment_resource:
        if cloud_seg_dic.get(segment_item['seg_id'], None) == None:
            # POST for new segment
            segment_item['cloudlet'] = cloudlet_resource_uri
            ret_dict = post(CLOUD_URL, CLOUD_SEGMENT, segment_item)
        else:
            segment_resource_uri = cloud_seg_dic.get(segment_item['seg_id'])['resource_uri']
            segment_item['cloudlet'] = cloudlet_resource_uri
            ret_dict = put(CLOUD_URL, segment_resource_uri, segment_item)

    #2. update stream
    stream_resource = get_files(cloudlet_url, stream_url)
    for stream_item in stream_resource:
        if cloud_seg_dic.get(stream_item['segment'], None) == None:
            # POST for new segment
            stream_item['cloudlet'] = cloudlet_resource_uri
            ret_dict = post(CLOUD_URL, CLOUD_SEGMENT, filter_item(stream_item))
        else:
            stream_resource_uri = cloud_seg_dic.get(stream_item['seg_id'])['resource_uri']
            stream_item['cloudlet'] = cloudlet_resource_uri
            ret_dict = put(CLOUD_URL, stream_resource_uri, filter_item(stream_item))

    #3. update tag
    tag_resource = get_files(cloudlet_url, tag_url)
    # TODO: this will overwrite previous tag
    # if we try to read tags from global, tag will continuous become longer
    prev_tag_list = dict()
    for tag_item in tag_resource:
        seg_id = tag_item['seg_id']
        prev_tag = prev_tag_list.get(seg_id, None)
        if prev_tag != None:
            prev_tag_list[seg_id] = "%s;%s" % (prev_tag, tag_item.get('tag'))
        else:
            prev_tag_list[seg_id] = tag_item.get('tag')

    for tag_item in tag_resource:
        if tag_item.get('tag'):
            tag_item['tag_list'] = prev_tag_list[tag_item['seg_id']]

        if cloud_seg_dic.get(tag_item['seg_id'], None) == None:
            # POST for new segment
            tag_item['cloudlet'] = cloudlet_resource_uri
            ret_dict = post(CLOUD_URL, CLOUD_SEGMENT, filter_item(tag_item))
        else:
            tag_resource_uri = cloud_seg_dic.get(tag_item['seg_id'])['resource_uri']
            tag_item['cloudlet'] = cloudlet_resource_uri
            ret_dict = put(CLOUD_URL, tag_resource_uri, filter_item(tag_item))


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.stderr.write("program [Cloudlet URL] [interval in sec]\n")
        sys.exit(1)

    cloudlet_url = sys.argv[1]
    interval = int(sys.argv[2])
    filter_date = datetime.strptime("2011-10-26 21:00:35", "%Y-%m-%d %H:%M:%S")
    while True:
        print "Update starting at %s" % datetime.now()
        update_to_cloud(cloudlet_url, filter_date)
        filter_date = datetime.now()
        time.sleep(interval)
        print ""


