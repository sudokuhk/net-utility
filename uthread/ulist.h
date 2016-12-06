#ifndef _ULIST_H__
#define _ULIST_H__

#define list_end    (-1)

class ulist
{
public:
    ulist() : next_(list_end)
    {
    }

    ~ulist()
    {
    }

    const int next() const
    {
        return next_;
    }

    void set_next(int idx)
    {
        next_ = idx;
    }
    
private:
    int next_;
};

#endif