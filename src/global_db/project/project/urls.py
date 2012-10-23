from django.conf.urls import patterns, include, url
from myapp.api import EntryResource, UserResource
from gigasight_global.api import CloudletResource, SegmentResource
from tastypie.api import Api


# Uncomment the next two lines to enable the admin:
from django.contrib import admin
admin.autodiscover()

v1_api = Api(api_name='v1')
v1_api.register(UserResource())
v1_api.register(EntryResource())

gm = Api(api_name='gm')
gm.register(CloudletResource())
gm.register(SegmentResource())


urlpatterns = patterns('',
    # Examples:
    # url(r'^$', 'project.views.home', name='home'),
    # url(r'^project/', include('project.foo.urls')),

    # Uncomment the admin/doc line below to enable admin documentation:
    # url(r'^admin/doc/', include('django.contrib.admindocs.urls')),

    # Uncomment the next line to enable the admin:
    url(r'^admin/', include(admin.site.urls)),
    
    #(r'^blog/', include('myapp.urls')),
    (r'^test/', include(v1_api.urls)),
    (r'^api/', include(gm.urls)),
)
