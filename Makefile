CFLAGS = -g -pthread
LDFLAGS = -lmemcached

TARGET = cmemcached
SRC = connect2memcached.cpp

TARGET:
	g++ $(SRC) $(CFLAGS) $(LDFLAGS) -o $(TARGET)

clean:
	rm $(TARGET)