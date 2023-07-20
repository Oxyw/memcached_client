CFLAGS = -g -pthread
LDFLAGS = -lmemcached

TARGET_single = cmemcached
TARGET_multi = cmemcached_multi

SRC_single = connect2memcached.cpp
SRC_multi = connect2memcached_multi.cpp

single:
	g++ $(SRC_single) $(CFLAGS) $(LDFLAGS) -o $(TARGET_single)

multi:
	g++ $(SRC_multi) $(CFLAGS) $(LDFLAGS) -o $(TARGET_multi)

clean:
	rm $(TARGET_single) $(TARGET_multi)