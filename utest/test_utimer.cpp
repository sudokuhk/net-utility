#include "unetwork/utimer.h"

#include <stdio.h>
#include <stdlib.h>

class cnode : public utimer
{
public:
    cnode(int i, int time) : i_(i), t_(time) {}

    virtual ~cnode() {}

    void print() { printf("i:%3d, t:%3d\n", i_, t_); }
private:
    int i_;
    int t_;
};

int main(int argc, char * argv[])
{
    utimermgr timer;
    
    for (int i = 0; i < 10; i++) {
        int abstime = rand() % 100;
        if (abstime < 0)
            abstime = -abstime;
        printf("add, abstime:%3d, idx:%d\n", abstime, i);

        cnode * node = new cnode(i,abstime);
            
        timer.add(abstime, node);
    }

    utimer * p = NULL;
    while ((p = timer.pop()) != NULL) {
        cnode * node = (cnode *)p;
        node->print();
        delete p;
    }

    return 0;
}
