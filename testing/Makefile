APACHE_CPPFLAGS := -I/usr/include \
		   -I/usr/include/apache2 \
                   -I/usr/include/apr-1.0

.PHONEY: all install

all : mod_osm.so test

mod_osm.so : mod_osm.cpp test.cpp test.h
	g++ $(APACHE_CPPFLAGS) $^ -shared -lboost_iostreams -lmysqlclient $(OUTPUT_OPTION)

test : test.cpp test.h
	g++ $(APACHE_CPPFLAGS) $^ -lboost_iostreams -lmysqlclient $(OUTPUT_OPTION)

install : mod_osm.so
	cp mod_osm.so /usr/lib/apache2/modules
	apache2ctl stop
	apache2ctl start

