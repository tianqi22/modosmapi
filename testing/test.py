import urllib2


wolvercotebbox = ( -1.3163, 51.7574, -1.2684, 51.7782 )
hhbbox = ( -0.5092, 51.7387, -0.4132, 51.7804 )

url = 'http://www.openstreetmap.org/api/0.5/map?bbox=%f,%f,%f,%f' % hhbbox

print urllib2.urlopen( url ).read()
