# memcached_client

A client for memcached in C++ using modified libmemcached API

Main functions: read the cache trace & send requests to memcached server & collect the returned results

- memcached: https://memcached.org/

- libmemcached: https://libmemcached.org/libMemcached.html

- cache trace: https://github.com/twitter/cache-trace

## Usage

- install libmemcached dependency

- build cmemcached

    - for single-tenant: `make single`
    - for multi-tenant: `make multi`

- run memcached instance(s)

    **Note:** for multi-tenant situations, the running ports for multiple memcached instances should be incremented consecutively from a particular starting port

- run cmemcached with tracepath in command line

    ```
    ./cmemcached tracepath1 tracepath2 ...
    ```
    **Note:** for multi-tenant situations, the macro `PORT` in source code corresponds to the starting port of the several memcached instances
