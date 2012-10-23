import datetime
from django.db import models

class Cloudlet(models.Model):
    ipaddr = models.IPAddressField()
    url_prefix = models.CharField()

    def __unicode__(self):
        return self.ipaddr

    def save(self, *args, **kwargs):
        return super(Cloudlet, self).save(*args, **kwargs)



class Segment(models.Model):
    seg_id = models.CharField(max_length=36, primary_key=True)
    cloudlet = models.ForeignKey(Cloudlet)
    mod_time = models.DateTimeField(default=datetime.datetime.now)
    user_id = models.CharField(max_length=32)
    start_time = models.DateTimeField(default=datetime.datetime.now)
    length = models.IntegerField(default=0);
    location = models.CharField(max_length=256)
    tag_list = models.TextField()

    def __unicode__(self):
        return self.seg_id

    def save(self, *args, **kwargs):
        self.mod_time = datetime.datetime.now()
        return super(Segment, self).save(*args, **kwargs)


