import datetime
from django.contrib.auth.models import User
from django.db import models

NFS_ROOT = "/mnt/"

class Segment(models.Model):
    pub_date = models.DateTimeField(default=datetime.datetime.now)
    user_id = models.CharField(max_length=256)

    def __unicode__(self):
        return self.user_id

    def save(self, *args, **kwargs):
        return super(Segment, self).save(*args, **kwargs)


class Stream(models.Model):
    STREAM_CREATED  = 1
    STREAM_UPDATING = 2
    STREAM_FINISH   = 3

    segment = models.ForeignKey(Segment)
    pub_date = models.DateTimeField(default=datetime.datetime.now)
    file_path = models.CharField(max_length=1024)
    container = models.CharField(max_length=100)
    tag = models.CharField(max_length = 1024)
    status = models.IntegerField()

    def __unicode__(self):
        return self.path

    def save(self, *args, **kwargs):
        if not self.status:
            self.status = Stream.STREAM_CREATED

        return super(Stream, self).save(*args, **kwargs)

