# memcached_client

A client for memcached in C++ using modified libmemcached API

Main functions: read the cache trace & send requests to memcached server & collect the returned results

- memcached: https://memcached.org/

- libmemcached: https://libmemcached.org/libMemcached.html

- cache trace: https://github.com/twitter/cache-trace

## Usage

- build cmemcached simply by `make`

- run memcached instance

- run cmemcached with tracepath in command line

```
./cmemcached tracepath1 tracepath2 ...
```
