from cookiegen.models import Segment, Cloudlet

# rendering
from django.template import loader, RequestContext
from django.http import HttpResponse
from django.shortcuts import render_to_response, get_object_or_404

import datetime
from django.utils import timezone

from scope import generate_cookie_django

def index(request):
    c = RequestContext(request, {
    })
    return render_to_response('cookiegen/index.html', {}, RequestContext(request))

def parse(url):
    import socket
    from urlparse import urlparse
    o = urlparse(url)
    hostname = o.hostname
    return hostname

def search(request):    
    req_location = request.GET['location']
    req_start_time = request.GET['start_time']
    req_end_time = request.GET['end_time']
    entries = Segment.objects.order_by('cloudlet')
    if req_location != '':
        entries = entries.filter(location__icontains=req_location)
    if req_start_time != '':
        entries = entries.filter(start_time__gte=req_start_time)
    if req_end_time != '':
        entries = entries.filter(start_time__lte=req_end_time)
    
    #print entries[0].cloudlet.url_prefix+'\n'

    selected_entries = []
    counter = 0
    for entry in entries:
        counter = counter + 1
        try:
            if request.GET['entry%d' % counter] == 'y':
                selected_entries.append(entry)
        except KeyError:
            pass
    if selected_entries == []:
        selected_entries = entries

    if request.GET['cookie']=="True":
        cookies = []
        urls = []
        entry_prev = None
        for entry in selected_entries:
            if entry_prev != None and entry.cloudlet != entry_prev.cloudlet:
                ip_addr = parse(entry_prev.cloudlet.url_prefix)
                cookies.append(generate_cookie_django(urls,
                                         servers=[ip_addr]))
                urls = []
            urls.append('http://127.0.0.1:5873/mp4video/'+entry.seg_id)
            entry_prev = entry
    
        if entry_prev != None:
            ip_addr = parse(entry_prev.cloudlet.url_prefix)
            print ip_addr
            cookies.append(generate_cookie_django(urls,
                                     servers=[ip_addr]))
    
        cookie = ''.join(cookies) 
        return HttpResponse(cookie, mimetype='application/x-diamond-scope')
    else:
        return render_to_response('cookiegen/index.html', 
            {'entries':entries, 'is_search':True, 
             'req_location':req_location, 'req_start_time':req_start_time, 
             'req_end_time':req_end_time }, RequestContext(request))

