# gigasight/api.py
from tastypie.authorization import Authorization
from tastypie.resources import ModelResource, ALL, ALL_WITH_RELATIONS
from django.contrib.auth.models import User
from tastypie.resources import ModelResource
from tastypie import fields
from gigasight.models import Segment
from gigasight.models import Stream

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
        authorization = Authorization()
        queryset = Segment.objects.all()
        resource_name = 'segment'
        list_allowed_methods = ['get', 'post']

class StreamResource(ModelResource):
    segment = fields.ForeignKey(SegmentResource, 'segment')

    class Meta:
        authorization = Authorization()
        queryset = Stream.objects.all()
        resource_name = 'stream'
        list_allowed_methods = ['get', 'post', 'put']

    def dehydrate(self, bundle):
        bundle.data['test'] = "test argument. TO BE DELETED"
        return bundle

