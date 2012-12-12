#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Requires fping.
"""

try:
	from lxml import etree
except ImportError:
	import xml.etree.cElementTree as etree
import libvirt
import sys,subprocess,os
import StringIO

debug = False
parser = etree.XMLParser()

conn = libvirt.openReadOnly(None)
if conn == None:
    print 'Failed to open connection to the hypervisor'
    sys.exit(1)

domain=conn.lookupByName(sys.argv[1])
desc = etree.fromstring(domain.XMLDesc(0))
macAddr = desc.find("devices/interface[@type='network']/mac").attrib["address"].lower().strip()
if debug:
    print >>sys.stderr,"XMLDesc = %s" % macAddr

output = subprocess.Popen(["arp", "-n"], stdout=subprocess.PIPE).communicate()[0]
lines = [line.split() for line in output.split("\n")[1:]]
if debug:
	print lines
IPaddr = [line[0] for line in lines if (line and (line[2] == macAddr))]
if IPaddr:
	print IPaddr[0]
