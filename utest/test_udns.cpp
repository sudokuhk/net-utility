#include <iostream>
#include <string>
#include <stdlib.h>
#include <stdio.h>

//#define UUDP_STREAM

#include <unetwork/udnsresolver.h>
#include <unetwork/uschedule.h>

class mytask : public utask
{
public:
    mytask(uschedule & sche, std::string & host, int timeout) 
        : sched_(sche)
        , resolver(sche, timeout)
        , host_(host)
    {
    }

    virtual ~mytask() 
    {
    }
    
    virtual void run()
    {
        std::vector<std::string> res = resolver.query(host_);
        printf("result[%lu]\n", res.size());
        for (size_t i = 0; i < res.size(); i++) {
            printf("%s\n", res[i].c_str());
        }
        printf("---------\n");
        
        res = resolver.query(host_);
        printf("result[%lu]\n", res.size());
        for (size_t i = 0; i < res.size(); i++) {
            printf("%s\n", res[i].c_str());
        }
        printf("---------\n");

        sched_.sleep(6 * 1000);
        
        res = resolver.query(host_);
        printf("result[%lu]\n", res.size());
        for (size_t i = 0; i < res.size(); i++) {
            printf("%s\n", res[i].c_str());
        }
        printf("---------\n");

        sched_.sleep(4 * 1000);
        res = resolver.query(host_);
        printf("result[%lu]\n", res.size());
        for (size_t i = 0; i < res.size(); i++) {
            printf("%s\n", res[i].c_str());
        }
        printf("---------\n");
    }

private:
    uschedule & sched_;
    udnsresolver resolver;
    std::string host_;
};

int main(int argc, char * argv[])
{
    std::string host;
    int timeo = 5;
    char ch;
    
    while((ch = getopt(argc, argv, "h:t:")) != -1) {
        switch(ch) {
            case 'h': 
                host = optarg;
                break;
            case 't':
                timeo = atoi(optarg);
                if (timeo == 0) {
                    timeo = 5;
                }
                break;
            default:
                fprintf(stderr, "unknown arguments. (%c)", ch);
                break;
        }
    }

    if (host.empty()) {
        printf("input domain name to resolver!\n");
        exit(0);
    }
    
    uschedule schedule(1024, 100, true, NULL);
    
    mytask task(schedule, host, timeo * 1000);
    schedule.add_task(&task);
    schedule.run();

    return 0;
}
