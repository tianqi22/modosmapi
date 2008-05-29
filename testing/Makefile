APACHE_CPPFLAGS := -I/usr/include \
		   -I/usr/include/apache2 \
                   -I/usr/include/apr-1.0

.PHONEY: all


#apache.so : apache.c
#	gcc $(APACHE_CPPFLAGS) -shared apache.c  -o mod_helloworld.so

mod_osm.so : mod_osm.cpp
	g++ $(APACHE_CPPFLAGS) -shared mod_osm.cpp -lboost_iostreams -lmysqlclient -o mod_osm.so
