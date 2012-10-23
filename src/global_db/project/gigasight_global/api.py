# gigasight/api.py
import os
from tastypie.authorization import Authorization
from tastypie.resources import ModelResource, ALL, ALL_WITH_RELATIONS
from django.contrib.auth.models import User
from tastypie.resources import ModelResource
from tastypie import fields
from gigasight_global.models import Cloudlet
from gigasight_global.models import Segment

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


class CloudletResource(ModelResource):
    class Meta:
        serializer = PrettyJSONSerializer()
        authorization = Authorization()
        queryset = Cloudlet.objects.all()
        always_return_data = True
        resource_name = 'cloudlet'
        list_allowed_methods = ['get', 'post']
        #excludes = ['seg_id']

class SegmentResource(ModelResource):
    cloudlet = fields.ForeignKey(CloudletResource, 'cloudlet')

    class Meta:
        serializer = PrettyJSONSerializer()
        authorization = Authorization()
        queryset = Segment.objects.all()
        always_return_data = True
        resource_name = 'segment'
        list_allowed_methods = ['get', 'post']
        #excludes = ['seg_id']


