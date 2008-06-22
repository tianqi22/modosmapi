#!/usr/bin/python
from __future__ import with_statement

import urllib2
import time

def t():
    lt = time.localtime()
    return (lt[3], lt[4], lt[5])

def writeFile( filename, url ):
    print url
    start = time.time()
    with open( filename, 'w' ) as f:
        f.write( urllib2.urlopen( url ).read() )
    end = time.time()
    print 'Fetch time:', (end-start)

wolvercotebbox = ( -1.3163, 51.7574, -1.2684, 51.7782 )
#hhbbox = ( -0.5092, 51.7387, -0.4132, 51.7803 )

hhbbox = ( -1.3163, 51.7387, -0.4132, 51.7803 )

url = 'http://localhost/osm/api/0.5/map?bbox=%f,%f,%f,%f' % hhbbox
writeFile( 'file1.xml', url )

url = 'http://localhost:3000/api/0.5/map?bbox=%f,%f,%f,%f' % hhbbox
writeFile( 'file2.xml', url )
