import datetime
import hashlib
from django.contrib.auth.models import User
from django.db import models
from uuid import uuid1

class Segment(models.Model):
    seg_id = models.CharField(max_length=36, primary_key=True, default=lambda :str(uuid1()))
    mod_time = models.DateTimeField(default=datetime.datetime.now)
    user_id = models.CharField(max_length=32)
    start_time = models.DateTimeField(default=datetime.datetime.now)
    length = models.IntegerField(default=0);
    location = models.CharField(max_length=256)

    def __unicode__(self):
        return self.seg_id

    def save(self, *args, **kwargs):
        self.mod_time = datetime.datetime.now()
        return super(Segment, self).save(*args, **kwargs)


class Stream(models.Model):
    STREAM_STATUS   = (
            ( 'CRE', 'Created' ),
            ( 'UPD', 'Updating' ),
            ( 'FIN', 'Finished' )
    )

    STREAM_TYPES    = (
            ( 'Video', (
                ( 'VO', 'Original Video' ),
                ( 'VD', 'Denatured Video' )
              )
            ),
            ( 'Audio', (
                ( 'AO', 'Original Audio' ),
                ( 'AD', 'Denatured Audio' )
              )
            ),
            ( 'GPS', (
                ( 'GO', 'Original GPS' ),
                ( 'GD', 'Denatured GPS' )
              )
            )
    )

    STREAM_ACLS     = (
            ( 'PUB', 'Public' ),
            ( 'PRI', 'Private' )
    )

    segment = models.ForeignKey(Segment)
    mod_time = models.DateTimeField(default=datetime.datetime.now)
    path = models.CharField(max_length=1024)
    stream_description = models.CharField(max_length=1024)
    status = models.CharField(max_length=3, choices=STREAM_STATUS)

    def __unicode__(self):
        return self.path

    def save(self, *args, **kwargs):
        if not self.status:
            self.status = 'CRE'

        self.mod_time = datetime.datetime.now()
        return super(Stream, self).save(*args, **kwargs)


class Tag(models.Model):
    segment = models.ForeignKey(Segment)
    mod_time = models.DateTimeField(default=datetime.datetime.now)
    offset = models.FloatField(default=0)
    duration = models.FloatField(default=0)
    tag = models.CharField(max_length=32)
    tag_value = models.CharField(max_length=1024)

    def __unicode__(self):
        if not self.tag_value:
            return self.tag
        else:
            return "%s %s" % (self.tag, self.tag_value)

    def save(self, *args, **kwargs):
        self.mod_time = datetime.datetime.now()
        return super(Tag, self).save(*args, **kwargs)

