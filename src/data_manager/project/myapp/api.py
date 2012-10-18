# myapp/api.py
from tastypie.authorization import Authorization
from tastypie.resources import ModelResource, ALL, ALL_WITH_RELATIONS
from django.contrib.auth.models import User
from tastypie.resources import ModelResource
from tastypie import fields
from myapp.models import Entry

from tempfile import NamedTemporaryFile
from django.core.serializers import json
from django.utils import simplejson
from tastypie.serializers import Serializer

NFS_ROOT = "/tmp"

class PrettyJSONSerializer(Serializer):
    json_indent = 2
    def to_json(self, data, options=None):
        options = options or {}
        data = self.to_simple(data, options)
        return simplejson.dumps(data, cls=json.DjangoJSONEncoder,
                sort_keys=True, ensure_ascii=False, indent=self.json_indent)

class UserResource(ModelResource):
    class Meta:
        queryset = User.objects.all()
        resource_name = 'user'

class EntryResource(ModelResource):
    user = fields.ForeignKey(UserResource, 'user')

    class Meta:
        queryset = Entry.objects.all()
        resource_name = 'entry'
        authorization = Authorization()
        filtering = {
                'user': ALL_WITH_RELATIONS,
                'pub_date': ['exact', 'lt', 'lte', 'gte', 'gt'],
        }

