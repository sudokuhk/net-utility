#include "uhttp/uurl.h"
#include "uhttp/uhttp.h"
#include "unetwork/uschedule.h"
#include "unetwork/utcpsocket.h"
#include <sys/stat.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

#define CONTENT_STR0 "intgInfoId=I100020879&intgInfo=%7B%22intgArrs%22%3A%5B%7B%22intgId%22%3A%22Q100078724%22%2C%22type%22%3A1%2C%22optionValue%22%3A%22"
#define CONTENT_STR1 "%22%7D%2C%7B%22intgId%22%3A%22Q100078732%22%2C%22type%22%3A1%2C%22optionValue%22%3A%22"
#define CONTENT_STR2 "%22%7D%5D%7D&captchCode=TXPG&timeStamp=1523845751426229124&channel=1&userName="
#define CONTENT_STR3 "&contact="
#define CONTENT_STR4 "&nonGzip=1"

char aplpa[27] = "abcdefghijklmnopqrstuvwxyz";

class uhttpclient 
    : public utask
    , public uhttphandler
{
public:
    uhttpclient(uschedule & schedule, uurl & url)
        : utask()
        , schedule_(schedule)
        , url_(url)
    {
    }
    
    virtual ~uhttpclient()
    {
    }

    void do_vote(uhttp & http, int id)
    {
        uuri uri("/learn/app/clientapi/question/saveIntgInfo.do");
        uhttprequest request;
        request.set_version(uhttp_version_1_1);
        request.set_method(uhttp_method_post);
        request.set_uri(uri);
        
        request.set_header("HOST", "mlearning.pingan.com.cn");
        request.set_header("User-Agent", "Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36");
        request.set_header("Content-Type", "application/x-www-form-urlencoded; charset=UTF-8");
        request.set_header("Accept", "application/json");
        request.set_header("Connection", "keep-alive");
        request.set_header("Referer", "http://www.zhi-niao.com/app/investigate/index.html?pageId=I100020879&share=1&umId=6335086FE5D39576E05490E2BADB9AA4&sid=E9A1231CF28E44658DD26427FCC43805&title=2018%25E5%25B9%25B4%25E7%25AC%25AC%25E4%25B8%2580%25E5%25AD%25A3%25E5%25BA%25A6%25E6%2596%25B0%25E4%25BA%25BA%25E4%25BA%25BA%25E6%25B0%2594%25E8%25AF%2584%25E9%2580%2589%25E2%2580%2594%25E2%2580%2594%25E6%2598%25A5%25E6%259A%2596%25E8%258A%25B1%25E5%25BC%2580%25E5%25AD%25A3&imgUrl=http%253A%252F%252Fmlearn.pingan.com.cn%252Flearn%252Fapp%252Fnew%252Ftemp%252Fimage%252F2018%252F04%252F11%252F026D7E5EE9B0449BB1C146F08CD3D093%252F4bc92e146d6645b38af8ea039d05d170.png&intro=%E7%9F%A5%E9%B8%9F%E9%97%AE%E5%8D%B7");
        request.set_header("Accept-Encoding", "gzip, deflate");
        request.set_header("Accept-Language", "zh-CN,zh;q=0.8,en;q=0.6");
        request.set_header("DNT", "1");
        request.set_header("Origin", "http://www.zhi-niao.com");
        
        const char * q1[9] = {
            "152343835673374857017",
            "152343835673440140165",
            "152343835673550165826",
            "152343835673623822931",
            "152343835673780099693",
            "152343835673880432919",
            "152343835673954109882",
            "152343835674045529279",
            "152343835674111843896",
        };
        
        const char * q2[6] = {
            "1523438498986438335",
            "152343849898746045763",
            "152343849898826278976",
            "152343849898976209959",
            "152343849899020661456",
            "152343849899141818452",
        };
        
        srand(time(NULL));
        static unsigned long long telephone = 13510001000 + random() % 10000000;
        char buf[4096];
        int len = snprintf(buf, 4096, "%llu", telephone++);
        static int q1idx = 0;
        if (q1idx == 9) {
            q1idx = 0;
        }
        
        char votename[10];
        int namelen = random() % 5 + 5;
        for (int i = 0; i < namelen; i++) {
            votename[i] = aplpa[random() % 26];
        }
        votename[namelen] = '\0';
        
        std::string content;
        content.append(CONTENT_STR0);
        content.append(q1[q1idx ++]);
        content.append(CONTENT_STR1);
        content.append(q2[id]);
        content.append(CONTENT_STR2);
        content.append(votename);
        content.append(CONTENT_STR3);
        content.append(buf);
        content.append(CONTENT_STR4);
        request.set_content(content.c_str(), content.size());
        std::cout << content << std::endl;
            
        http.send_request(request);

        uhttpresponse response;
        http.recv_response(response);
        
        return;
    }

    void vote(int id) {
        int fd = -1;
        utcpsocket * socket = new utcpsocket(1024 * 1024, fd, &schedule_);
        socket->connect(url_.host().c_str(), url_.port());
        uhttp * http = new uhttp(*socket, *this, false);
        socket->set_timeout(60 * 1000);
        
        do_vote(*http, id);
        
        delete http;
        delete socket;
    }
    
    void run()
    {
        int count = 0;
        srand(time(NULL));
        int odd = 0;
        int array[6] = {0};
        
        for (int i = 0; i < 10000; i++) {
            int idx = random() % 6;
            vote(idx);
            array[idx] ++;
            
            if (idx % 6 == 0) {
                count ++;
                sleep(1);
                continue;
            }
            odd ++;
            usleep(100 * 1000);
            
            if (odd % 2 == 0) {
                vote(0);
                array[0] ++;
                count ++;
                sleep(1);
            }
            
            if (count > 20) {
                break;
            }
        }
        
        for (int i = 0; i < 6; i++) {
            printf("array[%d] = %d\n", i, array[i]);
        }
        
        delete this;
    }
    
    int onhttp(const uhttprequest & request, uhttpresponse & response, int)
    {
        return 0;
    }

private:
    uschedule & schedule_;
    uurl url_;
};


int main(int argc, char * argv[])
{
    const char * addr_url = "http://183.63.51.85:80";

    uurl url(addr_url);
    if (url.protocol()== uprotocol_unknown) {
        printf("invalid protocol:<%s>!\n", addr_url);
        return 0;
    }

    if (url.protocol() == uprotocol_http) {
        printf("begin run!\n");
        uschedule schedule(8 * 1024, 100);
        uhttpclient * client = new uhttpclient(schedule, url);

        schedule.add_task(client);
        schedule.run();       
    } else {
        printf("unsupport protocol!\n");
    }
    
    return 0;
}
