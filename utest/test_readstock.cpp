#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string>
#include <vector>

std::string readstock(const char * filename, off_t & size)
{
    std::string data;
        
    if (access(filename, F_OK) != 0) {
        return data;
    }

    struct stat sb;
    if (stat(filename, &sb) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    if ((sb.st_mode & S_IFMT) != S_IFREG) {
        return data;
    }

    size = sb.st_size;
    if (size == 0) {
        return data;
    }

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        return data;
    }

    data.resize(size);
    if (read(fd, &data[0], size) != size) {
        close(fd);
        std::string().swap(data);
        return data;
    }

    printf("read file:%s, size:%ld\n", filename, size);

    close(fd);
    return data;
}

struct KLINE
{
    std::string symbol;
    std::string name;
    std::string datetime;
    int32_t open;
    int32_t low;
    int32_t high;
    int32_t close;
    int64_t volume;
    int64_t amount;
};

bool analyse_stock(const std::string & content, std::vector<KLINE> & out)
{
    KLINE kline;

    std::string::size_type nbegin = 0;
    std::string::size_type npos   = 0;
    int cnt = 0;

    while ((npos = content.find("\r\n", nbegin)) != std::string::npos) {
        std::string substr = content.substr(nbegin, npos - nbegin + 1);
        substr[substr.size() - 1] = ',';
        cnt ++;
        //printf("%s\n", substr.c_str());

        std::string::size_type begin_ = 0;
        std::string::size_type npos_  = 0;
        int idx = 0;
        out.resize(out.size() + 1);
        KLINE & kline = out[out.size() - 1];

        while ((npos_ = substr.find(',', begin_)) != std::string::npos) {
            std::string substr_ = substr.substr(begin_, npos_ - begin_ + 1);
            substr_[substr_.size() - 1] = '\0';

            int64_t value = ((int64_t)(atof(substr_.c_str()) * 100000 + 5)) / 10;

            //printf("value:%s\n", substr_.c_str());
            switch (idx) {
                case 0:
                    kline.symbol.swap(substr_);
                    break;
                case 1:
                    kline.name.swap(substr_);
                    break;
                case 2:
                    kline.datetime.swap(substr_);
                    break;
                case 3:
                    kline.open  = value;
                    break;
                case 4:
                    kline.low   = value;
                    break;
                case 5:
                    kline.high  = value;
                    break;
                case 6:
                    kline.close = value;
                    break;
                case 7:
                    kline.volume = value;
                    break;
                case 8:
                    kline.amount = value;
                    break;
                default:
                    break;
            }
            idx ++;

            begin_ = npos_ + 1;
        }
        nbegin = npos + 2;

        //if (cnt == 5)
        //    break;
    }

    printf("total cnt:%d\n", cnt);
    
    return true;
}

void print(KLINE & kline)
{
    printf("KLINE:%s:%s:%s:%d:%d:%d:%d:%ld:%ld:\n", 
        kline.symbol.c_str(),
        kline.name.c_str(),
        kline.datetime.c_str(),
        kline.open,
        kline.low,
        kline.high,
        kline.close,
        kline.volume,
        kline.amount);
}

int main(int argc, char * argv[])
{
    const char * filename = NULL;
    
    if (argc < 2) {
        printf("input stock file!\n");
        exit(0);
    }
    
    filename = argv[1];

    off_t size = 0;
    std::string content = readstock(filename, size);
    if (content.empty()) {
        printf("read failed, check it!(%s)\n", filename);
    } else {
        std::vector<KLINE> klines;
        analyse_stock(content, klines);

        printf("after analyse:\n");
        for (int i = 0; i < klines.size(); i++) {
            print(klines[i]);
        }
    }
    
    return 0;
}

