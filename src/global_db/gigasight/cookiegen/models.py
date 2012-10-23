from django.db import models

class Segments(models.Model):
    uuid = models.CharField(max_length=16)
    location = models.CharField(max_length=50)
    time = models.DateTimeField()
    description = models.CharField(max_length=300)
    ip_addr = models.CharField(max_length=50)
    path = models.CharField(max_length=50)

    def __unicode__(self):
        return self.description
