#
# STF filter, a searchlet for the OpenDiamond platform
#
# Copyright (c) 2012 Carnegie Mellon University
#
# This filter is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 2.
#
# This filter is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License.
# If not, see <http://www.gnu.org/licenses/>.
#

from PIL import Image, ImageColor
from collections import namedtuple
import itertools
import json
import numpy as np
import pymorph
import sys
import tempfile
import zipfile
from multiprocessing import Pool, Queue, Manager 
from urlparse import urlparse
import httplib
import urllib
import time
import datetime
import glob

from ..forest import normalize
from ..image import convert_to_cielab, convert_to_primaries, make_palette, output_colormap, LabeledImage
from ..svm_ import load_model, make_vectors, svm_predict
from .train_tree1 import load_forest_from_classifier, load_ndarray, DistImage

STRIDE0 = 1
STRIDE1 = 1
ALPHA = 0.5
SCORE_THRESHOLD = 0.7

CLOUDLET_RESOURCE = "http://75.146.76.236:5000"
SEGMENT_RESOURCE = "/api/dm/segment/"
STREAM_RESOURCE = "/api/dm/stream/"
TAG_RESOURCE = "/api/dm/tag/"

CLASSES = [
        "unlabeled", 
        "building", 
        "grass", 
        "tree", 
        "cow", 
        "sheep", 
        "sky", 
        "aeroplane", 
        "water", 
        "face", 
        "car", 
        "bicycle", 
        "flower", 
        "sign", 
        "bird", 
        "book", 
        "chair", 
        "road", 
        "cat", 
        "dog", 
        "body", 
        "boat"
    ]

ImageLabelTuple = namedtuple('ImageLabelTuple', ['image', 'label'])

def load_from_classifier(classifier):
    forest0 = load_forest_from_classifier(classifier, 'forest0.npz')
    hist0 = load_ndarray(classifier, 'hist0.npy')
    prior = np.true_divide(hist0[0].sum(axis=0), hist0[0].sum())
    hist0 = np.insert(normalize(hist0), 0, 0, axis=0)

    forest1 = load_forest_from_classifier(classifier, 'forest1.npz')
    hist1 = load_ndarray(classifier, 'hist1.npy')
    hist1 = np.insert(normalize(hist1), 0, 0, axis=0)

    svmmodels = []
    try:
        training_bosts = normalize(load_ndarray(classifier, 'bosts.npy')).T

        NLABELS = hist0.shape[2]
        for i in range(1, NLABELS):
            model = classifier.read('svmmodel%d' % i)
            tmp = tempfile.NamedTemporaryFile()
            tmp.write(model)
            tmp.flush()
            svmmodels.append(load_model(tmp.name))
            tmp.close()
    except KeyError:
        training_bosts = None
    return forest0, hist0, forest1, hist1, training_bosts, svmmodels, prior

def get_list(cloudlet_url, resource_url):
    #print "connecting to %s%s" % (cloudlet_url, resource_url)
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

def get_entry(cloudlet_url, resource_url):
    #print "connecting to %s%s" % (cloudlet_url, resource_url)
    end_point = urlparse("%s%s" % (cloudlet_url, resource_url))
    params = urllib.urlencode({})
    headers = {"Content-type":"application/json"}
    end_string = "%s" % (end_point[2])
    # HTTP response
    conn = httplib.HTTPConnection(end_point[1])
    conn.request("GET", end_string, params, headers)
    data = conn.getresponse().read()
    response_entry = json.loads(data)

    conn.close()
    return response_entry

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

def split_frame_list(ori_frame_list, split_threshold):
    frame_list = []
    score_list = []
    ori_frame_list.sort()
    for frame_score in ori_frame_list:
        frame_list.append(frame_score[0])
        score_list.append(frame_score[1])
    l = len(ori_frame_list)
    start_idx = 0
    end_idx = 1
    frame_list_split = []
    max = score_list[0]
    while end_idx < l:
        if frame_list[end_idx] - frame_list[end_idx - 1] > split_threshold:
            frame_list_split.append((frame_list[start_idx:end_idx], "%d" % (max * 100)))
            max = 0
            start_idx = end_idx
        if score_list[end_idx] > max:
            max = score_list[end_idx]
        end_idx += 1
    frame_list_split.append((frame_list[start_idx:end_idx], "%d" % (max * 100)))
    return frame_list_split

def convert_time(str_time):
    d, t = str_time.split('+')[0].split('T')
    yy, mm, dd = d.split('-')
    d = datetime.date(int(yy), int(mm), int(dd))
    hh, mi, ss = t.split(':')
    t = datetime.time(int(hh), int(mi), int(ss))
    dt = datetime.datetime.combine(d, t)
    return dt

def calculate_class(arg):
    #print "Classifying/segmenting starts..."
    q = arg[0]
    frame_counter = arg[1]
    name = "indexing"
    image = Image.open(name + "%d.png" % (frame_counter % processes))
    #image = image.resize((640, 360), Image.ANTIALIAS) 
    rgb = np.asarray(image)
    Lab = convert_to_cielab(rgb)
    pcv = convert_to_primaries(rgb, None)
    image = ImageLabelTuple(np.dstack((Lab, rgb, pcv)), None)

    print "Classifying/segmenting", name, frame_counter
    leafimage0 = forest0.compute_leafimage(image, STRIDE0)

    #print "Computing SVM classification"
    bost = leafimage0.compute_bost()
    bost = normalize(bost).T
    vector = make_vectors(bost, training_bosts)
    svmresults = [0] + [ svm_predict(m, vector[0])
                         for m in svmmodels ]
    ILP = np.power(svmresults, ALPHA)
    print [(CLASSES[class_index], "%.02f" % score) for class_index, score in enumerate(ILP) if score > SCORE_THRESHOLD]
    q.put([frame_counter, ILP])
    print

def main():
    import argparse
    import logging
    import os
    import yaml
    import cv

    global processes
    global forest0, svmmodels, training_bosts, hist0

    parser = argparse.ArgumentParser()
    parser.add_argument('classifier')
    parser.add_argument('cores', type=int, help='Number of processes of paralellism')
    parser.add_argument('--postprocess', action="store_true",
                        help='Run postprocessing, close blobs and remove noise')
    args = parser.parse_args()

    logging.basicConfig(level=logging.WARNING,
                        format="%(asctime)s - %(message)s")

    classifier = zipfile.ZipFile(args.classifier)
    forest0, hist0, forest1, hist1, training_bosts, svmmodels, prior = \
        load_from_classifier(classifier)
    classifier.close()
    
    processes = args.cores
    pool = Pool(processes = processes)

    KEY_FRAME_PERIOD = 2 # in seconds
    q = Manager().Queue()
    total_frame = 0

    new_flag = True
    while True:
        if not new_flag:
            print "wait..."
            time.sleep(1)
        stream_list = get_list(CLOUDLET_RESOURCE, STREAM_RESOURCE)
        #print stream_list
        new_flag = False
        prev_stream = None
        for stream in stream_list:
            if stream.get("stream_description").find("denatured") == -1 or stream.get("stream_description").find("video") == -1 or stream.get("stream_description").find("ppstf") != -1:
                prev_stream = stream
                continue
            ILP_max = [] 
            for i in xrange(len(CLASSES)):
                ILP_max.append(0)
            ILP_list = []
            for i in xrange(len(CLASSES)):
                ILP_list.append([])
            path, name = stream.get("path").replace("mnt", "cloudletstore").rsplit('/', 1)
            print os.path.join(path, name)
            path_p, name_p = prev_stream.get("path").replace("mnt", "cloudletstore").rsplit('/', 1)
            print os.path.join(path_p, name_p)
            statinfo = os.stat(os.path.join(path_p, name_p))      
            prev_stream = stream
           
            if statinfo.st_size == 0:
                continue

            new_flag = True
            frame_rate = 30
     
            capture = cv.CaptureFromFile(os.path.join(path, name))
            frame_rate = cv.GetCaptureProperty(capture, cv.CV_CAP_PROP_FPS)
            total_frames = cv.GetCaptureProperty(capture, cv.CV_CAP_PROP_FRAME_COUNT)
            frame = cv.QueryFrame(capture)
            print frame_rate, total_frames
            print capture
   
            start_time = time.time()
            
            key_frame_counter_base = 0    
            while frame:
                process_num = 0
                while frame:
                    cv.SaveImage("indexing" + "%d.png" % process_num, frame)
                    for i in xrange(int(KEY_FRAME_PERIOD * frame_rate)):
                        frame = cv.QueryFrame(capture)
                    process_num += 1
                    if process_num == processes:
                        break
                pool.map(calculate_class, [(q, x) for x in xrange(key_frame_counter_base, key_frame_counter_base + process_num)])
          
            while not q.empty():
                q_entry = q.get()
                key_frame_counter = q_entry[0]
                ILP = q_entry[1]
                for class_index, score in enumerate(ILP): 
                    if score > SCORE_THRESHOLD:
                        ILP_list[class_index].append((key_frame_counter * int(KEY_FRAME_PERIOD * frame_rate) + 1, score))
                        print (CLASSES[class_index], "%.02f" % score),
                    if score > ILP_max[class_index]:
                        ILP_max[class_index] = score
                print

                key_frame_counter_base += process_num

            for class_index, frame_list in enumerate(ILP_list):
                if not frame_list:
                    continue
                frame_list_split = split_frame_list(frame_list, int(KEY_FRAME_PERIOD * frame_rate) * 2)
                for frame_list, local_max_score in frame_list_split: 
                    tag_entry = {}
                    tag_entry["tag"] = CLASSES[class_index] + ":%d" % (ILP_max[class_index] * 100)
                    tag_entry["tag_value"] = local_max_score
                    tag_entry["offset"] = frame_list[0] / frame_rate
                    tag_entry["duration"] = (frame_list[-1] - frame_list[0]) / frame_rate
                    tag_entry["segment"] = stream.get("segment")
                    print tag_entry
                    ret_dict = post(CLOUDLET_RESOURCE, TAG_RESOURCE, tag_entry)        
        
            if stream.get("stream_description").find("ppstf") == -1:
                stream_entry = {"stream_description": stream.get("stream_description") + "ppstf;"}
                ret_dict = put(CLOUDLET_RESOURCE, stream.get("resource_uri"), stream_entry)        
           
            elapse_time = time.time() - start_time
            print "max score:"
            print [(CLASSES[class_index], "%.02f" % score) for class_index, score in enumerate(ILP_max)]
            print "total time: %.2f, key frames: %d, frame per sec: %.2f" \
               % (elapse_time, key_frame_counter_base, key_frame_counter_base / elapse_time)
            print
            
            
if __name__ == '__main__':
    main()

