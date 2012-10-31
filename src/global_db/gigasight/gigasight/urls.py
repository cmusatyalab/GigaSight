from django.conf.urls import patterns, include, url

# Uncomment the next two lines to enable the admin:
from django.contrib import admin
admin.autodiscover()

urlpatterns = patterns('',
    # Examples:
    # url(r'^$', 'gigasight.views.home', name='home'),
    # url(r'^gigasight/', include('gigasight.views.index')),
    url(r'^cookiegen/$', 'cookiegen.views.index'),
    #url(r'^cookiegen/location=(?P<location>[a-z,A-Z]+)/$', 'cookiegen.views.search'),   
    url(r'^cookiegen/search/$', 'cookiegen.views.search'),   
    #url(r'^cookiegen/generate/$', 'cookiegen.views.generate'),

    # Uncomment the admin/doc line below to enable admin documentation:
    # url(r'^admin/doc/', include('django.contrib.admindocs.urls')),

    # Uncomment the next line to enable the admin:
    url(r'^admin/', include(admin.site.urls)),
)
