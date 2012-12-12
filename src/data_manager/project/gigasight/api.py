# gigasight/api.py
import os
from tastypie.authorization import Authorization
from tastypie.resources import ModelResource, ALL, ALL_WITH_RELATIONS
from django.contrib.auth.models import User
from tastypie.resources import ModelResource
from tastypie import fields
from gigasight.models import Segment
from gigasight.models import Stream
from gigasight.models import Tag
from tempfile import NamedTemporaryFile

from django.core.serializers import json
from django.utils import simplejson
from tastypie.serializers import Serializer


NFS_ROOT = "/mnt/segments/"

class PrettyJSONSerializer(Serializer):
    json_indent = 2
    def to_json(self, data, options=None):
        options = options or {}
        data = self.to_simple(data, options)
        return simplejson.dumps(data, cls=json.DjangoJSONEncoder,
                sort_keys=True, ensure_ascii=False, indent=self.json_indent)


class SegmentResource(ModelResource):
    class Meta:
        serializer = PrettyJSONSerializer()
        authorization = Authorization()
        queryset = Segment.objects.all()
        always_return_data = True
        resource_name = 'segment'
        list_allowed_methods = ['get', 'post']
        #excludes = ['seg_id']
        filtering = {"mod_time":ALL}


class StreamResource(ModelResource):
    segment = fields.ForeignKey(SegmentResource, 'segment')

    class Meta:
        serializer = PrettyJSONSerializer()
        authorization = Authorization()
        queryset = Stream.objects.all()
        always_return_data = True
        resource_name = 'stream'
        list_allowed_methods = ['get', 'post', 'put']
        #excludes = ['container', 'id', 'pub_date', 'pk']
        filtering = {"mod_time":ALL, "segment_id":('exact'), "status":ALL}
        
    def obj_create(self, bundle, request=None, **kwargs):
        segment = bundle.data.get("segment", None)
        if segment:
            segment_id = [a.strip() for a in segment.split("/") if a != ''][-1]
            stream_root = os.path.join(NFS_ROOT, segment_id)
            if not os.path.exists(stream_root):
                os.makedirs(stream_root)
        else:
            stream_root = os.path.join(NFS_ROOT)
       
        #import pdb; pdb.set_trace()
        tmp_path = NamedTemporaryFile(prefix="stream-", delete=False, dir=stream_root)
        return super(StreamResource, self).obj_create(bundle, request, path=tmp_path.name)

    '''
    def dehydrate(self, bundle):
        bundle.data['test'] = "test argument. TO BE DELETED"
        return bundle
    '''

class TagResource(ModelResource):
    segment = fields.ForeignKey(SegmentResource, 'segment')

    class Meta:
        serializer = PrettyJSONSerializer()
        authorization = Authorization()
        queryset = Tag.objects.all()
        always_return_data = True
        resource_name = 'tag'
        list_allowed_methods = ['get', 'post', 'put']
        excludes = ['id', 'pk']
        filtering = {"mod_time":ALL, "segment_id":('exact'), "tag":ALL, "segment":ALL}


