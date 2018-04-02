#!/usr/bin/python2

import sys
import urllib2,urllib
from tioj_url import tioj_url, tioj_key

submission_id = sys.argv[1]

message = urllib.urlencode({"message":sys.stdin.read()})

respond = urllib2.urlopen(tioj_url+"/fetch/write_message?key="+tioj_key+"&sid="+submission_id,message)
