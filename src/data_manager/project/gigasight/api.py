# gigasight/api.py
from tastypie.authorization import Authorization
from tastypie.resources import ModelResource, ALL, ALL_WITH_RELATIONS
from django.contrib.auth.models import User
from tastypie.resources import ModelResource
from tastypie import fields
from gigasight.models import Segment
from gigasight.models import Stream
from tempfile import NamedTemporaryFile

from django.core.serializers import json
from django.utils import simplejson
from tastypie.serializers import Serializer

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
        excludes = ['pub_date', 'id']


class StreamResource(ModelResource):
    segment = fields.ForeignKey(SegmentResource, 'segment')

    class Meta:
        serializer = PrettyJSONSerializer()
        authorization = Authorization()
        queryset = Stream.objects.all()
        always_return_data = True
        resource_name = 'stream'
        list_allowed_methods = ['get', 'post', 'put']
        excludes = ['container', 'id', 'pub_date', 'pk']
        
    def obj_create(self, bundle, request=None, **kwargs):
        tmp_path = NamedTemporaryFile(prefix="stream-", delete=False)
        return super(StreamResource, self).obj_create(bundle, request, file_path=tmp_path.name)

    '''
    def dehydrate(self, bundle):
        bundle.data['test'] = "test argument. TO BE DELETED"
        return bundle
    '''

