#include <iostream>
#include <cstring>
#include <fstream>
#include <sstream>
#include <libmemcached/memcached.h> // "/usr/local/libmemcached-1.0.18/"
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <queue>
#include <sys/time.h>
#include <vector>
#include <stdexcept>

#define IP "127.0.0.1"
#define PORT 11211
#define BUFS 4096
#define SET_DB 16

const int N = 10;
const int endtime = 172800;  // end timestamp
long long offset[N], fsize[N];
char *truthful_work_load[N];
char buf[BUFS] = {0};
size_t reqs[N], hits[N], misses[N];

std::string file_stats_slabs = "stats_slabs" + std::to_string(PORT);
std::string file_result = "result" + std::to_string(PORT);

enum ERROR_CNT {
    EX_CONFIG = -2,
    EX_INPUT,
    EX_RUNTIME
};

struct cmp {
public:
    bool operator()(std::pair<uint32_t, uint32_t> a, std::pair<uint32_t, uint32_t> b) {
        return a.first > b.first;
    }
};

void generate(char *trace_path, size_t f)
{
    int fd;
    struct stat st;

    if ((fd = open(trace_path, O_RDONLY)) < 0)
    {
        printf("Unable to open '%s', %s\n", trace_path, strerror(errno));
        exit(EX_CONFIG);
    }
    if ((fstat(fd, &st)) < 0)
    {
        close(fd);
        printf("Unable to fstat '%s', %s\n", trace_path, strerror(errno));
        exit(EX_CONFIG);
    }

    std::cout << trace_path << " read finish" << std::endl;

    uint64_t filesize = st.st_size;
    /* set up mmap region */
    truthful_work_load[f] = (char *)mmap(NULL, filesize, PROT_READ, MAP_SHARED, fd, 0);
    if ((truthful_work_load) == MAP_FAILED)
    {
        close(fd);
        printf("Unable to allocate %zu bytes of memory, %s\n", st.st_size,
               strerror(errno));
        exit(EX_CONFIG);
    }
    fsize[f] = filesize;
}

std::string get(size_t f, bool change)
{
    char *work_pointer = nullptr;
    work_pointer = truthful_work_load[f] + offset[f];
    std::string req;
    char c;
    uint32_t cnt = 0;
    while (offset[f] + cnt <= fsize[f])
    {
        c = *(work_pointer);
        if (c == '\n')
            break;
        req += c;
        work_pointer++;
        cnt++;
    }
    if (change)
        offset[f] += req.size() + 1;
    return req;
}

void save_stats_slabs(size_t cnt)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(IP);
    // inet_aton(IP, &addr.sin_addr);
    addr.sin_port = htons(PORT);
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0)
    {
        std::cout << "socket error" << std::endl;
        exit(-1);
    }
    // server address
    if ((connect(server, (struct sockaddr *)&addr, sizeof(struct sockaddr))) < 0)
    {
        std::cout << "Couldn't connect." << std::endl;
        exit(-1);
    }
    
    /*
    char *cmd = "stats slabs\n";
    int len = send(server, cmd, strlen(cmd), 0);
    */
    std::string cmd = "stats slabs\r\n";
    int len = send(server, cmd.c_str(), (u_int)cmd.length(), 0);
    // save log
    std::ofstream outf(file_stats_slabs, std::ofstream::app);
    if (!outf)
    {
        std::cout << "Fail to open the output file!" << std::endl;
        exit(1);
    }
    outf << "request:" << cnt << std::endl;
    int sl = BUFS - 1;
    bool flag = false;
    do // loop receiving
    {
        memset(buf, 0, BUFS);
        len = recv(server, buf, sl, 0);
        if (len > 0)
        {
            outf << buf;
            for (int i = 0; i < len; i++)
                if (buf[i] == 'D')
                    flag = true;
        }
        if (flag)
            break;
    } while (len > 0);
    outf.close();
    close(server);
}

int main(int argc, char *argv[])
{
    // connect server
    memcached_st *memc;
    memcached_return rc;
    memcached_server_st *server;

    memc = memcached_create(NULL);
    server = memcached_server_list_append(NULL, IP, PORT, &rc);

    rc = memcached_server_push(memc, server);
    memcached_server_list_free(server);

    // request
    uint32_t timestamp, expiration = 0, flag = 0;
    std::string op, key;
    size_t key_length, value_length;

    // output
    std::string outpath = file_result;
    std::ofstream outFile(outpath, std::ofstream::out);
    if (!outFile)
    {
        std::cout << "Fail to open the output file!" << std::endl;
        exit(1);
    }

    // read trace(csv)
    size_t total_count = 0;
    std::priority_queue<std::pair<uint32_t, uint32_t>, std::vector<std::pair<uint32_t, uint32_t>>, cmp> pq; // min-heap

    for (int i = 1; i < argc; i++)
    {
        generate(argv[i], i);
        std::string line = get(i, false), tmp;
        std::istringstream sin(line);
        getline(sin, tmp, ',');
        uint32_t real_time = std::stoi(tmp);
        pq.push(std::make_pair(real_time, i));
    }

    timeval start_time, now;
    gettimeofday(&start_time, NULL);

    while (!pq.empty())
    {
        uint32_t fidx = pq.top().second;
        pq.pop();
        std::string line = get(fidx, true);
        // parse the 
        std::istringstream sin(line);
        std::vector<std::string> parts;
        std::string tmp;
        while (getline(sin, tmp, ','))
        {
            parts.push_back(tmp);
        }
        int partscnt = parts.size();

        try
        {
            timestamp = std::stoi(parts[0]);
        }
        catch (std::invalid_argument)
        {
            std::cout << "invalid_argument for timestamp: " << tmp << std::endl;
            std::cout << "in line: " << line << std::endl;
            exit(1);
        }
        if(timestamp > endtime)
            break;

        // key size
        tmp = parts[partscnt - 5];
        try
        {
            key_length = std::stoi(tmp);
        }
        catch (std::invalid_argument)
        {
            std::cout << "invalid_argument for key_length: " << tmp << std::endl;
            std::cout << "in line: " << line << std::endl;
            exit(1);
        }
        // value size
        tmp = parts[partscnt - 4];
        try
        {
            value_length = std::stoi(tmp);
        }
        catch (std::invalid_argument)
        {
            std::cout << "invalid_argument for value_length: " << tmp << std::endl;
            std::cout << "in line: " << line << std::endl;
            exit(1);
        }
        op = parts[partscnt - 2];
        // ttl
        tmp = parts[partscnt - 1];
        try
        {
            expiration = std::stoi(tmp);
        }
        catch (std::invalid_argument)
        {
            std::cout << "invalid_argument for expiration: " << tmp << std::endl;
            std::cout << "in line: " << line << std::endl;
            exit(1);
        }

        key = parts[1];
        if (partscnt > 7) // key contains comma(s)
        {
            partscnt -= 6;
            for (int i = 2; i <= partscnt; i++)
                key += "," + parts[i];
        }

        if (offset[fidx] < fsize[fidx])
        {
            line = get(fidx, false);
            std::istringstream is(line);
            getline(is, tmp, ',');
            uint32_t real_time = std::stoi(tmp);
            pq.push(std::make_pair(real_time, fidx));
        }

        // process the request
        if (op == "get")
        {
            char *result = memcached_get(memc, key.c_str(), key.length(), &value_length, &flag, &rc);
            /*
            if(rc == MEMCACHED_SUCCESS)
                std::cout << "Get value of the key: " << key << " sucessful!" << std::endl;
            else
                std::cout << "No such value for " << key << std::endl;
            */
            if (rc == MEMCACHED_SUCCESS)
                hits[fidx]++;
            else  // need set to simulate the access to the database
            {
                misses[fidx]++;
                std::string v(value_length, '1');
                rc = memcached_set(memc, key.c_str(), key.length(), v.c_str(), v.length(), expiration, (fidx | SET_DB));
            }
            free(result);
        }
        else if (op == "set")
        {
            std::string value(value_length, '1'); // generate value
            rc = memcached_set(memc, key.c_str(), key.length(), value.c_str(), value.length(), expiration, fidx);
        }
        else if (op == "incr")
        {
            uint32_t offset = stoi(std::string(value_length, '1'));
            uint64_t re_val;
            memcached_increment(memc, key.c_str(), key_length, offset, &re_val);
        }
        else if (op == "add")
        {
            std::string value(value_length, '1'); // generate value
            rc = memcached_set(memc, key.c_str(), key.length(), value.c_str(), value.length(), expiration, fidx);
        }
        else if (op == "delete")
        {
            rc = memcached_delete(memc, key.c_str(), key_length, expiration);
            /*
            if (rc == MEMCACHED_SUCCESS)
                std::cout << "delete key: " << key << " sucessful!" << std::endl;
            */
        }

        reqs[fidx]++;
        total_count++;
        if (total_count % 1000000 == 0) // print per million
        {
            gettimeofday(&now, NULL);
            outFile << timestamp << "," << total_count;
            for(int i = 1; i < argc; i++)
                outFile << "," << hits[i] << "," << misses[i];
            outFile << "," << (now.tv_sec - start_time.tv_sec) << std::endl;
            save_stats_slabs(total_count);
        }
    }

    outFile.close();
    // free
    memcached_free(memc);

    return 0;
}
